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

/*@{*/
void _InitializeBackgrounds(void);
void _StartupBackgrounds(void);
void _ShutdownBackgrounds(void);
void _DestroyBackgrounds(void);
/*@}*/

/** Set the background to use for the specified desktops.
 * @param desktop The desktop whose background to set (-1 for the default).
 * @param type The type of background.
 * @param value The background.
 */
void _SetBackground(int desktop, const char *type, const char *value);

/** Load the background for the specified desktop.
 * @param desktop The current desktop.
 */
void _LoadBackground(int desktop);

#endif /* BACKGROUND_H */

