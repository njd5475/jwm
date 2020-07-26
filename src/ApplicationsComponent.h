/*
 * ApplicationService.h
 *
 *  Created on: Jul 20, 2020
 *      Author: nick
 */

#ifndef SRC_APPLICATIONSCOMPONENT_H_
#define SRC_APPLICATIONSCOMPONENT_H_

#include "Component.h"

class ApplicationsComponent : public Component {
public:
  ApplicationsComponent();
  virtual ~ApplicationsComponent();

  void initialize();
  void start();
  void destroy();
  void stop();
};

#endif /* SRC_APPLICATIONSCOMPONENT_H_ */
