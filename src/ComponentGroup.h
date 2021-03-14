#ifndef _COMPONENT_GROUP_H_
#define _COMPONENT_GROUP_H_

#include "nwm.h"
#include <vector>

class Client;
class Component;
class WindowManager;
class Graphics;

class ComponentGroup {
public:
	ComponentGroup(Display *display, Window window, int left, int top, int right, int bottom);
	virtual ~ComponentGroup();

	void Draw();
	void add(Component*);
	int getX();
	int getY();
	int getWidth();
	int getHeight();
protected:
	void resizeForComponent(Component *c);
private:
	int _left;
	int _top;
	int _right;
	int _bottom;
	Pixmap _pixmap;
	Graphics* _graphics;
	Client *_clientNode;
	Display *_display;
	Window _window;
	std::vector<Component*> _components;
};

#endif
