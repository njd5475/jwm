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

#include <vector>
#include <string>

class ClientNode;
class ImageNode;

/** Structure to hold an icon. */
class IconNode {
public:
  IconNode(ImageNode* image, bool preserveAspect, const char* name);
  virtual ~IconNode();

  /** Static funtions */
  static IconNode* CreateIcon(ImageNode *image, bool preserveAspect, const char* name);

  /** Destroy an icon.
   * @param icon The icon to destroy.
   */
  static bool DestroyIcon(IconNode *icon);

  static ImageNode* GetBestImage(IconNode *icon, int rwidth, int rheight);

  static IconNode emptyIcon;

  int getWidth();
  int getHeight();
  ImageNode* getImage() const;
  const char* getName();
  bool isAspectPreserved();
  bool isBitmap() const;
#ifdef USE_XRENDER
  bool useRender() const;
#endif

private:
  ImageNode* image;
  const char *name; /**< The name of the icon. */
  int width; /**< Natural width. */
  int height; /**< Natural height. */

  bool preserveAspect; /**< Set to preserve the aspect ratio
   *   of the icon when scaling. */
  bool bitmap; /**< Set if this is a bitmap. */
  bool transient; /**< Set if this icon is transient. */
#ifdef USE_XRENDER
   bool render;                   /**< Set to use render. */
#endif

private:
  static IconNode* FindIcon(const char *name);

  static std::vector<IconNode*> images;
  static std::vector<std::string> iconPaths;

  static const unsigned EXTENSION_COUNT;
  static const unsigned MAX_EXTENSION_LENGTH;

  static IconNode* ReadNetWMIcon(Window win);
  static IconNode* ReadWMHintIcon(Window win);
  static IconNode* CreateIconFromDrawable(Drawable d, Pixmap mask);
  static IconNode* CreateIconFromBinary(const unsigned long *data,
      unsigned int length);
  static IconNode* LoadNamedIconHelper(const char *name, const char *path,
      char save, char preserveAspect);

  static char iconSizeSet;
  static char *defaultIconName;
protected:
  static GC iconGC;
public:
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

  /** Load an icon for a client.
   * @param np The client.
   */
  static void LoadIcon(ClientNode *np);

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

  /** Set the default icon. */
  static void SetDefaultIcon(const char *name);
};

/** Structure to hold a scaled icon. */
class ScaledIconNode : public IconNode {

public:
  ScaledIconNode(ImageNode* image, bool preserveAspect, const char* name);
  ScaledIconNode(ImageNode* image, bool preserveAspect, const char* name, int newWidth, int newHeight);
  virtual ~ScaledIconNode();

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

  static ScaledIconNode* GetScaledIcon(IconNode *icon, long fg, int rwidth, int rheight);

  /** Put a scaled icon.
   * @param image The image to display.
   * @param node The rendered image to display.
   * @param d The drawable on which to render the icon.
   * @param x The x-coordinate to place the icon.
   * @param y The y-coordinate to place the icon.
   * @param width The width of the icon.
   * @param height The height of the icon.
   * @return 1 if the icon was successfully rendered, 0 otherwise.
   */
  static void PutScaledRenderIcon(IconNode *image,
      ScaledIconNode *node,
      Drawable d, int x, int y, int width, int height);

  /** Create a scaled icon.
   * @param image The image.
   * @param fg The foreground color (for bitmaps).
   * @return The scaled icon.
   */
  static ScaledIconNode *CreateScaledRenderIcon(ImageNode *image, long fg);
private:

  static std::vector<ScaledIconNode*> nodes; /**< Scaled icons. */
  long fg; /**< Foreground color for bitmaps. */

  XID image;
  XID mask;

};

#endif /* ICON_H */

