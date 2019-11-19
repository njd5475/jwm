/**
 * @file dock.c
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Dock tray component.
 *
 */

#include "jwm.h"
#include "dock.h"
#include "tray.h"
#include "main.h"
#include "error.h"
#include "color.h"
#include "misc.h"
#include "settings.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

DockType *DockType::dock = NULL;
char DockType::owner = 0;

Atom DockType::dockAtom = 0;
unsigned long DockType::orientation = 0;

const char *DockType::BASE_SELECTION_NAME = "_NET_SYSTEM_TRAY_S%d";

/** Initialize dock data. */
void DockType::_InitializeDock(void) {
	owner = 0;
}

/** Startup the dock. */
void DockType::_StartupDock(void) {

	char *selectionName;

	if (!dock) {
		/* No dock has been requested. */
		return;
	}

	if (dock->window == None) {

		/* No dock yet. */

		/* Get the selection atom. */
		selectionName = AllocateStack(sizeof(BASE_SELECTION_NAME));
		snprintf(selectionName, sizeof(BASE_SELECTION_NAME), BASE_SELECTION_NAME, rootScreen);
		dockAtom = JXInternAtom(display, selectionName, False);
		ReleaseStack(selectionName);

		/* The location and size of the window doesn't matter here. */
		dock->window = JXCreateSimpleWindow(display, rootWindow,
		/* x, y, width, height */0, 0, 1, 1,
		/* border_size, border_color */0, 0,
		/* background */Colors::lookupColor(COLOR_TRAY_BG2));
		JXSelectInput(display, dock->window,
				SubstructureNotifyMask | SubstructureRedirectMask | EnterWindowMask | PointerMotionMask | PointerMotionHintMask);

	}

}

/** Shutdown the dock. */
void DockType::_ShutdownDock(void) {

	DockNode *np;

	if (dock) {

		/* Release memory used by the dock list. */
		while (dock->nodes) {
			np = dock->nodes->next;
			JXReparentWindow(display, dock->nodes->window, rootWindow, 0, 0);
			Release(dock->nodes);
			dock->nodes = np;
		}

		/* Release the selection. */
		if (owner) {
			JXSetSelectionOwner(display, dockAtom, None, CurrentTime);
		}

		/* Destroy the dock window. */
		JXDestroyWindow(display, dock->window);

	}

}

/** Destroy dock data. */
void DockType::_DestroyDock(void) {
	if (dock) {
		Release(dock);
		dock = NULL;
	}
}

/** Create a dock component. */
DockType::DockType(int width) :
		TrayComponent() {
	this->nodes = NULL;
	this->window = None;
	this->requestedWidth = 1;
	this->requestedHeight = 1;
	this->itemSize = width;
}

/** Set the size of a dock component. */
void DockType::SetSize(int width, int height) {

	/* Set the orientation. */
	if (width == 0) {
		orientation = SYSTEM_TRAY_ORIENTATION_HORZ;
	} else if (height == 0) {
		orientation = SYSTEM_TRAY_ORIENTATION_VERT;
	}

	/* Get the size. */
	int newWidth = width;
	int newHeight = height;
	_GetDockSize(&newWidth, &newHeight);
	if (width == 0) {
		newWidth = this->getWidth();
		newHeight = 0;
	} else {
		newWidth = 0;
		newHeight = this->getHeight();
	}
	this->requestNewSize(newWidth, newHeight);
}

