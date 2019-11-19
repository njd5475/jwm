/*
 * Portal.cpp
 *
 *  Created on: Nov 15, 2019
 *      Author: nick
 */

#include "jwm.h"
#include "client.h"
#include "Portal.h"
#include "color.h"
#include "font.h"
#include "button.h"
#include "hint.h"
#include "Graphics.h"

#define BUT_STATE_OK 	 1
#define BUT_STATE_CANCEL 2

std::vector<Portal> Portal::portals;

Portal::Portal(int x, int y, int width, int height) :
		x(x), y(y), width(width), height(height), buttonState(0), window(0), pixmap(0), node(0), graphics(0) {

	XSetWindowAttributes attrs;
	attrs.background_pixel = Colors::lookupColor(COLOR_MENU_BG);
	attrs.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask | ExposureMask;
	pixmap = JXCreatePixmap(display, rootWindow, width, height, rootDepth);
	window = JXCreateWindow(display, rootWindow, x, y, width, height, 0, CopyFromParent, InputOutput, CopyFromParent,
			CWBackPixel | CWEventMask, &attrs);
	graphics = Graphics::getRootGraphics(pixmap);

	XSizeHints shints;
	shints.x = x;
	shints.y = y;
	shints.flags = PPosition;
	JXSetWMNormalHints(display, window, &shints);
	JXStoreName(display, window, _("Portal"));
	Hints::SetAtomAtom(window, ATOM_NET_WM_WINDOW_TYPE, ATOM_NET_WM_WINDOW_TYPE_DIALOG);
	node = new ClientNode(window, 0, 0);
	node->setWMDialogStatus();
	node->FocusClient();

	/* Grab the mouse. */
	JXGrabButton(display, AnyButton, AnyModifier, window, True, ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
			None);
}

Portal::Portal(const Portal &p) {
	this->x = p.x;
	this->y = p.y;
	this->width = p.width;
	this->height = p.height;
	this->pixmap = p.pixmap;
	this->window = p.window;
	this->buttonState = p.buttonState;
	this->node = p.node;
	this->graphics = p.graphics;
}

Portal::~Portal() {
	// TODO Auto-generated destructor stub
}

void Portal::StartupPortals() {

}

void Portal::Add(int x, int y, int width, int height) {
	Portal p(x, y, width, height);
	portals.push_back(p);
}

void Portal::Draw() {
	int yoffset;

	/* Clear the dialog. */
	graphics->setForeground(COLOR_MENU_BG);
	graphics->fillRectangle(0, 0, width, height);

	/* Draw the message. */
	yoffset = 40;
	Fonts::RenderString(pixmap, FONT_MENU, COLOR_MENU_FG, 4, yoffset, width, _("\uF160ASomething"));
	Fonts::RenderString(pixmap, FONT_MENU, COLOR_MENU_FG, 4, yoffset*2, width, "Something else");

	ButtonNode button;
	int temp;

	int buttonWidth = Fonts::GetStringWidth(FONT_MENU, "Don't Press");
	temp = Fonts::GetStringWidth(FONT_MENU, "Press Button");
	if (temp > buttonWidth) {
		buttonWidth = temp;
	}
	buttonWidth += 16;
	int lineHeight = 20;
	int buttonHeight = lineHeight + 4;

	ResetButton(&button, pixmap);
	button.border = 1;
	button.font = FONT_MENU;
	button.width = buttonWidth;
	button.height = buttonHeight;
	button.alignment = ALIGN_CENTER;

	int okx = width / 3 - buttonWidth / 2;
	int cancelx = 2 * width / 3 - buttonWidth / 2;
	int buttony = height - lineHeight - lineHeight / 2;

	if (buttonState == BUT_STATE_OK) {
		button.type = BUTTON_MENU_ACTIVE;
	} else {
		button.type = BUTTON_MENU;
	}
	button.text = "Press Button";
	button.x = okx;
	button.y = buttony;
	DrawButton(&button);

	if (buttonState == BUT_STATE_CANCEL) {
		button.type = BUTTON_MENU_ACTIVE;
	} else {
		button.type = BUTTON_MENU;
	}
	button.text = "Don't Press";
	button.x = cancelx;
	button.y = buttony;
	DrawButton(&button);
}

void Portal::DrawAll() {
	std::vector<Portal>::iterator it;
	for (it = portals.begin(); it != portals.end(); ++it) {
		it->Draw();
	}
}

char Portal::ProcessEvents(const XEvent *event) {

	Window window;
	switch (event->type) {
	case Expose:
		window = event->xexpose.window;
		break;
	case ButtonPress:
		window = event->xbutton.window;
		break;
	case ButtonRelease:
		window = event->xbutton.window;
		break;
	case KeyPress:
		window = event->xkey.window;
		break;
	}

	std::vector<Portal>::iterator it;
	for (it = portals.begin(); it != portals.end(); ++it) {

		if (it->node && it->node->getWindow() == window) {
			it->graphics->copy(it->node->getWindow(), 0, 0, it->width, it->height, 0, 0);
			it->Draw();
			return 1;
		}
	}

	return 0;
}
