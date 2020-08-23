/*
 * ComponentBuilder.h
 *
 *  Created on: Aug 23, 2020
 *      Author: nick
 */

#ifndef COMPONENTBUILDER_H_
#define COMPONENTBUILDER_H_

class Component;

class ComponentBuilder {
public:
  ComponentBuilder();
  virtual ~ComponentBuilder();

  ComponentBuilder *percentage(float percentX, float percentY);
  ComponentBuilder *label(const char* text);
  Component *build();

private:
  Component *_current;
};

#endif /* COMPONENTBUILDER_H_ */
