/**
 * @file client.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client window functions.
 *
 */

#include <algorithm>
#include "jwm.h"
#include "client.h"
#include "clientlist.h"
#include "icon.h"
#include "group.h"
#include "tray.h"
#include "confirm.h"
#include "cursor.h"
#include "taskbar.h"
#include "screen.h"
#include "pager.h"
#include "color.h"
#include "place.h"
#include "event.h"
#include "settings.h"
#include "timing.h"
#include "grab.h"
#include "hint.h"
#include "misc.h"
#include "DesktopEnvironment.h"
#include "move.h"
#include "font.h"

#include <X11/Xlibint.h>

ClientNode *ClientNode::activeClient;

unsigned int ClientNode::clientCount;

std::vector<ClientNode*> ClientNode::nodes;

/** Load windows that are already mapped. */
void ClientNode::StartupClients(void) {

  XWindowAttributes attr;
  Window rootReturn, parentReturn, *childrenReturn;
  unsigned int childrenCount;
  unsigned int x;

  clientCount = 0;
  activeClient = NULL;
  currentDesktop = 0;

  /* Clear out the client lists. */
  ClientList::Initialize();

  /* Query client windows. */
  JXQueryTree(display, rootWindow, &rootReturn, &parentReturn, &childrenReturn,
      &childrenCount);

  /* Add each client. */
  for (x = 0; x < childrenCount; x++) {
    if (JXGetWindowAttributes(display, childrenReturn[x], &attr)) {
      if (attr.override_redirect == False && attr.map_state == IsViewable) {
        Create(childrenReturn[x], 1, 1);
      }
    }
  }

  JXFree(childrenReturn);

  LoadFocus();

  _RequireTaskUpdate();
  _RequirePagerUpdate();

  const unsigned titleHeight = Border::GetTitleHeight();
  int count;
//	int x;

  count = settings.desktopCount * Screens::GetScreenCount();
  cascadeOffsets = new int[count];

  for (x = 0; x < count; x++) {
    cascadeOffsets[x] = settings.borderWidth + titleHeight;
  }

}

ClientNode* ClientNode::Create(Window w, char alreadyMapped, char notOwner) {
  ClientNode *node = new ClientNode(w, alreadyMapped, notOwner);
  nodes.push_back(node);
  return node;
}

/** Place a client on the screen. */
void ClientNode::PlaceClient(ClientNode *np, char alreadyMapped) {

  BoundingBox box;
  const ScreenType *sp;

  if (alreadyMapped || (np->getState()->getStatus() & STAT_POSITION)
      || (!(np->getState()->getStatus() & STAT_PIGNORE)
          && (np->getSizeFlags() & (PPosition | USPosition)))) {

    np->GravitateClient(0);
    if (!alreadyMapped) {
      np->ConstrainSize();
      np->ConstrainPosition();
    }

  } else {

    sp = Screens::GetMouseScreen();
    GetScreenBounds(sp, &box);
    SubtractTrayBounds(&box, np->getState()->getLayer());
    SubtractStrutBounds(&box, np);

    /* If tiled is specified, first attempt to use tiled placement. */
    if (np->getState()->getStatus() & STAT_TILED) {
      if (np->TileClient(&box)) {
        return;
      }
    }

    /* Either tiled placement failed or was not specified. */
    if (np->getState()->getStatus() & STAT_CENTERED) {
      np->CenterClient(&box);
    } else {
      np->CascadeClient(&box);
    }

  }

}

/** Cascade placement. */
void ClientNode::CascadeClient(const BoundingBox *box) {
  const ScreenType *sp;
  const unsigned titleHeight = Border::GetTitleHeight();
  int north, south, east, west;
  int cascadeIndex;
  char overflow;

  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
  sp = Screens::GetMouseScreen();
  cascadeIndex = sp->index * settings.desktopCount + currentDesktop;

  /* Set the cascaded location. */
  this->setX(box->x + west + cascadeOffsets[cascadeIndex]);
  this->setY(box->y + north + cascadeOffsets[cascadeIndex]);
  cascadeOffsets[cascadeIndex] += settings.borderWidth + titleHeight;

  /* Check for cascade overflow. */
  overflow = 0;
  if (this->getX() + this->getWidth() - box->x > box->width) {
    overflow = 1;
  } else if (this->getY() + this->getHeight() - box->y > box->height) {
    overflow = 1;
  }

  if (overflow) {
    cascadeOffsets[cascadeIndex] = settings.borderWidth + titleHeight;
    this->setX(box->x + west + cascadeOffsets[cascadeIndex]);
    this->setY(box->y + north + cascadeOffsets[cascadeIndex]);

    /* Check for client overflow and update cascade position. */
    if (this->getX() + this->getWidth() - box->x > box->width) {
      this->setX(box->x + west);
    } else if (this->getY() + this->getHeight() - box->y > box->height) {
      this->setY(box->y + north);
    } else {
      cascadeOffsets[cascadeIndex] += settings.borderWidth + titleHeight;
    }
  }

  this->ConstrainSize();
  this->ConstrainPosition();

}

/** Release client windows. */
void ClientNode::ShutdownClients(void) {

  ClientList::Shutdown();

  Strut *sp;

  Release(cascadeOffsets);

  while (struts) {
    sp = struts->next;
    Release(struts);
    struts = sp;
  }

}

/** Set the focus to the window currently under the mouse pointer. */
void ClientNode::LoadFocus(void) {

  ClientNode *np;
  Window rootReturn, childReturn;
  int rootx, rooty;
  int winx, winy;
  unsigned int mask;

  JXQueryPointer(display, rootWindow, &rootReturn, &childReturn, &rootx, &rooty,
      &winx, &winy, &mask);

  np = FindClient(childReturn);
  if (np) {
    np->FocusClient();
  }

}

ClientNode::~ClientNode() {

  this->state.setDelete();
  SendClientMessage(this->window, ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW);
  ColormapNode *cp;

  Assert(this->window != None);

  /* Remove this client from the client list */
  ClientList::RemoveFrom(this);
  clientCount -= 1;
  XDeleteContext(display, this->window, clientContext);
  if (this->parent != None) {
    XDeleteContext(display, this->parent, frameContext);
  }

  if (this->state.getStatus() & STAT_URGENT) {
    _UnregisterCallback(SignalUrgent, this);
  }

  /* Make sure this client isn't active */
  if (activeClient == this && !shouldExit) {
    ClientList::FocusNextStacked(this);
  }
  if (activeClient == this) {

    /* Must be the last client. */
    Hints::SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, None);
    activeClient = NULL;
    JXSetInputFocus(display, rootWindow, RevertToParent, eventTime);

  }

  /* If the window manager is exiting (ie, not the client), then
   * reparent etc. */
  if (shouldExit && !(this->state.getStatus() & STAT_WMDIALOG)) {
    if (this->state.getMaxFlags()) {
      this->x = this->oldx;
      this->y = this->oldy;
      this->width = this->oldWidth;
      this->height = this->oldHeight;
      JXMoveResizeWindow(display, this->window, this->x, this->y, this->width,
          this->height);
    }
    this->GravitateClient(1);
    if ((this->state.getStatus() & STAT_HIDDEN)
        || (!(this->state.getStatus() & STAT_MAPPED)
            && (this->state.getStatus() & (STAT_MINIMIZED | STAT_SHADED)))) {
      JXMapWindow(display, this->window);
    }
    JXUngrabButton(display, AnyButton, AnyModifier, this->window);
    JXReparentWindow(display, this->window, rootWindow, this->x, this->y);
    JXRemoveFromSaveSet(display, this->window);
  }

  /* Destroy the parent */
  if (this->parent) {
    JXDestroyWindow(display, this->parent);
  }

  if (this->instanceName) {
    JXFree(this->instanceName);
  }
  if (this->className) {
    JXFree(this->className);
  }

  if(this->window != window) {

  }

  TaskBar::RemoveClientFromTaskBar(this);
  Places::RemoveClientStrut(this);

  while (this->colormaps) {
    cp = this->colormaps->next;
    Release(this->colormaps);
    this->colormaps = cp;
  }

  Icons::DestroyIcon(this->getIcon());

  _RequireRestack();

  if (this->name) {
    delete[] this->name;
  }
  this->name = NULL;

}

