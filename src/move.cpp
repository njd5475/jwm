/**
 * @file move.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client window move functions.
 *
 */

#include "jwm.h"
#include "move.h"

#include "border.h"
#include "client.h"
#include "clientlist.h"
#include "cursor.h"
#include "event.h"
#include "outline.h"
#include "pager.h"
#include "screen.h"
#include "status.h"
#include "tray.h"
#include "settings.h"
#include "timing.h"
#include "binding.h"
#include "DesktopEnvironment.h"

typedef struct {
  int left, right;
  int top, bottom;
  char valid;
} RectangleType;

static char shouldStopMove;
static char atLeft;
static char atRight;
static char atBottom;
static char atTop;
static char atSideFirst;
static ClientNode *currentClient;
static TimeType moveTime;

static void StopMove(ClientNode *np, int doMove, int oldx, int oldy);
static void RestartMove(ClientNode *np, int *doMove);
static void MoveController(int wasDestroyed);

static void DoSnap(ClientNode *np);
static void DoSnapScreen(ClientNode *np);
static void DoSnapBorder(ClientNode *np);
static char ShouldSnap(ClientNode *np);
static void GetClientRectangle(ClientNode *np, RectangleType *r);

static char CheckOverlapTopBottom(const RectangleType *a, const RectangleType *b);
static char CheckOverlapLeftRight(const RectangleType *a, const RectangleType *b);

static char CheckLeftValid(const RectangleType *client, const RectangleType *other, const RectangleType *left);
static char CheckRightValid(const RectangleType *client, const RectangleType *other, const RectangleType *right);
static char CheckTopValid(const RectangleType *client, const RectangleType *other, const RectangleType *top);
static char CheckBottomValid(const RectangleType *client, const RectangleType *other, const RectangleType *bottom);

static void SignalMove(const TimeType *now, int x, int y, Window w, void *data);
static void UpdateDesktop(const TimeType *now);

/** Callback for stopping moves. */
void MoveController(int wasDestroyed) {
  if (settings.moveMode == MOVE_OUTLINE) {
    Outline::ClearOutline();
  }

  JXUngrabPointer(display, CurrentTime);
  JXUngrabKeyboard(display, CurrentTime);

  DestroyMoveWindow();
  shouldStopMove = 1;
  atTop = 0;
  atBottom = 0;
  atLeft = 0;
  atRight = 0;
  atSideFirst = 0;
}

/** Move a client window. */
char ClientNode::MoveClient(int startx, int starty) {
  XEvent event;
  const ScreenType *sp;
  MaxFlags flags;
  int oldx, oldy;
  int doMove;
  int north, south, east, west;
  int height;

  if (!(this->getState()->getBorder() & BORDER_MOVE)) {
    return 0;
  }
  if (this->getState()->getStatus() & STAT_FULLSCREEN) {
    return 0;
  }

  if (!Cursors::GrabMouseForMove()) {
    return 0;
  }

  _RegisterCallback(0, SignalMove, NULL);
  this->controller = MoveController;
  shouldStopMove = 0;

  oldx = this->getX();
  oldy = this->getY();

  if (!(Cursors::GetMouseMask() & (Button1Mask | Button2Mask))) {
    this->StopMove(0, oldx, oldy);
    return 0;
  }

  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
  startx -= west;
  starty -= north;

  currentClient = this;
  atTop = atBottom = atLeft = atRight = atSideFirst = 0;
  doMove = 0;
  for (;;) {

    _WaitForEvent(&event);

    if (shouldStopMove) {
      this->controller = NULL;
      Cursors::SetDefaultCursor(this->parent);
      _UnregisterCallback(SignalMove, NULL);
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

      _DiscardMotionEvents(&event, this->getWindow());

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
      sp = Screens::GetCurrentScreen(this->getX() + this->getWidth() / 2, this->getY() + this->getHeight() / 2);
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
        if (this->getState()->getStatus() & STAT_AEROSNAP) {
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
          if (flags != this->getState()->getMaxFlags()) {
            if (settings.moveMode == MOVE_OUTLINE) {
              Outline::ClearOutline();
            }
            this->MaximizeClient(flags);
          }
          if (!this->getState()->getMaxFlags()) {
            DoSnap(this);
          }
        } else {
          DoSnap(this);
        }
      }

      if (flags != MAX_NONE) {
        this->RestartMove(&doMove);
      } else if (!doMove && (abs(this->getX() - oldx) > MOVE_DELTA || abs(this->getY() - oldy) > MOVE_DELTA)) {

        if (this->getState()->getMaxFlags()) {
          this->MaximizeClient(MAX_NONE);
        }

        CreateMoveWindow(this);
        doMove = 1;
      }

      if (doMove) {
        if (settings.moveMode == MOVE_OUTLINE) {
          Outline::ClearOutline();
          height = north + south;
          if (!(this->getState()->getStatus() & STAT_SHADED)) {
            height += this->getHeight();
          }
          Outline::DrawOutline(this->getX() - west, this->getY() - north, this->getWidth() + west + east, height);
        } else {
          if (this->getParent() != None) {
            JXMoveWindow(display, this->getParent(), this->getX() - west, this->getY() - north);
          } else {
            JXMoveWindow(display, this->getWindow(), this->getX(), this->getY());
          }
          this->SendConfigureEvent();
        }
        UpdateMoveWindow(this);
        _RequirePagerUpdate();
      }

      break;
    default:
      break;
    }
  }
}

