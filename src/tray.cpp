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
#include "button.h"
#include "confirm.h"
#include "binding.h"

#define DEFAULT_TRAY_WIDTH 32
#define DEFAULT_TRAY_HEIGHT 32

#define TRAY_BORDER_SIZE   1

std::vector<Tray*> Tray::trays;

/** Initialize tray data. */
void Tray::InitializeTray(void) {
}

/** Startup trays. */
void Tray::StartupTray(void) {
	XSetWindowAttributes attr;
	unsigned long attrMask;

	int variableSize;
	int variableRemainder;
	int width, height;
	int xoffset, yoffset;

	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		Tray *tp = (*it);
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
		attr.background_pixel = Colors::lookupColor(COLOR_TRAY_BG2);

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
		std::vector<TrayComponent*>::iterator it;
		for (it = tp->components.begin(); it != tp->components.end(); ++it) {
			TrayComponent *cp = *it;
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
	}

	_RequirePagerUpdate();
	_RequireTaskUpdate();

	Dialogs::ShowConfirmDialog(NULL, handleConfirm,
	_("Smile"),
	_("Are you ok?"),
	NULL);
}

void Tray::handleConfirm(ClientNode *np) {

}

Tray* Tray::Create() {
	Tray *t = new Tray();
	trays.push_back(t);
	return t;
}

/** Shutdown trays. */
void Tray::ShutdownTray(void) {
	std::vector<Tray*>::iterator tray;
	for (tray = trays.begin(); tray != trays.end(); ++tray) {
		std::vector<TrayComponent*>::iterator it;
		for (it = (*tray)->components.begin(); it != (*tray)->components.end(); ++it) {
			(*it)->Destroy();
		}
		JXDestroyWindow(display, (*tray)->window);
	}
}

/** Destroy tray data. */
void Tray::DestroyTray(void) {
	TrayComponent *cp;

	std::vector<Tray*>::iterator tray;
	for (tray = trays.begin(); tray != trays.end(); ++tray) {
		Release((*tray));
	}
}

/** Create an empty tray. */
Tray::Tray() {
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
}

Tray::~Tray() {
	if (autoHide != THIDE_OFF) {
		_UnregisterCallback(SignalTray, this);
	}
	std::vector<TrayComponent*>::iterator it;
	for (it = components.begin(); it != components.end(); ++it) {
		Release(*it);
	}
}

/** Add a tray component to a tray. */
void Tray::AddTrayComponent(TrayComponent *cp) {
	cp->SetParent(this);
	bool found = false;
	std::vector<TrayComponent*>::iterator it;
	for (it = components.begin(); it != components.end(); ++it) {
		if ((*it) == cp) {
			found = true;
			break;
		}
	}
	if (found) {
		this->components.erase(it);
	}
	this->components.push_back(cp);
}

/** Compute the max component width. */
int Tray::ComputeMaxWidth() {
	TrayComponent *cp;
	int result;
	int temp;

	result = 0;
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		temp = (*it)->getWidth();
		if (temp > 0 && temp > result) {
			result = temp;
		}
	}

	return result;
}

/** Compute the total width of a tray. */
int Tray::ComputeTotalWidth() {
	int result;

	result = 2 * TRAY_BORDER_SIZE;
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		result += (*it)->getWidth();
	}

	return result;
}

/** Compute the max component height. */
int Tray::ComputeMaxHeight() {
	TrayComponent *cp;
	int result;
	int temp;

	result = 0;
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		temp = (*it)->getHeight();
		if (temp > 0 && temp > result) {
			result = temp;
		}
	}

	return result;
}

/** Compute the total height of a tray. */
int Tray::ComputeTotalHeight() {
	TrayComponent *cp;
	int result;

	result = 2 * TRAY_BORDER_SIZE;
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		result += (*it)->getHeight();
	}

	return result;
}

/** Check if the tray fills the screen horizontally. */
char Tray::CheckHorizontalFill() {
	TrayComponent *cp;
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		cp = *it;
		if (cp->getWidth() == 0) {
			return 1;
		}
	}

	return 0;
}

/** Check if the tray fills the screen vertically. */
char Tray::CheckVerticalFill() {
	TrayComponent *cp;
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		cp = *it;
		if (cp->getHeight() == 0) {
			return 1;
		}
	}

	return 0;
}