/** Add a window to management. */
ClientNode::ClientNode(Window w, char alreadyMapped, char notOwner) :
    oldx(0), oldy(0), oldWidth(0), oldHeight(0), yinc(0), minWidth(0), maxWidth(
        0), minHeight(0), maxHeight(0), gravity(0), controller(0), instanceName(
        0), baseHeight(0), baseWidth(0), colormaps(), icon(0), sizeFlags(0), className(
        0), xinc(0), name(0) {

  XWindowAttributes attr;

  Assert(w != None);

  /* Get window attributes. */
  if (JXGetWindowAttributes(display, w, &attr) == 0) {
    return;
  }

  /* Determine if we should care about this window. */
  if (attr.override_redirect == True) {
    return;
  }
  if (attr.c_class == InputOnly) {
    return;
  }

  /* Prepare a client node for this window. */
  this->window = w;
  this->parent = None;
  this->owner = None;
  this->state.setDesktop(currentDesktop);

  this->x = attr.x;
  this->y = attr.y;
  this->width = attr.width;
  this->height = attr.height;
  this->cmap = attr.colormap;
  this->state.clearStatus();
  this->state.clearMaxFlags();
  this->state.resetLayer();
  this->state.setDefaultLayer(LAYER_NORMAL);

  this->state.resetBorder();
  this->mouseContext = MC_NONE;

  Hints::ReadClientInfo(this, alreadyMapped);

  if (!notOwner) {
    this->state.clearBorder();
    this->state.setBorderOutline();
    this->state.setBorderTitle();
    this->state.setBorderMove();
    this->state.setWMDialogStatus();
    this->state.setSticky();
    this->state.setLayer(LAYER_ABOVE); //FIXME: can encapsulate this easily
    this->state.setDefaultLayer(LAYER_ABOVE); //FIXME: can encapsulate this easily
  }

  Groups::ApplyGroups(this);
  if (this->icon == NULL) {
    Icons::LoadIcon(this);
  }

  /* We now know the layer, so insert */
  ClientList::InsertAt(this);

  Cursors::SetDefaultCursor(this->window);

  if (notOwner) {
    XSetWindowAttributes sattr;
    JXAddToSaveSet(display, this->window);
    sattr.event_mask = EnterWindowMask | ColormapChangeMask | PropertyChangeMask
        | KeyReleaseMask | StructureNotifyMask;
    sattr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask
        | PointerMotionMask | KeyPressMask | KeyReleaseMask;
    JXChangeWindowAttributes(display, this->window,
        CWEventMask | CWDontPropagate, &sattr);
  }
  JXGrabButton(display, AnyButton, AnyModifier, this->window, True,
      ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);

  Places::PlaceClient(this, alreadyMapped);
  this->ReparentClient();
  XSaveContext(display, this->window, clientContext, (char*) this);

  if (this->state.getStatus() & STAT_MAPPED) {
    JXMapWindow(display, this->window);
  }

  clientCount += 1;

  if (!alreadyMapped) {
    this->RaiseClient();
  }

  if (this->state.getStatus() & STAT_OPACITY) {
    this->SetOpacity(this->state.getOpacity(), 1);
  } else {
    this->SetOpacity(settings.inactiveClientOpacity, 1);
  }
  if (this->state.getStatus() & STAT_STICKY) {
    Hints::SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP, ~0UL);
  } else {
    Hints::SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP,
        this->state.getDesktop());
  }

  /* Shade the client if requested. */
  if (this->state.getStatus() & STAT_SHADED) {
    this->state.setNoShaded();
    this->ShadeClient();
  }

  /* Minimize the client if requested. */
  if (this->state.getStatus() & STAT_MINIMIZED) {
    this->state.setNoMinimized();
    this->MinimizeClient(0);
  }

  /* Maximize the client if requested. */
  if (this->state.getMaxFlags()) {
    const MaxFlags flags = this->state.getMaxFlags();
    this->state.setMaxFlags(MAX_NONE);
    this->MaximizeClient(flags);
  }

  if (this->state.getStatus() & STAT_URGENT) {
    _RegisterCallback(URGENCY_DELAY, SignalUrgent, this);
  }

  /* Update task bars. */
  TaskBar::AddClientToTaskBar(this);

  /* Make sure we're still in sync */
  Hints::WriteState(this);
  this->SendConfigureEvent();

  /* Hide the client if we're not on the right desktop. */
  if (this->state.getDesktop() != currentDesktop
      && !(this->state.getStatus() & STAT_STICKY)) {
    this->HideClient();
  }

  Places::ReadClientStrut(this);

  /* Focus transients if their parent has focus. */
  if (this->owner != None) {
    if (activeClient && this->owner == activeClient->window) {
      this->FocusClient();
    }
  }

  /* Make the client fullscreen if requested. */
  if (this->state.getStatus() & STAT_FULLSCREEN) {
    this->state.setNoFullscreen();
    this->SetClientFullScreen(1);
  }
  Border::ResetBorder(this);
}

/** Minimize a client window and all of its transients. */
void ClientNode::MinimizeClient(char lower) {
  this->MinimizeTransients(lower);
  _RequireRestack();
  _RequireTaskUpdate();
}

/** Minimize all transients as well as the specified client. */
void ClientNode::MinimizeTransients(char lower) {

  /* Unmap the window and update its state. */
  if (this->state.getStatus() & (STAT_MAPPED | STAT_SHADED)) {
    this->UnmapClient();
    if (this->parent != None) {
      JXUnmapWindow(display, this->parent);
    }
  }
  this->state.setMinimized();

  /* Minimize transient windows. */
  ClientList::MinimizeTransientWindows(this, lower);

  Hints::WriteState(this);

}

void ClientNode::saveBounds() {
  this->oldx = this->x;
  this->oldy = this->y;
  this->oldWidth = this->width;
  this->oldHeight = this->height;
}

/** Determine which way to move the client for the border. */
void ClientNode::GetGravityDelta(int gravity, int *x, int *y) {
  int north, south, east, west;
  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
  switch (gravity) {
  case NorthWestGravity:
    *y = -north;
    *x = -west;
    break;
  case NorthGravity:
    *y = -north;
    *x = (west - east) / 2;
    break;
  case NorthEastGravity:
    *y = -north;
    *x = west;
    break;
  case WestGravity:
    *x = -west;
    *y = (north - south) / 2;
    break;
  case CenterGravity:
    *y = (north - south) / 2;
    *x = (west - east) / 2;
    break;
  case EastGravity:
    *x = west;
    *y = (north - south) / 2;
    break;
  case SouthWestGravity:
    *y = south;
    *x = -west;
    break;
  case SouthGravity:
    *x = (west - east) / 2;
    *y = south;
    break;
  case SouthEastGravity:
    *y = south;
    *x = west;
    break;
  default: /* Static */
    *x = 0;
    *y = 0;
    break;
  }
}

/** Shade a client. */
void ClientNode::ShadeClient() {
  if ((this->state.getStatus() & (STAT_SHADED | STAT_FULLSCREEN))
      || !(this->state.getBorder() & BORDER_SHADE)) {
    return;
  }

  this->UnmapClient();
  this->state.setShaded();

  Hints::WriteState(this);
  Border::ResetBorder(this);
  _RequirePagerUpdate();

}

/** Unshade a client. */
void ClientNode::UnshadeClient() {
  if (!(this->state.getStatus() & STAT_SHADED)) {
    return;
  }

  if (!(this->state.getStatus() & (STAT_MINIMIZED | STAT_SDESKTOP))) {
    JXMapWindow(display, this->window);
    this->state.setMapped();
  }
  this->state.setNoShaded();

  Hints::WriteState(this);
  Border::ResetBorder(this);
  RefocusClient();
  _RequirePagerUpdate();

}

