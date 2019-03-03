/*
 * DockComponent.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "jwm.h"
#include "DesktopEnvironment.h"
#include "DockComponent.h"
#include "dock.h"

#include "DesktopEnvironment.h"

void DockComponent::initialize() {
  _InitializeDock();
}

void DockComponent::start() {
  _StartupDock();
}

void DockComponent::stop() {
  _ShutdownDock();
}

void DockComponent::destroy() {
  _DestroyDock();
}

DockComponent::DockComponent() {
  // TODO Auto-generated constructor stub

}

DockComponent::~DockComponent() {
  // TODO Auto-generated destructor stub
}
