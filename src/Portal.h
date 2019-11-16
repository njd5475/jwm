/*
 * Portal.h
 *
 *  Created on: Nov 15, 2019
 *      Author: nick
 */

#ifndef SRC_PORTAL_H_
#define SRC_PORTAL_H_

#include "main.h"
#include "client.h"
#include <vector>

class Portal {
public:
	Portal(const Portal &p);
	virtual ~Portal();
private:
	Portal(int x, int y, int width, int height);

	int x;
	int y;
	int width;
	int height;
	int window;
	Pixmap pixmap;
	int buttonState;
	ClientNode *node;
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
	static std::vector<Portal> portals;
};

#endif /* SRC_PORTAL_H_ */