/** Set a client's state to withdrawn. */
void ClientNode::SetClientWithdrawn() {
  if (activeClient == this) {
    activeClient = NULL;
    this->state.setNotActive();
    ClientList::FocusNextStacked(this);
  }

  if (this->state.getStatus() & STAT_MAPPED) {
    this->UnmapClient();
    if (this->parent != None) {
      JXUnmapWindow(display, this->parent);
    }
  } else if (this->state.getStatus() & STAT_SHADED) {
    if (!(this->state.getStatus() & STAT_MINIMIZED)) {
      if (this->parent != None) {
        JXUnmapWindow(display, this->parent);
      }
    }
  }

  this->state.setNoShaded();
  this->state.setNoMinimized();
  this->state.setNoSDesktop();

  Hints::WriteState(this);
  _RequireTaskUpdate();
  _RequirePagerUpdate();

}

/** Restore a window with its transients (helper method). */
void ClientNode::RestoreTransients(char raise) {
  int x;

  /* Make sure this window is on the current desktop. */
  this->SetClientDesktop(currentDesktop);

  /* Restore this window. */
  if (!(this->state.getStatus() & STAT_MAPPED)) {
    if (this->state.getStatus() & STAT_SHADED) {
      if (this->parent != None) {
        JXMapWindow(display, this->parent);
      }
    } else {
      JXMapWindow(display, this->window);
      if (this->parent != None) {
        JXMapWindow(display, this->parent);
      }
      this->state.setMapped();
    }
  }

  this->state.setNoMinimized();
  this->state.setNoSDesktop();

  /* Restore transient windows. */
  //ClientList::RestoreTransientWindows(this->window, raise);
  std::vector<ClientNode*> children = ClientList::GetChildren(this->window);
  for (int i = 0; i < children.size(); ++i) {
    ClientNode *tp = children[i];
    if (tp->state.getStatus() & STAT_MINIMIZED) {
      tp->RestoreTransients(raise);
    }

  }

  if (raise) {
    this->FocusClient();
    this->RaiseClient();
  }
  Hints::WriteState(this);

}

/** Restore a client window and its transients. */
void ClientNode::RestoreClient(char raise) {
  if ((this->state.getStatus() & STAT_FIXED)
      && !(this->state.getStatus() & STAT_STICKY)) {
    DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(
        this->state.getDesktop());
  }
  this->RestoreTransients(raise);
  _RequireRestack();
  _RequireTaskUpdate();
}

/** Update window state information. */
void ClientNode::_UpdateState() {
  const char alreadyMapped =
      (this->getState()->getStatus() & STAT_MAPPED) ? 1 : 0;
  const char active = (this->getState()->getStatus() & STAT_ACTIVE) ? 1 : 0;

  /* Remove from the layer list. */
  ClientList::RemoveFrom(this);

  /* Read the state (and new layer). */
  if (this->getState()->getStatus() & STAT_URGENT) {
    _UnregisterCallback(ClientNode::SignalUrgent, this);
  }
  this->state = Hints::ReadWindowState(this->getWindow(), alreadyMapped);
  if (this->getState()->getStatus() & STAT_URGENT) {
    _RegisterCallback(URGENCY_DELAY, ClientNode::SignalUrgent, this);
  }

  /* We don't handle mapping the window, so restore its mapped state. */
  if (!alreadyMapped) {
    this->state.resetMappedState();
  }

  /* Add to the layer list. */
  ClientList::InsertAt(this);

  if (active) {
    this->FocusClient();
  }

}

void ClientNode::UpdateWindowState(char alreadyMapped) {
  /* Read the window state. */
  this->state = Hints::ReadWindowState(this->window, alreadyMapped);
  if (this->minWidth == this->maxWidth && this->minHeight == this->maxHeight) {
    this->state.setNoBorderResize();
    this->state.setNoBorderMax();
    if (this->minWidth * this->xinc >= rootWidth
        && this->minHeight * this->yinc >= rootHeight) {
      this->state.setFullscreen();
    }
  }

  /* Make sure this client is on at least as high of a layer
   * as its owner. */
  ClientNode *pp;
  if (this->owner != None) {
    pp = FindClientByWindow(this->owner);
    if (pp) {
      this->state.setLayer(Max(pp->state.getLayer(), this->state.getLayer()));
    }
  }
}

/** Set the client layer. This will affect transients. */
void ClientNode::SetClientLayer(unsigned int layer) {
  Assert(layer <= LAST_LAYER);

  if (this->state.getLayer() != layer) {
    ClientNode *tp;
    std::vector<ClientNode*> children = ClientList::GetChildren(this->window);

    for (int i = 0; i < children.size(); ++i) {
      tp = children[i];
      if (tp == this || tp->owner == this->window) {
        ClientList::ChangeLayer(tp, layer);
      }
    }

    ClientList::ChangeLayer(this, layer);
    _RequireRestack();
  }
}

/** Set a client's sticky.getStatus(). This will update transients. */
void ClientNode::SetClientSticky(char isSticky) {

  bool old = false;

  /* Get the old sticky.getStatus(). */
  if (this->state.getStatus() & STAT_STICKY) {
    old = true;
  }

  if (isSticky && !old) {

    /* Change from non-sticky to sticky. */
    std::vector<ClientNode*> children = ClientList::GetChildren(this->window);

    ClientNode *tp;
    for (int i = 0; i < children.size(); ++i) {
      tp = children[i];
      tp->state.setSticky();
      Hints::SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, ~0UL);
      Hints::WriteState(tp);
    }

    this->state.setSticky();
    Hints::SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP, ~0UL);
    Hints::WriteState(this);

  } else if (!isSticky && old) {

    /* Change from sticky to non-sticky. */
    std::vector<ClientNode*> children = ClientList::GetChildren(this->window);

    ClientNode *tp;
    for (int i = 0; i < children.size(); ++i) {
      tp = children[i];
      if (tp == this || tp->owner == this->window) {
        tp->state.setNoSticky();
        Hints::WriteState(tp);
      }
    }

    this->state.setNoSticky();
    Hints::WriteState(this);

    /* Since this client is no longer sticky, we need to assign
     * a desktop. Here we use the current desktop.
     * Note that SetClientDesktop updates transients (which is good).
     */
    this->SetClientDesktop(currentDesktop);

  }

}

/** Set a client's desktop. This will update transients. */
void ClientNode::SetClientDesktop(unsigned int desktop) {

  if (JUNLIKELY(desktop >= settings.desktopCount)) {
    return;
  }

  if (!(this->state.getStatus() & STAT_STICKY)) {
    ClientNode *tp;

    std::vector<ClientNode*> all = ClientList::GetSelfAndChildren(this);
    for (int i = 0; i < all.size(); ++i) {
      tp = all[i];
      tp->state.setDesktop(desktop);

      if (desktop == currentDesktop) {
        tp->ShowClient();
      } else {
        tp->HideClient();
      }

      Hints::SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP,
          tp->state.getDesktop());

    }
    _RequirePagerUpdate();
    _RequireTaskUpdate();
  }

}

/** Hide a client. This will not update transients. */
void ClientNode::HideClient() {
  if (!(this->state.getStatus() & STAT_HIDDEN)) {
    if (activeClient == this) {
      activeClient = NULL;
    }
    this->state.setHidden();
    if (this->state.getStatus() & (STAT_MAPPED | STAT_SHADED)) {
      if (this->parent != None) {
        JXUnmapWindow(display, this->parent);
      } else {
        JXUnmapWindow(display, this->window);
      }
    }
    //this->state.clearStatus();
    this->state.setSDesktopStatus();
    this->state.setMapped();
    this->state.setShaded();
  }
}

/** Show a hidden client. This will not update transients. */
void ClientNode::ShowClient() {
  if (this->state.getStatus() & STAT_HIDDEN) {
    this->state.setNotHidden();
    if (this->state.getStatus() & (STAT_MAPPED | STAT_SHADED)) {
      if (!(this->state.getStatus() & STAT_MINIMIZED)) {
        if (this->parent != None) {
          JXMapWindow(display, this->parent);
        } else {
          JXMapWindow(display, this->window);
        }
        if (this->state.getStatus() & STAT_ACTIVE) {
          this->FocusClient();
        }
      }
    }
  }
}

