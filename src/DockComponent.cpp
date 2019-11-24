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
  DockType::_InitializeDock();
}

void DockComponent::start() {
  DockType::_StartupDock();
}

void DockComponent::stop() {
  DockType::_ShutdownDock();
}

void DockComponent::destroy() {
  DockType::_DestroyDock();
}

DockComponent::DockComponent() {

}

DockComponent::~DockComponent() {

}
