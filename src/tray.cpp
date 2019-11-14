/**
 * @file tray.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Tray functions.
 *
 */

#include "jwm.h"
#include "tray.h"
#include "color.h"
#include "main.h"
#include "pager.h"
#include "cursor.h"
#include "error.h"
#include "taskbar.h"
#include "menu.h"
#include "screen.h"
#include "settings.h"
#include "event.h"
#include "client.h"
#include "misc.h"
#include "hint.h"
#include "action.h"

#define DEFAULT_TRAY_WIDTH 32
#define DEFAULT_TRAY_HEIGHT 32

#define TRAY_BORDER_SIZE   1

unsigned int TrayType::trayCount = 0;

/** Initialize tray data. */
void TrayType::InitializeTray(void) {
	trays = NULL;
	trayCount = 0;
}

/** Startup trays. */
void TrayType::StartupTray(void) {
	XSetWindowAttributes attr;
	unsigned long attrMask;
	TrayType *tp;
	TrayComponentType *cp;
	int variableSize;
	int variableRemainder;
	int width, height;
	int xoffset, yoffset;

	for (tp = trays; tp; tp = tp->next) {

		tp->LayoutTray(&variableSize, &variableRemainder);

		/* Create the tray window. */
		/* The window is created larger for a border. */
		attrMask = CWOverrideRedirect;
		attr.override_redirect = True;

		/* We can't use PointerMotionHintMask since the exact position
		 * of the mouse on the tray is important for popups. */
		attrMask |= CWEventMask;
		attr.event_mask = ButtonPressMask | ButtonReleaseMask | SubstructureNotifyMask | ExposureMask | KeyPressMask
				| KeyReleaseMask | EnterWindowMask | PointerMotionMask;

		attrMask |= CWBackPixel;
		attr.background_pixel = Colors::colors[COLOR_TRAY_BG2];

		Assert(tp->getWidth() > 0);
		Assert(tp->getHeight() > 0);
		tp->window = JXCreateWindow(display, rootWindow, tp->x, tp->y, tp->width, tp->height, 0, rootDepth, InputOutput,
				rootVisual, attrMask, &attr);
		Hints::SetAtomAtom(tp->window, ATOM_NET_WM_WINDOW_TYPE, ATOM_NET_WM_WINDOW_TYPE_DOCK);

		if (settings.trayOpacity < UINT_MAX) {
			Hints::SetCardinalAtom(tp->window, ATOM_NET_WM_WINDOW_OPACITY, settings.trayOpacity);
		}

		Cursors::SetDefaultCursor(tp->window);

		/* Create and layout items on the tray. */
		xoffset = TRAY_BORDER_SIZE;
		yoffset = TRAY_BORDER_SIZE;
		for (cp = tp->components; cp; cp = cp->getNext()) {

			if (tp->layout == LAYOUT_HORIZONTAL) {
				height = tp->height - TRAY_BORDER_SIZE * 2;
				width = cp->getWidth();
				if (width == 0) {
					width = variableSize;
					if (variableRemainder) {
						width += 1;
						variableRemainder -= 1;
					}
				}
			} else {
				width = tp->width - TRAY_BORDER_SIZE * 2;
				height = cp->getHeight();
				if (height == 0) {
					height = variableSize;
					if (variableRemainder) {
						height += 1;
						variableRemainder -= 1;
					}
				}
			}
			cp->SetSize(Max(1, width), Max(1, height));
			cp->SetLocation(xoffset, yoffset);
			cp->SetScreenLocation(tp->x + xoffset, tp->y + yoffset);

			if (cp->getWindow() != None) {
				JXReparentWindow(display, cp->getWindow(), tp->window, xoffset, yoffset);
			}

			if (tp->layout == LAYOUT_HORIZONTAL) {
				xoffset += cp->getWidth();
			} else {
				yoffset += cp->getHeight();
			}
		}

		/* Show the tray. */
		JXMapWindow(display, tp->window);

		trayCount += 1;

	}

	_RequirePagerUpdate();
	_RequireTaskUpdate();
}

