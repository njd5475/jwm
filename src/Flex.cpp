/*
 * Flex.cpp
 *
 *  Created on: Dec 4, 2019
 *      Author: nick
 */

#include "Flex.h"
#include "main.h"
#include "Graphics.h"

using namespace std;

vector<Flex*> Flex::flexes;

Flex::Flex() :
    color(COLOR_CLOCK_FG) {

  XSetWindowAttributes attr;
  unsigned long attrMask;
  /* Create the tray window. */
  /* The window is created larger for a border. */
  attrMask = CWOverrideRedirect;
  attr.override_redirect = True;

  /* We can't use PointerMotionHintMask since the exact position
   * of the mouse on the tray is important for popups. */
  attrMask |= CWEventMask;
  attr.event_mask = ButtonPressMask | ButtonReleaseMask
      | SubstructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask
      | EnterWindowMask | PointerMotionMask;

  attrMask |= CWBackPixel;
  attr.background_pixel = Colors::lookupColor(COLOR_MENU_ACTIVE_FG);
  this->window = JXCreateWindow(display, rootWindow, 0, 300, 100, 100, 0, rootDepth, InputOutput, rootVisual, attrMask, &attr);
  JXMapWindow(display, this->window);
}

Flex::~Flex() {
  // TODO Auto-generated destructor stub
  JXDestroyWindow(display, this->window);
}

void Flex::Draw() {
  Graphics *g = Graphics::create(display, rootGC, this->window, 100, 100, rootDepth);
  g->setForeground(color);
  g->fillRectangle(0, 0, 100, 100);
  //g->copy(rootWindow, 0, 0, 100, 100, 0, 0);
  Graphics::destroy(g);
}

Flex* Flex::Create() {
  Flex *flex = new Flex();
  flexes.push_back(flex);
  return flex;
}

void Flex::DestroyFlexes() {
  for (auto f : flexes) {
    delete f;
  }
  flexes.clear();
}

void Flex::DrawAll() {
  for (auto f : flexes) {
    f->Draw();
  }
}
