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
#include "settings.h"
#include "DesktopEnvironment.h"
#include "battery.h"

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

      JObject *windowStyle = jsonObject(itemObj, "WindowStyle");
      if (windowStyle) {
        builder.buildWindowStyle(windowStyle);
      }

      JObject *popupStyle = jsonObject(itemObj, "PopupStyle");
      if (popupStyle) {
        builder.buildPopupStyle(popupStyle);
      }

      JObject *group = jsonObject(itemObj, "Group");
      if (group) {
        builder.buildGroup(group);
      }

      JObject *desktops = jsonObject(itemObj, "Desktops");
      if (desktops) {
        builder.buildDesktops(desktops);
      }


      JObject *mouse = jsonObject(itemObj, "Mouse");
      if (mouse) {
        builder.buildMouse(mouse);
      }
    }
  }
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
    MouseContextType mc = this->convertToMouseContext(context);
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

void Saver::buildPopupStyle(JObject *object) {
  this->buildStyle("PopupStyle", object);
}


void Saver::buildWindowStyle(JObject *object) {
  this->buildStyle("WindowStyle", object);
}

void Saver::buildStyle(const char *styleName, JObject *object) {
  this->buildStyle(0, styleName, object);
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

void Saver::buildGroup(JObject *jobj) {

}

void Saver::buildDesktops(JObject *jobj) {

}

void Saver::buildMouse(JObject *jobj) {

}


void Saver::buildBattery(unsigned indent, JObject* battery) {
  write("\n");
  writeIndent(indent);
  write("Battery");
}


#include "misc.h"
#include "Token.h"

static const char *const NEW_CONFIG_FILES[] = { "$XDG_CONFIG_HOME/nwm/nwmrc",
    "$HOME/.config/nwm/nwm", "$HOME/.nwmrc" };

/** Parse the NWM configuration. */
void Configuration::ParseConfig(const char *fileName) {

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
    }
  }
}

