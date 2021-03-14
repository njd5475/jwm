/**
 * @file client.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client window functions.
 *
 */

#include <algorithm>
#include "nwm.h"
#include "client.h"
#include "clientlist.h"
#include "icon.h"
#include "group.h"
#include "confirm.h"
#include "cursor.h"
#include "screen.h"
#include "color.h"
#include "place.h"
#include "event.h"
#include "settings.h"
#include "timing.h"
#include "grab.h"
#include "hint.h"
#include "DesktopEnvironment.h"
#include "move.h"
#include "font.h"
#include "outline.h"
#include "status.h"
#include "misc.h"

#include <X11/Xlibint.h>

Client *Client::activeClient;

unsigned int Client::clientCount;

std::vector<Client*> Client::nodes;

/** Load windows that are already mapped. */
void Client::StartupClients(void) {

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

  Events::_RequireTaskUpdate();
  Events::_RequirePagerUpdate();

  const unsigned titleHeight = Border::GetTitleHeight();
  unsigned int count;

  count = settings.desktopCount * Screens::GetScreenCount();
  cascadeOffsets = new int[count];

  for (x = 0; x < count; x++) {
    cascadeOffsets[x] = settings.borderWidth + titleHeight;
  }

}

Client* Client::Create(Window w, bool alreadyMapped, bool notOwner) {
  Client *node = new Client(w, alreadyMapped, notOwner);
  nodes.push_back(node);
  return node;
}

/** Place a client on the screen. */
void Client::PlaceClient(Client *np, char alreadyMapped) {

  BoundingBox box;
  const ScreenType *sp;

  if (alreadyMapped || (np->isPosition())
      || (!(np->isIgnoringProgramPosition())
          && (np->getSizeFlags() & (PPosition | USPosition)))) {

    np->GravitateClient(0);
    if (!alreadyMapped) {
      np->ConstrainSize();
      np->ConstrainPosition();
    }

  } else {

    sp = Screens::GetMouseScreen();
    GetScreenBounds(sp, &box);
    SubtractTrayBounds(&box, np->getLayer());
    SubtractStrutBounds(&box, np);

    /* If tiled is specified, first attempt to use tiled placement. */
    if (np->isTiled()) {
      if (np->TileClient(&box)) {
        return;
      }
    }

    /* Either tiled placement failed or was not specified. */
    if (np->isCentered()) {
      np->CenterClient(&box);
    } else {
      np->CascadeClient(&box);
    }

  }

}

/** Cascade placement. */
void Client::CascadeClient(const BoundingBox *box) {
  const ScreenType *sp;
  const unsigned titleHeight = Border::GetTitleHeight();
  int north, south, east, west;
  int cascadeIndex;
  char overflow;

  Border::GetBorderSize(this, &north, &south, &east, &west);
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
void Client::ShutdownClients(void) {

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
void Client::LoadFocus(void) {

  Client *np;
  Window rootReturn, childReturn;
  int rootx, rooty;
  int winx, winy;
  unsigned int mask;

  JXQueryPointer(display, rootWindow, &rootReturn, &childReturn, &rootx, &rooty,
      &winx, &winy, &mask);

  np = FindClient(childReturn);
  if (np) {
    np->keyboardFocus();
  }

}

Client::~Client() {

  this->setDelete();
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

  if (this->isUrgent()) {
    Events::_UnregisterCallback(SignalUrgent, this);
  }

  /* Make sure this client isn't active */
  if (activeClient == this && !shouldExit) {
    ClientList::FocusNextStacked(this);
  }
  if (activeClient == this) {

    /* Must be the last client. */
    Hints::SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, None);
    activeClient = NULL;
    JXSetInputFocus(display, rootWindow, RevertToParent, Events::eventTime);

  }

  /* If the window manager is exiting (ie, not the client), then
   * reparent etc. */
  if (shouldExit && !(this->isDialogWindow())) {
    if (this->getMaxFlags()) {
      this->x = this->oldx;
      this->y = this->oldy;
      this->width = this->oldWidth;
      this->height = this->oldHeight;
      JXMoveResizeWindow(display, this->window, this->x, this->y, this->width,
          this->height);
    }
    this->GravitateClient(1);
    if ((this->isHidden())
        || (!(this->isMapped())
            && (this->isStatus(STAT_MINIMIZED | STAT_SHADED)))) {
      JXMapWindow(display, this->window);
    }
    JXUngrabButton(display, AnyButton, AnyModifier, this->window);
    JXReparentWindow(display, this->window, rootWindow, this->x, this->y);
    JXRemoveFromSaveSet(display, this->window);
  }

  int res = JXKillClient(display, this->window);
  if (res) {
    Log("Error: Could not kill client");

  }

  /* Destroy the parent */
  if (this->parent) {
    JXDestroyWindow(display, this->parent);
    this->parent = None;
  }

  if (this->window) {
    JXDestroyWindow(display, this->window);
    this->window = 0;
  }

  if (this->instanceName) {
    JXFree(this->instanceName);
  }
  if (this->className) {
    JXFree(this->className);
  }

  Places::RemoveClientStrut(this);

  while (this->colormaps) {
    cp = this->colormaps->next;
    Release(this->colormaps);
    this->colormaps = cp;
  }

  Icon::DestroyIcon(this->getIcon());

  Events::_RequireRestack();

  if (this->name) {
    delete[] this->name;
  }
  this->name = NULL;

}

/** Add a window to management. */
Client::Client(Window w, bool alreadyMapped, bool notOwner) :
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
  this->setDesktop(currentDesktop);

  this->x = attr.x;
  this->y = attr.y;
  this->width = attr.width;
  this->height = attr.height;
  this->cmap = attr.colormap;
  this->clearStatus();
  this->clearMaxFlags();
  this->resetLayer();
  this->setDefaultLayer(LAYER_NORMAL);

  this->resetBorder();
  this->mouseContext = MC_NONE;

  Hints::ReadClientInfo(this, alreadyMapped);

  if (!notOwner) {
    this->clearBorder();
    this->setBorderOutline();
    this->setBorderTitle();
    this->setBorderMove();
    this->setDialogWindowStatus();
    this->setSticky();
    this->setLayer(LAYER_ABOVE); //FIXME: can encapsulate this easily
    this->setDefaultLayer(LAYER_ABOVE); //FIXME: can encapsulate this easily
  }

  Groups::ApplyGroups(this);
  if (this->icon == NULL) {
    Icon::LoadIcon(this);
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

  if (this->isMapped()) {
    JXMapWindow(display, this->window);
  }

  clientCount += 1;

  if (!alreadyMapped) {
    this->RaiseClient();
  }

  if (this->hasOpacity()) {
    this->SetOpacity(this->getOpacity(), 1);
  } else {
    this->SetOpacity(settings.inactiveClientOpacity, 1);
  }
  if (this->isSticky()) {
    Hints::SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP, ~0UL);
  } else {
    Hints::SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP,
        this->getDesktop());
  }

  /* Shade the client if requested. */
  if (this->isShaded()) {
    this->setNoShaded();
    this->ShadeClient();
  }

  /* Minimize the client if requested. */
  if (this->isMinimized()) {
    this->setNoMinimized();
    this->MinimizeClient(0);
  }

  /* Maximize the client if requested. */
  if (this->getMaxFlags()) {
    const MaxFlags flags = this->getMaxFlags();
    this->setMaxFlags(MAX_NONE);
    this->MaximizeClient(flags);
  }

  if (this->isUrgent()) {
    Events::_RegisterCallback(URGENCY_DELAY, SignalUrgent, this);
  }

  /* Make sure we're still in sync */
  Hints::WriteState(this);
  this->SendConfigureEvent();

  /* Hide the client if we're not on the right desktop. */
  if (this->getDesktop() != currentDesktop && !(this->isSticky())) {
    this->HideClient();
  }

  Places::ReadClientStrut(this);

  /* Focus transients if their parent has focus. */
  if (this->owner != None) {
    if (activeClient && this->owner == activeClient->window) {
      this->keyboardFocus();
    }
  }

  /* Make the client fullscreen if requested. */
  if (this->isFullscreen()) {
    this->setNoFullscreen();
    this->SetClientFullScreen(1);
  }
  Border::ResetBorder(this);
}

/** Minimize a client window and all of its transients. */
void Client::MinimizeClient(char lower) {
  this->MinimizeTransients(lower);
  Events::_RequireRestack();
  Events::_RequireTaskUpdate();
}

/** Minimize all transients as well as the specified client. */
void Client::MinimizeTransients(char lower) {

  /* Unmap the window and update its state. */
  if (this->isStatus(STAT_MAPPED | STAT_SHADED)) {
    this->UnmapClient();
    if (this->parent != None) {
      JXUnmapWindow(display, this->parent);
    }
  }
  this->setMinimized();

  /* Minimize transient windows. */
  ClientList::MinimizeTransientWindows(this, lower);

  Hints::WriteState(this);

}

void Client::saveBounds() {
  this->oldx = this->x;
  this->oldy = this->y;
  this->oldWidth = this->width;
  this->oldHeight = this->height;
}