/** Move a client window (keyboard or menu initiated). */
char ClientNode::MoveClientKeyboard() {
  XEvent event;
  int oldx, oldy;
  int moved;
  int height;
  int north, south, east, west;
  Window win;
  if (!(this->getState()->getBorder() & BORDER_MOVE)) {
    return 0;
  }
  if (this->getState()->getStatus() & STAT_FULLSCREEN) {
    return 0;
  }

  if (this->getState()->getMaxFlags() != MAX_NONE) {
    this->MaximizeClient(MAX_NONE);
  }

  win = this->parent != None ? this->parent : this->window;
  if (JUNLIKELY(JXGrabKeyboard(display, win, True, GrabModeAsync, GrabModeAsync, CurrentTime))) {
    Debug("MoveClient: could not grab keyboard");
    return 0;
  }
  if (!Cursors::GrabMouseForMove()) {
    JXUngrabKeyboard(display, CurrentTime);
    return 0;
  }

  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);

  oldx = this->getX();
  oldy = this->getY();

  _RegisterCallback(0, SignalMove, NULL);
  this->controller = MoveController;
  shouldStopMove = 0;

  CreateMoveWindow(this);
  UpdateMoveWindow(this);

  Cursors::MoveMouse(rootWindow, this->getX(), this->getY());
  _DiscardMotionEvents(&event, this->window);

  if (this->getState()->getStatus() & STAT_SHADED) {
    height = 0;
  } else {
    height = this->getHeight();
  }
  currentClient = this;

  for (;;) {

    _WaitForEvent(&event);

    if (shouldStopMove) {
      this->controller = NULL;
      Cursors::SetDefaultCursor(this->parent);
      _UnregisterCallback(SignalMove, NULL);
      return 1;
    }

    moved = 0;

    if (event.type == KeyPress) {
      ActionType action;

      _DiscardKeyEvents(&event, this->window);
      action = Binding::GetKey(MC_NONE, event.xkey.state, event.xkey.keycode);
      switch (action.action) {
      case UP:
        if (this->getY() + height > 0) {
          this->y -= 10;
        }
        break;
      case DOWN:
        if (this->getY() < rootHeight) {
          this->y += 10;
        }
        break;
      case RIGHT:
        if (this->getX() < rootWidth) {
          this->x += 10;
        }
        break;
      case LEFT:
        if (this->getX() + this->getWidth() > 0) {
          this->x -= 10;
        }
        break;
      default:
        this->StopMove(1, oldx, oldy);
        return 1;
      }

      Cursors::MoveMouse(rootWindow, this->getX(), this->getY());
      _DiscardMotionEvents(&event, this->window);

      moved = 1;

    } else if (event.type == MotionNotify) {

      _DiscardMotionEvents(&event, this->window);

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
        Outline::DrawOutline(this->getX() - west, this->getY() - west, this->getWidth() + west + east, height + north + west);
      } else {
        JXMoveWindow(display, win, this->getX() - west, this->getY() - north);
        this->SendConfigureEvent();
      }

      UpdateMoveWindow(this);
      _RequirePagerUpdate();

    }

  }

}