/** Compute the size of a tray. */
void Tray::ComputeTraySize() {
	TrayComponent *cp;
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
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		cp = *it;
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
void Tray::ShowTray() {
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
void Tray::ShowAllTrays(void) {
	if (shouldExit) {
		return;
	}

	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		(*it)->ShowTray();
	}
}

/** Hide a tray (for autohide). */
void Tray::HideTray() {
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
char Tray::ProcessTrayEvent(const XEvent *event) {

	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		if (event->xany.window == (*it)->window) {
			switch (event->type) {
			case Expose:
				HandleTrayExpose((*it), &event->xexpose);
				return 1;
			case EnterNotify:
				HandleTrayEnterNotify((*it), &event->xcrossing);
				return 1;
			case ButtonPress:
				HandleTrayButtonPress((*it), &event->xbutton);
				return 1;
			case ButtonRelease:
				HandleTrayButtonRelease((*it), &event->xbutton);
				return 1;
			case MotionNotify:
				HandleTrayMotionNotify((*it), &event->xmotion);
				return 1;
			default:
				return 0;
			}
		}
	}

	return 0;
}

/** Signal the tray (needed for autohide). */
void Tray::SignalTray(const TimeType *now, int x, int y, Window w, void *data) {
	Tray *tp = (Tray*) data;
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

void Tray::UngrabKeys(Display *d, unsigned int keyCode, unsigned int modifiers) {
	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		JXUngrabKey(display, AnyKey, AnyModifier, (*it)->getWindow());
	}
}

/** Handle a tray expose event. */
void Tray::HandleTrayExpose(Tray *tp, const XExposeEvent *event) {
	tp->DrawSpecificTray();
}

/** Handle a tray enter notify (for autohide). */
void Tray::HandleTrayEnterNotify(Tray *tp, const XCrossingEvent *event) {
	tp->ShowTray();
}

/** Get the tray component under the given coordinates. */
TrayComponent* Tray::GetTrayComponent(Tray *tp, int x, int y) {
	TrayComponent *cp;
	int xoffset, yoffset;

	xoffset = 0;
	yoffset = 0;
	std::vector<TrayComponent*>::iterator it;
	for (it = tp->components.begin(); it != tp->components.end(); ++it) {
		cp = *it;
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
void Tray::HandleTrayButtonPress(Tray *tp, const XButtonEvent *event) {
	TrayComponent *cp = GetTrayComponent(tp, event->x, event->y);
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
void Tray::HandleTrayButtonRelease(Tray *tp, const XButtonEvent *event) {
	TrayComponent *cp;

	// First inform any components that have a grab.
	std::vector<TrayComponent*>::iterator it;
	for (it = tp->components.begin(); it != tp->components.end(); ++it) {
		cp = *it;
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
void Tray::HandleTrayMotionNotify(Tray *tp, const XMotionEvent *event) {
	TrayComponent *cp = GetTrayComponent(tp, event->x, event->y);
	if (cp) {
		const int x = event->x - cp->getX();
		const int y = event->y - cp->getY();
		const int mask = event->state;
		cp->ProcessMotionEvent(x, y, mask);
	}
}

/** Draw all trays. */
void Tray::DrawTray(void) {
	Tray *tp;

	if (shouldExit) {
		return;
	}

	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		(*it)->DrawSpecificTray();
	}

}

/** Draw a specific tray. */
void Tray::DrawSpecificTray() {
	TrayComponent *cp;

	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		cp = *it;
		cp->Resize();
		cp->Draw();
		cp->UpdateSpecificTray(this);
	}

	if (settings.trayDecorations == DECO_MOTIF) {
		JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_TRAY_UP));
		JXDrawLine(display, this->window, rootGC, 0, 0, this->width - 1, 0);
		JXDrawLine(display, this->window, rootGC, 0, this->height - 1, 0, 0);

		JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_TRAY_DOWN));
		JXDrawLine(display, this->window, rootGC, 0, this->height - 1, this->width - 1, this->height - 1);
		JXDrawLine(display, this->window, rootGC, this->width - 1, 0, this->width - 1, this->height - 1);
	} else {
		JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_TRAY_DOWN));
		JXDrawRectangle(display, this->window, rootGC, 0, 0, this->width - 1, this->height - 1);
	}
}

/** Raise tray windows. */
void Tray::RaiseTrays(void) {

	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		(*it)->autoHide |= THIDE_RAISED;
		(*it)->ShowTray();
		JXRaiseWindow(display, (*it)->window);
	}
}

/** Lower tray windows. */
void Tray::LowerTrays(void) {
	Tray *tp;

	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		(*it)->autoHide &= ~THIDE_RAISED;
	}
	_RequireRestack();
}

