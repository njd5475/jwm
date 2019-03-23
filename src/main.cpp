/**
 * @file main.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief The main entry point and related JWM functions.
 *
 */

#include "jwm.h"
#include "main.h"
#include "parse.h"
#include "help.h"
#include "error.h"
#include "event.h"

#include "border.h"
#include "client.h"
#include "color.h"
#include "command.h"
#include "cursor.h"
#include "confirm.h"
#include "font.h"
#include "group.h"
#include "icon.h"
#include "taskbar.h"
#include "tray.h"
#include "traybutton.h"
#include "popup.h"
#include "pager.h"
#include "swallow.h"
#include "screen.h"
#include "root.h"
#include "place.h"
#include "clock.h"
#include "misc.h"
#include "settings.h"
#include "timing.h"
#include "grab.h"
#include "battery.h"
#include "binding.h"
#include "AbstractAction.h"
#include "DockComponent.h"
#include "DesktopComponent.h"
#include "BackgroundComponent.h"

#include <fcntl.h>
#include <errno.h>

#include "DesktopEnvironment.h"

static void Initialize(void);
static void Startup(void);
static void Shutdown(void);
static void Destroy(void);

static void CloseConnection(void);
static Bool SelectionReleased(Display *d, XEvent *e, XPointer arg);
static void StartupConnection(void);
static void ShutdownConnection(void);
static void EventLoop(void);
static void HandleExit(int sig);
static void HandleChild(int sig);
static void DoExit(int code);
static void SendRestart(void);
static void SendExit(void);
static void SendReload(void);
static void SendJWMMessage(const char *message);
static void log(int fd, const char *message);

static int fd = 0;
/** The main entry point. */
#ifndef UNIT_TEST
int main(int argc, char *argv[]) {
  fd = open("/var/log/jwm/jwm.log", O_CREAT | O_WRONLY | O_APPEND);

  log(fd, "Hello World!\n");

  int x;
  enum {
    COMMAND_RUN, COMMAND_RESTART, COMMAND_EXIT, COMMAND_RELOAD, COMMAND_PARSE
  } action;

  StartDebug();

  /* Parse command line options. */
  action = COMMAND_RUN;
  for (x = 1; x < argc; x++) {
    if (!strcmp(argv[x], "-v")) {
      DisplayAbout();
      DoExit(0);
    } else if (!strcmp(argv[x], "-h")) {
      DisplayHelp();
      DoExit(0);
    } else if (!strcmp(argv[x], "-p")) {
      action = COMMAND_PARSE;
    } else if (!strcmp(argv[x], "-restart")) {
      action = COMMAND_RESTART;
    } else if (!strcmp(argv[x], "-exit")) {
      action = COMMAND_EXIT;
    } else if (!strcmp(argv[x], "-reload")) {
      action = COMMAND_RELOAD;
    } else if (!strcmp(argv[x], "-display") && x + 1 < argc) {
      displayString = argv[++x];
    } else if (!strcmp(argv[x], "-f") && x + 1 < argc) {
      if (configPath) {
        Release(configPath);
      }
      configPath = CopyString(argv[++x]);
    } else {
      printf("unrecognized option: %s\n", argv[x]);
      DisplayHelp();
      DoExit(1);
    }
  }
  log(fd, "Finished parsing command line options.\n");

  switch (action) {
  case COMMAND_PARSE:
    log(fd, "Initializing\n");
    Initialize();
    ParseConfig(configPath);
    DoExit(0);
  case COMMAND_RESTART:
    log(fd, "Restarting\n");
    SendRestart();
    DoExit(0);
  case COMMAND_EXIT:
    log(fd, "Exiting\n");
    SendExit();
    DoExit(0);
  case COMMAND_RELOAD:
    log(fd, "Reloading\n");
    SendReload();
    DoExit(0);
  default:
    break;
  }

#if defined(HAVE_SETLOCALE) && (defined(ENABLE_NLS) || defined(ENABLE_ICONV))
  setlocale(LC_ALL, "");
#endif
#ifdef HAVE_GETTEXT
  bindtextdomain("jwm", LOCALEDIR);
  textdomain("jwm");
#endif

  //log(fd, "Registering Components\n");
  //environment.RegisterComponent(new DockComponent());
  //environment.RegisterComponent(new DesktopComponent());
  //environment.RegisterComponent(new BackgroundComponent());
  /* The main loop. */
  log(fd, "Starting connection...\n");
  StartupConnection();
  do {

    isRestarting = shouldRestart;
    shouldExit = 0;
    shouldRestart = 0;
    shouldReload = 0;

    /* Prepare JWM components. */
    log(fd, "Initializing...\n");
    Initialize();

    /* Parse the configuration file. */
    log(fd, "Parsing Config...\n");
    ParseConfig(configPath);

    /* Start up the JWM components. */
    log(fd, "Starting up components...\n");
    Startup();

    /* The main event loop. */
    log(fd, "Starting Event Loop\n");
    EventLoop();

    /* Shutdown JWM components. */
    log(fd, "Shutting down components\n");
    Shutdown();

    /* Perform any extra cleanup. */
    log(fd, "Destroying components\n");
    Destroy();

  } while (shouldRestart);
  ShutdownConnection();

  /* If we have a command to execute on shutdown, run it now. */
  if (exitCommand) {
    execl(SHELL_NAME, SHELL_NAME, "-c", exitCommand, NULL);
    Warning(_("exec failed: (%s) %s"), SHELL_NAME, exitCommand);
    DoExit(1);
  } else {
    DoExit(0);
  }

  close(fd);

  /* Control shoud never get here. */
  return -1;

}
#endif

