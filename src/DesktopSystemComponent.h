/*
 * DesktopComponent.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_DESKTOPSYSTEMCOMPONENT_H_
#define SRC_DESKTOPSYSTEMCOMPONENT_H_

#include "SystemComponent.h"

class DesktopComponent : public SystemComponent {
public:
  DesktopComponent();
  virtual ~DesktopComponent();

  virtual void initialize();
  virtual void start();
  virtual void stop();
  virtual void destroy();
private:
  static bool registered;
};

#endif /* SRC_DESKTOPSYSTEMCOMPONENT_H_ */
