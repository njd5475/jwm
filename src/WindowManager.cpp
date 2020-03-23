/*
 * WindowManager.cpp
 *
 *  Created on: Nov 7, 2019
 *      Author: nick
 */

#include "WindowManager.h"

#include <errno.h>
#include <sys/wait.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "battery.h"
#include "binding.h"
#include "border.h"
#include "BackgroundComponent.h"
#include "clock.h"
#include "color.h"
#include "command.h"
#include "confirm.h"
#include "cursor.h"
#include "debug.h"
#include "DesktopComponent.h"
#include "DesktopEnvironment.h"
#include "DockComponent.h"
#include "error.h"
#include "event.h"
#include "font.h"
#include "grab.h"
#include "group.h"
#include "hint.h"
#include "icon.h"
#include "jxlib.h"
#include "logger.h"
#include "LogWindow.h"
#include "main.h"
#include "pager.h"
#include "place.h"
#include "popup.h"
#include "root.h"
#include "screen.h"
#include "settings.h"
#include "swallow.h"
#include "taskbar.h"
#include "timing.h"
#include "tray.h"
#include "traybutton.h"
#include "Flex.h"

WindowManager::WindowManager() {


}

WindowManager::~WindowManager() {

}

void WindowManager::Initialize(void) {

	Log("Registering Components\n");
	DesktopEnvironment::DefaultEnvironment()->RegisterComponent(new DockComponent());
	DesktopEnvironment::DefaultEnvironment()->RegisterComponent(new DesktopComponent());
	DesktopEnvironment::DefaultEnvironment()->RegisterComponent(new BackgroundComponent());

	ILog(Binding::InitializeBindings);
	ILog(ClientNode::InitializeClients);
	ILog(Battery::InitializeBattery);
	ILog(Colors::InitializeColors);
	ILog(Commands::InitializeCommands);
	ILog(Cursors::InitializeCursors);
#ifndef DISABLE_CONFIRM
	ILog(Dialogs::InitializeDialogs);
#endif
	ILog(DesktopEnvironment::DefaultEnvironment()->InitializeComponents);
	ILog(Fonts::InitializeFonts);
	ILog(Groups::InitializeGroups);
	ILog(Hints::InitializeHints);
	ILog(Icon::InitializeIcons);
	ILog(PagerType::InitializePager);
	ILog(Places::InitializePlacement);
	ILog(Popups::InitializePopup);
	ILog(Roots::InitializeRootMenu);
	ILog(Screens::InitializeScreens);
	ILog(Setting::InitializeSettings);
	ILog(SwallowNode::InitializeSwallow);
	ILog(TaskBar::InitializeTaskBar);
	ILog(Tray::InitializeTray);
	ILog(TrayButton::InitializeTrayButtons);
}

/** Startup the various JWM components.
 * This is called after the X connection is opened.
 */
void WindowManager::Startup(void) {

	/* This order is important. */

	/* First we grab the server to prevent clients from changing things
	 * while we're still loading. */
	Grabs::GrabServer();

	Setting::StartupSettings();
	Screens::StartupScreens();

	Groups::StartupGroups();
	Colors::StartupColors();
	Fonts::StartupFonts();
	Icon::StartupIcons();
	Cursors::StartupCursors();

	PagerType::StartupPager();
	Battery::StartupBattery();
	TaskBar::StartupTaskBar();
	TrayButton::StartupTrayButtons();
	DesktopEnvironment::DefaultEnvironment()->StartupComponents();
	Hints::StartupHints();
	Tray::StartupTray();
	Binding::StartupBindings();
	Places::StartupPlacement();

#  ifndef DISABLE_CONFIRM
	Dialogs::StartupDialogs();
#  endif
	Popups::StartupPopup();

	Roots::StartupRootMenu();

	Cursors::SetDefaultCursor(rootWindow);
	Hints::ReadCurrentDesktop();
	JXFlush(display);

	Events::_RequireRestack();

	/* Allow clients to do their thing. */
	JXSync(display, True);
	Grabs::UngrabServer();

	ClientNode::StartupClients();
	SwallowNode::StartupSwallow();

	Tray::DrawTray();

	/* Send expose events. */
	Border::ExposeCurrentDesktop();

	/* Draw the background (if backgrounds are used). */
	DesktopEnvironment::DefaultEnvironment()->LoadBackground(currentDesktop);

	/* Run any startup commands. */
	Commands::StartupCommands();

//	LogWindow::Add(30, 30, 300, 200);
	LogWindow::StartupPortals();
//	LogWindow::DrawAll();

	Flex::Create();
}