/** Determine which way to move the client for the border. */
void Client::GetGravityDelta(int gravity, int *x, int *y) {
  int north, south, east, west;
  Border::GetBorderSize(this, &north, &south, &east, &west);
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
void Client::ShadeClient() {
  if ((this->isStatus(STAT_SHADED | STAT_FULLSCREEN))
      || !(this->getBorder() & BORDER_SHADE)) {
    return;
  }

  this->UnmapClient();
  this->setShaded();

  Hints::WriteState(this);
  Border::ResetBorder(this);
  Events::_RequirePagerUpdate();

}

/** Unshade a client. */
void Client::UnshadeClient() {
  if (!(this->isShaded())) {
    return;
  }

  if (!(this->isStatus(STAT_MINIMIZED | STAT_SDESKTOP))) {
    JXMapWindow(display, this->window);
    this->setMapped();
  }
  this->setNoShaded();

  Hints::WriteState(this);
  Border::ResetBorder(this);
  RefocusClient();
  Events::_RequirePagerUpdate();

}

/** Set a client's state to withdrawn. */
void Client::sendToBackground() {
  if (activeClient == this) {
    activeClient = NULL;
    this->setNotActive();
    ClientList::FocusNextStacked(this);
  }

  if (this->isMapped()) {
    this->UnmapClient();
    if (this->parent != None) {
      JXUnmapWindow(display, this->parent);
    }
  } else if (this->isShaded()) {
    if (!(this->isMinimized())) {
      if (this->parent != None) {
        JXUnmapWindow(display, this->parent);
      }
    }
  }

  this->setNoShaded();
  this->setNoMinimized();
  this->setNoSDesktop();

  Hints::WriteState(this);
  Events::_RequireTaskUpdate();
  Events::_RequirePagerUpdate();

}

/** Restore a window with its transients (helper method). */
void Client::RestoreTransients(char raise) {
  int x;

  /* Make sure this window is on the current desktop. */
  this->SetClientDesktop(currentDesktop);

  /* Restore this window. */
  if (!(this->isMapped())) {
    if (this->isShaded()) {
      if (this->parent != None) {
        JXMapWindow(display, this->parent);
      }
    } else {
      JXMapWindow(display, this->window);
      if (this->parent != None) {
        JXMapWindow(display, this->parent);
      }
      this->setMapped();
    }
  }

  this->setNoMinimized();
  this->setNoSDesktop();

  /* Restore transient windows. */
  //ClientList::RestoreTransientWindows(this->window, raise);
  std::vector<Client*> children = ClientList::GetChildren(this->window);
  for (int i = 0; i < children.size(); ++i) {
    Client *tp = children[i];
    if (tp->isMinimized()) {
      tp->RestoreTransients(raise);
    }

  }

  if (raise) {
    this->keyboardFocus();
    this->RaiseClient();
  }
  Hints::WriteState(this);

}

/** Restore a client window and its transients. */
void Client::RestoreClient(char raise) {
  if ((this->isFixed()) && !(this->isSticky())) {
    DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(this->getDesktop());
  }
  this->RestoreTransients(raise);
  Events::_RequireRestack();
  Events::_RequireTaskUpdate();
}

/** Update window state information. */
void Client::_UpdateState() {
  const char alreadyMapped = (this->isMapped()) ? 1 : 0;
  const char active = (this->isActive()) ? 1 : 0;

  /* Remove from the layer list. */
  ClientList::RemoveFrom(this);

  /* Read the state (and new layer). */
  if (this->isUrgent()) {
    Events::_UnregisterCallback(Client::SignalUrgent, this);
  }
  Hints::ReadWindowState(this, this->getWindow(), alreadyMapped);
  if (this->isUrgent()) {
    Events::_RegisterCallback(URGENCY_DELAY, Client::SignalUrgent, this);
  }

  /* We don't handle mapping the window, so restore its mapped state. */
  if (!alreadyMapped) {
    this->resetMappedState();
  }

  /* Add to the layer list. */
  ClientList::InsertAt(this);

  if (active) {
    this->keyboardFocus();
  }

}

/** Set the opacity of a client. */
void Client::SetOpacity(unsigned int opacity, char force) {
  Window w;
  if (this->getOpacity() == opacity && !force) {
    return;
  }

  w = this->parent != None ? this->parent : this->window;
  this->setOpacity(opacity);
  if (opacity == 0xFFFFFFFF) {
    JXDeleteProperty(display, w, Hints::atoms[ATOM_NET_WM_WINDOW_OPACITY]);
  } else {
    Hints::SetCardinalAtom(w, ATOM_NET_WM_WINDOW_OPACITY, opacity);
  }
}

void Client::UpdateWindowState(bool alreadyMapped) {
  /* Read the window state. */
  Hints::ReadWindowState(this, this->window, alreadyMapped);
  if (this->minWidth == this->maxWidth && this->minHeight == this->maxHeight) {
    this->setNoBorderResize();
    this->setNoBorderMax();
    if (this->minWidth * this->xinc >= rootWidth
        && this->minHeight * this->yinc >= rootHeight) {
      this->setFullscreen();
    }
  }

  /* Make sure this client is on at least as high of a layer
   * as its owner. */
  Client *pp;
  if (this->owner != None) {
    pp = FindClientByWindow(this->owner);
    if (pp) {
      this->setLayer(Max(pp->getLayer(), this->getLayer()));
    }
  }
}

/** Set the client layer. This will affect transients. */
void Client::SetClientLayer(unsigned int layer) {
  Assert(layer <= LAST_LAYER);

  if (this->getLayer() != layer) {
    Client *tp;
    std::vector<Client*> children = ClientList::GetChildren(this->window);

    for (int i = 0; i < children.size(); ++i) {
      tp = children[i];
      if (tp == this || tp->owner == this->window) {
        ClientList::ChangeLayer(tp, layer);
      }
    }

    ClientList::ChangeLayer(this, layer);
    Events::_RequireRestack();
  }
}

/** Set a client's sticky.getStatus(). This will update transients. */
void Client::SetClientSticky(char isSticky) {

  bool old = false;

  /* Get the old sticky.getStatus(). */
  if (this->isSticky()) {
    old = true;
  }

  if (isSticky && !old) {

    /* Change from non-sticky to sticky. */
    std::vector<Client*> children = ClientList::GetChildren(this->window);

    Client *tp;
    for (int i = 0; i < children.size(); ++i) {
      tp = children[i];
      tp->setSticky();
      Hints::SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, ~0UL);
      Hints::WriteState(tp);
    }

    this->setSticky();
    Hints::SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP, ~0UL);
    Hints::WriteState(this);

  } else if (!isSticky && old) {

    /* Change from sticky to non-sticky. */
    std::vector<Client*> children = ClientList::GetChildren(this->window);

    Client *tp;
    for (int i = 0; i < children.size(); ++i) {
      tp = children[i];
      if (tp == this || tp->owner == this->window) {
        tp->setNoSticky();
        Hints::WriteState(tp);
      }
    }

    this->setNoSticky();
    Hints::WriteState(this);

    /* Since this client is no longer sticky, we need to assign
     * a desktop. Here we use the current desktop.
     * Note that SetClientDesktop updates transients (which is good).
     */
    this->SetClientDesktop(currentDesktop);

  }

}

/** Set a client's desktop. This will update transients. */
void Client::SetClientDesktop(unsigned int desktop) {

  if (JUNLIKELY(desktop >= settings.desktopCount)) {
    return;
  }

  if (!(this->isSticky())) {
    Client *tp;

    std::vector<Client*> all = ClientList::GetSelfAndChildren(this);
    for (int i = 0; i < all.size(); ++i) {
      tp = all[i];
      tp->setDesktop(desktop);

      if (desktop == currentDesktop) {
        tp->ShowClient();
      } else {
        tp->HideClient();
      }

      Hints::SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, tp->getDesktop());

    }
    Events::_RequirePagerUpdate();
    Events::_RequireTaskUpdate();
  }

}

/** Hide a client. This will not update transients. */
void Client::HideClient() {
  if (!(this->isHidden())) {
    if (activeClient == this) {
      activeClient = NULL;
    }
    this->setHidden();
    if (this->isStatus(STAT_MAPPED | STAT_SHADED)) {
      if (this->parent != None) {
        JXUnmapWindow(display, this->parent);
      } else {
        JXUnmapWindow(display, this->window);
      }
    }
    //this->clearStatus();
    this->setSDesktopStatus();
    this->setMapped();
    this->setShaded();
  }
}

/** Show a hidden client. This will not update transients. */
void Client::ShowClient() {
  if (this->isHidden()) {
    this->setNotHidden();
    if (this->isStatus(STAT_MAPPED | STAT_SHADED)) {
      if (!(this->isMinimized())) {
        if (this->parent != None) {
          JXMapWindow(display, this->parent);
        } else {
          JXMapWindow(display, this->window);
        }
        if (this->isActive()) {
          this->keyboardFocus();
        }
      }
    }
  }
}

/** Maximize a client window. */
void Client::MaximizeClient(MaxFlags flags) {

  /* Don't allow maximization of full-screen clients. */
  if (this->isFullscreen()) {
    return;
  }
  if (!(this->getBorder() & BORDER_MAX)) {
    return;
  }

  if (this->isShaded()) {
    this->UnshadeClient();
  }

  if (this->isMinimized()) {
    this->RestoreClient(1);
  }

  this->RaiseClient();
  this->keyboardFocus();
  if (this->getMaxFlags()) {
    /* Undo existing maximization. */
    this->x = this->oldx;
    this->y = this->oldy;
    this->width = this->oldWidth;
    this->height = this->oldHeight;
    this->setMaxFlags(MAX_NONE);
  }
  if (flags != MAX_NONE) {
    /* Maximize if requested. */
    this->PlaceMaximizedClient(flags);
  }

  Hints::WriteState(this);
  Border::ResetBorder(this);
  Border::DrawBorder(this);
  this->SendConfigureEvent();
  Events::_RequirePagerUpdate();

}

