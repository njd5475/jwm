#include "nwm.h"
#include "ComponentGroup.h"
#include "Component.h"
#include "DesktopEnvironment.h"
#include "WindowManager.h"
#include "color.h"
#include "cursor.h"
#include "hint.h"
#include "jxlib.h"
#include "Graphics.h"
#include "client.h"
#include "event.h"

#include <X11/Xlibint.h>

ComponentGroup::ComponentGroup(Display *display, Window rootWindow) :
		_left(-1), _top(-1), _right(100), _bottom(100), _display(display), _window(
				rootWindow) {
	XSetWindowAttributes attrs;
	attrs.background_pixel = Colors::lookupColor(COLOR_MENU_BG);
	attrs.event_mask = ButtonPressMask | ButtonReleaseMask
			| SubstructureNotifyMask | ExposureMask | KeyPressMask
			| KeyReleaseMask | EnterWindowMask | PointerMotionMask;
	_pixmap = JXCreatePixmap(display, rootWindow, 100, 100, 4);
	_window = JXCreateWindow(display, rootWindow, 0, 0, 100, 100, 0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWBackPixel | CWEventMask, &attrs);
	_graphics = Graphics::getRootGraphics(_pixmap);

	XSizeHints shints;
	shints.x = 0;
	shints.y = 0;
	shints.flags = PPosition;
	JXSetWMNormalHints(display, _window, &shints);
	JXStoreName(display, _window, _("ComponentGroup"));
	Hints::SetAtomAtom(_window, ATOM_NET_WM_WINDOW_TYPE,
			ATOM_NET_WM_WINDOW_TYPE_UTILITY);
	_clientNode = ClientNode::Create(_window, false, false);
	_clientNode->setNoBorderClose();
	_clientNode->keyboardFocus();
	_clientNode->setNoBorderTitle();
	_clientNode->setNoBorderOutline();
	Hints::WriteState(_clientNode);

	/* Grab the mouse. */
	JXGrabButton(display, AnyButton, AnyModifier, _window, True,
			ButtonReleaseMask | ButtonMotionMask | ButtonPressMask,
			GrabModeAsync, GrabModeAsync, None, None);
	Cursors::GrabMouse(_window);
}

ComponentGroup::~ComponentGroup() {

}

void ComponentGroup::Draw() {
	_graphics->setForeground(COLOR_MENU_BG);
	_graphics->fillRectangle(0, 0, getWidth(), getHeight());
	for(auto c : _components) {
		c->Draw(_graphics);
	}
	_graphics->copy(_window, 0, 0, getWidth(), getHeight(), getX()+10, getY()+10);
}

void ComponentGroup::add(Component *c) {

	if (_components.size() == 0) {
		//first can just set bounds
		_left = c->getX();
		_top = c->getY();
		_right = c->getWidth();
		_bottom = c->getHeight();
	}

	this->resizeForComponent(c);
	Events::registerUnconsumedHandler(c);
}

void ComponentGroup::resizeForComponent(Component *c) {
	_left = min(c->getX(), _left);
	_top = min(c->getY(), _top);
	_right = max(c->getX() + c->getWidth(), _right);
	_bottom = max(c->getY() + c->getHeight(), _bottom);

	_components.push_back(c);

	// recreate pixmap
	JXFreePixmap(_display, _pixmap);

	_pixmap = JXCreatePixmap(display, rootWindow, getWidth(), getHeight(),
			rootDepth);

	Graphics::destroy(_graphics);

	_graphics = Graphics::getRootGraphics(_pixmap);

	// resize the window
	JXResizeWindow(display, _window, this->getWidth(), this->getHeight());
	JXMoveWindow(display, _window, this->getX(), this->getY());
	this->Draw();
}

int ComponentGroup::getX() {
	return _left;
}

int ComponentGroup::getY() {
	return _top;
}

int ComponentGroup::getWidth() {
	return abs(_right - _left);
}

int ComponentGroup::getHeight() {
	return abs(_bottom - _top);
}
