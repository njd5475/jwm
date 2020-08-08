/**
 * @file desktop.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the desktop management functions.
 *
 */

#include "jwm.h"
#include "desktop.h"
#include "main.h"
#include "client.h"
#include "clientlist.h"
#include "error.h"

#include "misc.h"
#include "settings.h"
#include "grab.h"
#include "event.h"
#include "DesktopEnvironment.h"

std::vector<const char*> Desktops::names;
std::vector<bool> Desktops::showing;

/** Startup desktop support. */
void Desktops::_StartupDesktops(void) {

	unsigned int x;

	for (x = 0; x < settings.desktopCount; x++) {
			char defaultName[4];
			memset(defaultName, 0, 4);
			snprintf(defaultName, 4, "w-%d", x + 1);
			names.push_back(strdup(defaultName));
	}

	for (x = 0; x < settings.desktopCount; x++) {
		showing.push_back(false);
	}
}

/** Release desktop data. */
void Desktops::_DestroyDesktops(void) {

	for (int x = 0; x < names.size(); x++) {
		free((void*)names[x]);
	}
	names.clear();
}

/** Get the right desktop. */
unsigned int Desktops::_GetRightDesktop(unsigned int desktop) {
	const int y = desktop / settings.desktopWidth;
	const int x = (desktop + 1) % settings.desktopWidth;
	return y * settings.desktopWidth + x;
}

/** Get the left desktop. */
unsigned int Desktops::_GetLeftDesktop(unsigned int desktop) {
	const int y = currentDesktop / settings.desktopWidth;
	int x = currentDesktop % settings.desktopWidth;
	x = x > 0 ? x - 1 : settings.desktopWidth - 1;
	return y * settings.desktopWidth + x;
}

/** Get the above desktop. */
unsigned int Desktops::_GetAboveDesktop(unsigned int desktop) {
	if (currentDesktop >= settings.desktopWidth) {
		return currentDesktop - settings.desktopWidth;
	}
	return currentDesktop + (settings.desktopHeight - 1) * settings.desktopWidth;
}

/** Get the below desktop. */
unsigned int Desktops::_GetBelowDesktop(unsigned int desktop) {
	return (currentDesktop + settings.desktopWidth) % settings.desktopCount;
}

/** Change to the desktop to the right. */
char Desktops::_RightDesktop(void) {
	if (settings.desktopWidth > 1) {
		const unsigned int desktop = _GetRightDesktop(currentDesktop);
		_ChangeDesktop(desktop);
		return 1;
	} else {
		return 0;
	}
}

/** Change to the desktop to the left. */
char Desktops::_LeftDesktop(void) {
	if (settings.desktopWidth > 1) {
		const unsigned int desktop = _GetLeftDesktop(currentDesktop);
		_ChangeDesktop(desktop);
		return 1;
	} else {
		return 0;
	}
}

/** Change to the desktop above. */
char Desktops::_AboveDesktop(void) {
	if (settings.desktopHeight > 1) {
		const int desktop = _GetAboveDesktop(currentDesktop);
		_ChangeDesktop(desktop);
		return 1;
	} else {
		return 0;
	}
}

/** Change to the desktop below. */
char Desktops::_BelowDesktop(void) {
	if (settings.desktopHeight > 1) {
		const unsigned int desktop = _GetBelowDesktop(currentDesktop);
		_ChangeDesktop(desktop);
		return 1;
	} else {
		return 0;
	}
}

