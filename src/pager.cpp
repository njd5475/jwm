/**
 * @file pager.c
 * @author Joe Wingbermuehle
 * @date 2004-2007
 *
 * @brief Pager tray component.
 *
 */

#include "jwm.h"
#include "pager.h"

#include "client.h"
#include "clientlist.h"
#include "color.h"
#include "cursor.h"
#include "event.h"
#include "tray.h"
#include "timing.h"
#include "popup.h"
#include "font.h"
#include "settings.h"
#include "DesktopEnvironment.h"

std::vector<PagerType*> PagerType::pagers;

/** Shutdown the pager. */
void PagerType::ShutdownPager(void) {
  for (auto pp : pagers) {
    JXFreePixmap(display, pp->buffer);
  }
}

/** Release pager data. */
void PagerType::DestroyPager(void) {
  pagers.clear();
}

/** Create a new pager tray component. */
PagerType::PagerType(char labeled, Tray *tray, TrayComponent *parent) :
    TrayComponent(tray, parent) {
  this->labeled = labeled;
  this->mousex = -settings.doubleClickDelta;
  this->mousey = -settings.doubleClickDelta;
  this->mouseTime.seconds = 0;
  this->mouseTime.ms = 0;
  this->buffer = None;

  Events::_RegisterCallback(settings.popupDelay / 2, SignalPager, this);
}

PagerType::~PagerType() {
  Logger::Log("Destroying PagerType\n");
  this->getTray()->RemoveTrayComponent(this);
  Events::_UnregisterCallback(SignalPager, this);
}

/** Initialize a pager tray component. */
void PagerType::Create() {

  Assert(this->getWidth() > 0);
  Assert(this->getHeight() > 0);

  this->pixmap = JXCreatePixmap(display, rootWindow, this->getWidth(),
      this->getHeight(), rootDepth);
  this->buffer = this->pixmap;

}

/** Set the size of a pager tray component. */
void PagerType::SetSize(int width, int height) {
  width = this->getTray()->getWidth() * 0.15; // percentage of tray width
  height = this->getTray()->getHeight(); // full height of tray

  TrayComponent::SetSize(width, height);

  if (width) {

    /* Vertical pager. */
    this->width = width;

    this->deskWidth = width / settings.desktopWidth;
    this->deskHeight = (this->deskWidth * rootHeight) / rootWidth;

    this->height = this->deskHeight * settings.desktopHeight
        + settings.desktopHeight - 1;

  } else if (height) {

    /* Horizontal pager. */
    this->height = height;

    this->deskHeight = height / settings.desktopHeight;
    this->deskWidth = (this->deskHeight * rootWidth) / rootHeight;

    this->width = this->deskWidth * settings.desktopWidth
        + settings.desktopWidth - 1;

  } else {
    Assert(0);
  }

  if (this->buffer != None) {
    JXFreePixmap(display, this->buffer);
    this->buffer = JXCreatePixmap(display, rootWindow, this->getWidth(),
        this->getHeight(), rootDepth);
    this->pixmap = this->buffer;
    this->Draw();
  }

  this->scalex = ((this->deskWidth - 2) << 16) / rootWidth;
  this->scaley = ((this->deskHeight - 2) << 16) / rootHeight;

}

/** Get the desktop for a pager given a set of coordinates. */
int PagerType::GetPagerDesktop(int x, int y) {

  int pagerx, pagery;

  pagerx = x / (this->deskWidth + 1);
  pagery = y / (this->deskHeight + 1);

  return pagery * settings.desktopWidth + pagerx;

}

void PagerType::ProcessButtonRelease(int x, int y, int mask) {

}

/** Process a button event on a pager tray component. */
void PagerType::ProcessButtonPress(int x, int y, int mask) {

  switch (mask) {
  case Button1:
  case Button2:

    /* Change to the selected desktop. */
    DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(
        this->GetPagerDesktop(x, y));
    break;

  case Button3:

    /* Move a client and possibly change its desktop. */
    this->StartPagerMove(x, y);
    break;

  case Button4:

    /* Change to the previous desktop. */
    DesktopEnvironment::DefaultEnvironment()->LeftDesktop();
    break;

  case Button5:

    /* Change to the next desktop. */
    DesktopEnvironment::DefaultEnvironment()->RightDesktop();
    break;

  default:
    break;
  }
}

