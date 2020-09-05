/**
 * @file button.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for rendering buttons.
 *
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "font.h"
#include "settings.h"
#include "Component.h"
#include "HashMap.h"

class Graphics;
class Icon;

/** Button types. */
typedef unsigned char ButtonType;
#define BUTTON_LABEL       0  /**< Label. */
#define BUTTON_MENU        1  /**< Menu item. */
#define BUTTON_MENU_ACTIVE 2  /**< Active menu item. */
#define BUTTON_TRAY        3  /**< Inactive tray button. */
#define BUTTON_TRAY_ACTIVE 4  /**< Active tray button. */
#define BUTTON_TASK        5  /**< Item in the task list. */
#define BUTTON_TASK_ACTIVE 6  /**< Active item in the task list. */

/** Draw a button.
 * @param bp The button to draw.
 */
void DrawButton(ButtonType type, AlignmentType alignment, FontType font, const char *text, bool fill,
		bool border, Drawable drawable, Icon *icon, int x, int y, int width, int height,
		int xoffset, int yoffset);

class Button : public Component {

public:
  Button(Component *parent, const char* text, int x, int y, int width, int height, void (*action)());
  virtual ~Button();

  void Draw(Graphics *g);

  virtual int getX();
  virtual int getY();
  virtual int getWidth();
  virtual int getHeight();
  bool isActive();
  bool contains(int x, int y);
  void mouseMoved(int mouseX, int mouseY);
  void mouseReleased();
  virtual bool process(const XEvent *event);
  virtual int getIntProp(const char* propName);
  virtual const char* getStringProp(const char* propName);
  virtual void initProperties(HashMap<ComponentProperty*>* properties);
private:
  int _x,_y,_width,_height;
  bool _active;
  const char* _text;
  void (*_action)();
};

#endif /* BUTTON_H */
