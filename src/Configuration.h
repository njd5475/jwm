/*
 * Configuration.h
 *
 *  Created on: Feb 27, 2020
 *      Author: nick
 */

#ifndef SRC_CONFIGURATION_H_
#define SRC_CONFIGURATION_H_

#include <vector>
#include "json.h"

class Configuration {

public:
  void loadConfig();
  bool isConfigLoaded();

  enum OpacityType {TRAY, MENU, ACTIVE_CLIENT, INACTIVE_CLIENT, UNKNOWN};

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
  static void loadStyle(JObject *object, FontType f, ColorName bg,
      ColorName bg2, ColorName fg, ColorName up, ColorName down,
      OpacityType otype);
  static void loadStyle(JObject* object, Configuration::StyleColors colors);

private:
  Configuration(JArray *configurations);
  virtual ~Configuration();

  JArray* _configs;
  bool _loaded;

  static std::vector<Configuration*> configurations;
};

#endif /* SRC_CONFIGURATION_H_ */
