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
#include "Graphics.h"

std::vector<Clock*> Clock::clocks;

/** Initialize clocks. */
void Clock::InitializeClock(void) {

}

/** Start clock(s). */
void Clock::StartupClock(void) {

}

/** Destroy clock(s). */
void Clock::DestroyClock(void) {
  for (auto clock : clocks) {
    delete clock;
  }
  clocks.clear();
}

const char *Clock::DEFAULT_FORMAT = "%I:%M:%S %p";

/** Create a clock tray component. */
Clock::Clock(const char *format, const char *zone, int width,
    int height, Tray *tray, TrayComponent *parent) :
    TrayComponent(tray, parent) {
  this->mousex = -settings.doubleClickDelta;
  this->mousey = -settings.doubleClickDelta;
  this->mouseTime.seconds = 0;
  this->mouseTime.ms = 0;
  this->userWidth = 0;
  this->SetScreenLocation(10, 10);

  if (!format) {
    format = DEFAULT_FORMAT;
  }
  this->format = CopyString(format);
  this->zone = CopyString(zone);
  memset(&this->lastTime, 0, sizeof(this->lastTime));

  if (width > 0) {
    this->requestedWidth = width;
    this->userWidth = 1;
  } else {
    this->requestedWidth = 0;
    this->userWidth = 0;
  }
  this->requestedHeight = height;

  Events::_RegisterCallback(Min(900, settings.popupDelay / 2), SignalClock, this);
}

Clock::~Clock() {
  if (this->format) {
    Release(this->format);
  }
  if (this->zone) {
    Release(this->zone);
  }
  Events::_UnregisterCallback(SignalClock, this);
}

/** Initialize a clock tray component. */
void Clock::Create() {

}

/** Resize a clock tray component. */
void Clock::Resize() {
  TrayComponent::Resize();
//  TimeType now;
//  memset(&this->lastTime, 0, sizeof(this->lastTime));
//  GetCurrentTime(&now);
//  this->DrawClock(&now);
  if (this->getPixmap() != None) {
    this->graphics->free();
    Graphics::destroy(this->graphics);
  }

  this->setPixmap(
      JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(),
          rootDepth));
  this->graphics = Graphics::wrap(this->pixmap, rootGC, display);

  Draw();
}

void Clock::Draw() {
  TimeType now;
  GetCurrentTime(&now);
  this->DrawClock(&now);
}

/** Destroy a clock tray component. */
void Clock::Destroy() {

}

/** Process a press event on a clock tray component. */
void Clock::ProcessButtonPress(int x, int y, int button) {
  this->handlePressActions(x, y, button);
}

void Clock::ProcessButtonRelease(int x, int y, int button) {
  this->handleReleaseActions(x, y, button);
}

/** Process a motion event on a clock tray component. */
void Clock::ProcessMotionEvent(int x, int y, int mask) {
  Clock *clk = (Clock*) this;
  clk->mousex = this->getScreenX() + x;
  clk->mousey = this->getScreenY() + y;
  GetCurrentTime(&clk->mouseTime);
}

/** Update a clock tray component. */
void Clock::SignalClock(const TimeType *now, int x, int y, Window w,
    void *data) {
  const char *longTime;
  Clock *clk = (Clock*) data;

  clk->DrawClock(now);
  if (clk->getTray()->getWindow() == w
      && abs(clk->mousex - x) < settings.doubleClickDelta
      && abs(clk->mousey - y) < settings.doubleClickDelta) {
    if (GetTimeDifference(now, &clk->mouseTime) >= settings.popupDelay) {
      longTime = GetTimeString("%c", clk->zone);
      Popups::ShowPopup(x, y, longTime, POPUP_CLOCK);
    }
  }

}

void Clock::Draw(Graphics *g) {

}

/** Draw a clock tray component. */
void Clock::DrawClock(const TimeType *now) {
  const char *timeString;
  int strWidth;
  int rwidth;

  graphics->setForeground(COLOR_MENU_BG);
  graphics->fillRectangle(0, 0, this->getWidth(), this->getHeight());
  // clear area
//  /* Determine if the clock is the right size. */
  timeString = GetTimeString(this->format, this->zone);
  strWidth = Fonts::GetStringWidth(FONT_CLOCK, timeString)+4;
//

  //static char buf[80];
  //sprintf(buf, "%d%%", (int) 50);
  //int strWidth = Fonts::GetStringWidth(FONT_CLOCK, buf);
  //strWidth += 16;
  if (strWidth == this->getRequestedWidth()) {
    Fonts::RenderString(this->getPixmap(), FONT_CLOCK, COLOR_CLOCK_FG,
        (this->getWidth() - strWidth) / 2,
        (this->getHeight() - Fonts::GetStringHeight(FONT_CLOCK)) / 2,
        this->getWidth(), timeString);

    this->UpdateSpecificTray(this->getTray());
  } else {
    //Warning(_("Requesting the tray to give us a better size"));
    this->requestNewSize(strWidth, this->getRequestedHeight());
    this->getTray()->ResizeTray();
  }
  this->graphics->setForeground(COLOR_MENU_ACTIVE_BG1);
  this->graphics->drawRectangle(0, 0, this->getWidth()-1, this->getHeight()-1);
  graphics->copy(this->getTray()->getWindow(), 0, 0, this->getWidth(), this->getHeight(),
      this->getX(), this->getY());

}

Clock* Clock::CreateClock(const char *format, const char *zone,
    int width, int height, Tray *tray, TrayComponent *parent) {
  Clock *type = new Clock(format, zone, width, height, tray, parent);
  clocks.push_back(type);
  return type;
}
