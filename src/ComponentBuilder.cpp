/*
 * ComponentBuilder.cpp
 *
 *  Created on: Aug 23, 2020
 *      Author: nick
 */

#include "ComponentBuilder.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <cstring>

#include "BaseComponent.h"
#include "color.h"
#include "Component.h"
#include "ComponentProperty.h"
#include "debug.h"
#include "font.h"
#include "Graphics.h"
#include "WindowManager.h"

class EmptyComponent: public Component {
public:
	EmptyComponent(Component *parent) :
			Component(parent), _properties(NULL) {
	}
	virtual ~EmptyComponent() {
	}

	virtual int getX() {
		return getParent()->getX();
	}
	virtual int getY() {
		return getParent()->getY();
	}
	virtual int getWidth() {
		return getParent()->getWidth();
	}
	virtual int getHeight() {
		return getParent()->getHeight();
	}
	virtual bool contains(int mouseX, int mouseY) {
		return mouseX >= getX() && mouseX < (getX() + getWidth())
				&& mouseY >= getY() && mouseY < getY() + getHeight();
	}
	virtual void Draw(Graphics *graphics) {
		getParent()->Draw(graphics);
	}
	virtual bool process(const XEvent *event) {
		if (this->getParent()) {
			this->getParent()->process(event);
		}
		if (event->type == MotionNotify) {
			int x = event->xmotion.x;
			int y = event->xmotion.y;
			if (contains(x, y)) {
				mouseMoved(x, y);
			}
		} else if (event->type == ButtonRelease) {
			int x = event->xmotion.x;
			int y = event->xmotion.y;
			if (contains(x, y)) {
				this->mouseReleased();
			}
		}
		return false;
	}
	virtual void mouseReleased() {
	}
	virtual void mouseMoved(int, int) {
	}
	virtual void initProperties(HashMap<ComponentProperty*> *properties) {
		_properties = properties;
		if (this->getParent()) {
			this->getParent()->initProperties(properties);
		}
	}
	virtual const char* getStringProp(const char *propName) {
		if (this->_properties->hasKey(propName)) {
			return (const char*) this->_properties->get(propName)->get();
		}
		return NULL;
	}
	virtual int getIntProp(const char *propName) {
		if (_properties->hasKey(propName)) {
			return *(int*) _properties->get(propName)->get();
		}
		return 0;
	}
protected:
	virtual void setIntProp(const char *propName, int propValue) {
		if (!_properties->hasKey(propName)) {
			ComponentProperty *nProp = new ComponentProperty();
			nProp->set(new int { propValue });
			(*_properties)[propName] = nProp;

			return;
		}

		ComponentProperty *prop = ((*_properties)[propName]).value();
		int *val = (int*) prop->get();
		(*val) = propValue;
	}
private:
	HashMap<ComponentProperty*> *_properties;
};

class Percentage: public EmptyComponent {
public:
	Percentage(Component *parent, float pX, float pY) :
			EmptyComponent(parent), _percentX(pX), _percentY(pY) {

	}
	virtual ~Percentage() {
	}
	virtual int getWidth() {
		return getParent()->getWidth() * _percentX;
	}
	virtual int getHeight() {
		return getParent()->getHeight() * _percentY;
	}
private:
	float _percentX;
	float _percentY;
};

class Label: public EmptyComponent {
public:
	Label(Component *parent, const char *text) :
			EmptyComponent(parent), _text(strdup(text)) {
	}
	virtual ~Label() {
		delete _text;
	}
	virtual int getWidth() {
		return Fonts::GetStringWidth(FONT_MENU, _text);
	}
	virtual int getHeight() {
		return Fonts::GetStringHeight(FONT_MENU);
	}
	virtual void Draw(Graphics *g) {
		getParent()->Draw(g);
		//int fg = = COLOR_MENU_BG;
		int bg = getIntProp("background");
		g->setForeground(bg);
		g->fillRectangle(getX(), getY(), getWidth(), getHeight());
		int fg = getIntProp("foreground");
		g->setForeground(fg);
		g->print(_text, getX(), getY(), getWidth());
	}
private:
	const char *_text;
};

