/*
 * Configuration.cpp
 *
 *  Created on: Feb 27, 2020
 *      Author: nick
 */

#include "Configuration.h"

#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <string.h>

#include "icon.h"
#include "root.h"
#include "settings.h"
#include "DesktopEnvironment.h"
#include "tray.h"
#include "traybutton.h"
#include "taskbar.h"
#include "pager.h"
#include "battery.h"
#include "dock.h"
#include "binding.h"
#include "clock.h"

using namespace std;

Builder::Builder(Configuration *cfg) :
    _cfg(cfg) {

}

Builder::~Builder() {

}

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

void Configuration::saveConfig(FILE *output) {
  Saver saver(this, output);
  processConfigs(saver);
}

void Configuration::loadConfig() {
  Loader builder(this);
  this->processConfigs(builder);
  _loaded = true;
}

void Configuration::processConfigs(Builder &builder) {
  JArrayItem **arrayItems = _configs->_internal.vItems;
  for (int i = 0; i < _configs->count; ++i) {
    JArrayItem *item = arrayItems[i];
    if (item->type == VAL_OBJ) {
      JObject *itemObj = item->value.object_val;

      JObject *iconPath = jsonObject(itemObj, "IconPath");
      if (iconPath) {
        builder.buildIconPath(iconPath);
      }

      JObject *menuStyle = jsonObject(itemObj, "MenuStyle");
      if (menuStyle) {
        builder.buildMenuStyle(menuStyle);
      }

      JObject *windowStyle = jsonObject(itemObj, "WindowStyle");
      if (windowStyle) {
        builder.buildWindowStyle(windowStyle);
      }

      JObject *trayStyle = jsonObject(itemObj, "TrayStyle");
      if (trayStyle) {
        builder.buildTrayStyle(trayStyle);
      }

      JObject *taskListStyle = jsonObject(itemObj, "TaskListStyle");
      if (taskListStyle) {
        builder.buildTaskListStyle(taskListStyle);
      }

      JObject *pagerStyle = jsonObject(itemObj, "PagerStyle");
      if (pagerStyle) {
        builder.buildPagerStyle(pagerStyle);
      }

      JObject *popupStyle = jsonObject(itemObj, "PopupStyle");
      if (popupStyle) {
        builder.buildPopupStyle(popupStyle);
      }

      JObject *clockStyle = jsonObject(itemObj, "ClockStyle");
      if (clockStyle) {
        builder.buildClockStyle(clockStyle);
      }

      JObject *trayButtonStyle = jsonObject(itemObj, "TrayButtonStyle");
      if (trayButtonStyle) {
        builder.buildTrayButtonStyle(trayButtonStyle);
      }

      JObject *group = jsonObject(itemObj, "Group");
      if (group) {
        builder.buildGroup(group);
      }

      JObject *tray = jsonObject(itemObj, "Tray");
      if (tray) {
        builder.buildTray(tray);
      }

      JObject *desktops = jsonObject(itemObj, "Desktops");
      if (desktops) {
        builder.buildDesktops(desktops);
      }

      JObject *rootMenu = jsonObject(itemObj, "RootMenu");
      if (rootMenu) {
        builder.buildRootMenu(rootMenu);
      }

      JObject *key = jsonObject(itemObj, "Key");
      if (key) {
        builder.buildKey(key);
      }

      JObject *mouse = jsonObject(itemObj, "Mouse");
      if (mouse) {
        builder.buildMouse(mouse);
      }
    }
  }
}

