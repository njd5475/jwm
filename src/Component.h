/*
 * Component.h
 *
 *  Created on: Aug 7, 2020
 *      Author: nick
 */

#ifndef SRC_COMPONENT_H_
#define SRC_COMPONENT_H_

#include "nwm.h"
#include "event.h"
#include "HashMap.h"
#include "ComponentProperty.h"

class Graphics;

class Component : public EventHandler {
public:
  Component(Component *parent);
  virtual ~Component();

  virtual void Draw(Graphics* g) =0;
  virtual void mouseMoved(int mouseX, int mouseY) = 0;
  virtual bool contains(int mouseX, int mouseY) = 0;
  virtual void mouseReleased() = 0;
  virtual int getX() = 0;
  virtual int getY() = 0;
  virtual int getWidth() = 0;
  virtual int getHeight() = 0;
  virtual bool process(const XEvent *event) = 0;
  virtual Component* getParent() {
    return _parent;
  }
  virtual void initProperties(HashMap<ComponentProperty*>* porperties) = 0;
  virtual int getIntProp(const char* propName) = 0;
  virtual const char* getStringProp(const char* propName) = 0;

private:
  Component *_parent;
};

#endif /* SRC_COMPONENT_H_ */
