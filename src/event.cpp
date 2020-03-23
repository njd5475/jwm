/**
 * @file event.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle X11 events.
 *
 */

#include "jwm.h"
#include "event.h"

#include "client.h"
#include "clientlist.h"
#include "confirm.h"
#include "cursor.h"
#include "icon.h"
#include "move.h"
#include "place.h"
#include "resize.h"
#include "root.h"
#include "swallow.h"
#include "taskbar.h"
#include "timing.h"
#include "winmenu.h"
#include "settings.h"
#include "tray.h"
#include "popup.h"
#include "pager.h"
#include "grab.h"
#include "action.h"
#include "binding.h"
#include "pager.h"
#include "DesktopEnvironment.h"
#include "LogWindow.h"
#include "Flex.h"

#ifdef USE_INOTIFYTOOLS
#include <inotifytools/inotify.h>
#include <inotifytools/inotifytools.h>
#endif

#define MIN_TIME_DELTA 50

Time Events::eventTime = CurrentTime;

std::vector<CallbackNode*> Events::callbacks;

char Events::restack_pending = 0;
char Events::task_update_pending = 0;
char Events::pager_update_pending = 0;

/** Wait for an event and process it. */
char Events::_WaitForEvent(XEvent *event) {
  struct timeval timeout;
  CallbackNode *cp;
  fd_set fds;
  long sleepTime;
  int fd;
  char handled;

#ifdef ConnectionNumber
  fd = ConnectionNumber(display);
#else
  fd = JXConnectionNumber(display);
#endif

  /* Compute how long we should sleep. */
  sleepTime = 10 * 1000; /* 10 seconds. */
  for (auto cp : callbacks) {
    if (cp->freq > 0 && cp->freq < sleepTime) {
      sleepTime = cp->freq;
    }
  }

  do {

    while (JXPending(display) == 0) {
      FD_ZERO(&fds);
      FD_SET(fd, &fds);
      timeout.tv_sec = sleepTime / 1000;
      timeout.tv_usec = (sleepTime % 1000) * 1000;
      if (select(fd + 1, &fds, NULL, NULL, &timeout) <= 0) {
        _Signal();
      }


#ifdef USE_INOTIFYTOOLS
      inotify_event* event = inotifytools_next_event(0);
      if(event != NULL) {
        Log("We received an event from inotify\n");
        Roots::Restart();
      }
#endif

      if (JUNLIKELY(shouldExit)) {
        return 0;
      }
    }

    _Signal();

    JXNextEvent(display, event);
    _UpdateTime(event);

    char buf[80];
    const char *eventName = "Unknown Event";
    switch (event->type) {
    case ConfigureRequest:
      eventName = "ConfigureRequest";
      _HandleConfigureRequest(&event->xconfigurerequest);
      handled = 1;
      break;
    case MapRequest:
      eventName = "MapRequest";
      _HandleMapRequest(&event->xmap);
      handled = 1;
      break;
    case PropertyNotify:
      eventName = "PropertyNotify";
      handled = _HandlePropertyNotify(&event->xproperty);
      break;
    case ClientMessage:
      eventName = "ClientMessage";
      _HandleClientMessage(&event->xclient);
      handled = 1;
      break;
    case UnmapNotify:
      eventName = "UnmapNotify";
      _HandleUnmapNotify(&event->xunmap);
      handled = 1;
      break;
    case Expose:
      eventName = "Expose";
      handled = _HandleExpose(&event->xexpose);
      break;
    case ColormapNotify:
      eventName = "ColormapNotify";
      _HandleColormapChange(&event->xcolormap);
      handled = 1;
      break;
    case DestroyNotify:
      eventName = "DestroyNotify";
      handled = _HandleDestroyNotify(&event->xdestroywindow);
      break;
    case SelectionClear:
      eventName = "SelectionClear";
      handled = _HandleSelectionClear(&event->xselectionclear);
      break;
    case ResizeRequest:

      eventName = "ResizeRequest";
      handled =
          DesktopEnvironment::DefaultEnvironment()->HandleDockResizeRequest(
              &event->xresizerequest);
      break;
    case MotionNotify:
      eventName = "MotionNotify";
      Cursors::SetMousePosition(event->xmotion.x_root, event->xmotion.y_root,
          event->xmotion.window);
      handled = 0;
      break;
    case ButtonPress:
    case ButtonRelease:
      if (event->type == ButtonPress) {
        eventName = "ButtonPress";
      } else {
        eventName = "ButtonRelease";
      }
      Cursors::SetMousePosition(event->xbutton.x_root, event->xbutton.y_root,
          event->xbutton.window);
      handled = 0;
      break;
    case EnterNotify:
      eventName = "EnterNotify";
      Cursors::SetMousePosition(event->xcrossing.x_root,
          event->xcrossing.y_root, event->xcrossing.window);
      handled = 0;
      break;
    case LeaveNotify:
      eventName = "LeaveNotify";
      Cursors::SetMousePosition(event->xcrossing.x_root,
          event->xcrossing.y_root,
          None);
      handled = 0;
      break;
    case ReparentNotify:
      eventName = "ReparentNotify";
      DesktopEnvironment::DefaultEnvironment()->HandleDockReparentNotify(
          &event->xreparent);
      handled = 1;
      break;
    case ConfigureNotify:
      eventName = "ConfigureNotify";
      handled = _HandleConfigureNotify(&event->xconfigure);
      break;
    case CreateNotify:
    case MapNotify:
    case GraphicsExpose:
    case NoExpose:
      if (event->type == CreateNotify) {
        eventName = "CreateNotify";
      } else if (event->type == MapNotify) {
        eventName = "MapNotify";
      } else if (event->type == GraphicsExpose) {
        eventName = "GraphicsExpose";
      } else if (event->type == NoExpose) {
        eventName = "NoExpose";
      }
      handled = 1;
      break;
    default:
      if (0) {
#ifdef USE_SHAPE
      } else if (haveShape && event->type == shapeEvent) {
        _HandleShapeEvent((XShapeEvent*) event);
        handled = 1;
#endif
      } else {
        handled = 0;
      }
      break;
    }
    sprintf(buf, "Event received [%s](%d)\n", eventName, event->type);
    Logger::Log(buf);
    Flex::DrawAll();

    if (!handled) {
      handled = Tray::ProcessTrayEvent(event);
    }
    if (!handled) {
      handled = Dialogs::ProcessDialogEvent(event);
    }
    if (!handled) {
      handled = LogWindow::ProcessEvents(event);
    }
    if (!handled) {
      handled = SwallowNode::ProcessSwallowEvent(event);
    }
    if (!handled) {
      handled = Popups::ProcessPopupEvent(event);
    }

  } while (handled && JLIKELY(!shouldExit));

  return !handled;

}

