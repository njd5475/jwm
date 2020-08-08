/*
 * ApplicationService.h
 *
 *  Created on: Jul 20, 2020
 *      Author: nick
 */

#ifndef SRC_APPLICATIONSSYSTEMCOMPONENT_H_
#define SRC_APPLICATIONSSYSTEMCOMPONENT_H_

#include "SystemComponent.h"

class ApplicationsComponent : public SystemComponent {
public:
  ApplicationsComponent();
  virtual ~ApplicationsComponent();

  void initialize();
  void start();
  void destroy();
  void stop();
};

#endif /* SRC_APPLICATIONSSYSTEMCOMPONENT_H_ */
