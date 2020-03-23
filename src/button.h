/**
 * @file button.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for rendering buttons.
 *
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "font.h"
#include "settings.h"

class Icon;

/** Button types. */
typedef unsigned char ButtonType;
#define BUTTON_LABEL       0  /**< Label. */
#define BUTTON_MENU        1  /**< Menu item. */
#define BUTTON_MENU_ACTIVE 2  /**< Active menu item. */
#define BUTTON_TRAY        3  /**< Inactive tray button. */
#define BUTTON_TRAY_ACTIVE 4  /**< Active tray button. */
#define BUTTON_TASK        5  /**< Item in the task list. */
#define BUTTON_TASK_ACTIVE 6  /**< Active item in the task list. */

/** Draw a button.
 * @param bp The button to draw.
 */
void DrawButton(ButtonType type, AlignmentType alignment, FontType font, const char *text, bool fill,
		bool border, Drawable drawable, Icon *icon, int x, int y, int width, int height,
		int xoffset, int yoffset);

#endif /* BUTTON_H */
