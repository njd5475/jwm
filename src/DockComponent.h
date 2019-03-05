/*
 * DockComponent.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_DOCKCOMPONENT_H_
#define SRC_DOCKCOMPONENT_H_

#include "Component.h"

class DockComponent : public Component {
public:
  DockComponent();
  virtual ~DockComponent();

  virtual void initialize();
  virtual void start();
  virtual void stop();
  virtual void destroy();
private:
  static bool registered;
};

#endif /* SRC_DOCKCOMPONENT_H_ */
