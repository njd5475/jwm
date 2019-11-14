/**
 * @file traybutton.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Button tray component.
 *
 */

#include "jwm.h"
#include "traybutton.h"
#include "tray.h"
#include "icon.h"
#include "image.h"
#include "error.h"
#include "root.h"
#include "main.h"
#include "font.h"
#include "button.h"
#include "misc.h"
#include "screen.h"
#include "popup.h"
#include "timing.h"
#include "command.h"
#include "cursor.h"
#include "settings.h"
#include "event.h"
#include "action.h"

TrayButton *TrayButton::buttons = NULL;

/** Startup tray buttons. */
void TrayButton::StartupTrayButtons(void) {
	TrayButton *bp;
	for (bp = buttons; bp; bp = bp->next) {
		if (bp->label) {
			bp->requestNewSize(Fonts::GetStringWidth(FONT_TRAY, bp->label) + 4, Fonts::GetStringHeight(FONT_TRAY));
		} else {
			bp->requestNewSize(0, 0);
		}
		if (bp->iconName) {
			bp->icon = Icons::LoadNamedIcon(bp->iconName, 1, 1);
			if (JLIKELY(bp->icon)) {
				int rWidth = 0, rHeight = 0;
				rWidth = bp->getRequestedWidth() + bp->icon->width + 4;
				if (bp->label) {
					rWidth = rWidth - 2;
				}
				rHeight = Max(bp->icon->height + 4, bp->getRequestedHeight());
				bp->requestNewSize(rWidth, rHeight);
			} else {
				Warning(_("could not load tray icon: \"%s\""), bp->iconName);
			}
		}
	}
}

/** Release tray button data. */
void TrayButton::DestroyTrayButtons(void) {
	TrayButton *bp;
	while (buttons) {
		bp = buttons->next;
		_UnregisterCallback(SignalTrayButton, buttons);
		if (buttons->label) {
			Release(buttons->label);
		}
		if (buttons->iconName) {
			Release(buttons->iconName);
		}
		if (buttons->popup) {
			Release(buttons->popup);
		}
		Release(buttons);
		buttons = bp;
	}
}

/** Create a button tray component. */
TrayButton::TrayButton(const char *iconName, const char *label, const char *popup, unsigned int width,
		unsigned int height) {

	if (JUNLIKELY((label == NULL || strlen(label) == 0) && (iconName == NULL || strlen(iconName) == 0))) {
		Warning(_("no icon or label for TrayButton"));
		return;
	}

	this->next = buttons;
	buttons = this;

	this->icon = NULL;
	this->iconName = CopyString(iconName);
	this->label = CopyString(label);
	this->popup = CopyString(popup);

	//TODO: bp is a subclass of TrayComponentType cp->getObject() = bp;
	this->requestNewSize(width, height);

	this->mousex = -settings.doubleClickDelta;
	this->mousey = -settings.doubleClickDelta;

	_RegisterCallback(settings.popupDelay / 2, SignalTrayButton, this);

	this->Create();
}

/** Set the size of a button tray component. */
void TrayButton::SetSize(int width, int height) {
	TrayButton *bp;
	int labelWidth, labelHeight;
	int iconWidth, iconHeight;

	bp = (TrayButton*) this;

	if (bp->label) {
		labelWidth = Fonts::GetStringWidth(FONT_TRAY, bp->label) + 6;
		labelHeight = Fonts::GetStringHeight(FONT_TRAY) + 6;
	} else {
		labelWidth = 4;
		labelHeight = 4;
	}

	if (bp->icon && bp->icon->width && bp->icon->height) {
		/* With an icon. */
		int ratio;

		iconWidth = bp->icon->width;
		iconHeight = bp->icon->height;

		/* Fixed point with 16 bit fraction. */
		ratio = (iconWidth << 16) / iconHeight;

		if (width > 0) {

			/* Compute height from width. */
			iconWidth = width - labelWidth;
			iconHeight = (iconWidth << 16) / ratio;
			height = Max(labelHeight, iconHeight + 4);

		} else if (height > 0) {

			/* Compute width from height. */
			iconHeight = height - 4;
			iconWidth = (iconHeight * ratio) >> 16;
			width = iconWidth + labelWidth;

		} else {

			/* Use best size. */
			height = Max(labelHeight, iconHeight + 4);
			iconWidth = ((height - 4) * ratio) >> 16;
			width = iconWidth + labelWidth;

		}

	} else {
		/* No icon. */
		if (width > 0) {
			height = labelHeight;
		} else if (height > 0) {
			width = labelWidth;
		} else {
			height = labelHeight;
			width = labelWidth;
		}
	}

	this->width = width;
	this->height = height;

}

/** Initialize a button tray component. */
void TrayButton::Create() {
	this->setPixmap(JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(), rootDepth));
}

/** Resize a button tray component. */
void TrayButton::Resize() {
	Destroy();
	Create();
}

/** Destroy a button tray component. */
void TrayButton::Destroy() {
	if (this->getPixmap() != None) {
		JXFreePixmap(display, this->getPixmap());
	}
}

/** Draw a tray button. */
void TrayButton::Draw() {
	TrayComponentType *cp = this;
	ButtonNode button;
	TrayButton *bp;

	bp = (TrayButton*) cp;

	TrayType::ClearTrayDrawable(cp);
	ResetButton(&button, cp->getPixmap());
	if (cp->wasGrabbed()) {
		button.type = BUTTON_TRAY_ACTIVE;
	} else {
		button.type = BUTTON_TRAY;
	}
	button.width = cp->getWidth();
	button.height = cp->getHeight();
	button.border = settings.trayDecorations == DECO_MOTIF;
	button.x = 0;
	button.y = 0;
	button.font = FONT_TRAY;
	button.text = bp->label;
	button.icon = bp->icon;
	DrawButton(&button);

}

/** Process a motion event. */
void TrayButton::ProcessMotionEvent(int x, int y, int mask) {
	this->mousex = this->getScreenX() + x;
	this->mousey = this->getScreenY() + y;
	GetCurrentTime(&this->mouseTime);
}

/** Signal (needed for popups). */
void TrayButton::SignalTrayButton(const TimeType *now, int x, int y, Window w, void *data) {
	TrayButton *bp = (TrayButton*) data;
	const char *popup;

	if (bp->popup) {
		popup = bp->popup;
	} else if (bp->label) {
		popup = bp->label;
	} else {
		return;
	}
	if (bp->getTray()->getWindow() == w && abs(bp->mousex - x) < settings.doubleClickDelta
			&& abs(bp->mousey - y) < settings.doubleClickDelta) {
		if (GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
			Popups::ShowPopup(x, y, popup, POPUP_BUTTON);
		}
	}
}
