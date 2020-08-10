/**
 * @file move.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client window move functions.
 *
 */

#include "nwm.h"
#include "move.h"

#include "border.h"
#include "client.h"
#include "clientlist.h"
#include "cursor.h"
#include "event.h"
#include "outline.h"
#include "screen.h"
#include "status.h"
#include "settings.h"
#include "timing.h"
#include "DesktopEnvironment.h"

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

static void DoSnapScreen(ClientNode *np);
static void DoSnapBorder(ClientNode *np);
static char ShouldSnap(ClientNode *np);

static char CheckLeftValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *left);
static char CheckRightValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *right);
static char CheckTopValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *top);
static char CheckBottomValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *bottom);

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

/** Determine if we should snap to the specified client. */
char ShouldSnap(ClientNode *np) {
  if (np->isHidden()) {
    return 0;
  } else if (np->isMinimized()) {
    return 0;
  } else {
    return 1;
  }
}

/** Get a rectangle to represent a client window. */
void GetClientRectangle(ClientNode *np, ClientRectangle *r) {

  int north, south, east, west;

  Border::GetBorderSize(np, &north, &south, &east, &west);

  r->left = np->getX() - west;
  r->right = np->getX() + np->getWidth() + east;
  r->top = np->getY() - north;
  if (np->isShaded()) {
    r->bottom = np->getY() + south;
  } else {
    r->bottom = np->getY() + np->getHeight() + south;
  }

  r->valid = 1;

}

/** Check for top/bottom overlap. */
char CheckOverlapTopBottom(const ClientRectangle *a, const ClientRectangle *b) {
  if (a->top >= b->bottom) {
    return 0;
  } else if (a->bottom <= b->top) {
    return 0;
  } else {
    return 1;
  }
}

/** Check for left/right overlap. */
char CheckOverlapLeftRight(const ClientRectangle *a, const ClientRectangle *b) {
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
char CheckLeftValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *left) {

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
char CheckRightValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *right) {

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
char CheckTopValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *top) {

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
char CheckBottomValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *bottom) {

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
    Events::_RequireRestack();
  } else if (atRight && DesktopEnvironment::DefaultEnvironment()->RightDesktop()) {
    currentClient->SetClientDesktop(currentDesktop);
    Events::_RequireRestack();
  } else if (atTop && DesktopEnvironment::DefaultEnvironment()->AboveDesktop()) {
    currentClient->SetClientDesktop(currentDesktop);
    Events::_RequireRestack();
  } else if (atBottom && DesktopEnvironment::DefaultEnvironment()->BelowDesktop()) {
    currentClient->SetClientDesktop(currentDesktop);
    Events::_RequireRestack();
  }
  currentClient->setNotHidden();
}
