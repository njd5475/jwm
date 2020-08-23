/*
 * Component.cpp
 *
 *  Created on: Aug 7, 2020
 *      Author: nick
 */

#include "Component.h"
#include "nwm.h"
#include "main.h"
#include "client.h"
#include "color.h"
#include "font.h"
#include "button.h"
#include "hint.h"
#include "Graphics.h"
#include "WindowManager.h"
#include "command.h"

Component::Component(Component *parent) : _parent(parent) {

}

Component::~Component() {
  // TODO Auto-generated destructor stub
}