/** Shutdown trays. */
void TrayType::ShutdownTray(void) {
	TrayType *tp;
	TrayComponentType *cp;

	for (tp = trays; tp; tp = tp->next) {
		for (cp = tp->components; cp; cp = cp->getNext()) {
			cp->Destroy();
		}
		JXDestroyWindow(display, tp->window);
	}
}

/** Destroy tray data. */
void TrayType::DestroyTray(void) {
	TrayType *tp;
	TrayComponentType *cp;

	while (trays) {
		tp = trays->next;
		if (trays->autoHide != THIDE_OFF) {
			_UnregisterCallback(SignalTray, trays);
		}
		while (trays->components) {
			cp = trays->components->getNext();
			Release(trays->components);
			trays->components = cp;
		}
		Release(trays);

		trays = tp;
	}
}

/** Create an empty tray. */
TrayType::TrayType() {
	this->requestedX = 0;
	this->requestedY = -1;
	this->x = 0;
	this->y = -1;
	this->requestedWidth = 0;
	this->requestedHeight = 0;
	this->width = 0;
	this->height = 0;
	this->layer = DEFAULT_TRAY_LAYER;
	this->layout = LAYOUT_HORIZONTAL;
	this->valign = TALIGN_FIXED;
	this->halign = TALIGN_FIXED;

	GetCurrentTime(&this->showTime);
	this->autoHide = THIDE_OFF;
	this->autoHideDelay = 0;
	this->hidden = 0;

	this->window = None;

	this->components = NULL;
	this->componentsTail = NULL;

	this->next = trays;
	trays = this;
}

TrayType::~TrayType() {

}

void TrayComponentType::addAction(const char *action, int mask) {
	ActionNode *ap = new ActionNode(action, mask);

	/* Make sure we actually have an action. */
	if (action == NULL || action[0] == 0 || mask == 0) {
		/* Valid (root menu 1). */
	} else if (!strncmp(action, "exec:", 5)) {
		/* Valid. */
	} else if (!strncmp(action, "root:", 5)) {
		/* Valid. However, the specified root menu may not exist.
		 * This case is handled in ValidateTrayButtons.
		 */
	} else if (!strcmp(action, "showdesktop")) {
		/* Valid. */
	} else {
		/* Invalid; don't add the action. */
		Warning(_("invalid action: \"%s\""), action);
		return;
	}
	this->actions.push_back(ap);
}

void TrayComponentType::validateActions() {
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		(*it)->ValidateAction();
	}
}

void TrayComponentType::handleReleaseActions(int x, int y, int button) {
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		(*it)->ProcessActionRelease(this, x, y, button);
	}
}

void TrayComponentType::handlePressActions(int x, int y, int button) {
	Log("Tray component handling press action\n");
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		(*it)->ProcessActionPress(this, x, y, button);
	}
}

/** Create an empty tray component. */
TrayComponentType::TrayComponentType() :
		screenx(0), screeny(0) {

	this->tray = NULL;

	this->x = 0;
	this->y = 0;
	this->requestedWidth = 0;
	this->requestedHeight = 0;
	this->width = 0;
	this->height = 0;
	this->grabbed = 0;

	this->window = None;
	this->pixmap = None;

	this->next = NULL;
}

TrayComponentType::~TrayComponentType() {
	std::vector<ActionNode*>::iterator it;
	for (it = this->actions.begin(); it != this->actions.end(); ++it) {
		delete *it;
	}
	this->actions.clear();
}

/** Add a tray component to a tray. */
void TrayType::AddTrayComponent(TrayComponentType *cp) {
	cp->SetParent(this);
	if (this->componentsTail) {
		this->componentsTail->SetNext(cp);
	} else {
		this->components = cp;
	}
	this->componentsTail = cp;
	cp->SetNext(NULL);
}

