/**
 * @file client.h
 * @author Joe Wingbermuehle
 * @date 2004-2007
 *
 * @brief Client window functions.
 *
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "main.h"
#include "border.h"
#include "hint.h"

struct TimeType;

/** Enumeration of window layers. */
#define LAYER_DESKTOP   0
#define LAYER_BELOW     1
#define LAYER_NORMAL    2
#define LAYER_ABOVE     3
#define LAYER_COUNT     4

#define FIRST_LAYER        LAYER_DESKTOP
#define LAST_LAYER         LAYER_ABOVE
#define DEFAULT_TRAY_LAYER LAYER_ABOVE

/** Window border flags.
 * We use an unsigned short for storing these, so we get at least 16
 * on reasonable architectures.
 */
typedef unsigned short BorderFlags;
#define BORDER_NONE        0
#define BORDER_OUTLINE     (1 << 0)    /**< Window has a border. */
#define BORDER_TITLE       (1 << 1)    /**< Window has a title bar. */
#define BORDER_MIN         (1 << 2)    /**< Window supports minimize. */
#define BORDER_MAX         (1 << 3)    /**< Window supports maximize. */
#define BORDER_CLOSE       (1 << 4)    /**< Window supports close. */
#define BORDER_RESIZE      (1 << 5)    /**< Window supports resizing. */
#define BORDER_MOVE        (1 << 6)    /**< Window supports moving. */
#define BORDER_MAX_V       (1 << 7)    /**< Maximize vertically. */
#define BORDER_MAX_H       (1 << 8)    /**< Maximize horizontally. */
#define BORDER_SHADE       (1 << 9)    /**< Allow shading. */
#define BORDER_CONSTRAIN   (1 << 10)   /**< Constrain to the screen. */
#define BORDER_FULLSCREEN  (1 << 11)   /**< Allow fullscreen. */

/** The default border flags. */
#define BORDER_DEFAULT (   \
        BORDER_OUTLINE     \
      | BORDER_TITLE       \
      | BORDER_MIN         \
      | BORDER_MAX         \
      | BORDER_CLOSE       \
      | BORDER_RESIZE      \
      | BORDER_MOVE        \
      | BORDER_MAX_V       \
      | BORDER_MAX_H       \
      | BORDER_SHADE       \
      | BORDER_FULLSCREEN  )

/** Window status flags.
 * We use an unsigned int for storing these, so we get 32 on
 * reasonable architectures.
 */
typedef unsigned int StatusFlags;
#define STAT_NONE       0
#define STAT_ACTIVE     (1 << 0)    /**< Has focus. */
#define STAT_MAPPED     (1 << 1)    /**< Shown (on some desktop). */
#define STAT_HIDDEN     (1 << 2)    /**< Not on the current desktop. */
#define STAT_STICKY     (1 << 3)    /**< This client is on all desktops. */
#define STAT_NOLIST     (1 << 4)    /**< Skip this client in the task list. */
#define STAT_MINIMIZED  (1 << 5)    /**< Minimized. */
#define STAT_SHADED     (1 << 6)    /**< Shaded. */
#define STAT_WMDIALOG   (1 << 7)    /**< This is a JWM dialog window. */
#define STAT_PIGNORE    (1 << 8)    /**< Ignore the program-position. */
#define STAT_SDESKTOP   (1 << 9)    /**< Minimized to show desktop. */
#define STAT_FULLSCREEN (1 << 10)   /**< Full screen. */
#define STAT_OPACITY    (1 << 11)   /**< Fixed opacity. */
#define STAT_NOFOCUS    (1 << 12)   /**< Don't focus on map. */
#define STAT_CANFOCUS   (1 << 13)   /**< Client accepts input focus. */
#define STAT_DELETE     (1 << 14)   /**< Client accepts WM_DELETE. */
#define STAT_TAKEFOCUS  (1 << 15)   /**< Client uses WM_TAKE_FOCUS. */
#define STAT_URGENT     (1 << 16)   /**< Urgency hint is set. */
#define STAT_NOTURGENT  (1 << 17)   /**< Ignore the urgency hint. */
#define STAT_CENTERED   (1 << 18)   /**< Use centered window placement. */
#define STAT_TILED      (1 << 19)   /**< Use tiled window placement. */
#define STAT_IIGNORE    (1 << 20)   /**< Ignore increment when maximized. */
#define STAT_NOPAGER    (1 << 21)   /**< Don't show in pager. */
#define STAT_SHAPED     (1 << 22)   /**< This window is shaped. */
#define STAT_FLASH      (1 << 23)   /**< Flashing for urgency. */
#define STAT_DRAG       (1 << 24)   /**< Pass mouse events to JWM. */
#define STAT_ILIST      (1 << 25)   /**< Ignore program-specified list. */
#define STAT_IPAGER     (1 << 26)   /**< Ignore program-specified pager. */
#define STAT_FIXED      (1 << 27)   /**< Keep on the specified desktop. */
#define STAT_AEROSNAP   (1 << 28)   /**< Enable Aero Snap. */
#define STAT_NODRAG     (1 << 29)   /**< Disable mod1+drag/resize. */
#define STAT_POSITION   (1 << 30)   /**< Config-specified position. */

