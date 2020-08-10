/*
 * DesktopEnvironment.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "nwm.h"
#include "DesktopEnvironment.h"
#include "background.h"
#include "event.h"
#include "desktop.h"
#include "command.h"

Display *display = NULL;
Window rootWindow;
int rootWidth, rootHeight;
int rootScreen;
Colormap rootColormap;
Visual *rootVisual;
int rootDepth;
GC rootGC;
int colormapCount;
Window supportingWindow;
Atom managerSelection;

char shouldExit = 0;
char shouldRestart = 0;
char isRestarting = 0;
char initializing = 0;
char shouldReload = 0;

unsigned int currentDesktop = 0;

char *exitCommand = NULL;

XContext clientContext;
XContext frameContext;

#ifdef USE_SHAPE
char haveShape;
int shapeEvent;
#endif
#ifdef USE_XRENDER
char haveRender;
#endif

char *configPath = NULL;

DesktopEnvironment* DesktopEnvironment::_instance = NULL;
char *DesktopEnvironment::displayString = NULL;

void DesktopEnvironment::LoadBackground(unsigned int num) {
  Backgrounds::_LoadBackground(num);
}

void DesktopEnvironment::ChangeDesktop(unsigned short int num) {
  Desktops::_ChangeDesktop(num);
}

void DesktopEnvironment::ShowDesktop() {
	Desktops::_ShowDesktop();
}

bool DesktopEnvironment::RegisterComponent(SystemComponent *component) {
  ++this->_componentCount;
  this->_components.push_back(component);
  return true;
}

DesktopEnvironment::DesktopEnvironment() :
     _components(0), _componentCount(0) {

}

DesktopEnvironment::~DesktopEnvironment() {
  for(auto c : this->_components) {
    delete c;
  }
  this->_components.clear();
  _componentCount = 0;
}

const char* DesktopEnvironment::getLocale() {
  const char* locale = Commands::ReadFromProcess("/usr/bin/locale", 1000);
  printf(locale);
  return "";
}

const char* DesktopEnvironment::GetDesktopName(unsigned short int num) {
  return Desktops::_GetDesktopName(num);
}

const unsigned DesktopEnvironment::GetRightDesktop(signed short int num) {
  return Desktops::_GetRightDesktop(num);
}

const unsigned DesktopEnvironment::GetLeftDesktop(signed short int num) {
  return Desktops::_GetLeftDesktop(num);
}

const unsigned DesktopEnvironment::GetAboveDesktop(signed short int num) {
  return Desktops::_GetAboveDesktop(num);
}

const unsigned DesktopEnvironment::GetBelowDesktop(signed short int num) {
  return Desktops::_GetBelowDesktop(num);
}

void DesktopEnvironment::InitializeComponents() {
  for(auto c : this->_components) {
    c->initialize();
  }
}

void DesktopEnvironment::StartupComponents() {
  for(auto c : this->_components) {
    c->start();
  }

  getLocale();
}

void DesktopEnvironment::ShutdownComponents() {
  for(auto c : this->_components) {
    c->stop();
  }
}

void DesktopEnvironment::DestroyComponents() {
  for(auto c : this->_components) {
    c->destroy();
  }
}

bool DesktopEnvironment::RightDesktop() {
  return Desktops::_RightDesktop();
}

bool DesktopEnvironment::LeftDesktop() {
  return Desktops::_LeftDesktop();
}

bool DesktopEnvironment::AboveDesktop() {
  return Desktops::_AboveDesktop();
}

bool DesktopEnvironment::BelowDesktop() {
  return Desktops::_BelowDesktop();
}

void DesktopEnvironment::SetDesktopName(int num, const char* name) {
	Desktops::_SetDesktopName(num, name);
}

void DesktopEnvironment::SetBackground(int id, const char* file, char* const value) {
  Backgrounds::_SetBackground(id, file, value);
}

bool DesktopEnvironment::OpenConnection() {
  /** Open a connection to the X server. */

  display = JXOpenDisplay(displayString);
  if (JUNLIKELY(!display)) {
    if (displayString) {
      printf("error: could not open display %s\n", displayString);
    } else {
      printf("error: could not open display\n");
    }
    return false;
  }

  rootScreen = DefaultScreen(display);
  rootWindow = RootWindow(display, rootScreen);
  rootWidth = DisplayWidth(display, rootScreen);
  rootHeight = DisplayHeight(display, rootScreen);
  rootDepth = DefaultDepth(display, rootScreen);
  rootVisual = DefaultVisual(display, rootScreen);
  rootColormap = DefaultColormap(display, rootScreen);
  rootGC = DefaultGC(display, rootScreen);
  colormapCount = MaxCmapsOfScreen(ScreenOfDisplay(display, rootScreen));

  XSetGraphicsExposures(display, rootGC, False);
  return true;
}
