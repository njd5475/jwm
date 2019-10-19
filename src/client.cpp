/**
 * @file client.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client window functions.
 *
 */

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

#include <X11/Xlibint.h>

ClientNode *ClientNode::activeClient;

unsigned int ClientNode::clientCount;

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
  for (x = 0; x < LAYER_COUNT; x++) {
    nodes[x] = NULL;
    nodeTail[x] = NULL;
  }

  /* Query client windows. */
  JXQueryTree(display, rootWindow, &rootReturn, &parentReturn, &childrenReturn, &childrenCount);

  /* Add each client. */
  for (x = 0; x < childrenCount; x++) {
    if (JXGetWindowAttributes(display, childrenReturn[x], &attr)) {
      if (attr.override_redirect == False && attr.map_state == IsViewable) {
        new ClientNode(childrenReturn[x], 1, 1);
      }
    }
  }

  JXFree(childrenReturn);

  LoadFocus();

  _RequireTaskUpdate();
  _RequirePagerUpdate();

}

/** Release client windows. */
void ClientNode::ShutdownClients(void) {

  int x;

  for (x = 0; x < LAYER_COUNT; x++) {
    while (nodeTail[x]) {
      nodeTail[x]->RemoveClient();
    }
  }

}

/** Set the focus to the window currently under the mouse pointer. */
void ClientNode::LoadFocus(void) {

  ClientNode *np;
  Window rootReturn, childReturn;
  int rootx, rooty;
  int winx, winy;
  unsigned int mask;

  JXQueryPointer(display, rootWindow, &rootReturn, &childReturn, &rootx, &rooty, &winx, &winy, &mask);

  np = FindClient(childReturn);
  if (np) {
    np->FocusClient();
  }

}

/** Add a window to management. */
ClientNode::ClientNode(Window w, char alreadyMapped, char notOwner) :
    oldx(0), oldy(0), oldWidth(0), oldHeight(0), yinc(0), minWidth(0), maxWidth(0), minHeight(0), maxHeight(0), gravity(
        0), controller(0), instanceName(0), baseHeight(0), baseWidth(0), colormaps(), icon(0), sizeFlags(0), className(
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
  this->state.desktop = currentDesktop;

  this->x = attr.x;
  this->y = attr.y;
  this->width = attr.width;
  this->height = attr.height;
  this->cmap = attr.colormap;
  this->state.status = STAT_NONE;
  this->state.maxFlags = MAX_NONE;
  this->state.layer = LAYER_NORMAL;
  this->state.defaultLayer = LAYER_NORMAL;

  this->state.border = BORDER_DEFAULT;
  this->mouseContext = MC_NONE;

  ReadClientInfo(this, alreadyMapped);

  if (!notOwner) {
    this->state.border = BORDER_OUTLINE | BORDER_TITLE | BORDER_MOVE;
    this->state.status |= STAT_WMDIALOG | STAT_STICKY;
    this->state.layer = LAYER_ABOVE;
    this->state.defaultLayer = LAYER_ABOVE;
  }

  ApplyGroups(this);
  if (this->icon == NULL) {
    this->LoadIcon();
  }

  /* We now know the layer, so insert */
  this->prev = NULL;
  this->next = nodes[this->state.layer];
  if (this->next) {
    this->next->prev = this;
  } else {
    nodeTail[this->state.layer] = this;
  }
  nodes[this->state.layer] = this;

  Cursors::SetDefaultCursor(this->window);

  if (notOwner) {
    XSetWindowAttributes sattr;
    JXAddToSaveSet(display, this->window);
    sattr.event_mask = EnterWindowMask | ColormapChangeMask | PropertyChangeMask | KeyReleaseMask | StructureNotifyMask;
    sattr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask
        | KeyReleaseMask;
    JXChangeWindowAttributes(display, this->window, CWEventMask | CWDontPropagate, &sattr);
  }
  JXGrabButton(display, AnyButton, AnyModifier, this->window, True, ButtonPressMask, GrabModeSync, GrabModeAsync, None,
      None);

  PlaceClient(this, alreadyMapped);
  this->ReparentClient();
  XSaveContext(display, this->window, clientContext, (char*) this);

  if (this->state.status & STAT_MAPPED) {
    JXMapWindow(display, this->window);
  }

  clientCount += 1;

  if (!alreadyMapped) {
    this->RaiseClient();
  }

  if (this->state.status & STAT_OPACITY) {
    this->SetOpacity(this->state.opacity, 1);
  } else {
    this->SetOpacity(settings.inactiveClientOpacity, 1);
  }
  if (this->state.status & STAT_STICKY) {
    SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP, ~0UL);
  } else {
    SetCardinalAtom(this->window, ATOM_NET_WM_DESKTOP, this->state.desktop);
  }

  /* Shade the client if requested. */
  if (this->state.status & STAT_SHADED) {
    this->state.status &= ~STAT_SHADED;
    this->ShadeClient();
  }

  /* Minimize the client if requested. */
  if (this->state.status & STAT_MINIMIZED) {
    this->state.status &= ~STAT_MINIMIZED;
    this->MinimizeClient(0);
  }

  /* Maximize the client if requested. */
  if (this->state.maxFlags) {
    const MaxFlags flags = this->state.maxFlags;
    this->state.maxFlags = MAX_NONE;
    this->MaximizeClient(flags);
  }

  if (this->state.status & STAT_URGENT) {
    _RegisterCallback(URGENCY_DELAY, SignalUrgent, this);
  }

  /* Update task bars. */
  TaskBarType::AddClientToTaskBar(this);

  /* Make sure we're still in sync */
  WriteState(this);
  this->SendConfigureEvent();

  /* Hide the client if we're not on the right desktop. */
  if (this->state.desktop != currentDesktop && !(this->state.status & STAT_STICKY)) {
    this->HideClient();
  }

  ReadClientStrut(this);

  /* Focus transients if their parent has focus. */
  if (this->owner != None) {
    if (activeClient && this->owner == activeClient->window) {
      this->FocusClient();
    }
  }

  /* Make the client fullscreen if requested. */
  if (this->state.status & STAT_FULLSCREEN) {
    this->state.status &= ~STAT_FULLSCREEN;
    this->SetClientFullScreen(1);
  }
  Border::ResetBorder(this);
}

