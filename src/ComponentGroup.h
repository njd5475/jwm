#ifndef _COMPONENT_GROUP_H_
#define _COMPONENT_GROUP_H_

#include "nwm.h"
#include <vector>

class ClientNode;
class Component;
class WindowManager;
class Graphics;

class ComponentGroup {
public:
	ComponentGroup(Display *display, Window window);
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
	ClientNode *_clientNode;
	Display *_display;
	Window _window;
	std::vector<Component*> _components;
};

#endif
