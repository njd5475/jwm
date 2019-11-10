/*
 * ClientState.h
 *
 *  Created on: Nov 8, 2019
 *      Author: nick
 */

#ifndef SRC_CLIENTSTATE_H_
#define SRC_CLIENTSTATE_H_

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

class ClientState {
public:
	ClientState();
	virtual ~ClientState();

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
	void clearStatus() {
		this->status = STAT_NONE;
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
		this->layer = this->defaultLayer;
	}
	void setDesktop(unsigned short desktop) {
		this->desktop = desktop;
	}
	unsigned char getLayer() const {
		return this->layer;
	}

	unsigned int getStatus() const {
		return this->status;
	}

	unsigned short getBorder() const {
		return this->border;
	}

	unsigned char getMaxFlags() const {
		return this->maxFlags;
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

};

#endif /* SRC_CLIENTSTATE_H_ */
