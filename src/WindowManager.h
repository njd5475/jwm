/*
 * WindowManager.h
 *
 *  Created on: Nov 7, 2019
 *      Author: nick
 */

#ifndef SRC_WINDOWMANAGER_H_
#define SRC_WINDOWMANAGER_H_
#include "nwm.h"
#include "ClientListener.h"
#include "HashMap.h"

class Component;
class Graphics;
class Client;
class ComponentGroup;

struct ComponentInfo {
  Pixmap pixmap;
  Window window;
  Graphics* graphics;
  Client* clientNode;
  Component *component;
};

class WindowManager : public ClientListener {
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
	static void SetWaitingPid(int pid);
	static int  GetWaitingPid();

private:
	WindowManager();
	virtual ~WindowManager();

	void Draw();
public:
	void add(Component* component);
	void newClient(Client* node);

	static WindowManager *WM();
private:
	ComponentGroup *_group;
	static WindowManager MANAGER;
	static int _WAITING_PID;
	std::vector<ComponentInfo> _components;
};

#endif /* SRC_WINDOWMANAGER_H_ */
