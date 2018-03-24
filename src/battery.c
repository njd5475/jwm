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

static BatteryType *Batterys;

static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void ProcessBatteryButtonPress(TrayComponentType *cp, int x, int y, int button);
static void ProcessBatteryButtonRelease(TrayComponentType *cp, int x, int y, int button);
static void ProcessBatteryMotionEvent(TrayComponentType *cp, int x, int y, int mask);
static void DrawBattery(BatteryType *clk, const TimeType *now);
static void SignalBattery(const struct TimeType *now, int x, int y, Window w,
    void *data);

/** Initialize Batterys. */
void InitializeBattery(void) {
  Batterys = NULL;
}

/** Start Battery(s). */
void StartupBattery(void) {

}

/** Destroy Battery(s). */
void DestroyBattery(void) {

}

/** Create a Battery tray component. */
TrayComponentType *CreateBattery(const char *format, const char *zone,
    int width, int height) {
  return 0;
}

/** Add an action to a Battery. */
void AddBatteryAction(TrayComponentType *cp, const char *action, int mask) {
}

/** Initialize a Battery tray component. */
void Create(TrayComponentType *cp) {
}

/** Resize a Battery tray component. */
void Resize(TrayComponentType *cp) {

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
void SignalBattery(const TimeType *now, int x, int y, Window w, void *data) {

}

/** Draw a Battery tray component. */
void DrawBattery(BatteryType *clk, const TimeType *now) {

}
