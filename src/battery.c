/**
 * @file battery.c
 * @author Nick
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

/** Structure to respresent a Battery tray component. */
typedef struct BatteryType {

   TrayComponentType *cp;        /**< Common component data. */

   char *mode;                 /**< The time format to use. */
   char *details;                   /**< The time zone to use (NULL = local). */
   struct ActionNode *actions;   /**< Actions */
   TimeType lastTime;            /**< Currently displayed time. */

   /* The following are used to control popups. */
   int mousex;                /**< Last mouse x-coordinate. */
   int mousey;                /**< Last mouse y-coordinate. */
   TimeType mouseTime;        /**< Time of the last mouse motion. */

   int userWidth;             /**< User-specified Battery width (or 0). */

   struct BatteryType *next;    /**< Next Battery in the list. */

} BatteryType;

/** The default time format to use. */
//static const char *DEFAULT_FORMAT = "%I:%M %p";

static BatteryType *Batterys;

static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void ProcessBatteryButtonPress(TrayComponentType *cp,
                                    int x, int y, int button);
static void ProcessBatteryButtonRelease(TrayComponentType *cp,
                                      int x, int y, int button);
static void ProcessBatteryMotionEvent(TrayComponentType *cp,
                                    int x, int y, int mask);

static void DrawBattery(BatteryType *clk, const TimeType *now);

static void SignalBattery(const struct TimeType *now, int x, int y, Window w,
                        void *data);


/** Initialize Batterys. */
void InitializeBattery(void)
{
   Batterys = NULL;
}

/** Start Battery(s). */
void StartupBattery(void)
{
   BatteryType *clk;
   for(clk = Batterys; clk; clk = clk->next) {
      if(clk->cp->requestedWidth == 0) {
         clk->cp->requestedWidth = 1;
      }
      if(clk->cp->requestedHeight == 0) {
         //clk->cp->requestedHeight = GetStringHeight(FONT_BATTERY) + 4;
      }
   }
}

/** Destroy Battery(s). */
void DestroyBattery(void)
{
   while(Batterys) {
      BatteryType *cp = Batterys->next;

      if(Batterys->mode) {
         Release(Batterys->mode);
      }
      if(Batterys->details) {
         Release(Batterys->details);
      }
      DestroyActions(Batterys->actions);
      UnregisterCallback(SignalBattery, Batterys);

      Release(Batterys);
      Batterys = cp;
   }
}

/** Create a Battery tray component. */
TrayComponentType *CreateBattery(const char *mode, const char *details,
                               int width, int height)
{

   TrayComponentType *cp;
   BatteryType *clk;

   clk = Allocate(sizeof(BatteryType));
   clk->next = Batterys;
   Batterys = clk;

   clk->mousex = -settings.doubleClickDelta;
   clk->mousey = -settings.doubleClickDelta;
   clk->mouseTime.seconds = 0;
   clk->mouseTime.ms = 0;
   clk->userWidth = 0;

   if(!mode) {
      mode = "percent";
   }
   clk->mode = CopyString(mode);
   clk->details = CopyString(details);
   clk->actions = NULL;
   memset(&clk->lastTime, 0, sizeof(clk->lastTime));

   cp = CreateTrayComponent();
   cp->object = clk;
   clk->cp = cp;
   if(width > 0) {
      cp->requestedWidth = width;
      clk->userWidth = 1;
   } else {
      cp->requestedWidth = 0;
      clk->userWidth = 0;
   }
   cp->requestedHeight = height;

   cp->Create = Create;
   cp->Resize = Resize;
   cp->Destroy = Destroy;
   cp->ProcessButtonPress = ProcessBatteryButtonPress;
   cp->ProcessButtonRelease = ProcessBatteryButtonRelease;
   cp->ProcessMotionEvent = ProcessBatteryMotionEvent;

   RegisterCallback(Min(900, settings.popupDelay / 2), SignalBattery, clk);

   return cp;
}

/** Add an action to a Battery. */
void AddBatteryAction(TrayComponentType *cp,
                    const char *action,
                    int mask)
{
   BatteryType *Battery = (BatteryType*)cp->object;
   AddAction(&Battery->actions, action, mask);
}

