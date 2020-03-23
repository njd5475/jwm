/*
 * Graphics.cpp
 *
 *  Created on: Nov 16, 2019
 *      Author: nick
 */

#include "jwm.h"
#include "main.h"
#include "Graphics.h"
#include "button.h"

Graphics::Graphics(const Pixmap p, GC gc, Display *display) :
		surface(p), context(gc), _display(display) {

}

Graphics::~Graphics() {

}

void Graphics::fillRectangle(int x, int y, int width, int height) {
	XFillRectangle(_display, surface, context, x, y, width, height);
}

void Graphics::drawRectangle(int x, int y, int width, int height) {
	XDrawRectangle(_display, surface, context, x, y, width, height);
}

void Graphics::line(int x1, int y1, int x2, int y2) {
	JXDrawLine(_display, surface, context, x1, y1, x2, y2);
}

void Graphics::point(int x, int y) {
	JXDrawPoint(_display, surface, context, x, y);
}

void Graphics::copy(Drawable dest, int srcX, int srcY, int width, int height, int destX, int destY) {
	JXCopyArea(_display, surface, dest, context, srcX, srcY, width, height, destX, destY);
}

void Graphics::free() {
	JXFreePixmap(_display, surface);
}

void Graphics::setForeground(unsigned short index) {
	JXSetForeground(_display, context, Colors::lookupColor(index));
}

Pixmap Graphics::getPixmap() {
  return this->surface;
}

void Graphics::drawButton(ButtonType type, AlignmentType alignment, FontType font, const char *text, bool fill,
		bool border, struct Icon *icon, int x, int y, int width, int height, int xoffset, int yoffset) {
	DrawButton(type, alignment, font, text, fill, border, this->surface, icon, x, y, width, height, xoffset, yoffset);
}

std::vector<Graphics*> Graphics::graphics;

Graphics* Graphics::create(Display *display, GC gc, Drawable d, int width, int height, int rootDepth) {
	Pixmap map = JXCreatePixmap(display, d, width, height, rootDepth);
	return wrap(map, gc, display);
}

void Graphics::destroy(Graphics *g) {
	std::vector<Graphics*>::iterator it;
	char found = 0;
	for (it = graphics.begin(); it != graphics.end(); ++it) {
		if (*it == g) {
			found = 1;
			break;
		}
	}

	if (found) {
		graphics.erase(it);
	}

	g->free();
	delete g;
}

Graphics* Graphics::wrap(const Pixmap p, GC gc, Display *display) {
	Graphics *g = new Graphics(p, gc, display);
	graphics.push_back(g);
	return g;
}

Graphics* Graphics::getRootGraphics(const Pixmap p) {
	return wrap(p, rootGC, display);
}