/** Maximize a client window. */
void ClientNode::MaximizeClient(MaxFlags flags) {

  /* Don't allow maximization of full-screen clients. */
  if (this->state.getStatus() & STAT_FULLSCREEN) {
    return;
  }
  if (!(this->state.getBorder() & BORDER_MAX)) {
    return;
  }

  if (this->state.getStatus() & STAT_SHADED) {
    this->UnshadeClient();
  }

  if (this->state.getStatus() & STAT_MINIMIZED) {
    this->RestoreClient(1);
  }

  this->RaiseClient();
  this->FocusClient();
  if (this->state.getMaxFlags()) {
    /* Undo existing maximization. */
    this->x = this->oldx;
    this->y = this->oldy;
    this->width = this->oldWidth;
    this->height = this->oldHeight;
    this->state.setMaxFlags(MAX_NONE);
  }
  if (flags != MAX_NONE) {
    /* Maximize if requested. */
    this->PlaceMaximizedClient(flags);
  }

  Hints::WriteState(this);
  Border::ResetBorder(this);
  Border::DrawBorder(this);
  this->SendConfigureEvent();
  _RequirePagerUpdate();

}

void ClientNode::clearController() {
  this->controller = NULL;
}

/** Maximize a client using its default maximize settings. */
void ClientNode::MaximizeClientDefault() {

  MaxFlags flags = MAX_NONE;

  if (this->state.getMaxFlags() == MAX_NONE) {
    if (this->state.getBorder() & BORDER_MAX_H) {
      flags |= MAX_HORIZ;
    }
    if (this->state.getBorder() & BORDER_MAX_V) {
      flags |= MAX_VERT;
    }
  }

  this->MaximizeClient(flags);

}
char ClientNode::TileClient(const BoundingBox *box) {

  int layer;
  int north, south, east, west;
  int i, j;
  int count;
  int *xs;
  int *ys;
  int leastOverlap;
  int bestx, besty;

  /* Count insertion points, including bounding box edges. */
  count = 2;
  std::vector<ClientNode*> insertionPoints =
      ClientList::GetMappedDesktopClients();
  for (int i = 0; i < insertionPoints.size(); ++i) {
    ClientNode *ip = insertionPoints[i];
    if (ip == this) {
      continue;
    }
    count += 2;
  }

  /* Allocate space for the points. */
  xs = AllocateStack(sizeof(int) * count);
  ys = AllocateStack(sizeof(int) * count);

  /* Insert points. */
  xs[0] = box->x;
  ys[0] = box->y;
  count = 1;

  const ClientNode *tp = NULL;
  for (int i = 0; i < insertionPoints.size(); ++i) {
    ClientNode *tp = insertionPoints[i];
    if (tp == this) {
      continue;
    }
    Border::GetBorderSize(tp->getState(), &north, &south, &east, &west);
    xs[count + 0] = tp->getX() - west;
    xs[count + 1] = tp->getX() + tp->getWidth() + east;
    ys[count + 0] = tp->getY() - north;
    ys[count + 1] = tp->getY() + tp->getHeight() + south;
    count += 2;
  }

  /* Try placing at lower right edge of box, too. */
  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
  xs[count] = box->x + box->width - this->getWidth() - east - west;
  ys[count] = box->y + box->height - this->getHeight() - north - south;
  count += 1;

  /* Sort the points. */
  qsort(xs, count, sizeof(int), Places::IntComparator);
  qsort(ys, count, sizeof(int), Places::IntComparator);

  /* Try all possible positions. */
  leastOverlap = INT_MAX;
  bestx = xs[0];
  besty = ys[0];
  for (i = 0; i < count; i++) {
    for (j = 0; j < count; j++) {
      const int overlap = this->TryTileClient(box, xs[i], ys[j]);
      if (overlap < leastOverlap) {
        leastOverlap = overlap;
        bestx = xs[i];
        besty = ys[j];
        if (overlap == 0) {
          break;
        }
      }
    }
  }

  ReleaseStack(xs);
  ReleaseStack(ys);

  if (leastOverlap < INT_MAX) {
    /* Set the client position. */
    Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
    this->setX(bestx + west);
    this->setY(besty + north);
    this->ConstrainSize();
    this->ConstrainPosition();
    return 1;
  } else {
    /* Tiled placement failed. */
    return 0;
  }
}

/** Place a maximized client on the screen. */
void ClientNode::PlaceMaximizedClient(MaxFlags flags) {
  BoundingBox box;
  const ScreenType *sp;
  int north, south, east, west;

  this->saveBounds();
  this->state.setMaxFlags(flags);

  ClientState newState;
  memcpy(&newState, this->getState(), sizeof(newState));
  Border::GetBorderSize(&newState, &north, &south, &east, &west);

  sp = Screens::GetCurrentScreen(
      this->getX() + (east + west + this->getWidth()) / 2,
      this->getY() + (north + south + this->getHeight()) / 2);
  Places::GetScreenBounds(sp, &box);
  if (!(flags & (MAX_HORIZ | MAX_LEFT | MAX_RIGHT))) {
    box.x = this->getX();
    box.width = this->getWidth();
  }
  if (!(flags & (MAX_VERT | MAX_TOP | MAX_BOTTOM))) {
    box.y = this->getY();
    box.height = this->getHeight();
  }
  Places::SubtractTrayBounds(&box, newState.getLayer());
  Places::SubtractStrutBounds(&box, this);

  if (box.width > this->getMaxWidth()) {
    box.width = this->getMaxWidth();
  }
  if (box.height > this->getMaxHeight()) {
    box.height = this->getMaxHeight();
  }

  if (this->getSizeFlags() & PAspect) {
    if (box.width * this->getAspect().miny
        < box.height * this->getAspect().minx) {
      box.height = (box.width * this->getAspect().miny)
          / this->getAspect().minx;
    }
    if (box.width * this->getAspect().maxy
        > box.height * this->getAspect().maxx) {
      box.width = (box.height * this->getAspect().maxx)
          / this->getAspect().maxy;
    }
  }

  int newX, newY, newHeight, newWidth;
  /* If maximizing horizontally, update width. */
  if (flags & MAX_HORIZ) {
    newX = box.x + west;
    newWidth = box.width - east - west;
    if (!(newState.getStatus() & STAT_IIGNORE)) {
      newWidth -= ((this->getWidth() - this->getBaseWidth()) % this->getXInc());
    }
  } else if (flags & MAX_LEFT) {
    newX = box.x + west;
    newWidth = box.width / 2 - east - west;
    if (!(newState.getStatus() & STAT_IIGNORE)) {
      newWidth -= ((this->getWidth() - this->getBaseWidth()) % this->getXInc());
    }
  } else if (flags & MAX_RIGHT) {
    newX = box.x + box.width / 2 + west;
    newWidth = box.width / 2 - east - west;
    if (!(newState.getStatus() & STAT_IIGNORE)) {
      newWidth -= ((this->getWidth() - this->getBaseWidth()) % this->getXInc());
    }
  }

  /* If maximizing vertically, update height. */
  if (flags & MAX_VERT) {
    newY = box.y + north;
    newHeight = box.height - north - south;
    if (!(newState.getStatus() & STAT_IIGNORE)) {
      newHeight -= ((this->getHeight() - this->getBaseHeight())
          % this->getYInc());
    }
  } else if (flags & MAX_TOP) {
    newY = box.y + north;
    newHeight = box.height / 2 - north - south;
    if (!(newState.getStatus() & STAT_IIGNORE)) {
      newHeight -= ((this->getHeight() - this->getBaseHeight())
          % this->getYInc());
    }
  } else if (flags & MAX_BOTTOM) {
    newY = box.y + box.height / 2 + north;
    newHeight = box.height / 2 - north - south;
    if (!(newState.getStatus() & STAT_IIGNORE)) {
      newHeight -= ((this->getHeight() - this->getBaseHeight())
          % this->getYInc());
    }
  }

  this->setX(newX);
  this->setY(newY);
  this->setWidth(newWidth);
  this->setHeight(newHeight);
}

