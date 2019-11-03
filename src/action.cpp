/**
 * @file action.c
 * @author Joe Wingbermuehle
 *
 * @brief Tray component actions.
 */

#include "jwm.h"
#include "action.h"
#include "tray.h"
#include "root.h"
#include "screen.h"
#include "misc.h"
#include "error.h"
#include "cursor.h"
#include "command.h"
#include "menu.h"
#include "DesktopEnvironment.h"

ActionNode::ActionNode(const char *action, int mask) :
		action(CopyString(action)), mask(mask) {

}

ActionNode::~ActionNode() {
	delete this->action;
}

/** Process a button press. */
void ActionNode::ProcessActionPress(struct TrayComponentType *cp, int x, int y,
		int button) {
	const ScreenType *sp;
	const int mask = 1 << button;
	int mwidth, mheight;
	int menu;

	if (JUNLIKELY(menuShown)) {
		return;
	}
	if (x < -1 || x > cp->getWidth()) {
		return;
	}
	if (y < -1 || y > cp->getHeight()) {
		return;
	}

	menu = -1;
	if (this->mask & mask) {
		if (this->action && this->action[0]) {
			if (strncmp(this->action, "root:", 5) != 0) {
				if (button == Button4 || button == Button5) {

					/* Don't wait for a release if the scroll wheel is used. */
					if (!strncmp(this->action, "exec:", 5)) {
						Commands::RunCommand(this->action + 5);
					} else if (!strcmp(this->action, "showdesktop")) {
						DesktopEnvironment::DefaultEnvironment()->ShowDesktop();
					}

				} else {

					if (!Cursors::GrabMouse(cp->getTray()->getWindow())) {
						return;
					}

					/* Show the button being pressed. */
					cp->grab();
					cp->Redraw();
					cp->UpdateSpecificTray(cp->getTray());
				}
				return;

			} else {
				menu = Roots::GetRootMenuIndexFromString(&this->action[5]);
			}
		} else {
			menu = 1;
		}
	}

	if (menu < 0) {
		return;
	}

	Roots::GetRootMenuSize(menu, &mwidth, &mheight);
	sp = GetCurrentScreen(cp->getScreenX(), cp->getScreenY());
	if (cp->getTray()->getLayout() == LAYOUT_HORIZONTAL) {
		x = cp->getScreenX() - 1;
		if (cp->getScreenY() + cp->getHeight() / 2 < sp->y + sp->height / 2) {
			y = cp->getScreenY() + cp->getHeight();
		} else {
			y = cp->getScreenY() - mheight;
		}
	} else {
		y = cp->getScreenY() - 1;
		if (cp->getScreenX() + cp->getWidth() / 2 < sp->x + sp->width / 2) {
			x = cp->getScreenX() + cp->getWidth();
		} else {
			x = cp->getScreenX() - mwidth;
		}
	}

	cp->grab();
	cp->Redraw();
	cp->UpdateSpecificTray(cp->getTray());

	if (Roots::ShowRootMenu(menu, x, y, 0)) {
		cp->ungrab();
		cp->Redraw();
		cp->UpdateSpecificTray(cp->getTray());
	}
}

/** Process a button release. */
void ActionNode::ProcessActionRelease(struct TrayComponentType *cp, int x,
		int y, int button) {
	const int mask = 1 << button;

	if (JUNLIKELY(menuShown)) {
		return;
	}

	cp->ungrab();
	cp->Redraw();
	cp->UpdateSpecificTray(cp->getTray());

	/* Since we grab the mouse, make sure the mouse is actually over
	 * the button. */
	if (x < -1 || x > cp->getWidth()) {
		return;
	}
	if (y < -1 || y > cp->getHeight()) {
		return;
	}

	/* Run the action (if any). */
	if (this->mask & mask) {
		if (this->action && strlen(this->action) > 0) {
			if (!strncmp(this->action, "exec:", 5)) {
				Commands::RunCommand(this->action + 5);
			} else if (!strcmp(this->action, "showdesktop")) {
				DesktopEnvironment::DefaultEnvironment()->ShowDesktop();
			}
		}
		return;
	}
}

/** Validate actions. */
void ActionNode::ValidateAction() {
	if (this->action && !strncmp(this->action, "root:", 5)) {
		const int bindex = Roots::GetRootMenuIndexFromString(&this->action[5]);
		if (JUNLIKELY(!Roots::IsRootMenuDefined(bindex))) {
			Warning(_("action: root menu \"%s\" not defined"),
					&this->action[5]);
		}
	}
}
