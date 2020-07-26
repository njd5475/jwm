/**
 * @file menu.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Menu display and handling functions.
 *
 */

#include "jwm.h"
#include "menu.h"
#include "font.h"
#include "client.h"
#include "icon.h"
#include "cursor.h"
#include "button.h"
#include "event.h"
#include "root.h"
#include "settings.h"
#include "parse.h"
#include "winmenu.h"
#include "screen.h"
#include "hint.h"
#include "misc.h"
#include "popup.h"
#include "binding.h"
#include "DesktopEnvironment.h"

#define BASE_ICON_OFFSET   3
#define MENU_BORDER_SIZE   1

#define MENU_NOSELECTION   0
#define MENU_LEAVE         1
#define MENU_SUBSELECT     2

int menuShown = 0;

std::vector<Menu*> Menus::menus;

/** Allocate an empty menu. */
Menu* Menus::CreateMenu() {
  Menu *menu = new Menu();
  Menus::menus.push_back(menu);
  return menu;
}

/** Create an empty menu item. */
MenuItem* Menus::CreateMenuItem(Menu *menu, MenuItemType type, const char *name,
    const char *tooltip, const char *iconName) {
  MenuItem *item = new MenuItem(menu, name, type, tooltip, iconName);
  menu->addItem(item);
  return item;
}

/** Initialize a menu. */
void Menus::InitializeMenu(Menu *menu) {

}

/** Show a menu. */
char Menus::ShowMenu(Menu *menu, MenuItem::RunMenuCommandType runner, int x,
    int y) {
  Log("Should show the menu\n");

  menu->Show(runner, x, y);

  return 1;
}

/** Hide a menu. */
void Menus::HideMenu(Menu *menu) {
  menu->Hide();
}

/** Destroy a menu. */
void Menus::DestroyMenu(Menu *menu) {
  if (menu) {
    delete menu;
  }
}

/** Show a submenu. */
bool Menus::ShowSubmenu(Menu *menu, Menu *parent,
    MenuItem::RunMenuCommandType runner, int x, int y) {
  return menu->ShowSubMenu(runner, x, y);
}

/** Prepare a menu to be shown. */
void Menus::PatchMenu(Menu *menu) {
  Log("Preparing the patch menu");
//  MenuItem *item;
//  for (item = menu->items; item; item = item->next) {
//    Menu *submenu = NULL;
//    switch (item->action.type & MA_ACTION_MASK) {
//    case MA_DESKTOP_MENU:
//      submenu = DesktopEnvironment::DefaultEnvironment()->CreateDesktopMenu(
//          1 << currentDesktop, item->action.context);
//      break;
//    case MA_SENDTO_MENU:
//      submenu = DesktopEnvironment::DefaultEnvironment()->CreateSendtoMenu(
//          item->action.type & ~MA_ACTION_MASK, item->action.context);
//      break;
//    case MA_WINDOW_MENU:
//      submenu = CreateWindowMenu((ClientNode*) item->action.context);
//      break;
//    case MA_DYNAMIC:
//      if (!item->getSubMenu()) {
//        submenu = Parser::ParseDynamicMenu(item->action.timeout_ms,
//            item->action.str);
//        if (JLIKELY(submenu)) {
//          submenu->getItemHeight() = item->action.value;
//        }
//      }
//      break;
//    default:
//      break;
//    }
//    if (submenu) {
//      InitializeMenu(submenu);
//      item->getSubMenu() = submenu;
//    }
//  }
}

/** Remove temporary items from a menu. */
void Menus::UnpatchMenu(Menu *menu) {
  menu->removeTemporaryItems();
}

/** Menu process loop.
 * Returns 0 if no selection was made or 1 if a selection was made.
 */
char Menus::MenuLoop(Menu *menu, MenuItem::RunMenuCommandType runner) {


}

