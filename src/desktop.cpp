/**
 * @file desktop.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the desktop management functions.
 *
 */

#include "jwm.h"
#include "desktop.h"
#include "main.h"
#include "client.h"
#include "clientlist.h"
#include "taskbar.h"
#include "error.h"
#include "menu.h"
#include "misc.h"
#include "settings.h"
#include "grab.h"
#include "event.h"
#include "tray.h"
#include "DesktopEnvironment.h"

static char **desktopNames = NULL;
static char *showingDesktop = NULL;

/** Startup desktop support. */
void _StartupDesktops(void)
{

   unsigned int x;

   if(desktopNames == NULL) {
      desktopNames = new char*[settings.desktopCount];
      for(x = 0; x < settings.desktopCount; x++) {
         desktopNames[x] = NULL;
      }
   }
   for(x = 0; x < settings.desktopCount; x++) {
      if(desktopNames[x] == NULL) {
         desktopNames[x] = new char[4];
         snprintf(desktopNames[x], 4, "w-%d", x + 1);
      }
   }
   if(showingDesktop == NULL) {
      showingDesktop = new char[settings.desktopCount];
      for(x = 0; x < settings.desktopCount; x++) {
         showingDesktop[x] = 0;
      }
   }
}

/** Release desktop data. */
void _DestroyDesktops(void)
{

   if(desktopNames) {
      unsigned int x;
      for(x = 0; x < settings.desktopCount; x++) {
         Release(desktopNames[x]);
      }
      Release(desktopNames);
      desktopNames = NULL;
   }
   if(showingDesktop) {
      Release(showingDesktop);
      showingDesktop = NULL;
   }

}

/** Get the right desktop. */
unsigned int _GetRightDesktop(unsigned int desktop)
{
   const int y = desktop / settings.desktopWidth;
   const int x = (desktop + 1) % settings.desktopWidth;
   return y * settings.desktopWidth + x;
}

/** Get the left desktop. */
unsigned int _GetLeftDesktop(unsigned int desktop)
{
   const int y = currentDesktop / settings.desktopWidth;
   int x = currentDesktop % settings.desktopWidth;
   x = x > 0 ? x - 1 : settings.desktopWidth - 1;
   return y * settings.desktopWidth + x;
}

/** Get the above desktop. */
unsigned int _GetAboveDesktop(unsigned int desktop)
{
   if(currentDesktop >= settings.desktopWidth) {
      return currentDesktop - settings.desktopWidth;
   }
   return currentDesktop + (settings.desktopHeight - 1) * settings.desktopWidth;
}

/** Get the below desktop. */
unsigned int _GetBelowDesktop(unsigned int desktop)
{
   return (currentDesktop + settings.desktopWidth) % settings.desktopCount;
}

