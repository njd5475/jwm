/*
 * TrayComponent.cpp
 *
 *  Created on: Nov 18, 2019
 *      Author: nick
 */

#include "nwm.h"
#include "error.h"
#include "TrayComponent.h"
#include "main.h"
#include "DesktopEnvironment.h"

/** Create an empty tray component. */
TrayComponent::TrayComponent(TrayComponent *parent) :
    screenx(0), screeny(0), parent(parent) {

  this->x = 0;
  this->y = 0;
  this->requestedWidth = 0;
  this->requestedHeight = 0;
  this->width = 0;
  this->height = 0;
  this->grabbed = 0;

  this->window = None;
  this->pixmap = None;
}

TrayComponent::~TrayComponent() {
}

void TrayComponent::addAction(const char *action, int mask) {

  /* Make sure we actually have an action. */
  if (action == NULL || action[0] == 0 || mask == 0) {
    /* Valid (root menu 1). */
  } else if (!strncmp(action, "exec:", 5)) {
    /* Valid. */
  } else if (!strncmp(action, "root:", 5)) {
    /* Valid. However, the specified root menu may not exist.
     * This case is handled in ValidateTrayButtons.
     */
  } else if (!strcmp(action, "showdesktop")) {
    /* Valid. */
  } else {
    /* Invalid; don't add the action. */
    Warning(_("invalid action: \"%s\""), action);
    return;
  }
}

int TrayComponent::getX() const {
  if (this->parent) {
    return this->parent->getX() + this->parent->getWidth();
  }

  return this->x;
}

int TrayComponent::getY() const {
  if (this->parent) {
    return this->parent->getY();
  }

  return this->y;
}
/** Update a specific component on a tray. */
void TrayComponent::UpdateSpecificTray() {
  if (JUNLIKELY(shouldExit)) {
    return;
  }

  /* If the tray is hidden, draw only the background. */
  if (this->getPixmap() != None) {
    JXCopyArea(display, this->getPixmap(), this->getWindow(), rootGC,
        0, 0, this->getWidth(), this->getHeight(), this->getX(), this->getY());
  }
}

void TrayComponent::Draw() {
  Log("TrayComponent Draw called but not implemented\n");
}

void TrayComponent::setPixmap(Pixmap pixmap) {
  this->pixmap = pixmap;
}

void TrayComponent::SetLocation(int x, int y) {
  this->x = x;
  this->y = y;
}

void TrayComponent::requestNewSize(int width, int height) {
  this->requestedWidth = width;
  this->requestedHeight = height;
}

void TrayComponent::SetScreenLocation(int x, int y) {
  this->screenx = x;
  this->screeny = y;
}

void TrayComponent::RefreshSize() {
  if (this->requestedWidth != 0) {
    this->width = this->requestedWidth;
  } else {
    this->width = 0;
  }
  if (this->requestedHeight != 0) {
    this->height = this->requestedHeight;
  } else {
    this->height = 0;
  }
}