/** Signal for showing popups. */
void Menus::MenuCallback(const TimeType *now, int x, int y, Window w,
    void *data) {
  Menu *menu = (Menu*) data;
  MenuItem *item;
  int i;

  /* Check if the mouse moved (and reset if it did). */
  menu->mouseEvent(x, y, *now); // always update the mouse position

  /* See if enough time has passed. */
  //TODO: add tooltip popup delay
//  TimeType lastTime = menu->getLastTime();
//  const unsigned long diff = GetTimeDifference(now, &lastTime);
//  if (diff < settings.popupDelay) {
//    return;
//  }
//
//  if (item->getTooltip()) {
//    Popups::ShowPopup(x, y, item->getTooltip(), POPUP_MENU);
//  }
}

/** Create and map a menu. */
void Menus::MapMenu(Menu *menu, int x, int y) {
  menu->UpdatePosition(x, y);
}

/** Draw a menu. */
void Menus::DrawMenu(Menu *menu) {
  menu->Draw();
}

/** Determine the action to take given an event. */
// TODO: Fix this ridiculous method
MenuSelectionType Menus::UpdateMotion(Menu *menu,
    MenuItem::RunMenuCommandType runner, XEvent *event) {

  MenuItem *ip;
  Menu *tp;
  Window subwindow;
  int x, y;

  vLog("Menus received motion update [%d, %d]\n", x, y);
  if (event->type == MotionNotify) {

    Cursors::SetMousePosition(event->xmotion.x_root, event->xmotion.y_root,
        event->xmotion.window);
    Events::_DiscardMotionEvents(event, menu->getWindow());

    x = event->xmotion.x_root - menu->getX();
    y = event->xmotion.y_root - menu->getY();
    subwindow = event->xmotion.subwindow;
    menu->mouseEvent(x + menu->getX(), y + menu->getY(), TimeType { 0 });
    menu->Draw();

  } else if (event->type == ButtonPress) {

//    if (event->xbutton.button == Button4) {  y = GetPreviousMenuIndex(tp);
//    } else if (event->xbutton.button == Button5) {  y = GetNextMenuIndex(tp);

    return MENU_NOSELECTION;

  } else if (event->type == KeyPress) {
//    ActionType action;
//
//    y = -1;
//    action = Binding::GetKey(MC_NONE, event->xkey.state, event->xkey.keycode);
//    switch (action.action) {
//    case UP:y = GetPreviousMenuIndex(tp);break;
//    case DOWN:y = GetNextMenuIndex(tp);break;
//    case RIGHT: tp = menu; y = 0;break;
//    case LEFT: // move to parent if there is a parent
//    case ESC: return MENU_SUBSELECT;
//    case ENTER:
//      //ip = GetMenuItem(tp, tp->getCurrentIndex());
//
//      if (ip != NULL) {
//        HideMenu(menu);
//        (runner)(ip->getAction(), 0); //call it
//      }
//      return MENU_SUBSELECT;
//    default:
//      break;
//    }
//
//    if (y >= 0) {
//      SetPosition(tp, y);
//    }
//
    return MENU_NOSELECTION;

  } else {
    Debug("invalid event type in menu.c:UpdateMotion");
    return MENU_SUBSELECT;
  }

  /* Update the selection on the current menu */
  if (x > 0 && y > 0 && x < menu->getWidth() && y < menu->getHeight()) {
    //menu->UpdateSelected(y);
  } else if (menu->getParent() && subwindow != menu->getParent()->getWindow()) {

    /* Leave if over a menu window. */
    for (tp = menu->getParent()->getParent(); tp; tp = tp->getParent()) {
      if (tp->getWindow() == subwindow) {
        return MENU_LEAVE;
      }
    }

  } else {

    /* Leave if over the parent, but not on this selection. */
    tp = menu->getParent();
    if (tp && subwindow == tp->getWindow()) {
      if (y < menu->getParentOffset()
          || y > tp->getItemHeight() + menu->getParentOffset()) {
        return MENU_LEAVE; //passing instuctions to the caller
      }
    }

  }

  return MENU_NOSELECTION;

}

/** Update the menu selection. */
void Menus::UpdateMenu(Menu *menu, int x, int y) {
  menu->UpdatePosition(x, y);
  menu->Draw();
}

