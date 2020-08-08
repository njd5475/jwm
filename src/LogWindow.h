/*
 * Portal.h
 *
 *  Created on: Nov 15, 2019
 *      Author: nick
 */

#ifndef SRC_LOGWINDOW_H_
#define SRC_LOGWINDOW_H_

#include <vector>

#include "jwm.h"
#include "main.h"
#include "LoggerListener.h"
#include "event.h"

class Component;
class ClientNode;
class Graphics;

class LogWindow : public LoggerListener, EventHandler {
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
	float percentage;
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
	static void ShutdownPortals();
	static void Add(int x, int y, int width, int height);
	static void DrawAll();
	bool process(const XEvent *event);

private:
	virtual void log(const char* message);
	std::vector<const char *> lines;
	static std::vector<LogWindow*> windows;
	std::vector<Component*> components;
};

#endif /* SRC_LOGWINDOW_H_ */
