/*
 * ComponentProperty.h
 *
 *  Created on: Aug 30, 2020
 *      Author: nick
 */

#ifndef SRC_COMPONENTPROPERTY_H_
#define SRC_COMPONENTPROPERTY_H_

class ComponentProperty {
public:
  ComponentProperty() { _value = 0; }
  virtual ~ComponentProperty() {}

  void set(void* val) { _value = val; }
  void* get() { return _value; }
private:
  void* _value;
};

#endif /* SRC_COMPONENTPROPERTY_H_ */
