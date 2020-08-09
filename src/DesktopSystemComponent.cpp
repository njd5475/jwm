/*
 * DesktopComponent.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "DesktopEnvironment.h"
#include "desktop.h"
#include "DesktopSystemComponent.h"

bool DesktopSystemComponent::registered = environment->RegisterComponent(new DesktopSystemComponent());

void DesktopSystemComponent::initialize() {
	Desktops::_InitializeDesktops();
}

void DesktopSystemComponent::start() {
	Desktops::_StartupDesktops();
}

void DesktopSystemComponent::stop() {
	Desktops::_ShutdownDesktops();
}

void DesktopSystemComponent::destroy() {
	Desktops::_DestroyDesktops();
}

DesktopSystemComponent::DesktopSystemComponent() {

}

DesktopSystemComponent::~DesktopSystemComponent() {

}