/** Wake up components that need to run at certain times. */
void Events::_Signal(void) {
  static TimeType last = ZERO_TIME;

  TimeType now;
  Window w;
  int x,
  y;

  if (restack_pending) {
    Logger::Log("Restacking Clients\n");
    ClientNode::RestackClients();
    restack_pending = 0;
  }
  if (task_update_pending) {
    Logger::Log("Updating task bars\n");
    TaskBar::UpdateTaskBar();
    task_update_pending = 0;
  }
  if (pager_update_pending) {
    Logger::Log("Updating pager\n");
    PagerType::UpdatePager();
    pager_update_pending = 0;
  }

  GetCurrentTime(&now);
  if (GetTimeDifference(&now, &last) < MIN_TIME_DELTA) {
    return;
  }
  last = now;

  Cursors::GetMousePosition(&x, &y, &w);
  for (auto cp : callbacks) {
    if (cp->freq == 0 || GetTimeDifference(&now, &cp->last) >= cp->freq) {
      cp->last = now;
      (cp->callback)(&now, x, y, w, cp->data);
    }
  }
}

/** Process an event. */
void Events::_ProcessEvent(XEvent *event) {
  switch (event->type) {
  case ButtonPress:
  case ButtonRelease:
    _HandleButtonEvent(&event->xbutton);
    break;
  case KeyPress:
    _HandleKeyPress(&event->xkey);
    break;
  case KeyRelease:
    _HandleKeyRelease(&event->xkey);
    break;
  case EnterNotify:
    _HandleEnterNotify(&event->xcrossing);
    break;
  case MotionNotify:
    while (JXCheckTypedEvent(display, MotionNotify, event))
      ;
    _UpdateTime(event);
    _HandleMotionNotify(&event->xmotion);
    break;
  case LeaveNotify:
  case DestroyNotify:
  case Expose:
  case ConfigureNotify:
    break;
  default:
    Debug("Unknown event type: %d", event->type);
    break;
  }
}

/** Discard button events for the specified windows. */
void Events::_DiscardButtonEvents() {
  XEvent event;
  JXSync(display, False);
  while (JXCheckMaskEvent(display, ButtonPressMask | ButtonReleaseMask, &event)) {
    _UpdateTime(&event);
  }
}

/** Discard motion events for the specified window. */
void Events::_DiscardMotionEvents(XEvent *event, Window w) {
  XEvent temp;
  JXSync(display, False);
  while (JXCheckTypedEvent(display, MotionNotify, &temp)) {
    _UpdateTime(&temp);
    Cursors::SetMousePosition(temp.xmotion.x_root, temp.xmotion.y_root,
        temp.xmotion.window);
    if (temp.xmotion.window == w) {
      *event = temp;
    }
  }
}

/** Discard key events for the specified window. */
void Events::_DiscardKeyEvents(XEvent *event, Window w) {
  JXSync(display, False);
  while (JXCheckTypedWindowEvent(display, w, KeyPress, event)) {
    _UpdateTime(event);
  }
}

/** Discard enter notify events. */
void Events::_DiscardEnterEvents() {
  XEvent event;
  JXSync(display, False);
  while (JXCheckMaskEvent(display, EnterWindowMask, &event)) {
    _UpdateTime(&event);
    Cursors::SetMousePosition(event.xmotion.x_root, event.xmotion.y_root,
        event.xmotion.window);
  }
}

/** Process a selection clear event. */
char Events::_HandleSelectionClear(const XSelectionClearEvent *event) {
  if (event->selection == managerSelection) {
    /* Lost WM selection. */
    shouldExit = 1;
    return 1;
  }
  return DesktopEnvironment::DefaultEnvironment()->HandleDockSelectionClear(
      event);
}

/** Process a button event. */
void Events::_HandleButtonEvent(const XButtonEvent *event) {
  Log("Handling a button event\n");
  static Time lastClickTime = 0;
  static int lastX = 0, lastY = 0;
  static unsigned doubleClickActive = 0;

  ClientNode *np;
  int button;
  int north, south, east, west;
  MouseContextType context;

  /* Determine the button to present for processing.
   * Press is positive, release is negative, double clicks
   * are multiplied by 11.
   */
  if (event->type == ButtonPress) {
    if (doubleClickActive == event->button && event->time != lastClickTime
        && event->time - lastClickTime <= settings.doubleClickSpeed
        && abs(event->x - lastX) <= settings.doubleClickDelta
        && abs(event->y - lastY) <= settings.doubleClickDelta) {
      button = event->button * 11;
    } else {
      button = event->button;
    }
    doubleClickActive = 0;
  } else {
    button = event->button;
  }
  if (event->type == ButtonRelease) {
    button = -button;
  }
  if (button < 11) {
    doubleClickActive = event->button;
    lastClickTime = event->time;
    lastX = event->x;
    lastY = event->y;
  }

  /* Dispatch the event. */
  np = ClientNode::FindClientByParent(event->window);
  if (np) {
    /* Click on the border. */
    if (event->type == ButtonPress) {
      np->keyboardFocus();
      np->RaiseClient();
    }
    context = Border::GetBorderContext(np, event->x, event->y);
    _ProcessBinding(context, np, event->state, button, event->x, event->y);
  } else if (event->window == rootWindow) {
    /* Click on the root.
     * Note that we use the raw button from the event for ShowRootMenu. */
    if (!Roots::ShowRootMenu(event->button, event->x, event->y, 0)) {
      _ProcessBinding(MC_ROOT, NULL, event->state, button, 0, 0);
    }
  } else {
    /* Click over window content. */
    const unsigned int mask = event->state & ~Binding::lockMask;
    np = ClientNode::FindClientByWindow(event->window);
    if (np) {
      const char move_resize = (np->isDragable())
          || ((mask == settings.moveMask) && !(np->isNotDraggable()));
      switch (event->button) {
      case Button1:
      case Button2:
        np->keyboardFocus();
        if (settings.focusModel == FOCUS_SLOPPY
            || settings.focusModel == FOCUS_CLICK) {
          np->RaiseClient();
        }
        if (move_resize) {
          Border::GetBorderSize(np, &north, &south, &east, &west);
          np->MoveClient(event->x + west, event->y + north);
        }
        break;
      case Button3:
        if (move_resize) {
          Border::GetBorderSize(np, &north, &south, &east, &west);
          np->ResizeClient(MC_BORDER | MC_BORDER_E | MC_BORDER_S,
              event->x + west, event->y + north);
        } else {
          np->keyboardFocus();
          if (settings.focusModel == FOCUS_SLOPPY
              || settings.focusModel == FOCUS_CLICK) {
            np->RaiseClient();
          }
        }
        break;
      default:
        break;
      }
      JXAllowEvents(display, ReplayPointer, eventTime);
    }
  }
}