void log(int fd, const char *message) {
  int len = strlen(message) + 1;
  write(fd, message, len);
}

/** Exit with the specified status code. */
void DoExit(int code) {

  Destroy();

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
void EventLoop(void) {

  XEvent event;
  TimeType start;

  /* Loop processing events until it's time to exit. */
  while (JLIKELY(!shouldExit)) {
    if (JLIKELY(_WaitForEvent(&event))) {
      _ProcessEvent(&event);
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
    if (_WaitForEvent(&event)) {
      _ProcessEvent(&event);
    }
  }

}

/** Predicate for XIfEvent to determine if we got the WM_Sn selection. */
Bool SelectionReleased(Display *d, XEvent *e, XPointer arg) {
  if (e->type == DestroyNotify) {
    if (e->xdestroywindow.window == *(Window*) arg) {
      return True;
    }
  }
  return False;
}

/** Prepare the connection. */
void StartupConnection(void) {

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
  if(!environment->OpenConnection()) {
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
  GrabServer();
  win = JXGetSelectionOwner(display, managerSelection);
  if (win != None) {
    JXSelectInput(display, win, StructureNotifyMask);
  }
  JXSetSelectionOwner(display, managerSelection, supportingWindow, CurrentTime);
  UngrabServer();

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
void CloseConnection(void) {
  JXFlush(display);
  JXCloseDisplay(display);
}

/** Close the X server connection. */
void ShutdownConnection(void) {
  CloseConnection();
}

/** Signal handler. */
void HandleExit(int sig) {
  shouldExit = 1;
}

/** Signal handler for SIGCHLD. */
void HandleChild(int sig) {
  const int savedErrno = errno;
  while (waitpid((pid_t) -1, NULL, WNOHANG) > 0)
    ;
  errno = savedErrno;
}

/** Initialize data structures.
 * This is called before the X connection is opened.
 */
#define ILog(fn) \
  log(fd, #fn "\n");\
  fn();

void Initialize(void) {
  ILog(InitializeBindings);
  ILog(InitializeClients);
  ILog(InitializeBattery);
  ILog(InitializeColors);
  ILog(InitializeCommands);
  ILog(InitializeCursors);
#ifndef DISABLE_CONFIRM
  ILog(InitializeDialogs);
#endif
  ILog(DesktopEnvironment::DefaultEnvironment()->InitializeComponents);
  ILog(InitializeFonts);
  ILog(InitializeGroups);
  ILog(InitializeHints);
  ILog(InitializeIcons);
  ILog(PagerType::InitializePager);
  ILog(InitializePlacement);
  ILog(InitializePopup);
  ILog(InitializeRootMenu);
  ILog(InitializeScreens);
  ILog(InitializeSettings);
  ILog(InitializeSwallow);
  ILog(TaskBarType::InitializeTaskBar);
  ILog(TrayType::InitializeTray);
  ILog(InitializeTrayButtons);
}

/** Startup the various JWM components.
 * This is called after the X connection is opened.
 */
void Startup(void) {

  /* This order is important. */

  /* First we grab the server to prevent clients from changing things
   * while we're still loading. */
  GrabServer();

  StartupSettings();
  StartupScreens();

  StartupGroups();
  StartupColors();
  StartupFonts();
  StartupIcons();
  StartupCursors();

  PagerType::StartupPager();
  StartupBattery();
  StartupTaskBar();
  TrayButton::StartupTrayButtons();
  StartupHints();
  DesktopEnvironment::DefaultEnvironment()->StartupComponents();
  TrayType::StartupTray();
  StartupBindings();
  StartupPlacement();

#  ifndef DISABLE_CONFIRM
  StartupDialogs();
#  endif
  StartupPopup();

  StartupRootMenu();

  SetDefaultCursor(rootWindow);
  ReadCurrentDesktop();
  JXFlush(display);

  _RequireRestack();

  /* Allow clients to do their thing. */
  JXSync(display, True);
  UngrabServer();

  SwallowNode::StartupSwallow();

  TrayType::DrawTray();

  /* Send expose events. */
  Border::ExposeCurrentDesktop();

  /* Draw the background (if backgrounds are used). */
  DesktopEnvironment::DefaultEnvironment()->LoadBackground(currentDesktop);

  /* Run any startup commands. */
  StartupCommands();

}

/** Shutdown the various JWM components.
 * This is called before the X connection is closed.
 */
void Shutdown(void) {

  /* This order is important. */

  ShutdownSwallow();

#  ifndef DISABLE_CONFIRM
  ShutdownDialogs();
#  endif
  ShutdownPopup();
  ShutdownBindings();
  PagerType::ShutdownPager();
  ShutdownRootMenu();
  DesktopEnvironment::DefaultEnvironment()->ShutdownComponents();
  TrayType::ShutdownTray();
  ShutdownTrayButtons();
  TaskBarType::ShutdownTaskBar();
  ClockType::ShutdownClock();
  ShutdownBattery();
  ShutdownIcons();
  ShutdownCursors();
  ShutdownFonts();
  ShutdownColors();
  ShutdownGroups();

  ShutdownPlacement();
  ShutdownHints();
  ShutdownScreens();
  ShutdownSettings();

  ShutdownCommands();

}

/** Clean up memory.
 * This is called after the X connection is closed.
 * Note that it is possible for this to be called more than once.
 */
void Destroy(void) {
  DestroyClients();
  DestroyBattery();
  DestroyColors();
  DestroyCommands();
  DestroyCursors();
#ifndef DISABLE_CONFIRM
  DestroyDialogs();
#endif
  DesktopEnvironment::DefaultEnvironment()->DestroyComponents();
  DestroyFonts();
  DestroyGroups();
  DestroyHints();
  DestroyIcons();
  DestroyBindings();
  PagerType::DestroyPager();
  DestroyPlacement();
  DestroyPopup();
  DestroyRootMenu();
  DestroyScreens();
  DestroySettings();
  SwallowNode::DestroySwallow();
  TaskBarType::DestroyTaskBar();
  TrayType::DestroyTray();
  TrayButton::DestroyTrayButtons();
}

/** Send _JWM_RESTART to the root window. */
void SendRestart(void) {
  SendJWMMessage(jwmRestart);
}

/** Send _JWM_EXIT to the root window. */
void SendExit(void) {
  SendJWMMessage(jwmExit);
}

/** Send _JWM_RELOAD to the root window. */
void SendReload(void) {
  SendJWMMessage(jwmReload);
}

/** Send a JWM message to the root window. */
void SendJWMMessage(const char *message) {
  XEvent event;
  if(!environment->OpenConnection()) {
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