/** Determine if a menu is valid (and can be shown). */
bool Menus::IsMenuValid(const Menu *menu) {
  if (menu) {
    return menu->valid();
  }
  return 0;
}

MenuItem::MenuItem(Menu *parent, const char *name, MenuItemType type,
    const char *tooltip, const char *iconName) :
    _name(name ? strdup(name) : NULL), _type(type), _tooltip(
        tooltip ? strdup(tooltip) : NULL), _iconName(
        iconName ? strdup(iconName) : NULL), _submenu(
    NULL), _icon(NULL), _parent(parent) {
  if (_iconName) {
    _icon = Icon::LoadNamedIcon(_iconName, 1, 1);
    if (!_icon) {
      vLog("Could not load icon %s\n", _iconName);
    }
  }
  memset(&this->_action, 0, sizeof(MenuAction));
}

MenuItem::~MenuItem() {
  if (_name) {
    delete[] (_name);
  }

  if (_tooltip) {
    delete[] _tooltip;
  }

  switch (_action.type & MA_ACTION_MASK) {
  case MA_EXECUTE:
  case MA_EXIT:
  case MA_DYNAMIC:
    if (_action.str) {
      delete[] (_action.str);
    }
    break;
  default:
    break;
  }

  if (_iconName) {
    delete[] (_iconName);
  }

  if (getSubMenu()) {
    delete getSubMenu();
  }
}

void MenuItem::Draw(Graphics *graphics, bool active, unsigned offset,
    unsigned width) {
  if (_type != MENU_ITEM_SEPARATOR) {
    ButtonType type = BUTTON_LABEL;
    ColorName fg = COLOR_MENU_FG;

    if (_icon == NULL && _iconName) {
      _icon = Icon::LoadNamedIcon(_iconName, 1, 1);
    }

    if (this->_parent && this->_parent->getMouseX() > -1
        && this->_parent->getMouseY() > -1) {
      int mx = _parent->getMouseX() - _parent->getX(), my = _parent->getMouseY()
          - _parent->getY();
      active = my > offset && my < offset + getHeight() && mx > 0 && mx < width;
    }

    if (active) {
      type = BUTTON_MENU_ACTIVE;
      fg = COLOR_MENU_ACTIVE_FG;
    }

    graphics->drawButton(type, ALIGN_LEFT, FONT_MENU, _name, true, false, _icon,
    MENU_BORDER_SIZE, offset, width - MENU_BORDER_SIZE * 2,
        _parent->getItemHeight() - 1, 0, 0);

    if (_submenu) {

      _submenu->UpdatePosition(
          this->_parent->getX() + this->_parent->getWidth(),
          this->_parent->getY());
      const int asize = (_parent->getItemHeight() + 7) / 8;
      const int y = offset + (_parent->getItemHeight() + 1) / 2;
      int x = width - 2 * asize - 1;
      int i;

      JXSetForeground(display, rootGC, Colors::lookupColor(fg));
      for (i = 0; i < asize; i++) {
        const int y1 = y - asize + i;
        const int y2 = y + asize - i;
        graphics->line(x, y1, x, y2);
        x += 1;
      }
      graphics->point(x, y);

      if (active) {
        _submenu->Draw();
      }
    }

  } else {
    if (settings.menuDecorations == DECO_MOTIF) {
      graphics->setForeground(COLOR_MENU_DOWN);
      graphics->line(4, offset + 2, width - 6, offset + 2);
      graphics->setForeground(COLOR_MENU_UP);
      graphics->line(4, offset + 3, width - 6, offset + 3);
    } else {
      graphics->setForeground(COLOR_MENU_FG);
      graphics->line(4, offset + 2, width - 6, offset + 2);
    }
  }
}

void MenuItem::setAction(MenuActionType type, void *context, unsigned value) {
  this->_action.type = type;
  this->_action.context = context;
  this->_action.value = value;
}

void MenuItem::setActionStr(MenuActionType type, const char *str) {
  this->_action.type = type;
  this->_action.str = strdup(str);
}

