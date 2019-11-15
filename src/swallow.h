/**
 * @file swallow.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Swallow tray component.
 *
 */

#ifndef SWALLOW_H
#define SWALLOW_H

#include "tray.h"

class SwallowNode : public TrayComponentType {
public:
	/** Create a swallowed application tray component.
	 * @param name The name of the application to swallow.
	 * @param command The command used to start the swallowed application.
	 * @param width The width to use (0 for default).
	 * @param height the height to use (0 for default).
	 */
	SwallowNode(const char *name,
			const char *command,
			int width, int height);
	virtual ~SwallowNode() {}

private:
	char *name;
	char *command;
	int border;
	int userWidth;
	int userHeight;

	struct SwallowNode *next;

	static SwallowNode *pendingNodes = NULL;
	static SwallowNode *swallowNodes = NULL;

	static void ReleaseNodes(SwallowNode *nodes);
	void Create();
	void Destroy();
	void Resize();

public:
	/*@{*/
	static void InitializeSwallow() {}
	static void StartupSwallow(void);
	static void ShutdownSwallow() {}
	static void DestroySwallow(void);
	/*@}*/

	/** Determine if a window should be swallowed.
	 * @param win The window.
	 * @return 1 if this window should be swallowed, 0 if not.
	 */
	static char CheckSwallowMap(Window win);

	/** Process an event on a swallowed window.
	 * @param event The event to process.
	 * @return 1 if the event was for a swallowed window, 0 if not.
	 */
	static char ProcessSwallowEvent(const XEvent *event);

	/** Determine if there are swallow processes pending.
	 * @return 1 if there are still pending swallow processes, 0 otherwise.
	 */
	static char IsSwallowPending(void);
};

#endif /* SWALLOW_H */

