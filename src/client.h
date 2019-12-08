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
	/** Get the x and y deltas for gravitating a client.
	 * @param np The client.
	 * @param gravity The gravity to use.
	 * @param x Location to store the x delta.
	 * @param y Location to store the y delta.
	 */
	void GetGravityDelta(int gravity, int *x, int *y);

	/** Get the bounding box for the screen.
	 * @param sp A pointer to the screen whose bounds to get.
	 * @param box The bounding box for the screen.
	 */
	static void GetScreenBounds(const struct ScreenType *sp, BoundingBox *box);

	static void SubtractStrutBounds(BoundingBox *box, const ClientNode *np);
	static void SubtractTrayBounds(BoundingBox *box, unsigned int layer);
	static void SubtractBounds(const BoundingBox *src, BoundingBox *dest);
  static ClientNode *Create(Window w, char alreadyMapped, char notOwner);

	/** The number of clients (maintained in client.c). */
	static unsigned int clientCount;

	void DrawBorder() {
		if (!(this->isStatus(STAT_HIDDEN | STAT_MINIMIZED))) {
			Border::DrawBorder(this);
		}
	}
	Window getOwner() const {
		return this->owner;
	}
	void setOwner(Window owner) {
		this->owner = owner;
	}
	int getX() const {
		return this->x;
	}

	int getY() const {
		return this->y;
	}
	void setX(int x) {
		this->x = x;
	}
	void setY(int y) {
		this->y = y;
	}
	void setHeight(int height) {
		this->height = height;
	}
	void setWidth(int width) {
		this->width = width;
	}

	int getBaseWidth() {return this->baseWidth;}
	int getBaseHeight() {return this->baseHeight;}
	int getMaxWidth() {return this->maxWidth;}
	int getMaxHeight() {return this->maxHeight;}
	int getMinWidth() {return this->minWidth;}
	int getMinHeight() {return this->minHeight;}
	int getYInc() {return this->yinc;}
	int getXInc() {return this->xinc;}

	AspectRatio getAspect() {return this->aspect;}

	void (*getController())(int) {
		return this->controller;
	}
	const char* getClassName() const {
		return this->className;
	}
	const char* getInstanceName() {
		return this->instanceName;
	}

	IconNode *getIcon() const {
		return this->icon;
	}
	void setIcon(IconNode *icon) {
		this->icon = icon;
	}

	ColormapNode* getColormaps() const {
		return this->colormaps;
	}

	Window getWindow() const {
		return this->window;
	}

	Window getParent() const {
		return this->parent;
	}

	const char* getName() const {
		return this->name;
	}

	int getWidth() const {
		return this->width;
	}

	int getHeight() const {
		return this->height;
	}

	int getGravity() const {
		return this->gravity;
	}
	MouseContextType getMouseContext() const {
		return this->mouseContext;
	}
	void setMouseContext(MouseContextType context) {
		this->mouseContext= context;
	}
	long int getSizeFlags() {
		return this->sizeFlags;
	}

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
	void setController(void (*controller)(int wasDestroyed)) {
		this->controller = controller;
	}
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

	/** Find a client by window or parent window.
	 * @param w The window.
	 * @return The client (NULL if not found).
	 */
	static ClientNode *FindClient(Window w);

	/** Find a client by window.
	 * @param w The window.
	 * @return The client (NULL if not found).
	 */
	static ClientNode *FindClientByWindow(Window w);

	/** Find a client by its parent window.
	 * @param p The parent window.
	 * @return The client (NULL if not found).
	 */
	static ClientNode *FindClientByParent(Window p);

	/** Get the active client.
	 * @return The active client (NULL if no client is active).
	 */
	static ClientNode *GetActiveClient(void);

	/*@{*/
	static void InitializeClients();
	static void StartupClients(void);
	static void ShutdownClients(void);
	static void DestroyClients();
	/*@}*/

	/** Add a window to management.
	 * @param w The client window.
	 * @param alreadyMapped 1 if the window is mapped, 0 if not.
	 * @param notOwner 1 if JWM doesn't own this window, 0 if JWM is the owner.
	 * @return The client window data.
	 */
	//static ClientNode *AddClientWindow(Window w, char alreadyMapped, char notOwner);
	/** Remove a client from management.
	 * @param np The client to remove.
	 */
	void RemoveClient();

	/** Minimize a client.
	 * @param np The client to minimize.
	 * @param lower Set to lower the client in the stacking order.
	 */
	void MinimizeClient(char lower);

	/** Shade a client.
	 * @param np The client to shade.
	 */
	void ShadeClient();

	/** Unshade a client.
	 * @param np The client to unshade.
	 */
	void UnshadeClient();

	/** Set a client's status to withdrawn.
	 * A withdrawn client is a client that is not visible in any way to the
	 * user. This may be a window that an application keeps around so that
	 * it can be reused at a later time.
	 * @param np The client whose status to change.
	 */
	void SetClientWithdrawn();

	/** Restore a client from minimized state.
	 * @param np The client to restore.
	 * @param raise 1 to raise the client, 0 to leave stacking unchanged.
	 */
	void RestoreClient(char raise);

	/** Maximize a client.
	 * @param np The client to maximize (NULL is allowed).
	 * @param flags The type of maximization to perform.
	 */
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
	void FocusClient();

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

	/** Restack the clients.
	 * This is used when a client is mapped so that the stacking order
	 * remains consistent.
	 */
	static void RestackClients(void);

	/** Set the layer of a client.
	 * @param np The client whose layer to set.
	 * @param layer the layer to assign to the client.
	 */
	void SetClientLayer(unsigned int layer);

	/** Set the desktop for a client.
	 * @param np The client.
	 * @param desktop The desktop to be assigned to the client.
	 */
	void SetClientDesktop(unsigned int desktop);

	/** Set the sticky status of a client.
	 * A sticky client will appear on all desktops.
	 * @param np The client.
	 * @param isSticky 1 to make the client sticky, 0 to make it not sticky.
	 */
	void SetClientSticky(char isSticky);

	/** Hide a client.
	 * This is used for changing desktops.
	 * @param np The client to hide.
	 */
	void HideClient();

	/** Show a client.
	 * This is used for changing desktops.
	 * @param np The client to show.
	 */
	void ShowClient();

	/** Update a client's colormap.
	 * @param np The client.
	 */
	void UpdateClientColormap(Colormap cmap);

	/** Reparent a client.
	 * This will create a window for a frame (or destroy it) depending on
	 * whether a client needs a frame.
	 * @param np The client.
	 */
	void ReparentClient();

	/** Send a configure event to a client.
	 * This will send updated location and size information to a client.
	 * @param np The client to get the event.
	 */
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
  void resetMaxFlags() {
    this->maxFlags = MAX_NONE;
  }
  void setDelete() {
    this->status |= STAT_DELETE;
  }
  void setTakeFocus() {
    this->status |= STAT_TAKEFOCUS;
  }
  void setNoDelete() {
    this->status &= ~STAT_DELETE;
  }
  void setNoTakeFocus() {
    this->status &= ~STAT_TAKEFOCUS;
  }
  void setLayer(unsigned char layer) {
    this->layer = layer;
  }
  void setDefaultLayer(unsigned char defaultLayer) {
    this->defaultLayer = defaultLayer;
  }
  unsigned char getDefaultLayer() const {
    return this->defaultLayer;
  }
  void clearMaxFlags() {
    this->maxFlags = MAX_NONE;
  }
  void resetBorder() {
    this->border = BORDER_DEFAULT;
  }
  void resetLayer() {
    this->layer = LAYER_NORMAL;
  }
  void resetDefaultLayer() {
    this->defaultLayer = LAYER_NORMAL;
  }
  void clearBorder() {
    this->border = BORDER_NONE;
  }
  void clearStatus() {
    this->status = STAT_NONE;
  }

  void setNotMapped() {
    this->status &= ~STAT_MAPPED;
  }

  unsigned int getOpacity() {
    return this->opacity;
  }

  unsigned short getDesktop() const {
    return this->desktop;
  }
  void setShaped() {
    this->status |= STAT_SHAPED;
  }
  void resetLayerToDefault() {
    this->SetClientLayer(this->getDefaultLayer());
    this->layer = this->defaultLayer;
  }
  void setDesktop(unsigned short desktop) {
    this->desktop = desktop;
  }
  unsigned char getLayer() const {
    return this->layer;
  }

  unsigned short getBorder() const {
    return this->border;
  }

  unsigned char getMaxFlags() const {
    return this->maxFlags;
  }

  bool isStatus(unsigned int flags) const {
    return (this->status & flags);
  }
  bool isFullscreen() const {
    return isStatus(STAT_FULLSCREEN);
  }
  bool isPosition() const {
    return isStatus(STAT_POSITION);
  }
  bool isMapped() const {
    return isStatus(STAT_MAPPED);
  }
  bool isShaded() const {
    return isStatus(STAT_SHADED);
  }
  bool isMinimized() const {
    return isStatus(STAT_MINIMIZED);
  }
  bool isShaped() const {
    return isStatus(STAT_SHAPED);
  }
  bool isIgnoringProgramPosition() const {
    return isStatus(STAT_PIGNORE);
  }
  bool isTiled() const {
    return isStatus(STAT_TILED);
  }
  bool isCentered() const {
    return isStatus(STAT_CENTERED);
  }
  bool isUrgent() const {
    return isStatus(STAT_URGENT);
  }
  bool isDialogWindow() const {
    return isStatus(STAT_WMDIALOG);
  }
  bool isHidden() const {
    return isStatus(STAT_HIDDEN);
  }
  bool hasOpacity() const {
    return isStatus(STAT_OPACITY);
  }
  bool isSticky() const {
    return isStatus(STAT_STICKY);
  }
  bool isActive() const {
    return isStatus(STAT_ACTIVE);
  }
  bool isFixed() const {
    return isStatus(STAT_FIXED);
  }
  bool willIgnoreIncrementWhenMaximized() const {
    return isStatus(STAT_IIGNORE);
  }
  bool canFocus() const {
    return isStatus(STAT_CANFOCUS);
  }
  bool shouldTakeFocus() const {
    return isStatus(STAT_TAKEFOCUS);
  }
  bool shouldDelete() const {
    return isStatus(STAT_DELETE);
  }
  bool shouldFlash() const {
    return isStatus(STAT_FLASH);
  }
  bool isNotUrgent() const {
    return isStatus(STAT_NOTURGENT);
  }
  bool shouldSkipInTaskList() const {
    return isStatus(STAT_NOLIST);
  }
  bool wasMinimizedToShowDesktop() const {
    return isStatus(STAT_SDESKTOP);
  }
  bool isDragable() const {
    return isStatus(STAT_DRAG);
  }
  bool isNotDraggable() const {
    return isStatus(STAT_NODRAG);
  }
  bool shouldIgnoreSpecifiedList() const {
    return isStatus(STAT_ILIST);
  }
  bool shouldIgnorePager() const {
    return isStatus(STAT_IPAGER);
  }
  bool notFocusableIfMapped() const {
    return isStatus(STAT_NOFOCUS);
  }
  bool shouldNotShowInPager() const {
    return isStatus(STAT_NOPAGER);
  }
  bool isAeroSnapEnabled() const {
    return isStatus(STAT_AEROSNAP);
  }

  void setActive() {
    this->status |= STAT_ACTIVE;
  }
  void setNotActive() {
    this->status &= ~STAT_ACTIVE;
  }
  void setMaxFlags(MaxFlags flags) {
    this->maxFlags = flags;
  }
  void setOpacity(unsigned int opacity) {
    this->opacity = opacity;
  }

  //TODO: Rename these methods to be better understood
  void setWMDialogStatus() {
    this->status |= STAT_WMDIALOG;
  }
  void setSDesktopStatus() {
    this->status |= STAT_SDESKTOP;
  }

  void setMapped() {
    this->status |= STAT_MAPPED;
  }
  void setCanFocus() {
    this->status |= STAT_CANFOCUS;
  }
  void setUrgent() {
    this->status |= STAT_URGENT;
  }
  void setNoFlash() {
    this->status &= ~STAT_FLASH;
  }
  void setShaded() {
    this->status |= STAT_SHADED;
  }
  void setMinimized() {
    this->status |= STAT_MINIMIZED;
  }
  void setNoPager() {
    this->status |= STAT_NOPAGER;
  }
  void setNoFullscreen() {
    this->status &= ~STAT_FULLSCREEN;
  }
  void setPositionFromConfig() {
    this->status |= STAT_POSITION;
  }
  void setHasNoList() {
    this->status ^= STAT_NOLIST;
  }
  void setNoShaded() {
    this->status &= ~STAT_SHADED;
  }
  void setNoList() {
    this->status |= STAT_NOLIST;
  }
  void setSticky() {
    this->status |= STAT_STICKY;
  }
  void setNoSticky() {
    this->status &= ~STAT_STICKY;
  }
  void setNoDrag() {
    this->status |= STAT_NODRAG;
  }
  void setNoMinimized() {
    this->status &= ~STAT_MINIMIZED;
  }
  void setNoSDesktop() {
    this->status &= ~STAT_SDESKTOP;
  }
  void clearToNoList() {
    this->status &= ~STAT_NOLIST;
  }
  void clearToNoPager() {
    this->status &= ~STAT_NOPAGER;
  }
  void resetMappedState() {
    this->status &= ~STAT_MAPPED;
  }
  void clearToSticky() {
    this->status &= ~STAT_STICKY;
  }
  void setEdgeSnap() {
    this->status |= STAT_AEROSNAP;
  }
  void setDrag() {
    this->status |= STAT_DRAG;
  }
  void setFixed() {
    this->status |= STAT_FIXED;
  }
  void setCurrentDesktop(unsigned int desktop) {
    this->desktop = desktop;
  }
  void ignoreProgramList() {
    this->status |= STAT_PIGNORE;
  }
  void ignoreProgramSpecificPager() {
    this->status |= STAT_IPAGER;
  }
  void setFullscreen() {
    this->status |= STAT_FULLSCREEN;
  }
  void setMaximized() {
    this->status |= MAX_HORIZ | MAX_VERT;
  }
  void setCentered() {
    this->status |= STAT_CENTERED;
  }
  void setFlash() {
    this->status |= STAT_FLASH;
  }
  void setTiled() {
    this->status |= STAT_TILED;
  }
  void setNotUrgent() {
    this->status |= STAT_NOTURGENT;
  }
  void setTaskListSkipped() {
    this->status |= STAT_NOLIST;
  }
  void setNoFocus() {
    this->status |= STAT_NOFOCUS;
  }
  void setOpacityFixed() {
    this->status |= STAT_OPACITY;
  }
  void ignoreProgramSpecificPosition() {
    this->status |= STAT_PIGNORE;
  }
  void ignoreIncrementWhenMaximized() {
    this->status |= STAT_IIGNORE;
  }

  void setBorderOutline() {
    /**< Window has a border. */
    this->border |= BORDER_OUTLINE;
  }
  void setBorderTitle() {
    /**< Window has a title bar. */
    this->border |= BORDER_TITLE;
  }
  void setBorderMin() {
    /**< Window supports minimize. */
    this->border |= BORDER_MIN;
  }
  void setBorderMax() {
    /**< Window supports maximize. */
    this->border |= BORDER_MAX;
  }
  void setBorderClose() {
    /**< Window supports close. */
    this->border |= BORDER_CLOSE;
  }
  void setBorderResize() {
    /**< Window supports resizing. */
    this->border |= BORDER_RESIZE;
  }
  void setBorderMove() {
    /**< Window supports moving. */
    this->border |= BORDER_MOVE;
  }
  void setBorderMaxVert() {
    /**< Maximize vertically. */
    this->border |= BORDER_MAX_V;
  }
  void setBorderMaxHoriz() {
    /**< Maximize horizontally. */
    this->border |= BORDER_MAX_H;
  }
  void setBorderShade() {
    /**< Allow shading. */
    this->border |= BORDER_SHADE;
  }
  void setBorderConstrain() {
    /**< Constrain to the screen. */
    this->border |= BORDER_CONSTRAIN;
  }
  void setBorderFullscreen() {
    /**< Allow fullscreen. */
    this->border |= BORDER_FULLSCREEN;
  }

  void setNoCanFocus() {
    this->status &= ~STAT_CANFOCUS;
  }
  void setNoBorderOutline() {
    /**< Window has a border. */
    this->border &= ~BORDER_OUTLINE;
  }
  void setNoBorderTitle() {
    /**< Window has a title bar. */
    this->border &= ~BORDER_TITLE;
  }
  void setNoBorderMin() {
    /**< Window supports minimize. */
    this->border &= ~BORDER_MIN;
  }
  void setNoBorderMax() {
    /**< Window supports maximize. */
    this->border &= ~BORDER_MAX;
  }
  void setNoBorderClose() {
    /**< Window supports close. */
    this->border &= ~BORDER_CLOSE;
  }
  void setNoBorderResize() {
    /**< Window supports resizing. */
    this->border &= ~BORDER_RESIZE;
  }
  void setNoBorderMove() {
    /**< Window supports moving. */
    this->border &= ~BORDER_MOVE;
  }
  void setNoBorderMaxVert() {
    /**< Maximize vertically. */
    this->border &= ~BORDER_MAX_V;
  }
  void setNoBorderMaxHoriz() {
    /**< Maximize horizontally. */
    this->border &= ~BORDER_MAX_H;
  }
  void setNoBorderShade() {
    /**< Allow shading. */
    this->border &= ~BORDER_SHADE;
  }
  void setNoBorderConstrain() {
    /**< Constrain to the screen. */
    this->border &= ~BORDER_CONSTRAIN;
  }
  void setNoBorderFullscreen() {
    /**< Allow fullscreen. */
    this->border &= ~BORDER_FULLSCREEN;
  }

  void setNoUrgent() {
    this->status &= ~STAT_URGENT;
  }

  void unsetNoPager() {
    this->status &= ~STAT_NOPAGER;
  }

  void unsetSkippingInTaskList() {
    this->status &= ~STAT_NOLIST;
  }

  void setNotHidden() {
    this->status &= ~STAT_HIDDEN;
  }

  void setHidden() {
    this->status |= STAT_HIDDEN;
  }


	//TODO: move all public static methods below
public:
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
private:

	static Strut *struts;
	/* desktopCount x screenCount */
	/* Note that we assume x and y are 0 based for all screens here. */
	static int *cascadeOffsets;

	static void LoadFocus(void);
	void RestackTransients();
	void RestoreTransients(char raise);
	static void KillClientHandler(ClientNode *np);
	void UnmapClient();
public:
	static ClientNode *activeClient;
};

#endif /* CLIENT_H */