/** Toggle maximized state. */
void Events::_ToggleMaximized(ClientNode *np, MaxFlags flags) {
  if (np) {
    if (np->getMaxFlags() == flags) {
      np->MaximizeClient(MAX_NONE);
    } else {
      np->MaximizeClient(flags);
    }
  }
}

void Events::Bind(KeyEventHandler *handler, Actions action) {
  Logger::Log("Events::Bind not implemented");
}

/** Process a key or mouse binding. */
void Events::_ProcessBinding(MouseContextType context, ClientNode *np,
    unsigned state, int code, int x, int y) {
  const ActionType key = Binding::GetKey(context, state, code);
  const char keyAction = context == MC_NONE;
  switch (key.action) {
  case EXEC:
    Binding::RunKeyCommand(context, state, code);
    break;
  case DESKTOP:
    DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(key.extra);
    break;
  case RDESKTOP:
    DesktopEnvironment::DefaultEnvironment()->RightDesktop();
    break;
  case LDESKTOP:
    DesktopEnvironment::DefaultEnvironment()->LeftDesktop();
    break;
  case UDESKTOP:
    DesktopEnvironment::DefaultEnvironment()->AboveDesktop();
    break;
  case DDESKTOP:
    DesktopEnvironment::DefaultEnvironment()->BelowDesktop();
    break;
  case SHOWDESK:
    DesktopEnvironment::DefaultEnvironment()->ShowDesktop();
    break;
  case SHOWTRAY:
    Tray::ShowAllTrays();
    break;
  case NEXT:
    ClientList::StartWindowWalk();
    TaskBar::FocusNext();
    break;
  case NEXTSTACK:
    ClientList::StartWindowStackWalk();
    ClientList::WalkWindowStack(1);
    break;
  case PREV:
    ClientList::StartWindowWalk();
    TaskBar::FocusPrevious();
    break;
  case PREVSTACK:
    ClientList::StartWindowStackWalk();
    ClientList::WalkWindowStack(0);
    break;
  case CLOSE:
    if (np) {
      np->DeleteClient();
    }
    break;
  case SHADE:
    if (np) {
      if (np->isShaded()) {
        np->ShadeClient();
      } else {
        np->UnshadeClient();
      }
    }
    break;
  case STICK:
    if (np) {
      if (np->isSticky()) {
        np->SetClientSticky(0);
      } else {
        np->SetClientSticky(1);
      }
    }
    break;
  case MOVE:
    if (np) {
      if (keyAction) {
        np->MoveClientKeyboard();
      } else {
        np->MoveClient(x, y);
      }
    }
    break;
  case RESIZE:
    if (np) {
      /* Use provided context by default. */
      const unsigned char corner = key.extra;
      MouseContextType resizeContext = context;
      if (corner) {
        /* Custom corner specified. */
        resizeContext = MC_BORDER;
        resizeContext |= (corner & RESIZE_N) ? MC_BORDER_N : MC_NONE;
        resizeContext |= (corner & RESIZE_S) ? MC_BORDER_S : MC_NONE;
        resizeContext |= (corner & RESIZE_E) ? MC_BORDER_E : MC_NONE;
        resizeContext |= (corner & RESIZE_W) ? MC_BORDER_W : MC_NONE;
      } else if (keyAction) {
        /* No corner specified for a key action, assume SE. */
        resizeContext = MC_BORDER | MC_BORDER_S | MC_BORDER_E;
      }
      if (keyAction) {
        np->ResizeClientKeyboard(resizeContext);
      } else {
        np->ResizeClient(resizeContext, x, y);
      }
    }
    break;
  case MIN:
    if (np) {
      np->MinimizeClient(1);
    }
    break;
  case MAX:
    if (np) {
      if (keyAction) {
        _ToggleMaximized(np, MAX_HORIZ | MAX_VERT);
      } else {
        np->MaximizeClientDefault();
      }
    }
    break;
  case RESTORE:
    if (np) {
      if (np->getMaxFlags()) {
        np->MaximizeClient(MAX_NONE);
      } else {
        np->MinimizeClient(1);
      }
    }
    break;
  case MAXTOP:
    _ToggleMaximized(np, MAX_TOP | MAX_HORIZ);
    break;
  case MAXBOTTOM:
    _ToggleMaximized(np, MAX_BOTTOM | MAX_HORIZ);
    break;
  case MAXLEFT:
    _ToggleMaximized(np, MAX_LEFT | MAX_VERT);
    break;
  case MAXRIGHT:
    _ToggleMaximized(np, MAX_RIGHT | MAX_VERT);
    break;
  case MAXV:
    _ToggleMaximized(np, MAX_VERT);
    break;
  case MAXH:
    _ToggleMaximized(np, MAX_HORIZ);
    break;
  case ROOT:
    Binding::ShowKeyMenu(context, state, code);
    break;
  case WIN:
    if (np) {
      if (keyAction) {
        np->RaiseClient();
        ShowWindowMenu(np, np->getX(), np->getY(), 1);
      } else {
        const unsigned bsize =
            (np->getBorder() & BORDER_OUTLINE) ? settings.borderWidth : 0;
        const unsigned titleHeight = Border::GetTitleHeight();
        const int mx = np->getX() + x - bsize;
        const int my = np->getY() + y - titleHeight - bsize;
        ShowWindowMenu(np, mx, my, 0);
      }
    }
    break;
  case RESTART:
    Roots::Restart();
    break;
  case EXIT:
    Roots::Exit(1);
    break;
  case FULLSCREEN:
    if (np) {
      if (np->isFullscreen()) {
        np->SetClientFullScreen(0);
      } else {
        np->SetClientFullScreen(1);
      }
    }
    break;
  case SEND:
    if (np) {
      const unsigned desktop = key.extra;
      np->SetClientDesktop(desktop);
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(desktop);
    }
    break;
  case SENDR:
    if (np) {
      const unsigned desktop =
          DesktopEnvironment::DefaultEnvironment()->GetRightDesktop(
              np->getDesktop());
      np->SetClientDesktop(desktop);
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(desktop);
    }
    break;
  case SENDL:
    if (np) {
      const unsigned desktop =
          DesktopEnvironment::DefaultEnvironment()->GetLeftDesktop(
              np->getDesktop());
      np->SetClientDesktop(desktop);
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(desktop);
    }
    break;
  case SENDU:
    if (np) {
      const unsigned desktop =
          DesktopEnvironment::DefaultEnvironment()->GetAboveDesktop(
              np->getDesktop());
      np->SetClientDesktop(desktop);
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(desktop);
    }
    break;
  case SENDD:
    if (np) {
      const unsigned desktop =
          DesktopEnvironment::DefaultEnvironment()->GetBelowDesktop(
              np->getDesktop());
      np->SetClientDesktop(desktop);
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(desktop);
    }
    break;
  default:
    break;
  }
  _DiscardEnterEvents();
}

