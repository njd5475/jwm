/**
 * @file confirm.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Confirm dialog functions.
 *
 */

#include "jwm.h"
#include "confirm.h"
#include "client.h"
#include "font.h"
#include "button.h"
#include "border.h"
#include "screen.h"
#include "misc.h"
#include "settings.h"
#include "action.h"
#include "binding.h"

#ifndef DISABLE_CONFIRM

/** Current state of dialog buttons. */
typedef unsigned char DialogButtonState;
#define DBS_NORMAL   0  /**< No button pressed. */
#define DBS_OK       1  /**< OK pressed. */
#define DBS_CANCEL   2  /**< Cancel pressed. */

typedef struct {

	int x, y;
	int width, height;
	int lineHeight;

	int okx;
	int cancelx;
	int buttony;
	int buttonWidth, buttonHeight;
	DialogButtonState buttonState;

	int lineCount;
	char **message;

	Pixmap pmap;
	ClientNode *node;

	void (*action)(ClientNode*);
	Window client;

} DialogType;

static DialogType *dialog = NULL;

static int minWidth = 0;

static void RunDialogAction(void);
static void DestroyConfirmDialog(void);
static void ComputeDimensions(const ClientNode *np);
static void DrawDialog(void);
static void DrawButtons(void);
static void ExposeConfirmDialog(void);
static char HandleDialogExpose(const XExposeEvent *event);
static char HandleDialogButtonPress(const XButtonEvent *event);
static char HandleDialogButtonRelease(const XButtonEvent *event);
static char HandleDialogKeyPress(const XKeyEvent *event);

static const char* GetOKString() {
	return _("OK");
}

static const char* GetCancelString() {
	return _("Cancel");
}

void Dialogs::DestroyDialogs() {

}

void Dialogs::InitializeDialogs() {

}

void Dialogs::StartupDialogs() {

}

/** Stop dialog processing. */
void Dialogs::ShutdownDialogs(void) {
	if (dialog) {
		DestroyConfirmDialog();
		dialog = NULL;
	}
}

/** Run the action associated with the current dialog. */
void RunDialogAction(void) {
	Log("Running Dialog Action");
	if (dialog->client == None) {
		(dialog->action)(NULL);
	} else {
		ClientNode *np = ClientNode::FindClientByWindow(dialog->client);
		if (np) {
			(dialog->action)(np);
		}
	}
}

/** Handle an event on a dialog window. */
char Dialogs::ProcessDialogEvent(const XEvent *event) {

	Assert(event);

	switch (event->type) {
	case Expose:
		return HandleDialogExpose(&event->xexpose);
	case ButtonPress:
		return HandleDialogButtonPress(&event->xbutton);
	case ButtonRelease:
		return HandleDialogButtonRelease(&event->xbutton);
	case KeyPress:
		return HandleDialogKeyPress(&event->xkey);
	default:
		return 0;
	}

}

/** Handle an expose event. */
char HandleDialogExpose(const XExposeEvent *event) {
	Assert(event);
	if (dialog && dialog->node->getWindow() == event->window) {
		ExposeConfirmDialog();
		return 1;
	} else {
		return 0;
	}
}

/** Handle a mouse button release event. */
char HandleDialogButtonRelease(const XButtonEvent *event) {

	Assert(event);

	if (dialog && event->window == dialog->node->getWindow()) {
		char cancelPressed = 0;
		char okPressed = 0;
		const int y = event->y;
		if (y >= dialog->buttony && y < dialog->buttony + dialog->buttonHeight) {
			const int x = event->x;
			if (x >= dialog->okx && x < dialog->okx + dialog->buttonWidth) {
				okPressed = 1;
			} else if (x >= dialog->cancelx && x < dialog->cancelx + dialog->buttonWidth) {
				cancelPressed = 1;
			}
		}

		if (okPressed) {
			RunDialogAction();
		}

		if (cancelPressed || okPressed) {
			DestroyConfirmDialog();
		} else {
			dialog->buttonState = DBS_NORMAL;
			DrawButtons();
			ExposeConfirmDialog();
		}

		return 1;
	} else {

		if (dialog) {
			if (dialog->buttonState != DBS_NORMAL) {
				dialog->buttonState = DBS_NORMAL;
				DrawButtons();
				ExposeConfirmDialog();
			}
		}

		return 0;

	}

}

