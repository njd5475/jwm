/**
 * @file cursor.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the cursor functions.
 *
 */

#ifndef CURSOR_H
#define CURSOR_H

#include "border.h"

class Cursors {

public:
	/*@{*/
	static void InitializeCursors();
	static void StartupCursors(void);
	static void ShutdownCursors(void);
    static void DestroyCursors();
	/*@}*/

	/** Grab the mouse for resizing a window.
	 * @param action The resize action.
	 * @return 1 on success, 0 on failure.
	 */
	static char GrabMouseForResize(MouseContextType context);

	/** Grab the mouse for moving a window.
	 * @return 1 on success, 0 on failure.
	 */
	static char GrabMouseForMove(void);

	/** Grab the mouse.
	 * @return 1 on success, 0 on failure.
	 */
	static char GrabMouse(Window w);

	/** Grab the mouse to select a window.
	 * @return 1 on success, 0 on failure.
	 */
	static char GrabMouseForChoose(void);

	/** Get the cursor to use given a border action.
	 * @param action The border action.
	 * @return The cursor to use.
	 */
	static Cursor GetFrameCursor(MouseContextType action);

	/** Move the mouse cursor.
	 * @param win The window to act as an origin for the coordinates.
	 * @param x The x-coordinate.
	 * @param y The y-coordinate.
	 */
	static void MoveMouse(Window win, int x, int y);

	/** Set the current mouse position.
	 * @param x The x-coordinate (relative to the current desktop).
	 * @param y The y-coordinate (relative to the current desktop).
	 * @param w The window under the mouse.
	 */
	static void SetMousePosition(int x, int y, Window w);

	/** Get the current mouse position.
	 * @param x Location to store the x-coordinate.
	 * @param y Location to store the y-coordinate.
	 * @param w The window under the mouse.
	 */
	static void GetMousePosition(int *x, int *y, Window *w);

	/** Get a mask of the current mouse buttons pressed.
	 * @return A mask of the current mouse buttons pressed.
	 */
	static unsigned int GetMouseMask(void);

	/** Reset to the default cursor on a window.
	 * @param w The window whose cursor to change.
	 */
	static void SetDefaultCursor(Window w);
private:
	static Cursor GetResizeCursor(MouseContextType context);
	static Cursor CreateCursor(unsigned int shape);
};

#endif /* CURSOR_H */

