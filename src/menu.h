/**
 * @file menu.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the menu functions.
 *
 */

#ifndef MENU_H
#define MENU_H

#include <X11/X.h>
#include <X11/Xlib.h>

#include "timing.h"

#include "Graphics.h"
//class Graphics;
struct IconNode;

struct ScreenType;

/** Enumeration of menu action types. */
typedef unsigned char MenuActionType;
#define MA_NONE               0
#define MA_EXECUTE            1
#define MA_DESKTOP            2
#define MA_SENDTO             3
#define MA_LAYER              4
#define MA_STICK              5
#define MA_MAXIMIZE           6
#define MA_MAXIMIZE_H         7
#define MA_MAXIMIZE_V         8
#define MA_MINIMIZE           9
#define MA_RESTORE            10
#define MA_SHADE              11
#define MA_MOVE               12
#define MA_RESIZE             13
#define MA_KILL               14
#define MA_CLOSE              15
#define MA_EXIT               16
#define MA_RESTART            17
#define MA_DYNAMIC            18
#define MA_SENDTO_MENU        19
#define MA_DESKTOP_MENU       20
#define MA_WINDOW_MENU        21
#define MA_ACTION_MASK        0x7F
#define MA_GROUP_MASK         0x80

struct Menu;
class ClientNode;

/** Enumeration of possible menu elements. */
typedef unsigned char MenuItemType;
#define MENU_ITEM_NORMAL      0  /**< Normal menu item (button). */
#define MENU_ITEM_SUBMENU     1  /**< Submenu. */
#define MENU_ITEM_SEPARATOR   2  /**< Item separator. */

/** Structure to represent a menu item. */
class MenuItem {
public:
  MenuItem(Menu* menu, const char* name, MenuItemType type, const char* tooltip, const char* iconName);
  virtual ~MenuItem();

  void setAction(MenuActionType type, void *context, unsigned value);
  void setActionStr(MenuActionType type, const char* str);
  void Draw(Graphics *graphics, bool active, unsigned offset,
      unsigned width);
  bool isSeparator() const;
  bool isNormal() const;
  bool isSubMenu() const;
  Menu* getSubMenu();
  int getHeight() const;
  int getWidth() const;
  const char* getTooltip();
  void removeTemporaryItems();
  Menu* CreateSubMenu();

  /** Structure to represent a menu action for callbacks. */
  struct MenuAction {

    void *context; /**< Action context (client, etc.). */

    /* Extra data for the action. */
    char *str;
    unsigned value;
    unsigned timeout_ms;

    MenuActionType type; /**< Type of action. */

  };

  typedef void (*RunMenuCommandType)(MenuAction *action, unsigned button);

  MenuAction *getAction();

private:
  Menu *_parent;
  MenuItemType _type; /**< The menu item type. */
  const char *_name; /**< Name to display (or NULL). */
  const char *_tooltip; /**< Tooltip to display (or NULL). */
  const char *_iconName; /**< Name of an icon to show (or NULL). */
  Menu *_submenu; /**< Submenu (or NULL). */

  /** An icon for this menu item.
   * This field is handled by menu.c if iconName is set. */
  IconNode *_icon; /**< Icon to display. */

  MenuAction _action; /**< Action to take if selected (or NULL). */
};

/** Structure to represent a menu or submenu. */
class Menu {
public:


  Menu();
  virtual ~Menu();

