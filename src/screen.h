/**
 * @file screen.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Header for screen functions.
 *
 * Note that screen here refers to physical monitors. Screens are
 * determined using the xinerama extension (if available). There will
 * always be at least one screen.
 *
 */

#ifndef SCREEN_H
#define SCREEN_H

/** Structure to contain information about a screen. */
typedef struct ScreenType {
	int index; /**< The index of this screen. */
	int x, y; /**< The location of this screen. */
	int width, height; /**< The size of this screen. */
} ScreenType;

class Screens {
public:
	static void InitializeScreens() {}
	static void DestroyScreens() {}
	static void StartupScreens(void);
	static void ShutdownScreens(void);

	/*@{*/
	/*@}*/

	/** Get the screen of the specified coordinates.
	 * @param x The x-coordinate.
	 * @param y The y-coordinate.
	 * @return The screen.
	 */
	static const ScreenType* GetCurrentScreen(int x, int y);

	/** Get the screen containing the mouse.
	 * @return The screen containing the mouse.
	 */
	static const ScreenType* GetMouseScreen(void);

	/** Get the screen of the specified index.
	 * @param index The screen index (0 based).
	 * @return The screen.
	 */
	static const ScreenType* GetScreen(int index);

	/** Get the number of screens.
	 * @return The number of screens.
	 */
	static int GetScreenCount(void);

};
#endif /* SCREEN_H */

