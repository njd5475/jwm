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
#include "help.h"
#include "error.h"
#include "Configuration.h"
#include "misc.h"

#include <fcntl.h>
#include <errno.h>

#include "WindowManager.h"
#include "DesktopEnvironment.h"

/** The main entry point. */
#ifndef UNIT_TEST
int main(int argc, char *argv[]) {

	Logger::AddFile("/var/log/jwm/jwm.log");
	Logger::EnableStandardOut();
	Logger::Log("Hello World!\n");

	int x;
	enum {
		COMMAND_RUN,
		COMMAND_RESTART,
		COMMAND_EXIT,
		COMMAND_RELOAD,
		COMMAND_PARSE
	} action;

	StartDebug();

	/* Parse command line options. */
	action = COMMAND_RUN;
	for (x = 1; x < argc; x++) {
		if (!strcmp(argv[x], "-v")) {
			Help::DisplayAbout();
			WindowManager::DoExit(0);
		} else if (!strcmp(argv[x], "-h")) {
			Help::DisplayHelp();
			WindowManager::DoExit(0);
		} else if (!strcmp(argv[x], "-p")) {
			action = COMMAND_PARSE;
		} else if (!strcmp(argv[x], "-restart")) {
			action = COMMAND_RESTART;
		} else if (!strcmp(argv[x], "-exit")) {
			action = COMMAND_EXIT;
		} else if (!strcmp(argv[x], "-reload")) {
			action = COMMAND_RELOAD;
		} else if (!strcmp(argv[x], "-display") && x + 1 < argc) {
			DesktopEnvironment::setDisplayString(argv[++x]);
		} else if (!strcmp(argv[x], "-f") && x + 1 < argc) {
			if (configPath) {
				Release(configPath);
			}
			configPath = CopyString(argv[++x]);
		} else {
			printf("unrecognized option: %s\n", argv[x]);
			Help::DisplayHelp();
			WindowManager::DoExit(1);
		}
	}
	Log("Finished parsing command line options.\n");

	switch (action) {
	case COMMAND_PARSE:
		Log("Initializing\n");
		WindowManager::Initialize();
		Configuration::ParseConfig(configPath);
		WindowManager::DoExit(0);
		break;
	case COMMAND_RESTART:
		Log("Restarting\n");
		WindowManager::SendRestart();
		WindowManager::DoExit(0);
		break;
	case COMMAND_EXIT:
		Log("Exiting\n");
		WindowManager::SendExit();
		WindowManager::DoExit(0);
		break;
	case COMMAND_RELOAD:
		Log("Reloading\n");
		WindowManager::SendReload();
		WindowManager::DoExit(0);
		break;
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

	/* The main loop. */
	Log("Starting connection...\n");
	WindowManager::StartupConnection();
	do {

		isRestarting = shouldRestart;
		shouldExit = 0;
		shouldRestart = 0;
		shouldReload = 0;

		/* Prepare JWM components. */
		Log("Initializing...\n");
		WindowManager::Initialize();

		/* Parse the configuration file. */
		Log("Parsing Config...\n");
		Configuration::ParseConfig(configPath);

		/* Start up the JWM components. */
		Log("Starting up components...\n");
		WindowManager::Startup();

		/* The main event loop. */
		Log("Starting Event Loop\n");
		WindowManager::EventLoop();

		/* Shutdown JWM components. */
		Log("Shutting down components\n");
		WindowManager::Shutdown();

		/* Perform any extra cleanup. */
		Log("Destroying components\n");
		WindowManager::Destroy();

	} while (shouldRestart);
	WindowManager::ShutdownConnection();

	/* If we have a command to execute on shutdown, run it now. */
	if (exitCommand) {
		execl(SHELL_NAME, SHELL_NAME, "-c", exitCommand, NULL);
		Warning(_("exec failed: (%s) %s"), SHELL_NAME, exitCommand);
		WindowManager::DoExit(1);
	} else {
		WindowManager::DoExit(0);
	}

	Logger::Close();

	/* Control shoud never get here. */
	return -1;

}
#endif