/** Handle a mouse button release event. */
char HandleDialogButtonPress(const XButtonEvent *event) {

	Assert(event);

	/* Find the dialog on which the press occured (if any). */
	if (dialog && event->window == dialog->node->getWindow()) {

		/* Determine which button was pressed (if any). */
		char cancelPressed = 0;
		char okPressed = 0;
		const int y = event->y;
		if (y >= dialog->buttony && y < dialog->buttony + dialog->buttonHeight) {
			const int x = event->x;
			if (x >= dialog->okx && x < dialog->okx + dialog->buttonWidth) {
				okPressed = 1;
			} else if (x >= dialog->cancelx && x < dialog->cancelx + dialog->buttonWidth) {
				cancelPressed = 1;
			}
		}

		dialog->buttonState = DBS_NORMAL;
		if (cancelPressed) {
			dialog->buttonState = DBS_CANCEL;
		}

		if (okPressed) {
			dialog->buttonState = DBS_OK;
		}

		/* Draw the buttons. */
		DrawButtons();
		ExposeConfirmDialog();

		return 1;

	} else {

		/* This event doesn't affect us. */
		return 0;

	}

}

/** Handle a key press. */
char HandleDialogKeyPress(const XKeyEvent *event) {
	if (dialog && event->window == dialog->node->getWindow()) {
		const ActionType key = Binding::GetKey(MC_NONE, event->state, event->keycode);
		switch (key.action) {
		case ENTER:
			RunDialogAction();
			DestroyConfirmDialog();
			break;
		case ESC:
			DestroyConfirmDialog();
			break;
		default:
			break;
		}
		return 1;
	} else {
		return 0;
	}
}

/** Show a confirm dialog. */
void Dialogs::ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...) {

va_list ap;
XSetWindowAttributes attrs;
XSizeHints shints;
Window window;
char *str;
int x;

Assert(action);

/* Only allow one dialog at a time. */
if(dialog) {
	DestroyConfirmDialog();
}

dialog = new DialogType;
dialog->client = np ? np->getWindow() : None;
dialog->action = action;
dialog->buttonState = DBS_NORMAL;

/* Get the number of lines. */
va_start(ap, action);
for(dialog->lineCount = 0; va_arg(ap, char*); dialog->lineCount++);
va_end(ap);

dialog->message = new char*[dialog->lineCount];
va_start(ap, action);
for(x = 0; x < dialog->lineCount; x++) {
	str = va_arg(ap, char*);
	dialog->message[x] = CopyString(str);
}
va_end(ap);

ComputeDimensions(np);

/* Create the pixmap used for rendering. */
dialog->pmap = JXCreatePixmap(display, rootWindow,
		dialog->width, dialog->height,
		rootDepth);

/* Create the window. */
attrs.background_pixel = Colors::lookupColor(COLOR_MENU_BG);
attrs.event_mask = ButtonPressMask
| ButtonReleaseMask
| KeyPressMask
| ExposureMask;
window = JXCreateWindow(display, rootWindow,
		dialog->x, dialog->y,
		dialog->width, dialog->height, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWBackPixel | CWEventMask, &attrs);
shints.x = dialog->x;
shints.y = dialog->y;
shints.flags = PPosition;
JXSetWMNormalHints(display, window, &shints);
JXStoreName(display, window, _("Confirm"));
Hints::SetAtomAtom(window, ATOM_NET_WM_WINDOW_TYPE,
		ATOM_NET_WM_WINDOW_TYPE_DIALOG);

/* Draw the dialog. */
DrawDialog();

/* Add the client and give it focus. */
dialog->node = new ClientNode(window, 0, 0);
Assert(dialog->node);
if(np) {
	dialog->node->setOwner(np->getWindow());
}
dialog->node->setWMDialogStatus();
dialog->node->FocusClient();

/* Grab the mouse. */
JXGrabButton(display, AnyButton, AnyModifier, window, True,
		ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);

}

/** Copy the pixmap to the confirm dialog. */
void ExposeConfirmDialog(void) {
	Assert(dialog);
	JXCopyArea(display, dialog->pmap, dialog->node->getWindow(), rootGC, 0, 0, dialog->width, dialog->height, 0, 0);
}

/** Destroy a confirm dialog. */
void DestroyConfirmDialog(void) {

	int x;

	Assert(dialog);

	/* This will take care of destroying the dialog window since
	 * its parent will be destroyed. */
	dialog->node->RemoveClient();

	/* Free the pixmap. */
	JXFreePixmap(display, dialog->pmap);

	/* Free the message. */
	for (x = 0; x < dialog->lineCount; x++) {
		Release(dialog->message[x]);
	}
	Release(dialog->message);

	Release(dialog);
	dialog = NULL;

}

