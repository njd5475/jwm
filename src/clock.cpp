/**
 * @file clock.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Clock tray component.
 *
 */

#include "jwm.h"
#include "clock.h"
#include "tray.h"
#include "color.h"
#include "font.h"
#include "timing.h"
#include "main.h"
#include "command.h"
#include "cursor.h"
#include "popup.h"
#include "misc.h"
#include "settings.h"
#include "event.h"
#include "action.h"

ClockType *ClockType::clocks = NULL;

/** Initialize clocks. */
void ClockType::InitializeClock(void) {
	clocks = NULL;
}

/** Start clock(s). */
void ClockType::StartupClock(void) {

}

/** Destroy clock(s). */
void ClockType::DestroyClock(void) {
	while (clocks) {
		ClockType *cp = clocks->next;

		if (clocks->format) {
			Release(clocks->format);
		}
		if (clocks->zone) {
			Release(clocks->zone);
		}
		_UnregisterCallback(SignalClock, clocks);

		Release(clocks);
		clocks = cp;
	}
}

const char *ClockType::DEFAULT_FORMAT = "%I:%M %p";

/** Create a clock tray component. */
ClockType::ClockType(const char *format, const char *zone, int width, int height, Tray *tray, TrayComponent *parent) :
		TrayComponent(tray, parent) {
	this->next = clocks;
	clocks = this;

	this->mousex = -settings.doubleClickDelta;
	this->mousey = -settings.doubleClickDelta;
	this->mouseTime.seconds = 0;
	this->mouseTime.ms = 0;
	this->userWidth = 0;
	this->SetScreenLocation(10, 10);

	if (!format) {
		format = DEFAULT_FORMAT;
	}
	this->format = CopyString(format);
	this->zone = CopyString(zone);
	memset(&this->lastTime, 0, sizeof(this->lastTime));

	if (width > 0) {
		this->requestedWidth = width;
		this->userWidth = 1;
	} else {
		this->requestedWidth = 0;
		this->userWidth = 0;
	}
	this->requestedHeight = height;

	_RegisterCallback(Min(900, settings.popupDelay / 2), SignalClock, this);
}

/** Initialize a clock tray component. */
void ClockType::Create() {
	this->setPixmap(JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(), rootDepth));
}

/** Resize a clock tray component. */
void ClockType::Resize() {
	TrayComponent::Resize();
	ClockType *clk;
	TimeType now;

	Assert(cp);

	clk = (ClockType*) this;

	Assert(clk);

	if (this->getPixmap() != None) {
		JXFreePixmap(display, this->getPixmap());
	}

	this->setPixmap(JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(), rootDepth));

	memset(&clk->lastTime, 0, sizeof(clk->lastTime));

	GetCurrentTime(&now);
	clk->DrawClock(&now);

}

/** Destroy a clock tray component. */
void ClockType::Destroy() {
	Assert(cp);
	if (this->getPixmap() != None) {
		JXFreePixmap(display, this->getPixmap());
	}
}

/** Process a press event on a clock tray component. */
void ClockType::ProcessButtonPress(int x, int y, int button) {
	this->handlePressActions(x, y, button);
}

void ClockType::ProcessButtonRelease(int x, int y, int button) {
	this->handleReleaseActions(x, y, button);
}

/** Process a motion event on a clock tray component. */
void ClockType::ProcessMotionEvent(int x, int y, int mask) {
	ClockType *clk = (ClockType*) this;
	clk->mousex = this->getScreenX() + x;
	clk->mousey = this->getScreenY() + y;
	GetCurrentTime(&clk->mouseTime);
}

/** Update a clock tray component. */
void ClockType::SignalClock(const TimeType *now, int x, int y, Window w, void *data) {
	const char *longTime;
	ClockType *clk = (ClockType*) data;

	clk->DrawClock(now);
	if (clk->getTray()->getWindow() == w && abs(clk->mousex - x) < settings.doubleClickDelta
			&& abs(clk->mousey - y) < settings.doubleClickDelta) {
		if (GetTimeDifference(now, &clk->mouseTime) >= settings.popupDelay) {
			longTime = GetTimeString("%c", clk->zone);
			Popups::ShowPopup(x, y, longTime, POPUP_CLOCK);
		}
	}

}

void ClockType::Draw(Graphics *g) {

}

/** Draw a clock tray component. */
void ClockType::DrawClock(const TimeType *now) {
	const char *timeString;
	int width;
	int rwidth;

	/* Only draw if the time changed. */
	if (now->seconds == this->lastTime.seconds) {
		return;
	}

	/* Clear the area. */
	if (Colors::lookupColor(COLOR_CLOCK_BG1) == Colors::lookupColor(COLOR_CLOCK_BG2)) {
		JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_CLOCK_BG1));
		JXFillRectangle(display, this->getPixmap(), rootGC, 0, 0, this->getWidth(), this->getHeight());
	} else {
		DrawHorizontalGradient(this->getPixmap(), rootGC, Colors::lookupColor(COLOR_CLOCK_BG1),
				Colors::lookupColor(COLOR_CLOCK_BG2), 0, 0, this->getWidth(), this->getHeight());
	}

	/* Determine if the clock is the right size. */
	timeString = GetTimeString(this->format, this->zone);
	width = Fonts::GetStringWidth(FONT_CLOCK, timeString);
	rwidth = width + 4;
	if (rwidth == this->getRequestedWidth() || this->userWidth) {

		/* Draw the clock. */
		Fonts::RenderString(this->getPixmap(), FONT_CLOCK, COLOR_CLOCK_FG, (this->getWidth() - width) / 2,
				(this->getHeight() - Fonts::GetStringHeight(FONT_CLOCK)) / 2, this->getWidth(), timeString);

		this->UpdateSpecificTray(this->getTray());

	} else {

		/* Wrong size. Resize. */
		this->requestNewSize(rwidth, this->getHeight());
		this->getTray()->ResizeTray();

	}

}
