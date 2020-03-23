/**
 * @file icon.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Icon functions.
 *
 */

#include "jwm.h"
#include "icon.h"
#include "client.h"
#include "main.h"
#include "image.h"
#include "misc.h"
#include "hint.h"
#include "color.h"
#include "settings.h"
#include "border.h"

std::vector<IconNode*> IconNode::images;
std::vector<std::string> IconNode::iconPaths;

GC IconNode::iconGC;

IconNode IconNode::emptyIcon(new Image(1, 1, true), true, "EmptyIcon");

#ifdef USE_ICONS

/* These extensions are appended to icon names during search. */
const char *ICON_EXTENSIONS[] = { "",
#ifdef USE_PNG
    ".png", ".PNG",
#endif
#if defined(USE_CAIRO) && defined(USE_RSVG)
    ".svg", ".SVG",
#endif
#ifdef USE_XPM
    ".xpm", ".XPM",
#endif
#ifdef USE_JPEG
    ".jpg", ".JPG", ".jpeg", ".JPEG",
#endif
#ifdef USE_XBM
    ".xbm", ".XBM",
#endif
    };

const unsigned IconNode::EXTENSION_COUNT = ARRAY_LENGTH(ICON_EXTENSIONS);

const unsigned IconNode::MAX_EXTENSION_LENGTH = 5;

char IconNode::iconSizeSet = 0;

char *IconNode::defaultIconName = "DefaultIconName";

/** Initialize icon data.
 * This must be initialized before parsing the configuration.
 */
void IconNode::InitializeIcons(void) {
  unsigned int x;
  memset(&emptyIcon, 0, sizeof(emptyIcon));
  iconSizeSet = 0;
  defaultIconName = NULL;
}

/** Startup icon support. */
void IconNode::StartupIcons(void) {
  XGCValues gcValues;
  XIconSize iconSize;
  unsigned long gcMask;
  gcMask = GCGraphicsExposures;
  gcValues.graphics_exposures = False;
  iconGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

  iconSize.min_width = Border::GetBorderIconSize();
  iconSize.min_height = Border::GetBorderIconSize();
  iconSize.max_width = iconSize.min_width;
  iconSize.max_height = iconSize.min_height;
  iconSize.width_inc = 1;
  iconSize.height_inc = 1;
  JXSetIconSizes(display, rootWindow, &iconSize, 1);
}

/** Shutdown icon support. */
void IconNode::ShutdownIcons(void) {
  for (auto icon : images) {
    delete icon;
  }

  JXFreeGC(display, iconGC);
}

/** Destroy icon data. */
void IconNode::DestroyIcons(void) {

  if (defaultIconName) {
    Release(defaultIconName);
    defaultIconName = NULL;
  }
}

/** Add an icon search path. */
void IconNode::AddIconPath(const char *cpath) {
  if (!cpath) {
    return;
  }

  char path[strlen(cpath)];
  memcpy(path, cpath, strlen(cpath)* sizeof(char));
  Trim(path);

  int length = strlen(path);
  unsigned short addSep = (length >= 1 && path[length - 1] != '/') ? 1 : 0;

  char *ippath = new char[length + addSep + 1];
  memset(ippath, 0, length + addSep + 1);
  strncpy(ippath, path, length + 1);
  if (addSep) {
    ippath[length] = '/';
    ippath[length + 1] = 0;
  }
  ExpandPath(&ippath);

  iconPaths.push_back(std::string(ippath));
}

