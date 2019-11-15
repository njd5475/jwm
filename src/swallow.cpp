/**
 * @file swallow.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Swallow tray component.
 *
 */

#include "jwm.h"
#include "swallow.h"
#include "main.h"
#include "tray.h"
#include "error.h"
#include "command.h"
#include "color.h"
#include "client.h"
#include "misc.h"

/** Start swallow processing. */
void SwallowNode::StartupSwallow(void) {
	SwallowNode *np;
	for (np = pendingNodes; np; np = np->next) {
		if (np->command) {
			Commands::RunCommand(np->command);
		}
	}
}

/** Destroy swallow data. */
void SwallowNode::DestroySwallow(void) {
	ReleaseNodes(pendingNodes);
	ReleaseNodes(swallowNodes);
	pendingNodes = NULL;
	swallowNodes = NULL;
}

/** Release a linked list of swallow nodes. */
void SwallowNode::ReleaseNodes(SwallowNode *nodes) {
	while (nodes) {
		SwallowNode *np = nodes->next;
		Assert(nodes->name);
		Release(nodes->name);
		if (nodes->command) {
			Release(nodes->command);
		}
		Release(nodes);
		nodes = np;
	}
}

/** Create a swallowed application tray component. */
SwallowNode::SwallowNode(const char *name, const char *command, int width,
		int height) :
		TrayComponentType(), border(0) {
	if (JUNLIKELY(!name)) {
		Warning(_("cannot swallow a client with no name"));
		return;
	}

	this->name = CopyString(name);
	this->command = CopyString(command);

	this->next = pendingNodes;
	pendingNodes = this;

	if (width) {
		this->requestedWidth = width;
		this->userWidth = 1;
	} else {
		this->requestedWidth = 1;
		this->userWidth = 0;
	}
	if (height) {
		this->requestedHeight = height;
		this->userHeight = 1;
	} else {
		this->requestedHeight = 1;
		this->userHeight = 0;
	}

}

/** Process an event on a swallowed window. */
char SwallowNode::ProcessSwallowEvent(const XEvent *event) {

	SwallowNode *np;
	int width, height;

	for (np = swallowNodes; np; np = np->next) {
		if (event->xany.window == np->window) {
			switch (event->type) {
			case DestroyNotify:
				np->window = None;
				np->requestedWidth = 1;
				np->requestedHeight = 1;
				np->getTray()->ResizeTray();
				break;
			case ResizeRequest:
				np->requestedWidth = event->xresizerequest.width
						+ np->border * 2;
				np->requestedHeight = event->xresizerequest.height
						+ np->border * 2;
				np->getTray()->ResizeTray();
				break;
			case ConfigureNotify:
				/* I don't think this should be necessary, but somehow
				 * resize requests slip by sometimes... */
				width = event->xconfigure.width + np->border * 2;
				height = event->xconfigure.height + np->border * 2;
				if (width != np->getRequestedWidth()
						&& height != np->getRequestedHeight()) {
					np->requestedWidth = width;
					np->requestedHeight = height;
					np->getTray()->ResizeTray();
				}
				break;
			default:
				break;
			}
			return 1;
		}
	}

	return 0;

}

void SwallowNode::Create() {

}

/** Handle a tray resize. */
void SwallowNode::Resize() {
	TrayComponentType::Resize();
	if (this->window != None) {
		const unsigned int width = this->getWidth() - this->border * 2;
		const unsigned int height = this->getHeight() - this->border * 2;
		JXResizeWindow(display, this->window, width, height);
	}

}

/** Destroy a swallow tray component. */
void SwallowNode::Destroy() {

	/* Destroy the window if there is one. */
	if (this->window) {

		JXReparentWindow(display, this->window, rootWindow, 0, 0);
		JXRemoveFromSaveSet(display, this->window);

		ClientState state;
		memset(&state, 0, sizeof(state));
		Hints::ReadWMProtocols(this->window, &state);
		if (state.getStatus() & STAT_DELETE) {
			ClientNode::SendClientMessage(this->window, ATOM_WM_PROTOCOLS,
					ATOM_WM_DELETE_WINDOW);
		} else {
			JXKillClient(display, this->window);
		}

	}

}

SwallowNode *SwallowNode::pendingNodes;
SwallowNode *SwallowNode::swallowNodes;

/** Determine if this is a window to be swallowed, if it is, swallow it. */
char SwallowNode::CheckSwallowMap(Window win) {

	SwallowNode **npp;
	XClassHint hint;
	XWindowAttributes attr;
	char result;

	/* Return if there are no programs left to swallow. */
	if (!pendingNodes) {
		return 0;
	}

	/* Get the name of the window. */
	if (JXGetClassHint(display, win, &hint) == 0) {
		return 0;
	}

	/* Check if we should swallow this window. */
	result = 0;
	npp = &pendingNodes;
	while (*npp) {

		SwallowNode *np = *npp;
		Assert(np->getTray()->window != None);

		if (!strcmp(hint.res_name, np->name)) {

			/* Swallow the window. */
			JXSelectInput(display, win,
					StructureNotifyMask | ResizeRedirectMask);
			JXAddToSaveSet(display, win);
			JXSetWindowBorder(display, win, Colors::lookupColor(COLOR_TRAY_BG2));
			JXReparentWindow(display, win, np->getTray()->getWindow(), 0, 0);
			JXMapRaised(display, win);
			np->window = win;

			/* Remove this node from the pendingNodes list and place it
			 * on the swallowNodes list. */
			*npp = np->next;
			np->next = swallowNodes;
			swallowNodes = np;

			/* Update the size. */
			JXGetWindowAttributes(display, win, &attr);
			np->border = attr.border_width;
			int newWidth = np->getWidth(), newHeight = np->getHeight();
			if (!np->userWidth) {
				newWidth = attr.width + 2 * np->border;
			}
			if (!np->userHeight) {
				newHeight = attr.height + 2 * np->border;
			}
			np->requestNewSize(newWidth, newHeight);

			np->getTray()->ResizeTray();
			result = 1;

			break;
		}

		npp = &np->next;

	}
	JXFree(hint.res_name);
	JXFree(hint.res_class);

	return result;

}

/** Determine if there are swallow processes pending. */
char SwallowNode::IsSwallowPending(void) {
	return pendingNodes ? 1 : 0;
}

