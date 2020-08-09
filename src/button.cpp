/**
 * @file button.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for rendering buttons.
 *
 */

#include "jwm.h"
#include "button.h"
#include "border.h"
#include "main.h"
#include "icon.h"
#include "image.h"
#include "misc.h"
#include "Graphics.h"
#include "settings.h"

void DrawButtonStyle(long fg, long bg1, long bg2, long up, long down,
    DecorationsType decorations, AlignmentType alignment, FontType font,
    const char *text, bool fill, bool border, Drawable drawable, Icon *icon,
    int x, int y, int width, int height, int xoffset, int yoffset);

/** Draw a button. */
void DrawButton(ButtonType type, AlignmentType alignment, FontType font,
    const char *text, bool fill, bool border, Drawable drawable, Icon *icon,
    int x, int y, int width, int height, int xoffset, int yoffset) {
  ColorName fg;
  long bg1, bg2;
  long up, down;
  DecorationsType decorations;

  /* Determine the colors to use. */
  switch (type) {
  case BUTTON_LABEL:
    fg = COLOR_MENU_FG;
    bg1 = Colors::lookupColor(COLOR_MENU_BG);
    bg2 = Colors::lookupColor(COLOR_MENU_BG);
    up = Colors::lookupColor(COLOR_MENU_UP);
    down = Colors::lookupColor(COLOR_MENU_DOWN);
    decorations = settings.menuDecorations;
    break;
  case BUTTON_MENU_ACTIVE:
    fg = COLOR_MENU_ACTIVE_FG;
    bg1 = Colors::lookupColor(COLOR_MENU_ACTIVE_BG1);
    bg2 = Colors::lookupColor(COLOR_MENU_ACTIVE_BG2);
    down = Colors::lookupColor(COLOR_MENU_ACTIVE_UP);
    up = Colors::lookupColor(COLOR_MENU_ACTIVE_DOWN);
    decorations = settings.menuDecorations;
    break;
  case BUTTON_TRAY:
    fg = COLOR_TRAYBUTTON_FG;
    bg1 = Colors::lookupColor(COLOR_TRAYBUTTON_BG1);
    bg2 = Colors::lookupColor(COLOR_TRAYBUTTON_BG2);
    up = Colors::lookupColor(COLOR_TRAYBUTTON_UP);
    down = Colors::lookupColor(COLOR_TRAYBUTTON_DOWN);
    decorations = settings.trayDecorations;
    break;
  case BUTTON_TRAY_ACTIVE:
    fg = COLOR_TRAYBUTTON_ACTIVE_FG;
    bg1 = Colors::lookupColor(COLOR_TRAYBUTTON_ACTIVE_BG1);
    bg2 = Colors::lookupColor(COLOR_TRAYBUTTON_ACTIVE_BG2);
    down = Colors::lookupColor(COLOR_TRAYBUTTON_ACTIVE_UP);
    up = Colors::lookupColor(COLOR_TRAYBUTTON_ACTIVE_DOWN);
    decorations = settings.trayDecorations;
    break;
  case BUTTON_TASK:
    fg = COLOR_TASKLIST_FG;
    bg1 = Colors::lookupColor(COLOR_TASKLIST_BG1);
    bg2 = Colors::lookupColor(COLOR_TASKLIST_BG2);
    up = Colors::lookupColor(COLOR_TASKLIST_UP);
    down = Colors::lookupColor(COLOR_TASKLIST_DOWN);
    decorations = settings.taskListDecorations;
    break;
  case BUTTON_TASK_ACTIVE:
    fg = COLOR_TASKLIST_ACTIVE_FG;
    bg1 = Colors::lookupColor(COLOR_TASKLIST_ACTIVE_BG1);
    bg2 = Colors::lookupColor(COLOR_TASKLIST_ACTIVE_BG2);
    down = Colors::lookupColor(COLOR_TASKLIST_ACTIVE_UP);
    up = Colors::lookupColor(COLOR_TASKLIST_ACTIVE_DOWN);
    decorations = settings.taskListDecorations;
    break;
  case BUTTON_MENU:
  default:
    fg = COLOR_MENU_FG;
    bg1 = Colors::lookupColor(COLOR_MENU_BG);
    bg2 = Colors::lookupColor(COLOR_MENU_BG);
    up = Colors::lookupColor(COLOR_MENU_UP);
    down = Colors::lookupColor(COLOR_MENU_DOWN);
    decorations = settings.menuDecorations;
    break;
  }

  DrawButtonStyle(fg, bg1, bg2, up, down, decorations, alignment, font, text,
      fill, border, drawable, icon, x, y, width, height, xoffset, yoffset);
}