/** Draw an icon. */
void ScaledIconNode::PutIcon(IconNode *icon, Drawable d, long fg, int x, int y,
    int width, int height) {
  ScaledIconNode *node;

  Assert(icon);

  if (icon == &emptyIcon || icon == NULL) {
    return;
  }

  /* Scale the icon. */
  node = ScaledIconNode::GetScaledIcon(icon, fg, width, height);
  if (node) {

    /* If we support xrender, use it. */
#ifdef USE_XRENDER
    if (icon->useRender()) {
      PutScaledRenderIcon(icon, node, d, x, y, width, height);
      return;
    }
#endif

    /* Draw the icon the old way. */
    if (node->getImage() != None) {

      const int ix = x + (width - node->getWidth()) / 2;
      const int iy = y + (height - node->getHeight()) / 2;

      /* Set the clip mask. */
      if (node->mask != None) {
        JXSetClipOrigin(display, iconGC, ix, iy);
        JXSetClipMask(display, iconGC, node->mask);
      }

      /* Draw the icon. */
      JXCopyArea(display, node->image, d, iconGC, 0, 0, node->getWidth(),
          node->getHeight(), ix, iy);

      /* Reset the clip mask. */
      if (node->mask != None) {
        JXSetClipMask(display, iconGC, None);
        JXSetClipOrigin(display, iconGC, 0, 0);
      }

    }

  }

}

/** Draw a scaled icon. */
void ScaledIconNode::PutScaledRenderIcon(IconNode *icon, ScaledIconNode *node,
    Drawable d, int x, int y, int width, int height) {

#ifdef USE_XRENDER

  Picture source;

  Assert(icon);
  Assert(haveRender);

  source = node->image;
  if (source != None) {

    XRenderPictureAttributes pa;
    XTransform xf;
    int xscale, yscale;
    int nwidth, nheight;
    Picture dest;
    Picture alpha = node->mask;
    XRenderPictFormat *fp = JXRenderFindVisualFormat(display, rootVisual);
    Assert(fp);

    pa.subwindow_mode = IncludeInferiors;
    dest = JXRenderCreatePicture(display, d, fp, CPSubwindowMode, &pa);

    width = width == 0 ? node->getWidth() : width;
    height = height == 0 ? node->getHeight() : height;
    if (icon->isAspectPreserved()) {
      const int ratio = (icon->getWidth() << 16) / icon->getHeight();
      nwidth = Min(width, (height * ratio) >> 16);
      nheight = Min(height, (nwidth << 16) / ratio);
      nwidth = (nheight * ratio) >> 16;
      nwidth = Max(1, nwidth);
      nheight = Max(1, nheight);
      x += (width - nwidth) / 2;
      y += (height - nheight) / 2;
    } else {
      nwidth = width;
      nheight = height;
    }
    xscale = (node->getWidth() << 16) / nwidth;
    yscale = (node->getHeight() << 16) / nheight;

    memset(&xf, 0, sizeof(xf));
    xf.matrix[0][0] = xscale;
    xf.matrix[1][1] = yscale;
    xf.matrix[2][2] = 65536;
    XRenderSetPictureTransform(display, source, &xf);
    XRenderSetPictureFilter(display, source, FilterBest, NULL, 0);
    XRenderSetPictureTransform(display, alpha, &xf);
    XRenderSetPictureFilter(display, alpha, FilterBest, NULL, 0);

    JXRenderComposite(display, PictOpOver, source, alpha, dest, 0, 0, 0, 0, x,
        y, width, height);

    JXRenderFreePicture(display, dest);

  }

#endif

}

