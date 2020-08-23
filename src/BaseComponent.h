/*
 * BaseComponent.h
 *
 *  Created on: Aug 22, 2020
 *      Author: nick
 */

#ifndef SRC_BASECOMPONENT_H_
#define SRC_BASECOMPONENT_H_

#include "Component.h"

class BaseComponent : public Component {
public:
  BaseComponent();
  virtual ~BaseComponent();

  virtual void Draw(Graphics* g);
  virtual void mouseMoved(int mouseX, int mouseY);
  virtual bool contains(int mouseX, int mouseY);
  virtual void mouseReleased();
  virtual int getX();
  virtual int getY();
  virtual int getWidth();
  virtual int getHeight();
  virtual bool process(const XEvent *event);
};

#endif /* SRC_BASECOMPONENT_H_ */