/** Draw a button. */
void DrawButtonStyle(long fg, long bg1, long bg2, long up, long down,
    DecorationsType decorations, AlignmentType alignment, FontType font,
    const char *text, bool fill, bool border, Drawable drawable, Icon *icon,
    int x, int y, int width, int height, int xoffset, int yoffset) {
  GC gc;

  int iconWidth, iconHeight;
  int textWidth, textHeight;

  gc = JXCreateGC(display, drawable, 0, NULL);

  /* Draw the background. */
  if (fill) {

    /* Draw the button background. */
    JXSetForeground(display, gc, bg1);
    if (bg1 == bg2) {
      /* single color */
      JXFillRectangle(display, drawable, gc, x, y, width, height);
    } else {
      /* gradient */
      DrawHorizontalGradient(drawable, gc, bg1, bg2, x, y, width, height);
    }

  }

  /* Draw the border. */
  if (border) {
    if (decorations == DECO_MOTIF) {
      JXSetForeground(display, gc, up);
      JXDrawLine(display, drawable, gc, x, y, x + width - 1, y);
      JXDrawLine(display, drawable, gc, x, y, x, y + height - 1);
      JXSetForeground(display, gc, down);
      JXDrawLine(display, drawable, gc, x, y + height - 1, x + width - 1,
          y + height - 1);
      JXDrawLine(display, drawable, gc, x + width - 1, y, x + width - 1,
          y + height - 1);
    } else {
      JXSetForeground(display, gc, down);
      JXDrawRectangle(display, drawable, gc, x, y, width - 1, height - 1);
    }
  }

  /* Determine the size of the icon (if any) to display. */
  iconWidth = 0;
  iconHeight = 0;
  if (icon) {
    if (icon == &Icon::emptyIcon) {
      iconWidth = Max(Min(width - 4, height - 4), 16);
      iconHeight = iconWidth;
    } else {
      const int ratio = (icon->getWidth() << 16) / Max(icon->getHeight(), 16);
      int maxIconWidth = width - 4;
      if (text) {
        /* Showing text, keep the icon square. */
        maxIconWidth = Min(width, height) - 4;
      }
      iconHeight = height - 4;
      iconWidth = (iconHeight * ratio) >> 16;
      if (iconWidth > maxIconWidth) {
        iconWidth = maxIconWidth;
        iconHeight = (iconWidth << 16) / ratio;
      }
    }
  }

  /* Determine how much room is left for text. */
  textWidth = 0;
  textHeight = 0;
  if (text && (width > height || !icon)) {
    textWidth = Fonts::GetStringWidth(font, text);
    textHeight = Fonts::GetStringHeight(font);
    if (iconWidth > 0 && textWidth + iconWidth + 7 > width) {
      textWidth = width - iconWidth - 7;
    } else if (iconWidth == 0 && textWidth + 5 > width) {
      textWidth = width - 5;
    }
    textWidth = textWidth < 0 ? 0 : textWidth;
  }

  /* Determine the offset of the text in the button. */
  if (alignment == ALIGN_CENTER || width <= height) {
    xoffset = (width - iconWidth - textWidth + 1) / 2;
    if (xoffset < 0) {
      xoffset = 0;
    }
  } else {
    xoffset = 2;
  }

  /* Display the icon. */
  if (icon) {
    yoffset = (height - iconHeight + 1) / 2;
    ScaledIconNode::PutIcon(icon, drawable, Colors::lookupColor(fg),
        x + xoffset, y + yoffset, iconWidth, iconHeight);
    xoffset += iconWidth + 2;
  }

  /* Display the label. */
  if (textWidth > 0) {
    yoffset = (height - textHeight + 1) / 2;
    Fonts::RenderString(drawable, font, fg, x + xoffset, y + yoffset, textWidth,
        text);
  }

  JXFreeGC(display, gc);

}

Button::Button(const char *text, int x, int y, int width, int height, void (*action)()) :
    _x(x), _y(y), _width(width), _height(height), _active(false), _action(action) {
  Assert(text);
  _text = CopyString(text);
}

Button::~Button() {

}

void Button::Draw(Graphics *g) {
  g->drawButton(this->isActive() ? BUTTON_MENU_ACTIVE : BUTTON_MENU,
  ALIGN_CENTER, FONT_MENU, this->_text, true, true, NULL, this->getX(),
      this->getY(), this->getWidth(), this->getHeight(), 0, 0);
}

int Button::getX() {
  return _x;
}

int Button::getY() {
  return _y;
}

int Button::getWidth() {
  return _width;
}

int Button::getHeight() {
  return _height;
}

bool Button::isActive() {
  return _active;
}

bool Button::contains(int mouseX, int mouseY) {
  if (mouseX >= this->getX() && mouseX <= (this->getX() + this->getWidth())
      && mouseY >= this->getY()
      && mouseY <= (this->getY() + this->getHeight())) {
    _active = true;
  } else {
    _active = false;
  }
//  printf("Mouse %d, %d test on %d,%d %dx%d (%s)\n", mouseX, mouseY,
//      this->getX(), this->getY(), this->getX() + this->getWidth(),
//      this->getY() + this->getHeight(), _active ? "Inside" : "Outside");
  return _active;
}

void Button::mouseMoved(int mouseX, int mouseY) {
  //TODO: Refactor this please.

}

void Button::mouseReleased() {
  _action();
}