/** Process a key press event. */
void Events::_HandleKeyPress(const XKeyEvent *event) {
  ClientNode *np;
  Cursors::SetMousePosition(event->x_root, event->y_root, event->window);
  np = ClientNode::GetActiveClient();
  _ProcessBinding(MC_NONE, np, event->state, event->keycode, 0, 0);
}

/** Handle a key release event. */
void Events::_HandleKeyRelease(const XKeyEvent *event) {
  const ActionType key = Binding::GetKey(MC_NONE, event->state, event->keycode);
  if (key.action != NEXTSTACK && key.action != NEXT && key.action != PREV
      && key.action != PREVSTACK) {
    ClientList::StopWindowWalk();
  }
}

/** Process a configure request. */
void Events::_HandleConfigureRequest(const XConfigureRequestEvent *event) {
  Places *np;

  if (DesktopEnvironment::DefaultEnvironment()->HandleDockConfigureRequest(
      event)) {
    return;
  }

  np = (Places*) ClientNode::FindClientByWindow(event->window);
  if (np) {

    int deltax, deltay;
    char changed = 0;
    char resized = 0;

    np->GetGravityDelta(np->getGravity(), &deltax, &deltay);
    if ((event->value_mask & CWWidth) && (event->width != np->getWidth())) {
      switch (np->getGravity()) {
      case EastGravity:
      case NorthEastGravity:
      case SouthEastGravity:
        /* Right side should not move. */
        np->setX(np->getX() - (event->width - np->getWidth()));
        break;
      case WestGravity:
      case NorthWestGravity:
      case SouthWestGravity:
        /* Left side should not move. */
        break;
      case CenterGravity:
        /* Center of the window should not move. */
        np->setX(np->getX() - (event->width - np->getWidth()) / 2);
        break;
      default:
        break;
      }
      np->setWidth(event->width);
      changed = 1;
      resized = 1;
    }
    if ((event->value_mask & CWHeight) && (event->height != np->getHeight())) {
      switch (np->getGravity()) {
      case NorthGravity:
      case NorthEastGravity:
      case NorthWestGravity:
        /* Top should not move. */
        break;
      case SouthGravity:
      case SouthEastGravity:
      case SouthWestGravity:
        /* Bottom should not move. */
        np->setY(np->getY() - (event->height - np->getHeight()));
        break;
      case CenterGravity:
        /* Center of the window should not move. */
        np->setY(np->getY() - (event->height - np->getHeight()) / 2);
        break;
      default:
        break;
      }
      np->setHeight(event->height);
      changed = 1;
      resized = 1;
    }
    if ((event->value_mask & CWX) && (event->x - deltax != np->getX())) {
      np->setX(event->x - deltax);
      changed = 1;
    }
    if ((event->value_mask & CWY) && (event->y - deltay != np->getY())) {
      np->setY(event->y - deltay);
      changed = 1;
    }

    /* Update stacking. */
    if ((event->value_mask & CWStackMode)) {
      Window above = None;
      if (event->value_mask & CWSibling) {
        above = event->above;
      }
      np->RestackClient(above, event->detail);
    }

    /* Return early if there's nothing to do. */
    if (!changed) {
      /* Nothing changed; send a synthetic configure event. */
      np->SendConfigureEvent();
      return;
    }

    /* Stop any move/resize that may be in progress. */
    if (np->getController()) {
      (np->getController())(0);
    }

    /* If the client is maximized, restore it first. */
    if (np->getMaxFlags()) {
      np->MaximizeClient(MAX_NONE);
    }

    if (np->getBorder() & BORDER_CONSTRAIN) {
      resized = 1;
    }
    if (resized) {
      /* The size changed so the parent will need to be redrawn. */
      np->ConstrainSize();
      np->ConstrainPosition();
      Border::ResetBorder(np);
    } else {
      /* Only the position changed; move the client. */
      int north, south, east, west;
      Border::GetBorderSize(np, &north, &south, &east, &west);

      if (np->getParent() != None) {
        JXMoveWindow(display, np->getParent(), np->getX() - west,
            np->getY() - north);
        np->SendConfigureEvent();
      } else {
        JXMoveWindow(display, np->getWindow(), np->getX(), np->getY());
      }
    }

    _RequirePagerUpdate();

  } else {

    /* We don't know about this window, just let the configure through. */

    XWindowChanges wc;
    wc.stack_mode = event->detail;
    wc.sibling = event->above;
    wc.border_width = event->border_width;
    wc.x = event->x;
    wc.y = event->y;
    wc.width = event->width;
    wc.height = event->height;
    JXConfigureWindow(display, event->window, event->value_mask, &wc);

  }
}

