/*
 * Portal.h
 *
 *  Created on: Nov 15, 2019
 *      Author: nick
 */

#ifndef SRC_LOGWINDOW_H_
#define SRC_LOGWINDOW_H_

#include <c++/6/bits/stl_vector.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "LoggerListener.h"
class ClientNode;
class Graphics;

class LogWindow : public LoggerListener {
public:
	LogWindow(const LogWindow &p);
	virtual ~LogWindow();
private:
	LogWindow(int x, int y, int width, int height);

	int x;
	int y;
	int width;
	int height;
	int window;
	Pixmap pixmap;
	int buttonState;
	ClientNode *node;
	Graphics *graphics;
public:
	int getX() {return x;}
	int getY() {return y;}
	int getWidth() {return width;}
	int getHeight() {return height;}
	void Draw();
public:
	static void StartupPortals();
	static void Add(int x, int y, int width, int height);
	static void DrawAll();
	static char ProcessEvents(const XEvent *event);

private:
	virtual void log(const char* message);
	std::vector<const char *> lines;
	static std::vector<LogWindow*> portals;
};

#endif /* SRC_LOGWINDOW_H_ */
