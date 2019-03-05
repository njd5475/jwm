/*
 * DesktopComponent.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "DesktopEnvironment.h"
#include "DesktopComponent.h"
#include "desktop.h"

bool DesktopComponent::registered = environment->RegisterComponent(new DesktopComponent());

void DesktopComponent::initialize() {
  _InitializeDesktops();
}

void DesktopComponent::start() {
  _StartupDesktops();
}

void DesktopComponent::stop() {
  _ShutdownDesktops();
}

void DesktopComponent::destroy() {
  _DestroyDesktops();
}

DesktopComponent::DesktopComponent() {
  // TODO Auto-generated constructor stub

}

DesktopComponent::~DesktopComponent() {
  // TODO Auto-generated destructor stub
}