/** Process a configure notify event. */
char Events::_HandleConfigureNotify(const XConfigureEvent *event) {
  if (event->window != rootWindow) {
    return 0;
  }
  if (rootWidth != event->width || rootHeight != event->height) {
    rootWidth = event->width;
    rootHeight = event->height;
    shouldRestart = 1;
    shouldExit = 1;
  }
  return 1;
}

/** Process an enter notify event. */
void Events::_HandleEnterNotify(const XCrossingEvent *event) {
  ClientNode *np;
  Cursor cur;
  np = ClientNode::FindClient(event->window);
  if (np) {
    if (!(np->isActive())
        && (settings.focusModel == FOCUS_SLOPPY
            || settings.focusModel == FOCUS_SLOPPY_TITLE)) {
      np->keyboardFocus();
    }
    if (np->getParent() == event->window) {
      np->setMouseContext(Border::GetBorderContext(np, event->x, event->y));
      cur = Cursors::GetFrameCursor(np->getMouseContext());
      JXDefineCursor(display, np->getParent(), cur);
    } else if (np->getMouseContext() != MC_NONE) {
      Cursors::SetDefaultCursor(np->getParent());
      np->setMouseContext(MC_NONE);
    }
  }

}

/** Handle an expose event. */
char Events::_HandleExpose(const XExposeEvent *event) {
  ClientNode *np;
  np = ClientNode::FindClientByParent(event->window);
  if (np) {
    if (event->count == 0) {
      Border::DrawBorder(np);
    }
    return 1;
  } else {
    np = ClientNode::FindClientByWindow(event->window);
    if (np) {
      if (np->isDialogWindow()) {

        /* Dialog expose events are handled elsewhere. */
        return 0;

      } else {

        /* Ignore other expose events for client windows. */
        return 1;

      }
    }
    return event->count ? 1 : 0;
  }
}

/** Handle a property notify event. */
char Events::_HandlePropertyNotify(const XPropertyEvent *event) {
  ClientNode *np = ClientNode::FindClientByWindow(event->window);
  if (np) {
    char changed = 0;
    Atom atom = event->atom;
    if (atom == XA_WM_NAME) {
      np->ReadWMName();
      changed = 1;
    } else if (atom == XA_WM_NORMAL_HINTS) {
      np->ReadWMNormalHints();
      if (np->ConstrainSize()) {
        Border::ResetBorder(np);
      }
      changed = 1;
    } else if (atom == XA_WM_HINTS) {
      if (np->isUrgent()) {
        _UnregisterCallback(ClientNode::SignalUrgent, np);
      }
      Hints::ReadWMHints(np->getWindow(), np, 1);
      if (np->isUrgent()) {
        _RegisterCallback(URGENCY_DELAY, ClientNode::SignalUrgent, np);
      }
      Hints::WriteState(np);
    } else if (atom == XA_WM_TRANSIENT_FOR) {
      unsigned long int owner = np->getOwner();
      JXGetTransientForHint(display, np->getWindow(), &owner);
      np->setOwner(owner);
    } else if (atom == XA_WM_ICON_NAME || XA_WM_CLIENT_MACHINE) {
    } else {
      if (event->atom == Hints::atoms[ATOM_WM_COLORMAP_WINDOWS]) {
        np->ReadWMColormaps();
        np->UpdateClientColormap(-1);
      } else if (event->atom == Hints::atoms[ATOM_WM_PROTOCOLS]) {
        Hints::ReadWMProtocols(np->getWindow(), np);
      } else if (event->atom == Hints::atoms[ATOM_NET_WM_ICON]) {
        Icon::LoadIcon(np);
        changed = 1;
      } else if (event->atom == Hints::atoms[ATOM_NET_WM_NAME]) {
        Hints::ReadWMName(np);
        changed = 1;
      } else if (event->atom == Hints::atoms[ATOM_NET_WM_STRUT_PARTIAL]) {
        Places::ReadClientStrut(np);
      } else if (event->atom == Hints::atoms[ATOM_NET_WM_STRUT]) {
        Places::ReadClientStrut(np);
      } else if (event->atom == Hints::atoms[ATOM_MOTIF_WM_HINTS]) {
        np->_UpdateState();
        Hints::WriteState(np);
        Border::ResetBorder(np);
        changed = 1;
      } else if (event->atom == Hints::atoms[ATOM_NET_WM_WINDOW_OPACITY]) {
        unsigned int opacity;
        Hints::ReadWMOpacity(np->getWindow(), &opacity);
        if (np->getParent() != None) {
          Hints::SetOpacity(np, opacity, 1); //FIXME: This is broken
        }
        np->setOpacity(opacity);
      }

    }

    if (changed) {
      Border::DrawBorder(np);
      _RequireTaskUpdate();
      _RequirePagerUpdate();
    }
    if (np->isDialogWindow()) {
      return 0;
    } else {
      return 1;
    }
  }

  return 1;
}