/** Create a scaled icon. */
ScaledIconNode* ScaledIconNode::CreateScaledRenderIcon(Image *image,
    long fg) {

  ScaledIconNode *result = NULL;

#ifdef USE_XRENDER

  XRenderPictFormat *fp;
  XColor color;
  GC maskGC;
  XImage *destImage;
  XImage *destMask;
  Pixmap pmap, mask;
  const unsigned width = image->getWidth();
  const unsigned height = image->getHeight();
  unsigned perLine;
  int x, y;
  int maskLine;

  Assert(haveRender);

  result = new ScaledIconNode(image, false, "Scaled", width, height);
  result->fg = fg;

  mask = JXCreatePixmap(display, rootWindow, width, height, 8);
  maskGC = JXCreateGC(display, mask, 0, NULL);
  pmap = JXCreatePixmap(display, rootWindow, width, height, rootDepth);

  destImage = JXCreateImage(display, rootVisual, rootDepth, ZPixmap, 0, NULL,
      width, height, 8, 0);
  destImage->data = new char[sizeof(long unsigned int) * width * height];

  destMask = JXCreateImage(display, rootVisual, 8, ZPixmap, 0, NULL, width,
      height, 8, 0);
  destMask->data = new char[width * height];

  if (image->isBitmap()) {
    perLine = (image->getWidth() >> 3) + ((image->getWidth() & 7) ? 1 : 0);
  } else {
    perLine = image->getWidth();
  }
  maskLine = 0;
  for (y = 0; y < height; y++) {
    const int yindex = y * perLine;
    for (x = 0; x < width; x++) {
      if (image->isBitmap()) {

        const int offset = yindex + (x >> 3);
        const int mask = 1 << (x & 7);
        unsigned long alpha = 0;
        if (image->getData()[offset] & mask) {
          alpha = 255;
          XPutPixel(destImage, x, y, fg);
        }
        destMask->data[maskLine + x] = alpha;

      } else {

        const int index = 4 * (yindex + x);
        const unsigned long alpha = image->getData()[index];
        color.red = image->getData()[index + 1];
        color.red |= color.red << 8;
        color.green = image->getData()[index + 2];
        color.green |= color.green << 8;
        color.blue = image->getData()[index + 3];
        color.blue |= color.blue << 8;

        color.red = (color.red * alpha) >> 8;
        color.green = (color.green * alpha) >> 8;
        color.blue = (color.blue * alpha) >> 8;

        Colors::GetColor(&color);
        XPutPixel(destImage, x, y, color.pixel);
        destMask->data[maskLine + x] = alpha;
      }
    }
    maskLine += destMask->bytes_per_line;
  }

  /* Render the image data to the image pixmap. */
  JXPutImage(display, pmap, rootGC, destImage, 0, 0, 0, 0, width, height);
  delete[] (destImage->data);
  destImage->data = NULL;
  JXDestroyImage(destImage);

  /* Render the alpha data to the mask pixmap. */
  JXPutImage(display, mask, maskGC, destMask, 0, 0, 0, 0, width, height);
  delete[] (destMask->data);
  destMask->data = NULL;
  JXDestroyImage(destMask);
  JXFreeGC(display, maskGC);

  /* Create the alpha picture. */
  fp = JXRenderFindStandardFormat(display, PictStandardA8);
  Assert(fp);
  result->mask = JXRenderCreatePicture(display, mask, fp, 0, NULL);
  JXFreePixmap(display, mask);

  /* Create the render picture. */
  fp = JXRenderFindVisualFormat(display, rootVisual);
  Assert(fp);
  result->image = JXRenderCreatePicture(display, pmap, fp, 0, NULL);
  JXFreePixmap(display, pmap);

#endif

  nodes.push_back(result);
  return result;

}

/** Load the icon for a client. */
void IconNode::LoadIcon(ClientNode *np) {
  IconNode *ico = np->getIcon();
  /* If client already has an icon, destroy it first. */
  IconNode::DestroyIcon(ico);
  ico = NULL;

  /* Attempt to read _NET_WM_ICON for an icon. */
  ico = ReadNetWMIcon(np->getWindow());
  if (ico) {
    np->setIcon(ico);
    return;
  }
  if (np->getOwner() != None) {
    ico = ReadNetWMIcon(np->getOwner());
    if (ico) {
      np->setIcon(ico);
      return;
    }
  }

  /* Attempt to read an icon from XWMHints. */
  ico = ReadWMHintIcon(np->getWindow());
  if (ico) {
    np->setIcon(ico);
    return;
  }
  if (np->getOwner() != None) {
    ico = ReadNetWMIcon(np->getOwner());
    if (ico) {
      np->setIcon(ico);
      return;
    }
  }

  /* Attempt to read an icon based on the window name. */
  if (np->getInstanceName()) {
    ico = IconNode::LoadNamedIcon(np->getInstanceName(), 1, 1);
    if (ico) {
      np->setIcon(ico);
      return;
    }
  }
}