/** Compute the max component width. */
int TrayType::ComputeMaxWidth() {
	TrayComponentType *cp;
	int result;
	int temp;

	result = 0;
	for (cp = this->components; cp; cp = cp->getNext()) {
		temp = cp->getWidth();
		if (temp > 0 && temp > result) {
			result = temp;
		}
	}

	return result;
}

/** Compute the total width of a tray. */
int TrayType::ComputeTotalWidth() {
	TrayComponentType *cp;
	int result;

	result = 2 * TRAY_BORDER_SIZE;
	for (cp = this->components; cp; cp = cp->getNext()) {
		result += cp->getWidth();
	}

	return result;
}

/** Compute the max component height. */
int TrayType::ComputeMaxHeight() {
	TrayComponentType *cp;
	int result;
	int temp;

	result = 0;
	for (cp = this->components; cp; cp = cp->getNext()) {
		temp = cp->getHeight();
		if (temp > 0 && temp > result) {
			result = temp;
		}
	}

	return result;
}

/** Compute the total height of a tray. */
int TrayType::ComputeTotalHeight() {
	TrayComponentType *cp;
	int result;

	result = 2 * TRAY_BORDER_SIZE;
	for (cp = this->components; cp; cp = cp->getNext()) {
		result += cp->getHeight();
	}

	return result;
}

/** Check if the tray fills the screen horizontally. */
char TrayType::CheckHorizontalFill() {
	TrayComponentType *cp;

	for (cp = this->components; cp; cp = cp->getNext()) {
		if (cp->getWidth() == 0) {
			return 1;
		}
	}

	return 0;
}

/** Check if the tray fills the screen vertically. */
char TrayType::CheckVerticalFill() {
	TrayComponentType *cp;

	for (cp = this->components; cp; cp = cp->getNext()) {
		if (cp->getHeight() == 0) {
			return 1;
		}
	}

	return 0;
}

/** Compute the size of a tray. */
void TrayType::ComputeTraySize() {
	TrayComponentType *cp;
	const ScreenType *sp;
	int x, y;

	/* Determine the first dimension. */
	if (this->layout == LAYOUT_HORIZONTAL) {

		if (this->height == 0) {
			this->height = this->ComputeMaxHeight() + TRAY_BORDER_SIZE * 2;
		}
		if (this->height == 0) {
			this->height = DEFAULT_TRAY_HEIGHT;
		}

	} else {

		if (this->width == 0) {
			this->width = this->ComputeMaxWidth() + TRAY_BORDER_SIZE * 2;
		}
		if (this->width == 0) {
			this->width = DEFAULT_TRAY_WIDTH;
		}

	}

	/* Now at least one size is known. Inform the components. */
	for (cp = this->components; cp; cp = cp->getNext()) {
		if (this->layout == LAYOUT_HORIZONTAL) {
			cp->SetSize(0, this->height - TRAY_BORDER_SIZE * 2);
		} else {
			cp->SetSize(this->width - TRAY_BORDER_SIZE * 2, 0);
		}
	}

	/* Initialize the coordinates. */
	this->x = this->requestedX;
	this->y = this->requestedY;

	/* Determine on which screen the tray will reside. */
	switch (this->valign) {
	case TALIGN_TOP:
		y = 0;
		break;
	case TALIGN_BOTTOM:
		y = rootHeight - 1;
		break;
	case TALIGN_CENTER:
		y = 1 + rootHeight / 2;
		break;
	default:
		if (this->y < 0) {
			y = rootHeight + this->y;
		} else {
			y = this->y;
		}
		break;
	}
	switch (this->halign) {
	case TALIGN_LEFT:
		x = 0;
		break;
	case TALIGN_RIGHT:
		x = rootWidth - 1;
		break;
	case TALIGN_CENTER:
		x = 1 + rootWidth / 2;
		break;
	default:
		if (this->x < 0) {
			x = rootWidth + this->x;
		} else {
			x = this->x;
		}
		break;
	}
	sp = Screens::GetCurrentScreen(x, y);

	/* Determine the missing dimension. */
	if (this->layout == LAYOUT_HORIZONTAL) {
		if (this->width == 0) {
			if (this->CheckHorizontalFill()) {
				this->width = sp->width + sp->x - x;
			} else {
				this->width = this->ComputeTotalWidth();
			}
			if (this->width == 0) {
				this->width = DEFAULT_TRAY_WIDTH;
			}
		}
	} else {
		if (this->height == 0) {
			if (this->CheckVerticalFill()) {
				this->height = sp->height + sp->y - y;
			} else {
				this->height = this->ComputeTotalHeight();
			}
			if (this->height == 0) {
				this->height = DEFAULT_TRAY_HEIGHT;
			}
		}
	}

	/* Compute the tray location. */
	switch (this->valign) {
	case TALIGN_TOP:
		this->y = sp->y;
		break;
	case TALIGN_BOTTOM:
		this->y = sp->y + sp->height - this->height;
		break;
	case TALIGN_CENTER:
		this->y = sp->y + (sp->height - this->height) / 2;
		break;
	default:
		if (this->y < 0) {
			this->y = sp->y + sp->height - this->height + this->y + 1;
		}
		break;
	}

	switch (this->halign) {
	case TALIGN_LEFT:
		this->x = sp->x;
		break;
	case TALIGN_RIGHT:
		this->x = sp->x + sp->width - this->width;
		break;
	case TALIGN_CENTER:
		this->x = sp->x + (sp->width - this->width) / 2;
		break;
	default:
		if (this->x < 0) {
			this->x = sp->x + sp->width - this->width + this->x + 1;
		}
		break;
	}
}