void ClientNode::InitializeClients() {

}

void ClientNode::DestroyClients() {
  for (auto p : nodes) {
    delete p;
  }
  nodes.clear();
}

/** Set a client's full screen state. */
void ClientNode::SetClientFullScreen(char fullScreen) {

  XEvent event;
  int north, south, east, west;
  BoundingBox box;
  const ScreenType *sp;

  /* Make sure there's something to do. */
  if (!fullScreen == !(this->state.getStatus() & STAT_FULLSCREEN)) {
    return;
  }
  if (!(this->state.getBorder() & BORDER_FULLSCREEN)) {
    return;
  }

  if (this->state.getStatus() & STAT_SHADED) {
    this->UnshadeClient();
  }

  if (fullScreen) {

    this->state.setFullscreen();

    if (!(this->state.getMaxFlags())) {
      this->oldx = this->x;
      this->oldy = this->y;
      this->oldWidth = this->width;
      this->oldHeight = this->height;
    }

    sp = Screens::GetCurrentScreen(this->x, this->y);
    Places::GetScreenBounds(sp, &box);

    Border::GetBorderSize(&this->state, &north, &south, &east, &west);
    box.x += west;
    box.y += north;
    box.width -= east + west;
    box.height -= north + south;

    this->x = box.x;
    this->y = box.y;
    this->width = box.width;
    this->height = box.height;
    Border::ResetBorder(this);

  } else {

    this->state.setNoFullscreen();

    this->x = this->oldx;
    this->y = this->oldy;
    this->width = this->oldWidth;
    this->height = this->oldHeight;
    this->ConstrainSize();
    this->ConstrainPosition();

    if (this->state.getMaxFlags() != MAX_NONE) {
      this->PlaceMaximizedClient(this->state.getMaxFlags());
    }

    Border::ResetBorder(this);

    event.type = MapRequest;
    event.xmaprequest.send_event = True;
    event.xmaprequest.display = display;
    event.xmaprequest.parent = this->parent;
    event.xmaprequest.window = this->window;
    JXSendEvent(display, rootWindow, False, SubstructureRedirectMask, &event);

  }

  Hints::WriteState(this);
  this->SendConfigureEvent();
  _RequireRestack();

}

/** Constrain the size of the client. */
char ClientNode::ConstrainSize() {

  BoundingBox box;
  const ScreenType *sp;
  int north, south, east, west;
  const int oldWidth = this->getWidth();
  const int oldHeight = this->getHeight();

  int x = this->getX();
  int y = this->getY();
  int width = this->getWidth();
  int height = this->getHeight();
  /* First we make sure the window isn't larger than the program allows.
   * We do this here to avoid moving the window below.
   */
  width = Min(width, this->getMaxWidth());
  height = Min(height, this->getMaxHeight());

  const ClientState *state = this->getState();
  /* Constrain the width if necessary. */
  sp = Screens::GetCurrentScreen(this->getX(), this->getY());
  GetScreenBounds(sp, &box);
  SubtractTrayBounds(&box, state->getLayer());
  SubtractStrutBounds(&box, this);
  Border::GetBorderSize(state, &north, &south, &east, &west);
  if (width + east + west > sp->width) {
    box.x += west;
    box.width -= east + west;
    if (box.width > this->getMaxWidth()) {
      box.width = this->getMaxWidth();
    }
    if (box.width > width) {
      box.width = width;
    }
    x = box.x;
    width = box.width - (box.width % this->getXInc());
  }

  /* Constrain the height if necessary. */
  if (height + north + south > sp->height) {
    box.y += north;
    box.height -= north + south;
    if (box.height > this->getMaxHeight()) {
      box.height = this->getMaxHeight();
    }
    if (box.height > height) {
      box.height = height;
    }
    y = box.y;
    height = box.height - (box.height % this->getYInc());
  }

  /* If the program has a minimum constraint, we apply that here.
   * Note that this could cause the window to overlap something. */
  width = Max(width, this->getMinWidth());
  height = Max(height, this->getMinHeight());

  /* Fix the aspect ratio. */
  if (this->getSizeFlags() & PAspect) {
    AspectRatio aspect = this->getAspect();
    if (width * aspect.miny < height * aspect.minx) {
      height = (width * aspect.miny) / aspect.minx;
    }
    if (width * aspect.maxy > height * aspect.maxx) {
      width = (height * aspect.maxx) / aspect.maxy;
    }
  }

  if (width != oldWidth || height != oldHeight) {
    return 1;
  } else {
    return 0;
  }

}

/** Get the screen bounds. */
void ClientNode::GetScreenBounds(const ScreenType *sp, BoundingBox *box) {
  box->x = sp->x;
  box->y = sp->y;
  box->width = sp->width;
  box->height = sp->height;
}

/** Shrink dest such that it does not intersect with src. */
void ClientNode::SubtractBounds(const BoundingBox *src, BoundingBox *dest) {

  BoundingBox boxes[4];

  if (src->x + src->width <= dest->x) {
    return;
  }
  if (src->y + src->height <= dest->y) {
    return;
  }
  if (dest->x + dest->width <= src->x) {
    return;
  }
  if (dest->y + dest->height <= src->y) {
    return;
  }

  /* There are four ways to do this:
   *  0. Increase the x-coordinate and decrease the width of dest.
   *  1. Increase the y-coordinate and decrease the height of dest.
   *  2. Decrease the width of dest.
   *  3. Decrease the height of dest.
   * We will chose the option which leaves the greatest area.
   * Note that negative areas are possible.
   */

  /* 0 */
  boxes[0] = *dest;
  boxes[0].x = src->x + src->width;
  boxes[0].width = dest->x + dest->width - boxes[0].x;

  /* 1 */
  boxes[1] = *dest;
  boxes[1].y = src->y + src->height;
  boxes[1].height = dest->y + dest->height - boxes[1].y;

  /* 2 */
  boxes[2] = *dest;
  boxes[2].width = src->x - dest->x;

  /* 3 */
  boxes[3] = *dest;
  boxes[3].height = src->y - dest->y;

  /* 0 and 1, winner in 0. */
  if (boxes[0].width * boxes[0].height < boxes[1].width * boxes[1].height) {
    boxes[0] = boxes[1];
  }

  /* 2 and 3, winner in 2. */
  if (boxes[2].width * boxes[2].height < boxes[3].width * boxes[3].height) {
    boxes[2] = boxes[3];
  }

  /* 0 and 2, winner in dest. */
  if (boxes[0].width * boxes[0].height < boxes[2].width * boxes[2].height) {
    *dest = boxes[2];
  } else {
    *dest = boxes[0];
  }

}

Strut *ClientNode::struts = NULL;
int *ClientNode::cascadeOffsets = NULL;

/** Insert a bounding box to the list of struts. */
void ClientNode::InsertStrut(const BoundingBox *box, ClientNode *np) {
  if (JLIKELY(box->width > 0 && box->height > 0)) {
    Strut *sp = new Strut;
    sp->client = np;
    sp->box = *box;
    sp->next = struts;
    struts = sp;
  }
}

/** Remove struts associated with a client. */
char ClientNode::DoRemoveClientStrut(ClientNode *np) {
  char updated = 0;
  Strut **spp = &struts;
  while (*spp) {
    Strut *sp = *spp;
    if (sp->client == np) {
      *spp = sp->next;
      Release(sp);
      updated = 1;
    } else {
      spp = &sp->next;
    }
  }
  return updated;
}

/** Remove struts from the bounding box. */
void ClientNode::SubtractStrutBounds(BoundingBox *box, const ClientNode *np) {

  Strut *sp;
  BoundingBox last;

  for (sp = struts; sp; sp = sp->next) {
    if (np != NULL && sp->client == np) {
      continue;
    }
    if (IsClientOnCurrentDesktop(sp->client)) {
      last = *box;
      SubtractBounds(&sp->box, box);
      if (box->width * box->height <= 0) {
        *box = last;
        break;
      }
    }
  }

}

