/**
 * @file winmenu.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for handling window menus.
 *
 */

#include "jwm.h"
#include "winmenu.h"
#include "client.h"
#include "move.h"
#include "resize.h"
#include "event.h"
#include "cursor.h"
#include "misc.h"
#include "root.h"
#include "settings.h"
#include "DesktopEnvironment.h"

static void CreateWindowLayerMenu(Menu *menu, ClientNode *np);
static void CreateWindowSendToMenu(Menu *menu, ClientNode *np);
static void AddWindowMenuItem(Menu *menu, const char *name, MenuActionType type, ClientNode *np, int value);

/** Show a window menu. */
void ShowWindowMenu(ClientNode *np, int x, int y, char keyboard) {
  Menu *menu = CreateWindowMenu(np);
  Menus::InitializeMenu(menu);
  Menus::ShowMenu(menu, RunWindowCommand, x, y, keyboard);
  Menus::DestroyMenu(menu);
}

/** Create a new window menu. */
Menu *CreateWindowMenu(ClientNode *np) {

  Menu *menu;

  menu = Menus::CreateMenu();

  /* Note that items are added in reverse order of display. */

  if (!(np->getState()->isDialogWindow())) {
    AddWindowMenuItem(menu, _("Close"), MA_CLOSE, np, 0);
    AddWindowMenuItem(menu, _("Kill"), MA_KILL, np, 0);
    AddWindowMenuItem(menu, NULL, MA_NONE, np, 0);
  }

  if (!((np->getState()->isStatus(STAT_FULLSCREEN | STAT_MINIMIZED)) || np->getState()->getMaxFlags())) {
    if (np->getState()->isStatus(STAT_MAPPED | STAT_SHADED)) {
      if (np->getState()->getBorder() & BORDER_RESIZE) {
        AddWindowMenuItem(menu, _("Resize"), MA_RESIZE, np, 0);
      }
      if (np->getState()->getBorder() & BORDER_MOVE) {
        AddWindowMenuItem(menu, _("Move"), MA_MOVE, np, 0);
      }
    }
  }

  if ((np->getState()->getBorder() & BORDER_MIN) && !(np->getState()->isMinimized())) {
    AddWindowMenuItem(menu, _("Minimize"), MA_MINIMIZE, np, 0);
  }

  if (!(np->getState()->isFullscreen())) {
    if (!(np->getState()->isMinimized())) {
      if (np->getState()->isShaded()) {
        AddWindowMenuItem(menu, _("Unshade"), MA_SHADE, np, 0);
      } else if (np->getState()->getBorder() & BORDER_SHADE) {
        AddWindowMenuItem(menu, _("Shade"), MA_SHADE, np, 0);
      }
    }
    if (np->getState()->getBorder() & BORDER_MAX) {
      if ((np->getState()->isMinimized()) || !(np->getState()->getMaxFlags() & MAX_VERT) || (np->getState()->getMaxFlags() & MAX_HORIZ)) {
        AddWindowMenuItem(menu, _("Maximize-y"), MA_MAXIMIZE_V, np, 0);
      }
      if ((np->getState()->isMinimized()) || !(np->getState()->getMaxFlags() & MAX_HORIZ) || (np->getState()->getMaxFlags() & MAX_VERT)) {
        AddWindowMenuItem(menu, _("Maximize-x"), MA_MAXIMIZE_H, np, 0);
      }
      if ((np->getState()->isMinimized()) || !(np->getState()->getMaxFlags() & (MAX_VERT | MAX_HORIZ))) {
        AddWindowMenuItem(menu, _("Maximize"), MA_MAXIMIZE, np, 0);
      }
      if (!(np->getState()->isMinimized())) {
        if ((np->getState()->getMaxFlags() & MAX_HORIZ) && (np->getState()->getMaxFlags() & MAX_VERT)) {
          AddWindowMenuItem(menu, _("Restore"), MA_MAXIMIZE, np, 0);
        } else if (np->getState()->getMaxFlags() & MAX_VERT) {
          AddWindowMenuItem(menu, _("Restore"), MA_MAXIMIZE_V, np, 0);
        } else if (np->getState()->getMaxFlags() & MAX_HORIZ) {
          AddWindowMenuItem(menu, _("Restore"), MA_MAXIMIZE_H, np, 0);
        }
      }
    }
  }

  if (np->getState()->isMinimized()) {
    AddWindowMenuItem(menu, _("Restore"), MA_RESTORE, np, 0);
  }

  if (!(np->getState()->isDialogWindow())) {
    if (settings.desktopCount > 1) {
      if (np->getState()->isSticky()) {
        AddWindowMenuItem(menu, _("Unstick"), MA_STICK, np, 0);
      } else {
        AddWindowMenuItem(menu, _("Stick"), MA_STICK, np, 0);
      }
    }

    CreateWindowLayerMenu(menu, np);

    if (settings.desktopCount > 1) {
      if (!(np->getState()->isSticky())) {
        CreateWindowSendToMenu(menu, np);
      }
    }

  }

  return menu;
}

