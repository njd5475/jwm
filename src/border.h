/**
 * @file border.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for handling window borders.
 *
 */

#ifndef BORDER_H
#define BORDER_H

#include "gradient.h"
#include "icon.h"
#include "Component.h"

struct ClientNode;
struct ClientState;

/** Border icon types. */
typedef unsigned char BorderIconType;
#define BI_CLOSE        0
#define BI_MAX          1
#define BI_MAX_ACTIVE   2
#define BI_MENU         3
#define BI_MIN          4
#define BI_COUNT        5

typedef unsigned char MouseContextType;

class Border : public Component {
public:
  /*@{*/
  void initialize(void);
  void start(void);
  void stop(void);
  void destroy(void);
  /*@}*/

  /** Determine the mouse context for a location.
   * @param np The client.
   * @param x The x-coordinate of the mouse (frame relative).
   * @param y The y-coordinate of the mouse (frame relative).
   * @return The context.
   */
  static MouseContextType GetBorderContext(const struct ClientNode *np,
      int x, int y);

  /** Reset the shape of a window border.
   * @param np The client.
   */
  static void ResetBorder(const struct ClientNode *np);

  /** Draw a window border.
   * @param np The client whose frame to draw.
   */
  static void DrawBorder(struct ClientNode *np);

  /** Get the size of a border icon.
   * @return The size in pixels (note that icons are square).
   */
  static int GetBorderIconSize(void);

  /** Get the height of a window title bar. */
  static unsigned GetTitleHeight(void);

  /** Get the size of a window border.
   * @param state The client state.
   * @param north Pointer to the value to contain the north border size.
   * @param south Pointer to the value to contain the south border size.
   * @param east Pointer to the value to contain the east border size.
   * @param west Pointer to the value to contain the west border size.
   */
  static void GetBorderSize(struct ClientState *state,
      int *north, int *south, int *east, int *west);

  /** Redraw all borders on the current desktop. */
  static void ExposeCurrentDesktop(void);

  /** Draw a rounded rectangle.
   * @param d The drawable on which to render.
   * @param gc The graphics context.
   * @param x The x-coodinate.
   * @param y The y-coordinate.
   * @param width The width.
   * @param height The height.
   * @param radius The corner radius.
   */
  static void DrawRoundedRectangle(Drawable d, GC gc, int x, int y,
      int width, int height, int radius);

  /** Set the icon to use for a border button. */
  static void SetBorderIcon(BorderIconType t, const char *name);

private:
  static char *buttonNames[BI_COUNT];
  static IconNode *buttonIcons[BI_COUNT];

  static char IsContextEnabled(MouseContextType context, const ClientNode *np);
  static void DrawBorderHelper(const ClientNode *np);
  static void DrawBorderHandles(const ClientNode *np,
      Pixmap canvas, GC gc);
  static void DrawBorderButton(const ClientNode *np, MouseContextType context,
      int x, int y, Pixmap canvas, GC gc, long fg);
  static void DrawButtonBorder(const ClientNode *np, int x,
      Pixmap canvas, GC gc);
  static void DrawLeftButton(const ClientNode *np, MouseContextType context,
      int x, int y, Pixmap canvas, GC gc, long fg);
  static void DrawRightButton(const ClientNode *np, MouseContextType context,
      int x, int y, Pixmap canvas, GC gc, long fg);
  static XPoint DrawBorderButtons(const ClientNode *np, Pixmap canvas, GC gc);
  static char DrawBorderIcon(BorderIconType t,
      unsigned xoffset, unsigned yoffset,
      Pixmap canvas, long fg);
  static void DrawIconButton(const ClientNode *np, int x, int y,
      Pixmap canvas, GC gc, long fg);
  static void DrawCloseButton(unsigned xoffset, unsigned yoffset,
      Pixmap canvas, GC gc, long fg);
  static void DrawMaxIButton(unsigned xoffset, unsigned yoffset,
      Pixmap canvas, GC gc, long fg);
  static void DrawMaxAButton(unsigned xoffset, unsigned yoffset,
      Pixmap canvas, GC gc, long fg);
  static void DrawMinButton(unsigned xoffset, unsigned yoffset,
      Pixmap canvas, GC gc, long fg);

#ifdef USE_SHAPE
  static void FillRoundedRectangle(Drawable d, GC gc, int x, int y,
      int width, int height, int radius);
#endif

  static bool _registered;
};

#endif /* BORDER_H */