/** Minimize a client window and all of its transients. */
void ClientNode::MinimizeClient(char lower) {
  Assert(np);
  this->MinimizeTransients(lower);
  _RequireRestack();
  _RequireTaskUpdate();
}

/** Minimize all transients as well as the specified client. */
void ClientNode::MinimizeTransients(char lower) {

  ClientNode *tp;
  int x;

  Assert(np);

  /* Unmap the window and update its state. */
  if (this->state.status & (STAT_MAPPED | STAT_SHADED)) {
    this->UnmapClient();
    if (this->parent != None) {
      JXUnmapWindow(display, this->parent);
    }
  }
  this->state.status |= STAT_MINIMIZED;

  /* Minimize transient windows. */
  for (x = 0; x < LAYER_COUNT; x++) {
    tp = nodes[x];
    while (tp) {
      ClientNode *next = tp->next;
      if (tp->owner == this->window && (tp->state.status & (STAT_MAPPED | STAT_SHADED))
          && !(tp->state.status & STAT_MINIMIZED)) {
        tp->MinimizeTransients(lower);
      }
      tp = next;
    }
  }

  /* Focus the next window. */
  if (this->state.status & STAT_ACTIVE) {
    FocusNextStacked(this);
  }

  if (lower) {
    /* Move this client to the end of the layer list. */
    if (nodeTail[this->state.layer] != this) {
      if (this->prev) {
        this->prev->next = this->next;
      } else {
        nodes[this->state.layer] = this->next;
      }
      this->next->prev = this->prev;
      tp = nodeTail[this->state.layer];
      nodeTail[this->state.layer] = this;
      tp->next = this;
      this->prev = tp;
      this->next = NULL;
    }
  }

  WriteState(this);

}

/** Shade a client. */
void ClientNode::ShadeClient() {

  Assert(np);

  if ((this->state.status & (STAT_SHADED | STAT_FULLSCREEN)) || !(this->state.border & BORDER_SHADE)) {
    return;
  }

  this->UnmapClient();
  this->state.status |= STAT_SHADED;

  WriteState(this);
  Border::ResetBorder(this);
  _RequirePagerUpdate();

}

/** Unshade a client. */
void ClientNode::UnshadeClient() {

  Assert(np);

  if (!(this->state.status & STAT_SHADED)) {
    return;
  }

  if (!(this->state.status & (STAT_MINIMIZED | STAT_SDESKTOP))) {
    JXMapWindow(display, this->window);
    this->state.status |= STAT_MAPPED;
  }
  this->state.status &= ~STAT_SHADED;

  WriteState(this);
  Border::ResetBorder(this);
  RefocusClient();
  _RequirePagerUpdate();

}