/** Change to the specified desktop. */
void Desktops::_ChangeDesktop(unsigned int desktop) {

	if (JUNLIKELY(desktop >= settings.desktopCount)) {
		return;
	}

	if (currentDesktop == desktop) {
		return;
	}

	/* Hide clients from the old desktop.
	 * Note that we show clients in a separate loop to prevent an issue
	 * with clients losing focus.
	 */
	for (unsigned int x = 0; x < LAYER_COUNT; x++) {
		std::vector<ClientNode*> clients = ClientList::GetLayerList(x);
		for (int i = 0; i < clients.size(); ++i) {
			ClientNode *np = clients[i];
			if (np->isSticky()) {
				continue;
			}
			if (np->getDesktop() == currentDesktop) {
				np->HideClient();
			}
		}
	}

	/* Show clients on the new desktop. */
	for (unsigned int x = 0; x < LAYER_COUNT; x++) {
		std::vector<ClientNode*> clients = ClientList::GetLayerList(x);
		for (int i = 0; i < clients.size(); ++i) {
			ClientNode *np = clients[i];
			if (np->isSticky()) {
				continue;
			}
			if (np->getDesktop() == desktop) {
				np->ShowClient();
			}
		}
	}

	showing[currentDesktop] = false;
	currentDesktop = desktop;
	showing[currentDesktop] = true;

	Hints::SetCardinalAtom(rootWindow, ATOM_NET_CURRENT_DESKTOP, currentDesktop);
	Hints::SetCardinalAtom(rootWindow, ATOM_NET_SHOWING_DESKTOP, showing[currentDesktop]);

	Events::_RequireRestack();
	Events::_RequireTaskUpdate();

	DesktopEnvironment::DefaultEnvironment()->LoadBackground(desktop);

}

/** Toggle the "show desktop" state. */
void Desktops::_ShowDesktop(void) {

	ClientNode *np;
	int layer;

	Grabs::GrabServer();
	for (layer = 0; layer < LAYER_COUNT; layer++) {
		std::vector<ClientNode*> clients = ClientList::GetLayerList(layer);
		for (int i = 0; i < clients.size(); ++i) {
			np = clients[i];
			if (np->shouldSkipInTaskList()) {
				continue;
			}
			if ((np->getDesktop() == currentDesktop) || (np->isSticky())) {
				if (showing[currentDesktop]) {
					if (np->wasMinimizedToShowDesktop()) {
						np->RestoreClient(0);
					}
				} else {
					if (np->isActive()) {
						JXSetInputFocus(display, rootWindow, RevertToParent, CurrentTime);
					}
					if (np->isStatus(STAT_MAPPED | STAT_SHADED)) {
						np->MinimizeClient(0);
						np->setSDesktopStatus();
					}
				}
			}
		}
	}
	Events::_RequireRestack();
	Events::_RequireTaskUpdate();
	JXSync(display, True);

	if (showing[currentDesktop]) {
		char first = 1;
		JXSync(display, False);
		for (layer = 0; layer < LAYER_COUNT; layer++) {
			std::vector<ClientNode*> clients = ClientList::GetLayerList(layer);
			for(int i = 0; i < clients.size(); ++i) {
				np = clients[i];
				if (np->shouldSkipInTaskList()) {
					continue;
				}
				if ((np->getDesktop() == currentDesktop) || (np->isSticky())) {
					if (first) {
						np->keyboardFocus();
						first = 0;
					}
					Border::DrawBorder(np);
				}
			}
		}
		showing[currentDesktop] = 0;
	} else {
		showing[currentDesktop] = 1;
	}
	Hints::SetCardinalAtom(rootWindow, ATOM_NET_SHOWING_DESKTOP, showing[currentDesktop]);
	Grabs::UngrabServer();

}

/** Set the name for a desktop. */
void Desktops::_SetDesktopName(unsigned int desktop, const char *str) {

	if (JUNLIKELY(!str)) {
		Warning(_("empty Desktops Name tag"));
		return;
	}

	Assert(desktop < settings.desktopWidth * settings.desktopHeight);

	Assert(NULL == names[desktop]);

	names[desktop] = CopyString(str);

}

/** Get the name of a desktop. */
const char* Desktops::_GetDesktopName(unsigned int desktop) {
	Assert(desktop < settings.desktopCount);
	if (names[desktop]) {
		return names[desktop];
	} else {
		return "";
	}
}