Menu* MenuItem::CreateSubMenu() {
  if (!this->_submenu) {
    this->_submenu = Menus::CreateMenu();
  }
  return this->_submenu;
}

bool MenuItem::isSeparator() const {
  return _type == MENU_ITEM_SEPARATOR;
}

bool MenuItem::isNormal() const {
  return _type == MENU_ITEM_NORMAL;
}

bool MenuItem::isSubMenu() const {
  return _type == MENU_ITEM_SUBMENU;
}

const char* MenuItem::getTooltip() {
  return _tooltip;
}

Menu* MenuItem::getSubMenu() {
  return _submenu;
}

MenuItem::MenuAction* MenuItem::getAction() {
  return &_action;
}

int MenuItem::getHeight() const {
  if (isSeparator()) {
    return 6;
  }
  return Fonts::GetStringHeight(FONT_MENU);
}

int MenuItem::getWidth() const {
  if (_name) {
    return Fonts::GetStringWidth(FONT_MENU, _name);
  }
  return 6;
}

void MenuItem::removeTemporaryItems() {
  if (getSubMenu()) {
    getSubMenu()->removeTemporaryItems();
    int action = _action.type & MA_ACTION_MASK;
    if (action == MA_DESKTOP_MENU || action == MA_SENDTO_MENU
        || action == MA_WINDOW_MENU || action == MA_DYNAMIC) {
      delete getSubMenu();
    }
  }
}

Menu::Menu() :
    _x(0), _y(0), _timeout_ms(DEFAULT_TIMEOUT_MS), _mousex(-1), _mousey(-1), _label(
        0), _dynamic(0), _graphics(NULL), _window(0), _parentOffset(0), _parent(
    NULL), _textOffset(0), _shown(false) {

}

Menu::~Menu() {
  if (this->_label) {
    Release(this->_label);
  }
  if (this->_dynamic) {
    Release(this->_dynamic);
  }

  for (auto item : items) {
    delete item;
  }
}

bool Menu::execute(MenuItem::RunMenuCommandType runner) {
  XEvent event;
    MenuItem *ip;
    Window pressw;
    int pressx, pressy;
    char hadMotion;

    hadMotion = 0;

    Cursors::GetMousePosition(&pressx, &pressy, &pressw);

    Menu* menu = this;
    for (;;) {

      Events::_WaitForEvent(&event);

      switch (event.type) {
      case Expose:
        if (event.xexpose.count == 0) {
          Menu *mp = menu;
          while (mp) {
            if (mp->getWindow() == event.xexpose.window) {
              mp->Draw();
              break;
            }
            mp = mp->getParent();
          }
        }
        break;

      case ButtonPress:

        pressx = -100;
        pressy = -100;

      case KeyPress:
      case MotionNotify:
        hadMotion = 1;
        switch (Menus::UpdateMotion(menu, runner, &event)) {
        case MENU_NOSELECTION: /* no selection */
          break;
        case MENU_LEAVE: /* mouse left the menu */
          JXPutBackEvent(display, &event);
          return 0;
        case MENU_SUBSELECT: /* selection made */
          return 1;
        }
        break;

      case ButtonRelease:

        if (event.xbutton.button == Button4) {
          break;
        }
        if (event.xbutton.button == Button5) {
          break;
        }
        if (!hadMotion) {
          break;
        }
        if (abs(event.xbutton.x_root - pressx) < settings.doubleClickDelta) {
          if (abs(event.xbutton.y_root - pressy) < settings.doubleClickDelta) {
            break;
          }
        }

        ip = menu->getItemAt(event.xbutton.x, event.xbutton.y);

        if (ip != NULL) {
          if (ip->isNormal()) {
            menu->Hide();
            (runner)(ip->getAction(), event.xbutton.button);
          } else if (ip->isSubMenu()) {
            const Menu *parent = menu->getParent();
            if (event.xbutton.x >= menu->getX()
                && event.xbutton.x < menu->getX() + menu->getWidth()
                && event.xbutton.y >= menu->getY()
                && event.xbutton.y < menu->getY() + menu->getHeight()) {
              break;
            } else if (parent && event.xbutton.x >= parent->getX()
                && event.xbutton.x < parent->getX() + parent->getWidth()
                && event.xbutton.y >= parent->getY()
                && event.xbutton.y < parent->getY() + parent->getHeight()) {
              break;
            }
          }
        }
        return 1;
      default:
        break;
      }

    }
}