/** Initialize a dock component. */
void DockType::Create() {

	XEvent event;

	/* Map the dock window. */
	if (this->getWindow() != None) {
		JXResizeWindow(display, this->getWindow(), this->getWidth(), this->getHeight());
		JXMapRaised(display, this->getWindow());
	}

	/* Set the orientation atom. */
	Hints::SetCardinalAtom(dock->getWindow(), ATOM_NET_SYSTEM_TRAY_ORIENTATION, orientation);

	/* Get the selection if we don't already own it.
	 * If we did already own it, getting it again would cause problems
	 * with some clients due to the way restarts are handled.
	 */
	if (!owner) {

		owner = 1;
		JXSetSelectionOwner(display, dockAtom, dock->getWindow(), CurrentTime);
		if (JUNLIKELY(JXGetSelectionOwner(display, dockAtom) != dock->getWindow())) {

			owner = 0;
			Warning(_("could not acquire system tray selection"));

		} else {

			memset(&event, 0, sizeof(event));
			event.xclient.type = ClientMessage;
			event.xclient.window = rootWindow;
			event.xclient.message_type = Hints::atoms[ATOM_MANAGER];
			event.xclient.format = 32;
			event.xclient.data.l[0] = CurrentTime;
			event.xclient.data.l[1] = dockAtom;
			event.xclient.data.l[2] = dock->getWindow();
			event.xclient.data.l[3] = 0;
			event.xclient.data.l[4] = 0;

			JXSendEvent(display, rootWindow, False, StructureNotifyMask, &event);

		}

	}

}

/** Resize a dock component. */
void DockType::Resize() {
	TrayComponent::Resize();
	JXResizeWindow(display, this->getWindow(), this->getWidth(), this->getHeight());
	_UpdateDock();
}

/** Handle a dock event. */
void DockType::_HandleDockEvent(const XClientMessageEvent *event) {
	Assert(event);
	switch (event->data.l[1]) {
	case SYSTEM_TRAY_REQUEST_DOCK:
		_DockWindow(event->data.l[2]);
		break;
	case SYSTEM_TRAY_BEGIN_MESSAGE:
		break;
	case SYSTEM_TRAY_CANCEL_MESSAGE:
		break;
	default:
		Debug("invalid opcode in dock event");
		break;
	}
}

/** Handle a resize request event. */
char DockType::_HandleDockResizeRequest(const XResizeRequestEvent *event) {
	DockNode *np;

	Assert(event);

	if (!dock) {
		return 0;
	}

	for (np = dock->nodes; np; np = np->next) {
		if (np->window == event->window) {
			_UpdateDock();
			return 1;
		}
	}

	return 0;
}

/** Handle a configure request event. */
char DockType::_HandleDockConfigureRequest(const XConfigureRequestEvent *event) {

	DockNode *np;

	Assert(event);

	if (!dock) {
		return 0;
	}

	for (np = dock->nodes; np; np = np->next) {
		if (np->window == event->window) {
			_UpdateDock();
			return 1;
		}
	}

	return 0;

}

/** Handle a reparent notify event. */
char DockType::_HandleDockReparentNotify(const XReparentEvent *event) {

	DockNode *np;
	char handled;

	Assert(event);

	/* Just return if there is no dock. */
	if (!dock) {
		return 0;
	}

	/* Check each docked window. */
	handled = 0;
	for (np = dock->nodes; np; np = np->next) {
		if (np->window == event->window) {
			if (event->parent != dock->getWindow()) {
				/* For some reason the application reparented the window.
				 * We make note of this condition and reparent every time
				 * the dock is updated. Unfortunately we can't do this for
				 * all applications because some won't deal with it.
				 */
				np->needs_reparent = 1;
				handled = 1;
			}
		}
	}

	/* Layout the stuff on the dock again if something happened. */
	if (handled) {
		_UpdateDock();
	}

	return handled;

}

/** Handle a selection clear event. */
char DockType::_HandleDockSelectionClear(const XSelectionClearEvent *event) {
	if (event->selection == dockAtom) {
		Debug("lost _NET_SYSTEM_TRAY selection");
		owner = 0;
	}
	return 0;
}