/** Subtract tray area from the bounding box. */
void ClientNode::SubtractTrayBounds(BoundingBox *box, unsigned int layer) {
  BoundingBox src;
  BoundingBox last;

  std::vector<BoundingBox> boxes = Tray::GetBoundsAbove(layer);
  std::vector<BoundingBox>::iterator it;
  for (it = boxes.begin(); it != boxes.end(); ++it) {
    BoundingBox bb = *it;
    src.x = bb.x;
    src.y = bb.y;
    src.width = bb.width;
    src.height = bb.height;
    if (src.x < 0) {
      src.width += src.x;
      src.x = 0;
    }
    if (src.y < 0) {
      src.height += src.y;
      src.y = 0;
    }

    last = *box;
    SubtractBounds(&src, box);
    if (box->width * box->height <= 0) {
      *box = last;
      break;
    }

  }
}

/** Constrain the position of a client. */
void ClientNode::ConstrainPosition() {

  BoundingBox box;
  int north, south, east, west;

  /* Get the bounds for placement. */
  box.x = 0;
  box.y = 0;
  box.width = rootWidth;
  box.height = rootHeight;
  SubtractTrayBounds(&box, this->getState()->getLayer());
  SubtractStrutBounds(&box, this);

  /* Fix the position. */
  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
  int width = this->getWidth();
  int height = this->getHeight();
  int x = this->getX();
  int y = this->getY();
  if (y + width + east + west > box.x + box.width) {
    x = box.x + box.width - width - east;
  }
  if (y + height + north + south > box.y + box.height) {
    y = box.y + box.height - height - south;
  }
  if (x < box.x + west) {
    x = box.x + west;
  }
  if (y < box.y + north) {
    y = box.y + north;
  }

  this->setX(x);
  this->setY(y);
  this->setWidth(width);
  this->setHeight(height);
}

/** Set the active client. */
void ClientNode::FocusClient() {
  if (this->state.getStatus() & STAT_HIDDEN) {
    return;
  }
  if (!(this->state.getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS))) {
    return;
  }

  if (activeClient != this || !(this->state.getStatus() & STAT_ACTIVE)) {
    if (activeClient) {
      activeClient->state.setNotActive();
      if (!(activeClient->state.getStatus() & STAT_OPACITY)) {
        activeClient->SetOpacity(settings.inactiveClientOpacity, 0);
      }
      Border::DrawBorder(activeClient);
      Hints::WriteNetState(activeClient);
    }
    this->state.setActive();
    activeClient = this;
    if (!(this->state.getStatus() & STAT_OPACITY)) {
      this->SetOpacity(settings.activeClientOpacity, 0);
    }

    Border::DrawBorder(this);
    _RequirePagerUpdate();
    _RequireTaskUpdate();
  }

  if (this->state.getStatus() & STAT_MAPPED) {
    this->UpdateClientColormap(-1);
    Hints::SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, this->window);
    Hints::WriteNetState(this);
    if (this->state.getStatus() & STAT_CANFOCUS) {
      JXSetInputFocus(display, this->window, RevertToParent, eventTime);
    }
    if (this->state.getStatus() & STAT_TAKEFOCUS) {
      SendClientMessage(this->window, ATOM_WM_PROTOCOLS, ATOM_WM_TAKE_FOCUS);
    }
  } else {
    JXSetInputFocus(display, rootWindow, RevertToParent, eventTime);
  }

}

/** Refocus the active client (if there is one). */
void ClientNode::RefocusClient(void) {
  if (activeClient) {
    activeClient->FocusClient();
  }
}

/** Send a delete message to a client. */
void ClientNode::DeleteClient() {
  Hints::ReadWMProtocols(this->window, &this->state);
  if (this->state.getStatus() & STAT_DELETE) {
    SendClientMessage(this->window, ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW);
  } else {
    this->KillClient();
  }
}

/** Callback to kill a client after a confirm dialog. */
void ClientNode::KillClientHandler(ClientNode *np) {
  if (np == activeClient) {
    ClientList::FocusNextStacked(np);
  }

  JXKillClient(display, np->window);
}

/** Kill a client window. */
void ClientNode::KillClient() {
  Dialogs::ShowConfirmDialog(this, ClientNode::KillClientHandler,
  _("Kill this window?"),
  _("This may cause data to be lost!"),
  NULL);
}

/** Place transients on top of the owner. */
void ClientNode::RestackTransients() {
  ClientNode *tp;
  unsigned int layer;

  /* TODO: Place any transient windows on top of the owner */
//	for (layer = 0; layer < LAYER_COUNT; layer++) {
//		for (tp = nodes[layer]; tp; tp = tp->next) {
//			if (tp->owner == this->window && tp->prev) {
//
//				ClientNode *next = tp->next;
//
//				tp->prev->next = tp->next;
//				if (tp->next) {
//					tp->next->prev = tp->prev;
//				} else {
//					nodeTail[tp->state.getLayer()] = tp->prev;
//				}
//				tp->next = nodes[tp->state.getLayer()];
//				nodes[tp->state.getLayer()]->prev = tp;
//				tp->prev = NULL;
//				nodes[tp->state.getLayer()] = tp;
//
//				tp = next;
//
//			}
//
//			/* tp will be tp->next if the above code is executed. */
//			/* Thus, if it is NULL, we are done with this layer. */
//			if (!tp) {
//				break;
//			}
//		}
//	}
}

/** Raise the client. This will affect transients. */
void ClientNode::RaiseClient() {

  ClientList::BringToTopOfLayer(this);

  this->RestackTransients();
  _RequireRestack();

}

/** Restack a client window. This will not affect transients. */
void ClientNode::RestackClient(Window above, int detail) {

  ClientNode *tp;
  char inserted = 0;

  /* Remove from the window list. */
  ClientList::RemoveFrom(this);

  /* Insert back into the window list. */
  if (above != None && above != this->window) {

    ClientList::InsertRelative(this, above, detail);
  }
  if (!inserted) {

    /* Insert absolute for the layer. */
    if (detail == Below || detail == BottomIf) {

      /* Insert to the bottom of the stack. */
      ClientList::InsertAt(this);

    } else {

      /* Insert at the top of the stack. */
      ClientList::InsertFirst(this);
    }
  }

  this->RestackTransients();
  _RequireRestack();

}

/** Restack the clients according the way we want them. */
void ClientNode::RestackClients(void) {

  unsigned int layer, index;
  int trayCount;
  Window *stack;
  Window fw;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  /* Allocate memory for restacking. */
  trayCount = Tray::GetTrayCount();
  stack = AllocateStack((clientCount + trayCount) * sizeof(Window));

  /* Prepare the stacking array. */
  fw = None;
  index = 0;
  if (activeClient
      && (activeClient->getState()->getStatus() & STAT_FULLSCREEN)) {
    fw = activeClient->window;
    std::vector<ClientNode*> clients = ClientList::GetLayerList(
        activeClient->getState()->getLayer());
    for (int i = 0; i < clients.size(); ++i) {
      ClientNode *np = clients[i];
      if (np->getOwner() == fw) {
        if (np->getParent() != None) {
          stack[index] = np->getParent();
        } else {
          stack[index] = np->getWindow();
        }
        index += 1;
      }
    }
    if (activeClient->parent != None) {
      stack[index] = activeClient->parent;
    } else {
      stack[index] = activeClient->window;
    }
    index += 1;
  }
  layer = LAST_LAYER;
  for (;;) {

    std::vector<ClientNode*> clients = ClientList::GetLayerList(layer);
    for (int i = 0; i < clients.size(); ++i) {
      ClientNode *np = clients[i];
      if ((np->getState()->getStatus() & (STAT_MAPPED | STAT_SHADED))
          && !(np->getState()->getStatus() & STAT_HIDDEN)) {
        if (fw != None && (np->getWindow() == fw || np->getOwner() == fw)) {
          continue;
        }
        if (np->getParent() != None) {
          stack[index] = np->getParent();
        } else {
          stack[index] = np->getWindow();
        }
        index += 1;
      }
    }

    std::vector<Window> windows = Tray::getTrayWindowsAt(layer);
    std::vector<Window>::iterator it;
    for (it = windows.begin(); it != windows.end(); ++it) {
      stack[index] = (*it);
      index += 1;
    }

    if (layer == FIRST_LAYER) {
      break;
    }
    layer -= 1;

  }

  JXRestackWindows(display, stack, index);

  ReleaseStack(stack);
  TaskBar::UpdateNetClientList();
  _RequirePagerUpdate();

}