/** Process a motion event on a pager tray component. */
void PagerType::ProcessMotionEvent(int x, int y, int mask) {
  this->mousex = this->getScreenX() + x;
  this->mousey = this->getScreenY() + y;
  GetCurrentTime(&this->mouseTime);
}

/** Start a pager move operation. */
void PagerType::StartPagerMove(int x, int y) {

  XEvent event;
  PagerType *pp;
  ClientNode *np;
  int layer;
  int desktop;
  int cx, cy;
  int cwidth, cheight;

  int north, south, east, west;
  int oldx, oldy;
  int oldDesk;
  int startx, starty;
  MaxFlags maxFlags;

  pp = (PagerType*) this;

  /* Determine the selected desktop. */
  desktop = pp->GetPagerDesktop(x, y);
  x -= (desktop % settings.desktopWidth) * (pp->deskWidth + 1);
  y -= (desktop / settings.desktopWidth) * (pp->deskHeight + 1);

  //TODO: Special code and needs lots of TLC
  /* Find the client under the specified coordinates. */
//  for (layer = LAST_LAYER; layer >= FIRST_LAYER; layer--) {
//    for (np = nodes[layer]; np; np = np->getNext()) {
  std::vector<ClientNode*> all = ClientList::GetList();
  for (int i = 0; i < all.size(); ++i) {
    np = all[i];
    /* Skip this client if it isn't mapped. */
    if (!(np->isMapped())) {
      continue;
    }
    //TODO: Pager should have an ignore list.
    if (np->shouldNotShowInPager()) {
      continue;
    }

    /* Skip this client if it isn't on the selected desktop. */
    if (np->isSticky()) {
      if (currentDesktop != desktop) {
        continue;
      }
    } else {
      if (np->getDesktop() != desktop) {
        continue;
      }
    }

    /* Get the offset and size of the client on the pager. */
    cx = 1 + ((np->getX() * pp->scalex) >> 16);
    cy = 1 + ((np->getY() * pp->scaley) >> 16);
    cwidth = (np->getWidth() * pp->scalex) >> 16;
    cheight = (np->getHeight() * pp->scaley) >> 16;

    /* Normalize the offset and size. */
    if (cx + cwidth > pp->deskWidth) {
      cwidth = pp->deskWidth - cx;
    }
    if (cy + cheight > pp->deskHeight) {
      cheight = pp->deskHeight - cy;
    }
    if (cx < 0) {
      cwidth += cx;
      cx = 0;
    }
    if (cy < 0) {
      cheight += cy;
      cy = 0;
    }

    /* Skip the client if we are no longer in bounds. */
    if (cwidth <= 0 || cheight <= 0) {
      continue;
    }

    /* Check the y-coordinate. */
    if (y < cy || y > cy + cheight) {
      continue;
    }

    /* Check the x-coordinate. */
    if (x < cx || x > cx + cwidth) {
      continue;
    }

    /* Found it. Exit. */
    goto ClientFound;

  }

  /* Client wasn't found. Just return. */
  return;

  ClientFound:

  Assert(np);

  /* The selected client was found. Now make sure we can move it. */
  if (!(np->getBorder() & BORDER_MOVE)) {
    return;
  }

  /* Start the move. */
  if (!Cursors::GrabMouseForMove()) {
    return;
  }

  /* If the client is maximized, unmaximize it. */
  maxFlags = np->getMaxFlags();
  if (np->getMaxFlags()) {
    np->MaximizeClient(MAX_NONE);
  }

  Border::GetBorderSize(np, &north, &south, &east, &west);

  np->setController(PagerType::PagerMoveController);
  shouldStopMove = 0;

  oldx = np->getX();
  oldy = np->getY();
  oldDesk = np->getDesktop();

  startx = x;
  starty = y;

  if (!(Cursors::GetMouseMask() & Button3Mask)) {
    np->StopPagerMove(oldx, oldy, oldDesk, maxFlags);
  }

  for (;;) {

    Events::_WaitForEvent(&event);

    if (shouldStopMove) {
      np->clearController();
      return;
    }

    switch (event.type) {
    case ButtonRelease:

      /* Done when the 3rd mouse button is released. */
      if (event.xbutton.button == Button3) {
        np->StopPagerMove(oldx, oldy, oldDesk, maxFlags);
        return;
      }
      break;

    case MotionNotify:

      Cursors::SetMousePosition(event.xmotion.x_root, event.xmotion.y_root,
          event.xmotion.window);

      /* Get the mouse position on the pager. */
      x = event.xmotion.x_root - this->getScreenX();
      y = event.xmotion.y_root - this->getScreenY();

      /* Don't move if we are off of the pager. */
      if (x < 0 || x > this->getWidth()) {
        break;
      }
      if (y < 0 || y > this->getHeight()) {
        break;
      }

      /* Determine the new client desktop. */
      desktop = pp->GetPagerDesktop(x, y);
      x -= pp->deskWidth * (desktop % settings.desktopWidth);
      y -= pp->deskHeight * (desktop / settings.desktopWidth);

      /* If this client isn't sticky and now on a different desktop
       * change the client's desktop. */
      if (!(np->isSticky())) {
        if (desktop != oldDesk) {
          np->SetClientDesktop((unsigned int) desktop);
          oldDesk = desktop;
        }
      }

      /* Get new client coordinates. */
      oldx = startx + (x - startx);
      oldx = (oldx << 16) / pp->scalex;
      oldx -= (np->getWidth() + east + west) / 2;
      oldy = starty + (y - starty);
      oldy = (oldy << 16) / pp->scaley;
      oldy -= (np->getHeight() + north + south) / 2;

      /* Move the window. */
      np->setX(oldx);
      np->setY(oldy);
      JXMoveWindow(display, np->getParent(), np->getX() - west,
          np->getY() - north);
      np->SendConfigureEvent();
      Events::_RequirePagerUpdate();

      break;

    default:
      break;
    }

  }

}

