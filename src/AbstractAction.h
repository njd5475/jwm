/*
 * AbstractAction.h
 *
 *  Created on: Dec 15, 2018
 *      Author: nick
 */

#ifndef SRC_ABSTRACTACTION_H_
#define SRC_ABSTRACTACTION_H_

typedef enum {
  NONE = 0,
  UP,
  DOWN,
  RIGHT,
  LEFT,
  ESC,
  ENTER,
  NEXT,
  NEXTSTACK,
  PREV,
  PREVSTACK,
  CLOSE,
  MIN,
  MAX,
  SHADE,
  STICK,
  MOVE,
  RESIZE,
  ROOT,
  WIN,
  DESKTOP,
  RDESKTOP,
  LDESKTOP,
  UDESKTOP,
  DDESKTOP,
  SHOWDESK,
  SHOWTRAY,
  EXEC,
  RESTART,
  EXIT,
  FULLSCREEN,
  SEND,
  SENDR,
  SENDL,
  SENDU,
  SENDD,
  MAXTOP,
  MAXBOTTOM,
  MAXLEFT,
  MAXRIGHT,
  MAXV,
  MAXH,
  RESTORE,
  INVALID = 255,
  RESIZE_N = 1, /* Extra value mask for resize north. */
  RESIZE_S = 2, /* Extra value mask for resize south. */
  RESIZE_E = 4, /* Extra value mask for resize east. */
  RESIZE_W = 8, /* Extra value mask for resize west. */
} Actions;

class AbstractAction {
public:
  AbstractAction();
  virtual ~AbstractAction();

  virtual void press() {};
  virtual void release() {};
  virtual bool validate() {return true;};
};

class ActionGroup {
public:
  ActionGroup() {}
  virtual ~ActionGroup() {}

  virtual void add(AbstractAction &action) {
    const unsigned int num = sizeof(_actions)/sizeof(AbstractAction);

  }

private:
  AbstractAction **_actions;
};

#endif /* SRC_ABSTRACTACTION_H_ */
