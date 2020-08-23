/*
 * ComponentBuilder.cpp
 *
 *  Created on: Aug 23, 2020
 *      Author: nick
 */

#include "ComponentBuilder.h"

#include <X11/Xlib.h>
#include <cstring>

#include "BaseComponent.h"
#include "color.h"
#include "Component.h"
#include "font.h"
#include "Graphics.h"
#include "WindowManager.h"

class EmptyComponent: public Component {
public:
  EmptyComponent(Component *parent) :
      Component(parent) {
  }
  virtual ~EmptyComponent() {
  }
  virtual int getX() {
    return getParent()->getX();
  }
  virtual int getY() {
    return getParent()->getY();
  }
  virtual int getWidth() {
    return getParent()->getWidth();
  }
  virtual int getHeight() {
    return getParent()->getHeight();
  }
  virtual bool contains(int mouseX, int mouseY) {
    return mouseX >= getX() && mouseX < (getX() + getWidth())
        && mouseY >= getY() && mouseY < getY() + getHeight();
  }
  virtual void Draw(Graphics *graphics) {
    getParent()->Draw(graphics);
  }
  virtual bool process(const XEvent *event) {
    return false;
  }
  virtual void mouseReleased() {
  }
  virtual void mouseMoved(int, int) {
  }
};

class Percentage: public EmptyComponent {
public:
  Percentage(Component *parent, float pX, float pY) :
      EmptyComponent(parent), _percentX(pX), _percentY(pY) {

  }
  virtual ~Percentage() {
  }
  virtual int getWidth() {
    return getParent()->getWidth() * _percentX;
  }
  virtual int getHeight() {
    return getParent()->getHeight() * _percentY;
  }
private:
  float _percentX;
  float _percentY;
};

class Label: public EmptyComponent {
public:
  Label(Component *parent, const char *text) :
      EmptyComponent(parent), _text(strdup(text)) {
  }
  virtual ~Label() {
    delete _text;
  }
  virtual int getWidth() {
    return Fonts::GetStringWidth(FONT_MENU, _text);
  }
  virtual int getHeight() {
    return Fonts::GetStringHeight(FONT_MENU);
  }
  virtual void Draw(Graphics *g) {
    getParent()->Draw(g);
    g->fillRectangle(0, 0, getWidth(), getHeight());
    g->print(_text, 0, 0, getWidth());
  }
private:
  const char *_text;
};

ComponentBuilder::ComponentBuilder() :
    _current(new BaseComponent()) {
  // TODO Auto-generated constructor stub

}

ComponentBuilder::~ComponentBuilder() {
  // TODO Auto-generated destructor stub
}

ComponentBuilder* ComponentBuilder::percentage(float percentX, float percentY) {
  this->_current = new Percentage(_current, percentX, percentY);
  return this;
}

ComponentBuilder* ComponentBuilder::label(const char *text) {
  this->_current = new Label(_current, text);
  return this;
}

Component* ComponentBuilder::build() {
  WindowManager::WM()->add(_current);
  Component *added = _current;
  _current = new BaseComponent();
  return added;
}