/** Handle a client message. */
void Events::_HandleClientMessage(const XClientMessageEvent *event) {

  ClientNode *np;
#ifdef DEBUG
  char *atomName;
#endif

  np = ClientNode::FindClientByWindow(event->window);
  if (np) {
    if (event->message_type == Hints::atoms[ATOM_WM_CHANGE_STATE]) {

      if (np->getController()) {
        (np->getController())(0);
      }

      switch (event->data.l[0]) {
      case WithdrawnState:
        np->sendToBackground();
        break;
      case IconicState:
        np->MinimizeClient(1);
        break;
      case NormalState:
        np->RestoreClient(1);
        break;
      default:
        break;
      }

    } else if (event->message_type == Hints::atoms[ATOM_NET_ACTIVE_WINDOW]) {

      np->RestoreClient(1);
      np->ShadeClient();
      np->keyboardFocus();

    } else if (event->message_type == Hints::atoms[ATOM_NET_WM_DESKTOP]) {

      if (event->data.l[0] == ~0L) {
        np->SetClientSticky(1);
      } else {

        if (np->getController()) {
          (np->getController())(0);
        }

        if (event->data.l[0] >= 0
            && event->data.l[0] < (long) settings.desktopCount) {
          np->clearToSticky();
          np->SetClientDesktop(event->data.l[0]);
        }
      }

    } else if (event->message_type == Hints::atoms[ATOM_NET_CLOSE_WINDOW]) {

      np->DeleteClient();

    } else if (event->message_type
        == Hints::atoms[ATOM_NET_MOVERESIZE_WINDOW]) {

      _HandleNetMoveResize(event, np);

    } else if (event->message_type == Hints::atoms[ATOM_NET_WM_MOVERESIZE]) {

      HandleNetWMMoveResize(event, np);

    } else if (event->message_type == Hints::atoms[ATOM_NET_RESTACK_WINDOW]) {

      _HandleNetRestack(event, np);

    } else if (event->message_type == Hints::atoms[ATOM_NET_WM_STATE]) {

      _HandleNetWMState(event, np);

    } else {

#ifdef DEBUG
      atomName = JXGetAtomName(display, event->message_type);
      Debug("Unknown ClientMessage to client: %s", atomName);
      JXFree(atomName);
#endif

    }

  } else if (event->window == rootWindow) {

    if (event->message_type == Hints::atoms[ATOM_JWM_RESTART]) {
      Roots::Restart();
    } else if (event->message_type == Hints::atoms[ATOM_JWM_EXIT]) {
      Roots::Exit(0);
    } else if (event->message_type == Hints::atoms[ATOM_JWM_RELOAD]) {
      Roots::ReloadMenu();
    } else if (event->message_type == Hints::atoms[ATOM_NET_CURRENT_DESKTOP]) {
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(event->data.l[0]);
    } else if (event->message_type == Hints::atoms[ATOM_NET_SHOWING_DESKTOP]) {
      DesktopEnvironment::DefaultEnvironment()->ShowDesktop();
    } else {
#ifdef DEBUG
      atomName = JXGetAtomName(display, event->message_type);
      Debug("Unknown ClientMessage to root: %s", atomName);
      JXFree(atomName);
#endif
    }

  } else if (event->message_type
      == Hints::atoms[ATOM_NET_REQUEST_FRAME_EXTENTS]) {

    _HandleFrameExtentsRequest(event);

  } else if (event->message_type == Hints::atoms[ATOM_NET_SYSTEM_TRAY_OPCODE]) {

    DesktopEnvironment::DefaultEnvironment()->HandleDockEvent(event);

  } else {
#ifdef DEBUG
    atomName = JXGetAtomName(display, event->message_type);
    Debug("ClientMessage to unknown window (0x%x): %s", event->window,
        atomName);
    JXFree(atomName);
#endif
  }

}

/** Handle a _NET_MOVERESIZE_WINDOW request. */
void Events::_HandleNetMoveResize(const XClientMessageEvent *event,
    ClientNode *np) {

  long flags;
  int gravity;
  int deltax, deltay;

  Assert(event);
  Assert(np);

  flags = event->data.l[0] >> 8;
  gravity = event->data.l[0] & 0xFF;
  if (gravity == 0) {
    gravity = np->getGravity();
  }
  np->GetGravityDelta(gravity, &deltax, &deltay);

  if (flags & (1 << 2)) {
    const long width = event->data.l[3];
    switch (gravity) {
    case EastGravity:
    case NorthEastGravity:
    case SouthEastGravity:
      /* Right side should not move. */
      np->setX(np->getX() - (width - np->getWidth()));
      break;
    case WestGravity:
    case NorthWestGravity:
    case SouthWestGravity:
      /* Left side should not move. */
      break;
    case CenterGravity:
      /* Center of the window should not move. */
      np->setX(np->getX() - (width - np->getWidth()) / 2);
      break;
    default:
      break;
    }
    np->setWidth(width);
  }
  if (flags & (1 << 3)) {
    const long height = event->data.l[4];
    switch (gravity) {
    case NorthGravity:
    case NorthEastGravity:
    case NorthWestGravity:
      /* Top should not move. */
      break;
    case SouthGravity:
    case SouthEastGravity:
    case SouthWestGravity:
      /* Bottom should not move. */
      np->setY(np->getY() - (height - np->getHeight()));
      break;
    case CenterGravity:
      /* Center of the window should not move. */
      np->setY(np->getY() - (height - np->getHeight()) / 2);
      break;
    default:
      break;
    }
    np->setHeight(height);
  }
  if (flags & (1 << 0)) {
    np->setX(event->data.l[1] - deltax);
  }
  if (flags & (1 << 1)) {
    np->setY(event->data.l[2] - deltay);
  }

  /* Don't let maximized clients be moved or resized. */
  if (JUNLIKELY(np->isFullscreen())) {
    np->SetClientFullScreen(0);
  }
  if (JUNLIKELY(np->getMaxFlags())) {
    np->MaximizeClient(MAX_NONE);
  }

  np->ConstrainSize();
  Border::ResetBorder(np);
  np->SendConfigureEvent();
  _RequirePagerUpdate();

}

/** Handle a _NET_WM_MOVERESIZE request. */
void Events::HandleNetWMMoveResize(const XClientMessageEvent *event,
    ClientNode *np) {

  long x = event->data.l[0] - np->getX();
  long y = event->data.l[1] - np->getY();
  const long direction = event->data.l[2];
  int deltax, deltay;

  np->GetGravityDelta(np->getGravity(), &deltax, &deltay);
  x -= deltax;
  y -= deltay;

  switch (direction) {
  case 0: /* top-left */
    np->ResizeClient(MC_BORDER | MC_BORDER_N | MC_BORDER_W, x, y);
    break;
  case 1: /* top */
    np->ResizeClient(MC_BORDER | MC_BORDER_N, x, y);
    break;
  case 2: /* top-right */
    np->ResizeClient(MC_BORDER | MC_BORDER_N | MC_BORDER_E, x, y);
    break;
  case 3: /* right */
    np->ResizeClient(MC_BORDER | MC_BORDER_E, x, y);
    break;
  case 4: /* bottom-right */
    np->ResizeClient(MC_BORDER | MC_BORDER_S | MC_BORDER_E, x, y);
    break;
  case 5: /* bottom */
    np->ResizeClient(MC_BORDER | MC_BORDER_S, x, y);
    break;
  case 6: /* bottom-left */
    np->ResizeClient(MC_BORDER | MC_BORDER_S | MC_BORDER_W, x, y);
    break;
  case 7: /* left */
    np->ResizeClient(MC_BORDER | MC_BORDER_W, x, y);
    break;
  case 8: /* move */
    np->MoveClient(x, y);
    break;
  case 9: /* resize-keyboard */
    np->ResizeClientKeyboard(MC_BORDER | MC_BORDER_S | MC_BORDER_E);
    break;
  case 10: /* move-keyboard */
    np->MoveClientKeyboard();
    break;
  case 11: /* cancel */
    if (np->getController()) {
      (np->getController())(0);
    }
    break;
  default:
    break;
  }

}

