/*
 * BaseComponent.cpp
 *
 *  Created on: Aug 22, 2020
 *      Author: nick
 */

#include "BaseComponent.h"
#include "screen.h"

BaseComponent::BaseComponent() :
    Component(NULL) {
  // TODO Auto-generated constructor stub

}

BaseComponent::~BaseComponent() {
  // TODO Auto-generated destructor stub
}

void BaseComponent::Draw(Graphics *g) {
}

void BaseComponent::mouseMoved(int mouseX, int mouseY) {
}

bool BaseComponent::contains(int mouseX, int mouseY) {
  return mouseX >= getX() && mouseX < (getX() + getWidth())
          && mouseY >= getY() && mouseY < getY() + getHeight();
}

void BaseComponent::mouseReleased() {
}

int BaseComponent::getX() {
  return Screens::GetMouseScreen()->x;
}

int BaseComponent::getY() {
  return Screens::GetMouseScreen()->y;
}

int BaseComponent::getWidth() {
  return Screens::GetMouseScreen()->width;
}

int BaseComponent::getHeight() {
  return Screens::GetMouseScreen()->height;
}

bool BaseComponent::process(const XEvent *event) {
  return false;
}

void BaseComponent::initProperties(HashMap<ComponentProperty*>* properties) {
  //do nothings
}

int BaseComponent::getIntProp(const char* propName) {
  return 0;
}

const char* BaseComponent::getStringProp(const char* propName) {
  return "";
}