/** Load an icon from a file. */
IconNode* IconNode::LoadNamedIcon(const char *name, char save,
    char preserveAspect) {

  IconNode *icon;

  Assert(name);

  /* If no icon is specified, return an empty icon. */
  if (name[0] == 0) {
    return &emptyIcon;
  }

  /* See if this icon has already been loaded. */
  icon = FindIcon(name);
  if (icon) {
    return icon;
  }

  /* Check for an absolute file name. */
  if (name[0] == '/') {
    Image *image = Image::LoadImage(name, 0, 0, 1);
    if (image) {
      icon = IconNode::CreateIcon(image, preserveAspect, name);
      Image::DestroyImage(image);
      return icon;
    } else {
      return &emptyIcon;
    }
  }

  /* Try icon paths. */
  for (auto path : iconPaths) {
    icon = LoadNamedIconHelper(name, path.c_str(), save, preserveAspect);
    if (icon) {
      return icon;
    }
  }

  /* The default icon. */
  return NULL;
}

/** Helper for loading icons by name. */
IconNode* IconNode::LoadNamedIconHelper(const char *name, const char *path,
    char save, char preserveAspect) {
  Image *image;
  char *temp;
  const unsigned nameLength = strlen(name);
  const unsigned pathLength = strlen(path);
  unsigned i;
  char hasExtension;

  /* Full file name. */
  temp = AllocateStack(nameLength + pathLength + MAX_EXTENSION_LENGTH + 1);
  memcpy(&temp[0], path, pathLength);
  memcpy(&temp[pathLength], name, nameLength + 1);

  /* Determine if the extension is provided.
   * We avoid extra file opens if so.
   */
  hasExtension = 0;
  for (i = 1; i < EXTENSION_COUNT; i++) {
    const unsigned offset = nameLength + pathLength;
    const unsigned extLength = strlen(ICON_EXTENSIONS[i]);
    if (JUNLIKELY(offset < extLength)) {
      continue;
    }
    if (!strcmp(ICON_EXTENSIONS[i], &temp[offset])) {
      hasExtension = 1;
      break;
    }
  }

  /* Attempt to load the image. */
  image = NULL;
  if (hasExtension) {
    image = Image::LoadImage(temp, 0, 0, 1);
  } else {
    for (i = 0; i < EXTENSION_COUNT; i++) {
      const unsigned len = strlen(ICON_EXTENSIONS[i]);
      memcpy(&temp[pathLength + nameLength], ICON_EXTENSIONS[i], len + 1);
      image = Image::LoadImage(temp, 0, 0, 1);
      if (image) {
        break;
      }
    }
  }
  ReleaseStack(temp);

  /* Create the icon if we were able to load the image. */
  if (image) {
    IconNode *result = IconNode::CreateIcon(image, preserveAspect, temp);
    Image::DestroyImage(image);
    return result;
  }

  return NULL;
}

/** Read the icon property from a client. */
IconNode* IconNode::ReadNetWMIcon(Window win) {
  static const long MAX_LENGTH = 1 << 20;
  IconNode *icon = NULL;
  unsigned long count;
  int status;
  unsigned long extra;
  Atom realType;
  int realFormat;
  unsigned char *data;
  status = JXGetWindowProperty(display, win, Hints::atoms[ATOM_NET_WM_ICON], 0,
      MAX_LENGTH, False, XA_CARDINAL, &realType, &realFormat, &count, &extra,
      &data);
  if (status == Success && realFormat != 0 && data) {
    icon = CreateIconFromBinary((unsigned long*) data, count);
    JXFree(data);
  }
  return icon;
}

/** Read the icon WMHint property from a client. */
IconNode* IconNode::ReadWMHintIcon(Window win) {
  IconNode *icon = NULL;
  XWMHints *hints = JXGetWMHints(display, win);
  if (hints) {
    Drawable d = None;
    Pixmap mask = None;
    if (hints->flags & IconMaskHint) {
      mask = hints->icon_mask;
    }
    if (hints->flags & IconPixmapHint) {
      d = hints->icon_pixmap;
    }
    if (d != None) {
      icon = CreateIconFromDrawable(d, mask);
    }
    JXFree(hints);
  }
  return icon;
}