  void Draw();
  void freeGraphics();
  void addItem(MenuItem *item);
  void addSeparator();
  void setActiveItem(int index);
  MenuItem *getItem(int index);
  bool valid() const;
  int getItemHeight() const;
  Window getWindow();
  Menu* getParent();
  int getItemCount();
  int getCurrentIndex();
  int getLastIndex();
  int getParentOffset();
  TimeType getLastTime();
  int getOffset(int index);
  int getX() const;
  int getY() const;
  int getMouseX() const;
  int getMouseY() const;
  int getHeight() const;
  int getWidth() const;
  void clearSelection();
  void nextIndex();
  void removeTemporaryItems();
  void SetLastIndex();
  void UpdateSelected(int y);
  MenuItem* getCurrentItem();
  void mouseEvent(int x, int y, const TimeType now);
  void UpdatePosition(int x, int y, char keyboard);
  const struct ScreenType* getScreen();
private:
  /* These fields must be set before calling ShowMenu */
  std::vector<MenuItem*> items; /**< Menu items. */
  char *_label; /**< Menu label (NULL for no label). */
  char *_dynamic; /**< Generating command of dynamic menu. */
  unsigned _timeout_ms; /**< Timeout in milliseconds for dynamic menus. */

  /* These fields are handled by menu.c */
  Window _window; /**< The menu window. */
  //Pixmap pixmap; /**< Pixmap where the menu is rendered. */
  Graphics *_graphics;
  int _x; /**< The x-coordinate of the menu. */
  int _y; /**< The y-coordinate of the menu. */
  int _currentIndex; /**< The current menu selection. */
  int _lastIndex; /**< The last menu selection. */
  unsigned int _itemCount; /**< Number of menu items (excluding separators). */
  int _parentOffset; /**< y-offset of this menu wrt the parent. */
  int _textOffset; /**< x-offset of text in the menu. */
  Menu* _parent; /**< The parent menu (or NULL). */
  //const struct ScreenType* _screen;
  int _mousex, _mousey;
  TimeType _lastTime;

};

typedef unsigned char MenuSelectionType;

class Menus {

public:
  /** Allocate an empty menu. */
  static Menu *CreateMenu();

  /** Create an empty menu item. */
  static MenuItem *CreateMenuItem(Menu *menu, MenuItemType type, const char* name, const char* tooltip, const char* iconName);

  /** Initialize a menu structure to be shown.
   * @param menu The menu to initialize.
   */
  static void InitializeMenu(Menu *menu);

  /** Show a menu.
   * @param menu The menu to show.
   * @param runner Callback executed when an item is selected.
   * @param x The x-coordinate of the menu.
   * @param y The y-coordinate of the menu.
   * @param keyboard Set if the request came from a key binding.
   * @return 1 if the menu was shown, 0 otherwise.
   */
  static char ShowMenu(Menu *menu, MenuItem::RunMenuCommandType runner,
      int x, int y, char keyboard);

  /** Destroy a menu structure.
   * @param menu The menu to destroy.
   */
  static void DestroyMenu(Menu *menu);
  static int GetMenuIndex(Menu *menu, int index);
private:
  static char ShowSubmenu(Menu *menu, Menu *parent,
      MenuItem::RunMenuCommandType runner,
      int x, int y, char keyboard);

  static void PatchMenu(Menu *menu);
  static void UnpatchMenu(Menu *menu);
  static void MapMenu(Menu *menu, int x, int y, char keyboard);
  static void HideMenu(Menu *menu);
  static void DrawMenu(Menu *menu);

  static char MenuLoop(Menu *menu, MenuItem::RunMenuCommandType runner);
  static void MenuCallback(const TimeType *now, int x, int y,
      Window w, void *data);
  static MenuSelectionType UpdateMotion(Menu *menu,
      MenuItem::RunMenuCommandType runner,
      XEvent *event);

  static void UpdateMenu(Menu *menu, int x, int y);
  static void DrawMenuItem(Menu *menu, MenuItem *item, int index);
  static MenuItem *GetMenuItem(Menu *menu, int index);
  static int GetNextMenuIndex(Menu *menu);
  static int GetPreviousMenuIndex(Menu *menu);
  static void SetPosition(Menu *tp, int index);
  static char IsMenuValid(const Menu *menu);

  static std::vector<Menu*> menus;

};
/** The number of open menus. */
extern int menuShown;

#endif /* MENU_H */

