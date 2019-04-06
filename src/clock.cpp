/**
 * @file clock.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Clock tray component.
 *
 */

#include "jwm.h"
#include "clock.h"
#include "tray.h"
#include "color.h"
#include "font.h"
#include "timing.h"
#include "main.h"
#include "command.h"
#include "cursor.h"
#include "popup.h"
#include "misc.h"
#include "settings.h"
#include "event.h"
#include "action.h"

static ClockType *ClockType::clocks;

/** Initialize clocks. */
void ClockType::InitializeClock(void) {
  clocks = NULL;
}

/** Start clock(s). */
void ClockType::StartupClock(void) {
  ClockType *clk;
  for (clk = clocks; clk; clk = clk->getNext()) {
    int newWidth = clk->cp->getWidth();
    int newHeight = clk->cp->getHeight();
    if (clk->cp->getRequestedWidth() == 0) {
      newWidth = 1;
    }
    if (clk->cp->getRequestedHeight() == 0) {
      newHeight = GetStringHeight(FONT_CLOCK) + 4;
    }
    clk->cp->requestNewSize(newWidth, newHeight);
  }
}

/** Destroy clock(s). */
void ClockType::DestroyClock(void) {
  while (clocks) {
    ClockType *cp = clocks->next;

    if (clocks->format) {
      Release(clocks->format);
    }
    if (clocks->zone) {
      Release(clocks->zone);
    }
    _UnregisterCallback(SignalClock, clocks);

    Release(clocks);
    clocks = cp;
  }
}

const char *ClockType::DEFAULT_FORMAT;

/** Create a clock tray component. */
ClockType::ClockType(const char *format, const char *zone, int width, int height)
: TrayComponentType() {
  this->next = clocks;
  clocks = this;

  this->mousex = -settings.doubleClickDelta;
  this->mousey = -settings.doubleClickDelta;
  this->mouseTime.seconds = 0;
  this->mouseTime.ms = 0;
  this->userWidth = 0;

  if (!format) {
    format = DEFAULT_FORMAT;
  }
  this->format = CopyString(format);
  this->zone = CopyString(zone);
  memset(&this->lastTime, 0, sizeof(this->lastTime));

  this->object = this;
  this->cp = this;
  if (width > 0) {
    this->requestedWidth = width;
    this->userWidth = 1;
  } else {
    this->requestedWidth = 0;
    this->userWidth = 0;
  }
  this->requestedHeight = height;

  _RegisterCallback(Min(900, settings.popupDelay / 2), SignalClock, this);
}

/** Initialize a clock tray component. */
void ClockType::Create(TrayComponentType *cp) {
  cp->setPixmap(JXCreatePixmap(display, rootWindow, cp->getWidth(), cp->getHeight(), rootDepth));
}

/** Resize a clock tray component. */
void ClockType::Resize(TrayComponentType *cp) {

  ClockType *clk;
  TimeType now;

  Assert(cp);

  clk = (ClockType*) cp->getObject();

  Assert(clk);

  if (cp->getPixmap() != None) {
    JXFreePixmap(display, cp->getPixmap());
  }

  cp->setPixmap(JXCreatePixmap(display, rootWindow, cp->getWidth(), cp->getHeight(), rootDepth));

  memset(&clk->lastTime, 0, sizeof(clk->lastTime));

  GetCurrentTime(&now);
  DrawClock(clk, &now);

}

/** Destroy a clock tray component. */
void ClockType::Destroy(TrayComponentType *cp) {
  Assert(cp);
  if (cp->getPixmap() != None) {
    JXFreePixmap(display, cp->getPixmap());
  }
}

/** Process a press event on a clock tray component. */
void ClockType::ProcessClockButtonPress(TrayComponentType *cp, int x, int y, int button) {
  cp->handlePressActions(x,y,button);
}

void ClockType::ProcessClockButtonRelease(TrayComponentType *cp, int x, int y, int button) {
  cp->handleReleaseActions(x,y,button);
}

/** Process a motion event on a clock tray component. */
void ClockType::ProcessClockMotionEvent(TrayComponentType *cp, int x, int y, int mask) {
  ClockType *clk = (ClockType*) cp->getObject();
  clk->mousex = cp->getScreenX() + x;
  clk->mousey = cp->getScreenY() + y;
  GetCurrentTime(&clk->mouseTime);
}

/** Update a clock tray component. */
void ClockType::SignalClock(const TimeType *now, int x, int y, Window w, void *data) {
  ClockType *cp = (ClockType*) data;
  const char *longTime;

  DrawClock(cp, now);
  if (cp->cp->getTray()->getWindow() == w && abs(cp->mousex - x) < settings.doubleClickDelta
      && abs(cp->mousey - y) < settings.doubleClickDelta) {
    if (GetTimeDifference(now, &cp->mouseTime) >= settings.popupDelay) {
      longTime = GetTimeString("%c", cp->zone);
      ShowPopup(x, y, longTime, POPUP_CLOCK);
    }
  }

}

/** Draw a clock tray component. */
void ClockType::DrawClock(ClockType *clk, const TimeType *now) {

  TrayComponentType *cp;
  const char *timeString;
  int width;
  int rwidth;

  /* Only draw if the time changed. */
  if (now->seconds == clk->lastTime.seconds) {
    return;
  }

  /* Clear the area. */
  cp = clk->cp;
  if (colors[COLOR_CLOCK_BG1] == colors[COLOR_CLOCK_BG2]) {
    JXSetForeground(display, rootGC, colors[COLOR_CLOCK_BG1]);
    JXFillRectangle(display, cp->getPixmap(), rootGC, 0, 0, cp->getWidth(), cp->getHeight());
  } else {
    DrawHorizontalGradient(cp->getPixmap(), rootGC, colors[COLOR_CLOCK_BG1], colors[COLOR_CLOCK_BG2], 0, 0,
        cp->getWidth(), cp->getHeight());
  }

  /* Determine if the clock is the right size. */
  timeString = GetTimeString(clk->format, clk->zone);
  width = GetStringWidth(FONT_CLOCK, timeString);
  rwidth = width + 4;
  if (rwidth == clk->cp->getRequestedWidth() || clk->userWidth) {

    /* Draw the clock. */
    RenderString(cp->getPixmap(), FONT_CLOCK, COLOR_CLOCK_FG, (cp->getWidth() - width) / 2,
        (cp->getHeight() - GetStringHeight(FONT_CLOCK)) / 2, cp->getWidth(), timeString);

    clk->cp->UpdateSpecificTray(clk->cp->getTray());

  } else {

    /* Wrong size. Resize. */
    clk->cp->requestNewSize(rwidth, clk->cp->getHeight());
    clk->cp->getTray()->ResizeTray();

  }

}