/** Initialize a Battery tray component. */
void Create(TrayComponentType *cp)
{
   cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
                               rootDepth);
}

/** Resize a Battery tray component. */
void Resize(TrayComponentType *cp)
{

   BatteryType *clk;
   TimeType now;

   Assert(cp);

   clk = (BatteryType*)cp->object;

   Assert(clk);

   if(cp->pixmap != None) {
      JXFreePixmap(display, cp->pixmap);
   }

   cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
                               rootDepth);

   memset(&clk->lastTime, 0, sizeof(clk->lastTime));

   GetCurrentTime(&now);
   DrawBattery(clk, &now);

}

/** Destroy a Battery tray component. */
void Destroy(TrayComponentType *cp)
{
   Assert(cp);
   if(cp->pixmap != None) {
      JXFreePixmap(display, cp->pixmap);
   }
}

/** Process a press event on a Battery tray component. */
void ProcessBatteryButtonPress(TrayComponentType *cp, int x, int y, int button)
{
   const BatteryType *clk = (BatteryType*)cp->object;
   ProcessActionPress(clk->actions, cp, x, y, button);
}

void ProcessBatteryButtonRelease(TrayComponentType *cp, int x, int y, int button)
{
   const BatteryType *clk = (BatteryType*)cp->object;
   ProcessActionRelease(clk->actions, cp, x, y, button);
}

/** Process a motion event on a Battery tray component. */
void ProcessBatteryMotionEvent(TrayComponentType *cp,
                             int x, int y, int mask)
{
   BatteryType *clk = (BatteryType*)cp->object;
   clk->mousex = cp->screenx + x;
   clk->mousey = cp->screeny + y;
   GetCurrentTime(&clk->mouseTime);
}

/** Update a Battery tray component. */
void SignalBattery(const TimeType *now, int x, int y, Window w, void *data)
{

   //BatteryType *cp = (BatteryType*)data;
   //const char *longTime;

   //DrawBattery(cp, now);
//   if(cp->cp->tray->window == w &&
//      abs(cp->mousex - x) < settings.doubleClickDelta &&
//      abs(cp->mousey - y) < settings.doubleClickDelta) {
//      if(GetTimeDifference(now, &cp->mouseTime) >= settings.popupDelay) {
//         //longTime = GetTimeString("%c", cp->zone);
//         //ShowPopup(x, y, "Soonish!", POPUP_BATTERY);
//      }
//   }

}

/** Draw a Battery tray component. */
void DrawBattery(BatteryType *clk, const TimeType *now)
{

//   TrayComponentType *cp;
//   const char *batteryString;
//   int width;
//   int rwidth;
//
//   /* Only draw if the time changed. */
//   if(now->seconds == clk->lastTime.seconds) {
//      return;
//   }
//
//   /* Clear the area. */
//   cp = clk->cp;
//   if(colors[COLOR_BATTERY_BG1] == colors[COLOR_BATTERY_BG2]) {
//      JXSetForeground(display, rootGC, colors[COLOR_BATTERY_BG1]);
//      JXFillRectangle(display, cp->pixmap, rootGC, 0, 0,
//                      cp->width, cp->height);
//   } else {
//      DrawHorizontalGradient(cp->pixmap, rootGC,
//                             colors[COLOR_BATTERY_BG1], colors[COLOR_BATTERY_BG2],
//                             0, 0, cp->width, cp->height);
//   }
//
//   /* Determine if the Battery is the right size. */
//   batteryString = "Soon!";
//   width = GetStringWidth(FONT_BATTERY, batteryString);
//   rwidth = width + 4;
//   if(rwidth == clk->cp->requestedWidth || clk->userWidth) {
//
//      /* Draw the Battery. */
//      RenderString(cp->pixmap, FONT_BATTERY, COLOR_BATTERY_FG,
//                   (cp->width - width) / 2,
//                   (cp->height - GetStringHeight(FONT_BATTERY)) / 2,
//                   cp->width, batteryString);
//
//      UpdateSpecificTray(clk->cp->tray, clk->cp);
//
//   } else {
//
//      /* Wrong size. Resize. */
//      clk->cp->requestedWidth = rwidth;
//      ResizeTray(clk->cp->tray);
//
//   }

}
