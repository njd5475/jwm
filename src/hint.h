/**
 * @file hint.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for reading and writing X properties.
 *
 */

#ifndef HINT_H
#define HINT_H

#include "nwm.h"
struct ClientNode;

/** Enumeration of atoms. */
typedef enum {

	/* Misc */
	ATOM_COMPOUND_TEXT,
	ATOM_UTF8_STRING,
	ATOM_XROOTPMAP_ID,
	ATOM_MANAGER,

	/* Standard atoms */
	ATOM_WM_STATE,
	ATOM_WM_PROTOCOLS,
	ATOM_WM_DELETE_WINDOW,
	ATOM_WM_TAKE_FOCUS,
	ATOM_WM_CHANGE_STATE,
	ATOM_WM_COLORMAP_WINDOWS,

	/* WM Spec atoms */
	ATOM_NET_SUPPORTED,
	ATOM_NET_NUMBER_OF_DESKTOPS,
	ATOM_NET_DESKTOP_NAMES,
	ATOM_NET_DESKTOP_GEOMETRY,
	ATOM_NET_DESKTOP_VIEWPORT,
	ATOM_NET_CURRENT_DESKTOP,
	ATOM_NET_ACTIVE_WINDOW,
	ATOM_NET_WORKAREA,
	ATOM_NET_SUPPORTING_WM_CHECK,
	ATOM_NET_SHOWING_DESKTOP,
	ATOM_NET_FRAME_EXTENTS,
	ATOM_NET_WM_DESKTOP,

	ATOM_NET_WM_STATE,
	ATOM_NET_WM_STATE_STICKY,
	ATOM_NET_WM_STATE_MAXIMIZED_VERT,
	ATOM_NET_WM_STATE_MAXIMIZED_HORZ,
	ATOM_NET_WM_STATE_SHADED,
	ATOM_NET_WM_STATE_FULLSCREEN,
	ATOM_NET_WM_STATE_HIDDEN,
	ATOM_NET_WM_STATE_SKIP_TASKBAR,
	ATOM_NET_WM_STATE_SKIP_PAGER,
	ATOM_NET_WM_STATE_BELOW,
	ATOM_NET_WM_STATE_ABOVE,
	ATOM_NET_WM_STATE_DEMANDS_ATTENTION,
	ATOM_NET_WM_STATE_FOCUSED,

	ATOM_NET_WM_ALLOWED_ACTIONS,
	ATOM_NET_WM_ACTION_MOVE,
	ATOM_NET_WM_ACTION_RESIZE,
	ATOM_NET_WM_ACTION_MINIMIZE,
	ATOM_NET_WM_ACTION_SHADE,
	ATOM_NET_WM_ACTION_STICK,
	ATOM_NET_WM_ACTION_FULLSCREEN,
	ATOM_NET_WM_ACTION_MAXIMIZE_HORZ,
	ATOM_NET_WM_ACTION_MAXIMIZE_VERT,
	ATOM_NET_WM_ACTION_CHANGE_DESKTOP,
	ATOM_NET_WM_ACTION_CLOSE,
	ATOM_NET_WM_ACTION_BELOW,
	ATOM_NET_WM_ACTION_ABOVE,

	ATOM_NET_CLOSE_WINDOW,
	ATOM_NET_MOVERESIZE_WINDOW,
	ATOM_NET_RESTACK_WINDOW,
	ATOM_NET_REQUEST_FRAME_EXTENTS,

	ATOM_NET_WM_PID,
	ATOM_NET_WM_NAME,
	ATOM_NET_WM_VISIBLE_NAME,
	ATOM_NET_WM_HANDLED_ICONS,
	ATOM_NET_WM_ICON,
	ATOM_NET_WM_ICON_NAME,
	ATOM_NET_WM_USER_TIME,
	ATOM_NET_WM_USER_TIME_WINDOW,
	ATOM_NET_WM_VISIBLE_ICON_NAME,
	ATOM_NET_WM_WINDOW_TYPE,
	ATOM_NET_WM_WINDOW_TYPE_DESKTOP,
	ATOM_NET_WM_WINDOW_TYPE_DOCK,
	ATOM_NET_WM_WINDOW_TYPE_SPLASH,
	ATOM_NET_WM_WINDOW_TYPE_DIALOG,
	ATOM_NET_WM_WINDOW_TYPE_NORMAL,
	ATOM_NET_WM_WINDOW_TYPE_MENU,
	ATOM_NET_WM_WINDOW_TYPE_NOTIFICATION,
	ATOM_NET_WM_WINDOW_TYPE_TOOLBAR,
	ATOM_NET_WM_WINDOW_TYPE_UTILITY,

	ATOM_NET_CLIENT_LIST,
	ATOM_NET_CLIENT_LIST_STACKING,

	ATOM_NET_WM_STRUT_PARTIAL,
	ATOM_NET_WM_WINDOW_OPACITY,
	ATOM_NET_WM_STRUT,
	ATOM_NET_WM_MOVERESIZE,

	ATOM_NET_SYSTEM_TRAY_OPCODE,
	ATOM_NET_SYSTEM_TRAY_ORIENTATION,

	/* MWM atoms */
	ATOM_MOTIF_WM_HINTS,

	/* NWM-specific atoms. */
	ATOM_NWM_RESTART,
	ATOM_NWM_EXIT,
	ATOM_NWM_RELOAD,
	ATOM_NWM_WM_STATE_MAXIMIZED_TOP,
	ATOM_NWM_WM_STATE_MAXIMIZED_BOTTOM,
	ATOM_NWM_WM_STATE_MAXIMIZED_LEFT,
	ATOM_NWM_WM_STATE_MAXIMIZED_RIGHT,

	ATOM_COUNT
} AtomType;