char PagerType::shouldStopMove = 0;

/** Client-terminated pager move. */
void PagerType::PagerMoveController(int wasDestroyed) {

  JXUngrabPointer(display, CurrentTime);
  JXUngrabKeyboard(display, CurrentTime);
  shouldStopMove = 1;

}

/** Draw a pager. */
void PagerType::Draw() {
  Pixmap buffer;
  int width, height;
  int deskWidth, deskHeight;
  unsigned int x;
  const char *name;
  int xc, yc;
  int textWidth, textHeight;
  int dx, dy;

  buffer = this->getPixmap();
  width = this->getWidth();
  height = this->getHeight();
  deskWidth = this->deskWidth;
  deskHeight = this->deskHeight;

  /* Draw the background. */
  JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_PAGER_BG));
  JXFillRectangle(display, buffer, rootGC, 0, 0, width, height);

  /* Highlight the current desktop. */
  JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_PAGER_ACTIVE_BG));
  dx = currentDesktop % settings.desktopWidth;
  dy = currentDesktop / settings.desktopWidth;
  JXFillRectangle(display, buffer, rootGC, dx * (deskWidth + 1),
      dy * (deskHeight + 1), deskWidth, deskHeight);

  /* Draw the labels. */
  if (this->labeled) {
    textHeight = Fonts::GetStringHeight(FONT_PAGER);
    if (textHeight < deskHeight) {
      for (x = 0; x < settings.desktopCount; x++) {
        dx = x % settings.desktopWidth;
        dy = x / settings.desktopWidth;
        name = DesktopEnvironment::DefaultEnvironment()->GetDesktopName(x);
        textWidth = Fonts::GetStringWidth(FONT_PAGER, name);
        if (textWidth < deskWidth) {
          xc = dx * (deskWidth + 1) + (deskWidth - textWidth) / 2;
          yc = dy * (deskHeight + 1) + (deskHeight - textHeight) / 2;
          Fonts::RenderString(buffer, FONT_PAGER, COLOR_PAGER_TEXT, xc, yc,
              deskWidth, name);
        }
      }
    }
  }

  /* Draw the clients. */
  std::vector<ClientNode*> all = ClientList::GetList();
  for (int i = 0; i < all.size(); ++i) {
    this->DrawPagerClient(all[i]);
  }

  /* Draw the desktop dividers. */
  JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_PAGER_OUTLINE));
  for (x = 1; x < settings.desktopHeight; x++) {
    JXDrawLine(display, buffer, rootGC, 0, (deskHeight + 1) * x - 1, width,
        (deskHeight + 1) * x - 1);
  }
  for (x = 1; x < settings.desktopWidth; x++) {
    JXDrawLine(display, buffer, rootGC, (deskWidth + 1) * x - 1, 0,
        (deskWidth + 1) * x - 1, height);
  }

}

