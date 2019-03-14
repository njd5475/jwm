/*
 * DesktopEnvironment.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_DESKTOPENVIRONMENT_H_
#define SRC_DESKTOPENVIRONMENT_H_

#include "jwm.h"
#include "Component.h"
#include <vector>

struct Menu;
struct TrayComponentType;

class DesktopEnvironment {
public:
  DesktopEnvironment();
  virtual ~DesktopEnvironment();

  virtual void InitializeComponents();
  virtual void StartupComponents();
  virtual void ShutdownComponents();
  virtual void DestroyComponents();
  virtual bool RegisterComponent(Component *component);
  virtual unsigned int ComponentCount() {return _componentCount;}

  virtual void ShowDesktop();
  virtual void ChangeDesktop(short unsigned int num);
  virtual const char* GetDesktopName(short unsigned int num);
  virtual void LoadBackground(unsigned int num);
  virtual bool RightDesktop();
  virtual bool LeftDesktop();
  virtual bool AboveDesktop();
  virtual bool BelowDesktop();
  virtual void SetDesktopName(int num, const char* name);
  virtual void SetBackground(int id, const char* file, char* const name);
  virtual TrayComponentType* CreateDock(int dockId);
  virtual Menu* CreateDesktopMenu(int desktop, void* mem);
  virtual Menu* CreateSendtoMenu(int desktop, void* mem);

  virtual const unsigned GetRightDesktop(short signed int num);
  virtual const unsigned GetLeftDesktop(short signed int num);
  virtual const unsigned GetAboveDesktop(short signed int num);
  virtual const unsigned GetBelowDesktop(short signed int num);

  virtual char HandleDockReparentNotify(const XReparentEvent* event);
  virtual char HandleDockResizeRequest(XResizeRequestEvent* event);
  virtual char HandleDockConfigureRequest(const XConfigureRequestEvent* event);
  virtual char HandleDockDestroy(const Window);
  virtual void HandleDockEvent(const XClientMessageEvent* event);
  virtual char HandleDockSelectionClear(const XSelectionClearEvent* event);
private:
  std::vector<Component*> _components;
  unsigned int _componentCount;
public:
  static DesktopEnvironment *DefaultEnvironment() {
    if(!_instance) {
      _instance = new DesktopEnvironment();
    }
    return _instance;
  }
private:
  static DesktopEnvironment *_instance;
};

#define environment DesktopEnvironment::DefaultEnvironment()

#endif /* SRC_DESKTOPENVIRONMENT_H_ */
