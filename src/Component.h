/*
 * Component.h
 *
 *  Created on: Aug 7, 2020
 *      Author: nick
 */

#ifndef SRC_COMPONENT_H_
#define SRC_COMPONENT_H_

class Graphics;

class Component {
public:
  Component();
  virtual ~Component();

  virtual void Draw(Graphics* g) =0;
  virtual void mouseMoved(int mouseX, int mouseY) = 0;
  virtual bool contains(int mouseX, int mouseY) = 0;
  virtual void mouseReleased() = 0;
};

#endif /* SRC_COMPONENT_H_ */
