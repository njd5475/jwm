/*
 * DesktopEnvironment.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "DesktopEnvironment.h"
#include "dock.h"
#include "background.h"
#include "event.h"

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

char DesktopEnvironment::HandleDockReparentNotify(const XReparentEvent* event) {
  return _HandleDockReparentNotify(event);
}

char DesktopEnvironment::HandleDockDestroy(unsigned long int num) {
  return _HandleDockDestroy(num);
}

char DesktopEnvironment::HandleDockConfigureRequest(const XConfigureRequestEvent* event) {
  return _HandleDockConfigureRequest(event);
}

void DesktopEnvironment::HandleDockEvent(const XClientMessageEvent* event) {
  _HandleDockEvent(event);
}

void DesktopEnvironment::LoadBackground(unsigned int num) {
  _LoadBackground(num);
}

void DesktopEnvironment::ChangeDesktop(unsigned short int num) {
  _ChangeDesktop(num);
}

void DesktopEnvironment::ShowDesktop() {
  _ShowDesktop();
}

void DesktopEnvironment::RegisterComponent(Component *component) {
  ++this->_componentCount;
  this->_components.push_back(component);
}

DesktopEnvironment::DesktopEnvironment() :
    _componentCount(0), _components(0) {
  // TODO Auto-generated constructor stub

}

DesktopEnvironment::~DesktopEnvironment() {
  for (std::vector<Component*>::iterator it = this->_components.begin() ; it != this->_components.end(); ++it) {
    delete *it;
  }
  this->_components.clear();
  _componentCount = 0;
}

const char* DesktopEnvironment::GetDesktopName(unsigned short int num) {
  return _GetDesktopName(num);
}

const unsigned DesktopEnvironment::GetRightDesktop(signed short int num) {
  return _GetRightDesktop(num);
}

const unsigned DesktopEnvironment::GetLeftDesktop(signed short int num) {
  return _GetLeftDesktop(num);
}

const unsigned DesktopEnvironment::GetAboveDesktop(signed short int num) {
  return _GetAboveDesktop(num);
}

const unsigned DesktopEnvironment::GetBelowDesktop(signed short int num) {
  return _GetBelowDesktop(num);
}

char DesktopEnvironment::HandleDockResizeRequest(XResizeRequestEvent* event) {
  return _HandleDockResizeRequest(event);
}

void DesktopEnvironment::InitializeComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin() ; it != this->_components.end(); ++it) {
    (*it)->initialize();
  }
}

void DesktopEnvironment::StartupComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin() ; it != this->_components.end(); ++it) {
    (*it)->start();
  }
}

void DesktopEnvironment::ShutdownComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin() ; it != this->_components.end(); ++it) {
    (*it)->stop();
  }
}

void DesktopEnvironment::DestroyComponents() {
  for (std::vector<Component*>::iterator it = this->_components.begin() ; it != this->_components.end(); ++it) {
    (*it)->destroy();
  }
}

bool DesktopEnvironment::RightDesktop() {
  return _RightDesktop();
}

bool DesktopEnvironment::LeftDesktop() {
  return _LeftDesktop();
}

bool DesktopEnvironment::AboveDesktop() {
  return _AboveDesktop();
}

bool DesktopEnvironment::BelowDesktop() {
  return _BelowDesktop();
}

void DesktopEnvironment::SetDesktopName(int num, const char* name) {
  _SetDesktopName(num, name);
}

void DesktopEnvironment::SetBackground(int id, const char* file, char* const name) {
  _SetBackground(id, file, name);
}

TrayComponentType* DesktopEnvironment::CreateDock(int width) {
  return _CreateDock(width);
}

Menu* DesktopEnvironment::CreateDesktopMenu(int desktop, void* mem) {
  return _CreateDesktopMenu(desktop, mem);
}

Menu* DesktopEnvironment::CreateSendtoMenu(int desktop, void* mem) {
  return _CreateSendtoMenu(desktop, mem);
}

char DesktopEnvironment::HandleDockSelectionClear(const XSelectionClearEvent* event) {
  return _HandleDockSelectionClear(event);
}