/** Stop move. */
void ClientNode::StopMove(int doMove, int oldx, int oldy) {
  int north, south, east, west;

  Assert(this->controller);

  (this->controller)(0);

  this->controller = NULL;

  Cursors::SetDefaultCursor(this->parent);
  _UnregisterCallback(SignalMove, NULL);

  if (!doMove) {
    this->x = oldx;
    this->y = oldy;
    return;
  }

  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
  if (this->parent != None) {
    JXMoveWindow(display, this->parent, this->getX() - west, this->getY() - north);
  } else {
    JXMoveWindow(display, this->window, this->getX() - west, this->getY() - north);
  }
  this->SendConfigureEvent();
}

/** Restart a move. */
void ClientNode::RestartMove(int *doMove) {
  if (*doMove) {
    int north, south, east, west;
    *doMove = 0;
    DestroyMoveWindow();
    Border::GetBorderSize(this->getState(), &north, &south, &east, &west);
    if (this->getParent() != None) {
      JXMoveWindow(display, this->getParent(), this->getX() - west, this->getY() - north);
    } else {
      JXMoveWindow(display, this->getWindow(), this->getX() - west, this->getY() - north);
    }
    this->SendConfigureEvent();
  }
}

/** Snap to the screen and/or neighboring windows. */
void DoSnap(ClientNode *np) {
  switch (settings.snapMode) {
  case SNAP_BORDER:
    np->DoSnapBorder();
    np->DoSnapScreen();
    break;
  case SNAP_SCREEN:
    np->DoSnapScreen();
    break;
  default:
    break;
  }
}

/** Snap to the screen. */
void ClientNode::DoSnapScreen() {

  RectangleType client;
  int screen;
  const ScreenType *sp;
  int screenCount;
  int north, south, east, west;

  GetClientRectangle(this, &client);

  Border::GetBorderSize(this->getState(), &north, &south, &east, &west);

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
      if (!(this->getState()->getStatus() & STAT_SHADED)) {
        this->y -= this->getHeight();
      }
    }
    if (abs(client.top - sp->y) <= settings.snapDistance) {
      this->y = north + sp->y;
    }

  }

}

