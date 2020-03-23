/**
 * @file image.h
 * @author Joe Wingbermuehle
 * @date 2005-2014
 *
 * @brief Functions to load images.
 *
 */

#ifndef IMAGE_H
#define IMAGE_H

#ifndef MAKE_DEPEND

/* We should include png.h here. See jwm.h for an explanation. */

#  ifdef USE_XPM
#     include <X11/xpm.h>
#  endif
#  ifdef USE_JPEG
#     include <jpeglib.h>
#  endif
#  ifdef USE_CAIRO
#     include <cairo.h>
#     include <cairo-svg.h>
#  endif
#  ifdef USE_RSVG
#     include <librsvg/rsvg.h>
#  endif
#endif /* MAKE_DEPEND */

/** Structure to represent an image. */
class ImageNode {

public:
  ImageNode(unsigned width, unsigned height, bool bitmap);
  virtual ~ImageNode();

  int getWidth() const;
  int getHeight() const;
  bool isBitmap() const;
  const unsigned char* getData();
  void CopyFrom(const unsigned long* input, unsigned offset);
#ifdef USE_XRENDER
  bool getRender() const;
#endif

  /** Destroy an image node.
   * @param image The image to destroy.
   */
  static bool DestroyImage(ImageNode *image);

private:
  unsigned char *data; /**< Image data. */
  int width; /**< Width of the image. */
  int height; /**< Height of the image. */
  bool bitmap; /**< 1 if a bitmap, 0 otherwise. */
#ifdef USE_XRENDER
  bool render; /**< 1 to use render, 0 otherwise. */
#endif

private:
  static std::vector<ImageNode*> images;

public:
  /** Load an image from a file.
   * @param fileName The file containing the image.
   * @param rwidth The preferred width.
   * @param rheight The preferred height.
   * @param preserveAspect Set to preserve image aspect when scaling.
   * @return A new image node (NULL if the image could not be loaded).
   */
  static ImageNode *LoadImage(const char *fileName, int rwidth, int rheight,
      char preserveAspect);

  /** Load an image from a Drawable.
   * @param pmap The drawable.
   * @param mask The mask (may be None).
   * @return a new image node (NULL if there were errors).
   */
  static ImageNode *LoadImageFromDrawable(Drawable pmap, Pixmap mask);

  /** Create an image node.
   * @param width The image width.
   * @param height The image height.
   * @param bitmap 1 if a bitmap, 0 otherwise.
   * @return A newly allocated image node.
   */
  static ImageNode *CreateImage(unsigned int width, unsigned int height, char bitmap);

#ifdef USE_CAIRO
#ifdef USE_RSVG
  static ImageNode *LoadSVGImage(const char *fileName, int rwidth, int rheight,
      char preserveAspect);
#endif
#endif
#ifdef USE_JPEG
  static ImageNode *LoadJPEGImage(const char *fileName, int rwidth, int rheight,
      char preserveAspect);
#endif
#ifdef USE_PNG
  static ImageNode *LoadPNGImage(const char *fileName, int rwidth, int rheight,
      char preserveAspect);
#endif
#ifdef USE_XPM
  static ImageNode *LoadXPMImage(const char *fileName, int rwidth, int rheight,
      char preserveAspect);
#endif
#ifdef USE_XBM
  static ImageNode *LoadXBMImage(const char *fileName, int rwidth, int rheight,
      char preserveAspect);
#endif
#ifdef USE_ICONS
  static ImageNode *CreateImageFromXImages(XImage *image, XImage *shape);
#endif

#ifdef USE_XPM

  static int AllocateColor(Display *d, Colormap cmap, char *name,
      XColor *c, void *closure);
  static int FreeColors(Display *d, Colormap cmap, Pixel *pixels, int n,
      void *closure);
#endif

  static void JPEGErrorHandler(j_common_ptr cinfo);
};

#endif /* IMAGE_H */
