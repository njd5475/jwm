/**
 * @file icon.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header file for icon functions.
 *
 */

#ifndef ICON_H
#define ICON_H

struct ClientNode;

/** Structure to hold a scaled icon. */
typedef struct ScaledIconNode {

	int width; /**< The scaled width of the icon. */
	int height; /**< The scaled height of the icon. */
	long fg; /**< Foreground color for bitmaps. */

	XID image;
	XID mask;

	struct ScaledIconNode *next;

} ScaledIconNode;

/** Structure to hold an icon. */
typedef struct IconNode {

	char *name; /**< The name of the icon. */
	struct ImageNode *images; /**< Images associated with this icon. */
	struct ScaledIconNode *nodes; /**< Scaled icons. */
	int width; /**< Natural width. */
	int height; /**< Natural height. */

	struct IconNode *next; /**< The next icon in the list. */
	struct IconNode *prev; /**< The previous icon in the list. */

	char preserveAspect; /**< Set to preserve the aspect ratio
	 *   of the icon when scaling. */
	char bitmap; /**< Set if this is a bitmap. */
	char transient; /**< Set if this icon is transient. */
#ifdef USE_XRENDER
   char render;                   /**< Set to use render. */
#endif

} IconNode;

class Icons {
public:

	static IconNode emptyIcon;

	/*@{*/
	static void InitializeIcons(void);
	static void StartupIcons(void);
	static void ShutdownIcons(void);
	static void DestroyIcons(void);
	/*@}*/

	/** Add an icon path.
	 * This adds a path to the list of icon search paths.
	 * @param path The icon path to add.
	 */
	static void AddIconPath(char *path);

	/** Render an icon.
	 * This will scale an icon if necessary to fit the requested size. The
	 * aspect ratio of the icon is preserved.
	 * @param icon The icon to render.
	 * @param d The drawable on which to place the icon.
	 * @param fg The foreground color.
	 * @param x The x offset on the drawable to render the icon.
	 * @param y The y offset on the drawable to render the icon.
	 * @param width The width of the icon to display.
	 * @param height The height of the icon to display.
	 */
	static void PutIcon(IconNode *icon, Drawable d,
			long fg, int x, int y, int width, int height);

	/** Load an icon for a client.
	 * @param np The client.
	 */
	static void LoadIcon(struct ClientNode *np);

	/** Load an icon.
	 * @param name The name of the icon to load.
	 * @param save Set if this icon should be saved in the icon hash.
	 * @param preserveAspect Set to preserve the aspect ratio when scaling.
	 * @return A pointer to the icon (NULL if not found).
	 */
	static IconNode *LoadNamedIcon(const char *name, char save, char preserveAspect);

	/** Load the default icon.
	 * @return The default icon.
	 */
	static IconNode *GetDefaultIcon(void);

	/** Destroy an icon.
	 * @param icon The icon to destroy.
	 */
	static void DestroyIcon(IconNode *icon);

	/** Set the default icon. */
	static void SetDefaultIcon(const char *name);

};

#endif /* ICON_H */