/** Shutdown the various JWM components.
 * This is called before the X connection is closed.
 */
void WindowManager::Shutdown(void) {

	/* This order is important. */

	SwallowNode::ShutdownSwallow();

#  ifndef DISABLE_CONFIRM
	Dialogs::ShutdownDialogs();
#  endif
	Popups::ShutdownPopup();
	LogWindow::ShutdownPortals();
	Binding::ShutdownBindings();
	PagerType::ShutdownPager();
	Roots::ShutdownRootMenu();
	ClientNode::ShutdownClients();
	DesktopEnvironment::DefaultEnvironment()->ShutdownComponents();
	Tray::ShutdownTray();
	TrayButton::ShutdownTrayButtons();
	TaskBar::ShutdownTaskBar();
	ClockType::ShutdownClock();
	Battery::ShutdownBattery();
	Icon::ShutdownIcons();
	Cursors::ShutdownCursors();
	Fonts::ShutdownFonts();
	Colors::ShutdownColors();
	Groups::ShutdownGroups();

	Places::ShutdownPlacement();
	Hints::ShutdownHints();
	Screens::ShutdownScreens();
	Setting::ShutdownSettings();

  Commands::ShutdownCommands();
}

/** Clean up memory.
 * This is called after the X connection is closed.
 * Note that it is possible for this to be called more than once.
 */
void WindowManager::Destroy(void) {
	ClientNode::DestroyClients();
	Battery::DestroyBattery();
	Colors::DestroyColors();
	Commands::DestroyCommands();
	Cursors::DestroyCursors();
#ifndef DISABLE_CONFIRM
	Dialogs::DestroyDialogs();
#endif
	DesktopEnvironment::DefaultEnvironment()->DestroyComponents();
	Fonts::DestroyFonts();
	Groups::DestroyGroups();
	Hints::DestroyHints();
	Icon::DestroyIcons();
	Binding::DestroyBindings();
	PagerType::DestroyPager();
	Places::DestroyPlacement();
	Popups::DestroyPopup();
	Roots::DestroyRootMenu();
	Screens::DestroyScreens();
	Setting::DestroySettings();
	SwallowNode::DestroySwallow();
	TaskBar::DestroyTaskBar();
	TrayButton::DestroyTrayButtons();
	Tray::DestroyTray();
	Flex::DestroyFlexes();
}

/** Send _JWM_RESTART to the root window. */
void WindowManager::SendRestart(void) {
	SendJWMMessage(jwmRestart);
}

/** Send _JWM_EXIT to the root window. */
void WindowManager::SendExit(void) {
	SendJWMMessage(jwmExit);
}

/** Send _JWM_RELOAD to the root window. */
void WindowManager::SendReload(void) {
	SendJWMMessage(jwmReload);
}

/** Send a JWM message to the root window. */
void WindowManager::SendJWMMessage(const char *message) {
	XEvent event;
	if (!environment->OpenConnection()) {
		DoExit(1);
	}
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = rootWindow;
	event.xclient.message_type = JXInternAtom(display, message, False);
	event.xclient.format = 32;
	JXSendEvent(display, rootWindow, False, SubstructureRedirectMask, &event);
	CloseConnection();
}

