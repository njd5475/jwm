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

#include "LogWindow.h"

#define BUT_STATE_OK 	 1
#define BUT_STATE_CANCEL 2

std::vector<LogWindow*> LogWindow::windows;

LogWindow::LogWindow(int x, int y, int width, int height) :
    LoggerListener(), x(x), y(y), width(width), height(height), buttonState(0), window(
        0), pixmap(0), node(0), graphics(0), percentage(0.0f) {

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
  int temp;

  int buttonWidth = Fonts::GetStringWidth(FONT_MENU, "Don't Press");
  temp = Fonts::GetStringWidth(FONT_MENU, "Press Button");
  if (temp > buttonWidth) {
    buttonWidth = temp;
  }
  buttonWidth += 16;
  int buttonHeight = lineHeight + 4;

  int okx = width / 3 - buttonWidth / 2;
  int cancelx = 2 * width / 3 - buttonWidth / 2;
  int buttony = height - lineHeight - lineHeight / 2;

  DrawButton(buttonState == BUT_STATE_OK ? BUTTON_MENU_ACTIVE : BUTTON_MENU,
  ALIGN_CENTER, FONT_MENU, "Press Button", true, true, pixmap, NULL, okx,
      buttony, buttonWidth, buttonHeight, 0, 0);

  DrawButton(buttonState == BUT_STATE_OK ? BUTTON_MENU_ACTIVE : BUTTON_MENU,
  ALIGN_CENTER, FONT_MENU, "Don't Press", true, true, pixmap, NULL, cancelx,
      buttony, buttonWidth, buttonHeight, 0, 0);
}

void LogWindow::DrawAll() {
  std::vector<LogWindow*>::iterator it;
  for (it = windows.begin(); it != windows.end(); ++it) {
    (*it)->Draw();
  }
}

bool LogWindow::process(const XEvent *event) {

  Window window;
  switch (event->type) {
  case Expose:
    window = event->xexpose.window;
    break;
  case ButtonPress:
    window = event->xbutton.window;
    Log("Button Pressed");

    break;
  case ButtonRelease:
    window = event->xbutton.window;
    Log("Button Released");
    break;
  case KeyPress:
    window = event->xkey.window;
    break;
  }

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