/** Display a tray (for autohide). */
void TrayType::ShowTray() {
	Window win1, win2;
	int winx, winy;
	unsigned int mask;
	int mousex, mousey;

	if (this->hidden) {

		this->hidden = 0;
		GetCurrentTime(&this->showTime);
		JXMoveWindow(display, this->window, this->x, this->y);

		JXQueryPointer(display, rootWindow, &win1, &win2, &mousex, &mousey, &winx, &winy, &mask);
		Cursors::SetMousePosition(mousex, mousey, win2);

	}
}

/** Show all trays. */
void TrayType::ShowAllTrays(void) {
	TrayType *tp;

	if (shouldExit) {
		return;
	}

	for (tp = trays; tp; tp = tp->next) {
		tp->ShowTray();
	}
}

/** Hide a tray (for autohide). */
void TrayType::HideTray() {
	const ScreenType *sp;
	LayoutType layout;
	int x, y;

	/* Don't hide if the tray is raised. */
	if (this->autoHide & THIDE_RAISED) {
		return;
	}

	this->hidden = 1;

	/* Derive the location for hiding the tray. */
	sp = Screens::GetCurrentScreen(this->x, this->y);
	layout = this->autoHide;
	if (layout == THIDE_ON) {
		if (this->layout == LAYOUT_HORIZONTAL) {
			if (this->y >= sp->height / 2) {
				layout = THIDE_BOTTOM;
			} else {
				layout = THIDE_TOP;
			}
		} else {
			if (this->x >= sp->width / 2) {
				layout = THIDE_RIGHT;
			} else {
				layout = THIDE_LEFT;
			}
		}
	}

	/* Determine where to move the tray. */
	switch (layout) {
	case THIDE_LEFT:
		x = sp->y - this->width + 1;
		y = this->y;
		break;
	case THIDE_RIGHT:
		x = sp->y + sp->width - 1;
		y = this->y;
		break;
	case THIDE_TOP:
		x = this->x;
		y = sp->y - this->height + 1;
		break;
	case THIDE_BOTTOM:
		x = this->x;
		y = sp->y + sp->height - 1;
		break;
	case THIDE_INVISIBLE:
		/* Off the top of the screen. */
		x = this->x;
		y = 0 - this->height - 1;
		break;
	default:
		Assert(0);
		return;
	}

	/* Move and redraw. */
	JXMoveWindow(display, this->window, x, y);
	this->DrawSpecificTray();
}

