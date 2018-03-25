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

/** Structure to represent a Battery tray component. */
typedef struct BatteryType {
  TrayComponentType *cp; /**< Common component data. */

  float lastLevel; /**< Currently displayed level */
  struct ActionNode *actions; /**< Actions */

  /* The following are used to control popups. */
  int mousex; /**< Last mouse x-coordinate. */
  int mousey; /**< Last mouse y-coordinate. */
  TimeType mouseTime; /**< Time of the last mouse motion. */

  int userWidth; /**< User-specified Battery width (or 0). */

  struct BatteryType *next; /**< Next Battery in the list. */
} BatteryType;

static BatteryType *batteries;

static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void ProcessBatteryButtonPress(TrayComponentType *cp, int x, int y, int button);
static void ProcessBatteryButtonRelease(TrayComponentType *cp, int x, int y, int button);
static void ProcessBatteryMotionEvent(TrayComponentType *cp, int x, int y, int mask);
static void DrawBattery(BatteryType *bat, float percentage);
static void PollBattery(const struct TimeType *now, int x, int y, Window w, void *data);
static char *quickFileRead(int fd);
static int *readAsInt(int fd);
static float QueryBatteryPercentage();

#include <fcntl.h>
#define FILEMODE S_IRWXU | S_IRGRP | S_IROTH

static int chargeNowFile;
static int chargeFullFile;

/** Initialize Batterys. */
void InitializeBattery(void) {
  Warning(_("Battery tray has been initialized"));
  batteries = NULL;
}

/** Destroy Battery(s). */
void DestroyBattery(void) {
  close(chargeFullFile);
  close(chargeNowFile);
  Warning(_("Battery destroyed files closed"));
}

/** Start Battery(s). */
void StartupBattery(void) {
  if((chargeNowFile = open("/sys/class/power_supply/BAT1/charge_now", O_RDONLY, FILEMODE)) < 0) {
    perror("Error in file opening");
    return NULL;
  }
  if((chargeFullFile = open("/sys/class/power_supply/BAT1/charge_full", O_RDONLY, FILEMODE)) < 0) {
    perror("Error in file opening");
    return NULL;
  }
  Warning(_("Battery started and files opened bat at %.02f"), QueryBatteryPercentage());
  Warning(_("Battery started and files opened bat at %.02f"), QueryBatteryPercentage());
}

/** Create a Battery tray component. */
TrayComponentType *CreateBattery(int width, int height) {
  Warning(_("Creating Battery Component"));
  TrayComponentType *cp;
  BatteryType *bat;
  bat = Allocate(sizeof(BatteryType));
  bat->next = batteries;

  batteries = bat; //move to head

  cp = CreateTrayComponent();
  cp->object = bat;
  cp->width = 20;
  cp->height = 20;
  bat->cp = cp;
  cp->Create = Create;
  cp->Resize = Resize;
  cp->Destroy = Destroy;
  cp->ProcessButtonPress = ProcessBatteryButtonPress;
  cp->ProcessButtonRelease = ProcessBatteryMotionEvent;

  RegisterCallback(900, PollBattery, bat);

  return cp;
}

/** Add an action to a Battery. */
void AddBatteryAction(TrayComponentType *cp, const char *action, int mask) {
}

/** Initialize a Battery tray component. */
void Create(TrayComponentType *cp) {
}

/** Resize a Battery tray component. */
void Resize(TrayComponentType *cp) {
  Warning(_("Battery received a resize call, width: %d, height: %d"), cp->width, cp->height);

  BatteryType *bat;
  TimeType now;

  Assert(cp);

  bat = (BatteryType*)cp->object;

  Assert(clk);

  if(cp->pixmap != None) {
    Warning(_("Battery pixmap released!"));
     JXFreePixmap(display, cp->pixmap);
  }

  cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height, rootDepth);

  DrawBattery(bat, QueryBatteryPercentage());
}

/** Destroy a Battery tray component. */
void Destroy(TrayComponentType *cp) {

}

/** Process a press event on a Battery tray component. */
void ProcessBatteryButtonPress(TrayComponentType *cp, int x, int y, int button) {
}

void ProcessBatteryButtonRelease(TrayComponentType *cp, int x, int y,
    int button) {
}

/** Process a motion event on a Battery tray component. */
void ProcessBatteryMotionEvent(TrayComponentType *cp, int x, int y, int mask) {
}

float QueryBatteryPercentage() {
  int chargeNow = readAsInt(chargeNowFile);
  int chargeFull = readAsInt(chargeFullFile);
  return (100 * (chargeNow / (float)chargeFull));
}

/** Update a Battery tray component. */
void PollBattery(const TimeType *now, int x, int y, Window w, void *data) {
  DrawBattery(data, QueryBatteryPercentage());
}

/** Draw a Battery tray component. */
void DrawBattery(BatteryType *bat, float percentage) {
  if(percentage == bat->lastLevel) {
    return; //short circuit since there was no change
  }

  JXSetForeground(display, rootGC, colors[COLOR_CLOCK_BG1]);
  JXFillRectangle(display, bat->cp->pixmap, rootGC, 0, 0, bat->cp->width, bat->cp->height);

  static char buf[80];
  sprintf(buf, "%d%%", (int)percentage);
  int strWidth = GetStringWidth(FONT_CLOCK, buf); 
  strWidth += 16;
  if(strWidth == bat->cp->requestedWidth) {
     RenderString(bat->cp->pixmap, FONT_CLOCK, COLOR_CLOCK_FG,
       (bat->cp->width - strWidth)/2, (bat->cp->height - GetStringHeight(FONT_CLOCK))/2, bat->cp->width, buf);

     UpdateSpecificTray(bat->cp->tray, bat->cp);
  } else {
    Warning(_("Requesting the tray to give us a better size"));
    bat->cp->requestedWidth = strWidth;
    ResizeTray(bat->cp->tray);
  }

  //update battery level
  bat->lastLevel = percentage;
}

int *readAsInt(int fd) {
  char *str = quickFileRead(fd);
  return strtol(str, &str, 10);
}


char *quickFileRead(int fd) {
  char buf[255];
  memset(buf, 0, 255);

  lseek(fd, 0, SEEK_SET);

  size_t count = read(fd, buf, 254);

  if(count < 0) {
    perror("Could not read any of the file!");
    return NULL;
  }

  char *contents = (char*) malloc(count);
  strcpy(contents, buf);
  return contents;
}
