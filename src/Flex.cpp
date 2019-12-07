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
    color(COLOR_TASKLIST_ACTIVE_BG2) {

}

Flex::~Flex() {
  // TODO Auto-generated destructor stub
}

void Flex::Draw() {
  Graphics *g = Graphics::create(display, rootGC, rootWindow, 100, 100, 2);
  g->setForeground(color);
  g->fillRectangle(0, 0, 100, 100);
  g->copy(rootWindow, 0, 0, 100, 100, 100, 100);
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
