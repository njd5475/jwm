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

class TrayComponent;
class Tray;

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

  virtual bool OpenConnection();
  virtual void ShowDesktop();
  virtual void ChangeDesktop(short unsigned int num);
  virtual const char* GetDesktopName(short unsigned int num);
  virtual void LoadBackground(unsigned int num);
  virtual bool RightDesktop();
  virtual bool LeftDesktop();
  virtual bool AboveDesktop();
  virtual bool BelowDesktop();
  virtual void SetDesktopName(int num, const char* name);
  virtual void SetBackground(int id, const char* file, char* const value);

  virtual const unsigned GetRightDesktop(short signed int num);
  virtual const unsigned GetLeftDesktop(short signed int num);
  virtual const unsigned GetAboveDesktop(short signed int num);
  virtual const unsigned GetBelowDesktop(short signed int num);

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
  static void setDisplayString(char* str) {
	  displayString = str;
  }
private:
  static char *displayString;
  static DesktopEnvironment *_instance;
};

#define environment DesktopEnvironment::DefaultEnvironment()

#endif /* SRC_DESKTOPENVIRONMENT_H_ */
