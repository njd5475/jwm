/**
 * @file place.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client placement functions.
 *
 */

#include "nwm.h"
#include "place.h"
#include "client.h"
#include "border.h"
#include "screen.h"
#include "settings.h"
#include "clientlist.h"
#include "misc.h"

/** Startup placement. */
void Places::StartupPlacement(void) {

  SetWorkarea();
}

/** Shutdown placement. */
void Places::ShutdownPlacement(void) {


}

/** Remove struts associated with a client. */
void Places::RemoveClientStrut(ClientNode *np) {
  if (DoRemoveClientStrut(np)) {
    SetWorkarea();
  }
}

/** Add client specified struts to our list. */
void Places::ReadClientStrut(ClientNode *np) {

  BoundingBox box;
  int status;
  Atom actualType;
  int actualFormat;
  unsigned long count;
  unsigned long bytesLeft;
  unsigned char *value;
  long *lvalue;
  long leftWidth, rightWidth, topHeight, bottomHeight;
  char updated;

  updated = DoRemoveClientStrut(np);

  box.x = 0;
  box.y = 0;
  box.width = 0;
  box.height = 0;

  /* First try to read _NET_WM_STRUT_PARTIAL */
  /* Format is:
   *   left_width, right_width, top_width, bottom_width,
   *   left_start_y, left_end_y, right_start_y, right_end_y,
   *   top_start_x, top_end_x, bottom_start_x, bottom_end_x
   */
  count = 0;
  status = JXGetWindowProperty(display, np->getWindow(), Hints::atoms[ATOM_NET_WM_STRUT_PARTIAL], 0, 12, False, XA_CARDINAL,
      &actualType, &actualFormat, &count, &bytesLeft, &value);
  if (status == Success && actualFormat != 0) {
    if (JLIKELY(count == 12)) {

      long leftStart, leftStop;
      long rightStart, rightStop;
      long topStart, topStop;
      long bottomStart, bottomStop;

      lvalue = (long*) value;
      leftWidth = lvalue[0];
      rightWidth = lvalue[1];
      topHeight = lvalue[2];
      bottomHeight = lvalue[3];
      leftStart = lvalue[4];
      leftStop = lvalue[5];
      rightStart = lvalue[6];
      rightStop = lvalue[7];
      topStart = lvalue[8];
      topStop = lvalue[9];
      bottomStart = lvalue[10];
      bottomStop = lvalue[11];

      if (leftWidth > 0) {
        box.x = 0;
        box.y = leftStart;
        box.height = leftStop - leftStart;
        box.width = leftWidth;
        InsertStrut(&box, np);
      }

      if (rightWidth > 0) {
        box.x = rootWidth - rightWidth;
        box.y = rightStart;
        box.height = rightStop - rightStart;
        box.width = rightWidth;
        InsertStrut(&box, np);
      }

      if (topHeight > 0) {
        box.x = topStart;
        box.y = 0;
        box.height = topHeight;
        box.width = topStop - topStart;
        InsertStrut(&box, np);
      }

      if (bottomHeight > 0) {
        box.x = bottomStart;
        box.y = rootHeight - bottomHeight;
        box.width = bottomStop - bottomStart;
        box.height = bottomHeight;
        InsertStrut(&box, np);
      }

    }
    JXFree(value);
    SetWorkarea();
    return;
  }

  /* Next try to read _NET_WM_STRUT */
  /* Format is: left_width, right_width, top_width, bottom_width */
  count = 0;
  status = JXGetWindowProperty(display, np->getWindow(), Hints::atoms[ATOM_NET_WM_STRUT], 0, 4, False, XA_CARDINAL,
      &actualType, &actualFormat, &count, &bytesLeft, &value);
  if (status == Success && actualFormat != 0) {
    if (JLIKELY(count == 4)) {
      lvalue = (long*) value;
      leftWidth = lvalue[0];
      rightWidth = lvalue[1];
      topHeight = lvalue[2];
      bottomHeight = lvalue[3];

      if (leftWidth > 0) {
        box.x = 0;
        box.y = 0;
        box.width = leftWidth;
        box.height = rootHeight;
        InsertStrut(&box, np);
      }

      if (rightWidth > 0) {
        box.x = rootWidth - rightWidth;
        box.y = 0;
        box.width = rightWidth;
        box.height = rootHeight;
        InsertStrut(&box, np);
      }

      if (topHeight > 0) {
        box.x = 0;
        box.y = 0;
        box.width = rootWidth;
        box.height = topHeight;
        InsertStrut(&box, np);
      }

      if (bottomHeight > 0) {
        box.x = 0;
        box.y = rootHeight - bottomHeight;
        box.width = rootWidth;
        box.height = bottomHeight;
        InsertStrut(&box, np);
      }

    }
    JXFree(value);
    SetWorkarea();
    return;
  }

  /* Struts were removed. */
  if (updated) {
    SetWorkarea();
  }

}

/** Compare two integers. */
int Places::IntComparator(const void *a, const void *b) {
  const int ia = *(const int*) a;
  const int ib = *(const int*) b;
  return ia - ib;
}

/** Set _NET_WORKAREA. */
void Places::SetWorkarea(void) {
  BoundingBox box;
  unsigned long *array;
  unsigned int count;
  int x;

  count = 4 * settings.desktopCount * sizeof(unsigned long);
  array = (unsigned long*) AllocateStack(count);

  box.x = 0;
  box.y = 0;
  box.width = rootWidth;
  box.height = rootHeight;

  Places::SubtractTrayBounds(&box, LAYER_NORMAL);
  Places::SubtractStrutBounds(&box, NULL);

  for (x = 0; x < settings.desktopCount; x++) {
    array[x * 4 + 0] = box.x;
    array[x * 4 + 1] = box.y;
    array[x * 4 + 2] = box.width;
    array[x * 4 + 3] = box.height;
  }
  JXChangeProperty(display, rootWindow, Hints::atoms[ATOM_NET_WORKAREA], XA_CARDINAL, 32, PropModeReplace,
      (unsigned char* )array, settings.desktopCount * 4);

  ReleaseStack(array);
}