/** Process a tray event. */
char TrayType::ProcessTrayEvent(const XEvent *event) {
	TrayType *tp;

	for (tp = trays; tp; tp = tp->next) {
		if (event->xany.window == tp->window) {
			switch (event->type) {
			case Expose:
				HandleTrayExpose(tp, &event->xexpose);
				return 1;
			case EnterNotify:
				HandleTrayEnterNotify(tp, &event->xcrossing);
				return 1;
			case ButtonPress:
				HandleTrayButtonPress(tp, &event->xbutton);
				return 1;
			case ButtonRelease:
				HandleTrayButtonRelease(tp, &event->xbutton);
				return 1;
			case MotionNotify:
				HandleTrayMotionNotify(tp, &event->xmotion);
				return 1;
			default:
				return 0;
			}
		}
	}

	return 0;
}

/** Signal the tray (needed for autohide). */
void TrayType::SignalTray(const TimeType *now, int x, int y, Window w, void *data) {
	TrayType *tp = (TrayType*) data;
	Assert(tp->autoHide != THIDE_OFF);
	if (tp->hidden || menuShown) {
		return;
	}

	if (x < tp->x || x >= tp->x + tp->width || y < tp->y || y >= tp->y + tp->height) {
		if (GetTimeDifference(now, &tp->showTime) >= tp->autoHideDelay) {
			tp->HideTray();
		}
	} else {
		tp->showTime = *now;
	}
}

/** Handle a tray expose event. */
void TrayType::HandleTrayExpose(TrayType *tp, const XExposeEvent *event) {
	tp->DrawSpecificTray();
}

/** Handle a tray enter notify (for autohide). */
void TrayType::HandleTrayEnterNotify(TrayType *tp, const XCrossingEvent *event) {
	tp->ShowTray();
}

/** Get the tray component under the given coordinates. */
TrayComponentType* TrayType::GetTrayComponent(TrayType *tp, int x, int y) {
	TrayComponentType *cp;
	int xoffset, yoffset;

	xoffset = 0;
	yoffset = 0;
	for (cp = tp->components; cp; cp = cp->getNext()) {
		const int startx = xoffset;
		const int starty = yoffset;
		const int width = cp->getWidth();
		const int height = cp->getHeight();
		if (x >= startx && x < startx + width) {
			if (y >= starty && y < starty + height) {
				return cp;
			}
		}
		if (tp->layout == LAYOUT_HORIZONTAL) {
			xoffset += width;
		} else {
			yoffset += height;
		}
	}

	return NULL;
}

/** Handle a button press on a tray. */
void TrayType::HandleTrayButtonPress(TrayType *tp, const XButtonEvent *event) {
	TrayComponentType *cp = GetTrayComponent(tp, event->x, event->y);
	if (cp) {
		const int x = event->x - cp->getX();
		const int y = event->y - cp->getY();
		const int mask = event->button;
		_DiscardButtonEvents();
		cp->ProcessButtonPress(x, y, mask);
	} else {
		Log("Could not find a component at location\n");
	}
}

/** Handle a button release on a tray. */
void TrayType::HandleTrayButtonRelease(TrayType *tp, const XButtonEvent *event) {
	TrayComponentType *cp;

	// First inform any components that have a grab.
	for (cp = tp->components; cp; cp = cp->getNext()) {
		if (cp->wasGrabbed()) {
			const int x = event->x - cp->getX();
			const int y = event->y - cp->getY();
			const int mask = event->button;
			cp->ProcessButtonRelease(x, y, mask);
			JXUngrabPointer(display, CurrentTime);
			cp->ungrab();
			return;
		}
	}

	cp = GetTrayComponent(tp, event->x, event->y);
	if (cp) {
		const int x = event->x - cp->getX();
		const int y = event->y - cp->getY();
		const int mask = event->button;
		cp->ProcessButtonRelease(x, y, mask);
	}
}