/** Set a client's state to withdrawn. */
void ClientNode::SetClientWithdrawn() {

  Assert(np);

  if (activeClient == this) {
    activeClient = NULL;
    this->state.status &= ~STAT_ACTIVE;
    FocusNextStacked(this);
  }

  if (this->state.status & STAT_MAPPED) {
    this->UnmapClient();
    if (this->parent != None) {
      JXUnmapWindow(display, this->parent);
    }
  } else if (this->state.status & STAT_SHADED) {
    if (!(this->state.status & STAT_MINIMIZED)) {
      if (this->parent != None) {
        JXUnmapWindow(display, this->parent);
      }
    }
  }

  this->state.status &= ~STAT_SHADED;
  this->state.status &= ~STAT_MINIMIZED;
  this->state.status &= ~STAT_SDESKTOP;

  WriteState(this);
  _RequireTaskUpdate();
  _RequirePagerUpdate();

}

/** Restore a window with its transients (helper method). */
void ClientNode::RestoreTransients(char raise) {

  ClientNode *tp;
  int x;

  /* Make sure this window is on the current desktop. */
  this->SetClientDesktop(currentDesktop);

  /* Restore this window. */
  if (!(this->state.status & STAT_MAPPED)) {
    if (this->state.status & STAT_SHADED) {
      if (this->parent != None) {
        JXMapWindow(display, this->parent);
      }
    } else {
      JXMapWindow(display, this->window);
      if (this->parent != None) {
        JXMapWindow(display, this->parent);
      }
      this->state.status |= STAT_MAPPED;
    }
  }
  this->state.status &= ~STAT_MINIMIZED;
  this->state.status &= ~STAT_SDESKTOP;

  /* Restore transient windows. */
  for (x = 0; x < LAYER_COUNT; x++) {
    for (tp = nodes[x]; tp; tp = tp->next) {
      if (tp->owner == this->window && (tp->state.status & STAT_MINIMIZED)) {
        tp->RestoreTransients(raise);
      }
    }
  }

  if (raise) {
    this->FocusClient();
    this->RaiseClient();
  }
  WriteState(this);

}

/** Restore a client window and its transients. */
void ClientNode::RestoreClient(char raise) {
  if ((this->state.status & STAT_FIXED) && !(this->state.status & STAT_STICKY)) {
    DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(this->state.desktop);
  }
  this->RestoreTransients(raise);
  _RequireRestack();
  _RequireTaskUpdate();
}

/** Update window state information. */
void ClientNode::_UpdateState() {
  const char alreadyMapped = (this->getState()->status & STAT_MAPPED) ? 1 : 0;
  const char active = (this->getState()->status & STAT_ACTIVE) ? 1 : 0;

  /* Remove from the layer list. */
  if (this->getPrev() != NULL) {
    this->prev->next = this->getNext();
  } else {
    Assert(nodes[np->getState()->layer] == np);
    nodes[this->getState()->layer] = this->getNext();
  }
  if (this->getNext() != NULL) {
    this->getNext()->prev = this->getPrev();
  } else {
    Assert(nodeTail[np->getState()->layer] == np);
    nodeTail[this->getState()->layer] = this->getPrev();
  }

  /* Read the state (and new layer). */
  if (this->getState()->status & STAT_URGENT) {
    _UnregisterCallback(ClientNode::SignalUrgent, this);
  }
  this->state = ReadWindowState(this->getWindow(), alreadyMapped);
  if (this->getState()->status & STAT_URGENT) {
    _RegisterCallback(URGENCY_DELAY, ClientNode::SignalUrgent, this);
  }

  /* We don't handle mapping the window, so restore its mapped state. */
  if (!alreadyMapped) {
    this->getState()->status &= ~STAT_MAPPED;
  }

  /* Add to the layer list. */
  this->prev = NULL;
  this->next = nodes[this->getState()->layer];
  if (this->getNext() == NULL) {
    nodeTail[this->getState()->layer] = this;
  } else {
    this->getNext()->prev = this;
  }
  nodes[this->getState()->layer] = this;

  if (active) {
    this->FocusClient();
  }

}

