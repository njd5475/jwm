/*
 * Portal.cpp
 *
 *  Created on: Nov 15, 2019
 *      Author: nick
 */

#include <algorithm>
#include "jwm.h"
#include "main.h"
#include "client.h"
#include "color.h"
#include "font.h"
#include "button.h"
#include "hint.h"
#include "Graphics.h"
#include "WindowManager.h"
#include "command.h"

#include "LogWindow.h"

#define BUT_STATE_OK 	 1
#define BUT_STATE_CANCEL 2

std::vector<LogWindow*> LogWindow::windows;

void exit() {
  WindowManager::DoExit(0);
}

void nothing() {
  //Commands::RunCommand("");
}

LogWindow::LogWindow(int x, int y, int width, int height) :
    x(x), y(y), width(width), height(height), window(0), percentage(0.0), pixmap(0), buttonState(0), node(0), graphics(0) {

  XSetWindowAttributes attrs;
  attrs.background_pixel = Colors::lookupColor(COLOR_MENU_BG);
  attrs.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask
      | ExposureMask;
  pixmap = JXCreatePixmap(display, rootWindow, width, height, rootDepth);
  window = JXCreateWindow(display, rootWindow, x, y, width, height, 0,
      CopyFromParent, InputOutput, CopyFromParent, CWBackPixel | CWEventMask,
      &attrs);
  graphics = Graphics::getRootGraphics(pixmap);

  XSizeHints shints;
  shints.x = x;
  shints.y = y;
  shints.flags = PPosition;
  JXSetWMNormalHints(display, window, &shints);
  JXStoreName(display, window, _("Portal"));
  Hints::SetAtomAtom(window, ATOM_NET_WM_WINDOW_TYPE,
      ATOM_NET_WM_WINDOW_TYPE_UTILITY);
  node = ClientNode::Create(window, 0, 0);
  node->setNoBorderClose();
  node->keyboardFocus();
  Hints::WriteState(node);

  /* Grab the mouse. */
  JXGrabButton(display, AnyButton, AnyModifier, window, True, ButtonReleaseMask,
      GrabModeAsync, GrabModeAsync, None, None);
  Events::registerHandler(this);

  int lineHeight = 20;
  int temp = Fonts::GetStringWidth(FONT_MENU, "Press Button");
  int buttonWidth = Fonts::GetStringWidth(FONT_MENU, "Don't Press");

  buttonWidth = std::max(temp, buttonWidth);
  buttonWidth += 16;
  int buttonHeight = lineHeight + 4;

  int okx = width / 3 - buttonWidth / 2;
  int cancelx = 2 * width / 3 - buttonWidth / 2;
  int buttony = height - lineHeight - lineHeight / 2;
  this->components.push_back(
      new Button("Start", okx, buttony, buttonWidth, buttonHeight, &nothing));
  this->components.push_back(
      new Button("Exit", cancelx, buttony, buttonWidth, buttonHeight, &exit));
}

LogWindow::LogWindow(const LogWindow &p) {
  this->x = p.x;
  this->y = p.y;
  this->width = p.width;
  this->height = p.height;
  this->pixmap = p.pixmap;
  this->window = p.window;
  this->buttonState = p.buttonState;
  this->node = p.node;
  this->graphics = p.graphics;
  this->percentage = p.percentage;
  Events::registerHandler(this);
}

LogWindow::~LogWindow() {
  for (auto line : lines) {
    free((void*) line);
  }
  lines.clear();

  node->DeleteClient();

  JXDestroyWindow(display, this->window);
  JXFreePixmap(display, this->pixmap);

  Logger::RemoveListener(this);

  std::vector<LogWindow*>::iterator found;
  if ((found = std::find(windows.begin(), windows.end(), this))
      != windows.end()) {
    windows.erase(found);
  }
}

void LogWindow::StartupPortals() {

}

void LogWindow::ShutdownPortals() {
  while (!windows.empty()) {
    delete *windows.erase(windows.begin());
  }
}

void LogWindow::Add(int x, int y, int width, int height) {
  LogWindow *p = new LogWindow(x, y, width, height);
  Logger::AddListener(p);
  windows.push_back(p);

}

void LogWindow::Draw() {
  /* Clear the dialog. */
  graphics->setForeground(COLOR_MENU_BG);
  graphics->fillRectangle(0, 0, width, height);

  /* Draw the message. */
  int lineHeight = 20;
  int lns = (int) (lines.size() * this->percentage);
  std::vector<const char*>::reverse_iterator it = lines.rbegin();
  it += lns;
  for (it = lines.rbegin(); it != lines.rend(); ++it) {
    Fonts::RenderString(pixmap, FONT_MENU, COLOR_MENU_FG, 4, lineHeight * lns,
        width, (*it));
    ++lns;
  }

  for (auto c : components) {
    c->Draw(graphics);
  }
}

void LogWindow::DrawAll() {
  std::vector<LogWindow*>::iterator it;
  for (it = windows.begin(); it != windows.end(); ++it) {
    (*it)->Draw();
  }
}

bool LogWindow::process(const XEvent *event) {

  Window window = 0;
  int mx = 0, my = 0;
  switch (event->type) {
  case Expose:
    window = event->xexpose.window;
    break;
  case ButtonPress:
    window = event->xbutton.window;
    mx = event->xbutton.x;
    my = event->xbutton.y;
    Log("LogWindow: Button Pressed");

    break;
  case ButtonRelease:
    window = event->xbutton.window;
    Log("LogWindow: Button Released");
    mx = event->xbutton.x;
    my = event->xbutton.y;
    for(auto c : components){
      if(c->contains(mx,my)) {
        c->mouseReleased();
      }
    }
    break;
  case MotionNotify:
    window = event->xmotion.window;
    mx = event->xbutton.x;
    my = event->xbutton.y;
    break;
  case KeyPress:
    window = event->xkey.window;
    break;
  }

  for (auto c : components) {
    if(c->contains(mx, my)) {
      c->mouseMoved(mx, my);
    }
  }
  this->Draw();
  graphics->copy(node->getWindow(), 0, 0, this->width, this->height, 0, 0);


  if (this->node && this->node->getWindow() == window) {
    this->graphics->copy(this->node->getWindow(), 0, 0, this->width,
        this->height, 0, 0);
    this->Draw();
    return true;
  }

  return false;
}

void LogWindow::log(const char *message) {
  if (!message || strlen(message) == 0) {
    return;
  }
  lines.push_back(strdup(message));
  this->Draw();
  graphics->copy(node->getWindow(), 0, 0, this->width, this->height, 0, 0);
}
