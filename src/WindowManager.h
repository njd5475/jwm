/*
 * WindowManager.h
 *
 *  Created on: Nov 7, 2019
 *      Author: nick
 */

#ifndef SRC_WINDOWMANAGER_H_
#define SRC_WINDOWMANAGER_H_
#include "nwm.h"
#include "HashMap.h"

class Component;
class Graphics;
class ClientNode;

struct ComponentInfo {
  Pixmap pixmap;
  Window window;
  Graphics* graphics;
  ClientNode* clientNode;
  Component *component;
};

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
	static void SendNWMMessage(const char *message);
	static void DrawAll();

private:
	WindowManager();
	virtual ~WindowManager();

public:
	void add(Component* component);

	static WindowManager *WM();
private:
	static WindowManager MANAGER;
	std::vector<ComponentInfo> _components;
};

#endif /* SRC_WINDOWMANAGER_H_ */