/** Handle a motion notify event. */
void TrayType::HandleTrayMotionNotify(TrayType *tp, const XMotionEvent *event) {
	TrayComponentType *cp = GetTrayComponent(tp, event->x, event->y);
	if (cp) {
		const int x = event->x - cp->getX();
		const int y = event->y - cp->getY();
		const int mask = event->state;
		cp->ProcessMotionEvent(x, y, mask);
	}
}

/** Draw all trays. */
void TrayType::DrawTray(void) {
	TrayType *tp;

	if (shouldExit) {
		return;
	}

	for (tp = trays; tp; tp = tp->next) {
		tp->DrawSpecificTray();
	}
}

/** Draw a specific tray. */
void TrayType::DrawSpecificTray() {
	TrayComponentType *cp;

	for (cp = this->components; cp; cp = cp->getNext()) {
		cp->Resize();
		cp->Draw();
		cp->UpdateSpecificTray(this);
	}

	if (settings.trayDecorations == DECO_MOTIF) {
		JXSetForeground(display, rootGC, Colors::colors[COLOR_TRAY_UP]);
		JXDrawLine(display, this->window, rootGC, 0, 0, this->width - 1, 0);
		JXDrawLine(display, this->window, rootGC, 0, this->height - 1, 0, 0);

		JXSetForeground(display, rootGC, Colors::colors[COLOR_TRAY_DOWN]);
		JXDrawLine(display, this->window, rootGC, 0, this->height - 1, this->width - 1, this->height - 1);
		JXDrawLine(display, this->window, rootGC, this->width - 1, 0, this->width - 1, this->height - 1);
	} else {
		JXSetForeground(display, rootGC, Colors::colors[COLOR_TRAY_DOWN]);
		JXDrawRectangle(display, this->window, rootGC, 0, 0, this->width - 1, this->height - 1);
	}
}

/** Raise tray windows. */
void TrayType::RaiseTrays(void) {
	TrayType *tp;
	for (tp = trays; tp; tp = tp->getNext()) {
		tp->autoHide |= THIDE_RAISED;
		tp->ShowTray();
		JXRaiseWindow(display, tp->window);
	}
}

TrayType *TrayType::trays = NULL;

/** Lower tray windows. */
void TrayType::LowerTrays(void) {
	TrayType *tp;
	for (tp = TrayType::trays; tp; tp = tp->getNext()) {
		tp->autoHide &= ~THIDE_RAISED;
	}
	_RequireRestack();
}

/** Update a specific component on a tray. */
void TrayComponentType::UpdateSpecificTray(const TrayType *tp) {
	if (JUNLIKELY(shouldExit)) {
		return;
	}

	/* If the tray is hidden, draw only the background. */
	if (this->pixmap != None) {
		JXCopyArea(display, this->pixmap, this->window, rootGC, 0, 0, this->getWidth(), this->getHeight(), this->x,
				this->y);
	}
}

void TrayComponentType::RefreshSize() {
	if (this->requestedWidth != 0) {
		this->width = this->requestedWidth;
	} else {
		this->width = 0;
	}
	if (this->requestedHeight != 0) {
		this->height = this->requestedHeight;
	} else {
		this->height = 0;
	}
}

