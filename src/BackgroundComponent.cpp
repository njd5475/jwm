  /*
 * BackgroundComponent.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "DesktopEnvironment.h"
#include "BackgroundComponent.h"
#include "background.h"

bool BackgroundComponent::registered = environment->RegisterComponent(new BackgroundComponent());

void BackgroundComponent::set(int desktop, const char* type, const char* value) {
  Backgrounds::_SetBackground(desktop, type, value);
}

void BackgroundComponent::loadBackground(int desktop) {
  Backgrounds::_LoadBackground(desktop);
}

BackgroundComponent::BackgroundComponent() {

}

BackgroundComponent::~BackgroundComponent() {

}

void BackgroundComponent::initialize() {
  Backgrounds::_InitializeBackgrounds();
}

void BackgroundComponent::start() {
  Backgrounds::_StartupBackgrounds();
}

void BackgroundComponent::stop() {
  Backgrounds::_ShutdownBackgrounds();
}

void BackgroundComponent::destroy() {
  Backgrounds::_DestroyBackgrounds();
}

