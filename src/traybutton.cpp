/**
 * @file traybutton.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Button tray component.
 *
 */

#include "jwm.h"
#include "traybutton.h"
#include "tray.h"
#include "icon.h"
#include "image.h"
#include "error.h"
#include "root.h"
#include "main.h"
#include "font.h"
#include "button.h"
#include "misc.h"
#include "screen.h"
#include "popup.h"
#include "timing.h"
#include "command.h"
#include "cursor.h"
#include "settings.h"
#include "event.h"
#include "action.h"

std::vector<TrayButton*> TrayButton::buttons;

/** Startup tray buttons. */
void TrayButton::StartupTrayButtons(void) {
  std::vector<TrayButton*>::iterator it;
  for (it = buttons.begin(); it != buttons.end(); ++it) {
    TrayButton *bp = (*it);
    if (bp->label) {
      bp->requestNewSize(Fonts::GetStringWidth(FONT_TRAY, bp->label) + 4,
          Fonts::GetStringHeight(FONT_TRAY));
    } else {
      bp->requestNewSize(0, 0);
    }
    if (bp->iconName) {
      bp->icon = Icon::LoadNamedIcon(bp->iconName, 1, 1);
      if (JLIKELY(bp->icon)) {
        int rWidth = 0, rHeight = 0;
        rWidth = bp->getRequestedWidth() + bp->icon->getWidth() + 4;
        if (bp->label) {
          rWidth = rWidth - 2;
        }
        rHeight = Max(bp->icon->getHeight() + 4, bp->getRequestedHeight());
        bp->requestNewSize(rWidth, rHeight);
      } else {
        Warning(_("could not load tray icon: \"%s\""), bp->iconName);
      }
    }
  }
}

TrayButton* TrayButton::Create(const char *iconName, const char *label,
    const char *popup, unsigned int width, unsigned int height, Tray *tray,
    TrayComponent *parent) {
  TrayButton *button = new TrayButton(iconName, label, popup, width, height,
      tray, parent);
  buttons.push_back(button);
  return button;
}

/** Release tray button data. */
void TrayButton::DestroyTrayButtons(void) {
  TrayButton *bp;
  std::vector<TrayButton*>::iterator it = buttons.begin();
  while (!buttons.empty()) {
    it = buttons.begin();
    bp = (*it);
    buttons.erase(it);
  }
  buttons.clear();
}

/** Create a button tray component. */
TrayButton::TrayButton(const char *iconName, const char *label,
    const char *popup, unsigned int width, unsigned int height, Tray *tray,
    TrayComponent *parent) :
    TrayComponent(tray, parent) {

  this->icon = NULL;
  this->iconName = CopyString(iconName);
  this->label = CopyString(label);
  this->popup = CopyString(popup);

  //TODO: bp is a subclass of TrayComponentType cp->getObject() = bp;
  this->requestNewSize(width, height);

  this->mousex = -settings.doubleClickDelta;
  this->mousey = -settings.doubleClickDelta;

  Events::_RegisterCallback(settings.popupDelay / 2, SignalTrayButton, this);

  this->Create();
}

TrayButton::~TrayButton() {
  Events::_UnregisterCallback(SignalTrayButton, this);
  if (this->label) {
    delete[] (this->label);
  }
  if (this->iconName) {
    delete[] (this->iconName);
  }
  if (this->popup) {
    Release(this->popup);
  }
}

/** Set the size of a button tray component. */
void TrayButton::SetSize(int width, int height) {

  this->width = 10;
  this->height = getTray()->getHeight();

  this->Destroy();
  this->Create();
}

/** Resize a button tray component. */
void TrayButton::Resize() {
  TrayComponent::Resize();
}

void TrayButton::Create() {
  this->setPixmap(
      JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(),
          rootDepth));
}

/** Destroy a button tray component. */
void TrayButton::Destroy() {
  if (this->getPixmap() != None) {
    JXFreePixmap(display, this->getPixmap());
    this->setPixmap(None);
  }
}

void TrayButton::Draw(Graphics *g) {

}

void TrayButton::UpdateSpecificTray(const Tray *tp) {
  TrayComponent::UpdateSpecificTray(tp);
}

/** Draw a tray button. */
void TrayButton::Draw() {
  Tray::ClearTrayDrawable(this);

  DrawButton(this->wasGrabbed() ? BUTTON_TRAY_ACTIVE : BUTTON_TRAY,
  ALIGN_CENTER, FONT_TASKLIST, this->label, true,
      settings.trayDecorations == DECO_FLAT, getPixmap(), this->icon, getX(),
      getY(), getWidth(), getHeight(), 0, 0);

}

/** Process a motion event. */
void TrayButton::ProcessMotionEvent(int x, int y, int mask) {
  this->mousex = this->getScreenX() + x;
  this->mousey = this->getScreenY() + y;
  GetCurrentTime(&this->mouseTime);
}

/** Signal (needed for popups). */
void TrayButton::SignalTrayButton(const TimeType *now, int x, int y, Window w,
    void *data) {
  TrayButton *bp = (TrayButton*) data;
  const char *popup;

  if (bp->popup) {
    popup = bp->popup;
  } else if (bp->label) {
    popup = bp->label;
  } else {
    return;
  }
  if (bp->getTray()->getWindow() == w
      && abs(bp->mousex - x) < settings.doubleClickDelta
      && abs(bp->mousey - y) < settings.doubleClickDelta) {
    if (GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
      Popups::ShowPopup(x, y, popup, POPUP_BUTTON);
    }
  }
}
