/**
 * @file background.h
 * @author Joe Wingbermuehle
 * @date 2007
 *
 * @brief Background control functions.
 *
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

class Backgrounds {
public:
  /*@{*/
  static void _InitializeBackgrounds(void);
  static void _StartupBackgrounds(void);
  static void _ShutdownBackgrounds(void);
  static void _DestroyBackgrounds(void);
  /*@}*/

  /** Set the background to use for the specified desktops.
   * @param desktop The desktop whose background to set (-1 for the default).
   * @param type The type of background.
   * @param value The background.
   */
  static void _SetBackground(int desktop, const char *type, const char *value);

  /** Load the background for the s pecified desktop.
   * @param desktop The current desktop.
   */
  static void _LoadBackground(int desktop);
};

#endif /* BACKGROUND_H */