/** Create a window layer submenu. */
void CreateWindowLayerMenu(Menu *menu, ClientNode *np) {

  Menu *submenu;
  MenuItem *item;

  item = Menus::CreateMenuItem(MENU_ITEM_SUBMENU);
  item->name = CopyString(_("Layer"));
  item->action.type = MA_NONE;
  item->next = menu->items;
  menu->items = item;

  submenu = Menus::CreateMenu();
  item->submenu = submenu;

  if (np->getState()->getLayer() == LAYER_ABOVE) {
    AddWindowMenuItem(submenu, _("[Above]"), MA_LAYER, np, LAYER_ABOVE);
  } else {
    AddWindowMenuItem(submenu, _("Above"), MA_LAYER, np, LAYER_ABOVE);
  }
  if (np->getState()->getLayer() == LAYER_NORMAL) {
    AddWindowMenuItem(submenu, _("[Normal]"), MA_LAYER, np, LAYER_NORMAL);
  } else {
    AddWindowMenuItem(submenu, _("Normal"), MA_LAYER, np, LAYER_NORMAL);
  }
  if (np->getState()->getLayer() == LAYER_BELOW) {
    AddWindowMenuItem(submenu, _("[Below]"), MA_LAYER, np, LAYER_BELOW);
  } else {
    AddWindowMenuItem(submenu, _("Below"), MA_LAYER, np, LAYER_BELOW);
  }

}

/** Create a send to submenu. */
void CreateWindowSendToMenu(Menu *menu, ClientNode *np) {

  unsigned int mask;
  unsigned int x;

  mask = 0;
  for (x = 0; x < settings.desktopCount; x++) {
    if (np->getState()->getDesktop() == x || (np->getState()->isSticky())) {
      mask |= 1 << x;
    }
  }

  AddWindowMenuItem(menu, _("Send To"), MA_NONE, np, 0);

  /* Now the first item in the menu is for the desktop list. */
  menu->items->submenu = environment->CreateDesktopMenu(mask, np);

}

/** Add an item to the current window menu. */
void AddWindowMenuItem(Menu *menu, const char *name, MenuActionType type, ClientNode *np, int value) {

  MenuItem *item;

  item = Menus::CreateMenuItem(name ? MENU_ITEM_NORMAL : MENU_ITEM_SEPARATOR);
  item->name = CopyString(name);
  item->action.type = type;
  item->action.context = np;
  item->action.value = value;
  item->next = menu->items;
  menu->items = item;

}

/** Select a window for performing an action. */
void ChooseWindow(MenuAction *action) {

  XEvent event;
  ClientNode *np;

  if (!Cursors::GrabMouseForChoose()) {
    return;
  }

  for (;;) {

    _WaitForEvent(&event);

    if (event.type == ButtonPress) {
      if (event.xbutton.button == Button1) {
        np = ClientNode::FindClient(event.xbutton.subwindow);
        if (np) {
          action->context = np;
          RunWindowCommand(action, event.xbutton.button);
        }
      }
      break;
    } else if (event.type == KeyPress) {
      break;
    }

  }

  JXUngrabPointer(display, CurrentTime);

}

/** Window menu action callback. */
void RunWindowCommand(MenuAction *action, unsigned button) {
  ClientNode *client = (ClientNode*) action->context;
  switch (action->type) {
  case MA_STICK:
    if (client->getState()->isSticky()) {
      client->SetClientSticky(0);
    } else {
      client->SetClientSticky(1);
    }
    break;
  case MA_MAXIMIZE:
    if ((client->getState()->getMaxFlags() & MAX_HORIZ) && (client->getState()->getMaxFlags() & MAX_VERT)
        && !(client->getState()->isMinimized())) {
      client->MaximizeClient(MAX_NONE);
    } else {
      client->MaximizeClient( MAX_VERT | MAX_HORIZ);
    }
    break;
  case MA_MAXIMIZE_H:
    if ((client->getState()->getMaxFlags() & MAX_HORIZ) && !(client->getState()->getMaxFlags() & MAX_VERT)
        && !(client->getState()->isMinimized())) {
      client->MaximizeClient( MAX_NONE);
    } else {
      client->MaximizeClient( MAX_HORIZ);
    }
    break;
  case MA_MAXIMIZE_V:
    if ((client->getState()->getMaxFlags() & MAX_VERT) && !(client->getState()->getMaxFlags() & MAX_HORIZ)
        && !(client->getState()->isMinimized())) {
      client->MaximizeClient( MAX_NONE);
    } else {
      client->MaximizeClient(MAX_VERT);
    }
    break;
  case MA_MINIMIZE:
    client->MinimizeClient(1);
    break;
  case MA_RESTORE:
    client->RestoreClient(1);
    break;
  case MA_CLOSE:
    client->DeleteClient();
    break;
  case MA_SENDTO:
  case MA_DESKTOP:
    client->SetClientDesktop(action->value);
    break;
  case MA_SHADE:
    if (client->getState()->isShaded()) {
      client->UnshadeClient();
    } else {
      client->ShadeClient();
    }
    break;
  case MA_MOVE:
    client->MoveClientKeyboard();
    break;
  case MA_RESIZE:
    client->ResizeClientKeyboard(MC_BORDER | MC_BORDER_S | MC_BORDER_E);
    break;
  case MA_KILL:
    client->KillClient();
    break;
  case MA_LAYER:
    client->SetClientLayer(action->value);
    break;
  default:
    break;
  }

}