/** Add a window to the dock. */
void DockType::_DockWindow(Window win) {
	DockNode *np;

	/* If no dock is running, just return. */
	if (!dock) {
		return;
	}

	/* Make sure we have a valid window to add. */
	if (JUNLIKELY(win == None)) {
		return;
	}

	/* If this window is already docked ignore it. */
	for (np = dock->nodes; np; np = np->next) {
		if (np->window == win) {
			return;
		}
	}

	/* Add the window to our list. */
	np = new DockNode;
	np->window = win;
	np->needs_reparent = 0;
	np->next = dock->nodes;
	dock->nodes = np;

	/* Update the requested size. */
	int newWidth = dock->getRequestedWidth(), newHeight = dock->getRequestedHeight();
	_GetDockSize(&newWidth, &newHeight);
	dock->requestNewSize(newWidth, newHeight);

	/* It's safe to reparent at (0, 0) since we call
	 * ResizeTray which will invoke the Resize callback.
	 */
	JXAddToSaveSet(display, win);
	JXReparentWindow(display, win, dock->getWindow(), 0, 0);
	JXMapRaised(display, win);

	/* Resize the tray containing the dock. */
	dock->getTray()->ResizeTray();

}

/** Remove a window from the dock. */
char DockType::_HandleDockDestroy(Window win) {
	DockNode **np;

	/* If no dock is running, just return. */
	if (!dock) {
		return 0;
	}

	for (np = &dock->nodes; *np; np = &(*np)->next) {
		DockNode *dp = *np;
		if (dp->window == win) {

			/* Remove the window from our list. */
			*np = dp->next;
			Release(dp);

			/* Update the requested size. */
			int newWidth = dock->getRequestedWidth(), newHeight = dock->getRequestedHeight();
			_GetDockSize(&newWidth, &newHeight);
			dock->requestNewSize(newWidth, newHeight);

			/* Resize the tray. */
			dock->getTray()->ResizeTray();
			return 1;
		}
	}

	return 0;
}

/** Layout items on the dock. */
void DockType::_UpdateDock(void) {

	XConfigureEvent event;
	DockNode *np;
	int x, y;
	int itemSize;

	Assert(dock);

	/* Determine the size of items in the dock. */
	_GetDockItemSize(&itemSize);

	x = 0;
	y = 0;
	memset(&event, 0, sizeof(event));
	for (np = dock->nodes; np; np = np->next) {

		JXMoveResizeWindow(display, np->window, x, y, itemSize, itemSize);

		/* Reparent if this window likes to go other places. */
		if (np->needs_reparent) {
			JXReparentWindow(display, np->window, dock->getWindow(), x, y);
		}

		event.type = ConfigureNotify;
		event.event = np->window;
		event.window = np->window;
		event.x = dock->getScreenX() + x;
		event.y = dock->getScreenY() + y;
		event.width = itemSize;
		event.height = itemSize;
		JXSendEvent(display, np->window, False, StructureNotifyMask, (XEvent* )&event);

		if (orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
			x += itemSize + settings.dockSpacing;
		} else {
			y += itemSize + settings.dockSpacing;
		}

	}

}

/** Get the size of a particular window on the dock. */
void DockType::_GetDockItemSize(int *size) {
	/* Determine the default size of items in the dock. */
	if (0 == SYSTEM_TRAY_ORIENTATION_HORZ) {
		*size = dock->getHeight();
	} else {
		*size = dock->getWidth();
	}
	if (dock->itemSize > 0 && *size > dock->itemSize) {
		*size = dock->itemSize;
	}
}

/** Get the size of the dock. */
void DockType::_GetDockSize(int *width, int *height) {
	DockNode *np;
	int itemSize;

	Assert(dock != NULL);

	/* Get the dock item size. */
	if (0 == SYSTEM_TRAY_ORIENTATION_HORZ) {
		itemSize = dock->getHeight();
	} else {
		itemSize = dock->getWidth();
	}
	if (dock->itemSize > 0 && itemSize > dock->itemSize) {
		itemSize = dock->itemSize;
	}

	/* Determine the size of the items on the dock. */
	for (np = dock->nodes; np; np = np->next) {
		const unsigned spacing = (np->next ? settings.dockSpacing : 0);
		if (orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
			/* Horizontal tray; height fixed, placement is left to right. */
			*width += itemSize + spacing;
		} else {
			/* Vertical tray; width fixed, placement is top to bottom. */
			*height += itemSize + spacing;
		}
	}

	/* Don't allow the dock to have zero size since a size of
	 * zero indicates a variable sized component. */
	if (orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
		*width = Max(*width, 1);
	} else {
		*height = Max(*height, 1);
	}

}