/** Prepare the connection. */
void WindowManager::StartupConnection(void) {

	XSetWindowAttributes attr;
#ifdef USE_SHAPE
	int shapeError;
#endif
#ifdef USE_XRENDER
	int renderEvent;
	int renderError;
#endif
	struct sigaction sa;
	char name[32];
	Window win;
	XEvent event;
	int revert;

	initializing = 1;
	if (!environment->OpenConnection()) {
		DoExit(1);
	}

#if 0
  XSynchronize(display, True);
#endif

	/* Create the supporting window used to verify JWM is running. */
	supportingWindow = JXCreateSimpleWindow(display, rootWindow, 0, 0, 1, 1, 0, 0, 0);

	/* Get the atom used for the window manager selection. */
	snprintf(name, 32, "WM_S%d", rootScreen);
	managerSelection = JXInternAtom(display, name, False);

	/* Get the current window manager and take the selection. */
	Grabs::GrabServer();
	win = JXGetSelectionOwner(display, managerSelection);
	if (win != None) {
		JXSelectInput(display, win, StructureNotifyMask);
	}
	JXSetSelectionOwner(display, managerSelection, supportingWindow, CurrentTime);
	Grabs::UngrabServer();

	/* Wait for the current selection owner to give up the selection. */
	if (win != None) {
		/* Note that we need to wait for the current selection owner
		 * to exit before we can expect to select SubstructureRedirectMask. */
		XIfEvent(display, &event, SelectionReleased, (XPointer) &win);
		JXSync(display, False);
	}

	event.xclient.display = display;
	event.xclient.type = ClientMessage;
	event.xclient.window = rootWindow;
	event.xclient.message_type = JXInternAtom(display, managerProperty, False);
	event.xclient.format = 32;
	event.xclient.data.l[0] = CurrentTime;
	event.xclient.data.l[1] = managerSelection;
	event.xclient.data.l[2] = supportingWindow;
	event.xclient.data.l[3] = 2;
	event.xclient.data.l[4] = 0;
	JXSendEvent(display, rootWindow, False, StructureNotifyMask, &event);
	JXSync(display, False);

	JXSetErrorHandler(ErrorHandler);

	clientContext = XUniqueContext();
	frameContext = XUniqueContext();

	/* Set the events we want for the root window.
	 * Note that asking for SubstructureRedirect will fail
	 * if another window manager is already running.
	 */
	attr.event_mask = SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask
			| ColormapChangeMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | PointerMotionHintMask;
	JXChangeWindowAttributes(display, rootWindow, CWEventMask, &attr);

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = 0;
	sa.sa_handler = HandleExit;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	sa.sa_handler = HandleChild;
	sigaction(SIGCHLD, &sa, NULL);

#ifdef USE_SHAPE
	haveShape = JXShapeQueryExtension(display, &shapeEvent, &shapeError);
	if (haveShape) {
		Debug("shape extension enabled");
	} else {
		Debug("shape extension disabled");
	}
#endif

#ifdef USE_XRENDER
	haveRender = JXRenderQueryExtension(display, &renderEvent, &renderError);
	if (haveRender) {
		Debug("render extension enabled");
	} else {
		Debug("render extension disabled");
	}
#endif

	/* Make sure we have input focus. */
	win = None;
	JXGetInputFocus(display, &win, &revert);
	if (win == None) {
		JXSetInputFocus(display, rootWindow, RevertToParent, CurrentTime);
	}

	initializing = 0;

}

/** Close the X server connection. */
void WindowManager::CloseConnection(void) {
	JXFlush(display);
	JXCloseDisplay(display);
}

/** Close the X server connection. */
void WindowManager::ShutdownConnection(void) {
	CloseConnection();
}

/** Signal handler. */
void WindowManager::HandleExit(int sig) {
	shouldExit = 1;
}

/** Signal handler for SIGCHLD. */
void WindowManager::HandleChild(int sig) {
	const int savedErrno = errno;
	while (waitpid((pid_t) -1, NULL, WNOHANG) > 0)
		;
	errno = savedErrno;
}

/** Exit with the specified status code. */
void WindowManager::DoExit(int code) {

	WindowManager::Destroy();

	if (configPath) {
		Release(configPath);
		configPath = NULL;
	}
	if (exitCommand) {
		Release(exitCommand);
		exitCommand = NULL;
	}

	StopDebug();
	exit(code);
}

/** Main JWM event loop. */
void WindowManager::EventLoop(void) {

	XEvent event;
	TimeType start;

	/* Loop processing events until it's time to exit. */
	while (JLIKELY(!shouldExit)) {
		if (JLIKELY(Events::_WaitForEvent(&event))) {
		  Events::_ProcessEvent(&event);
		}
	}

	/* Process events one last time. */
	GetCurrentTime(&start);
	for (;;) {
		if (JXPending(display) == 0) {
			if (!SwallowNode::IsSwallowPending()) {
				break;
			} else {
				TimeType now;
				GetCurrentTime(&now);
				if (GetTimeDifference(&start, &now) > RESTART_DELAY) {
					break;
				}
			}
		}
		if (Events::_WaitForEvent(&event)) {
		  Events::_ProcessEvent(&event);
		}
	}

}

/** Predicate for XIfEvent to determine if we got the WM_Sn selection. */
Bool WindowManager::SelectionReleased(Display *d, XEvent *e, XPointer arg) {
	if (e->type == DestroyNotify) {
		if (e->xdestroywindow.window == *(Window*) arg) {
			return True;
		}
	}
	return False;
}

