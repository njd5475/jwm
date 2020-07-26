/*
 * Configuration.h
 *
 *  Created on: Feb 27, 2020
 *      Author: nick
 */

#ifndef SRC_CONFIGURATION_H_
#define SRC_CONFIGURATION_H_

#include <vector>

#include "color.h"
#include "font.h"
#include "menu.h"
#include "json.h"
#include "AbstractAction.h"

class Tray;
class Configuration;

class Builder {

public:
  Builder(Configuration* cfg);
  virtual ~Builder();
  virtual void buildIconPath(JObject* jobj) = 0;
  virtual void buildMenuStyle(JObject* jobj) = 0;
  virtual void buildTaskListStyle(JObject* jobj) = 0;
  virtual void buildPagerStyle(JObject* jobj) = 0;
  virtual void buildPopupStyle(JObject* jobj) = 0;
  virtual void buildTrayStyle(JObject* jobj) = 0;
  virtual void buildTray(JObject  *jobj) = 0;
  virtual void buildGroup(JObject *jobj) = 0;
  virtual void buildDesktops(JObject *jobj) = 0;
  virtual void buildTrayButtonStyle(JObject* jobj) = 0;
  virtual void buildRootMenu(JObject* jobj) = 0;
  virtual void buildClockStyle(JObject* jobj) = 0;
  virtual void buildWindowStyle(JObject* jobj) = 0;
  virtual void buildKey(JObject* jobj) = 0;
  virtual void buildMouse(JObject* jobj) = 0;

protected:
  Configuration* _cfg;
};

class Configuration {

public:
  void loadConfig();
  void saveConfig(FILE* output);
  bool isConfigLoaded();
  void write(FILE* output);

  enum OpacityType {TRAY, MENU, TASKLIST, POPUP, PAGER, TRAYBUTTON, ACTIVE_CLIENT, INACTIVE_CLIENT, UNKNOWN};

  struct ActiveColors {
    ColorName bg;
    ColorName bg2;
    ColorName fg;
    ColorName up;
    ColorName down;
  };

  struct StyleColors {
    FontType font;
    ColorName bg;
    ColorName bg2;
    ColorName fg;
    ColorName up;
    ColorName down;
    OpacityType opacityType;
    ActiveColors active;
  };

  static void addConfiguration(JArray *configurations);
  static void load(JArray *configs);
  static void saveConfigs(FILE* output);
  static void loadStyle(JObject *object, FontType f, ColorName bg,
      ColorName bg2, ColorName fg, ColorName up, ColorName down,
      OpacityType otype);
  static void loadStyle(JObject* object, Configuration::StyleColors colors);
  static void loadWindowSettings(JObject* object);
  static Menu *loadMenu(JObject *menuObject, Menu* menu = NULL);
  static MenuItem *loadMenuItem(Menu* menu, MenuItemType type, JObject *item);

private:
  Configuration(JArray *configurations);
  virtual ~Configuration();
  void processConfigs(Builder &builder);

  JArray* _configs;
  bool _loaded;

  static std::vector<Configuration*> configurations;
};

class Saver : public Builder {
public:
  Saver(Configuration *cfg, FILE *ouput);
  virtual ~Saver();

  virtual void buildIconPath(JObject* iconPath);
  virtual void buildMenuStyle(JObject* jobj);
  virtual void buildMenus(unsigned indent, const char* elementName, JObject *object);
  virtual void buildTaskListStyle(JObject* jobj);
  virtual void buildPagerStyle(JObject* jobj);
  virtual void buildPopupStyle(JObject* jobj);
  virtual void buildTrayStyle(JObject* jobj);
  virtual void buildTray(JObject  *jobj);
  virtual void buildDesktops(JObject *jobj);
  virtual void buildGroup(JObject *jobj);
  virtual void buildTrayButtonStyle(JObject* jobj);
  virtual void buildClockStyle(JObject* jobj);
  virtual void buildRootMenu(JObject* jobj);
  virtual void buildWindowStyle(JObject* jobj);
  virtual void buildStyle(const char* styleName, JObject* jobj);
  virtual void buildStyle(unsigned indent, const char* styleName, JObject* jobj);
  virtual void buildMenuItem(int indent, const char* type, JObject *itemObj);
  virtual void buildKey(JObject* jobj);
  virtual void buildMouse(JObject* jobj);

  virtual void write(const char* str);
  virtual void write(const char* key, const char* value);
  virtual void writeIndent(unsigned indent);
  virtual void write(JObject *obj, const char *key);

  void buildTrayButton(unsigned indent, JObject *trayButton);
  void buildPager(unsigned indent, JObject *pager);
  void buildTaskList(unsigned indent, JObject* taskList);
  void buildBattery(unsigned indent, JObject* battery);
  void buildDock(unsigned indent, JObject* dock);
  void buildClock(unsigned indent, JObject* clock);

private:
  FILE* _output;
  const unsigned TABS;
};

class Loader : public Builder {
public:
  Loader(Configuration *cfg);
  virtual ~Loader();

  virtual void buildIconPath(JObject* iconPath);
  virtual void buildMenuStyle(JObject* jobj);
  virtual void buildTaskListStyle(JObject* jobj);
  virtual void buildPagerStyle(JObject* jobj);
  virtual void buildPopupStyle(JObject* jobj);
  virtual void buildClockStyle(JObject* jobj);
  virtual void buildTrayStyle(JObject* jobj);
  virtual void buildTray(JObject  *jobj);
  virtual void buildDesktops(JObject *jobj);
  virtual void buildGroup(JObject *jobj);
  virtual void buildTrayButtonStyle(JObject* jobj);
  virtual void buildRootMenu(JObject* jobj);
  virtual void buildWindowStyle(JObject* jobj);
  virtual void buildKey(JObject* jobj);
  virtual void buildMouse(JObject* jobj);

  void buildTrayButton(Tray *tray, JObject *trayButton);
  void buildPager(Tray *tray, JObject *pager);
  void buildTaskList(Tray *tray, JObject* taskList);
  void buildBattery(Tray *tray, JObject* battery);
  void buildDock(Tray *tray, JObject* dock);
  void buildClock(Tray *tray, JObject* clock);
  Actions convertToAction(const char* action);
  int convertToMouseContext(const char* context);

};

#endif /* SRC_CONFIGURATION_H_ */
