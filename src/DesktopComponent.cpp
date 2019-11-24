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
	Desktops::_InitializeDesktops();
}

void DesktopComponent::start() {
	Desktops::_StartupDesktops();
}

void DesktopComponent::stop() {
	Desktops::_ShutdownDesktops();
}

void DesktopComponent::destroy() {
	Desktops::_DestroyDesktops();
}

DesktopComponent::DesktopComponent() {

}

DesktopComponent::~DesktopComponent() {

}