void ClientNode::UpdateWindowState(char alreadyMapped) {
  /* Read the window state. */
  this->state = ReadWindowState(this->window, alreadyMapped);
  if (this->minWidth == this->maxWidth && this->minHeight == this->maxHeight) {
    this->state.border &= ~BORDER_RESIZE;
    this->state.border &= ~BORDER_MAX;
    if (this->minWidth * this->xinc >= rootWidth && this->minHeight * this->yinc >= rootHeight) {
      this->state.status |= STAT_FULLSCREEN;
    }
  }

  /* Make sure this client is on at least as high of a layer
   * as its owner. */
  ClientNode *pp;
  if (this->owner != None) {
    pp = FindClientByWindow(this->owner);
    if (pp) {
      this->state.layer = Max(pp->state.layer, this->state.layer);
    }
  }
}

/** Set the client layer. This will affect transients. */
void ClientNode::SetClientLayer(unsigned int layer) {

  ClientNode *tp, *next;

  Assert(np);
  Assert(layer <= LAST_LAYER);

  if (this->state.layer != layer) {
    int x;

    /* Loop through all clients so we get transients. */
    for (x = FIRST_LAYER; x <= LAST_LAYER; x++) {
      tp = nodes[x];
      while (tp) {
        next = tp->next;
        if (tp == this || tp->owner == this->window) {

          /* Remove from the old node list */
          if (next) {
            next->prev = tp->prev;
          } else {
            nodeTail[tp->state.layer] = tp->prev;
          }
          if (tp->prev) {
            tp->prev->next = next;
          } else {
            nodes[tp->state.layer] = next;
          }

          /* Insert into the new node list */
          tp->prev = NULL;
          tp->next = nodes[layer];
          if (nodes[layer]) {
            nodes[layer]->prev = tp;
          } else {
            nodeTail[layer] = tp;
          }
          nodes[layer] = tp;

          /* Set the new layer */
          tp->state.layer = layer;
          WriteState(tp);

        }
        tp = next;
      }
    }

    _RequireRestack();

  }

}

/** Set a client's sticky status. This will update transients. */
void ClientNode::SetClientSticky(char isSticky) {

  ClientNode *tp;
  int x;
  char old;

  /* Get the old sticky status. */
  if (this->state.status & STAT_STICKY) {
    old = 1;
  } else {
    old = 0;
  }

  if (isSticky && !old) {

    /* Change from non-sticky to sticky. */

    for (x = 0; x < LAYER_COUNT; x++) {
      for (tp = nodes[x]; tp; tp = tp->next) {
        if (tp == this || tp->owner == this->window) {
          tp->state.status |= STAT_STICKY;
          SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, ~0UL);
          WriteState(tp);
        }
      }
    }

  } else if (!isSticky && old) {

    /* Change from sticky to non-sticky. */

    for (x = 0; x < LAYER_COUNT; x++) {
      for (tp = nodes[x]; tp; tp = tp->next) {
        if (tp == this || tp->owner == this->window) {
          tp->state.status &= ~STAT_STICKY;
          WriteState(tp);
        }
      }
    }

    /* Since this client is no longer sticky, we need to assign
     * a desktop. Here we use the current desktop.
     * Note that SetClientDesktop updates transients (which is good).
     */
    this->SetClientDesktop(currentDesktop);

  }

}

/** Set a client's desktop. This will update transients. */
void ClientNode::SetClientDesktop(unsigned int desktop) {

  ClientNode *tp;

  Assert(np);

  if (JUNLIKELY(desktop >= settings.desktopCount)) {
    return;
  }

  if (!(this->state.status & STAT_STICKY)) {
    int x;
    for (x = 0; x < LAYER_COUNT; x++) {
      for (tp = nodes[x]; tp; tp = tp->next) {
        if (tp == this || tp->owner == this->window) {

          tp->state.desktop = desktop;

          if (desktop == currentDesktop) {
            tp->ShowClient();
          } else {
            tp->HideClient();
          }

          SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, tp->state.desktop);
        }
      }
    }
    _RequirePagerUpdate();
    _RequireTaskUpdate();
  }

}

/** Hide a client. This will not update transients. */
void ClientNode::HideClient() {
  if (!(this->state.status & STAT_HIDDEN)) {
    if (activeClient == this) {
      activeClient = NULL;
    }
    this->state.status |= STAT_HIDDEN;
    if (this->state.status & (STAT_MAPPED | STAT_SHADED)) {
      if (this->parent != None) {
        JXUnmapWindow(display, this->parent);
      } else {
        JXUnmapWindow(display, this->window);
      }
    }
  }
}