extern const char nwmRestart[];
extern const char nwmExit[];
extern const char nwmReload[];
extern const char managerProperty[];

#define FIRST_NET_ATOM ATOM_NET_SUPPORTED
#define LAST_NET_ATOM  ATOM_NET_WM_STRUT

#define FIRST_MWM_ATOM ATOM_MOTIF_WM_HINTS
#define LAST_MWM_ATOM  ATOM_MOTIF_WM_HINTS

typedef unsigned char WinLayerType;

class Hints {
public:
	static Atom atoms[ATOM_COUNT];
	typedef struct {
	  Atom *atom;
	  const char *name;
	} AtomNode;
	static const AtomNode atomList[];

	/*@{*/
	static void InitializeHints() {}
	static void StartupHints(void);
	static void ShutdownHints() {}
	static void DestroyHints() {}
	/*@}*/

	/** Determine the current desktop. */
	static void ReadCurrentDesktop(void);

	/** Read client info.
	 * @param np The client.
	 * @param alreadyMapped Set if the client is already mapped.
	 */
	static void ReadClientInfo(ClientNode *np, bool alreadyMapped);

	/** Read a client's name.
	 * @param np The client.
	 */
	static void ReadWMName(ClientNode *np);

	/** Read a client's class.
	 * @param np The client.
	 */
	static void ReadWMClass(const ClientNode *np);

	/** Read normal hints for a client.
	 * @param np The client.
	 */
	static void ReadWMNormalHints(const ClientNode *np);

	/** Read the WM_PROTOCOLS property for a window.
	 * @param w The window.
	 * @param state The client state to update.
	 */
	//TODO: Move to clientnode
	static void ReadWMProtocols(Window w, ClientNode *node);

	/** Read colormap information for a client.
	 * @param np The client.
	 */
	static void ReadWMColormaps(const ClientNode *np);

	/** Determine the layer of a client.
	 * @param np The client.
	 */
	static void ReadWinLayer(const ClientNode *np);

	/** Read the current state of a window.
	 * @param win The window.
	 * @param alreadyMapped Set if the window is already mapped.
	 * @return The window state.
	 */
  //TODO: Move to clientnode
	static void ReadWindowState(ClientNode* node, Window win, bool alreadyMapped);

	/** Read WM hints.
	 * @param win The window.
	 * @param state The state hints to update.
	 * @param alreadyMapped Set if the window is already mapped.
	 */
  //TODO: Move to clientnode
	static void ReadWMHints(Window win, ClientNode *node, bool alreadyMapped);

	/** Read opacity.
	 * @param win The window.
	 * @param opacity The opacity to update.
	 */
	static void ReadWMOpacity(Window win, unsigned *opacity);

	/** Set the state of a client window.
	 * Note that this will call WriteNetState.
	 * @param np The client.
	 */
	static void WriteState(const ClientNode *np);

	/** Set _NET_WM_STATE.
	 * @param np The client.
	 */
	static void WriteNetState(const ClientNode *np);

	/** Set the opacity of a client window.
	 * @param np The client.
	 * @param opacity The opacity to set.
	 * @param force Set the opacity even if it hasn't changed.
	 */
	static void SetOpacity(const ClientNode *np, unsigned int opacity, char force);

	/** Set the frame extents of a window.
	 * @param win The window.
	 * @param state The client state.
	 */
	static void WriteFrameExtents(Window win, const ClientNode *state);

	/** Read a cardinal atom.
	 * @param window The window.
	 * @param atom The atom to read.
	 * @param value A pointer to the location to save the atom.
	 * @return 1 on success, 0 on failure.
	 */
	static char GetCardinalAtom(Window window, AtomType atom, unsigned long *value);

	/** Set a cardinal atom.
	 * @param window The window.
	 * @param atom The atom to set.
	 * @param value The value.
	 */
	static void SetCardinalAtom(Window window, AtomType atom, unsigned long value);

	/** Read a window atom.
	 * @param window The window.
	 * @param atom The atom to read.
	 * @param value A pointer to the location to save the atom.
	 * @return 1 on success, 0 on failure.
	 */
	static char GetWindowAtom(Window window, AtomType atom, Window *value);

	/** Set a window atom.
	 * @param window The window.
	 * @param atom The atom to set.
	 * @param value The value.
	 */
	static void SetWindowAtom(Window window, AtomType atom, unsigned long value);

	/** Set a pixmap atom.
	 * @param window The window.
	 * @param atom The atom to set.
	 * @param value The value.
	 */
	static void SetPixmapAtom(Window window, AtomType atom, Pixmap value);

	/** Set an atom atom.
	 * @param window The window.
	 * @param atom The atom to set.
	 * @param value The value.
	 */
	static void SetAtomAtom(Window window, AtomType atom, AtomType value);

	static bool IsDeleteAtomSet(Window w);
private:

	static char CheckShape(Window win);
  //TODO: Move to clientnode
	static void WriteNetAllowed(const ClientNode *np);
	static void ReadWMState(Window win, ClientNode *state);
	static void ReadMotifHints(Window win, ClientNode *state);

};

#endif /* HINT_H */

