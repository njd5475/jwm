/**
 * @file resize.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle resizing client windows.
 *
 */

#include "jwm.h"
#include "resize.h"
#include "client.h"
#include "outline.h"
#include "cursor.h"
#include "misc.h"
#include "pager.h"
#include "status.h"
#include "event.h"
#include "settings.h"
#include "binding.h"

static char shouldStopResize;

static void ResizeController(int wasDestroyed);

/** Callback to stop a resize. */
void ResizeController(int wasDestroyed) {
  if (settings.resizeMode == RESIZE_OUTLINE) {
    Outline::ClearOutline();
  }
  JXUngrabPointer(display, CurrentTime);
  JXUngrabKeyboard(display, CurrentTime);
  DestroyResizeWindow();
  shouldStopResize = 1;
}

/** Update the size of a client window. */
void ClientNode::UpdateSize(const MouseContextType context, const int x, const int y, const int startx,
    const int starty, const int oldx, const int oldy, const int oldw, const int oldh) {
  if (context & MC_BORDER_N) {
    int delta = (y - starty) / this->yinc;
    delta *= this->yinc;
    if (oldh - delta >= this->minHeight && (oldh - delta <= this->maxHeight || delta > 0)) {
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
    if (oldw - delta >= this->minWidth && (oldw - delta <= this->maxWidth || delta > 0)) {
      this->width = oldw - delta;
      this->x = oldx + delta;
    }
    if (!(context & (MC_BORDER_N | MC_BORDER_S))) {
      this->FixHeight();
    }
  }

  if (this->sizeFlags & PAspect) {
    if ((context & (MC_BORDER_N | MC_BORDER_S)) && (context & (MC_BORDER_E | MC_BORDER_W))) {

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

/** Resize a client window (mouse initiated). */
void ClientNode::ResizeClient(MouseContextType context, int startx, int starty) {

  XEvent event;
  int oldx, oldy;
  int oldw, oldh;
  int gwidth, gheight;
  int lastgwidth, lastgheight;
  int north, south, east, west;

  Assert(this);

  if (!(this->state.getBorder() & BORDER_RESIZE)) {
    return;
  }
  if (this->state.isFullscreen()) {
    return;
  }
  if (this->state.getMaxFlags()) {
    this->state.resetMaxFlags();
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

  Border::GetBorderSize(&this->state, &north, &south, &east, &west);

  startx += this->x - west;
  starty += this->y - north;

  CreateResizeWindow(this);
  UpdateResizeWindow(this, gwidth, gheight);

  if (!(Cursors::GetMouseMask() & (Button1Mask | Button3Mask))) {
    this->StopResize();
    return;
  }

  for (;;) {

    _WaitForEvent(&event);

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

      Cursors::SetMousePosition(event.xmotion.x_root, event.xmotion.y_root, event.xmotion.window);
      _DiscardMotionEvents(&event, this->window);

      this->UpdateSize(context, event.xmotion.x, event.xmotion.y, startx, starty, oldx, oldy, oldw, oldh);

      lastgwidth = gwidth;
      lastgheight = gheight;

      gwidth = (this->width - this->baseWidth) / this->xinc;
      gheight = (this->height - this->baseHeight) / this->yinc;

      if (lastgheight != gheight || lastgwidth != gwidth) {

        UpdateResizeWindow(this, gwidth, gheight);

        if (settings.resizeMode == RESIZE_OUTLINE) {
          Outline::ClearOutline();
          if (this->state.isShaded()) {
            Outline::DrawOutline(this->x - west, this->y - north, this->width + west + east, north + south);
          } else {
            Outline::DrawOutline(this->x - west, this->y - north, this->width + west + east, this->height + north + south);
          }
        } else {
          Border::ResetBorder(this);
          this->SendConfigureEvent();
        }

        _RequirePagerUpdate();

      }

      break;
    default:
      break;
    }
  }

}

/** Resize a client window (keyboard or menu initiated). */
void ClientNode::ResizeClientKeyboard(MouseContextType context) {

  XEvent event;
  int gwidth, gheight;
  int lastgwidth, lastgheight;
  int north, south, east, west;
  int startx, starty;
  int oldx, oldy, oldw, oldh;
  int ratio, minr, maxr;

  Assert(this);

  if (!(this->state.getBorder() & BORDER_RESIZE)) {
    return;
  }
  if (this->state.isFullscreen()) {
    return;
  }
  if (this->state.getMaxFlags()) {
    this->state.resetMaxFlags();
    Hints::WriteState(this);
    Border::ResetBorder(this);
  }

  if (JUNLIKELY(JXGrabKeyboard(display, this->parent, True, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)) {
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

  Border::GetBorderSize(&this->state, &north, &south, &east, &west);

  CreateResizeWindow(this);
  UpdateResizeWindow(this, gwidth, gheight);

  if (context & MC_BORDER_N) {
    starty = this->y - north;
  } else if (context & MC_BORDER_S) {
    if (this->state.isShaded()) {
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
  _DiscardMotionEvents(&event, this->window);

  for (;;) {

    _WaitForEvent(&event);

    if (shouldStopResize) {
      this->controller = NULL;
      return;
    }

    if (event.type == KeyPress) {
      int deltax = 0;
      int deltay = 0;
      ActionType action;

      _DiscardKeyEvents(&event, this->window);
      action = Binding::GetKey(MC_NONE, event.xkey.state, event.xkey.keycode);
      switch (action.action) {
      case UP:
        deltay = Min(-this->yinc, -10);
        break;
      case DOWN:
        deltay = Max(this->yinc, 10);
        break;
      case RIGHT:
        deltax = Max(this->xinc, 10);
        break;
      case LEFT:
        deltax = Min(-this->xinc, -10);
        break;
      default:
        this->StopResize();
        return;
      }

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

      Cursors::SetMousePosition(event.xmotion.x_root, event.xmotion.y_root, event.xmotion.window);
      _DiscardMotionEvents(&event, this->window);

      this->UpdateSize(context, event.xmotion.x, event.xmotion.y, startx, starty, oldx, oldy, oldw, oldh);

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
        if (this->state.isShaded()) {
          Outline::DrawOutline(this->x - west, this->y - north, this->width + west + east, north + south);
        } else {
          Outline::DrawOutline(this->x - west, this->y - north, this->width + west + east, this->height + north + south);
        }
      } else {
        Border::ResetBorder(this);
        this->SendConfigureEvent();
      }

      _RequirePagerUpdate();

    }

  }

}

/** Stop a resize action. */
void ClientNode::StopResize() {
  this->controller = NULL;

  /* Set the old width/height if maximized so the window
   * is restored to the new size. */
  if (this->state.getMaxFlags() & MAX_VERT) {
    this->oldWidth = this->width;
    this->oldx = this->x;
  }
  if (this->state.getMaxFlags() & MAX_HORIZ) {
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
void ClientNode::FixWidth() {
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
void ClientNode::FixHeight() {
  if ((this->sizeFlags & PAspect) && this->height > 0) {
    if (this->width * this->aspect.miny < this->height * this->aspect.minx) {
      this->height = (this->width * this->aspect.miny) / this->aspect.minx;
    }
    if (this->width * this->aspect.maxy > this->height * this->aspect.maxx) {
      this->height = (this->width * this->aspect.maxy) / this->aspect.maxx;
    }
  }
}

