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

#include <stack>
#include "TrayComponent.h"
#include "event.h"

class SwallowNode : public TrayComponent, EventHandler {
public:
  /** Create a swallowed application tray component.
   * @param name The name of the application to swallow.
   * @param command The command used to start the swallowed application.
   * @param width The width to use (0 for default).
   * @param height the height to use (0 for default).
   */
  SwallowNode(const char *name, const char *command, int width, int height, Tray *tray, TrayComponent *parent);
  virtual ~SwallowNode();

private:
  char *name;
  char *command;
  int border;
  int userWidth;
  int userHeight;

  static std::vector<SwallowNode*> nodes;

  static std::stack<SwallowNode*> pendingNodes;
  static std::vector<SwallowNode*> swallowNodes;

  static void ReleaseNodes(SwallowNode *nodes);
  void Create();
  void Destroy();
  void Resize();
  void Draw(Graphics *g);

  virtual void ProcessButtonPress(int x, int y, int mask) {}
  virtual void ProcessButtonRelease(int x, int y, int mask) {}
  virtual void ProcessMotionEvent(int x, int y, int mask) {}

  /** Process an event on a swallowed window.
   * @param event The event to process.
   * @return 1 if the event was for a swallowed window, 0 if not.
   */
  bool process(const XEvent *event);

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

  /** Determine if there are swallow processes pending.
   * @return 1 if there are still pending swallow processes, 0 otherwise.
   */
  static char IsSwallowPending(void);
};

#endif /* SWALLOW_H */

