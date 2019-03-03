/*
 * DesktopComponent.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_DESKTOPCOMPONENT_H_
#define SRC_DESKTOPCOMPONENT_H_

#include "Component.h"

class DesktopComponent : public Component {
public:
  DesktopComponent();
  virtual ~DesktopComponent();

  virtual void initialize();
  virtual void start();
  virtual void stop();
  virtual void destroy();
};

#endif /* SRC_DESKTOPCOMPONENT_H_ */