/** Layout tray components on a tray. */
void TrayType::LayoutTray(int *variableSize, int *variableRemainder) {
	TrayComponentType *cp;
	unsigned int variableCount;
	int width, height;
	int temp;

	if (this->requestedWidth >= 0) {
		this->width = this->requestedWidth;
	} else {
		this->width = rootWidth + this->requestedWidth - this->x;
	}
	if (this->requestedHeight >= 0) {
		this->height = this->requestedHeight;
	} else {
		this->height = rootHeight + this->requestedHeight - this->y;
	}

	for (cp = this->components; cp; cp = cp->getNext()) {
		cp->RefreshSize();
	}

	this->ComputeTraySize();

	/* Get the remaining size after setting fixed size components. */
	/* Also, keep track of the number of variable size components. */
	width = this->width - TRAY_BORDER_SIZE * 2;
	height = this->height - TRAY_BORDER_SIZE * 2;
	variableCount = 0;
	for (cp = this->components; cp; cp = cp->getNext()) {
		if (this->layout == LAYOUT_HORIZONTAL) {
			temp = cp->getWidth();
			if (temp > 0) {
				width -= temp;
			} else {
				variableCount += 1;
			}
		} else {
			temp = cp->getHeight();
			if (temp > 0) {
				height -= temp;
			} else {
				variableCount += 1;
			}
		}
	}

	/* Distribute excess size among variable size components.
	 * If there are no variable size components, shrink the tray.
	 * If we are out of room, just give them a size of one.
	 */
	*variableSize = 1;
	*variableRemainder = 0;
	if (this->layout == LAYOUT_HORIZONTAL) {
		if (variableCount) {
			if (width >= variableCount) {
				*variableSize = width / variableCount;
				*variableRemainder = width % variableCount;
			}
		} else if (width > 0) {
			this->width -= width;
		}
	} else {
		if (variableCount) {
			if (height >= variableCount) {
				*variableSize = height / variableCount;
				*variableRemainder = height % variableCount;
			}
		} else if (height > 0) {
			this->height -= height;
		}
	}

	this->width = Max(1, this->width);
	this->height = Max(40, this->height);
}

/** Resize a tray. */
void TrayType::ResizeTray() {
	int variableSize;
	int variableRemainder;
	int xoffset, yoffset;
	int width, height;

	Assert(this);

	this->LayoutTray(&variableSize, &variableRemainder);

	/* Reposition items on the tray. */
	xoffset = TRAY_BORDER_SIZE;
	yoffset = TRAY_BORDER_SIZE;
	TrayComponentType *tc;
	for (tc = this->components; tc; tc = tc->getNext()) {
		tc->SetLocation(xoffset, yoffset);
		tc->SetScreenLocation(this->x + xoffset, this->y + yoffset);

		if (this->layout == LAYOUT_HORIZONTAL) {
			height = this->height - TRAY_BORDER_SIZE * 2;
			width = this->getWidth();
			if (width == 0) {
				width = variableSize;
				if (variableRemainder) {
					width += 1;
					variableRemainder -= 1;
				}
			}
		} else {
			width = this->width - TRAY_BORDER_SIZE * 2;
			height = this->getHeight();
			if (height == 0) {
				height = variableSize;
				if (variableRemainder) {
					height += 1;
					variableRemainder -= 1;
				}
			}
		}
		tc->SetSize(width, height);
		tc->Resize();

		if (this->window != None) {
			JXMoveWindow(display, this->window, xoffset, yoffset);
		}

		if (this->layout == LAYOUT_HORIZONTAL) {
			xoffset += this->getWidth();
		} else {
			yoffset += this->getHeight();
		}
	}

	JXMoveResizeWindow(display, this->window, this->x, this->y, this->width, this->height);

	_RequireTaskUpdate();
	this->DrawSpecificTray();

	if (this->hidden) {
		this->HideTray();
	}
}

/** Draw the tray background on a drawable. */
void TrayType::ClearTrayDrawable(const TrayComponentType *cp) {
	const Drawable d = cp->getPixmap() != None ? cp->getPixmap() : cp->getWindow();
	if (Colors::colors[COLOR_TRAY_BG1] == Colors::colors[COLOR_TRAY_BG2]) {
		JXSetForeground(display, rootGC, Colors::colors[COLOR_TRAY_BG1]);
		JXFillRectangle(display, d, rootGC, 0, 0, cp->getWidth(), cp->getHeight());
	} else {
		DrawHorizontalGradient(d, rootGC, Colors::colors[COLOR_TRAY_BG1], Colors::colors[COLOR_TRAY_BG2], 0, 0,
				cp->getWidth(), cp->getHeight());
	}
}

