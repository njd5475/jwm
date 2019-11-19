/**
 * @file spacer.h
 * @author Joe Wingbermuehle
 * @date 2011
 *
 * @brief Spacer tray component.
 *
 */

#include "jwm.h"
#include "main.h"
#include "spacer.h"
#include "tray.h"

/** Create a spacer tray component. */
Spacer::Spacer(int width, int height) {
	if (JUNLIKELY(width < 0)) {
		width = 0;
	}
	if (JUNLIKELY(height < 0)) {
		height = 0;
	}

	this->requestNewSize(width, height);
	this->setPixmap(JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(), rootDepth));
	Tray::ClearTrayDrawable(this);
}

/** Set the size. */
void Spacer::SetSize(int width, int height) {
	if (width == 0) {
		this->width = this->getRequestedWidth();
		this->height = height;
	} else {
		this->width = width;
		this->height = this->requestedHeight;
	}
}

/** Resize. */
void Spacer::Resize() {
	TrayComponent::Resize();
	if (this->getPixmap() != None) {
		JXFreePixmap(display, this->getPixmap());
	}
	this->setPixmap(JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(), rootDepth));
	Tray::ClearTrayDrawable(this);
}

void Spacer::Create() {

}

/** Destroy. */
void Spacer::Destroy() {
	if (this->getPixmap() != None) {
		JXFreePixmap(display, this->getPixmap());
	}
}