/** Send a client message to a window. */
void ClientNode::SendClientMessage(Window w, AtomType type, AtomType message) {

  XEvent event;
  int status;

  memset(&event, 0, sizeof(event));
  event.xclient.type = ClientMessage;
  event.xclient.window = w;
  event.xclient.message_type = Hints::atoms[type];
  event.xclient.format = 32;
  event.xclient.data.l[0] = Hints::atoms[message];
  event.xclient.data.l[1] = eventTime;

  status = JXSendEvent(display, w, False, 0, &event);
  if (JUNLIKELY(status == False)) {
    Debug("SendClientMessage failed");
  }

}

/** Remove a client window from management. */
void ClientNode::RemoveClient() {
  std::vector<ClientNode*>::iterator found;
  if((found = std::find(nodes.begin(), nodes.end(), this)) != nodes.end()) {
    nodes.erase(found);
  }

  delete this;
}

/** Determine the title to display for a client. */
void ClientNode::ReadWMName() {

  unsigned long count;
  int status;
  unsigned long extra;
  Atom realType;
  int realFormat;
  unsigned char *name;

  if (this->getName()) {
    delete[] (this->getName());
  }

  status = JXGetWindowProperty(display, this->getWindow(),
      Hints::atoms[ATOM_NET_WM_NAME], 0, 1024, False,
      Hints::atoms[ATOM_UTF8_STRING], &realType, &realFormat, &count, &extra,
      &name);
  if (status != Success || realFormat == 0) {
    this->name = NULL;
  } else {
    this->name = new char[count + 1];
    memcpy(this->name, name, count);
    this->name[count] = 0;
    JXFree(name);
    this->name = Fonts::ConvertFromUTF8(this->name);
  }

#ifdef USE_XUTF8
  if (!this->getName()) {
    status = JXGetWindowProperty(display, this->getWindow(), XA_WM_NAME, 0,
        1024, False, Hints::atoms[ATOM_COMPOUND_TEXT], &realType, &realFormat,
        &count, &extra, &name);
    if (status == Success && realFormat != 0) {
      char **tlist;
      XTextProperty tprop;
      int tcount;
      tprop.value = name;
      tprop.encoding = Hints::atoms[ATOM_COMPOUND_TEXT];
      tprop.format = realFormat;
      tprop.nitems = count;
      if (XmbTextPropertyToTextList(display, &tprop, &tlist, &tcount) == Success
          && tcount > 0) {
        const size_t len = strlen(tlist[0]) + 1;
        this->name = new char[len];
        memcpy(this->name, tlist[0], len);
        XFreeStringList(tlist);
      }
      JXFree(name);
    }
  }
#endif

  if (!this->getName()) {
    char *temp = NULL;
    if (JXFetchName(display, this->getWindow(), &temp)) {
      const size_t len = strlen(temp) + 1;
      this->name = new char[len];
      memcpy(this->name, temp, len);
      JXFree(temp);
    }
  }

}

/** Read the window class for a client. */
void ClientNode::ReadWMClass() {
  XClassHint hint;
  if (JXGetClassHint(display, this->getWindow(), &hint)) {
    this->instanceName = hint.res_name;
    this->className = hint.res_class;
  }
}

/** Get the active client (possibly NULL). */
ClientNode* ClientNode::GetActiveClient(void) {
  return activeClient;
}

/** Find a client by parent or window. */
ClientNode* ClientNode::FindClient(Window w) {
  ClientNode *np;
  np = FindClientByWindow(w);
  if (!np) {
    np = FindClientByParent(w);
  }
  return np;
}

/** Find a client by window. */
ClientNode* ClientNode::FindClientByWindow(Window w) {
  ClientNode *np;
  if (!XFindContext(display, w, clientContext, (char**) &np)) {
    return np;
  } else {
    return NULL;
  }
}

/** Find a client by its frame window. */
ClientNode* ClientNode::FindClientByParent(Window p) {
  ClientNode *np;
  if (!XFindContext(display, p, frameContext, (char**) &np)) {
    return np;
  } else {
    return NULL;
  }
}

/** Reparent a client window. */
void ClientNode::ReparentClient() {
  ClientNode *np = this;
  XSetWindowAttributes attr;
  XEvent event;
  int attrMask;
  int x, y, width, height;
  int north, south, east, west;

  if ((this->state.getBorder() & (BORDER_TITLE | BORDER_OUTLINE)) == 0) {

    if (this->parent == None) {
      return;
    }

    JXReparentWindow(display, this->window, rootWindow, this->x, this->y);
    XDeleteContext(display, this->parent, frameContext);
    JXDestroyWindow(display, this->parent);
    this->parent = None;

  } else {

    if (this->parent != None) {
      return;
    }

    attrMask = 0;

    /* We can't use PointerMotionHint mask here since the exact location
     * of the mouse on the frame is important. */
    attrMask |= CWEventMask;
    attr.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask
        | PointerMotionMask | SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | KeyPressMask | KeyReleaseMask;

    attrMask |= CWDontPropagate;
    attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

    attrMask |= CWBackPixel;
    attr.background_pixel = Colors::lookupColor(COLOR_TITLE_BG2);

    attrMask |= CWBorderPixel;
    attr.border_pixel = 0;

    x = this->x;
    y = this->y;
    width = this->width;
    height = this->height;
    Border::GetBorderSize(&this->state, &north, &south, &east, &west);
    x -= west;
    y -= north;
    width += east + west;
    height += north + south;

    /* Create the frame window. */
    this->parent = JXCreateWindow(display, rootWindow, x, y, width, height, 0,
        rootDepth, InputOutput, rootVisual, attrMask, &attr);
    XSaveContext(display, this->parent, frameContext, (const char*) this);

    JXSetWindowBorderWidth(display, this->window, 0);

    /* Reparent the client window. */
    JXReparentWindow(display, this->window, this->parent, west, north);

    if (this->state.getStatus() & STAT_MAPPED) {
      JXMapWindow(display, this->parent);
    }
  }

  JXSync(display, False);
  JXCheckTypedWindowEvent(display, this->window, UnmapNotify, &event);

}

/** Send a configure event to a client window. */
void ClientNode::SendConfigureEvent() {

  XConfigureEvent event;
  const ScreenType *sp;

  memset(&event, 0, sizeof(event));
  event.display = display;
  event.type = ConfigureNotify;
  event.event = this->window;
  event.window = this->window;
  if (this->state.getStatus() & STAT_FULLSCREEN) {
    sp = Screens::GetCurrentScreen(this->x, this->y);
    event.x = sp->x;
    event.y = sp->y;
    event.width = sp->width;
    event.height = sp->height;
  } else {
    event.x = this->x;
    event.y = this->y;
    event.width = this->width;
    event.height = this->height;
  }

  JXSendEvent(display, this->window, False, StructureNotifyMask,
      (XEvent* )&event);

}

/** Update a window's colormap.
 * A call to this function indicates that the colormap(s) for the given
 * client changed. This will change the active colormap(s) if the given
 * client is active.
 */
void ClientNode::UpdateClientColormap(Colormap cmap) {
  if (this == activeClient) {
    if (this->cmap != -1) {
      this->cmap = cmap;
    }
    ColormapNode *cp = this->colormaps;

    char wasInstalled = 0;
    while (cp) {
      XWindowAttributes attr;
      if (JXGetWindowAttributes(display, cp->window, &attr)) {
        if (attr.colormap != None) {
          if (attr.colormap == this->cmap) {
            wasInstalled = 1;
          }
          JXInstallColormap(display, attr.colormap);
        }
      }
      cp = cp->next;
    }

    if (!wasInstalled && this->cmap != None) {
      JXInstallColormap(display, this->cmap);
    }

  }

}