/** Get a linked list of trays. */
TrayType* TrayType::GetTrays(void) {
	return TrayType::trays;
}

/** Get the number of trays. */
unsigned int TrayType::GetTrayCount(void) {
	return TrayType::trayCount;
}

/** Determine if a tray should autohide. */
void TrayType::SetAutoHideTray(TrayAutoHideType autohide, unsigned timeout_ms) {
	if (JUNLIKELY(this->autoHide != THIDE_OFF)) {
		_UnregisterCallback(TrayType::SignalTray, this);
	}

	this->autoHide = autohide;
	this->autoHideDelay = timeout_ms;

	if (autohide != THIDE_OFF) {
		_RegisterCallback(timeout_ms, TrayType::SignalTray, this);
	}
}

/** Set the x-coordinate of a tray. */
void TrayType::SetTrayX(const char *str) {
	Assert(this);
	Assert(str);
	this->requestedX = atoi(str);
}

/** Set the y-coordinate of a tray. */
void TrayType::SetTrayY(const char *str) {
	Assert(this);
	Assert(str);
	this->requestedY = atoi(str);
}

/** Set the width of a tray. */
void TrayType::SetTrayWidth(const char *str) {
	this->requestedWidth = atoi(str);
}

/** Set the height of a tray. */
void TrayType::SetTrayHeight(const char *str) {
	this->requestedHeight = atoi(str);
}

/** Set the tray orientation. */
void TrayType::SetTrayLayout(const char *str) {
	if (!str) {

		/* Compute based on requested size. */

	} else if (!strcmp(str, "horizontal")) {

		this->layout = LAYOUT_HORIZONTAL;
		return;

	} else if (!strcmp(str, "vertical")) {

		this->layout = LAYOUT_VERTICAL;
		return;

	} else {
		Warning(_("invalid tray layout: \"%s\""), str);
	}

	/* Prefer horizontal layout, but use vertical if
	 * width is finite and height is larger than width or infinite.
	 */
	if (this->requestedWidth > 0 && (this->requestedHeight == 0 || this->requestedHeight > this->requestedWidth)) {
		this->layout = LAYOUT_VERTICAL;
	} else {
		this->layout = LAYOUT_HORIZONTAL;
	}
}

/** Set the layer for a tray. */
void TrayType::SetTrayLayer(WinLayerType layer) {
	this->layer = layer;
}

/** Set the horizontal tray alignment. */
void TrayType::SetTrayHorizontalAlignment(const char *str) {
	static const StringMappingType mapping[] = { { "center", TALIGN_CENTER }, { "fixed", TALIGN_FIXED }, { "left",
	TALIGN_LEFT }, { "right", TALIGN_RIGHT } };

	if (!str) {
		this->halign = TALIGN_FIXED;
	} else {
		const int x = FindValue(mapping, ARRAY_LENGTH(mapping), str);
		if (JLIKELY(x >= 0)) {
			this->halign = x;
		} else {
			Warning(_("invalid tray horizontal alignment: \"%s\""), str);
			this->halign = TALIGN_FIXED;
		}
	}
}

/** Set the vertical tray alignment. */
void TrayType::SetTrayVerticalAlignment(const char *str) {
	static const StringMappingType mapping[] = { { "bottom", TALIGN_BOTTOM }, { "center", TALIGN_CENTER }, { "fixed",
	TALIGN_FIXED }, { "top", TALIGN_TOP } };

	if (!str) {
		this->valign = TALIGN_FIXED;
	} else {
		const int x = FindValue(mapping, ARRAY_LENGTH(mapping), str);
		if (JLIKELY(x >= 0)) {
			this->valign = x;
		} else {
			Warning(_("invalid tray vertical alignment: \"%s\""), str);
			this->valign = TALIGN_FIXED;
		}
	}
}
