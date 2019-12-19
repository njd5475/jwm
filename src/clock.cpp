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

std::vector<ClockType*> ClockType::clocks;

/** Initialize clocks. */
void ClockType::InitializeClock(void) {

}

/** Start clock(s). */
void ClockType::StartupClock(void) {

}

/** Destroy clock(s). */
void ClockType::DestroyClock(void) {
  for (auto clock : clocks) {
    delete clock;
  }
  clocks.clear();
}

const char *ClockType::DEFAULT_FORMAT = "%I:%M:%S %p";

/** Create a clock tray component. */
ClockType::ClockType(const char *format, const char *zone, int width,
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

  _RegisterCallback(Min(900, settings.popupDelay / 2), SignalClock, this);
}

ClockType::~ClockType() {
  if (this->format) {
    Release(this->format);
  }
  if (this->zone) {
    Release(this->zone);
  }
  _UnregisterCallback(SignalClock, this);
}

/** Initialize a clock tray component. */
void ClockType::Create() {

}

/** Resize a clock tray component. */
void ClockType::Resize() {
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

void ClockType::Draw() {
  TimeType now;
  GetCurrentTime(&now);
  this->DrawClock(&now);
}

/** Destroy a clock tray component. */
void ClockType::Destroy() {

}

/** Process a press event on a clock tray component. */
void ClockType::ProcessButtonPress(int x, int y, int button) {
  this->handlePressActions(x, y, button);
}

void ClockType::ProcessButtonRelease(int x, int y, int button) {
  this->handleReleaseActions(x, y, button);
}

/** Process a motion event on a clock tray component. */
void ClockType::ProcessMotionEvent(int x, int y, int mask) {
  ClockType *clk = (ClockType*) this;
  clk->mousex = this->getScreenX() + x;
  clk->mousey = this->getScreenY() + y;
  GetCurrentTime(&clk->mouseTime);
}

/** Update a clock tray component. */
void ClockType::SignalClock(const TimeType *now, int x, int y, Window w,
    void *data) {
  const char *longTime;
  ClockType *clk = (ClockType*) data;

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

void ClockType::Draw(Graphics *g) {

}

/** Draw a clock tray component. */
void ClockType::DrawClock(const TimeType *now) {
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

  graphics->copy(this->getTray()->getWindow(), 0, 0, this->getWidth(), this->getHeight(),
      this->getX(), this->getY());

}

ClockType* ClockType::CreateClock(const char *format, const char *zone,
    int width, int height, Tray *tray, TrayComponent *parent) {
  ClockType *type = new ClockType(format, zone, width, height, tray, parent);
  clocks.push_back(type);
  return type;
}
