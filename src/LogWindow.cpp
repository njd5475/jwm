/*
 * Portal.cpp
 *
 *  Created on: Nov 15, 2019
 *      Author: nick
 */

#include "LogWindow.h"

#include "jwm.h"
#include "client.h"
#include "color.h"
#include "font.h"
#include "button.h"
#include "hint.h"
#include "Graphics.h"

#define BUT_STATE_OK 	 1
#define BUT_STATE_CANCEL 2

std::vector<LogWindow*> LogWindow::portals;

LogWindow::LogWindow(int x, int y, int width, int height) : LoggerListener(),
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

LogWindow::LogWindow(const LogWindow &p) {
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

LogWindow::~LogWindow() {
	// TODO Auto-generated destructor stub
}

void LogWindow::StartupPortals() {

}

void LogWindow::Add(int x, int y, int width, int height) {
	LogWindow *p = new LogWindow(x, y, width, height);
	Logger::AddListener(p);
	portals.push_back(p);

}

void LogWindow::Draw() {
	/* Clear the dialog. */
	graphics->setForeground(COLOR_MENU_BG);
	graphics->fillRectangle(0, 0, width, height);

	/* Draw the message. */
	int lineHeight = 20;
	int lns = 0;
	std::vector<const char*>::reverse_iterator it;
	for(it = lines.rbegin(); it != lines.rend(); ++it) {
		Fonts::RenderString(pixmap, FONT_MENU, COLOR_MENU_FG, 4, lineHeight*lns, width, (*it));
		++lns;
	}
	ButtonNode button;
	int temp;

	int buttonWidth = Fonts::GetStringWidth(FONT_MENU, "Don't Press");
	temp = Fonts::GetStringWidth(FONT_MENU, "Press Button");
	if (temp > buttonWidth) {
		buttonWidth = temp;
	}
	buttonWidth += 16;
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

void LogWindow::DrawAll() {
	std::vector<LogWindow*>::iterator it;
	for (it = portals.begin(); it != portals.end(); ++it) {
		(*it)->Draw();
	}
}

char LogWindow::ProcessEvents(const XEvent *event) {

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

	std::vector<LogWindow*>::iterator it;
	for (it = portals.begin(); it != portals.end(); ++it) {
		LogWindow *p = (*it);
		if (p->node && p->node->getWindow() == window) {
			p->graphics->copy(p->node->getWindow(), 0, 0, p->width, p->height, 0, 0);
			p->Draw();
			return 1;
		}
	}

	return 0;
}

void LogWindow::log(const char* message) {
	lines.push_back(strdup(message));
	this->Draw();
	this->graphics->copy(this->node->getWindow(), 0, 0, this->width, this->height, 0, 0);
}
