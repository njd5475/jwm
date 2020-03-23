/*
 * Configuration.cpp
 *
 *  Created on: Feb 27, 2020
 *      Author: nick
 */

#include <string>
#include <sstream>
#include <stdlib.h>
#include "logger.h"
#include "settings.h"
#include "font.h"
#include "Configuration.h"
#include "menu.h"
#include "root.h"
#include "icon.h"

using namespace std;

vector<string> split(const char *str) {
  stringstream strstream(str);
  string segment;
  vector<string> seglist;

  while (getline(strstream, segment, '_')) {
    seglist.push_back(segment);
  }
  return seglist;
}

vector<Configuration*> Configuration::configurations;

Configuration::Configuration(JArray *configurations) :
    _configs(configurations), _loaded(false) {
  // TODO Auto-generated constructor stub

}

Configuration::~Configuration() {
  // TODO Auto-generated destructor stub
}

void Configuration::loadConfig() {
  JArrayItem **arrayItems = _configs->_internal.vItems;
  for (int i = 0; i < _configs->count; ++i) {
    JArrayItem *item = arrayItems[i];
    if (item->type == VAL_OBJ) {
      JObject *itemObj = item->value.object_val;

      JObject *iconPath = jsonObject(itemObj, "IconPath");
      if(iconPath) {
        const char *path = jsonString(iconPath, "path");
        Icon::AddIconPath(path);
      }

      JObject *menuStyle = jsonObject(itemObj, "MenuStyle");
      if (menuStyle) {
        Configuration::loadStyle(menuStyle, (StyleColors ) { FONT_MENU,
            COLOR_MENU_BG, COLOR_MENU_BG,
            COLOR_MENU_FG,
            COLOR_MENU_UP, COLOR_MENU_DOWN, MENU, {
            COLOR_MENU_ACTIVE_BG1, COLOR_MENU_ACTIVE_BG2,
            COLOR_MENU_ACTIVE_FG, COLOR_MENU_ACTIVE_UP,
            COLOR_MENU_ACTIVE_DOWN } });
      }

      JObject *windowStyle = jsonObject(itemObj, "WindowStyle");
      if (windowStyle) {
        Configuration::loadStyle(windowStyle, (StyleColors ) { FONT_BORDER,
            COLOR_TITLE_BG1,
            COLOR_TITLE_BG2, COLOR_TITLE_FG, COLOR_TITLE_UP,
            COLOR_TITLE_DOWN, ACTIVE_CLIENT, { COLOR_TITLE_ACTIVE_BG1,
            COLOR_TITLE_ACTIVE_BG2, COLOR_TITLE_ACTIVE_FG,
            COLOR_TITLE_ACTIVE_UP, COLOR_TITLE_ACTIVE_DOWN } });
        Configuration::loadWindowSettings(windowStyle);
      }

      JObject *trayStyle = jsonObject(itemObj, "TrayStyle");
      if (trayStyle) {
        Configuration::loadStyle(trayStyle, (StyleColors ) { FONT_TRAY,
            COLOR_TRAY_BG1, COLOR_TRAY_BG2,
            COLOR_TRAY_FG, COLOR_TRAY_UP, COLOR_TRAY_DOWN, TRAY, {
            COLOR_TRAY_ACTIVE_BG1, COLOR_TRAY_ACTIVE_BG2,
            COLOR_TRAY_ACTIVE_FG, COLOR_TRAY_ACTIVE_UP,
            COLOR_TRAY_ACTIVE_DOWN } });
      }

      JObject *taskListStyle = jsonObject(itemObj, "TaskListStyle");
      if (taskListStyle) {
        Configuration::loadStyle(taskListStyle, (StyleColors ) { FONT_TASKLIST,
            COLOR_TASKLIST_BG1, COLOR_TASKLIST_BG2,
            COLOR_TASKLIST_FG, COLOR_TASKLIST_UP, COLOR_TASKLIST_DOWN, TASKLIST,
                {
                COLOR_TASKLIST_ACTIVE_BG1, COLOR_TASKLIST_ACTIVE_BG2,
                COLOR_TASKLIST_ACTIVE_FG, COLOR_TASKLIST_ACTIVE_UP,
                COLOR_TASKLIST_ACTIVE_DOWN } });
      }

      JObject *pagerStyle = jsonObject(itemObj, "PagerStyle");
      if (pagerStyle) {
        Configuration::loadStyle(pagerStyle, (StyleColors ) { FONT_PAGER,
            COLOR_PAGER_BG, COLOR_PAGER_BG,
            COLOR_PAGER_FG, COLOR_PAGER_BG, COLOR_PAGER_BG, PAGER, {
            COLOR_PAGER_ACTIVE_BG, COLOR_PAGER_ACTIVE_BG,
            COLOR_PAGER_ACTIVE_FG, COLOR_PAGER_ACTIVE_BG,
            COLOR_PAGER_ACTIVE_BG } });
      }

      JObject *popupStyle = jsonObject(itemObj, "PopupStyle");
      if (popupStyle) {
        Configuration::loadStyle(popupStyle, (StyleColors ) { FONT_POPUP,
            COLOR_POPUP_BG, COLOR_POPUP_BG,
            COLOR_POPUP_FG, COLOR_POPUP_BG, COLOR_POPUP_BG, POPUP, {
            COLOR_COUNT, COLOR_COUNT,
            COLOR_COUNT, COLOR_COUNT,
            COLOR_COUNT } });
      }

      JObject *trayButtonStyle = jsonObject(itemObj, "TrayButtonStyle");
      if (trayButtonStyle) {
        Configuration::loadStyle(trayButtonStyle, (StyleColors ) {
                FONT_TRAYBUTTON,
                COLOR_TRAYBUTTON_BG1, COLOR_TRAYBUTTON_BG2,
                COLOR_TRAYBUTTON_FG, COLOR_TRAYBUTTON_UP, COLOR_TRAYBUTTON_DOWN,
                TRAYBUTTON, {
                COLOR_TRAYBUTTON_ACTIVE_BG1, COLOR_TRAYBUTTON_ACTIVE_BG2,
                COLOR_TRAYBUTTON_ACTIVE_FG, COLOR_TRAYBUTTON_ACTIVE_UP,
                COLOR_TRAYBUTTON_ACTIVE_DOWN } });
      }

      JObject *rootMenu = jsonObject(itemObj, "RootMenu");
      if (rootMenu) {
        Menu *menu = Configuration::loadMenu(rootMenu);
        if (!menu) {

        }

        const char *onroot = jsonString(rootMenu, "onroot");
        if (!onroot) {
          onroot = "123";
        }

        Roots::SetRootMenu(onroot, menu);
      }
    }
  }
  _loaded = true;
}

