/*
 * ApplicationService.cpp
 *
 *  Created on: Jul 20, 2020
 *      Author: nick
 */

#include "stdlib.h"
#include "logger.h"
#include "ApplicationsSystemComponent.h"

ApplicationsComponent::ApplicationsComponent() {
  // TODO Auto-generated constructor stub

}

ApplicationsComponent::~ApplicationsComponent() {
  // TODO Auto-generated destructor stub
}

void ApplicationsComponent::initialize() {
  vLog("PATH is %s\n", std::getenv("PATH"));
}

void ApplicationsComponent::start() {

}

void ApplicationsComponent::stop() {

}

void ApplicationsComponent::destroy() {

}

