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

#include "hint.h"
#include "timing.h"

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
  struct TrayComponent *components;

  /** End of the tray components. */
  struct TrayComponent *componentsTail;

  struct Tray *next; /**< Next tray. */
public:
  Tray();
  virtual ~Tray();
  LayoutType getLayout() const {return this->layout;}
  Window getWindow() const {return this->window;}
  Tray *getNext() const {return this->next;}
  WinLayerType getLayer() const {return this->layer;}
  int getHeight() const {return this->height;}
  int getWidth() const {return this->width;}
  int getX() const {return this->x;}
  int getY() const {return this->y;}
  char isHidden() const {return this->hidden;}
  TrayAutoHideType getAutoHide() const {return this->autoHide;}
public:
  static void InitializeTray(void);
  static void StartupTray(void);
  static void ShutdownTray(void);
  static void DestroyTray(void);
  static void handleConfirm(ClientNode *np);
  /** Add a tray component to a tray.
   * @param tp The tray to update.
   * @param cp The tray component to add.
   */
  void AddTrayComponent(TrayComponent *cp);

  /** Show a tray.
   * @param tp The tray to show.
   */
  void ShowTray();

  /** Show all trays. */
  static void ShowAllTrays(void);

  /** Hide a tray.
   * @param tp The tray to hide.
   */
  void HideTray();

  /** Draw all trays. */
  static void DrawTray(void);

  /** Draw a specific tray.
   * @param tp The tray to draw.
   */
  void DrawSpecificTray();

  /** Raise tray windows. */
  static void RaiseTrays(void);

  /** Lower tray windows. */
  static void LowerTrays(void);

  /** Resize a tray.
   * @param tp The tray to resize containing the new requested size information.
   */
  void ResizeTray();

  /** Draw the tray background on a drawable. */
  static void ClearTrayDrawable(const TrayComponent *cp);

  /** Get a linked list of trays.
   * @return The trays.
   */
  static Tray *GetTrays(void);

  /** Get the number of trays.
   * @return The number of trays.
   */
  static unsigned int GetTrayCount(void);

  /** Process an event that may be for a tray.
   * @param event The event to process.
   * @return 1 if this event was for a tray, 0 otherwise.
   */
  static char ProcessTrayEvent(const XEvent *event);

  /** Set whether auto-hide is enabled for a tray.
   * @param tp The tray.
   * @param autohide The auto-hide setting.
   * @param delay_ms The auto-hide timeout in milliseconds.
   */
  void SetAutoHideTray(TrayAutoHideType autohide, unsigned delay_ms);

  /** Set the tray x-coordinate.
   * @param tp The tray.
   * @param str The x-coordinate (ASCII, pixels, negative ok).
   */
  void SetTrayX(const char *str);

  /** Set the tray y-coordinate.
   * @param tp The tray.
   * @param str The y-coordinate (ASCII, pixels, negative ok).
   */
  void SetTrayY(const char *str);

  /** Set the tray width.
   * @param tp The tray.
   * @param str The width (ASCII, pixels).
   */
  void SetTrayWidth(const char *str);

  /** Set the tray height.
   * @param tp The tray.
   * @param str The height (ASCII, pixels).
   */
  void SetTrayHeight( const char *str);

  /** Set the tray layout.
   * @param tp The tray.
   * @param str A string representation of the layout to use.
   */
  void SetTrayLayout( const char *str);

  /** Set the tray layer.
   * @param tp The tray.
   * @param layer The layer.
   */
  void SetTrayLayer( WinLayerType layer);

  /** Set the tray horizontal alignment.
   * @param tp The tray.
   * @param str The alignment (ASCII).
   */
  void SetTrayHorizontalAlignment(const char *str);

  /** Set the tray vertical alignment.
   * @param tp The tray.
   * @param str The alignment (ASCII).
   */
  void SetTrayVerticalAlignment(const char *str);

  void LayoutTray(int *variableSize, int *variableRemainder);

  static void HandleTrayExpose(Tray *tp, const XExposeEvent *event);
  static void HandleTrayEnterNotify(Tray *tp, const XCrossingEvent *event);

  static TrayComponent *GetTrayComponent(Tray *tp, int x, int y);
  static void HandleTrayButtonPress(Tray *tp, const XButtonEvent *event);
  static void HandleTrayButtonRelease(Tray *tp, const XButtonEvent *event);
  static void HandleTrayMotionNotify(Tray *tp, const XMotionEvent *event);

  void ComputeTraySize();
  int ComputeMaxWidth();
  int ComputeTotalWidth();
  int ComputeMaxHeight();
  int ComputeTotalHeight();
  char CheckHorizontalFill();
  char CheckVerticalFill();

  static void SignalTray(const TimeType *now, int x, int y, Window w, void *data);

  static Tray *trays;
  static unsigned int trayCount;
};

#endif /* TRAY_H */

