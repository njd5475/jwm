/*
 * DesktopEnvironment.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "jwm.h"
#include "DesktopEnvironment.h"
#include "dock.h"
#include "background.h"
#include "event.h"
#include "desktop.h"
#include "tray.h"

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

char DesktopEnvironment::HandleDockReparentNotify(const XReparentEvent* event) {
  return DockType::_HandleDockReparentNotify(event);
}

char DesktopEnvironment::HandleDockDestroy(unsigned long int num) {
  return DockType::_HandleDockDestroy(num);
}

char DesktopEnvironment::HandleDockConfigureRequest(const XConfigureRequestEvent* event) {
  return DockType::_HandleDockConfigureRequest(event);
}

void DesktopEnvironment::HandleDockEvent(const XClientMessageEvent* event) {
  DockType::_HandleDockEvent(event);
}

void DesktopEnvironment::LoadBackground(unsigned int num) {
  Backgrounds::_LoadBackground(num);
}

void DesktopEnvironment::ChangeDesktop(unsigned short int num) {
  Desktops::_ChangeDesktop(num);
}

void DesktopEnvironment::ShowDesktop() {
	Desktops::_ShowDesktop();
}

bool DesktopEnvironment::RegisterComponent(Component *component) {
  ++this->_componentCount;
  this->_components.push_back(component);
}

DesktopEnvironment::DesktopEnvironment() :
    _componentCount(0), _components(0) {
  // TODO Auto-generated constructor stub

}

DesktopEnvironment::~DesktopEnvironment() {
  for (std::vector<Component*>::iterator it = this->_components.begin(); it != this->_components.end(); ++it) {
    delete *it;
  }
  this->_components.clear();
  _componentCount = 0;
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

char DesktopEnvironment::HandleDockResizeRequest(XResizeRequestEvent* event) {
  return this->HandleDockResizeRequest(event);
}

void DesktopEnvironment::InitializeComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin(); it != this->_components.end(); ++it) {
    (*it)->initialize();
  }
}

void DesktopEnvironment::StartupComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin(); it != this->_components.end(); ++it) {
    (*it)->start();
  }
}

void DesktopEnvironment::ShutdownComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin(); it != this->_components.end(); ++it) {
    (*it)->stop();
  }
}

void DesktopEnvironment::DestroyComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin(); it != this->_components.end(); ++it) {
    (*it)->destroy();
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

void DesktopEnvironment::SetBackground(int id, const char* file, char* const name) {
  Backgrounds::_SetBackground(id, file, name);
}

TrayComponent* DesktopEnvironment::CreateDock(int width, Tray *tray, TrayComponent *parent) {
  return DockType::dock = new DockType(width, tray, parent);
}

Menu* DesktopEnvironment::CreateDesktopMenu(int desktop, void* mem) {
  return Desktops::_CreateDesktopMenu(desktop, mem);
}

Menu* DesktopEnvironment::CreateSendtoMenu(int desktop, void* mem) {
  return Desktops::_CreateSendtoMenu(desktop, mem);
}

char DesktopEnvironment::HandleDockSelectionClear(const XSelectionClearEvent* event) {
  return this->HandleDockSelectionClear(event);
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