/** Maximization flags. */
typedef unsigned char MaxFlags;
#define MAX_NONE     0           /**< Don't maximize. */
#define MAX_HORIZ    (1 << 0)    /**< Horizontal maximization. */
#define MAX_VERT     (1 << 1)    /**< Vertical maximization. */
#define MAX_LEFT     (1 << 2)    /**< Maximize on left. */
#define MAX_RIGHT    (1 << 3)    /**< Maximize on right. */
#define MAX_TOP      (1 << 4)    /**< Maximize on top. */
#define MAX_BOTTOM   (1 << 5)    /**< Maximize on bottom. */

/** Colormap window linked list. */
typedef struct ColormapNode {
  Window window; /**< A window containing a colormap. */
  struct ColormapNode *next; /**< Next value in the linked list. */
} ColormapNode;

/** The aspect ratio of a window. */
typedef struct AspectRatio {
  int minx; /**< The x component of the minimum aspect ratio. */
  int miny; /**< The y component of the minimum aspect ratio. */
  int maxx; /**< The x component of the maximum aspect ratio. */
  int maxy; /**< The y component of the maximum aspect ratio. */
} AspectRatio;

/** Bounding box. */
typedef struct BoundingBox {
  int x; /**< x-coordinate of the bounding box. */
  int y; /**< y-coordinate of the bounding box. */
  int width; /**< Width of the bounding box. */
  int height; /**< Height of the bounding box. */
} BoundingBox;

typedef struct Strut {
  ClientNode *client;
  BoundingBox box;
  struct Strut *next;
} Strut;

/** Struture to store information about a client window. */
class ClientNode {

private:
  ClientNode(Window w, char alreadyMapped, char notOwner);

protected:
  virtual ~ClientNode();

  static std::vector<ClientNode*> nodes;

  Window window; /**< The client window. */
  Window parent; /**< The frame window. */
  Window owner; /**< The owner window (for transients). */

  int x, y; /**< The location of the window. */
  int width; /**< The width of the window. */
  int height; /**< The height of the window. */
  int oldx; /**< The old x coordinate (for maximize). */
  int oldy; /**< The old y coordinate (for maximize). */
  int oldWidth; /**< The old width (for maximize). */
  int oldHeight; /**< The old height (for maximize). */

  long sizeFlags; /**< Size flags from XGetWMNormalHints. */
  int baseWidth; /**< Base width for resizing. */
  int baseHeight; /**< Base height for resizing. */
  int minWidth; /**< Minimum width of this window. */
  int minHeight; /**< Minimum height of this window. */
  int maxWidth; /**< Maximum width of this window. */
  int maxHeight; /**< Maximum height of this window. */
  int xinc; /**< Resize x increment. */
  int yinc; /**< Resize y increment. */
  AspectRatio aspect; /**< Aspect ratio. */
  int gravity; /**< Gravity for reparenting. */

