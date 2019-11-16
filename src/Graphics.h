/*
 * Graphics.h
 *
 *  Created on: Nov 16, 2019
 *      Author: nick
 */

#ifndef SRC_GRAPHICS_H_
#define SRC_GRAPHICS_H_

#include "jwm.h"
#include <vector>
#include "button.h"

class Graphics {
private:
	Graphics(const Pixmap p, GC gc, Display *display);
	virtual ~Graphics();

public:
	void fillRectangle(int x, int y, int width, int height);
	void drawRectangle(int x, int y, int width, int height);
	void free();
	void line(int x1, int y1, int x2, int y2);
	void point(int x, int y);
	void copy(Drawable dest, int srcX, int srcY, int width, int height, int destX, int destY);
	void resetButton(ButtonNode *bn);
	void setForeground(unsigned short index);
public:
	Pixmap surface;
	GC context;
	Display *_display;

public:
	static Graphics *create(Display *display, GC gc, Drawable d, int width, int height, int rootDepth);
	static Graphics *wrap(const Pixmap p, GC gc, Display *display);

	static Graphics *getRootGraphics(const Pixmap p);
private:
	static std::vector<Graphics*> graphics;
};

#endif /* SRC_GRAPHICS_H_ */
