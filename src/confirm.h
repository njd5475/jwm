/**
 * @file confirm.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the confirm dialog functions.
 *
 */

#ifndef CONFIRM_H
#define CONFIRM_H

#include "event.h"

struct ClientNode;

class DialogsEventHandler : public EventHandler {

public:

  /** Handle an event on a dialog window.
   * @param event The event.
   * @return 1 if handled, 0 if not handled.
   */
  bool process(const XEvent* event);
};


class Dialogs {
public:
	/*@{*/
	static void InitializeDialogs();
	static void StartupDialogs();
	static void ShutdownDialogs(void);
	static void DestroyDialogs();
	/*@}*/


	/** Show a confirm dialog.
	 * @param np A client window associated with the dialog.
	 * @param action A callback to run if "OK" is clicked.
	 */
	static void ShowConfirmDialog(struct ClientNode *np,
			void (*action)(struct ClientNode*), ...);

private:

	static DialogsEventHandler handler;
};

#endif /* CONFIRM_H */

