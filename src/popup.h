/**
 * @file popup.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for popup functions.
 *
 */

#ifndef POPUP_H
#define POPUP_H

#include "event.h"
#include "settings.h"

class PopupEventHandler : public EventHandler {
public:
  /** Process a popup event.
   * @param event The event to process.
   * @return 1 if handled, 0 otherwise.
   */
  bool process(const XEvent *event);

};

class Popups {
public:
  /*@{*/
  static void InitializePopup() {}
  static void StartupPopup(void);
  static void ShutdownPopup(void);
  static void DestroyPopup() {}
  /*@}*/

  /** Show a popup window.
   * @param x The x coordinate of the left edge of the popup window.
   * @param y The y coordinate of the bottom edge of the popup window.
   * @param text The text to display in the popup.
   * @param context The popup context.
   */
  static void ShowPopup(int x, int y, const char *text,
      const PopupMaskType context);

private:
  static PopupEventHandler handler;
};

#endif /* POPUP_H */