/** Snap to window borders. */
void ClientNode::DoSnapBorder() {

  ClientNode *tp;
  const TrayType *tray;
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
    for (tray = TrayType::GetTrays(); tray; tray = tray->getNext()) {

      if (tray->isHidden()) {
        continue;
      }

      other.left = tray->getX();
      other.right = tray->getX() + tray->getWidth();
      other.top = tray->getY();
      other.bottom = tray->getY() + tray->getHeight();

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
    for (tp = nodeTail[layer]; tp; tp = tp->prev) {

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

/** Determine if we should snap to the specified client. */
char ShouldSnap(ClientNode *np) {
  if (np->getState()->getStatus() & STAT_HIDDEN) {
    return 0;
  } else if (np->getState()->getStatus() & STAT_MINIMIZED) {
    return 0;
  } else {
    return 1;
  }
}

/** Get a rectangle to represent a client window. */
void GetClientRectangle(ClientNode *np, RectangleType *r) {

  int north, south, east, west;

  Border::GetBorderSize(np->getState(), &north, &south, &east, &west);

  r->left = np->getX() - west;
  r->right = np->getX() + np->getWidth() + east;
  r->top = np->getY() - north;
  if (np->getState()->getStatus() & STAT_SHADED) {
    r->bottom = np->getY() + south;
  } else {
    r->bottom = np->getY() + np->getHeight() + south;
  }

  r->valid = 1;

}

/** Check for top/bottom overlap. */
char CheckOverlapTopBottom(const RectangleType *a, const RectangleType *b) {
  if (a->top >= b->bottom) {
    return 0;
  } else if (a->bottom <= b->top) {
    return 0;
  } else {
    return 1;
  }
}

/** Check for left/right overlap. */
char CheckOverlapLeftRight(const RectangleType *a, const RectangleType *b) {
  if (a->left >= b->right) {
    return 0;
  } else if (a->right <= b->left) {
    return 0;
  } else {
    return 1;
  }
}

/** Check if the current left snap position is valid.
 * @param client The window being moved.
 * @param other A window higher in stacking order than
 * previously check windows.
 * @param left The top/bottom of the current left snap window.
 * @return 1 if the current left snap position is still valid, otherwise 0.
 */
char CheckLeftValid(const RectangleType *client, const RectangleType *other, const RectangleType *left) {

  if (!left->valid) {
    return 0;
  }

  if (left->right > other->right) {
    return 1;
  }

  /* If left and client go higher than other then still valid. */
  if (left->top < other->top && client->top < other->top) {
    return 1;
  }

  /* If left and client go lower than other then still valid. */
  if (left->bottom > other->bottom && client->bottom > other->bottom) {
    return 1;
  }

  if (other->left >= left->right) {
    return 1;
  }

  return 0;

}

/** Check if the current right snap position is valid. */
char CheckRightValid(const RectangleType *client, const RectangleType *other, const RectangleType *right) {

  if (!right->valid) {
    return 0;
  }

  if (right->left < other->left) {
    return 1;
  }

  /* If right and client go higher than other then still valid. */
  if (right->top < other->top && client->top < other->top) {
    return 1;
  }

  /* If right and client go lower than other then still valid. */
  if (right->bottom > other->bottom && client->bottom > other->bottom) {
    return 1;
  }

  if (other->right <= right->left) {
    return 1;
  }

  return 0;

}

/** Check if the current top snap position is valid. */
char CheckTopValid(const RectangleType *client, const RectangleType *other, const RectangleType *top) {

  if (!top->valid) {
    return 0;
  }

  if (top->bottom > other->bottom) {
    return 1;
  }

  /* If top and client are to the left of other then still valid. */
  if (top->left < other->left && client->left < other->left) {
    return 1;
  }

  /* If top and client are to the right of other then still valid. */
  if (top->right > other->right && client->right > other->right) {
    return 1;
  }

  if (other->top >= top->bottom) {
    return 1;
  }

  return 0;

}

/** Check if the current bottom snap position is valid. */
char CheckBottomValid(const RectangleType *client, const RectangleType *other, const RectangleType *bottom) {

  if (!bottom->valid) {
    return 0;
  }

  if (bottom->top < other->top) {
    return 1;
  }

  /* If bottom and client are to the left of other then still valid. */
  if (bottom->left < other->left && client->left < other->left) {
    return 1;
  }

  /* If bottom and client are to the right of other then still valid. */
  if (bottom->right > other->right && client->right > other->right) {
    return 1;
  }

  if (other->bottom <= bottom->top) {
    return 1;
  }

  return 0;

}

/** Switch desktops if appropriate. */
void SignalMove(const TimeType *now, int x, int y, Window w, void *data) {
  UpdateDesktop(now);
}

/** Switch to the specified desktop. */
void UpdateDesktop(const TimeType *now) {
  if (settings.desktopDelay == 0) {
    return;
  }
  if (GetTimeDifference(now, &moveTime) < settings.desktopDelay) {
    return;
  }
  moveTime = *now;

  /* We temporarily mark the client as hidden to avoid hidding it
   * when changing desktops. */
  currentClient->setHidden();
  if (atLeft && DesktopEnvironment::DefaultEnvironment()->LeftDesktop()) {
    currentClient->SetClientDesktop(currentDesktop);
    _RequireRestack();
  } else if (atRight && DesktopEnvironment::DefaultEnvironment()->RightDesktop()) {
    currentClient->SetClientDesktop(currentDesktop);
    _RequireRestack();
  } else if (atTop && DesktopEnvironment::DefaultEnvironment()->AboveDesktop()) {
    currentClient->SetClientDesktop(currentDesktop);
    _RequireRestack();
  } else if (atBottom && DesktopEnvironment::DefaultEnvironment()->BelowDesktop()) {
    currentClient->SetClientDesktop(currentDesktop);
    _RequireRestack();
  }
  currentClient->setNotHidden();
}