  Colormap cmap; /**< This window's colormap. */
  ColormapNode *colormaps; /**< Colormaps assigned to this window. */

  char *name; /**< Name of this window for display. */
  char *instanceName; /**< Name of this window for properties. */
  char *className; /**< Name of the window class. */

  MouseContextType mouseContext;

  struct IconNode *icon; /**< Icon assigned to this window. */

  /** Callback to stop move/resize. */
  void (*controller)(int wasDestroyed);

protected:
  void saveBounds();

public:

  static char DoRemoveClientStrut(ClientNode *np);
  static void InsertStrut(const BoundingBox *box, ClientNode *np);
  static void RestackClients(void);
  static void GetScreenBounds(const struct ScreenType *sp, BoundingBox *box);
  static void SubtractStrutBounds(BoundingBox *box, const ClientNode *np);
  static void SubtractTrayBounds(BoundingBox *box, unsigned int layer);
  static void SubtractBounds(const BoundingBox *src, BoundingBox *dest);
  static ClientNode *Create(Window w, char alreadyMapped, char notOwner);
  static ClientNode *FindClient(Window w); // by window or parent
  static ClientNode *FindClientByWindow(Window w);
  static ClientNode *FindClientByParent(Window p);
  static ClientNode *GetActiveClient(void);

  static void InitializeClients();
  static void StartupClients(void);
  static void ShutdownClients(void);
  static void DestroyClients();
  /** Set the keyboard focus back to the active client. */
  static void RefocusClient(void);

  /** Place a client on the screen.
   * @param np The client to place.
   * @param alreadyMapped 1 if already mapped, 0 if unmapped.
   */
  static void PlaceClient(ClientNode *np, char alreadyMapped);

  /** Send a message to a client.
   * @param w The client window.
   * @param type The type of message to send.
   * @param message The message to send.
   */
  static void SendClientMessage(Window w, AtomType type, AtomType message);

  /** Update callback for clients with the urgency hint set. */
  static void SignalUrgent(const struct TimeType *now, int x, int y, Window w,
      void *data);

  /** The number of clients (maintained in client.c). */
  static unsigned int clientCount;

public:

  void (*getController())(int) {
    return this->controller;
  }

  /** Get the x and y deltas for gravitating a client.
   * @param np The client.
   * @param gravity The gravity to use.
   * @param x Location to store the x delta.
   * @param y Location to store the y delta.
   */
  void GetGravityDelta(int gravity, int *x, int *y);

  void DrawBorder();
  Window getOwner() const;
  void setOwner(Window owner);

  int getX() const;
  int getY() const;

  //TODO: Encapsulate size
  void setX(int x);
  void setY(int y);
  void setHeight(int height);
  void setWidth(int width);

  int getBaseWidth() {return this->baseWidth;}
  int getBaseHeight() {return this->baseHeight;}
  int getMaxWidth() {return this->maxWidth;}
  int getMaxHeight() {return this->maxHeight;}
  int getMinWidth() {return this->minWidth;}
  int getMinHeight() {return this->minHeight;}
  int getYInc() {return this->yinc;}
  int getXInc() {return this->xinc;}

  AspectRatio getAspect() {return this->aspect;}
  const char* getClassName() const;
  const char* getInstanceName();

  IconNode *getIcon() const;
  void setIcon(IconNode *icon);

  ColormapNode* getColormaps() const;

  Window getWindow() const;
  Window getParent() const;
  const char* getName() const;
  int getWidth() const;
  int getHeight() const;
  int getGravity() const;
  MouseContextType getMouseContext() const;
  void setMouseContext(MouseContextType context);
  long int getSizeFlags();

