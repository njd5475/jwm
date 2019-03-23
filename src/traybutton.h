/**
 * @file traybutton.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Button tray component.
 *
 */

#ifndef TRAY_BUTTON_H
#define TRAY_BUTTON_H

struct TrayComponentType;
struct TimeType;
struct IconNode;

#include "tray.h"

class TrayButton : public TrayComponentType {

public:
  /*@{*/
#define InitializeTrayButtons()  (void)(0)
  static void StartupTrayButtons(void);
#define ShutdownTrayButtons()    (void)(0)
  static void DestroyTrayButtons(void);
  /*@}*/

  /** Create a tray button component.
   * @param iconName The name of the icon to use for the button.
   * @param label The label to use for the button.
   * @param popup Text to display in a popup window.
   * @param width The width to use for the button (0 for default).
   * @param height The height to use for the button (0 for default).
   * @return A new tray button component.
   */
  TrayButton(const char *iconName,
      const char *label,
      const char *popup,
      unsigned int width,
      unsigned int height);

  char *label;
  char *popup;
  char *iconName;
  IconNode *icon;

  int mousex;
  int mousey;
  TimeType mouseTime;

  struct TrayButton *next;

  static TrayButton *buttons = NULL;

  static void Create(TrayComponentType *cp);
  static void Destroy(TrayComponentType *cp);
  static void SetSize(TrayComponentType *cp, int width, int height);
  static void Resize(TrayComponentType *cp);
  static void Draw(TrayComponentType *cp);

  static void ProcessButtonPress(TrayComponentType *cp, int x, int y, int button);
  static void ProcessButtonRelease(TrayComponentType *cp, int x, int y, int button);
  static void ProcessMotionEvent(TrayComponentType *cp, int x, int y, int mask);
  static void SignalTrayButton(const TimeType *now, int x, int y, Window w, void *data);
};

#endif /* TRAY_BUTTON_H */