Menu* Configuration::loadMenu(JObject *menuObject, Menu *menu = NULL) {
  JArray *children = jsonArray(menuObject, "children");

  if (menu == NULL) {
    menu = Menus::CreateMenu();
  }
  if (children) {
    for (int i = 0; i < children->count; ++i) {
      JArrayItem *val = children->_internal.vItems[i];
      if (val->type == VAL_OBJ) {
        JObject *item = val->value.object_val;

        JObject *program = jsonObject(item, "Program");
        if (program) {
          loadMenuItem(menu, MENU_ITEM_NORMAL, program);
        }

        JObject *submenu = jsonObject(item, "Menu");
        if (submenu) {
          MenuItem *theItem = loadMenuItem(menu, MENU_ITEM_SUBMENU, submenu);
          Menu *subMenu = theItem->CreateSubMenu();
          loadMenu(submenu, subMenu);
        }

        JObject *separator = jsonObject(item, "Separator");
        if (separator) {
          menu->addSeparator();
        }

        JObject *restart = jsonObject(item, "Restart");
        if (restart) {
          MenuItem *restartItem = new MenuItem(menu, "Restart", MENU_ITEM_NORMAL,
              NULL, NULL);
          restartItem->setAction(MA_RESTART, NULL, 1);
          menu->addItem(restartItem);
        }

        JObject *exit = jsonObject(item, "Exit");
        if (exit) {
          MenuItem *exitItem = new MenuItem(menu, "Exit", MENU_ITEM_NORMAL,
              NULL, NULL);
          exitItem->setAction(MA_EXIT, NULL, 1);
          menu->addItem(exitItem);
        }
      }
    }
  }
  return menu;
}

MenuItem* Configuration::loadMenuItem(Menu *menu, MenuItemType type,
    JObject *item) {

  const char *icon = jsonString(item, "icon");
  const char *label = jsonString(item, "label");
  const char *tooltip = jsonString(item, "tooltip");
  const char *exec = jsonString(item, "exec");

  MenuItem *menuItem = Menus::CreateMenuItem(menu, type, label, tooltip, icon);

  if (exec) {
    menuItem->setActionStr(MA_EXECUTE, exec);
  }

  return menuItem;
}