/** Handle a _NET_RESTACK_WINDOW request. */
void Events::_HandleNetRestack(const XClientMessageEvent *event,
    ClientNode *np) {
  const Window sibling = event->data.l[1];
  const int detail = event->data.l[2];
  np->RestackClient(sibling, detail);
}

/** Handle a _NET_WM_STATE request. */
void Events::_HandleNetWMState(const XClientMessageEvent *event,
    ClientNode *np) {

  unsigned int x;
  MaxFlags maxFlags;
  char actionStick;
  char actionShade;
  char actionFullScreen;
  char actionMinimize;
  char actionNolist;
  char actionNopager;
  char actionBelow;
  char actionAbove;

  /* Up to two actions to be applied together. */
  maxFlags = MAX_NONE;
  actionStick = 0;
  actionShade = 0;
  actionFullScreen = 0;
  actionMinimize = 0;
  actionNolist = 0;
  actionNopager = 0;
  actionBelow = 0;
  actionAbove = 0;

  for (x = 1; x <= 2; x++) {
    if (event->data.l[x] == (long) Hints::atoms[ATOM_NET_WM_STATE_STICKY]) {
      actionStick = 1;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_MAXIMIZED_VERT]) {
      maxFlags |= MAX_VERT;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_MAXIMIZED_HORZ]) {
      maxFlags |= MAX_HORIZ;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_SHADED]) {
      actionShade = 1;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_FULLSCREEN]) {
      actionFullScreen = 1;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_HIDDEN]) {
      actionMinimize = 1;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_SKIP_TASKBAR]) {
      actionNolist = 1;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_SKIP_PAGER]) {
      actionNopager = 1;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_BELOW]) {
      actionBelow = 1;
    } else if (event->data.l[x]
        == (long) Hints::atoms[ATOM_NET_WM_STATE_ABOVE]) {
      actionAbove = 1;
    }
  }

  switch (event->data.l[0]) {
  case 0: /* Remove */
    if (actionStick) {
      np->SetClientSticky(0);
    }
    if (maxFlags != MAX_NONE && np->getMaxFlags()) {
      np->MaximizeClient(np->getMaxFlags() & ~maxFlags);
    }
    if (actionShade) {
      np->ShadeClient();
    }
    if (actionFullScreen) {
      np->SetClientFullScreen(0);
    }
    if (actionMinimize) {
      np->RestoreClient(0);
    }
    if (actionNolist && !(np->shouldIgnoreSpecifiedList())) {
      np->clearToNoList();
      _RequireTaskUpdate();
    }
    if (actionNopager && !(np->shouldIgnorePager())) {
      np->clearToNoPager();
      _RequirePagerUpdate();
    }
    if (actionBelow && np->getLayer() == LAYER_BELOW) {
      np->resetLayerToDefault();
    }
    if (actionAbove && np->getLayer() == LAYER_ABOVE) {
      np->resetLayerToDefault();
    }
    break;
  case 1: /* Add */
    if (actionStick) {
      np->SetClientSticky(1);
    }
    if (maxFlags != MAX_NONE) {
      np->MaximizeClient(np->getMaxFlags() | maxFlags);
    }
    if (actionShade) {
      np->ShadeClient();
    }
    if (actionFullScreen) {
      np->SetClientFullScreen(1);
    }
    if (actionMinimize) {
      np->MinimizeClient(1);
    }
    if (actionNolist && !(np->shouldIgnoreSpecifiedList())) {
      np->setNoList();
      _RequireTaskUpdate();
    }
    if (actionNopager && !(np->shouldIgnorePager())) {
      np->setNoPager();
      _RequirePagerUpdate();
    }
    if (actionBelow) {
      np->SetClientLayer(LAYER_BELOW);
    }
    if (actionAbove) {
      np->SetClientLayer(LAYER_ABOVE);
    }
    break;
  case 2: /* Toggle */
    if (actionStick) {
      if (np->isSticky()) {
        np->SetClientSticky(0);
      } else {
        np->SetClientSticky(1);
      }
    }
    if (maxFlags) {
      np->MaximizeClient(np->getMaxFlags() ^ maxFlags);
    }
    if (actionShade) {
      if (np->isShaded()) {
        np->UnshadeClient();
      } else {
        np->ShadeClient();
      }
    }
    if (actionFullScreen) {
      if (np->isFullscreen()) {
        np->SetClientFullScreen(0);
      } else {
        np->SetClientFullScreen(1);
      }
    }
    if (actionBelow) {
      if (np->getLayer() == LAYER_BELOW) {
        np->resetLayerToDefault();
      } else {
        np->SetClientLayer(LAYER_BELOW);
      }
    }
    if (actionAbove) {
      if (np->getLayer() == LAYER_ABOVE) {
        np->resetLayerToDefault();
      } else {
        np->SetClientLayer(LAYER_ABOVE);
      }
    }
    /* Note that we don't handle toggling of hidden per EWMH
     * recommendations. */
    if (actionNolist && !(np->shouldIgnoreSpecifiedList())) {
      np->setHasNoList();
      _RequireTaskUpdate();
    }
    if (actionNopager && !(np->shouldIgnorePager())) {
      np->setNoPager();
      _RequirePagerUpdate();
    }
    break;
  default:
    Debug("bad _NET_WM_STATE action: %ld", event->data.l[0]);
    break;
  }

  /* Update _NET_WM_STATE if needed.
   * The state update is handled elsewhere for the other actions.
   */
  if (actionNolist | actionNopager | actionAbove | actionBelow) {
    Hints::WriteState(np);
  }

}

/** Handle a _NET_REQUEST_FRAME_EXTENTS request. */
void Events::_HandleFrameExtentsRequest(const XClientMessageEvent *event) {
  Hints::WriteFrameExtents(event->window, (const ClientNode*) &event->data);
}

