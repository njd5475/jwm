/*
 * WindowManager.h
 *
 *  Created on: Nov 7, 2019
 *      Author: nick
 */

#ifndef SRC_WINDOWMANAGER_H_
#define SRC_WINDOWMANAGER_H_
#include "jwm.h"

class WindowManager {
public:
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

	WindowManager();
	virtual ~WindowManager();
};

#endif /* SRC_WINDOWMANAGER_H_ */
