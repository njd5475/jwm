/**
 * @file font.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the font functions.
 *
 */

#ifndef FONT_H
#define FONT_H

#include "color.h"

/** Enumeration of different components that use fonts. */
typedef enum {
	FONT_BORDER = 0,
	FONT_MENU,
	FONT_POPUP,
	FONT_TRAY,
	FONT_PAGER,
	FONT_CLOCK,
	FONT_TASKLIST,
	FONT_TRAYBUTTON,
	FONT_NOOP,
	FONT_COUNT
} FontType;

class Fonts {

public:
	static void InitializeFonts(void);
	static void StartupFonts(void);
	static void ShutdownFonts(void);
	static void DestroyFonts(void);

	/** Set the font to use for a component.
	 * @param type The font component.
	 * @param value The font to use.
	 */
	static void SetFont(FontType type, const char *value);

	/** Render a string.
	 * @param d The drawable on which to render the string.
	 * @param font The font to use.
	 * @param color The text color to use.
	 * @param x The x-coordinate at which to render.
	 * @param y The y-coordinate at which to render.
	 * @param width The maximum width allowed.
	 * @param str The string to render.
	 */
	static void RenderString(Drawable d, FontType font, ColorName color, int x, int y, int width, const char *str);

	/** Get the width of a string.
	 * @param ft The font used to determine the width.
	 * @param str The string whose width to get.
	 * @return The width of the string in pixels.
	 */
	static int GetStringWidth(FontType ft, const char *str);

	/** Get the height of a string.
	 * @param ft The font used to determine the height.
	 * @return The height in pixels.
	 */
	static int GetStringHeight(FontType ft);

	/** Convert a string from UTF-8.
	 * Note that the string passed into this function is freed via Release
	 * if the same string is not returned from the function.
	 */
	static char *ConvertFromUTF8(char *str);
};

#endif /* FONT_H */

