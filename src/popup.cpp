/**
 * @file popup.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for displaying popup windows.
 *
 */

#include "nwm.h"
#include "popup.h"
#include "main.h"
#include "color.h"
#include "font.h"
#include "screen.h"
#include "cursor.h"
#include "timing.h"
#include "misc.h"
#include "settings.h"
#include "event.h"
#include "hint.h"
#include "logger.h"

typedef struct PopupType {
	int x, y; /* The coordinates of the upper-left corner of the popup. */
	int mx, my; /* The mouse position when the popup was created. */
	Window mw;
	int width, height;
	char *text; /* The raw popup text. */
	char *lines; /* Popup text split into NUL-separated lines. */
	int lineCount; /* The number of lines. */
	Window window;
	Pixmap pmap;
} PopupType;

static PopupType popup;

static void MeasurePopupText();
static void SignalPopup(const TimeType *now, int x, int y, Window w,
		void *data);

PopupEventHandler Popups::handler;

/** Startup popups. */
void Popups::StartupPopup(void) {
	popup.text = NULL;
	popup.window = None;
	Events::_RegisterCallback(100, SignalPopup, NULL);
	Events::registerHandler(&handler);
}

/** Shutdown popups. */
void Popups::ShutdownPopup(void) {
  Events::_UnregisterCallback(SignalPopup, NULL);
	if (popup.text) {
		Release(popup.text);
		Release(popup.lines);
		popup.text = NULL;
	}
	if (popup.window != None) {
		JXDestroyWindow(display, popup.window);
		JXFreePixmap(display, popup.pmap);
		popup.window = None;
	}
}

/** Calculate dimensions of a popup window given the popup text. */
void MeasurePopupText() {
	const int textHeight = Fonts::GetStringHeight(FONT_POPUP) + 1;
	char *ptr;

	popup.lines = CopyString(popup.text);
	ptr = popup.lines;

	popup.width = 0;
	popup.height = 1;
	popup.lineCount = 0;
	for (;;) {
		char *end = strchr(ptr, '\n');
		int currentWidth;
		if (end) {
			*end = 0;
		}
		currentWidth = Fonts::GetStringWidth(FONT_POPUP, ptr) + 9;
		popup.width = Max(popup.width, currentWidth);
		popup.height += textHeight;
		popup.lineCount += 1;
		if (end) {
			ptr = end + 1;
		} else {
			break;
		}
	}
}

/** Show a popup window. */
void Popups::ShowPopup(int x, int y, const char *text,
		const PopupMaskType context) {
	Log("Should show popup\n");
	const ScreenType *sp;
	char *ptr;
	int textHeight;
	int i;

	if (!(settings.popupMask & context)) {
		return;
	}

	if (popup.text) {
		if (x == popup.x && y == popup.y && !strcmp(popup.text, text)) {
			/* This popup is already shown. */
			return;
		}
		Release(popup.text);
		Release(popup.lines);
		popup.text = NULL;
	}

	if (text[0] == 0) {
		return;
	}

	Cursors::GetMousePosition(&popup.mx, &popup.my, &popup.mw);
	popup.text = CopyString(text);

	MeasurePopupText();
	sp = Screens::GetCurrentScreen(x, y);
	if (popup.width > sp->width) {
		popup.width = sp->width;
	}

	popup.x = x;
	if (y + 2 * popup.height + 2 >= sp->height) {
		popup.y = y - popup.height - 2;
	} else {
		popup.y = y + Fonts::GetStringHeight(FONT_POPUP) + 2;
	}

	if (popup.width + popup.x > sp->x + sp->width) {
		popup.x = sp->x + sp->width - popup.width - 2;
	}
	if (popup.height + popup.y > sp->y + sp->height) {
		popup.y = sp->y + sp->height - popup.height - 2;
	}
	if (popup.x < 2) {
		popup.x = 2;
	}
	if (popup.y < 2) {
		popup.y = 2;
	}

	if (popup.window == None) {

		XSetWindowAttributes attr;
		unsigned long attrMask = 0;

		attrMask |= CWEventMask;
		attr.event_mask = ExposureMask | PointerMotionMask
				| PointerMotionHintMask;

		attrMask |= CWSaveUnder;
		attr.save_under = True;

		attrMask |= CWDontPropagate;
		attr.do_not_propagate_mask = PointerMotionMask | ButtonPressMask
				| ButtonReleaseMask;

		popup.window = JXCreateWindow(display, rootWindow, popup.x, popup.y,
				popup.width, popup.height, 0, CopyFromParent, InputOutput,
				CopyFromParent, attrMask, &attr);
		Hints::SetAtomAtom(popup.window, ATOM_NET_WM_WINDOW_TYPE,
				ATOM_NET_WM_WINDOW_TYPE_NOTIFICATION);
		JXMapRaised(display, popup.window);

	} else {

		JXMoveResizeWindow(display, popup.window, popup.x, popup.y, popup.width,
				popup.height);
		JXFreePixmap(display, popup.pmap);

	}

	popup.pmap = JXCreatePixmap(display, popup.window, popup.width,
			popup.height, rootDepth);

	JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_POPUP_BG));
	JXFillRectangle(display, popup.pmap, rootGC, 0, 0, popup.width - 1,
			popup.height - 1);
	JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_POPUP_OUTLINE));
	JXDrawRectangle(display, popup.pmap, rootGC, 0, 0, popup.width - 1,
			popup.height - 1);
	ptr = popup.lines;
	textHeight = Fonts::GetStringHeight(FONT_POPUP) + 1;
	for (i = 0; i < popup.lineCount; i++) {
		Fonts::RenderString(popup.pmap, FONT_POPUP, COLOR_POPUP_FG, 4,
				textHeight * i + 1, popup.width, ptr);
		ptr += strlen(ptr) + 1;
	}
	JXCopyArea(display, popup.pmap, popup.window, rootGC, 0, 0, popup.width,
			popup.height, 0, 0);

}

/** Signal popup (this is used to hide popups after awhile). */
void SignalPopup(const TimeType *now, int x, int y, Window w,
		void *data) {
	if (popup.window != None) {
		if (popup.mw != w || abs(popup.mx - x) > 0 || abs(popup.my - y) > 0) {
			JXDestroyWindow(display, popup.window);
			JXFreePixmap(display, popup.pmap);
			popup.window = None;
		}
	}
}

/** Process an event on a popup window. */
bool PopupEventHandler::process(const XEvent *event) {
	if (popup.window != None && event->xany.window == popup.window) {
		if (event->type == Expose && event->xexpose.count == 0) {
			JXCopyArea(display, popup.pmap, popup.window, rootGC, 0, 0,
					popup.width, popup.height, 0, 0);
		} else if (event->type == MotionNotify) {
			JXDestroyWindow(display, popup.window);
			JXFreePixmap(display, popup.pmap);
			popup.window = None;
		}
		return true;
	}
	return false;
}