/** Show a hidden client. This will not update transients. */
void ClientNode::ShowClient() {
  if (this->state.status & STAT_HIDDEN) {
    this->state.status &= ~STAT_HIDDEN;
    if (this->state.status & (STAT_MAPPED | STAT_SHADED)) {
      if (!(this->state.status & STAT_MINIMIZED)) {
        if (this->parent != None) {
          JXMapWindow(display, this->parent);
        } else {
          JXMapWindow(display, this->window);
        }
        if (this->state.status & STAT_ACTIVE) {
          this->FocusClient();
        }
      }
    }
  }
}

/** Maximize a client window. */
void ClientNode::MaximizeClient(MaxFlags flags) {

  /* Don't allow maximization of full-screen clients. */
  if (this->state.status & STAT_FULLSCREEN) {
    return;
  }
  if (!(this->state.border & BORDER_MAX)) {
    return;
  }

  if (this->state.status & STAT_SHADED) {
    this->UnshadeClient();
  }

  if (this->state.status & STAT_MINIMIZED) {
    this->RestoreClient(1);
  }

  this->RaiseClient();
  this->FocusClient();
  if (this->state.maxFlags) {
    /* Undo existing maximization. */
    this->x = this->oldx;
    this->y = this->oldy;
    this->width = this->oldWidth;
    this->height = this->oldHeight;
    this->state.maxFlags = MAX_NONE;
  }
  if (flags != MAX_NONE) {
    /* Maximize if requested. */
    this->PlaceMaximizedClient(flags);
  }

  WriteState(this);
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

  Assert(np);

  if (this->state.maxFlags == MAX_NONE) {
    if (this->state.border & BORDER_MAX_H) {
      flags |= MAX_HORIZ;
    }
    if (this->state.border & BORDER_MAX_V) {
      flags |= MAX_VERT;
    }
  }

  this->MaximizeClient(flags);

}

/** Set a client's full screen state. */
void ClientNode::SetClientFullScreen(char fullScreen) {

  XEvent event;
  int north, south, east, west;
  BoundingBox box;
  const ScreenType *sp;

  Assert(np);

  /* Make sure there's something to do. */
  if (!fullScreen == !(this->state.status & STAT_FULLSCREEN)) {
    return;
  }
  if (!(this->state.border & BORDER_FULLSCREEN)) {
    return;
  }

  if (this->state.status & STAT_SHADED) {
    this->UnshadeClient();
  }

  if (fullScreen) {

    this->state.status |= STAT_FULLSCREEN;

    if (!(this->state.maxFlags)) {
      this->oldx = this->x;
      this->oldy = this->y;
      this->oldWidth = this->width;
      this->oldHeight = this->height;
    }

    sp = GetCurrentScreen(this->x, this->y);
    GetScreenBounds(sp, &box);

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

    this->state.status &= ~STAT_FULLSCREEN;

    this->x = this->oldx;
    this->y = this->oldy;
    this->width = this->oldWidth;
    this->height = this->oldHeight;
    this->ConstrainSize();
    this->ConstrainPosition();

    if (this->state.maxFlags != MAX_NONE) {
      this->PlaceMaximizedClient(this->state.maxFlags);
    }

    Border::ResetBorder(this);

    event.type = MapRequest;
    event.xmaprequest.send_event = True;
    event.xmaprequest.display = display;
    event.xmaprequest.parent = this->parent;
    event.xmaprequest.window = this->window;
    JXSendEvent(display, rootWindow, False, SubstructureRedirectMask, &event);

  }

  WriteState(this);
  this->SendConfigureEvent();
  _RequireRestack();

}