/** Create an icon from XPM image data. */
IconNode* IconNode::GetDefaultIcon(void) {
  static const char *const name = "default";
  const unsigned width = 8;
  const unsigned height = 8;
  const unsigned border = 1;
  Image *image;
  IconNode *result;
  unsigned bytes;
  unsigned x, y;

  /* Load the specified default, if configured. */
  if (defaultIconName) {
    result = LoadNamedIcon(defaultIconName, 1, 1);
    return result ? result : &emptyIcon;
  }

  /* Check if this icon has already been loaded */
  result = FindIcon(name);
  if (result) {
    return result;
  }

  /* Allocate image data. */
  bytes = (width * height + 7) / 8;
  image = Image::CreateImage(width, height, 1);

  return result;
}

IconNode* IconNode::CreateIconFromDrawable(Drawable d, Pixmap mask) {
  Image *image;

  image = Image::LoadImageFromDrawable(d, mask);
  if (image) {
    IconNode *result = IconNode::CreateIcon(image, true, "FromDrawable");
    return result;
  } else {
    return NULL;
  }
}

/** Get the best image for the requested size. */
Image* IconNode::GetBestImage(IconNode *icon, int rwidth, int rheight) {
  Image *best;
  Image *ip;

  /* If we don't have an image loaded, load one. */
  if (icon->images.empty()) {
    return Image::LoadImage(icon->getName(), rwidth, rheight,
        icon->isAspectPreserved());
  }

  /* Find the best image to use.
   * Select the smallest image to completely cover the
   * requested size.  If no image completely covers the
   * requested size, select the one that overlaps the most area.
   * If no size is specified, use the largest. */
  best = (*(--images.end()))->getImage();
  for (auto ip : images) {
    const int best_area = best->getWidth() * best->getHeight();
    const int other_area = ip->getWidth() * ip->getHeight();
    int best_overlap;
    int other_overlap;
    if (rwidth == 0 && rheight == 0) {
      best_overlap = 0;
      other_overlap = 0;
    } else if (rwidth == 0) {
      best_overlap = Min(best->getHeight(), rheight);
      other_overlap = Min(ip->getHeight(), rheight);
    } else if (rheight == 0) {
      best_overlap = Min(best->getWidth(), rwidth);
      other_overlap = Min(ip->getWidth(), rwidth);
    } else {
      best_overlap =
      Min(best->getWidth(), rwidth) * Min(best->getHeight(), rheight);
      other_overlap = Min(ip->getWidth(),
          rwidth) * Min(ip->getHeight(), rheight);
    }
    if (other_overlap > best_overlap) {
      best = ip->getImage();
    } else if (other_overlap == best_overlap) {
      if (other_area < best_area) {
        best = ip->getImage();
      }
    }
  }
  return best;
}

std::vector<ScaledIconNode*> ScaledIconNode::nodes;

