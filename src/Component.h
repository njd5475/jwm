/*
 * Component.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_COMPONENT_H_
#define SRC_COMPONENT_H_

class Component {
public:
  Component();
  virtual ~Component();

  virtual void initialize() = 0;
  virtual void start() = 0;
  virtual void destroy() = 0;
  virtual void stop() = 0;

};

#endif /* SRC_COMPONENT_H_ */