bool Menu::isShown() {
  return _shown;
}

void Menu::Hide() {
  Menu *mp = this;
  do {
    JXUnmapWindow(display, mp->getWindow());
    mp = mp->getParent();
  } while (mp);
}

bool Menu::Show(MenuItem::RunMenuCommandType runner, int x, int y) {
  /* Don't show the menu if there isn't anything to show. */
  if (!this->valid()) {
    /* Return 1 if there is an invalid menu.
     * This allows empty root menus to be defined to disable
     * scrolling on the root window.
     */
    return false;
  }
  if (JUNLIKELY(shouldExit)) {
    return false;
  }

  if (x < 0 && y < 0) {
    Window w;
    Cursors::GetMousePosition(&x, &y, &w);
    x -= this->getItemHeight() / 2;
    y -= this->getItemHeight() / 2 + this->getOffset(0);
  }

  if (!Cursors::GrabMouse(rootWindow)) {
    return 0;
  }
  if (JXGrabKeyboard(display, rootWindow, False, GrabModeAsync,
      GrabModeAsync, CurrentTime) != GrabSuccess) {
    JXUngrabPointer(display, CurrentTime);
    return 0;
  }

  Events::_RegisterCallback(settings.popupDelay, Menus::MenuCallback, this);
  //this->ShowSubmenu(runner, x, y);
  Events::_UnregisterCallback(Menus::MenuCallback, this);
  //Menus::UnpatchMenu(this);

  JXUngrabKeyboard(display, CurrentTime);
  JXUngrabPointer(display, CurrentTime);
  ClientNode::RefocusClient();

  if (shouldReload) {
    Roots::ReloadMenu();
  }
}

bool Menu::ShowSubMenu(MenuItem::RunMenuCommandType runner, int x, int y) {
  bool status;

  //this->PatchMenu(menu);
  //this->MapMenu(x, y);

  menuShown += 1;
  status = this->execute(runner);
  menuShown -= 1;

  JXDestroyWindow(display, this->getWindow());
  this->freeGraphics();

  return status;
}

bool Menu::valid() const {
  for (auto item : items) {
    if (!item->isSeparator()) {
      return true;
    }
  }
  return false;
}

void Menu::addItem(MenuItem *item) {
  if (item == NULL) {
    Log("WARNING: Attempt to add a bad MenuItem\n");
    return;
  }
  items.push_back(item);
}

void Menu::addSeparator() {
  addItem(new MenuItem(this, NULL, MENU_ITEM_SEPARATOR, NULL, NULL));
}

int Menu::getItemHeight() const {
  return Fonts::GetStringHeight(FONT_MENU);
}

Window Menu::getWindow() {
  return _window;
}

void Menu::Draw() {

  if (_graphics == NULL) {
    this->UpdatePosition(_x, _y);
  }

  int height = getHeight();
  int width = getWidth();
  JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_MENU_BG));
  _graphics->fillRectangle(0, 0, width, height);

  if (settings.menuDecorations == DECO_MOTIF) {
    JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_MENU_UP));
    _graphics->line(0, 0, width, 0);
    _graphics->line(0, 0, 0, height);

    JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_MENU_DOWN));
    _graphics->line(0, height - 1, width, height - 1);
    _graphics->line(width - 1, 0, width - 1, height);
  } else {
    JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_MENU_DOWN));
    _graphics->drawRectangle(0, 0, width - 1, height - 1);
  }

  if (_label) {
    _graphics->drawButton(BUTTON_LABEL, ALIGN_LEFT, FONT_MENU, _label, true,
        false, NULL,
        MENU_BORDER_SIZE, MENU_BORDER_SIZE, width - MENU_BORDER_SIZE * 2,
        getItemHeight() - 1, 0, 0);
  }

  int curY = 0;
  for (auto item : items) {
    item->Draw(_graphics, false, curY, width);
    curY += item->getHeight();
  }
  _graphics->copy(getWindow(), 0, 0, getWidth(), getHeight(), 0, 0);
}