/** Change to the desktop to the right. */
char _RightDesktop(void)
{
   if(settings.desktopWidth > 1) {
      const unsigned int desktop = _GetRightDesktop(currentDesktop);
      _ChangeDesktop(desktop);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the desktop to the left. */
char _LeftDesktop(void)
{
   if(settings.desktopWidth > 1) {
      const unsigned int desktop = _GetLeftDesktop(currentDesktop);
      _ChangeDesktop(desktop);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the desktop above. */
char _AboveDesktop(void)
{
   if(settings.desktopHeight > 1) {
      const int desktop = _GetAboveDesktop(currentDesktop);
      _ChangeDesktop(desktop);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the desktop below. */
char _BelowDesktop(void)
{
   if(settings.desktopHeight > 1) {
      const unsigned int desktop = _GetBelowDesktop(currentDesktop);
      _ChangeDesktop(desktop);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the specified desktop. */
void _ChangeDesktop(unsigned int desktop)
{

   ClientNode *np;
   unsigned int x;

   if(JUNLIKELY(desktop >= settings.desktopCount)) {
      return;
   }

   if(currentDesktop == desktop) {
      return;
   }

   /* Hide clients from the old desktop.
    * Note that we show clients in a separate loop to prevent an issue
    * with clients losing focus.
    */
   for(x = 0; x < LAYER_COUNT; x++) {
      for(np = nodes[x]; np; np = np->getNext()) {
         if(np->getState()->getStatus() & STAT_STICKY) {
            continue;
         }
         if(np->getState()->getDesktop() == currentDesktop) {
            np->HideClient();
         }
      }
   }

   /* Show clients on the new desktop. */
   for(x = 0; x < LAYER_COUNT; x++) {
      for(np = nodes[x]; np; np = np->getNext()) {
         if(np->getState()->getStatus() & STAT_STICKY) {
            continue;
         }
         if(np->getState()->getDesktop() == desktop) {
            np->ShowClient();
         }
      }
   }

   currentDesktop = desktop;

   Hints::SetCardinalAtom(rootWindow, ATOM_NET_CURRENT_DESKTOP, currentDesktop);
   Hints::SetCardinalAtom(rootWindow, ATOM_NET_SHOWING_DESKTOP,
                   showingDesktop[currentDesktop]);

   _RequireRestack();
   _RequireTaskUpdate();

   DesktopEnvironment::DefaultEnvironment()->LoadBackground(desktop);

}

/** Create a desktop menu. */
Menu *_CreateDesktopMenu(unsigned int mask, void *context)
{

   Menu *menu;
   int x;

   menu = Menus::CreateMenu();
   for(x = settings.desktopCount - 1; x >= 0; x--) {
      const size_t len = strlen(desktopNames[x]);
      MenuItem *item = Menus::CreateMenuItem(MENU_ITEM_NORMAL);
      item->next = menu->items;
      menu->items = item;

      item->action.type = MA_DESKTOP;
      item->action.context = context;
      item->action.value = x;

      item->name = new char[len + 3];
      item->name[0] = (mask & (1 << x)) ? '[' : ' ';
      memcpy(&item->name[1], desktopNames[x], len);
      item->name[len + 1] = (mask & (1 << x)) ? ']' : ' ';
      item->name[len + 2] = 0;
   }

   return menu;

}

/** Create a sendto menu. */
Menu *_CreateSendtoMenu(MenuActionType mask, void *context)
{

   Menu *menu;
   int x;

   menu = Menus::CreateMenu();
   for(x = settings.desktopCount - 1; x >= 0; x--) {
      const size_t len = strlen(desktopNames[x]);
      MenuItem *item = Menus::CreateMenuItem(MENU_ITEM_NORMAL);
      item->next = menu->items;
      menu->items = item;

      item->action.type = MA_SENDTO | mask;
      item->action.context = context;
      item->action.value = x;

      item->name = new char[len + 3];
      item->name[0] = (x == currentDesktop) ? '[' : ' ';
      memcpy(&item->name[1], desktopNames[x], len);
      item->name[len + 1] = (x == currentDesktop) ? ']' : ' ';
      item->name[len + 2] = 0;
   }

   return menu;
}

/** Toggle the "show desktop" state. */
void _ShowDesktop(void)
{

   ClientNode *np;
   int layer;

   Grabs::GrabServer();
   for(layer = 0; layer < LAYER_COUNT; layer++) {
      for(np = nodes[layer]; np; np = np->getNext()) {
         if(np->getState()->getStatus() & STAT_NOLIST) {
            continue;
         }
         if((np->getState()->getDesktop() == currentDesktop) ||
            (np->getState()->getStatus() & STAT_STICKY)) {
            if(showingDesktop[currentDesktop]) {
               if(np->getState()->getStatus() & STAT_SDESKTOP) {
                  np->RestoreClient(0);
               }
            } else {
               if(np->getState()->getStatus() & STAT_ACTIVE) {
                  JXSetInputFocus(display, rootWindow, RevertToParent,
                                  CurrentTime);
               }
               if(np->getState()->getStatus() & (STAT_MAPPED | STAT_SHADED)) {
                  np->MinimizeClient(0);
                  np->setSDesktopStatus();
               }
            }
         }
      }
   }
   _RequireRestack();
   _RequireTaskUpdate();
   JXSync(display, True);

   if(showingDesktop[currentDesktop]) {
      char first = 1;
      JXSync(display, False);
      for(layer = 0; layer < LAYER_COUNT; layer++) {
         for(np = nodes[layer]; np; np = np->getNext()) {
            if(np->getState()->getStatus() & STAT_NOLIST) {
               continue;
            }
            if((np->getState()->getDesktop() == currentDesktop) ||
               (np->getState()->getStatus() & STAT_STICKY)) {
               if(first) {
                  np->FocusClient();
                  first = 0;
               }
               Border::DrawBorder(np);
            }
         }
      }
      showingDesktop[currentDesktop] = 0;
   } else {
      showingDesktop[currentDesktop] = 1;
   }
   Hints::SetCardinalAtom(rootWindow, ATOM_NET_SHOWING_DESKTOP,
                   showingDesktop[currentDesktop]);
   Grabs::UngrabServer();
   TrayType::DrawTray();

}

/** Set the name for a desktop. */
void _SetDesktopName(unsigned int desktop, const char *str)
{


   if(JUNLIKELY(!str)) {
      Warning(_("empty Desktops Name tag"));
      return;
   }

   Assert(desktop < settings.desktopWidth * settings.desktopHeight);

   if(!desktopNames) {
      unsigned int x;
      desktopNames = new char*[settings.desktopCount];
      for(x = 0; x < settings.desktopCount; x++) {
         desktopNames[x] = NULL;
      }
   }

   Assert(desktopNames[desktop] == NULL);

   desktopNames[desktop] = CopyString(str);

}

/** Get the name of a desktop. */
const char *_GetDesktopName(unsigned int desktop)
{
   Assert(desktop < settings.desktopCount);
   if(desktopNames && desktopNames[desktop]) {
      return desktopNames[desktop];
   } else {
      return "";
   }
}
