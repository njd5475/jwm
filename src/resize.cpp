/**
 * @file resize.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle resizing client windows.
 *
 */

#include "jwm.h"
#include "resize.h"
#include "client.h"
#include "outline.h"
#include "cursor.h"
#include "misc.h"
#include "status.h"
#include "event.h"
#include "settings.h"

char shouldStopResize = 0;

/** Callback to stop a resize. */
void ResizeController(int wasDestroyed) {
  if (settings.resizeMode == RESIZE_OUTLINE) {
    Outline::ClearOutline();
  }
  JXUngrabPointer(display, CurrentTime);
  JXUngrabKeyboard(display, CurrentTime);
  DestroyResizeWindow();
  shouldStopResize = 1;
}
