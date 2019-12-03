/**
 * @file Battery.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Battery tray component.
 *
 */

#include "jwm.h"
#include "battery.h"
#include "tray.h"
#include "color.h"
#include "font.h"
#include "timing.h"
#include "main.h"
#include "command.h"
#include "cursor.h"
#include "popup.h"
#include "misc.h"
#include "error.h"
#include "settings.h"
#include "event.h"
#include "action.h"
#include "Graphics.h"

static void PollBattery(const struct TimeType *now, int x, int y, Window w,
    void *data);
static char* quickFileRead(int fd);
static int readAsInt(int fd);
static float QueryBatteryPercentage();

#include <fcntl.h>
#define FILEMODE S_IRWXU | S_IRGRP | S_IROTH

static int chargeNowFile;
static int chargeFullFile;

/** Initialize Batterys. */
void Battery::InitializeBattery(void) {
  Warning(_("Battery tray has been initialized"));
}

/** Destroy Battery(s). */
void Battery::DestroyBattery(void) {
  close(chargeFullFile);
  close(chargeNowFile);
  Warning(_("Battery destroyed files closed"));
}

/** Start Battery(s). */
void Battery::StartupBattery(void) {
  if ((chargeNowFile = open("/sys/class/power_supply/BAT1/charge_now", O_RDONLY,
  FILEMODE)) < 0) {
    perror("Error in file opening");
    return;
  }
  if ((chargeFullFile = open("/sys/class/power_supply/BAT1/charge_full",
  O_RDONLY, FILEMODE)) < 0) {
    perror("Error in file opening");
    return;
  }
  Warning(_("Battery started and files opened bat at %.02f"),
      QueryBatteryPercentage());
  Warning(_("Battery started and files opened bat at %.02f"),
      QueryBatteryPercentage());
}

/** Create a Battery tray component. */
Battery::Battery(int width, int height, Tray *tray, TrayComponent *parent) :
    TrayComponent(tray, parent), lastLevel(0.0) {
  Warning(_("Creating Battery Component"));
  this->SetSize(width, height);

  this->setPixmap(
      JXCreatePixmap(display, rootWindow, width, height, rootDepth));
  this->graphics = Graphics::wrap(this->getPixmap(), rootGC, display);

  _RegisterCallback(900, PollBattery, this);
}

Battery::~Battery() {

}

/** Add an action to a Battery. */
void Battery::AddBatteryAction(const char *action, int mask) {
}

/** Resize a Battery tray component. */
void Battery::Resize() {
  TrayComponent::Resize();
  Warning(_("Battery received a resize call, width: %d, height: %d"),
      this->getWidth(), this->getHeight());
  TimeType now;

  if (this->getPixmap() != None) {
    Warning(_("Battery pixmap released!"));
    this->graphics->free();
    Graphics::destroy(this->graphics);
  }

  this->setPixmap(
      JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(),
          rootDepth));
  this->graphics = Graphics::wrap(this->pixmap, rootGC, display);

  Draw();
}

void Battery::Create() {

}

void Battery::Draw(Graphics *g) {

}

float QueryBatteryPercentage() {
  int chargeNow = (int) readAsInt(chargeNowFile);
  int chargeFull = (int) readAsInt(chargeFullFile);
  return (100 * (chargeNow / (float) chargeFull));
}

/** Update a Battery tray component. */
void PollBattery(const TimeType *now, int x, int y, Window w, void *data) {

  ((Battery*) data)->Draw();
}

/** Draw a Battery tray component. */
void Battery::Draw() {

  float percentage = QueryBatteryPercentage();
  if (percentage == this->lastLevel) {
    return; //no change
  }

  this->graphics->setForeground(COLOR_MENU_FG);
  this->graphics->fillRectangle(0, 0, this->getWidth(), this->getHeight());

  static char buf[80];
  sprintf(buf, "%d%%", (int) percentage);
  int strWidth = Fonts::GetStringWidth(FONT_CLOCK, buf);
  strWidth += 16;
  if (strWidth == this->getRequestedWidth()) {
    Fonts::RenderString(this->getPixmap(), FONT_CLOCK, COLOR_CLOCK_FG,
        (this->getWidth() - strWidth) / 2,
        (this->getHeight() - Fonts::GetStringHeight(FONT_CLOCK)) / 2,
        this->getWidth(), buf);

    this->UpdateSpecificTray(this->getTray());
  } else {
    Warning(_("Requesting the tray to give us a better size"));
    this->requestNewSize(strWidth, this->getRequestedHeight());
    this->getTray()->ResizeTray();
  }

  //update battery level
  this->lastLevel = percentage;

  JXCopyArea(display, this->getPixmap(), window, rootGC, 0, 0, this->getWidth(),
      this->getHeight(), this->getX(), this->getY());
}

int readAsInt(int fd) {
  char buf[255];
  memset(buf, 0, 255);

  lseek(fd, 0, SEEK_SET);

  size_t count = read(fd, buf, 254);

  if (count < 0) {
    perror("Could not read any of the file!");
    return 0;
  }
  return (int) atoi(buf);
}

