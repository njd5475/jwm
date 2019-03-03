  /*
 * BackgroundComponent.cpp
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#include "BackgroundComponent.h"
#include "background.h"

void BackgroundComponent::set(int desktop, const char* type, const char* value) {
  _SetBackground(desktop, type, value);
}

void BackgroundComponent::loadBackground(int desktop) {
  _LoadBackground(desktop);
}

BackgroundComponent::BackgroundComponent() {
  // TODO Auto-generated constructor stub

}

BackgroundComponent::~BackgroundComponent() {
  // TODO Auto-generated destructor stub
}

void BackgroundComponent::initialize() {
  _InitializeBackgrounds();
}

void BackgroundComponent::start() {
  _StartupBackgrounds();
}

void BackgroundComponent::stop() {
  _ShutdownBackgrounds();
}

void BackgroundComponent::destroy() {
  _DestroyBackgrounds();
}