/** Layout tray components on a tray. */
void Tray::LayoutTray(int *variableSize, int *variableRemainder) {
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

	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		(*it)->RefreshSize();
	}

	this->ComputeTraySize();

	/* Get the remaining size after setting fixed size components. */
	/* Also, keep track of the number of variable size components. */
	width = this->width - TRAY_BORDER_SIZE * 2;
	height = this->height - TRAY_BORDER_SIZE * 2;
	variableCount = 0;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		TrayComponent *cp = *it;
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
void Tray::ResizeTray() {
	int variableSize;
	int variableRemainder;
	int xoffset, yoffset;
	int width, height;

	Assert(this);

	this->LayoutTray(&variableSize, &variableRemainder);

	/* Reposition items on the tray. */
	xoffset = TRAY_BORDER_SIZE;
	yoffset = TRAY_BORDER_SIZE;
	std::vector<TrayComponent*>::iterator it;
	for (it = this->components.begin(); it != this->components.end(); ++it) {
		TrayComponent *tc;
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
void Tray::ClearTrayDrawable(const TrayComponent *cp) {
	const Drawable d = cp->getPixmap() != None ? cp->getPixmap() : cp->getWindow();
	if (Colors::lookupColor(COLOR_TRAY_BG1) == Colors::lookupColor(COLOR_TRAY_BG2)) {
		JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_TRAY_BG1));
		JXFillRectangle(display, d, rootGC, 0, 0, cp->getWidth(), cp->getHeight());
	} else {
		DrawHorizontalGradient(d, rootGC, Colors::lookupColor(COLOR_TRAY_BG1), Colors::lookupColor(COLOR_TRAY_BG2), 0,
				0, cp->getWidth(), cp->getHeight());
	}
}

/** Determine if a tray should autohide. */
void Tray::SetAutoHideTray(TrayAutoHideType autohide, unsigned timeout_ms) {
	if (JUNLIKELY(this->autoHide != THIDE_OFF)) {
		_UnregisterCallback(Tray::SignalTray, this);
	}

	this->autoHide = autohide;
	this->autoHideDelay = timeout_ms;

	if (autohide != THIDE_OFF) {
		_RegisterCallback(timeout_ms, Tray::SignalTray, this);
	}
}

/** Set the x-coordinate of a tray. */
void Tray::SetTrayX(const char *str) {
	Assert(this);
	Assert(str);
	this->requestedX = atoi(str);
}

/** Set the y-coordinate of a tray. */
void Tray::SetTrayY(const char *str) {
	Assert(this);
	Assert(str);
	this->requestedY = atoi(str);
}

/** Set the width of a tray. */
void Tray::SetTrayWidth(const char *str) {
	this->requestedWidth = atoi(str);
}

/** Set the height of a tray. */
void Tray::SetTrayHeight(const char *str) {
	this->requestedHeight = atoi(str);
}

/** Set the tray orientation. */
void Tray::SetTrayLayout(const char *str) {
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
void Tray::SetTrayLayer(WinLayerType layer) {
	this->layer = layer;
}

/** Set the horizontal tray alignment. */
void Tray::SetTrayHorizontalAlignment(const char *str) {
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
void Tray::SetTrayVerticalAlignment(const char *str) {
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

std::vector<BoundingBox> Tray::GetBoundsAbove(int layer) {
	std::vector<BoundingBox> boxes;
	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		Tray *tp = *it;
		if (tp->getLayer() > layer && tp->getAutoHide() == THIDE_OFF) {
			boxes.push_back( { tp->getX(), tp->getY(), tp->getWidth(), tp->getHeight() });
		}
	}

	return boxes;
}

std::vector<BoundingBox> Tray::GetVisibleBounds() {
	std::vector<BoundingBox> boxes;
	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		Tray *tp = *it;
		if (!tp->isHidden()) {
			boxes.push_back( { tp->getX(), tp->getY(), tp->getWidth(), tp->getHeight() });
		}
	}

	return boxes;
}

void Tray::GrabKey(KeyNode *kn) {
	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		Binding::GrabKey(kn, (*it)->getWindow());
	}
}

std::vector<Window> Tray::getTrayWindowsAt(int layer) {
	std::vector<Window> windows;
	std::vector<Tray*>::iterator it;
	for (it = trays.begin(); it != trays.end(); ++it) {
		if ((*it)->getLayer() == layer) {
			windows.push_back((*it)->getWindow());
		}
	}
	return windows;
}

TrayComponent *Tray::getLastComponent() {
	if(this->components.empty()) {
		return NULL;
	}
	return *this->components.end();
}