  void SetOpacity(unsigned int opacity, char force);
  void _UpdateState();
  void UpdateWindowState(char alreadyMapped);
  void ReadWMColormaps();
  void ReadWMNormalHints();
  void ReadWMName();
  void ReadWMClass();
  void LoadIcon();
  void StopMove(int doMove, int oldx, int oldy);
  char MoveClient(int startx, int starty);
  char MoveClientKeyboard();
  void RestartMove(int *doMove);
  void DoSnapScreen();
  void DoSnapBorder();
  void clearController();
  void setController(void (*controller)(int wasDestroyed));
  void StopPagerMove(int x, int y, int desktop, MaxFlags maxFlags);

  void GravitateClient(char negate);
  void PlaceMaximizedClient(MaxFlags flags);

  /** Place a maximized client on the screen.
   * @param np The client to place.
   * @param flags The type of maximization to perform.
   */
  void PlaceMaximizedClient(ClientNode *np, MaxFlags flags);

  void ConstrainPosition();
  char ConstrainSize();
  void CascadeClient(const BoundingBox *box);
  char TileClient(const BoundingBox *box);
  int TryTileClient(const BoundingBox *box, int x, int y);
  void CenterClient(const BoundingBox *box);
  void FixHeight();
  void FixWidth();
  void StopResize();
  void ResizeClientKeyboard(MouseContextType context);
  void ResizeClient(MouseContextType context, int startx, int starty);
  void UpdateSize(const MouseContextType context, const int x, const int y, const int startx,
      const int starty, const int oldx, const int oldy, const int oldw, const int oldh);

  void RemoveClient();
  void MinimizeClient(char lower);
  void ShadeClient();
  void UnshadeClient();

  /** Set a client's status to withdrawn.
   * A withdrawn client is a client that is not visible in any way to the
   * user. This may be a window that an application keeps around so that
   * it can be reused at a later time.
   * @param np The client whose status to change.
   */
  void sendToBackground();

  /** Restore a client from minimized state.
   * @param np The client to restore.
   * @param raise 1 to raise the client, 0 to leave stacking unchanged.
   */
  void RestoreClient(char raise);
  void MaximizeClient(MaxFlags flags);

  /** Maximize a client using the default maximize settings.
   * @param np The client to maximize.
   */
  void MaximizeClientDefault();

  /** Set the full screen status of a client.
   * @param np The client.
   * @param fullScreen 1 to make full screen, 0 to make not full screen.
   */
  void SetClientFullScreen(char fullScreen);

  /** Set the keyboard focus to a client.
   * @param np The client to focus.
   */
  void keyboardFocus();

  /** Tell a client to exit.
   * @param np The client to delete.
   */
  void DeleteClient();

  /** Force a client to exit.
   * @param np The client to kill.
   */
  void KillClient();

  /** Raise a client to the top of its layer.
   * @param np The client to raise.
   */
  void RaiseClient();

  /** Restack a client.
   * @param np The client to restack.
   * @param above A reference window (or None).
   * @param detail The stack mode (Above, Below, etc).
   */
  void RestackClient(Window above, int detail);
  void SetClientLayer(unsigned int layer);
  void SetClientDesktop(unsigned int desktop);
  void SetClientSticky(char isSticky);

  void HideClient();
  void ShowClient();

  void UpdateClientColormap(Colormap cmap);
  void ReparentClient();
  void SendConfigureEvent();
  void MinimizeTransients(char lower);

private:
  unsigned int status; /**< Status bit mask. */
  unsigned int opacity; /**< Opacity (0 - 0xFFFFFFFF). */
  unsigned short border; /**< Border bit mask. */
  unsigned short desktop; /**< Desktop. */
  unsigned char maxFlags; /**< Maximization status. */
  unsigned char layer; /**< Current window layer. */
  unsigned char defaultLayer; /**< Default window layer. */

public:

  void resetMaxFlags();
  void resetBorder();
  void resetLayer();
  void resetDefaultLayer();
  void resetLayerToDefault();
  void resetMappedState();

  void clearMaxFlags();
  void clearBorder();
  void clearStatus();
  void clearToNoList();
  void clearToNoPager();
  void clearToSticky();

  void ignoreProgramList();
  void ignoreProgramSpecificPager();
  void ignoreProgramSpecificPosition();
  void ignoreIncrementWhenMaximized();