/** Set the active client. */
void ClientNode::FocusClient() {
  if (this->state.status & STAT_HIDDEN) {
    return;
  }
  if (!(this->state.status & (STAT_CANFOCUS | STAT_TAKEFOCUS))) {
    return;
  }

  if (activeClient != this || !(this->state.status & STAT_ACTIVE)) {
    if (activeClient) {
      activeClient->state.status &= ~STAT_ACTIVE;
      if (!(activeClient->state.status & STAT_OPACITY)) {
        activeClient->SetOpacity(settings.inactiveClientOpacity, 0);
      }
      Border::DrawBorder(activeClient);
      WriteNetState(activeClient);
    }
    this->state.status |= STAT_ACTIVE;
    activeClient = this;
    if (!(this->state.status & STAT_OPACITY)) {
      this->SetOpacity(settings.activeClientOpacity, 0);
    }

    Border::DrawBorder(this);
    _RequirePagerUpdate();
    _RequireTaskUpdate();
  }

  if (this->state.status & STAT_MAPPED) {
    this->UpdateClientColormap(-1);
    SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, this->window);
    WriteNetState(this);
    if (this->state.status & STAT_CANFOCUS) {
      JXSetInputFocus(display, this->window, RevertToParent, eventTime);
    }
    if (this->state.status & STAT_TAKEFOCUS) {
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
  Assert(np);
  ReadWMProtocols(this->window, &this->state);
  if (this->state.status & STAT_DELETE) {
    SendClientMessage(this->window, ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW);
  } else {
    this->KillClient();
  }
}

/** Callback to kill a client after a confirm dialog. */
void ClientNode::KillClientHandler(ClientNode *np) {
  if (np == activeClient) {
    FocusNextStacked(np);
  }

  JXKillClient(display, np->window);
}

/** Kill a client window. */
void ClientNode::KillClient() {
  ShowConfirmDialog(this, ClientNode::KillClientHandler,
  _("Kill this window?"),
  _("This may cause data to be lost!"),
  NULL);
}

/** Place transients on top of the owner. */
void ClientNode::RestackTransients() {
  ClientNode *tp;
  unsigned int layer;

  /* Place any transient windows on top of the owner */
  for (layer = 0; layer < LAYER_COUNT; layer++) {
    for (tp = nodes[layer]; tp; tp = tp->next) {
      if (tp->owner == this->window && tp->prev) {

        ClientNode *next = tp->next;

        tp->prev->next = tp->next;
        if (tp->next) {
          tp->next->prev = tp->prev;
        } else {
          nodeTail[tp->state.layer] = tp->prev;
        }
        tp->next = nodes[tp->state.layer];
        nodes[tp->state.layer]->prev = tp;
        tp->prev = NULL;
        nodes[tp->state.layer] = tp;

        tp = next;

      }

      /* tp will be tp->next if the above code is executed. */
      /* Thus, if it is NULL, we are done with this layer. */
      if (!tp) {
        break;
      }
    }
  }
}

/** Raise the client. This will affect transients. */
void ClientNode::RaiseClient() {
  if (nodes[this->state.layer] != this) {

    /* Raise the window */
    Assert(this->prev);
    this->prev->next = this->next;
    if (this->next) {
      this->next->prev = this->prev;
    } else {
      nodeTail[this->state.layer] = this->prev;
    }
    this->next = nodes[this->state.layer];
    nodes[this->state.layer]->prev = this;
    this->prev = NULL;
    nodes[this->state.layer] = this;

  }

  this->RestackTransients();
  _RequireRestack();

}

/** Restack a client window. This will not affect transients. */
void ClientNode::RestackClient(Window above, int detail) {

  ClientNode *tp;
  char inserted = 0;

  /* Remove from the window list. */
  if (this->prev) {
    this->prev->next = this->next;
  } else {
    nodes[this->state.layer] = this->next;
  }
  if (this->next) {
    this->next->prev = this->prev;
  } else {
    nodeTail[this->state.layer] = this->prev;
  }

  /* Insert back into the window list. */
  if (above != None && above != this->window) {

    /* Insert relative to some other window. */
    char found = 0;
    for (tp = nodes[this->state.layer]; tp; tp = tp->next) {
      if (tp == this) {
        found = 1;
      } else if (tp->window == above) {
        char insert_before = 0;
        inserted = 1;
        switch (detail) {
        case Above:
        case TopIf:
          insert_before = 1;
          break;
        case Below:
        case BottomIf:
          insert_before = 0;
          break;
        case Opposite:
          insert_before = !found;
          break;
        }
        if (insert_before) {

          /* Insert before this window. */
          this->prev = tp->prev;
          this->next = tp;
          if (tp->prev) {
            tp->prev->next = this;
          } else {
            nodes[this->state.layer] = this;
          }
          tp->prev = this;

        } else {

          /* Insert after this window. */
          this->prev = tp;
          this->next = tp->next;
          if (tp->next) {
            tp->next->prev = this;
          } else {
            nodeTail[this->state.layer] = this;
          }
          tp->next = this;

        }
        break;
      }
    }
  }
  if (!inserted) {

    /* Insert absolute for the layer. */
    if (detail == Below || detail == BottomIf) {

      /* Insert to the bottom of the stack. */
      this->next = NULL;
      this->prev = nodeTail[this->state.layer];
      if (nodeTail[this->state.layer]) {
        nodeTail[this->state.layer]->next = this;
      } else {
        nodes[this->state.layer] = this;
      }
      nodeTail[this->state.layer] = this;

    } else {

      /* Insert at the top of the stack. */
      this->next = nodes[this->state.layer];
      this->prev = NULL;
      if (nodes[this->state.layer]) {
        nodes[this->state.layer]->prev = this;
      } else {
        nodeTail[this->state.layer] = this;
      }
      nodes[this->state.layer] = this;

    }
  }

  this->RestackTransients();
  _RequireRestack();

}

/** Restack the clients according the way we want them. */
void ClientNode::RestackClients(void) {

  TrayType *tp;
  ClientNode *np;
  unsigned int layer, index;
  int trayCount;
  Window *stack;
  Window fw;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  /* Allocate memory for restacking. */
  trayCount = TrayType::GetTrayCount();
  stack = AllocateStack((clientCount + trayCount) * sizeof(Window));

  /* Prepare the stacking array. */
  fw = None;
  index = 0;
  if (activeClient && (activeClient->getState()->status & STAT_FULLSCREEN)) {
    fw = activeClient->window;
    for (np = nodes[activeClient->getState()->layer]; np; np = np->getNext()) {
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

    for (np = nodes[layer]; np; np = np->getNext()) {
      if ((np->getState()->status & (STAT_MAPPED | STAT_SHADED)) && !(np->getState()->status & STAT_HIDDEN)) {
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

    for (tp = TrayType::GetTrays(); tp; tp = tp->getNext()) {
      if (layer == tp->getLayer()) {
        stack[index] = tp->getWindow();
        index += 1;
      }
    }

    if (layer == FIRST_LAYER) {
      break;
    }
    layer -= 1;

  }

  JXRestackWindows(display, stack, index);

  ReleaseStack(stack);
  TaskBarType::UpdateNetClientList();
  _RequirePagerUpdate();

}

/** Send a client message to a window. */
void ClientNode::SendClientMessage(Window w, AtomType type, AtomType message) {

  XEvent event;
  int status;

  memset(&event, 0, sizeof(event));
  event.xclient.type = ClientMessage;
  event.xclient.window = w;
  event.xclient.message_type = atoms[type];
  event.xclient.format = 32;
  event.xclient.data.l[0] = atoms[message];
  event.xclient.data.l[1] = eventTime;

  status = JXSendEvent(display, w, False, 0, &event);
  if (JUNLIKELY(status == False)) {
    Debug("SendClientMessage failed");
  }

}

/** Remove a client window from management. */
void ClientNode::RemoveClient() {

  ColormapNode *cp;

  Assert(np);
  Assert(this->window != None);

  /* Remove this client from the client list */
  if (this->next) {
    this->next->prev = this->prev;
  } else {
    nodeTail[this->state.layer] = this->prev;
  }
  if (this->prev) {
    this->prev->next = this->next;
  } else {
    nodes[this->state.layer] = this->next;
  }
  clientCount -= 1;
  XDeleteContext(display, this->window, clientContext);
  if (this->parent != None) {
    XDeleteContext(display, this->parent, frameContext);
  }

  if (this->state.status & STAT_URGENT) {
    _UnregisterCallback(SignalUrgent, this);
  }

  /* Make sure this client isn't active */
  if (activeClient == this && !shouldExit) {
    FocusNextStacked(this);
  }
  if (activeClient == this) {

    /* Must be the last client. */
    SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, None);
    activeClient = NULL;
    JXSetInputFocus(display, rootWindow, RevertToParent, eventTime);

  }

  /* If the window manager is exiting (ie, not the client), then
   * reparent etc. */
  if (shouldExit && !(this->state.status & STAT_WMDIALOG)) {
    if (this->state.maxFlags) {
      this->x = this->oldx;
      this->y = this->oldy;
      this->width = this->oldWidth;
      this->height = this->oldHeight;
      JXMoveResizeWindow(display, this->window, this->x, this->y, this->width, this->height);
    }
    this->GravitateClient(1);
    if ((this->state.status & STAT_HIDDEN)
        || (!(this->state.status & STAT_MAPPED) && (this->state.status & (STAT_MINIMIZED | STAT_SHADED)))) {
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

  if (this->name) {
    Release(this->name);
  }
  if (this->instanceName) {
    JXFree(this->instanceName);
  }
  if (this->className) {
    JXFree(this->className);
  }

  TaskBarType::RemoveClientFromTaskBar(this);
  RemoveClientStrut(this);

  while (this->colormaps) {
    cp = this->colormaps->next;
    Release(this->colormaps);
    this->colormaps = cp;
  }

  DestroyIcon(this->getIcon());

  delete this;

  _RequireRestack();

}

/** Get the active client (possibly NULL). */
ClientNode *ClientNode::GetActiveClient(void) {
  return activeClient;
}

/** Find a client by parent or window. */
ClientNode *ClientNode::FindClient(Window w) {
  ClientNode *np;
  np = FindClientByWindow(w);
  if (!np) {
    np = FindClientByParent(w);
  }
  return np;
}

/** Find a client by window. */
ClientNode *ClientNode::FindClientByWindow(Window w) {
  ClientNode *np;
  if (!XFindContext(display, w, clientContext, (char**) &np)) {
    return np;
  } else {
    return NULL;
  }
}

/** Find a client by its frame window. */
ClientNode *ClientNode::FindClientByParent(Window p) {
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

  if ((this->state.border & (BORDER_TITLE | BORDER_OUTLINE)) == 0) {

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
    attr.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask | PointerMotionMask | SubstructureRedirectMask
        | SubstructureNotifyMask | EnterWindowMask | LeaveWindowMask | KeyPressMask | KeyReleaseMask;

    attrMask |= CWDontPropagate;
    attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

    attrMask |= CWBackPixel;
    attr.background_pixel = colors[COLOR_TITLE_BG2];

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
    this->parent = JXCreateWindow(display, rootWindow, x, y, width, height, 0, rootDepth, InputOutput, rootVisual,
        attrMask, &attr);
    XSaveContext(display, this->parent, frameContext, (const char*) this);

    JXSetWindowBorderWidth(display, this->window, 0);

    /* Reparent the client window. */
    JXReparentWindow(display, this->window, this->parent, west, north);

    if (this->state.status & STAT_MAPPED) {
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

  Assert(np);

  memset(&event, 0, sizeof(event));
  event.display = display;
  event.type = ConfigureNotify;
  event.event = this->window;
  event.window = this->window;
  if (this->state.status & STAT_FULLSCREEN) {
    sp = GetCurrentScreen(this->x, this->y);
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

  JXSendEvent(display, this->window, False, StructureNotifyMask, (XEvent* )&event);

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
void ClientNode::SignalUrgent(const TimeType *now, int x, int y, Window w, void *data) {
  ClientNode *np = (ClientNode*) data;

  /* Redraw borders. */
  if (np->getState()->status & STAT_FLASH) {
    np->state.status &= ~STAT_FLASH;
  } else if (!(np->state.status & STAT_NOTURGENT)) {
    np->state.status |= STAT_FLASH;
  }
  Border::DrawBorder(np);
  _RequireTaskUpdate();
  _RequirePagerUpdate();

}

/** Unmap a client window and consume the UnmapNotify event. */
void ClientNode::UnmapClient() {
  if (this->state.status & STAT_MAPPED) {
    XEvent e;

    /* Unmap the window and record that we did so. */
    this->state.status &= ~STAT_MAPPED;
    JXUnmapWindow(display, this->getWindow());

    /* Discard the unmap event so we don't process it later. */
    JXSync(display, False);
    if (JXCheckTypedWindowEvent(display, this->getWindow(), UnmapNotify, &e)) {
      _UpdateTime(&e);
    }
  }
}

/** Read colormap information for a client. */
void ClientNode::ReadWMColormaps()
{

   Window *windows;
   ColormapNode *cp;
   int count;

   Assert(np);

   if(JXGetWMColormapWindows(display, this->getWindow(), &windows, &count)) {
      if(count > 0) {
         int x;

         /* Free old colormaps. */
         while(this->colormaps) {
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
         for(x = 0; x < count; x++) {
            cp = new ColormapNode;
            cp->window = windows[x];
            cp->next = this->getColormaps();
            this->colormaps = cp;
         }

         JXFree(windows);

      }
   }

}

