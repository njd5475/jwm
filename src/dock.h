/**
 * @file dock.h
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Dock tray component (used for system notifications).
 *
 */

#ifndef DOCK_H
#define DOCK_H

#include "TrayComponent.h"

/** Structure to represent a docked window. */
typedef struct DockNode {

  Window window;
  char needs_reparent;

  struct DockNode *next;

} DockNode;

/** Structure to represent a dock tray component. */
class DockType  : public TrayComponentType {
public:
  DockType(int width);
  virtual ~DockType() {}

  Window window;
  int itemSize;

  DockNode *nodes;

  static const char *BASE_SELECTION_NAME;

  static DockType *dock;
  static char owner;
  static Atom dockAtom;
  static unsigned long orientation;

  void SetSize(int width, int height);
  void Create();
  void Resize();

  static void _DockWindow(Window win);
  static void _UpdateDock(void);
  static void _GetDockItemSize(int *size);
  static void _GetDockSize(int *width, int *height);

  /*@{*/
  static void _InitializeDock(void);
  static void _StartupDock(void);
  static void _ShutdownDock(void);
  static void _DestroyDock(void);
  /*@}*/

  /** Create a dock to be used for notifications.
   * Note that only one dock can be created.
   * @param width The width of an item in the dock.
   */
  struct TrayComponentType *_CreateDock(int width);

  /** Handle a client message sent to the dock window.
   * @param event The event.
   */
  static void _HandleDockEvent(const XClientMessageEvent *event);

  /** Handle a destroy event.
   * @param win The window that was destroyed.
   * @return 1 if handled, 0 otherwise.
   */
  static char _HandleDockDestroy(Window win);

  /** Handle a selection clear event.
   * @param event The selection clear event.
   * @return 1 if handled, 0 otherwise.
   */
  static char _HandleDockSelectionClear(const XSelectionClearEvent *event);

  /** Handle a resize request.
   * @param event The resize request event.
   * @return 1 if handled, 0 otherwise.
   */
  static char _HandleDockResizeRequest(const XResizeRequestEvent *event);

  /** Handle a configure request.
   * @param event The configure request event.
   * @return 1 if handled, 0 otherwise.
   */
  static char _HandleDockConfigureRequest(const XConfigureRequestEvent *event);

  /** Handle a reparent notify event.
   * @param event The reparent notify event.
   * @return 1 if handled, 0 otherwise.
   */
  static char _HandleDockReparentNotify(const XReparentEvent *event);

};
#endif