Menu* Configuration::loadMenu(JObject *menuObject, Menu *menu) {
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
          MenuItem *restartItem = new MenuItem(menu, "Restart",
          MENU_ITEM_NORMAL,
          NULL, NULL);
          restartItem->setAction(MA_RESTART, NULL, 1);
          menu->addItem(restartItem);
        }

        JObject *exit = jsonObject(item, "Exit");
        if (exit) {
          MenuItem *exitItem = new MenuItem(menu, "Exit",
          MENU_ITEM_NORMAL,
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

void Configuration::write(FILE *output) {

}

bool Configuration::isConfigLoaded() {
  return _loaded;
}

void Configuration::load(JArray *configs) {
  Configuration *cfg = new Configuration(configs);
  configurations.push_back(cfg);
  cfg->loadConfig();
}

void Configuration::saveConfigs(FILE *output) {
  for (auto config : configurations) {
    config->saveConfig(output);
  }
}

Loader::Loader(Configuration *cfg) :
    Builder(cfg) {

}

Loader::~Loader() {

}

void Loader::buildIconPath(JObject *iconPath) {
  const char *path = jsonString(iconPath, "path");

  Icon::AddIconPath(path);
}

void Loader::buildMenuStyle(JObject *menuStyle) {
  Configuration::loadStyle(menuStyle, (Configuration::StyleColors ) { FONT_MENU,
      COLOR_MENU_BG, COLOR_MENU_BG,
      COLOR_MENU_FG,
      COLOR_MENU_UP, COLOR_MENU_DOWN, Configuration::MENU, {
      COLOR_MENU_ACTIVE_BG1, COLOR_MENU_ACTIVE_BG2,
      COLOR_MENU_ACTIVE_FG, COLOR_MENU_ACTIVE_UP,
      COLOR_MENU_ACTIVE_DOWN } });
}

void Loader::buildTaskListStyle(JObject *taskListStyle) {
  Configuration::loadStyle(taskListStyle, (Configuration::StyleColors ) {
          FONT_TASKLIST,
          COLOR_TASKLIST_BG1, COLOR_TASKLIST_BG2,
          COLOR_TASKLIST_FG, COLOR_TASKLIST_UP, COLOR_TASKLIST_DOWN,
          Configuration::TASKLIST, {
          COLOR_TASKLIST_ACTIVE_BG1, COLOR_TASKLIST_ACTIVE_BG2,
          COLOR_TASKLIST_ACTIVE_FG, COLOR_TASKLIST_ACTIVE_UP,
          COLOR_TASKLIST_ACTIVE_DOWN } });
}

void Loader::buildPagerStyle(JObject *pagerStyle) {
  Configuration::loadStyle(pagerStyle, (Configuration::StyleColors ) {
          FONT_PAGER,
          COLOR_PAGER_BG, COLOR_PAGER_BG,
          COLOR_PAGER_FG, COLOR_PAGER_BG, COLOR_PAGER_BG, Configuration::PAGER,
          {
          COLOR_PAGER_ACTIVE_BG, COLOR_PAGER_ACTIVE_BG,
          COLOR_PAGER_ACTIVE_FG, COLOR_PAGER_ACTIVE_BG,
          COLOR_PAGER_ACTIVE_BG } });
}

void Loader::buildPopupStyle(JObject *popupStyle) {
  Configuration::loadStyle(popupStyle, (Configuration::StyleColors ) {
          FONT_POPUP,
          COLOR_POPUP_BG, COLOR_POPUP_BG,
          COLOR_POPUP_FG, COLOR_POPUP_BG, COLOR_POPUP_BG, Configuration::POPUP,
          {
          COLOR_COUNT, COLOR_COUNT,
          COLOR_COUNT, COLOR_COUNT,
          COLOR_COUNT } });
}

void Loader::buildTrayStyle(JObject *trayStyle) {
  Configuration::loadStyle(trayStyle, (Configuration::StyleColors ) { FONT_TRAY,
      COLOR_TRAY_BG1, COLOR_TRAY_BG2,
      COLOR_TRAY_FG, COLOR_TRAY_UP, COLOR_TRAY_DOWN, Configuration::TRAY, {
      COLOR_TRAY_ACTIVE_BG1, COLOR_TRAY_ACTIVE_BG2,
      COLOR_TRAY_ACTIVE_FG, COLOR_TRAY_ACTIVE_UP,
      COLOR_TRAY_ACTIVE_DOWN } });
}

void Loader::buildTrayButtonStyle(JObject *trayButtonStyle) {
  Configuration::loadStyle(trayButtonStyle, (Configuration::StyleColors ) {
          FONT_TRAYBUTTON,
          COLOR_TRAYBUTTON_BG1, COLOR_TRAYBUTTON_BG2,
          COLOR_TRAYBUTTON_FG, COLOR_TRAYBUTTON_UP,
          COLOR_TRAYBUTTON_DOWN, Configuration::TRAYBUTTON, {
          COLOR_TRAYBUTTON_ACTIVE_BG1, COLOR_TRAYBUTTON_ACTIVE_BG2,
          COLOR_TRAYBUTTON_ACTIVE_FG, COLOR_TRAYBUTTON_ACTIVE_UP,
          COLOR_TRAYBUTTON_ACTIVE_DOWN } });
}

void Loader::buildClockStyle(JObject *clockStyle) {
  Configuration::loadStyle(clockStyle, (Configuration::StyleColors ) {
          FONT_CLOCK,
          COLOR_CLOCK_BG1, COLOR_CLOCK_BG2,
          COLOR_CLOCK_FG, COLOR_CLOCK_BG1, COLOR_CLOCK_BG2,
          Configuration::TRAYBUTTON, {
          COLOR_CLOCK_BG1, COLOR_CLOCK_BG2,
          COLOR_CLOCK_FG, COLOR_CLOCK_BG1,
          COLOR_CLOCK_BG1 } });
}

void Loader::buildRootMenu(JObject *rootMenu) {
  Menu *menu = Configuration::loadMenu(rootMenu);
  if (!menu) {

  }

  const char *onroot = jsonString(rootMenu, "onroot");
  if (!onroot) {
    onroot = "123";
  }

  Roots::SetRootMenu(onroot, menu);
}

void Loader::buildWindowStyle(JObject *windowStyle) {
  Configuration::loadStyle(windowStyle, (Configuration::StyleColors ) {
          FONT_BORDER,
          COLOR_TITLE_BG1,
          COLOR_TITLE_BG2, COLOR_TITLE_FG, COLOR_TITLE_UP,
          COLOR_TITLE_DOWN, Configuration::ACTIVE_CLIENT, {
          COLOR_TITLE_ACTIVE_BG1,
          COLOR_TITLE_ACTIVE_BG2, COLOR_TITLE_ACTIVE_FG,
          COLOR_TITLE_ACTIVE_UP, COLOR_TITLE_ACTIVE_DOWN } });
  Configuration::loadWindowSettings(windowStyle);
}

void Loader::buildTray(JObject *jobj) {
  const char *strX = jsonString(jobj, "x");
  const char *strY = jsonString(jobj, "y");
  const char *strWidth = jsonString(jobj, "width");
  const char *strHeight = jsonString(jobj, "height");
  const char *strAutohide = jsonString(jobj, "autohide");
  const char *strDelay = jsonString(jobj, "delay");
  bool autohide = false;
  float delay = 0.5f;

  Tray *tray = Tray::Create();

  if (!strX) {
    strX = "0";
  }

  if (!strY) {
    strY = "0";
  }

  if (strAutohide) {
    autohide = !(strcmp(strAutohide, "off") == 0);
  }

  if (strDelay) {
    delay = atof(strDelay);
  }

  if (strWidth) {
    tray->SetTrayWidth(strWidth);
  }

  if (strHeight) {
    tray->SetTrayHeight(strHeight);
  }
  tray->SetAutoHideTray(autohide ? THIDE_ON : THIDE_OFF, delay);
  tray->SetTrayX(strX);
  tray->SetTrayY(strY);

  // process children after default properties
  JArray *children = jsonArray(jobj, "children");
  if (children) {
    for (int i = 0; i < children->count; ++i) {
      JArrayItem *val = children->_internal.vItems[i];
      if (val->type == VAL_OBJ) {
        JObject *item = val->value.object_val;

        JObject *trayButton = jsonObject(item, "TrayButton");
        if (trayButton) {
          buildTrayButton(tray, trayButton);
        }

        JObject *pager = jsonObject(item, "Pager");
        if (pager) {
          buildPager(tray, pager);
        }

        JObject *taskList = jsonObject(item, "TaskList");
        if (taskList) {
          buildTaskList(tray, taskList);
        }

        JObject *battery = jsonObject(item, "Battery");
        if (battery) {
          buildBattery(tray, battery);
        }

        JObject *dock = jsonObject(item, "Dock");
        if (dock) {
          buildDock(tray, dock);
        }

        JObject *clock = jsonObject(item, "Clock");
        if (clock) {
          buildClock(tray, clock);
        }
      }
    }
  }

}

void Loader::buildTrayButton(Tray *tray, JObject *trayButton) {
  const char *label = jsonString(trayButton, "label");
  const char *icon = jsonString(trayButton, "icon");
  const char *popup = jsonString(trayButton, "popup");
  const char *strWidth = jsonString(trayButton, "width");
  const char *strHeight = jsonString(trayButton, "height");
  const char *action = jsonString(trayButton, "action");

  unsigned width = 0, height = 0;

  if (strWidth) {
    width = atoi(strWidth);
  }

  if (strHeight) {
    height = atoi(strHeight);
  }

  TrayButton *button = TrayButton::Create(icon, label, popup, width, height,
      tray, tray->getLastComponent());

  if (action) {
    const int default_mask = (1 << 1) | (1 << 2) | (1 << 3); // button mask default is any clicked
    button->addAction(action, default_mask);
  }

  tray->AddTrayComponent(button);
}

void Loader::buildPager(Tray *tray, JObject *pager) {
  const char *strLabeled = jsonString(pager, "labeled");
  char labeled = 0;
  if (strLabeled) {
    labeled = !strcmp(strLabeled, "false");
  }

  PagerType *pagerType = PagerType::CreatePager(labeled ? 1 : 0, tray,
      tray->getLastComponent());

  tray->AddTrayComponent(pagerType);
}

void Loader::buildTaskList(Tray *tray, JObject *taskList) {
  const char *strMaxWidth = jsonString(taskList, "maxwidth");
  unsigned maxWidth = 0;

  if (strMaxWidth) {
    maxWidth = atoi(strMaxWidth);
  }
  TaskBar *taskBar = TaskBar::Create(tray, tray->getLastComponent());
  taskBar->SetMaxTaskBarItemWidth(maxWidth);

  tray->AddTrayComponent(taskBar);
}

void Loader::buildBattery(Tray *tray, JObject *battery) {
  tray->AddTrayComponent(new Battery(0, 0, tray, tray->getLastComponent()));
}

void Loader::buildDock(Tray *tray, JObject *dock) {
  DockType *dockType = DockType::Create(0, tray, tray->getLastComponent());
  tray->AddTrayComponent(dockType);
}

void Loader::buildClock(Tray *tray, JObject *clock) {
  const char *format = jsonString(clock, "format");
  const char *zone = jsonString(clock, "zone");

  ClockType *clockType = ClockType::CreateClock(format, zone ? zone : "UTC", 0,
      9, tray, tray->getLastComponent());
  tray->AddTrayComponent(clockType);
}

void Loader::buildGroup(JObject *jobj) {

}

void Loader::buildDesktops(JObject *jobj) {
  const char *strWidth = jsonString(jobj, "width");
  const char *strHeight = jsonString(jobj, "height");
  char *strBackground = jsonString(jobj, "background");

  unsigned width = 4, height = 1;

  if (strWidth) {
    width = atoi(strWidth);
  }

  if (strHeight) {
    height = atoi(strHeight);
  }

  settings.desktopHeight = height;
  settings.desktopWidth = width;

  if (strBackground) {
    DesktopEnvironment::DefaultEnvironment()->SetBackground(0, NULL,
        strBackground);
  }
}

#define LITERAL(action) #action
void Loader::buildKey(JObject *jobj) {
  const char *key = jsonString(jobj, "key");
  const char *keycode = jsonString(jobj, "keycode");
  const char *action = jsonString(jobj, "action");
  const char *mask = jsonString(jobj, "mask");
  const char *command = NULL;

  if (key && action) {
    ActionType theAction;
    theAction.action = this->convertToAction(action);
    theAction.extra = 0;

    if (JUNLIKELY(theAction.action == INVALID)) {
      vLog("invalid Key action: \"%s\"", action);
    } else {
      Binding::InsertBinding(theAction, mask, key, keycode, command);
    }

  }
}

Actions Loader::convertToAction(const char *action) {
  Actions toRet = INVALID;
  if (strcasecmp(action, LITERAL(LEFT))) {
    toRet = LEFT;
  } else if (strcasecmp(action, LITERAL(RIGHT))) {
    toRet = RIGHT;
  } else if (strcasecmp(action, LITERAL(UP))) {
    toRet = UP;
  } else if (strcasecmp(action, LITERAL(DOWN))) {
    toRet = DOWN;
  } else if (strcasecmp(action, LITERAL(CLOSE))) {
    toRet = CLOSE;
  } else if (strcasecmp(action, LITERAL(NONE))) {
    toRet = NONE;
  } else if (strcasecmp(action, "escape")) {
    toRet = ESC;
  } else if (strcasecmp(action, LITERAL(RESTART))) {
    toRet = RESTART;
  } else if (strcasecmp(action, LITERAL(LDESKTOP))) {
    toRet = LDESKTOP;
  } else if (strcasecmp(action, LITERAL(DESKTOP))) {
    toRet = DESKTOP;
  } else if (strcasecmp(action, LITERAL(EXIT))) {
    toRet = EXIT;
  } else if (strcasecmp(action, LITERAL(FULLSCREEN))) {
    toRet = FULLSCREEN;
  } else if (strcasecmp(action, LITERAL(MAXBOTTOM))) {
    toRet = MAXBOTTOM;
  } else if (strcasecmp(action, LITERAL(MAXH))) {
    toRet = MAXH;
  } else if (strcasecmp(action, LITERAL(MAX))) {
    toRet = MAX;
  } else if (strcasecmp(action, LITERAL(MAXLEFT))) {
    toRet = MAXLEFT;
  } else if (strcasecmp(action, LITERAL(MAXRIGHT))) {
    toRet = MAXRIGHT;
  } else if (strcasecmp(action, LITERAL(DDESKTOP))) {
    toRet = DDESKTOP;
  } else if (strcasecmp(action, LITERAL(SHOWDESK))) {
    toRet = SHOWDESK;
  } else if (strcasecmp(action, LITERAL(SHOWTRAY))) {
    toRet = SHOWTRAY;
  } else if (strcasecmp(action, LITERAL(EXEC))) {
    toRet = EXEC;
  } else if (strcasecmp(action, LITERAL(RESTART))) {
    toRet = RESTART;
  } else if (strcasecmp(action, LITERAL(SEND))) {
    toRet = SEND;
  } else if (strcasecmp(action, LITERAL(SENDR))) {
    toRet = SENDR;
  } else if (strcasecmp(action, LITERAL(SENDL))) {
    toRet = SENDL;
  } else if (strcasecmp(action, LITERAL(SENDU))) {
    toRet = SENDU;
  } else if (strcasecmp(action, LITERAL(SENDD))) {
    toRet = SENDD;
  } else if (strcasecmp(action, LITERAL(MAXTOP))) {
    toRet = MAXTOP;
  } else if (strcasecmp(action, LITERAL(MAXBOTTOM))) {
    toRet = MAXBOTTOM;
  } else if (strcasecmp(action, LITERAL(MAXLEFT))) {
    toRet = MAXLEFT;
  } else if (strcasecmp(action, LITERAL(MAXRIGHT))) {
    toRet = MAXRIGHT;
  } else if (strcasecmp(action, LITERAL(MAXV))) {
    toRet = MAXV;
  } else if (strcasecmp(action, LITERAL(MAXH))) {
    toRet = MAXH;
  } else if (strcasecmp(action, LITERAL(RESTORE))) {
    toRet = RESTORE;
  }
  return toRet;
}

int Loader::convertToMouseContext(const char *context) {
  MouseContextType mc = MC_NONE;
  if (strcasecmp(context, "root")) {
    mc = MC_ROOT;
  } else if (strcasecmp(context, "border")) {
    mc = MC_BORDER;
  } else if (strcasecmp(context, "move") || strcasecmp(context, "title")) {
    mc = MC_MOVE;
  } else if (strcasecmp(context, "close")) {
    mc = MC_CLOSE;
  } else if (strcasecmp(context, "maximize")) {
    mc = MC_MAXIMIZE;
  } else if (strcasecmp(context, "minimize")) {
    mc = MC_MINIMIZE;
  } else if (strcasecmp(context, "icon")) {
    mc = MC_ICON;
  }
  return mc;
}

void Loader::buildMouse(JObject *jobj) {
  const char *context = jsonString(jobj, "context");
  int button = jsonInt(jobj, "button");
  const char *action = jsonString(jobj, "action");
  const char *mask = jsonString(jobj, "mask");
  const char *command = NULL;

  if (button && action) {
    ActionType theAction;
    theAction.action = this->convertToAction(action);
    theAction.extra = 0;
    MouseContextType mc = this->convertToMouseContext(context);

    if (JUNLIKELY(theAction.action == INVALID)) {
      vLog("invalid Key action: \"%s\"", action);
    } else {
      Binding::InsertMouseBinding(button, mask, mc, theAction, command);
    }
  }
}

Saver::Saver(Configuration *cfg, FILE *output) :
    _output(output), Builder(cfg), TABS(4) {

}

Saver::~Saver() {

}

void Saver::buildIconPath(JObject *iconPath) {
  const char *path = jsonString(iconPath, "path");
//TODO: replace spaces with escape characters
  write("IconPath");
  write("path", path);
  write("\n");
}

void Saver::write(const char *str) {
  fprintf(_output, str);
}

void Saver::write(const char *key, const char *value) {
  if (strchr(value, ' ')) {
    fprintf(_output, " %s='%s'", key, value);
  } else {
    fprintf(_output, " %s=%s", key, value);
  }
}

void Saver::buildMenuStyle(JObject *object) {
  buildStyle(0, "MenuStyle", object);
}

void Saver::buildMenus(unsigned indent, const char *elementName,
    JObject *object) {
  this->buildMenuItem(indent, elementName, object);
  JArray *children = jsonArray(object, "children");

  if (children) {
    ++indent;
    for (int i = 0; i < children->count; ++i) {
      JArrayItem *val = children->_internal.vItems[i];
      if (val->type == VAL_OBJ) {
        JObject *item = val->value.object_val;

        JObject *program = jsonObject(item, "Program");
        if (program) {
          buildMenuItem(indent, "Program", program);
        }

        JObject *submenu = jsonObject(item, "Menu");
        if (submenu) {
          buildMenus(indent, "Menu", submenu);
        }

        JObject *separator = jsonObject(item, "Separator");
        if (separator) {
          for (unsigned i = 0; i < indent; ++i) {
            fprintf(_output, "  ");
          }
          buildMenuItem(indent, "Separator", separator);
        }

        JObject *restart = jsonObject(item, "Restart");
        if (restart) {
          buildMenuItem(indent, "Restart", restart);
        }

        JObject *exit = jsonObject(item, "Exit");
        if (exit) {
          buildMenuItem(indent, "Exit", exit);
        }
      }
    }
  }
}

void Saver::writeIndent(unsigned indent) {
  for (unsigned i = 0; i < indent * TABS; ++i) {
    write(" ");
  }
}

void Saver::write(JObject *obj, const char *key) {
  const char* value = jsonString(obj, key);
  if(value) {
    write(key, value);
  }
}

void Saver::buildMenuItem(int indent, const char *type, JObject *itemObj) {
  this->writeIndent(indent);
  const char *icon = jsonString(itemObj, "icon");
  const char *label = jsonString(itemObj, "label");
  const char *tooltip = jsonString(itemObj, "tooltip");
  const char *exec = jsonString(itemObj, "exec");

  fprintf(_output, "%s", type);

  if (icon) {
    write("icon", icon);
  }

  if (label) {
    write("label", label);
  }

  if (tooltip) {
    write("tooltip", tooltip);
  }

  if (exec) {
    write("exec", exec);
  }

  write("\n");
}

void Saver::buildTaskListStyle(JObject *object) {
  this->buildStyle("TaskListStyle", object);
}

void Saver::buildPagerStyle(JObject *object) {
  this->buildStyle("PagerStyle", object);
}

void Saver::buildPopupStyle(JObject *object) {
  this->buildStyle("PopupStyle", object);
}

void Saver::buildTrayStyle(JObject *object) {
  this->buildStyle("TrayStyle", object);
}

void Saver::buildTrayButtonStyle(JObject *object) {
  this->buildStyle("TrayButtonStyle", object);
}

void Saver::buildRootMenu(JObject *object) {
  this->buildMenus(0, "RootMenu", object);
}

void Saver::buildWindowStyle(JObject *object) {
  this->buildStyle("WindowStyle", object);
}

void Saver::buildStyle(const char *styleName, JObject *object) {
  this->buildStyle(0, styleName, object);
}

void Saver::buildClockStyle(JObject *object) {
  this->buildStyle(0, "ClockStyle", object);
}

void Saver::buildStyle(unsigned indent, const char *styleName,
    JObject *object) {
  this->write("\n");
  this->writeIndent(indent);
  fprintf(_output, "%s", styleName);
  const char *font = jsonString(object, "font");
  const char *foreground = jsonString(object, "foreground");
  const char *background = jsonString(object, "background");
  const char *opacityString = jsonString(object, "opacity");
  const char *outline = jsonString(object, "outline");
  const char *decorations = jsonString(object, "decorations");

  if (font) {
    write("font", font);
  }

  if (foreground) {
    write("foreground", foreground);
  }

  if (background) {
    write("background", background);
  }

  if (outline) {
    write("outline", outline);
  }

  if (opacityString) {
    write("opacity", opacityString);
  }

  if (decorations) {
    write("decorations", decorations);
  }

  write("\n");
}

void Saver::buildTray(JObject *jobj) {
  const char *strX = jsonString(jobj, "x");
  const char *strY = jsonString(jobj, "y");
  const char *strWidth = jsonString(jobj, "width");
  const char *strHeight = jsonString(jobj, "height");
  const char *strAutohide = jsonString(jobj, "autohide");
  const char *strDelay = jsonString(jobj, "delay");
  bool autohide = false;
  float delay = 0.5f;

  if (!strX) {
    strX = "0";
  }

  if (!strY) {
    strY = "0";
  }

  if (strAutohide) {
    autohide = !(strcmp(strAutohide, "off") == 0);
  }

  if (strDelay) {
    delay = atof(strDelay);
  }

  this->write("\n");
  this->write("Tray");

  if (strWidth) {
    write("width", strWidth);
  }

  if (strHeight) {
    write("height", strHeight);
  }
  write("autohide", autohide ? "on" : "off");
  write("x", strX);
  write("y", strY);

  // process children after default properties
  JArray *children = jsonArray(jobj, "children");
  if (children) {
    for (int i = 0; i < children->count; ++i) {
      JArrayItem *val = children->_internal.vItems[i];
      if (val->type == VAL_OBJ) {
        JObject *item = val->value.object_val;

        JObject *trayButton = jsonObject(item, "TrayButton");
        if (trayButton) {
          buildTrayButton(1, trayButton);
        }

        JObject *pager = jsonObject(item, "Pager");
        if (pager) {
          buildPager(1, pager);
        }

        JObject *taskList = jsonObject(item, "TaskList");
        if (taskList) {
          buildTaskList(1, taskList);
        }

        JObject *battery = jsonObject(item, "Battery");
        if (battery) {
          buildBattery(1, battery);
        }

        JObject *dock = jsonObject(item, "Dock");
        if (dock) {
          buildDock(1, dock);
        }

        JObject *clock = jsonObject(item, "Clock");
        if (clock) {
          buildClock(1, clock);
        }
      }
    }
  }
}

void Saver::buildGroup(JObject *jobj) {

}

void Saver::buildDesktops(JObject *jobj) {

}

void Saver::buildKey(JObject *jobj) {

}

void Saver::buildMouse(JObject *jobj) {

}

void Saver::buildTrayButton(unsigned indent, JObject *trayButton) {
  write("\n");
  writeIndent(indent);
  write("TrayButton");
  write(trayButton, "icon");
  write(trayButton, "label");
  write(trayButton, "action");
}

void Saver::buildPager(unsigned indent, JObject *pager) {

  write("\n");
  writeIndent(indent);
  write("Pager");
  write(pager, "labeled");
}

void Saver::buildTaskList(unsigned indent, JObject* taskList) {
  write("\n");
  writeIndent(indent);
  write("TaskList");
  write(taskList, "maxwidth");
}

void Saver::buildBattery(unsigned indent, JObject* battery) {
  write("\n");
  writeIndent(indent);
  write("Battery");
}

void Saver::buildDock(unsigned indent, JObject* dock) {
  write("\n");
  writeIndent(indent);
  write("Dock");
}

void Saver::buildClock(unsigned indent, JObject* clock) {
  write("\n");
  writeIndent(indent);
  write("Clock");
  write(clock, "format");

}

#include "misc.h"
#include "Token.h"

static const char *const NEW_CONFIG_FILES[] = { "$XDG_CONFIG_HOME/nwm/nwmrc",
    "$HOME/.config/nwm/nwm", "$HOME/.nwmrc" };

/** Parse the JWM configuration. */
void Configuration::ParseConfig(const char *fileName) {
  Binding::ValidateKeys();

  static const unsigned int NEW_CONFIG_FILE_COUNT = ARRAY_LENGTH(
      NEW_CONFIG_FILES);
  for (unsigned i = 0; i < NEW_CONFIG_FILE_COUNT; ++i) {
    const char *configFile = NEW_CONFIG_FILES[i];
    short type;
    JItemValue config = NicksConfigParser::parse(configFile);

    if (!config.array_val) {
      vLog("Could not parse config file %s\n", configFile);
    } else {
      Configuration::load(config.array_val);
      Configuration::saveConfigs(stdout);
    }
  }
}

