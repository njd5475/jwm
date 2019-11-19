/*
 * TrayComponent.cpp
 *
 *  Created on: Nov 18, 2019
 *      Author: nick
 */

#include "jwm.h"
#include "error.h"
#include "action.h"
#include "TrayComponent.h"
#include "main.h"
#include "DesktopEnvironment.h"
#include "tray.h"

void TrayComponent::addAction(const char *action, int mask) {
	ActionNode *ap = new ActionNode(action, mask);

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
	this->actions.push_back(ap);
}

void TrayComponent::validateActions() {
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		(*it)->ValidateAction();
	}
}


void TrayComponent::handleReleaseActions(int x, int y, int button) {
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		(*it)->ProcessActionRelease(this, x, y, button);
	}
}

void TrayComponent::handlePressActions(int x, int y, int button) {
	Log("Tray component handling press action\n");
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		(*it)->ProcessActionPress(this, x, y, button);
	}
}

/** Create an empty tray component. */
TrayComponent::TrayComponent() :
		screenx(0), screeny(0) {

	this->tray = NULL;

	this->x = 0;
	this->y = 0;
	this->requestedWidth = 0;
	this->requestedHeight = 0;
	this->width = 0;
	this->height = 0;
	this->grabbed = 0;

	this->window = None;
	this->pixmap = None;

	this->next = NULL;
}

TrayComponent::~TrayComponent() {
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		delete *it;
	}
	this->actions.clear();
}

/** Update a specific component on a tray. */
void TrayComponent::UpdateSpecificTray(const TrayType *tp) {
	if (JUNLIKELY(shouldExit)) {
		return;
	}

	/* If the tray is hidden, draw only the background. */
	if (this->getPixmap() != None) {
		JXCopyArea(display, this->getPixmap(), tp->getWindow(), rootGC, 0, 0, this->getWidth(), this->getHeight(), 10, 10);
	}
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