class AlignBelow: public EmptyComponent {
public:
	AlignBelow(Component *parent, Component *below) :
			EmptyComponent(parent), _below(below) {
		Assert(_below);
	}
	virtual ~AlignBelow() {
	}
	virtual int getY() {
		return _below->getY() + _below->getHeight();
	}
private:
	Component *_below;
};

class Clicked: public EmptyComponent {
public:
	Clicked(Component *parent, ClickHandler *handler) :
			EmptyComponent(parent), _handler(handler) {

	}

	virtual ~Clicked() {
	}

	virtual bool process(const XEvent *event) {
		if (this->getParent()) {
			this->getParent()->process(event);
		}
		if (event->type == ButtonRelease && event->xbutton.button == Button1) {
			int x = event->xbutton.x;
			int y = event->xbutton.y;
			if (this->contains(x, y)) {
				_handler->click(event, this);
				return true;
			}
		}
		return false;
	}
private:
	ClickHandler *_handler;
};

class Hover: public EmptyComponent {
public:
	Hover(Component *parent, const int &color) :
			EmptyComponent(parent), _hoverColor(color), _hovering(false), _oldBkColor(
					color) {
	}

	virtual void initProperties(HashMap<ComponentProperty*> *properties) {
		EmptyComponent::initProperties(properties);
		if (properties->hasKey("background")) {
			_oldBkColor = *(int*) properties->get("background")->get();
		}
	}

//	virtual void Draw(Graphics *g) {
//
//		getParent()->Draw(g);
//		//int fg = = COLOR_MENU_BG;
//		int bg = getIntProp("background");
//		g->setForeground(bg);
//		g->fillRectangle(0, 0, getWidth(), getHeight());
//		int fg = getIntProp("foreground");
//		g->setForeground(fg);
//
//	}

	virtual void mouseMoved(int x, int y) {
		_hovering = this->contains(x, y);
	}

	virtual bool contains(int x, int y) {
		bool c = this->getParent()->contains(x, y);
		_hovering = c;
		if (_hovering) {
			setIntProp("background", _hoverColor);
		} else {
			setIntProp("background", _oldBkColor);
		}
		return c;
	}
private:
	int _hoverColor;
	bool _hovering;
	int _oldBkColor;
};

ComponentBuilder::ComponentBuilder() :
		_current(new BaseComponent()), _properties(
				new HashMap<ComponentProperty*>) {
// TODO Auto-generated constructor stub
}

ComponentBuilder::~ComponentBuilder() {
// TODO Auto-generated destructor stub
}

ComponentBuilder* ComponentBuilder::percentage(float percentX, float percentY) {
	this->_current = new Percentage(_current, percentX, percentY);
	return this;
}

ComponentBuilder* ComponentBuilder::label(const char *text) {
	ComponentProperty *fg = new ComponentProperty();
	int *defaultFg = new int;
	*defaultFg = COLOR_MENU_FG;
	fg->set(defaultFg);
	(*this->_properties)["foreground"] = fg;

	ComponentProperty *bg = new ComponentProperty();
	int *defaultBg = new int;
	*defaultBg = COLOR_CLOCK_BG1;
	bg->set(defaultBg);
	(*this->_properties)["background"] = bg;

	this->_current = new Label(_current, text);
	return this;
}

ComponentBuilder* ComponentBuilder::below(const char *name) {
	this->_current = new AlignBelow(_current, get(name));
	return this;
}

ComponentBuilder* ComponentBuilder::clicked(ClickHandler *handler) {
	this->_current = new Clicked(_current, handler);
	return this;
}

ComponentBuilder* ComponentBuilder::hover(const int &color) {
	this->_current = new Hover(_current, color);
	return this;
}

bool ComponentBuilder::saveAs(const char *name, Component *component) {
	if (!_components.hasKey(name)) {
		_components[strdup(name)] = component;
		return true;
	}
	return false;
}

Component* ComponentBuilder::get(const char *name) {
	if (_components.hasKey(name)) {
		return _components[name].value();
	}
	return NULL;
}

Component* ComponentBuilder::build(const char *name) {
	HashMap<ComponentProperty*> *props = _properties;
	_properties = new HashMap<ComponentProperty*>;

	_current->initProperties(props);

	WindowManager::WM()->add(_current);

	saveAs(name, _current);

	Component *added = _current;
	_current = new BaseComponent();

	return added;
}
