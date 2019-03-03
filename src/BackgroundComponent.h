/*
 * BackgroundComponent.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_BACKGROUNDCOMPONENT_H_
#define SRC_BACKGROUNDCOMPONENT_H_

#include "Component.h"

class BackgroundComponent : public Component {
public:
  BackgroundComponent();
  virtual ~BackgroundComponent();

  virtual void initialize();
  virtual void start();
  virtual void stop();
  virtual void destroy();
  virtual void set(int desktop, const char* type, const char* value);
  virtual void loadBackground(int desktop);
};

#endif /* SRC_BACKGROUNDCOMPONENT_H_ */
