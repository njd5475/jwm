/*
 * TrayComponent.h
 *
 *  Created on: Nov 18, 2019
 *      Author: nick
 */

#ifndef SRC_TRAYCOMPONENT_H_
#define SRC_TRAYCOMPONENT_H_

#include "action.h"

class Graphics;

class ActionNode;
class Tray;

class TrayComponent {

private:
	/** The tray containing the component.
	 * UpdateSpecificTray(TrayType*, TrayComponentType*) should be called
	 * when content changes.
	 */
	Tray *tray;

	int x; /**< x-coordinate on the tray (valid only after Create). */
	int y; /**< y-coordinate on the tray (valid only after Create). */

	int screenx; /**< x-coordinate on the screen (valid only after Create). */
	int screeny; /**< y-coordinate on the screen (valid only after Create). */

	TrayComponent *parent;
protected:
	/** Additional information needed for the component. */
	//void *object;
	int requestedWidth; /**< Requested width. */
	int requestedHeight; /**< Requested height. */
	Pixmap pixmap; /**< Content (if a pixmap, otherwise None). */

	int width; /**< Actual width. */
	int height; /**< Actual height. */

	Window window; /**< Content (if a window, otherwise None). */

private:

	char grabbed; /**< 1 if the mouse was grabbed by this component. */

public:
	TrayComponent(Tray *tray, TrayComponent *parent);
	virtual ~TrayComponent();

	void SetParent(Tray *parent) {this->tray = parent;}
	void RefreshSize();
	int getX() const;
	int getY() const;
	int getHeight() const {return this->height;}
	int getWidth() const {return this->width;}
	int getScreenX() const {return this->screenx;}
	int getScreenY() const {return this->screeny;}
	int getRequestedWidth() const {return this->requestedWidth;}
	int getRequestedHeight() const {return this->requestedHeight;}
	Window getWindow() const {return this->window;}
	void requestNewSize(int width, int height) {
		this->requestedWidth = width;
		this->requestedHeight = height;
	}

	void SetLocation(int x, int y) {this->x=x;this->y=y;}
	void SetScreenLocation(int x, int y) {this->screenx=x;this->screeny=y;}
	Tray *getTray() const {return this->tray;}
	Pixmap getPixmap() const {return this->pixmap;}
	void setPixmap(Pixmap pixmap) {
		this->pixmap = pixmap;
	}

	/** Callback to destroy the component. */
	virtual void Destroy() {}

	/** Callback to set the size known so far.
	 * This is needed for items that maintain width/height ratios.
	 * Either width or height may be zero.
	 * This is called before Create.
	 */

public:

	/** Update a component on a tray.
	 * @param tp The tray containing the component.
	 * @param cp The component that needs updating.
	 */
	void UpdateSpecificTray(const Tray *tp);

	virtual void SetSize(int width, int height) {
		this->width = width;
		this->height = height;
	}

	/** Callback to resize the component. */
	virtual void Resize() {
		if(this->width != this->requestedWidth && this->requestedWidth != 0) {
			this->width = this->requestedWidth;
		}
		if(this->height != this->requestedHeight && this->requestedHeight != 0) {
			this->height = this->requestedHeight;
		}
		this->Destroy();
		this->Create();
	}

	virtual void Draw() {}

	virtual void Create() = 0;

	virtual void Draw(Graphics *g) = 0;

	/** Callback for mouse presses. */
	virtual void ProcessButtonPress(int x, int y, int mask) = 0;

	/** Callback for mouse releases. */
	virtual void ProcessButtonRelease(int x, int y, int mask) = 0;

	/** Callback for mouse motion. */
	virtual void ProcessMotionEvent(int x, int y, int mask) = 0;
	/** Callback to redraw the component contents.
	 * This is only needed for components that use actions.
	 */
	virtual void Redraw() {}

	void ungrab() {this->grabbed = 0;}
	void grab() {this->grabbed = 1;}
	char wasGrabbed() {return this->grabbed;}

	void addAction(const char *action, int mask);
	void validateActions();
	virtual void handleReleaseActions(int x, int y, int button);
	virtual void handlePressActions(int x, int y, int button);
private:
	std::vector<ActionNode*> actions;
};

#endif /* SRC_TRAYCOMPONENT_H_ */