void Configuration::loadWindowSettings(JObject *window) {
  const char *align = jsonString(window, "align");
  const char *borderWidth = jsonString(window, "width");
  const char *titleHeight = jsonString(window, "height");
  const char *corner = jsonString(window, "corner");
  const char *inactiveClientOpacity = jsonString(window, "opacity");

  if (borderWidth) {
    settings.borderWidth = atoi(borderWidth);
  }

  if (titleHeight) {
    settings.titleHeight = atoi(titleHeight);
  }

  if (corner) {
    settings.cornerRadius = atoi(corner);
  }

  settings.titleTextAlignment = ALIGN_LEFT;
  if (align) {
    if (strcmp(align, "right") == 0) {
      settings.titleTextAlignment = ALIGN_RIGHT;
    } else if (!strcmp(align, "center") == 0) {
      settings.titleTextAlignment = ALIGN_CENTER;
    } else if (!strcmp(align, "left") == 0) {
      settings.titleTextAlignment = ALIGN_LEFT;
    }
  }
}

void Configuration::loadStyle(JObject *object, StyleColors colors) {
  loadStyle(object, colors.font, colors.bg, colors.bg2, colors.fg, colors.up,
      colors.down, colors.opacityType);

  JArray *children = jsonArray(object, "children");

  if (children) {
    unsigned size = -1;
    JObject **actives = jsonArrayKeyFilter(children, "active", &size);
    JObject *active = NULL;
    for (unsigned i = 0; i < size; ++i) {
      active = jsonObject(actives[i], "active");
      loadStyle(active, FONT_NOOP, colors.active.bg, colors.active.bg2,
          colors.active.fg, colors.active.up, colors.active.down, UNKNOWN);
    }
  }
}

void Configuration::loadStyle(JObject *object, FontType f, ColorName bg,
    ColorName bg2, ColorName fg, ColorName up, ColorName down,
    OpacityType otype) {

  const char *font = jsonString(object, "font");
  const char *foreground = jsonString(object, "foreground");
  const char *background = jsonString(object, "background");
  const char *opacityString = jsonString(object, "opacity");
  const char *outline = jsonString(object, "outline");
  const char *decorations = jsonString(object, "decorations");

  if (font) {
    Fonts::SetFont(FONT_MENU, font);
  }

  if (foreground) {
    Colors::SetColor(fg, foreground);
  }

  if (background) {
    vector<string> gradients = split(background);
    Colors::SetColor(bg, gradients[0].c_str());
    unsigned i = gradients.size() > 1 ? 1 : 0;
    Colors::SetColor(bg2, gradients[i].c_str());
  }

  if (outline) {
    vector<string> gradients = split(outline);
    Colors::SetColor(up, gradients[0].c_str());
    unsigned i = gradients.size() > 1 ? 1 : 0;
    Colors::SetColor(down, gradients[i].c_str());
  }

  if (opacityString) {

    float opacity = atof(opacityString);

    if (otype == TRAY) {
      settings.trayOpacity = opacity;
    } else if (otype == MENU) {
      settings.menuOpacity = opacity;
    } else if (otype == ACTIVE_CLIENT) {
      settings.activeClientOpacity = opacity;
    } else if (otype == INACTIVE_CLIENT) {
      settings.inactiveClientOpacity = opacity;
    }
  }

  if (decorations) {
    DecorationsType dType = DECO_UNSET;
    if (strcmp(decorations, "motif") == 0) {
      dType = DECO_MOTIF;
    } else if (strcmp(decorations, "flat") == 0) {
      dType = DECO_FLAT;
    }

    if (otype == TRAY) {
      settings.trayDecorations = dType;
    } else if (otype == MENU) {
      settings.menuDecorations = dType;
    } else if (otype == ACTIVE_CLIENT || otype == INACTIVE_CLIENT) {
      settings.windowDecorations = dType;
    } else if (otype == TASKLIST) {
      settings.taskListDecorations = dType;
    }
  }
}

bool Configuration::isConfigLoaded() {
  return _loaded;
}

void Configuration::load(JArray *configs) {
  Configuration *cfg = new Configuration(configs);
  configurations.push_back(cfg);
  cfg->loadConfig();
}