/** Compute the size of a dialog window. */
void ComputeDimensions(const ClientNode *np) {

	const ScreenType *sp;
	int width;
	int x;

	Assert(dialog);

	/* Get the min width from the size of the buttons. */
	if (!minWidth) {
		minWidth = Fonts::GetStringWidth(FONT_MENU, GetCancelString()) * 3;
		width = Fonts::GetStringWidth(FONT_MENU, GetOKString()) * 3;
		if (width > minWidth) {
			minWidth = width;
		}
		minWidth += 16 * 3;
	}
	dialog->width = minWidth;

	/* Take into account the size of the message. */
	for (x = 0; x < dialog->lineCount; x++) {
		width = Fonts::GetStringWidth(FONT_MENU, dialog->message[x]);
		if (width > dialog->width) {
			dialog->width = width;
		}
	}
	dialog->lineHeight = Fonts::GetStringHeight(FONT_MENU);
	dialog->width += 8;
	dialog->height = (dialog->lineCount + 2) * dialog->lineHeight;

	if (np) {

		dialog->x = np->getX() + (np->getWidth() - dialog->width) / 2;
		dialog->y = np->getY() + (np->getHeight() - dialog->height) / 2;

		if (dialog->x < 0) {
			dialog->x = 0;
		}
		if (dialog->y < 0) {
			dialog->y = 0;
		}
		if (dialog->x + dialog->width >= rootWidth) {
			dialog->x = rootWidth - dialog->width - (settings.borderWidth * 2);
		}
		if (dialog->y + dialog->height >= rootHeight) {
			const unsigned titleHeight = Border::GetTitleHeight();
			dialog->y = rootHeight - dialog->height - (settings.borderWidth * 2 + titleHeight);
		}

	} else {

		sp = Screens::GetMouseScreen();

		dialog->x = (sp->width - dialog->width) / 2 + sp->x;
		dialog->y = (sp->height - dialog->height) / 2 + sp->y;

	}

}

/** Render the dialog to the pixmap. */
void DrawDialog(void) {

	int yoffset;
	int x;

	Assert(dialog);

	/* Clear the dialog. */
	JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_MENU_BG));
	JXFillRectangle(display, dialog->pmap, rootGC, 0, 0, dialog->width, dialog->height);

	/* Draw the message. */
	yoffset = 4;
	for (x = 0; x < dialog->lineCount; x++) {
		Fonts::RenderString(dialog->pmap, FONT_MENU, COLOR_MENU_FG, 4, yoffset, dialog->width, dialog->message[x]);
		yoffset += dialog->lineHeight;
	}

	/* Draw the buttons. */
	DrawButtons();

}

/** Draw the buttons on the dialog window. */
void DrawButtons(void) {

	int temp;

	Assert(dialog);

	dialog->buttonWidth = Fonts::GetStringWidth(FONT_MENU, GetCancelString());
	temp = Fonts::GetStringWidth(FONT_MENU, GetOKString());
	if (temp > dialog->buttonWidth) {
		dialog->buttonWidth = temp;
	}
	dialog->buttonWidth += 16;
	dialog->buttonHeight = dialog->lineHeight + 4;

	Drawable drawable = dialog->pmap;

	bool border = true;
	FontType font = FONT_MENU;
	int width = dialog->buttonWidth;
	int height = dialog->buttonHeight;
	AlignmentType alignment = ALIGN_CENTER;

	dialog->okx = dialog->width / 3 - dialog->buttonWidth / 2;
	dialog->cancelx = 2 * dialog->width / 3 - dialog->buttonWidth / 2;
	dialog->buttony = dialog->height - dialog->lineHeight - dialog->lineHeight / 2;

	ButtonType type = BUTTON_MENU;
	if (dialog->buttonState == DBS_OK) {
		type = BUTTON_MENU_ACTIVE;
	}
	const char *text = GetOKString();
	int x = dialog->okx;
	int y = dialog->buttony;
	int xoffset = 0, yoffset = 0;
	IconNode *icon = NULL;
	DrawButton(type, alignment, font, text, true, border, drawable, icon, x, y, width, height, xoffset, yoffset);

	if (dialog->buttonState == DBS_CANCEL) {
		type = BUTTON_MENU_ACTIVE;
	}
	const char *cancelText = GetCancelString();
	x = dialog->cancelx;
	y = dialog->buttony;
	DrawButton(type, alignment, font, cancelText, true, border, drawable, icon, x, y, width, height, xoffset, yoffset);

}

#else /* DISABLE_CONFIRM */

/** Process an event on a dialog window. */
char Dialogs::ProcessDialogEvent(const XEvent *event)
{
  return 0;
}

/** Show a confirm dialog. */
void Dialogs::ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...)
{
  Assert(action);
  (action)(np);
}

#endif /* DISABLE_CONFIRM */

