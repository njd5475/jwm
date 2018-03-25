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

  char *format; /**< The time format to use. */
  char *zone; /**< The time zone to use (NULL = local). */
  struct ActionNode *actions; /**< Actions */
  TimeType lastTime; /**< Currently displayed time. */

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
static void DrawBattery(BatteryType *bat, int level);
static void PollBattery(const struct TimeType *now, int x, int y, Window w, void *data);

/** Initialize Batterys. */
void InitializeBattery(void) {
  Warning(_("Battery tray has been initialized"));
  batteries = NULL;
}

/** Start Battery(s). */
void StartupBattery(void) {

}

/** Destroy Battery(s). */
void DestroyBattery(void) {

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

  DrawBattery(bat, 0);
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

/** Update a Battery tray component. */
void PollBattery(const TimeType *now, int x, int y, Window w, void *data) {
  DrawBattery(data, 0);
}

/** Draw a Battery tray component. */
void DrawBattery(BatteryType *bat, int level) {
  JXSetForeground(display, rootGC, colors[COLOR_CLOCK_BG1]);
  JXFillRectangle(display, bat->cp->pixmap, rootGC, 0, 0, bat->cp->width, bat->cp->height);

  static char buf[80];
  sprintf(buf, "%d%%", level);
  int strWidth = GetStringWidth(FONT_CLOCK, buf); 
  RenderString(bat->cp->pixmap, FONT_CLOCK, COLOR_CLOCK_FG,
    (bat->cp->width - strWidth)/2, (bat->cp->height - GetStringHeight(FONT_CLOCK))/2, bat->cp->width, buf);

  UpdateSpecificTray(bat->cp->tray, bat->cp);
}
