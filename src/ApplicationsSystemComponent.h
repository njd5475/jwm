/*
 * ApplicationService.h
 *
 *  Created on: Jul 20, 2020
 *      Author: nick
 */

#ifndef SRC_APPLICATIONSSYSTEMCOMPONENT_H_
#define SRC_APPLICATIONSSYSTEMCOMPONENT_H_

#include "SystemComponent.h"

#include <vector>
#include <string>

class ApplicationsSystemComponent : public SystemComponent {
public:
  ApplicationsSystemComponent();
  virtual ~ApplicationsSystemComponent();

  void initialize();
  void start();
  void destroy();
  void stop();

private:
  std::vector<std::string> files;
};

#endif /* SRC_APPLICATIONSSYSTEMCOMPONENT_H_ */
