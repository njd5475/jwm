/*
 * BackgroundComponent.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_BACKGROUNDSYSTEMCOMPONENT_H_
#define SRC_BACKGROUNDSYSTEMCOMPONENT_H_

#include "SystemComponent.h"

class BackgroundComponent : public SystemComponent {
public:
  BackgroundComponent();
  virtual ~BackgroundComponent();

  virtual void initialize();
  virtual void start();
  virtual void stop();
  virtual void destroy();
  virtual void set(int desktop, const char* type, const char* value);
  virtual void loadBackground(int desktop);
private:
  static bool registered;
};

#endif /* SRC_BACKGROUNDSYSTEMCOMPONENT_H_ */
