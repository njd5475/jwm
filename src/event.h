/**
 * @file event.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the event functions.
 *
 */

#ifndef EVENT_H
#define EVENT_H

#include <vector>
#include "timing.h"
#include "KeyEventHandler.h"
#include "AbstractAction.h"

typedef unsigned char MouseContextType;

typedef unsigned char MaxFlags;

typedef void (*SignalCallback)(const struct TimeType *now, int x, int y,
    Window w, void *data);
class ClientNode;

typedef struct CallbackNode {
  TimeType last;
  int freq;
  SignalCallback callback;
  void *data;
  struct CallbackNode *next;
} CallbackNode;

class EventHandler {
public:
  virtual ~EventHandler() {}
  virtual bool process(const XEvent *event) = 0;
};

class Events {
public:

  static void Bind(KeyEventHandler *handler, Actions action);

  /** Last event time. */
  static Time eventTime;

  /** Wait for an event and process it.
   * @return 1 if there is an event to process, 0 otherwise.
   */
  static char _WaitForEvent(XEvent *event);

  /** Process an event.
   * @param event The event to process.
   */
  static void _ProcessEvent(XEvent *event);

  /** Descard excess button events. */
  static void _DiscardButtonEvents(void);

  /** Discard excess motion events.
   * @param event The event to return.
   * @param w The window whose events to discard.
   */
  static void _DiscardMotionEvents(XEvent *event, Window w);

  /** Discard excess key events.
   * @param event The event to return.
   * @param w The window whose events to discard.
   */
  static void _DiscardKeyEvents(XEvent *event, Window w);

  /** Update the last event time.
   * @param event The event containing the time to use.
   */
  static void _UpdateTime(const XEvent *event);

  /** Register a callback.
   * @param freq The frequency in milliseconds.
   * @param callback The callback function.
   * @param data Data to pass to the callback.
   */
  static void _RegisterCallback(int freq, SignalCallback callback, void *data);

  /** Unregister a callback.
   * @param callback The callback to remove.
   * @param data The data passed to the register function.
   */
  static void _UnregisterCallback(SignalCallback callback, void *data);

  /** Restack clients before waiting for an event. */
  static void _RequireRestack();

  /** Update the task bar before waiting for an event. */
  static void _RequireTaskUpdate();

  /** Update the pager before waiting for an event. */
  static void _RequirePagerUpdate();

  static void registerHandler(EventHandler* handler);

private:

  static std::vector<EventHandler*> handlers;
  static std::vector<CallbackNode*> callbacks;
  static char restack_pending;
  static char task_update_pending;
  static char pager_update_pending;

  static void _Signal(void);

  static void _ProcessBinding(MouseContextType context, ClientNode *np,
      unsigned state, int code, int x, int y);

  static void _HandleConfigureRequest(const XConfigureRequestEvent *event);
  static char _HandleConfigureNotify(const XConfigureEvent *event);
  static char _HandleExpose(const XExposeEvent *event);
  static char _HandlePropertyNotify(const XPropertyEvent *event);
  static void _HandleClientMessage(const XClientMessageEvent *event);
  static void _HandleColormapChange(const XColormapEvent *event);
  static char _HandleDestroyNotify(const XDestroyWindowEvent *event);
  static void _HandleMapRequest(const XMapEvent *event);
  static void _HandleUnmapNotify(const XUnmapEvent *event);
  static void _HandleButtonEvent(const XButtonEvent *event);
  static void _ToggleMaximized(ClientNode *np, MaxFlags flags);
  static void _HandleKeyPress(const XKeyEvent *event);
  static void _HandleKeyRelease(const XKeyEvent *event);
  static void _HandleEnterNotify(const XCrossingEvent *event);
  static void _HandleMotionNotify(const XMotionEvent *event);
  static char _HandleSelectionClear(const XSelectionClearEvent *event);

  static void _HandleNetMoveResize(const XClientMessageEvent *event,
      ClientNode *np);
  static void HandleNetWMMoveResize(const XClientMessageEvent *evnet,
      ClientNode *np);
  static void _HandleNetRestack(const XClientMessageEvent *event, ClientNode *np);
  static void _HandleNetWMState(const XClientMessageEvent *event, ClientNode *np);
  static void _HandleFrameExtentsRequest(const XClientMessageEvent *event);
  static void _DiscardEnterEvents();

#ifdef USE_SHAPE
static void _HandleShapeEvent(const XShapeEvent *event);
#endif
};

#endif /* EVENT_H */