void Menu::freeGraphics() {
  _graphics->free();
}

int Menu::getX() const {
  return _x;
}

int Menu::getY() const {
  return _y;
}

int Menu::getWidth() const {
  int width = 0;
  int submenu = 0;
  for (auto item : items) {
    width = std::max(width, item->getWidth());
  }
  width += submenu + _textOffset;
  width += 7 + 2 * MENU_BORDER_SIZE;
  return width;
}

int Menu::getHeight() const {
  int height = MENU_BORDER_SIZE;
  if (_label) {
    height += getItemHeight();
  }
  for (MenuItem *item : items) {
    height += item->getHeight();
  }
  return height;
}

TimeType Menu::getLastTime() {
  return _lastTime;
}

int Menu::getItemCount() {
  return items.size();
}

int Menu::getOffset(int index) {
  int cur = index;
  int offset = 0;
  while (cur >= 0) {
    offset += items[cur]->getHeight();
    --cur;
  }
  return offset;
}

Menu* Menu::getParent() {
  return _parent;
}

MenuItem* Menu::getItemAt(int x, int y) {
  if (x < this->getX() || x > this->getX() + this->getWidth()) {
    return NULL;
  }

  int curY = this->getY();
  for (auto item : items) {
    curY += item->getHeight();
    if (y < curY) {
      return item;
    }
  }

  return NULL;
}

int Menu::getMouseX() const {
  return _mousex;
}

int Menu::getMouseY() const {
  return _mousey;
}

void Menu::mouseEvent(int x, int y, TimeType now) {
  this->_mousex = x;
  this->_mousey = y;
  if (now.ms > 0) {
    this->_lastTime = now;
  }

//TODO: Change the selected component automatically
  vLog("Menu received mouse event [%d, %d]\n", x, y);
  Draw();
}

const ScreenType* Menu::getScreen() {
  if (getParent()) {
    return getParent()->getScreen();
  }
  return Screens::GetCurrentScreen(getX(), getY());
}

void Menu::UpdatePosition(int x, int y) {
  XSetWindowAttributes attr;
  unsigned long attrMask;

  const ScreenType *screen = getScreen();
  if (x + getWidth() > screen->x + screen->width) {
    if (getParent()) {
      x = getParent()->getX() - getWidth();
    } else {
      x = screen->x + screen->width - getWidth();
    }
  }
  if (y + getHeight() > screen->y + screen->height) {
    y = screen->y + screen->height - getHeight();
  }
  if (y < 0) {
    y = 0;
  }

  _x = x;
  _y = y;

  attrMask = 0;

  attrMask |= CWEventMask;
  attr.event_mask = ExposureMask;

  attrMask |= CWBackPixel;
  attr.background_pixel = Colors::lookupColor(COLOR_MENU_BG);

  attrMask |= CWSaveUnder;
  attr.save_under = True;

  _window = JXCreateWindow(display, rootWindow, x, y, getWidth(), getHeight(),
      0, CopyFromParent, InputOutput, CopyFromParent, attrMask, &attr);
  Hints::SetAtomAtom(getWindow(), ATOM_NET_WM_WINDOW_TYPE,
      ATOM_NET_WM_WINDOW_TYPE_MENU);

  _graphics = Graphics::create(display, rootGC, getWindow(), getWidth(),
      getHeight(), rootDepth);

  if (settings.menuOpacity < UINT_MAX) {
    Hints::SetCardinalAtom(getWindow(), ATOM_NET_WM_WINDOW_OPACITY,
        settings.menuOpacity);
  }

  JXMapRaised(display, getWindow());

}

void Menu::removeTemporaryItems() {
  for (auto item : items) {
    item->removeTemporaryItems();
  }
}

int Menu::getParentOffset() {
  return _parentOffset;
}