/** Handle a motion notify event. */
void Events::_HandleMotionNotify(const XMotionEvent *event) {

  ClientNode *np;
  Cursor cur;

  if (event->is_hint) {
    return;
  }

  np = ClientNode::FindClientByParent(event->window);
  if (np) {
    const MouseContextType context = Border::GetBorderContext(np, event->x,
        event->y);
    if (np->getMouseContext() != context) {
      np->setMouseContext(context);
      cur = Cursors::GetFrameCursor(context);
      JXDefineCursor(display, np->getParent(), cur);
    }
  }

}

/** Handle a shape event. */
#ifdef USE_SHAPE
void Events::_HandleShapeEvent(const XShapeEvent *event) {
  ClientNode *np;
  np = ClientNode::FindClientByWindow(event->window);
  if (np) {
    np->setShaded();
    Border::ResetBorder(np);
  }
}
#endif /* USE_SHAPE */

/** Handle a colormap event. */
void Events::_HandleColormapChange(const XColormapEvent *event) {
  ClientNode *np;
  if (event->c_new == True) {
    np = ClientNode::FindClientByWindow(event->window);
    if (np) {
      np->UpdateClientColormap(event->colormap);
    }
  }
}

/** Handle a map request. */
void Events::_HandleMapRequest(const XMapEvent *event) {
  ClientNode *np;
  Assert(event);
  if (SwallowNode::CheckSwallowMap(event->window)) {
    return;
  }
  np = ClientNode::FindClientByWindow(event->window);
  if (!np) {
    Grabs::GrabServer();
    np = ClientNode::Create(event->window, 0, 1);
    if (np) {
      if (!(np->notFocusableIfMapped())) {
        np->keyboardFocus();
      }
    } else {
      JXMapWindow(display, event->window);
    }
    Grabs::UngrabServer();
  } else {
    if (!(np->isMapped())) {
      np->_UpdateState();
      np->setMapped();
      XMapWindow(display, np->getWindow());
      if (np->getParent() != None) {
        XMapWindow(display, np->getParent());
      }
      if (!(np->isSticky())) {
        np->setCurrentDesktop(currentDesktop);
      }
      if (!(np->notFocusableIfMapped())) {
        np->keyboardFocus();
        np->RaiseClient();
      }
      Hints::WriteState(np);
      _RequireTaskUpdate();
      _RequirePagerUpdate();
    }
  }
  _RequireRestack();
}

/** Handle an unmap notify event. */
void Events::_HandleUnmapNotify(const XUnmapEvent *event) {
  ClientNode *np;
  XEvent e;

  Assert(event);

  if (event->window != event->event) {
    /* Allow ICCCM synthetic UnmapNotify events through. */
    if (event->event != rootWindow || !event->send_event) {
      return;
    }
  }

  np = ClientNode::FindClientByWindow(event->window);
  if (np) {

    /* Grab the server to prevent the client from destroying the
     * window after we check for a DestroyNotify. */
    Grabs::GrabServer();

    if (np->getController()) {
      (np->getController())(1);
    }

    if (JXCheckTypedWindowEvent(display, np->getWindow(), DestroyNotify, &e)) {
      _UpdateTime(&e);
      np->RemoveClient();
    } else if ((np->isMapped()) || event->send_event) {
      if (!(np->isHidden())) {
        np->HideClient();
        JXUngrabButton(display, AnyButton, AnyModifier, np->getWindow());
        np->GravitateClient(1);
        JXReparentWindow(display, np->getWindow(), rootWindow, np->getX(),
            np->getY());
        Hints::WriteState(np);
        JXRemoveFromSaveSet(display, np->getWindow());
        np->RemoveClient();
      }
    }
    Grabs::UngrabServer();

  }
}

/** Handle a destroy notify event. */
char Events::_HandleDestroyNotify(const XDestroyWindowEvent *event) {
  ClientNode *np;
  np = ClientNode::FindClientByWindow(event->window);
  if (np) {
    if (np->getController()) {
      (np->getController())(1);
    }
    np->RemoveClient();
    return 1;
  } else {
    return DesktopEnvironment::DefaultEnvironment()->HandleDockDestroy(
        event->window);
  }
}

/** Update the last event time. */
void Events::_UpdateTime(const XEvent *event) {
  Time t = CurrentTime;
  Assert(event);
  switch (event->type) {
  case KeyPress:
  case KeyRelease:
    t = event->xkey.time;
    break;
  case ButtonPress:
  case ButtonRelease:
    t = event->xbutton.time;
    break;
  case MotionNotify:
    t = event->xmotion.time;
    break;
  case EnterNotify:
  case LeaveNotify:
    t = event->xcrossing.time;
    break;
  case PropertyNotify:
    t = event->xproperty.time;
    break;
  case SelectionClear:
    t = event->xselectionclear.time;
    break;
  case SelectionRequest:
    t = event->xselectionrequest.time;
    break;
  case SelectionNotify:
    t = event->xselection.time;
    break;
  default:
    break;
  }
  if (t != CurrentTime) {
    if (t > eventTime || t < eventTime - 60000) {
      eventTime = t;
    }
  }
}

/** Register a callback. */
void Events::_RegisterCallback(int freq, SignalCallback callback, void *data) {
  Logger::Log("Logging callback\n");
  CallbackNode *cp = new CallbackNode;
  cp->last.seconds = 0;
  cp->last.ms = 0;
  cp->freq = freq;
  cp->callback = callback;
  cp->data = data;
  callbacks.push_back(cp);
}

/** Unregister a callback. */
void Events::_UnregisterCallback(SignalCallback callback, void *data) {
  std::vector<CallbackNode*>::iterator it;
  for (it = callbacks.begin(); it != callbacks.end(); ++it) {
    CallbackNode *cn = (*it);
    if (cn->callback == callback && cn->data == data) {
      callbacks.erase(it);
      delete cn;
      return;
    }

  }

}

/** Restack clients before waiting for an event. */
void Events::_RequireRestack() {
  restack_pending = 1;
}

/** Update the task bar before waiting for an event. */
void Events::_RequireTaskUpdate() {
  task_update_pending = 1;
}

/** Update the pager before waiting for an event. */
void Events::_RequirePagerUpdate() {
  pager_update_pending = 1;
}
