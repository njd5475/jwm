/*
 * ComponentBuilder.h
 *
 *  Created on: Aug 23, 2020
 *      Author: nick
 */

#ifndef COMPONENTBUILDER_H_
#define COMPONENTBUILDER_H_

#include "HashMap.h"
#include "nwm.h"

class Component;

class ClickHandler {
public:
  virtual ~ClickHandler() {}
  virtual void click(const XEvent *event, Component* me) = 0;
};

class ComponentBuilder {
public:
  ComponentBuilder();
  virtual ~ComponentBuilder();

  ComponentBuilder *percentage(float percentX, float percentY);
  ComponentBuilder *label(const char* text);
  ComponentBuilder *below(const char* name);
  ComponentBuilder *clicked(ClickHandler *handler);
  Component *build(const char* name);
protected:
  bool saveAs(const char* name, Component *component);
  Component *get(const char* name);
private:
  HashMap<Component*> _components;
  Component *_current;
};

#endif /* COMPONENTBUILDER_H_ */