/** Update callback for clients with the urgency hint set. */
void ClientNode::SignalUrgent(const TimeType *now, int x, int y, Window w,
    void *data) {
  ClientNode *np = (ClientNode*) data;

  /* Redraw borders. */
  if (np->getState()->getStatus() & STAT_FLASH) {
    np->state.setNoFlash();
  } else if (!(np->state.getStatus() & STAT_NOTURGENT)) {
    np->state.setFlash();
  }
  Border::DrawBorder(np);
  _RequireTaskUpdate();
  _RequirePagerUpdate();

}

/** Unmap a client window and consume the UnmapNotify event. */
void ClientNode::UnmapClient() {
  if (this->state.getStatus() & STAT_MAPPED) {
    XEvent e;

    /* Unmap the window and record that we did so. */
    this->state.setNotMapped();
    JXUnmapWindow(display, this->getWindow());

    /* Discard the unmap event so we don't process it later. */
    JXSync(display, False);
    if (JXCheckTypedWindowEvent(display, this->getWindow(), UnmapNotify, &e)) {
      _UpdateTime(&e);
    }
  }
}

/** Read colormap information for a client. */
void ClientNode::ReadWMColormaps() {

  Window *windows;
  ColormapNode *cp;
  int count;

  if (JXGetWMColormapWindows(display, this->getWindow(), &windows, &count)) {
    if (count > 0) {
      int x;

      /* Free old colormaps. */
      while (this->colormaps) {
        cp = this->colormaps->next;
        Release(this->colormaps);
        this->colormaps = cp;
      }

      /* Put the maps in the list in order so they will come out in
       * reverse order. This way they will be installed with the
       * most important last.
       * Keep track of at most colormapCount colormaps for each
       * window to avoid doing extra work. */
      count = Min(colormapCount, count);
      for (x = 0; x < count; x++) {
        cp = new ColormapNode;
        cp->window = windows[x];
        cp->next = this->getColormaps();
        this->colormaps = cp;
      }

      JXFree(windows);

    }
  }

}

/** Attempt to place the client at the specified coordinates. */
int ClientNode::TryTileClient(const BoundingBox *box, int x, int y) {
  int layer;
  int north, south, east, west;
  int x1, x2, y1, y2;
  int ox1, ox2, oy1, oy2;
  int overlap;

  /* Set the client position. */
  ClientState newState = *this->getState();
  Border::GetBorderSize(&newState, &north, &south, &east, &west);
  this->x = x + west;
  this->y = y + north;
  this->ConstrainSize();
  this->ConstrainPosition();

  /* Get the client boundaries. */
  x1 = this->x - west;
  x2 = this->x + this->width + east;
  y1 = this->y - north;
  y2 = this->y + this->height + south;

  overlap = 0;

  /* Return maximum cost for window outside bounding box. */
  if (x1 < box->x || x2 > box->x + box->width || y1 < box->y
      || y2 > box->y + box->height) {
    return INT_MAX;
  }

  /* Loop over each client. */
  for (layer = this->getState()->getLayer(); layer < LAYER_COUNT; layer++) {
    std::vector<ClientNode*> clients = ClientList::GetLayerList(layer);
    for (int i = 0; i < clients.size(); ++i) {
      ClientNode *tp = clients[i];

      /* Skip clients that aren't visible. */
      if (!IsClientOnCurrentDesktop(tp)) {
        continue;
      }
      if (!(tp->state.getStatus() & STAT_MAPPED)) {
        continue;
      }
      if (tp == this) {
        continue;
      }

      /* Get the boundaries of the other client. */
      Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
      ox1 = tp->x - west;
      ox2 = tp->x + tp->width + east;
      oy1 = tp->y - north;
      oy2 = tp->y + tp->height + south;

      /* Check for an overlap. */
      if (x2 <= ox1 || x1 >= ox2) {
        continue;
      }
      if (y2 <= oy1 || y1 >= oy2) {
        continue;
      }
      overlap += (Min(ox2, x2) - Max(ox1, x1)) * (Min(oy2, y2) - Max(oy1, y1));
    }
  }

  return overlap;
}

/** Tiled placement. */

/** Move the window in the specified direction for reparenting. */
void ClientNode::GravitateClient(char negate) {
  int deltax, deltay;
  this->GetGravityDelta(this->gravity, &deltax, &deltay);
  if (negate) {
    this->x += deltax;
    this->y += deltay;
  } else {
    this->x -= deltax;
    this->y -= deltay;
  }
}

/** Snap to window borders. */
void ClientNode::DoSnapBorder() {

  ClientNode *tp;
  const Tray *tray;
  RectangleType client, other;
  RectangleType left = { 0 };
  RectangleType right = { 0 };
  RectangleType top = { 0 };
  RectangleType bottom = { 0 };
  int layer;
  int north, south, east, west;

  GetClientRectangle(this, &client);

  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);

  other.valid = 1;

  /* Work from the bottom of the window stack to the top. */
  for (layer = 0; layer < LAYER_COUNT; layer++) {

    /* Check tray windows. */
    std::vector<BoundingBox> boxes = Tray::GetVisibleBounds();
    std::vector<BoundingBox>::iterator it;
    for (it = boxes.begin(); it != boxes.end(); ++it) {
      BoundingBox box = *it;

      other.left = box.x;
      other.right = box.x + box.width;
      other.top = box.y;
      other.bottom = box.y + box.height;

      left.valid = CheckLeftValid(&client, &other, &left);
      right.valid = CheckRightValid(&client, &other, &right);
      top.valid = CheckTopValid(&client, &other, &top);
      bottom.valid = CheckBottomValid(&client, &other, &bottom);

      if (CheckOverlapTopBottom(&client, &other)) {
        if (abs(client.left - other.right) <= settings.snapDistance) {
          left = other;
        }
        if (abs(client.right - other.left) <= settings.snapDistance) {
          right = other;
        }
      }
      if (CheckOverlapLeftRight(&client, &other)) {
        if (abs(client.top - other.bottom) <= settings.snapDistance) {
          top = other;
        }
        if (abs(client.bottom - other.top) <= settings.snapDistance) {
          bottom = other;
        }
      }

    }

    /* Check client windows. */
    std::vector<ClientNode*> clients = ClientList::GetList();
    for (int i = 0; i < clients.size(); ++i) {
      tp = clients[i];
      if (tp == this || !ShouldSnap(tp)) {
        continue;
      }

      GetClientRectangle(tp, &other);

      /* Check if this border invalidates any previous value. */
      left.valid = CheckLeftValid(&client, &other, &left);
      right.valid = CheckRightValid(&client, &other, &right);
      top.valid = CheckTopValid(&client, &other, &top);
      bottom.valid = CheckBottomValid(&client, &other, &bottom);

      /* Compute the new snap values. */
      if (CheckOverlapTopBottom(&client, &other)) {
        if (abs(client.left - other.right) <= settings.snapDistance) {
          left = other;
        }
        if (abs(client.right - other.left) <= settings.snapDistance) {
          right = other;
        }
      }
      if (CheckOverlapLeftRight(&client, &other)) {
        if (abs(client.top - other.bottom) <= settings.snapDistance) {
          top = other;
        }
        if (abs(client.bottom - other.top) <= settings.snapDistance) {
          bottom = other;
        }
      }

    }

  }

  if (right.valid) {
    this->x = right.left - this->getWidth() - west;
  }
  if (left.valid) {
    this->x = left.right + east;
  }
  if (bottom.valid) {
    this->y = bottom.top - south;
    if (!(this->getState()->getStatus() & STAT_SHADED)) {
      this->y -= this->getHeight();
    }
  }
  if (top.valid) {
    this->y = top.bottom + north;
  }

}