void Client::clearController() {
  this->controller = NULL;
}

/** Maximize a client using its default maximize settings. */
void Client::MaximizeClientDefault() {

  MaxFlags flags = MAX_NONE;

  if (this->getMaxFlags() == MAX_NONE) {
    if (this->getBorder() & BORDER_MAX_H) {
      flags |= MAX_HORIZ;
    }
    if (this->getBorder() & BORDER_MAX_V) {
      flags |= MAX_VERT;
    }
  }

  this->MaximizeClient(flags);

}

char Client::TileClient(const BoundingBox *box) {

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
  std::vector<Client*> insertionPoints =
      ClientList::GetMappedDesktopClients();
  for (int i = 0; i < insertionPoints.size(); ++i) {
    Client *ip = insertionPoints[i];
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

  const Client *tp = NULL;
  for (int i = 0; i < insertionPoints.size(); ++i) {
    Client *tp = insertionPoints[i];
    if (tp == this) {
      continue;
    }
    Border::GetBorderSize(tp, &north, &south, &east, &west);
    xs[count + 0] = tp->getX() - west;
    xs[count + 1] = tp->getX() + tp->getWidth() + east;
    ys[count + 0] = tp->getY() - north;
    ys[count + 1] = tp->getY() + tp->getHeight() + south;
    count += 2;
  }

  /* Try placing at lower right edge of box, too. */
  Border::GetBorderSize(this, &north, &south, &east, &west);
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
    Border::GetBorderSize(this, &north, &south, &east, &west);
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
void Client::PlaceMaximizedClient(MaxFlags flags) {
  BoundingBox box;
  const ScreenType *sp;
  int north, south, east, west;

  this->saveBounds();
  this->setMaxFlags(flags);

  Border::GetBorderSize(this, &north, &south, &east, &west);

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
  Places::SubtractTrayBounds(&box, this->getLayer());
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
    if (!(this->willIgnoreIncrementWhenMaximized())) {
      newWidth -= ((this->getWidth() - this->getBaseWidth()) % this->getXInc());
    }
  } else if (flags & MAX_LEFT) {
    newX = box.x + west;
    newWidth = box.width / 2 - east - west;
    if (!(this->willIgnoreIncrementWhenMaximized())) {
      newWidth -= ((this->getWidth() - this->getBaseWidth()) % this->getXInc());
    }
  } else if (flags & MAX_RIGHT) {
    newX = box.x + box.width / 2 + west;
    newWidth = box.width / 2 - east - west;
    if (!(this->willIgnoreIncrementWhenMaximized())) {
      newWidth -= ((this->getWidth() - this->getBaseWidth()) % this->getXInc());
    }
  }

  /* If maximizing vertically, update height. */
  if (flags & MAX_VERT) {
    newY = box.y + north;
    newHeight = box.height - north - south;
    if (!(this->willIgnoreIncrementWhenMaximized())) {
      newHeight -= ((this->getHeight() - this->getBaseHeight())
          % this->getYInc());
    }
  } else if (flags & MAX_TOP) {
    newY = box.y + north;
    newHeight = box.height / 2 - north - south;
    if (!(this->willIgnoreIncrementWhenMaximized())) {
      newHeight -= ((this->getHeight() - this->getBaseHeight())
          % this->getYInc());
    }
  } else if (flags & MAX_BOTTOM) {
    newY = box.y + box.height / 2 + north;
    newHeight = box.height / 2 - north - south;
    if (!(this->willIgnoreIncrementWhenMaximized())) {
      newHeight -= ((this->getHeight() - this->getBaseHeight())
          % this->getYInc());
    }
  }

  this->setX(newX);
  this->setY(newY);
  this->setWidth(newWidth);
  this->setHeight(newHeight);
}

void Client::InitializeClients() {

}

void Client::DestroyClients() {
  for (auto p : nodes) {
    delete p;
  }
  nodes.clear();
}

/** Set a client's full screen state. */
void Client::SetClientFullScreen(char fullScreen) {

  XEvent event;
  int north, south, east, west;
  BoundingBox box;
  const ScreenType *sp;

  /* Make sure there's something to do. */
  if (!fullScreen == !(this->isFullscreen())) {
    return;
  }
  if (!(this->getBorder() & BORDER_FULLSCREEN)) {
    return;
  }

  if (this->isShaded()) {
    this->UnshadeClient();
  }

  if (fullScreen) {

    this->setFullscreen();

    if (!(this->getMaxFlags())) {
      this->oldx = this->x;
      this->oldy = this->y;
      this->oldWidth = this->width;
      this->oldHeight = this->height;
    }

    sp = Screens::GetCurrentScreen(this->x, this->y);
    Places::GetScreenBounds(sp, &box);

    Border::GetBorderSize(this, &north, &south, &east, &west);
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

    this->setNoFullscreen();

    this->x = this->oldx;
    this->y = this->oldy;
    this->width = this->oldWidth;
    this->height = this->oldHeight;
    this->ConstrainSize();
    this->ConstrainPosition();

    if (this->getMaxFlags() != MAX_NONE) {
      this->PlaceMaximizedClient(this->getMaxFlags());
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
  Events::_RequireRestack();

}

/** Constrain the size of the client. */
char Client::ConstrainSize() {

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

  /* Constrain the width if necessary. */
  sp = Screens::GetCurrentScreen(this->getX(), this->getY());
  GetScreenBounds(sp, &box);
  SubtractTrayBounds(&box, this->getLayer());
  SubtractStrutBounds(&box, this);
  Border::GetBorderSize(this, &north, &south, &east, &west);
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
void Client::GetScreenBounds(const ScreenType *sp, BoundingBox *box) {
  box->x = sp->x;
  box->y = sp->y;
  box->width = sp->width;
  box->height = sp->height;
}

/** Shrink dest such that it does not intersect with src. */
void Client::SubtractBounds(const BoundingBox *src, BoundingBox *dest) {

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

Strut *Client::struts = NULL;
int *Client::cascadeOffsets = NULL;

/** Insert a bounding box to the list of struts. */
void Client::InsertStrut(const BoundingBox *box, Client *np) {
  if (JLIKELY(box->width > 0 && box->height > 0)) {
    Strut *sp = new Strut;
    sp->client = np;
    sp->box = *box;
    sp->next = struts;
    struts = sp;
  }
}

/** Remove struts associated with a client. */
char Client::DoRemoveClientStrut(Client *np) {
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
void Client::SubtractStrutBounds(BoundingBox *box, const Client *np) {

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
void Client::SubtractTrayBounds(BoundingBox *box, unsigned int layer) {
  BoundingBox src;
  BoundingBox last;

  std::vector<BoundingBox> boxes;
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
void Client::ConstrainPosition() {

  BoundingBox box;
  int north, south, east, west;

  /* Get the bounds for placement. */
  box.x = 0;
  box.y = 0;
  box.width = rootWidth;
  box.height = rootHeight;
  SubtractTrayBounds(&box, this->getLayer());
  SubtractStrutBounds(&box, this);

  /* Fix the position. */
  Border::GetBorderSize(this, &north, &south, &east, &west);
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
void Client::keyboardFocus() {
  if (this->isHidden()) {
    return;
  }
  if (!(this->isStatus(STAT_CANFOCUS | STAT_TAKEFOCUS))) {
    return;
  }

  if (activeClient != this || !(this->isActive())) {
    if (activeClient) {
      activeClient->setNotActive();
      if (!(activeClient->hasOpacity())) {
        activeClient->SetOpacity(settings.inactiveClientOpacity, 0);
      }
      Border::DrawBorder(activeClient);
      Hints::WriteNetState(activeClient);
    }
    this->setActive();
    activeClient = this;
    if (!(this->hasOpacity())) {
      this->SetOpacity(settings.activeClientOpacity, 0);
    }

    Border::DrawBorder(this);
    Events::_RequirePagerUpdate();
    Events::_RequireTaskUpdate();
  }

  if (this->isMapped()) {
    this->UpdateClientColormap(-1);
    Hints::SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, this->window);
    Hints::WriteNetState(this);
    if (this->canFocus()) {
      JXSetInputFocus(display, this->window, RevertToParent, Events::eventTime);
    }
    if (this->shouldTakeFocus()) {
      SendClientMessage(this->window, ATOM_WM_PROTOCOLS, ATOM_WM_TAKE_FOCUS);
    }
  } else {
    JXSetInputFocus(display, rootWindow, RevertToParent, Events::eventTime);
  }

}

/** Refocus the active client (if there is one). */
void Client::RefocusClient(void) {
  if (activeClient) {
    activeClient->keyboardFocus();
  }
}

/** Send a delete message to a client. */
void Client::DeleteClient() {
  Hints::ReadWMProtocols(this->window, this);
  if (this->shouldDelete()) {
    SendClientMessage(this->window, ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW);
  } else {
    this->KillClient();
  }
}

/** Callback to kill a client after a confirm dialog. */
void Client::KillClientHandler(Client *np) {
  if (np == activeClient) {
    ClientList::FocusNextStacked(np);
  }

  JXKillClient(display, np->window);
}

/** Kill a client window. */
void Client::KillClient() {
  Dialogs::ShowConfirmDialog(this, Client::KillClientHandler,
  _("Kill this window?"),
  _("This may cause data to be lost!"),
  NULL);
}

/** Place transients on top of the owner. */
void Client::RestackTransients() {
//  ClientNode *tp;
//  unsigned int layer;

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
//					nodeTail[tp->getLayer()] = tp->prev;
//				}
//				tp->next = nodes[tp->getLayer()];
//				nodes[tp->getLayer()]->prev = tp;
//				tp->prev = NULL;
//				nodes[tp->getLayer()] = tp;
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
void Client::RaiseClient() {

  ClientList::BringToTopOfLayer(this);

  this->RestackTransients();
  Events::_RequireRestack();

}

/** Restack a client window. This will not affect transients. */
void Client::RestackClient(Window above, int detail) {

  Client *tp;
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
  Events::_RequireRestack();

}

/** Restack the clients according the way we want them. */
void Client::RestackClients(void) {

  unsigned int layer, index;
  int trayCount = 0;
  Window *stack;
  Window fw;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  /* Allocate memory for restacking. */

  stack = AllocateStack((clientCount + trayCount) * sizeof(Window));

  /* Prepare the stacking array. */
  fw = None;
  index = 0;
  if (activeClient && (activeClient->isFullscreen())) {
    fw = activeClient->window;
    std::vector<Client*> clients = ClientList::GetLayerList(
        activeClient->getLayer());
    for (int i = 0; i < clients.size(); ++i) {
      Client *np = clients[i];
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

    std::vector<Client*> clients = ClientList::GetLayerList(layer);
    for (int i = 0; i < clients.size(); ++i) {
      Client *np = clients[i];
      if ((np->isStatus(STAT_MAPPED | STAT_SHADED)) && !(np->isHidden())) {
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

    std::vector<Window> windows;
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
  Events::_RequirePagerUpdate();

}

/** Send a client message to a window. */
void Client::SendClientMessage(Window w, AtomType type, AtomType message) {

  XEvent event;
  int status;

  memset(&event, 0, sizeof(event));
  event.xclient.type = ClientMessage;
  event.xclient.window = w;
  event.xclient.message_type = Hints::atoms[type];
  event.xclient.format = 32;
  event.xclient.data.l[0] = Hints::atoms[message];
  event.xclient.data.l[1] = Events::eventTime;

  status = JXSendEvent(display, w, False, 0, &event);
  if (JUNLIKELY(status == False)) {
    Debug("SendClientMessage failed");
  }

}

/** Remove a client window from management. */
void Client::RemoveClient() {
  std::vector<Client*>::iterator found;
  if ((found = std::find(nodes.begin(), nodes.end(), this)) != nodes.end()) {
    nodes.erase(found);
  }

  delete this;
}

/** Determine the title to display for a client. */
void Client::ReadWMName() {

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
void Client::ReadWMClass() {
  XClassHint hint;
  if (JXGetClassHint(display, this->getWindow(), &hint)) {
    this->instanceName = hint.res_name;
    this->className = hint.res_class;
  }
}

/** Get the active client (possibly NULL). */
Client* Client::GetActiveClient(void) {
  return activeClient;
}

/** Find a client by parent or window. */
Client* Client::FindClient(Window w) {
  Client *np;
  np = FindClientByWindow(w);
  if (!np) {
    np = FindClientByParent(w);
  }
  return np;
}

/** Find a client by window. */
Client* Client::FindClientByWindow(Window w) {
  Client *np;
  if (!XFindContext(display, w, clientContext, (char**) &np)) {
    return np;
  } else {
    return NULL;
  }
}

/** Find a client by its frame window. */
Client* Client::FindClientByParent(Window p) {
  Client *np;
  if (!XFindContext(display, p, frameContext, (char**) &np)) {
    return np;
  } else {
    return NULL;
  }
}

/** Reparent a client window. */
void Client::ReparentClient() {
  Client *np = this;
  XSetWindowAttributes attr;
  XEvent event;
  int attrMask;
  int x, y, width, height;
  int north, south, east, west;

  if ((this->getBorder() & (BORDER_TITLE | BORDER_OUTLINE)) == 0) {

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
    Border::GetBorderSize(this, &north, &south, &east, &west);
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

    if (this->isMapped()) {
      JXMapWindow(display, this->parent);
    }
  }

  JXSync(display, False);
  JXCheckTypedWindowEvent(display, this->window, UnmapNotify, &event);

}

/** Send a configure event to a client window. */
void Client::SendConfigureEvent() {

  XConfigureEvent event;
  const ScreenType *sp;

  memset(&event, 0, sizeof(event));
  event.display = display;
  event.type = ConfigureNotify;
  event.event = this->window;
  event.window = this->window;
  if (this->isFullscreen()) {
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
void Client::UpdateClientColormap(Colormap cmap) {
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
void Client::SignalUrgent(const TimeType *now, int x, int y, Window w,
    void *data) {
  Client *np = (Client*) data;

  /* Redraw borders. */
  if (np->shouldFlash()) {
    np->setNoFlash();
  } else if (!(np->isNotUrgent())) {
    np->setFlash();
  }
  Border::DrawBorder(np);
  Events::_RequireTaskUpdate();
  Events::_RequirePagerUpdate();

}

/** Unmap a client window and consume the UnmapNotify event. */
void Client::UnmapClient() {
  if (this->isMapped()) {
    XEvent e;

    /* Unmap the window and record that we did so. */
    this->setNotMapped();
    JXUnmapWindow(display, this->getWindow());

    /* Discard the unmap event so we don't process it later. */
    JXSync(display, False);
    if (JXCheckTypedWindowEvent(display, this->getWindow(), UnmapNotify, &e)) {
      Events::_UpdateTime(&e);
    }
  }
}

/** Read colormap information for a client. */
void Client::ReadWMColormaps() {

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
int Client::TryTileClient(const BoundingBox *box, int x, int y) {
  int layer;
  int north, south, east, west;
  int x1, x2, y1, y2;
  int ox1, ox2, oy1, oy2;
  int overlap;

  /* Set the client position. */
  Border::GetBorderSize(this, &north, &south, &east, &west);
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
  for (layer = this->getLayer(); layer < LAYER_COUNT; layer++) {
    std::vector<Client*> clients = ClientList::GetLayerList(layer);
    for (int i = 0; i < clients.size(); ++i) {
      Client *tp = clients[i];

      /* Skip clients that aren't visible. */
      if (!IsClientOnCurrentDesktop(tp)) {
        continue;
      }
      if (!(tp->isMapped())) {
        continue;
      }
      if (tp == this) {
        continue;
      }

      /* Get the boundaries of the other client. */
      Border::GetBorderSize(this, &north, &south, &east, &west);
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

/** Read the "normal hints" for a client. */
void Client::ReadWMNormalHints() {

  XSizeHints hints;
  long temp;

  if (!JXGetWMNormalHints(display, this->getWindow(), &hints, &temp)) {
    this->sizeFlags = 0;
  } else {
    this->sizeFlags = hints.flags;
  }

  if (this->sizeFlags & PResizeInc) {
    this->xinc = Max(1, hints.width_inc);
    this->yinc = Max(1, hints.height_inc);
  } else {
    this->xinc = 1;
    this->yinc = 1;
  }

  if (this->sizeFlags & PMinSize) {
    this->minWidth = Max(0, hints.min_width);
    this->minHeight = Max(0, hints.min_height);
  } else {
    this->minWidth = 1;
    this->minHeight = 1;
  }

  if (this->sizeFlags & PMaxSize) {
    this->maxWidth = hints.max_width;
    this->maxHeight = hints.max_height;
    if (this->maxWidth <= 0) {
      this->maxWidth = rootWidth;
    }
    if (this->maxHeight <= 0) {
      this->maxHeight = rootHeight;
    }
  } else {
    this->maxWidth = MAX_WINDOW_WIDTH;
    this->maxHeight = MAX_WINDOW_HEIGHT;
  }

  if (this->sizeFlags & PBaseSize) {
    this->baseWidth = hints.base_width;
    this->baseHeight = hints.base_height;
  } else if (this->sizeFlags & PMinSize) {
    this->baseWidth = this->minWidth;
    this->baseHeight = this->minHeight;
  } else {
    this->baseWidth = 0;
    this->baseHeight = 0;
  }

  if (this->sizeFlags & PAspect) {
    this->aspect.minx = hints.min_aspect.x;
    this->aspect.miny = hints.min_aspect.y;
    this->aspect.maxx = hints.max_aspect.x;
    this->aspect.maxy = hints.max_aspect.y;
  }

  if (this->sizeFlags & PWinGravity) {
    this->gravity = hints.win_gravity;
  } else {
    this->gravity = 1;
  }

}

/** Tiled placement. */

/** Move the window in the specified direction for reparenting. */
void Client::GravitateClient(char negate) {
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
void Client::DoSnapBorder() {

  Client *tp;
  ClientRectangle client, other;
  ClientRectangle left = { 0 };
  ClientRectangle right = { 0 };
  ClientRectangle top = { 0 };
  ClientRectangle bottom = { 0 };
  int layer;
  int north, south, east, west;

  GetClientRectangle(this, &client);

  Border::GetBorderSize(this, &north, &south, &east, &west);

  other.valid = 1;

  /* Work from the bottom of the window stack to the top. */
  for (layer = 0; layer < LAYER_COUNT; layer++) {

    /* Check tray windows. */
    std::vector<BoundingBox> boxes;
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
    std::vector<Client*> clients = ClientList::GetList();
    for (unsigned int i = 0; i < clients.size(); ++i) {
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
    if (!(this->isShaded())) {
      this->y -= this->getHeight();
    }
  }
  if (top.valid) {
    this->y = top.bottom + north;
  }

}

void Client::DrawBorder() {
  if (!(this->isStatus(STAT_HIDDEN | STAT_MINIMIZED))) {
    Border::DrawBorder(this);
  }
}

Window Client::getOwner() const {
  return this->owner;
}

void Client::setOwner(Window owner) {
  this->owner = owner;
}

int Client::getX() const {
  return this->x;
}

int Client::getY() const {
  return this->y;
}

void Client::setX(int x) {
  this->x = x;
}

void Client::setY(int y) {
  this->y = y;
}

void Client::setHeight(int height) {
  this->height = height;
}

void Client::setWidth(int width) {
  this->width = width;
}

const char* Client::getClassName() const {
  return this->className;
}

const char* Client::getInstanceName() {
  return this->instanceName;
}

Icon* Client::getIcon() const {
  return this->icon;
}

void Client::setIcon(Icon *icon) {
  this->icon = icon;
}

ColormapNode* Client::getColormaps() const {
  return this->colormaps;
}

Window Client::getWindow() const {
  return this->window;
}

Window Client::getParent() const {
  return this->parent;
}

const char* Client::getName() const {
  return this->name;
}

int Client::getWidth() const {
  return this->width;
}

int Client::getHeight() const {
  return this->height;
}

int Client::getGravity() const {
  return this->gravity;
}

MouseContextType Client::getMouseContext() const {
  return this->mouseContext;
}

void Client::setMouseContext(MouseContextType context) {
  this->mouseContext = context;
}

long int Client::getSizeFlags() {
  return this->sizeFlags;
}

void Client::setController(void (*controller)(int wasDestroyed)) {
  this->controller = controller;
}

void Client::resetMaxFlags() {
  this->maxFlags = MAX_NONE;
}

void Client::setDelete() {
  this->status |= STAT_DELETE;
}

void Client::setTakeFocus() {
  this->status |= STAT_TAKEFOCUS;
}

void Client::setNoDelete() {
  this->status &= ~STAT_DELETE;
}

void Client::setNoTakeFocus() {
  this->status &= ~STAT_TAKEFOCUS;
}

void Client::setLayer(unsigned char layer) {
  this->layer = layer;
}

void Client::setDefaultLayer(unsigned char defaultLayer) {
  this->defaultLayer = defaultLayer;
}

unsigned char Client::getDefaultLayer() const {
  return this->defaultLayer;
}

void Client::clearMaxFlags() {
  this->maxFlags = MAX_NONE;
}

void Client::resetBorder() {
  this->border = BORDER_DEFAULT;
}

void Client::resetLayer() {
  this->layer = LAYER_NORMAL;
}

void Client::resetDefaultLayer() {
  this->defaultLayer = LAYER_NORMAL;
}

void Client::clearBorder() {
  this->border = BORDER_NONE;
}

void Client::clearStatus() {
  this->status = STAT_NONE;
}

void Client::setNotMapped() {
  this->status &= ~STAT_MAPPED;
}

unsigned int Client::getOpacity() {
  return this->opacity;
}

unsigned short Client::getDesktop() const {
  return this->desktop;
}

void Client::setShaped() {
  this->status |= STAT_SHAPED;
}

void Client::resetLayerToDefault() {
  this->SetClientLayer(this->getDefaultLayer());
  this->layer = this->defaultLayer;
}

void Client::setDesktop(unsigned short desktop) {
  this->desktop = desktop;
}

unsigned char Client::getLayer() const {
  return this->layer;
}

unsigned short Client::getBorder() const {
  return this->border;
}

unsigned char Client::getMaxFlags() const {
  return this->maxFlags;
}

bool Client::isStatus(unsigned int flags) const {
  return (this->status & flags);
}

bool Client::isFullscreen() const {
  return isStatus(STAT_FULLSCREEN);
}

bool Client::isPosition() const {
  return isStatus(STAT_POSITION);
}

bool Client::isMapped() const {
  return isStatus(STAT_MAPPED);
}

bool Client::isShaded() const {
  return isStatus(STAT_SHADED);
}

bool Client::isMinimized() const {
  return isStatus(STAT_MINIMIZED);
}

bool Client::isShaped() const {
  return isStatus(STAT_SHAPED);
}

bool Client::isIgnoringProgramPosition() const {
  return isStatus(STAT_PIGNORE);
}

bool Client::isTiled() const {
  return isStatus(STAT_TILED);
}

bool Client::isCentered() const {
  return isStatus(STAT_CENTERED);
}

bool Client::isUrgent() const {
  return isStatus(STAT_URGENT);
}

bool Client::isDialogWindow() const {
  return isStatus(STAT_WMDIALOG);
}

bool Client::isHidden() const {
  return isStatus(STAT_HIDDEN);
}

bool Client::hasOpacity() const {
  return isStatus(STAT_OPACITY);
}

bool Client::isSticky() const {
  return isStatus(STAT_STICKY);
}

bool Client::isActive() const {
  return isStatus(STAT_ACTIVE);
}

bool Client::isFixed() const {
  return isStatus(STAT_FIXED);
}

bool Client::willIgnoreIncrementWhenMaximized() const {
  return isStatus(STAT_IIGNORE);
}

bool Client::canFocus() const {
  return isStatus(STAT_CANFOCUS);
}

bool Client::shouldTakeFocus() const {
  return isStatus(STAT_TAKEFOCUS);
}

bool Client::shouldDelete() const {
  return isStatus(STAT_DELETE);
}

bool Client::shouldFlash() const {
  return isStatus(STAT_FLASH);
}

bool Client::isNotUrgent() const {
  return isStatus(STAT_NOTURGENT);
}

bool Client::shouldSkipInTaskList() const {
  return isStatus(STAT_NOLIST);
}

bool Client::wasMinimizedToShowDesktop() const {
  return isStatus(STAT_SDESKTOP);
}

bool Client::isDragable() const {
  return isStatus(STAT_DRAG);
}

bool Client::isNotDraggable() const {
  return isStatus(STAT_NODRAG);
}

bool Client::shouldIgnoreSpecifiedList() const {
  return isStatus(STAT_ILIST);
}

bool Client::shouldIgnorePager() const {
  return isStatus(STAT_IPAGER);
}

bool Client::notFocusableIfMapped() const {
  return isStatus(STAT_NOFOCUS);
}

bool Client::shouldNotShowInPager() const {
  return isStatus(STAT_NOPAGER);
}

bool Client::isAeroSnapEnabled() const {
  return isStatus(STAT_AEROSNAP);
}

void Client::setActive() {
  this->status |= STAT_ACTIVE;
}

void Client::setNotActive() {
  this->status &= ~STAT_ACTIVE;
}

void Client::setMaxFlags(MaxFlags flags) {
  this->maxFlags = flags;
}

void Client::setOpacity(unsigned int opacity) {
  this->opacity = opacity;
}

//TODO: Rename these methods to be better understood
void Client::setDialogWindowStatus() {
  this->status |= STAT_WMDIALOG;
}

void Client::setSDesktopStatus() {
  this->status |= STAT_SDESKTOP;
}

void Client::setMapped() {
  this->status |= STAT_MAPPED;
}

void Client::setCanFocus() {
  this->status |= STAT_CANFOCUS;
}

void Client::setUrgent() {
  this->status |= STAT_URGENT;
}

void Client::setNoFlash() {
  this->status &= ~STAT_FLASH;
}

void Client::setShaded() {
  this->status |= STAT_SHADED;
}

void Client::setMinimized() {
  this->status |= STAT_MINIMIZED;
}

void Client::setNoPager() {
  this->status |= STAT_NOPAGER;
}

void Client::setNoFullscreen() {
  this->status &= ~STAT_FULLSCREEN;
}

void Client::setPositionFromConfig() {
  this->status |= STAT_POSITION;
}

void Client::setHasNoList() {
  this->status ^= STAT_NOLIST;
}

void Client::setNoShaded() {
  this->status &= ~STAT_SHADED;
}

void Client::setNoList() {
  this->status |= STAT_NOLIST;
}

void Client::setSticky() {
  this->status |= STAT_STICKY;
}

void Client::setNoSticky() {
  this->status &= ~STAT_STICKY;
}

void Client::setNoDrag() {
  this->status |= STAT_NODRAG;
}

void Client::setNoMinimized() {
  this->status &= ~STAT_MINIMIZED;
}

void Client::setNoSDesktop() {
  this->status &= ~STAT_SDESKTOP;
}

void Client::clearToNoList() {
  this->status &= ~STAT_NOLIST;
}

void Client::clearToNoPager() {
  this->status &= ~STAT_NOPAGER;
}

void Client::resetMappedState() {
  this->status &= ~STAT_MAPPED;
}

void Client::clearToSticky() {
  this->status &= ~STAT_STICKY;
}

void Client::setEdgeSnap() {
  this->status |= STAT_AEROSNAP;
}

void Client::setDrag() {
  this->status |= STAT_DRAG;
}

void Client::setFixed() {
  this->status |= STAT_FIXED;
}

void Client::setCurrentDesktop(unsigned int desktop) {
  this->desktop = desktop;
}

void Client::ignoreProgramList() {
  this->status |= STAT_PIGNORE;
}

void Client::ignoreProgramSpecificPager() {
  this->status |= STAT_IPAGER;
}

void Client::setFullscreen() {
  this->status |= STAT_FULLSCREEN;
}

void Client::setMaximized() {
  this->status |= MAX_HORIZ | MAX_VERT;
}

void Client::setCentered() {
  this->status |= STAT_CENTERED;
}

void Client::setFlash() {
  this->status |= STAT_FLASH;
}

void Client::setTiled() {
  this->status |= STAT_TILED;
}

void Client::setNotUrgent() {
  this->status |= STAT_NOTURGENT;
}

void Client::setTaskListSkipped() {
  this->status |= STAT_NOLIST;
}

void Client::setNoFocus() {
  this->status |= STAT_NOFOCUS;
}

void Client::setOpacityFixed() {
  this->status |= STAT_OPACITY;
}

void Client::ignoreProgramSpecificPosition() {
  this->status |= STAT_PIGNORE;
}

void Client::ignoreIncrementWhenMaximized() {
  this->status |= STAT_IIGNORE;
}

void Client::setBorderOutline() {
  /**< Window has a border. */
  this->border |= BORDER_OUTLINE;
}

void Client::setBorderTitle() {
  /**< Window has a title bar. */
  this->border |= BORDER_TITLE;
}

void Client::setBorderMin() {
  /**< Window supports minimize. */
  this->border |= BORDER_MIN;
}

void Client::setBorderMax() {
  /**< Window supports maximize. */
  this->border |= BORDER_MAX;
}

void Client::setBorderClose() {
  /**< Window supports close. */
  this->border |= BORDER_CLOSE;
}

void Client::setBorderResize() {
  /**< Window supports resizing. */
  this->border |= BORDER_RESIZE;
}

void Client::setBorderMove() {
  /**< Window supports moving. */
  this->border |= BORDER_MOVE;
}

void Client::setBorderMaxVert() {
  /**< Maximize vertically. */
  this->border |= BORDER_MAX_V;
}

void Client::setBorderMaxHoriz() {
  /**< Maximize horizontally. */
  this->border |= BORDER_MAX_H;
}

void Client::setBorderShade() {
  /**< Allow shading. */
  this->border |= BORDER_SHADE;
}

void Client::setBorderConstrain() {
  /**< Constrain to the screen. */
  this->border |= BORDER_CONSTRAIN;
}

void Client::setBorderFullscreen() {
  /**< Allow fullscreen. */
  this->border |= BORDER_FULLSCREEN;
}

void Client::setNoCanFocus() {
  this->status &= ~STAT_CANFOCUS;
}

void Client::setNoBorderOutline() {
  /**< Window has a border. */
  this->border &= ~BORDER_OUTLINE;
}

void Client::setNoBorderTitle() {
  /**< Window has a title bar. */
  this->border &= ~BORDER_TITLE;
}

void Client::setNoBorderMin() {
  /**< Window supports minimize. */
  this->border &= ~BORDER_MIN;
}

void Client::setNoBorderMax() {
  /**< Window supports maximize. */
  this->border &= ~BORDER_MAX;
}

void Client::setNoBorderClose() {
  /**< Window supports close. */
  this->border &= ~BORDER_CLOSE;
}

void Client::setNoBorderResize() {
  /**< Window supports resizing. */
  this->border &= ~BORDER_RESIZE;
}

void Client::setNoBorderMove() {
  /**< Window supports moving. */
  this->border &= ~BORDER_MOVE;
}

void Client::setNoBorderMaxVert() {
  /**< Maximize vertically. */
  this->border &= ~BORDER_MAX_V;
}

void Client::setNoBorderMaxHoriz() {
  /**< Maximize horizontally. */
  this->border &= ~BORDER_MAX_H;
}

void Client::setNoBorderShade() {
  /**< Allow shading. */
  this->border &= ~BORDER_SHADE;
}

void Client::setNoBorderConstrain() {
  /**< Constrain to the screen. */
  this->border &= ~BORDER_CONSTRAIN;
}

void Client::setNoBorderFullscreen() {
  /**< Allow fullscreen. */
  this->border &= ~BORDER_FULLSCREEN;
}

void Client::setNoUrgent() {
  this->status &= ~STAT_URGENT;
}

void Client::unsetNoPager() {
  this->status &= ~STAT_NOPAGER;
}

void Client::unsetSkippingInTaskList() {
  this->status &= ~STAT_NOLIST;
}

void Client::setNotHidden() {
  this->status &= ~STAT_HIDDEN;
}

void Client::setHidden() {
  this->status |= STAT_HIDDEN;
}

/** Update the size of a client window. */
void Client::UpdateSize(const MouseContextType context, const int x,
    const int y, const int startx, const int starty, const int oldx,
    const int oldy, const int oldw, const int oldh) {
  if (context & MC_BORDER_N) {
    int delta = (y - starty) / this->yinc;
    delta *= this->yinc;
    if (oldh - delta >= this->minHeight
        && (oldh - delta <= this->maxHeight || delta > 0)) {
      this->height = oldh - delta;
      this->y = oldy + delta;
    }
    if (!(context & (MC_BORDER_E | MC_BORDER_W))) {
      this->FixWidth();
    }
  }
  if (context & MC_BORDER_S) {
    int delta = (y - starty) / this->yinc;
    delta *= this->yinc;
    this->height = oldh + delta;
    this->height = Max(this->height, this->minHeight);
    this->height = Min(this->height, this->maxHeight);
    if (!(context & (MC_BORDER_E | MC_BORDER_W))) {
      this->FixWidth();
    }
  }
  if (context & MC_BORDER_E) {
    int delta = (x - startx) / this->xinc;
    delta *= this->xinc;
    this->width = oldw + delta;
    this->width = Max(this->width, this->minWidth);
    this->width = Min(this->width, this->maxWidth);
    if (!(context & (MC_BORDER_N | MC_BORDER_S))) {
      this->FixHeight();
    }
  }
  if (context & MC_BORDER_W) {
    int delta = (x - startx) / this->xinc;
    delta *= this->xinc;
    if (oldw - delta >= this->minWidth
        && (oldw - delta <= this->maxWidth || delta > 0)) {
      this->width = oldw - delta;
      this->x = oldx + delta;
    }
    if (!(context & (MC_BORDER_N | MC_BORDER_S))) {
      this->FixHeight();
    }
  }

  if (this->sizeFlags & PAspect) {
    if ((context & (MC_BORDER_N | MC_BORDER_S))
        && (context & (MC_BORDER_E | MC_BORDER_W))) {

      if (this->width * this->aspect.miny < this->height * this->aspect.minx) {
        const int delta = this->width;
        this->width = (this->height * this->aspect.minx) / this->aspect.miny;
        if (context & MC_BORDER_W) {
          this->x -= this->width - delta;
        }
      }
      if (this->width * this->aspect.maxy > this->height * this->aspect.maxx) {
        const int delta = this->height;
        this->height = (this->width * this->aspect.maxy) / this->aspect.maxx;
        if (context & MC_BORDER_N) {
          this->y -= this->height - delta;
        }
      }
    }
  }
}

bool shouldStopResize = false;
bool shouldStopMove = false;

void ResizeController(int wasDestroyed) {
  if (settings.resizeMode == RESIZE_OUTLINE) {
    Outline::ClearOutline();
  }
  JXUngrabPointer(display, CurrentTime);
  JXUngrabKeyboard(display, CurrentTime);
  DestroyResizeWindow();
  shouldStopResize = true;
}

/** Resize a client window (mouse initiated). */
void Client::ResizeClient(MouseContextType context, int startx,
    int starty) {

  XEvent event;
  int oldx, oldy;
  int oldw, oldh;
  int gwidth, gheight;
  int lastgwidth, lastgheight;
  int north, south, east, west;

  Assert(this);

  if (!(this->getBorder() & BORDER_RESIZE)) {
    return;
  }
  if (this->isFullscreen()) {
    return;
  }
  if (this->getMaxFlags()) {
    this->resetMaxFlags();
    Hints::WriteState(this);
    Border::ResetBorder(this);
  }
  if (JUNLIKELY(!Cursors::GrabMouseForResize(context))) {
    return;
  }

  this->controller = ResizeController;
  shouldStopResize = 0;

  oldx = this->x;
  oldy = this->y;
  oldw = this->width;
  oldh = this->height;

  gwidth = (this->width - this->baseWidth) / this->xinc;
  gheight = (this->height - this->baseHeight) / this->yinc;

  Border::GetBorderSize(this, &north, &south, &east, &west);

  startx += this->x - west;
  starty += this->y - north;

  CreateResizeWindow(this);
  UpdateResizeWindow(this, gwidth, gheight);

  if (!(Cursors::GetMouseMask() & (Button1Mask | Button3Mask))) {
    this->StopResize();
    return;
  }

  for (;;) {

    Events::_WaitForEvent(&event);

    if (shouldStopResize) {
      this->controller = NULL;
      return;
    }

    switch (event.type) {
    case ButtonRelease:
      if (event.xbutton.button == Button1 || event.xbutton.button == Button3) {
        this->StopResize();
        return;
      }
      break;
    case MotionNotify:

      Cursors::SetMousePosition(event.xmotion.x_root, event.xmotion.y_root,
          event.xmotion.window);
      Events::_DiscardMotionEvents(&event, this->window);

      this->UpdateSize(context, event.xmotion.x, event.xmotion.y, startx,
          starty, oldx, oldy, oldw, oldh);

      lastgwidth = gwidth;
      lastgheight = gheight;

      gwidth = (this->width - this->baseWidth) / this->xinc;
      gheight = (this->height - this->baseHeight) / this->yinc;

      if (lastgheight != gheight || lastgwidth != gwidth) {

        UpdateResizeWindow(this, gwidth, gheight);

        if (settings.resizeMode == RESIZE_OUTLINE) {
          Outline::ClearOutline();
          if (this->isShaded()) {
            Outline::DrawOutline(this->x - west, this->y - north,
                this->width + west + east, north + south);
          } else {
            Outline::DrawOutline(this->x - west, this->y - north,
                this->width + west + east, this->height + north + south);
          }
        } else {
          Border::ResetBorder(this);
          this->SendConfigureEvent();
        }

        Events::_RequirePagerUpdate();

      }

      break;
    default:
      break;
    }
  }

}

/** Resize a client window (keyboard or menu initiated). */
void Client::ResizeClientKeyboard(MouseContextType context) {

  XEvent event;
  int gwidth, gheight;
  int lastgwidth, lastgheight;
  int north, south, east, west;
  int startx, starty;
  int oldx, oldy, oldw, oldh;
  int ratio, minr, maxr;

  Assert(this);

  if (!(this->getBorder() & BORDER_RESIZE)) {
    return;
  }
  if (this->isFullscreen()) {
    return;
  }
  if (this->getMaxFlags()) {
    this->resetMaxFlags();
    Hints::WriteState(this);
    Border::ResetBorder(this);
  }

  if (JUNLIKELY(
      JXGrabKeyboard(display, this->parent, True, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)) {
    return;
  }
  if (!Cursors::GrabMouseForResize(context)) {
    JXUngrabKeyboard(display, CurrentTime);
    return;
  }

  this->controller = ResizeController;
  shouldStopResize = 0;

  oldx = this->x;
  oldy = this->y;
  oldw = this->width;
  oldh = this->height;

  gwidth = (this->width - this->baseWidth) / this->xinc;
  gheight = (this->height - this->baseHeight) / this->yinc;

  Border::GetBorderSize(this, &north, &south, &east, &west);

  CreateResizeWindow(this);
  UpdateResizeWindow(this, gwidth, gheight);

  if (context & MC_BORDER_N) {
    starty = this->y - north;
  } else if (context & MC_BORDER_S) {
    if (this->isShaded()) {
      starty = this->y;
    } else {
      starty = this->y + this->height;
    }
  } else {
    starty = this->y + this->height / 2;
  }
  if (context & MC_BORDER_W) {
    startx = this->x - west;
  } else if (context & MC_BORDER_E) {
    startx = this->x + this->width;
  } else {
    startx = this->x + this->width / 2;
  }
  Cursors::MoveMouse(rootWindow, startx, starty);
  Events::_DiscardMotionEvents(&event, this->window);

  for (;;) {

    Events::_WaitForEvent(&event);

    if (shouldStopResize) {
      this->controller = NULL;
      return;
    }

    if (event.type == KeyPress) {
      int deltax = 0;
      int deltay = 0;

      Events::_DiscardKeyEvents(&event, this->window);

      deltay -= deltay % this->yinc;
      this->height += deltay;
      this->height = Max(this->height, this->minHeight);
      this->height = Min(this->height, this->maxHeight);
      deltax -= deltax % this->xinc;
      this->width += deltax;
      this->width = Max(this->width, this->minWidth);
      this->width = Min(this->width, this->maxWidth);

      if (this->sizeFlags & PAspect) {

        ratio = (this->width << 16) / this->height;

        minr = (this->aspect.minx << 16) / this->aspect.miny;
        if (ratio < minr) {
          this->width = (this->height * minr) >> 16;
        }

        maxr = (this->aspect.maxx << 16) / this->aspect.maxy;
        if (ratio > maxr) {
          this->height = (this->width << 16) / maxr;
        }

      }

    } else if (event.type == MotionNotify) {

      Cursors::SetMousePosition(event.xmotion.x_root, event.xmotion.y_root,
          event.xmotion.window);
      Events::_DiscardMotionEvents(&event, this->window);

      this->UpdateSize(context, event.xmotion.x, event.xmotion.y, startx,
          starty, oldx, oldy, oldw, oldh);

    } else if (event.type == ButtonRelease) {
      this->StopResize();
      return;
    }

    lastgwidth = gwidth;
    lastgheight = gheight;
    gwidth = (this->width - this->baseWidth) / this->xinc;
    gheight = (this->height - this->baseHeight) / this->yinc;

    if (lastgwidth != gwidth || lastgheight != gheight) {

      UpdateResizeWindow(this, gwidth, gheight);

      if (settings.resizeMode == RESIZE_OUTLINE) {
        Outline::ClearOutline();
        if (this->isShaded()) {
          Outline::DrawOutline(this->x - west, this->y - north,
              this->width + west + east, north + south);
        } else {
          Outline::DrawOutline(this->x - west, this->y - north,
              this->width + west + east, this->height + north + south);
        }
      } else {
        Border::ResetBorder(this);
        this->SendConfigureEvent();
      }

      Events::_RequirePagerUpdate();

    }

  }

}

/** Stop a resize action. */
void Client::StopResize() {
  this->controller = NULL;

  /* Set the old width/height if maximized so the window
   * is restored to the new size. */
  if (this->getMaxFlags() & MAX_VERT) {
    this->oldWidth = this->width;
    this->oldx = this->x;
  }
  if (this->getMaxFlags() & MAX_HORIZ) {
    this->oldHeight = this->height;
    this->oldy = this->y;
  }

  if (settings.resizeMode == RESIZE_OUTLINE) {
    Outline::ClearOutline();
  }

  JXUngrabPointer(display, CurrentTime);
  JXUngrabKeyboard(display, CurrentTime);

  DestroyResizeWindow();

  Border::ResetBorder(this);
  this->SendConfigureEvent();

}

/** Fix the width to match the aspect ratio. */
void Client::FixWidth() {
  if ((this->sizeFlags & PAspect) && this->height > 0) {
    if (this->width * this->aspect.miny < this->height * this->aspect.minx) {
      this->width = (this->height * this->aspect.minx) / this->aspect.miny;
    }
    if (this->width * this->aspect.maxy > this->height * this->aspect.maxx) {
      this->width = (this->height * this->aspect.maxx) / this->aspect.maxy;
    }
  }
}

/** Fix the height to match the aspect ratio. */
void Client::FixHeight() {
  if ((this->sizeFlags & PAspect) && this->height > 0) {
    if (this->width * this->aspect.miny < this->height * this->aspect.minx) {
      this->height = (this->width * this->aspect.miny) / this->aspect.minx;
    }
    if (this->width * this->aspect.maxy > this->height * this->aspect.maxx) {
      this->height = (this->width * this->aspect.maxy) / this->aspect.maxx;
    }
  }
}

/** Centered placement. */
void Client::CenterClient(const BoundingBox *box) {
  this->x = box->x + (box->width / 2) - (this->width / 2);
  this->y = box->y + (box->height / 2) - (this->height / 2);
  this->ConstrainSize();
  this->ConstrainPosition();
}

/** Stop an active pager move. */
void Client::StopPagerMove(int x, int y, int desktop, MaxFlags maxFlags) {

  int north, south, east, west;

  Assert(this->controller);

  /* Release grabs. */
  (this->controller)(0);

  this->x = x;
  this->y = y;

  Border::GetBorderSize(this, &north, &south, &east, &west);
  JXMoveWindow(display, this->parent, this->getX() - west,
      this->getY() - north);
  this->SendConfigureEvent();

  /* Restore the maximized state of the client. */
  if (maxFlags != MAX_NONE) {
    this->MaximizeClient(maxFlags);
  }

  /* Redraw the pager. */
  Events::_RequirePagerUpdate();

}

/** Callback for stopping moves. */
void MoveController(int wasDestroyed) {
  if (settings.moveMode == MOVE_OUTLINE) {
    Outline::ClearOutline();
  }

  JXUngrabPointer(display, CurrentTime);
  JXUngrabKeyboard(display, CurrentTime);

  DestroyMoveWindow();
  shouldStopMove = true;
  atTop = 0;
  atBottom = 0;
  atLeft = 0;
  atRight = 0;
  atSideFirst = 0;
}

/** Move a client window. */
char Client::MoveClient(int startx, int starty) {
  XEvent event;
  const ScreenType *sp;
  MaxFlags flags;
  int oldx, oldy;
  int doMove;
  int north, south, east, west;
  int height;

  if (!(this->getBorder() & BORDER_MOVE)) {
    return 0;
  }
  if (this->isFullscreen()) {
    return 0;
  }

  if (!Cursors::GrabMouseForMove()) {
    return 0;
  }

  Events::_RegisterCallback(0, SignalMove, NULL);
  this->controller = MoveController;
  shouldStopMove = false;

  oldx = this->getX();
  oldy = this->getY();

  if (!(Cursors::GetMouseMask() & (Button1Mask | Button2Mask))) {
    this->StopMove(0, oldx, oldy);
    return 0;
  }

  Border::GetBorderSize(this, &north, &south, &east, &west);
  startx -= west;
  starty -= north;

  currentClient = this;
  atTop = atBottom = atLeft = atRight = atSideFirst = 0;
  doMove = 0;
  for (;;) {

    Events::_WaitForEvent(&event);

    if (shouldStopMove) {
      this->controller = NULL;
      Cursors::SetDefaultCursor(this->parent);
      Events::_UnregisterCallback(SignalMove, NULL);
      return doMove;
    }

    switch (event.type) {
    case ButtonRelease:
      if (event.xbutton.button == Button1 || event.xbutton.button == Button2) {
        this->StopMove(doMove, oldx, oldy);
        return doMove;
      }
      break;
    case MotionNotify:

      Events::_DiscardMotionEvents(&event, this->getWindow());

      this->x = event.xmotion.x_root - startx;
      this->y = event.xmotion.y_root - starty;

      /* Get the move time used for desktop switching. */
      if (!(atLeft | atTop | atRight | atBottom)) {
        if (event.xmotion.state & Mod1Mask) {
          moveTime.seconds = 0;
          moveTime.ms = 0;
        } else {
          GetCurrentTime(&moveTime);
        }
      }

      /* Determine if we are at a border for desktop switching. */
      sp = Screens::GetCurrentScreen(this->getX() + this->getWidth() / 2,
          this->getY() + this->getHeight() / 2);
      atLeft = atTop = atRight = atBottom = 0;
      if (event.xmotion.x_root <= sp->x) {
        atLeft = 1;
      } else if (event.xmotion.x_root >= sp->x + sp->width - 1) {
        atRight = 1;
      }
      if (event.xmotion.y_root <= sp->y) {
        atTop = 1;
      } else if (event.xmotion.y_root >= sp->y + sp->height - 1) {
        atBottom = 1;
      }

      flags = MAX_NONE;
      if (event.xmotion.state & Mod1Mask) {
        /* Switch desktops immediately if alt is pressed. */
        if (atLeft | atRight | atTop | atBottom) {
          TimeType now;
          GetCurrentTime(&now);
          UpdateDesktop(&now);
        }
      } else {
        /* If alt is not pressed, snap to borders. */
        if (this->isAeroSnapEnabled()) {
          if (atTop & atLeft) {
            if (atSideFirst) {
              flags = MAX_TOP | MAX_LEFT;
            } else {
              flags = MAX_TOP | MAX_HORIZ;
            }
          } else if (atTop & atRight) {
            if (atSideFirst) {
              flags = MAX_TOP | MAX_RIGHT;
            } else {
              flags = MAX_TOP | MAX_HORIZ;
            }
          } else if (atBottom & atLeft) {
            if (atSideFirst) {
              flags = MAX_BOTTOM | MAX_LEFT;
            } else {
              flags = MAX_BOTTOM | MAX_HORIZ;
            }
          } else if (atBottom & atRight) {
            if (atSideFirst) {
              flags = MAX_BOTTOM | MAX_RIGHT;
            } else {
              flags = MAX_BOTTOM | MAX_HORIZ;
            }
          } else if (atLeft) {
            flags = MAX_LEFT | MAX_VERT;
            atSideFirst = 1;
          } else if (atRight) {
            flags = MAX_RIGHT | MAX_VERT;
            atSideFirst = 1;
          } else if (atTop | atBottom) {
            flags = MAX_VERT | MAX_HORIZ;
            atSideFirst = 0;
          }
          if (flags != this->getMaxFlags()) {
            if (settings.moveMode == MOVE_OUTLINE) {
              Outline::ClearOutline();
            }
            this->MaximizeClient(flags);
          }
          if (!this->getMaxFlags()) {
            DoSnap(this);
          }
        } else {
          DoSnap(this);
        }
      }

      if (flags != MAX_NONE) {
        this->RestartMove(&doMove);
      } else if (!doMove
          && (abs(this->getX() - oldx) > MOVE_DELTA
              || abs(this->getY() - oldy) > MOVE_DELTA)) {

        if (this->getMaxFlags()) {
          this->MaximizeClient(MAX_NONE);
        }

        CreateMoveWindow(this);
        doMove = 1;
      }

      if (doMove) {
        if (settings.moveMode == MOVE_OUTLINE) {
          Outline::ClearOutline();
          height = north + south;
          if (!(this->isShaded())) {
            height += this->getHeight();
          }
          Outline::DrawOutline(this->getX() - west, this->getY() - north,
              this->getWidth() + west + east, height);
        } else {
          if (this->getParent() != None) {
            JXMoveWindow(display, this->getParent(), this->getX() - west,
                this->getY() - north);
          } else {
            JXMoveWindow(display, this->getWindow(), this->getX(),
                this->getY());
          }
          this->SendConfigureEvent();
        }
        UpdateMoveWindow(this);
        Events::_RequirePagerUpdate();
      }

      break;
    default:
      break;
    }
  }
}

/** Move a client window (keyboard or menu initiated). */
char Client::MoveClientKeyboard() {
  XEvent event;
  int oldx, oldy;
  int moved;
  int height;
  int north, south, east, west;
  Window win;
  if (!(this->getBorder() & BORDER_MOVE)) {
    return 0;
  }
  if (this->isFullscreen()) {
    return 0;
  }

  if (this->getMaxFlags() != MAX_NONE) {
    this->MaximizeClient(MAX_NONE);
  }

  win = this->parent != None ? this->parent : this->window;
  if (JUNLIKELY(
      JXGrabKeyboard(display, win, True, GrabModeAsync, GrabModeAsync, CurrentTime))) {
    Debug("MoveClient: could not grab keyboard");
    return 0;
  }
  if (!Cursors::GrabMouseForMove()) {
    JXUngrabKeyboard(display, CurrentTime);
    return 0;
  }

  Border::GetBorderSize(this, &north, &south, &east, &west);

  oldx = this->getX();
  oldy = this->getY();

  Events::_RegisterCallback(0, SignalMove, NULL);
  this->controller = MoveController;
  shouldStopMove = false;

  CreateMoveWindow(this);
  UpdateMoveWindow(this);

  Cursors::MoveMouse(rootWindow, this->getX(), this->getY());
  Events::_DiscardMotionEvents(&event, this->window);

  if (this->isShaded()) {
    height = 0;
  } else {
    height = this->getHeight();
  }
  currentClient = this;

  for (;;) {

    Events::_WaitForEvent(&event);

    if (shouldStopMove) {
      this->controller = NULL;
      Cursors::SetDefaultCursor(this->parent);
      Events::_UnregisterCallback(SignalMove, NULL);
      return 1;
    }

    moved = 0;

    if (event.type == KeyPress) {

      Events::_DiscardKeyEvents(&event, this->window);

      Cursors::MoveMouse(rootWindow, this->getX(), this->getY());
      Events::_DiscardMotionEvents(&event, this->window);

      moved = 1;

    } else if (event.type == MotionNotify) {

      Events::_DiscardMotionEvents(&event, this->window);

      this->x = event.xmotion.x;
      this->y = event.xmotion.y;

      moved = 1;

    } else if (event.type == ButtonRelease) {
      this->StopMove(1, oldx, oldy);
      return 1;

    }

    if (moved) {

      if (settings.moveMode == MOVE_OUTLINE) {
        Outline::ClearOutline();
        Outline::DrawOutline(this->getX() - west, this->getY() - west,
            this->getWidth() + west + east, height + north + west);
      } else {
        JXMoveWindow(display, win, this->getX() - west, this->getY() - north);
        this->SendConfigureEvent();
      }

      UpdateMoveWindow(this);
      Events::_RequirePagerUpdate();

    }

  }

}

/** Stop move. */
void Client::StopMove(int doMove, int oldx, int oldy) {
  int north, south, east, west;

  Assert(this->controller); //must be moving

  (this->controller)(0);

  this->controller = NULL;

  Cursors::SetDefaultCursor(this->parent);
  Events::_UnregisterCallback(SignalMove, NULL);

  if (!doMove) {
    this->x = oldx;
    this->y = oldy;
    return;
  }

  Border::GetBorderSize(this, &north, &south, &east, &west);
  if (this->parent != None) {
    JXMoveWindow(display, this->parent, this->getX() - west,
        this->getY() - north);
  } else {
    JXMoveWindow(display, this->window, this->getX() - west,
        this->getY() - north);
  }
  this->SendConfigureEvent();
}

/** Restart a move. */
void Client::RestartMove(int *doMove) {
  if (*doMove) {
    int north, south, east, west;
    *doMove = 0;
    DestroyMoveWindow();
    Border::GetBorderSize(this, &north, &south, &east, &west);
    if (this->getParent() != None) {
      JXMoveWindow(display, this->getParent(), this->getX() - west,
          this->getY() - north);
    } else {
      JXMoveWindow(display, this->getWindow(), this->getX() - west,
          this->getY() - north);
    }
    this->SendConfigureEvent();
  }
}

/** Snap to the screen. */
void Client::DoSnapScreen() {

  ClientRectangle client;
  int screen;
  const ScreenType *sp;
  int screenCount;
  int north, south, east, west;

  GetClientRectangle(this, &client);

  Border::GetBorderSize(this, &north, &south, &east, &west);

  screenCount = Screens::GetScreenCount();
  for (screen = 0; screen < screenCount; screen++) {

    sp = Screens::GetScreen(screen);

    if (abs(client.right - sp->width - sp->x) <= settings.snapDistance) {
      this->x = sp->x + sp->width - west - this->getWidth();
    }
    if (abs(client.left - sp->x) <= settings.snapDistance) {
      this->x = sp->x + east;
    }
    if (abs(client.bottom - sp->height - sp->y) <= settings.snapDistance) {
      this->y = sp->y + sp->height - south;
      if (!(this->isShaded())) {
        this->y -= this->getHeight();
      }
    }
    if (abs(client.top - sp->y) <= settings.snapDistance) {
      this->y = north + sp->y;
    }

  }

}

