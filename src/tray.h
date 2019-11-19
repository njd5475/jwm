/**
 * @file tray.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Tray functions.
 *
 */

#ifndef TRAY_H
#define TRAY_H

#include <vector>
#include "hint.h"
#include "timing.h"

class TrayComponent;
struct KeyNode;
struct BoundingBox;

/** Enumeration of tray layouts. */
typedef unsigned char LayoutType;
#define LAYOUT_HORIZONTAL  0  /**< Left-to-right. */
#define LAYOUT_VERTICAL    1  /**< Top-to-bottom. */

/** Enumeration of tray alignments. */
typedef unsigned char TrayAlignmentType;
#define TALIGN_FIXED    0 /**< Fixed at user specified x and y coordinates. */
#define TALIGN_LEFT     1 /**< Left aligned. */
#define TALIGN_TOP      2 /**< Top aligned. */
#define TALIGN_CENTER   3 /**< Center aligned. */
#define TALIGN_RIGHT    4 /**< Right aligned. */
#define TALIGN_BOTTOM   5 /**< Bottom aligned. */

/** Enumeration of tray autohide values. */
typedef unsigned char TrayAutoHideType;
#define THIDE_OFF       0 /**< No autohide. */
#define THIDE_LEFT      1 /**< Hide on the left. */
#define THIDE_RIGHT     2 /**< Hide on the right. */
#define THIDE_TOP       3 /**< Hide on the top. */
#define THIDE_BOTTOM    4 /**< Hide on the bottom. */
#define THIDE_ON        5 /**< Auto-select hide location. */
#define THIDE_INVISIBLE 6 /**< Make the tray invisible when hidden. */
#define THIDE_RAISED    8 /**< Mask to indicate the tray is raised. */

/** Structure to hold common tray component data.
 * Sizing is handled as follows:
 *  - The component is created via a factory method. It sets its
 *    requested size (0 for no preference).
 *  - The SetSize callback is issued with size constraints
 *    (0 for no constraint). The component should update
 *    width and height in SetSize.
 *  - The Create callback is issued with finalized size information.
 * Resizing is handled as follows:
 *  - A component determines that it needs to change size. It updates
 *    its requested size (0 for no preference).
 *  - The component calls ResizeTray.
 *  - The SetSize callback is issued with size constraints
 *    (0 for no constraint). The component should update
 *    width and height in SetSize.
 *  - The Resize callback is issued with finalized size information.
 */

/** Structure to represent a tray. */
class Tray {

private:
  int requestedX; /**< The user-requested x-coordinate of the tray. */
  int requestedY; /**< The user-requested y-coordinate of the tray. */

  int x; /**< The x-coordinate of the tray. */
  int y; /**< The y-coordinate of the tray. */

  int requestedWidth; /**< Total requested width of the tray. */
  int requestedHeight; /**< Total requested height of the tray. */

  int width; /**< Actual width of the tray. */
  int height; /**< Actual height of the tray. */

  WinLayerType layer; /**< Layer. */
  LayoutType layout; /**< Layout. */
  TrayAlignmentType valign; /**< Vertical alignment. */
  TrayAlignmentType halign; /**< Horizontal alignment. */

  TimeType showTime;
  TrayAutoHideType autoHide;
  unsigned autoHideDelay;
  char hidden; /**< 1 if hidden (due to autohide), 0 otherwise. */

  Window window; /**< The tray window. */

  /** Start of the tray components. */
  std::vector<TrayComponent*> components;
  static std::vector<Tray*> trays;

  Tray();
  virtual ~Tray();

public:
  LayoutType getLayout() const {return this->layout;}
  Window getWindow() const {return this->window;}
  WinLayerType getLayer() const {return this->layer;}
  int getHeight() const {return this->height;}
  int getWidth() const {return this->width;}
  int getX() const {return this->x;}
  int getY() const {return this->y;}
  char isHidden() const {return this->hidden;}
  TrayAutoHideType getAutoHide() const {return this->autoHide;}

  void AddTrayComponent(TrayComponent *cp);
  void ShowTray();
  void HideTray();
  void DrawSpecificTray();
  void ResizeTray();
  void SetAutoHideTray(TrayAutoHideType autohide, unsigned delay_ms);

  void SetTrayX(const char *str);
  void SetTrayY(const char *str);
  void SetTrayWidth(const char *str);
  void SetTrayHeight( const char *str);
  void SetTrayLayout( const char *str);
  void SetTrayLayer( WinLayerType layer);
  void SetTrayHorizontalAlignment(const char *str);
  void SetTrayVerticalAlignment(const char *str);

  void LayoutTray(int *variableSize, int *variableRemainder);

  void ComputeTraySize();
  int ComputeMaxWidth();
  int ComputeTotalWidth();
  int ComputeMaxHeight();
  int ComputeTotalHeight();
  char CheckHorizontalFill();
  char CheckVerticalFill();

public:

  static void InitializeTray(void);
  static void StartupTray(void);
  static void ShutdownTray(void);
  static void DestroyTray(void);
  static void handleConfirm(ClientNode *np);

  static void ShowAllTrays(void);
  static void DrawTray(void);
  static void RaiseTrays(void);
  static void LowerTrays(void);

  static void ClearTrayDrawable(const TrayComponent *cp);

  static char ProcessTrayEvent(const XEvent *event);

  static void HandleTrayExpose(Tray *tp, const XExposeEvent *event);
  static void HandleTrayEnterNotify(Tray *tp, const XCrossingEvent *event);

  static TrayComponent *GetTrayComponent(Tray *tp, int x, int y);
  static void HandleTrayButtonPress(Tray *tp, const XButtonEvent *event);
  static void HandleTrayButtonRelease(Tray *tp, const XButtonEvent *event);
  static void HandleTrayMotionNotify(Tray *tp, const XMotionEvent *event);

  static void SignalTray(const TimeType *now, int x, int y, Window w, void *data);
  static void UngrabKeys(Display *d, unsigned int keyCode, unsigned int modifiers);
  static void GrabKey(KeyNode *kn);
  static std::vector<BoundingBox> GetBoundsAbove(int layer);
  static std::vector<BoundingBox> GetVisibleBounds();
  static std::vector<Window> getTrayWindowsAt(int layer);
  static unsigned int GetTrayCount() {return trays.size();}
  static Tray* Create();
};

#endif /* TRAY_H */