  bool isStatus(unsigned int flags) const;
  bool isFullscreen() const;
  bool isPosition() const;
  bool isMapped() const;
  bool isShaded() const;
  bool isMinimized() const;
  bool isShaped() const;
  bool isIgnoringProgramPosition() const;
  bool isTiled() const;
  bool isCentered() const;
  bool isUrgent() const;
  bool isDialogWindow() const;
  bool isHidden() const;
  bool isSticky() const;
  bool isActive() const;
  bool isFixed() const;
  bool isDragable() const;
  bool isNotUrgent() const;
  bool isNotDraggable() const;
  bool isAeroSnapEnabled() const;
  bool shouldTakeFocus() const;
  bool shouldDelete() const;
  bool shouldFlash() const;
  bool shouldSkipInTaskList() const;
  bool shouldIgnoreSpecifiedList() const;
  bool shouldIgnorePager() const;
  bool shouldNotShowInPager() const;
  bool wasMinimizedToShowDesktop() const;
  bool notFocusableIfMapped() const;
  bool willIgnoreIncrementWhenMaximized() const;
  bool hasOpacity() const;
  bool canFocus() const;

  void setDefaultLayer(unsigned char defaultLayer);
  unsigned char getDefaultLayer() const;
  void setDesktop(unsigned short desktop);
  unsigned short getDesktop() const;
  void setCurrentDesktop(unsigned int desktop);

  unsigned int getOpacity();
  unsigned char getLayer() const;
  unsigned short getBorder() const;
  unsigned char getMaxFlags() const;

  void setOpacity(unsigned int opacity);
  void setLayer(unsigned char layer);
  void setMaxFlags(MaxFlags flags);
  void setNotMapped();
  void setShaped();
  void setDelete();
  void setTakeFocus();
  void setNoDelete();
  void setNoTakeFocus();
  void setActive();
  void setNotActive();
  //TODO: Rename these methods to be better understood
  void setDialogWindowStatus();
  void setSDesktopStatus();
  void setMapped();
  void setCanFocus();
  void setUrgent();
  void setNoFlash();
  void setShaded();
  void setMinimized();
  void setNoPager();
  void setNoFullscreen();
  void setPositionFromConfig();
  void setHasNoList();
  void setNoShaded();
  void setNoList();
  void setSticky();
  void setNoSticky();
  void setNoDrag();
  void setNoMinimized();
  void setNoSDesktop();
  void setEdgeSnap();
  void setDrag();
  void setFixed();
  void setFullscreen();
  void setMaximized();
  void setCentered();
  void setFlash();
  void setTiled();
  void setNotUrgent();
  void setTaskListSkipped();
  void setNoFocus();
  void setOpacityFixed();
  void setBorderOutline();
  void setBorderTitle();
  void setBorderMin();
  void setBorderMax();
  void setBorderClose();
  void setBorderResize();
  void setBorderMove();
  void setBorderMaxVert();
  void setBorderMaxHoriz();
  void setBorderShade();
  void setBorderConstrain();
  void setBorderFullscreen();
  void setNoCanFocus();
  void setNoBorderOutline();
  void setNoBorderTitle();
  void setNoBorderMin();
  void setNoBorderMax();
  void setNoBorderClose();
  void setNoBorderResize();
  void setNoBorderMove();
  void setNoBorderMaxVert();
  void setNoBorderMaxHoriz();
  void setNoBorderShade();
  void setNoBorderConstrain();
  void setNoBorderFullscreen();
  void setNoUrgent();
  void setNotHidden();
  void setHidden();
  void unsetNoPager();
  void unsetSkippingInTaskList();

private:

  static Strut *struts;
  static int *cascadeOffsets;
  static ClientNode *activeClient;

  static void LoadFocus(void);
  static void KillClientHandler(ClientNode *np);

  void RestackTransients();
  void RestoreTransients(char raise);
  void UnmapClient();

};

#endif /* CLIENT_H */

