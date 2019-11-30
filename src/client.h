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

#include "ClientState.h"

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
public:
	ClientNode(Window w, char alreadyMapped, char notOwner);
	virtual ~ClientNode() {}
private:
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

	ClientState state; /**< Window state. */

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

	/** The number of clients (maintained in client.c). */
	static unsigned int clientCount;

	void DrawBorder() {
		if (!(getState()->getStatus() & (STAT_HIDDEN | STAT_MINIMIZED))) {
			Border::DrawBorder(this);
		}
	}
	unsigned int getDesktop() const {
	  return this->state.getDesktop();
	}
	Window getOwner() const {
		return this->owner;
	}
	void setOpacity(unsigned int opacity) {
		this->state.setOpacity(opacity);
	}
	void setNoPager() {
		this->state.setNoPager();
	}
	void setHasNoList() {
		this->state.setHasNoList();
	}
	void setNoList() {
		this->state.setNoList();
	}
	void clearToNoList() {
		this->state.clearToNoList();
	}
	void clearToNoPager() {
		this->state.clearToNoPager();
	}
	void clearToSticky() {
		this->state.clearToSticky();
	}
	void setOwner(Window owner) {
		this->owner = owner;
	}
	void setNoBorderTitle() {
		this->state.setNoBorderTitle();
	}
	void ignoreProgramSpecificPosition() {
		this->state.ignoreProgramSpecificPosition();
	}
	void ignoreIncrementWhenMaximized() {
		this->state.ignoreIncrementWhenMaximized();
	}
	void setNoBorderShade() {
		this->state.setNoBorderShade();
	}
	void setBorderTitle() {
		this->state.setBorderTitle();
	}
	void setNoBorderOutline() {
		this->state.setNoBorderOutline();
	}
	void setBorderOutline() {
		this->state.setBorderOutline();
	}
	void unsetNoPager() {
		this->state.unsetNoPager();
	}
	void setSticky() {
		this->state.setSticky();
	}
	void ignoreProgramList() {
		this->state.ignoreProgramList();
	}
	void unsetSkippingInTaskList() {
		this->state.unsetSkippingInTaskList();
	}
	void ignoreProgramSpecificPager() {
		this->state.ignoreProgramSpecificPager();
	}
	void setMaximized() {
		this->state.setMaximized();
	}
	void setMinimized() {
		this->state.setMinimized();
	}
	void setOpacityFixed() {
		this->state.setOpacityFixed();
	}
	void setNoBorderMaxHoriz() {
		this->state.setNoBorderMaxHoriz();
	}
	void setNoBorderMaxVert() {
		this->state.setNoBorderMaxVert();
	}
	void setNoFocus() {
		this->state.setNoFocus();
	}
	void setNotHidden() {
		this->state.setNotHidden();
	}
	void setHidden() {
		this->state.setHidden();
	}
	void setNoBorderMin() {
		this->state.setNoBorderMin();
	}
	void setNoBorderMax() {
		this->state.setNoBorderMax();
	}
	void setNoBorderClose() {
		this->state.setNoBorderClose();
	}
	void setNoBorderMove() {
		this->state.setNoBorderMove();
	}
	void setNoBorderResize() {
		this->state.setNoBorderResize();
	}
	void setCentered() {
		this->state.setCentered();
	}
	void setTiled() {
		this->state.setTiled();
	}
	void setNotUrgent() {
		this->state.setNotUrgent();
	}
	void setNoFlash() {
		this->state.setNoFlash();
	}
	void setNoCanFocus() {
		this->state.setNoCanFocus();
	}
	void setCanFocus() {
		this->state.setCanFocus();
	}
	void changeState(ClientState state) {
		this->state = state;
	}
	void setUrgent() {
		this->state.setUrgent();
	}
	void setBorderConstrain() {
		this->state.setBorderConstrain();
	}
	void setFullscreen() {
		this->state.setFullscreen();
	}
	void setNoBorderFullscreen() {
		this->state.setNoBorderFullscreen();
	}
	void setDrag() {
		this->state.setDrag();
	}
	void setFixed() {
		this->state.setFixed();
	}
	void setEdgeSnap() {
		this->state.setEdgeSnap();
	}
	void setNoDrag() {
		this->state.setNoDrag();
	}
	void setPositionFromConfig() {
		this->state.setPositionFromConfig();
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
	void setDesktop(unsigned int desktop) {
	  this->state.setDesktop(desktop);
	}
	//TODO: Rename these methods to be better understood
	void setWMDialogStatus() {
		this->state.setWMDialogStatus();
	}
	//TODO: Rename these methods to be better understood
	void setSDesktopStatus() {
		this->state.setSDesktopStatus();
	}
	void resetLayerToDefault() {
		this->SetClientLayer(this->state.getDefaultLayer());
	}
	void clearStatus() {
		this->state.clearStatus();
	}
	void setCurrentDesktop(unsigned char desktop) {
		this->state.setCurrentDesktop(desktop);
	}
	void setMapped() {
		this->state.setMapped();
	}
	bool isMapped() {
		return this->state.getStatus() & STAT_MAPPED;
	}
	void setShaded() {
		this->state.setShaded();
	}
	int getBaseWidth() {return this->baseWidth;}
	int getBaseHeight() {return this->baseHeight;}
	int getMaxWidth() {return this->maxWidth;}
	int getMaxHeight() {return this->maxHeight;}
	int getMinWidth() {return this->minWidth;}
	int getMinHeight() {return this->minHeight;}
	int getYInc() {return this->yinc;}
	int getXInc() {return this->xinc;}

	unsigned int getStatus() const {
		return this->state.getStatus();
	}
	AspectRatio getAspect() {return this->aspect;}

	void (*getController())(int) {
		return this->controller;
	}
	const char* getClassName() {
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
	const ClientState* getState() const {
		return &this->state;
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

	void setLayer(unsigned int layer) {
		this->state.setLayer(layer);
	}
	unsigned int getLayer() const {
		return this->state.getLayer();
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