/** Get a scaled icon. */
ScaledIconNode* ScaledIconNode::GetScaledIcon(IconNode *icon, long fg,
    int rwidth, int rheight) {

  XColor color;
  XImage *image;
  XPoint *points;
  Image *imageNode;
  ScaledIconNode *np;
  GC maskGC;
  int x, y;
  int scalex, scaley; /* Fixed point. */
  int srcx, srcy; /* Fixed point. */
  int nwidth, nheight;
  unsigned char *data;
  unsigned perLine;

  if (rwidth == 0) {
    rwidth = icon->getWidth();
  }
  if (rheight == 0) {
    rheight = icon->getHeight();
  }

  if (icon->isAspectPreserved()) {
    const int ratio = (icon->getWidth() << 16) / Max(icon->getHeight(), 16);
    nwidth = Min(rwidth, (rheight * ratio) >> 16);
    nheight = Min(rheight, (nwidth << 16) / ratio);
    nwidth = (nheight * ratio) >> 16;
  } else {
    nheight = rheight;
    nwidth = rwidth;
  }
  nwidth = Max(1, nwidth);
  nheight = Max(1, nheight);

  /* Check if this size already exists. */
  ScaledIconNode *toRet = NULL;
  for (auto np : nodes) {
    if (!icon->isBitmap() || np->fg == fg) {
#ifdef USE_XRENDER
      /* If we are using xrender and only have one image size
       * available, we can simply scale the existing icon. */
      if (icon->useRender()) {
        toRet = np;
      }
#endif
      if (np->getWidth() == nwidth && np->getHeight() == nheight) {
        toRet = np;
      }
    }
  }

  if (toRet) {
    return toRet;
  }

  /* Need to load the image. */
  imageNode = IconNode::GetBestImage(icon, nwidth, nheight);
  if (JUNLIKELY(!imageNode)) {
    return NULL;
  }

  /* See if we can use XRender to create the icon. */
#ifdef USE_XRENDER
  if (icon->useRender()) {
    np = CreateScaledRenderIcon(imageNode, fg);

    /* Don't keep the image data around after creating the icon. */
    if (icon->getImage() == NULL) {
      delete imageNode;
    }

    return np;
  }
#endif

  /* Create a new ScaledIconNode the old-fashioned way. */
  np = new ScaledIconNode(imageNode, false, "Scaled", nwidth, nheight);
  np->fg = fg;
  nodes.push_back(np);

  /* Create a mask. */
  np->mask = JXCreatePixmap(display, rootWindow, nwidth, nheight, 1);
  maskGC = JXCreateGC(display, np->mask, 0, NULL);
  JXSetForeground(display, maskGC, 0);
  JXFillRectangle(display, np->mask, maskGC, 0, 0, nwidth, nheight);
  JXSetForeground(display, maskGC, 1);

  /* Create a temporary XImage for scaling. */
  image = JXCreateImage(display, rootVisual, rootDepth, ZPixmap, 0, NULL,
      nwidth, nheight, 8, 0);
  image->data = new char[sizeof(unsigned long) * nwidth * nheight];

  /* Determine the scale factor. */
  scalex = (imageNode->getWidth() << 16) / nwidth;
  scaley = (imageNode->getHeight() << 16) / nheight;

  points = new XPoint[nwidth];
  data = imageNode->getData();
  if (imageNode->isBitmap()) {
    perLine = (imageNode->getWidth() >> 3)
        + ((imageNode->getWidth() & 7) ? 1 : 0);
  } else {
    perLine = imageNode->getWidth();
  }
  srcy = 0;
  for (y = 0; y < nheight; y++) {
    const int yindex = (srcy >> 16) * perLine;
    int pindex = 0;
    srcx = 0;
    for (x = 0; x < nwidth; x++) {
      if (imageNode->isBitmap()) {
        const int tx = srcx >> 16;
        const int offset = yindex + (tx >> 3);
        const int mask = 1 << (tx & 7);
        if (data[offset] & mask) {
          points[pindex].x = x;
          points[pindex].y = y;
          XPutPixel(image, x, y, fg);
          pindex += 1;
        }
      } else {
        const int yindex = (srcy >> 16) * imageNode->getWidth();
        const int index = 4 * (yindex + (srcx >> 16));
        color.red = data[index + 1];
        color.red |= color.red << 8;
        color.green = data[index + 2];
        color.green |= color.green << 8;
        color.blue = data[index + 3];
        color.blue |= color.blue << 8;
        Colors::GetColor(&color);
        XPutPixel(image, x, y, color.pixel);
        if (data[index] >= 128) {
          points[pindex].x = x;
          points[pindex].y = y;
          pindex += 1;
        }
      }
      srcx += scalex;
    }
    JXDrawPoints(display, np->mask, maskGC, points, pindex, CoordModeOrigin);
    srcy += scaley;
  }
  delete[] points;

  /* Release the mask GC. */
  JXFreeGC(display, maskGC);

  /* Create the color data pixmap. */
  np->image = JXCreatePixmap(display, rootWindow, nwidth, nheight, rootDepth);

  /* Render the image to the color data pixmap. */
  JXPutImage(display, np->image, rootGC, image, 0, 0, 0, 0, nwidth, nheight);
  /* Release the XImage. */
  delete[] image->data;
  image->data = NULL;
  JXDestroyImage(image);

  if (icon->getImage() == NULL) {
    delete imageNode;
  }

  return np;

}

