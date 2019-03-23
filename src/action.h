/**
 * @file action.h
 * @author Joe Wingbermuehle
 *
 * @brief Tray component actions.
 *
 */

#ifndef ACTION_H
#define ACTION_H

#include "AbstractAction.h"

struct TrayComponentType;

/** Enumeration of actions.
 * Note that we use the high bits to store additional information
 * for some key types (for example the desktop number).
 */
typedef struct ActionType {
  Actions action;
  unsigned char extra;
} ActionType;

/** Add an action to a list of actions.
 * @param actions The action list to update.
 * @param action The action to add to the list.
 * @param mask The mouse button mask.
 */

class ActionNode {

public:
  ActionNode(const char* action, int mask);
  virtual ~ActionNode();

  /** Process a button press event.
   * @param actions The action list.
   * @param cp The tray component.
   * @param x The mouse x-coordinate.
   * @param y The mouse y-coordinate.
   * @param button The mouse button.
   */
  void ProcessActionPress(struct TrayComponentType *cp, int x, int y, int button);

  /** Process a button release event.
   * @param actions The action list.
   * @param cp The tray component.
   * @param x The mouse x-coordinate.
   * @param y The mouse y-coordinate.
   * @param button The mouse button.
   */
  void ProcessActionRelease(struct TrayComponentType *cp, int x, int y, int button);

  /** Validate actions.
   * @param actions The action list to validate.
   */
  void ValidateAction();
private:
  char *action;
  int mask;
};

#endif /* ACTION_H */