void PagerType::Draw(Graphics *g) {

}

/** Update the pager. */
void PagerType::UpdatePager(void) {

  PagerType *pp;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  for(auto pp : pagers) {

    /* Draw the pager. */
    pp->Draw();

    /* Tell the tray to redraw. */
    pp->UpdateSpecificTray(pp->getTray());

  }

}

/** Signal pagers (for popups). */
void PagerType::SignalPager(const TimeType *now, int x, int y, Window w,
    void *data) {
  PagerType *pp = (PagerType*) data;
  if (pp->getTray()->getWindow() == w
      && abs(pp->mousex - x) < settings.doubleClickDelta
      && abs(pp->mousey - y) < settings.doubleClickDelta) {
    if (GetTimeDifference(now, &pp->mouseTime) >= settings.popupDelay) {
      const int desktop = pp->GetPagerDesktop(x - pp->getScreenX(),
          y - pp->getScreenY());
      if (desktop >= 0 && desktop < settings.desktopCount) {
        const char *desktopName =
            DesktopEnvironment::DefaultEnvironment()->GetDesktopName(desktop);
        if (desktopName) {
          Popups::ShowPopup(x, y, desktopName, POPUP_PAGER);
        }
      }

    }
  }
}

/** Draw a client on the pager. */
void PagerType::DrawPagerClient(ClientNode *np) {

  int x, y;
  int width, height;
  int offx, offy;

  /* Don't draw the client if it isn't mapped. */
  if (!(np->isMapped())) {
    return;
  }
  if (np->shouldNotShowInPager()) {
    return;
  }

  /* Determine the desktop for the client. */
  if (np->isSticky()) {
    offx = currentDesktop % settings.desktopWidth;
    offy = currentDesktop / settings.desktopWidth;
  } else {
    offx = np->getDesktop() % settings.desktopWidth;
    offy = np->getDesktop() / settings.desktopWidth;
  }
  offx *= this->deskWidth + 1;
  offy *= this->deskHeight + 1;

  /* Determine the location and size of the client on the pager. */
  x = 1 + ((np->getX() * this->scalex) >> 16);
  y = 1 + ((np->getY() * this->scaley) >> 16);
  width = (np->getWidth() * this->scalex) >> 16;
  height = (np->getHeight() * this->scaley) >> 16;

  /* Normalize the size and offset. */
  if (x + width > this->deskWidth) {
    width = this->deskWidth - x;
  }
  if (y + height > this->deskHeight) {
    height = this->deskHeight - y;
  }
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }

  /* Return if there's nothing to do. */
  if (width <= 0 || height <= 0) {
    return;
  }

  /* Move to the correct desktop on the pager. */
  x += offx;
  y += offy;

  /* Draw the client outline. */
  JXSetForeground(display, rootGC, Colors::lookupColor(COLOR_PAGER_OUTLINE));
  JXDrawRectangle(display, this->getPixmap(), rootGC, x, y, width, height);

  /* Fill the client if there's room. */
  if (width > 1 && height > 1) {
    ColorName fillColor;
    if ((np->isActive())
        && (np->getDesktop() == currentDesktop
            || (np->isSticky()))) {
      fillColor = COLOR_PAGER_ACTIVE_FG;
    } else if (np->shouldFlash()) {
      fillColor = COLOR_PAGER_ACTIVE_FG;
    } else {
      fillColor = COLOR_PAGER_FG;
    }
    JXSetForeground(display, rootGC, Colors::lookupColor(fillColor));
    JXFillRectangle(display, this->getPixmap(), rootGC, x + 1, y + 1, width - 1,
        height - 1);
  }

}


TrayComponent *PagerType::CreatePager(char labeled, Tray * tray, TrayComponent *parent) {
  PagerType *pager = new PagerType(labeled, tray, parent);
  pagers.push_back(pager);
  return pager;
}