/** Create an icon from binary data (as specified via window properties). */
IconNode* IconNode::CreateIconFromBinary(const unsigned long *input,
    unsigned int length) {
  IconNode *result = NULL;
  unsigned int offset = 0;

  if (!input) {
    return NULL;
  }

  while (offset < length) {

    const unsigned width = input[offset + 0];
    const unsigned height = input[offset + 1];
    const unsigned char *data;
    Image *image;
    unsigned x;

    if (JUNLIKELY(width * height + 2 > length - offset)) {
      Debug("invalid image size: %d x %d + 2 > %d", width, height,
          length - offset);
      return result;
    } else if (JUNLIKELY(width == 0 || height == 0)) {
      Debug("invalid image size: %d x %d", width, height);
      return result;
    }

    image = Image::CreateImage(width, height, 0);
    if (result == NULL) {
      result = IconNode::CreateIcon(image, false, "");
    }
    image->CopyFrom(input, offset);

    /* Don't insert this icon into the hash since it is transient. */

  }

  return result;
}

IconNode::IconNode(Image *image, bool preserveAspect, const char *name) :
    name(CopyString(name)), preserveAspect(preserveAspect), width(
        image->getWidth()), height(image->getHeight()), bitmap(
        image->isBitmap()), image(image) {
#ifdef USE_XRENDER
  this->render = image->getRender();
#endif

  this->transient = 1;
}

IconNode::~IconNode() {
  Image::DestroyImage(this->image);
  if (this->getName()) {
    delete[] this->getName();
  }
}

int IconNode::getWidth() {
  return width;
}

int IconNode::getHeight() {
  return height;
}

const char* IconNode::getName() {
  return name;
}

Image* IconNode::getImage() const {
  return image;
}

bool IconNode::isBitmap() const {
  return bitmap;
}

#ifdef USE_XRENDER
bool IconNode::useRender() const {
  return render;
}
#endif

bool IconNode::isAspectPreserved() {
  return preserveAspect;
}

/** Create an empty icon node. */
IconNode* IconNode::CreateIcon(Image *image, bool preserveAspect,
    const char *name) {
  IconNode *icon = new IconNode(image, preserveAspect, name);
  images.push_back(icon);
  return icon;
}

ScaledIconNode::ScaledIconNode(Image *image, bool preserveAspect,
    const char *name, int newWidth, int newHeight) :
    IconNode(image, preserveAspect, name) {

}

ScaledIconNode::ScaledIconNode(Image *image, bool preserveAspect,
    const char *name) :
    IconNode(image, preserveAspect, name), fg(0), mask(0), image(0) {

}

ScaledIconNode::~ScaledIconNode() {
  if (this->image != None) {
    JXRenderFreePicture(display, this->image);
  }

  if (this->image != None) {
    JXFreePixmap(display, this->image);
  }

  if (this->mask != None) {
    JXRenderFreePicture(display, this->mask);
  }

  if (this->mask != None) {
    JXFreePixmap(display, this->mask);
  }
}

/** Destroy an icon. */
bool IconNode::DestroyIcon(IconNode *icon) {
  bool found = false;
  std::vector<IconNode*>::iterator it;
  for (it = images.begin(); it != images.end(); ++it) {
    if ((*it) == icon) {
      found = true;
      break;
    }
  }
  if (found) {
    images.erase(it);
  } else {
    Logger::Log("Could not find component in tray to remove\n");
  }
  return found;
}

/** Find a icon in the icon hash table. */
IconNode* IconNode::FindIcon(const char *name) {
  for (auto icon : images) {
    if (!strcmp(icon->getName(), name)) {
      return icon;
    }
  }

  return NULL;
}

/** Set the name of the default icon. */
void IconNode::SetDefaultIcon(const char *name) {
  if (defaultIconName) {
    Release(defaultIconName);
  }
  defaultIconName = CopyString(name);
}

#endif /* USE_ICONS */
