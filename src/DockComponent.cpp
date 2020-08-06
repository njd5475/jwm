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

bool DockComponent::registered = environment->RegisterComponent(new DockComponent());

void DockComponent::initialize() {
  Dock::_InitializeDock();
}

void DockComponent::start() {
  Dock::_StartupDock();
}

void DockComponent::stop() {
  Dock::_ShutdownDock();
}

void DockComponent::destroy() {
  Dock::_DestroyDock();
}

DockComponent::DockComponent() : Component() {

}

DockComponent::~DockComponent() {

}
