/**
 * @file place.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client placement functions.
 *
 */

#include "jwm.h"
#include "place.h"
#include "client.h"
#include "border.h"
#include "screen.h"
#include "tray.h"
#include "settings.h"
#include "clientlist.h"
#include "misc.h"

typedef struct Strut {
  ClientNode *client;
  BoundingBox box;
  struct Strut *next;
} Strut;

static Strut *struts = NULL;

/* desktopCount x screenCount */
/* Note that we assume x and y are 0 based for all screens here. */
static int *cascadeOffsets = NULL;

static char DoRemoveClientStrut(ClientNode *np);
static void InsertStrut(const BoundingBox *box, ClientNode *np);
static void CenterClient(const BoundingBox *box, ClientNode *np);
static int IntComparator(const void *a, const void *b);
static int TryTileClient(const BoundingBox *box, ClientNode *np, int x, int y);
static char TileClient(const BoundingBox *box, ClientNode *np);
static void CascadeClient(const BoundingBox *box, ClientNode *np);

static void SubtractStrutBounds(BoundingBox *box, const ClientNode *np);
static void SubtractBounds(const BoundingBox *src, BoundingBox *dest);
static void SubtractTrayBounds(BoundingBox *box, unsigned int layer);
static void SetWorkarea(void);

/** Startup placement. */
void StartupPlacement(void) {
  const unsigned titleHeight = Border::GetTitleHeight();
  int count;
  int x;

  count = settings.desktopCount * GetScreenCount();
  cascadeOffsets = new int[count];

  for (x = 0; x < count; x++) {
    cascadeOffsets[x] = settings.borderWidth + titleHeight;
  }

  SetWorkarea();
}

/** Shutdown placement. */
void ShutdownPlacement(void) {

  Strut *sp;

  Release(cascadeOffsets);

  while (struts) {
    sp = struts->next;
    Release(struts);
    struts = sp;
  }

}

/** Remove struts associated with a client. */
void RemoveClientStrut(ClientNode *np) {
  if (DoRemoveClientStrut(np)) {
    SetWorkarea();
  }
}

/** Remove struts associated with a client. */
char DoRemoveClientStrut(ClientNode *np) {
  char updated = 0;
  Strut **spp = &struts;
  while (*spp) {
    Strut *sp = *spp;
    if (sp->client == np) {
      *spp = sp->next;
      Release(sp);
      updated = 1;
    } else {
      spp = &sp->next;
    }
  }
  return updated;
}

/** Insert a bounding box to the list of struts. */
void InsertStrut(const BoundingBox *box, ClientNode *np) {
  if (JLIKELY(box->width > 0 && box->height > 0)) {
    Strut *sp = new Strut;
    sp->client = np;
    sp->box = *box;
    sp->next = struts;
    struts = sp;
  }
}

/** Add client specified struts to our list. */
void ReadClientStrut(ClientNode *np) {

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
  status = JXGetWindowProperty(display, np->getWindow(), atoms[ATOM_NET_WM_STRUT_PARTIAL], 0, 12, False, XA_CARDINAL,
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
  status = JXGetWindowProperty(display, np->getWindow(), atoms[ATOM_NET_WM_STRUT], 0, 4, False, XA_CARDINAL,
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

/** Get the screen bounds. */
void GetScreenBounds(const ScreenType *sp, BoundingBox *box) {
  box->x = sp->x;
  box->y = sp->y;
  box->width = sp->width;
  box->height = sp->height;
}

/** Shrink dest such that it does not intersect with src. */
void SubtractBounds(const BoundingBox *src, BoundingBox *dest) {

  BoundingBox boxes[4];

  if (src->x + src->width <= dest->x) {
    return;
  }
  if (src->y + src->height <= dest->y) {
    return;
  }
  if (dest->x + dest->width <= src->x) {
    return;
  }
  if (dest->y + dest->height <= src->y) {
    return;
  }

  /* There are four ways to do this:
   *  0. Increase the x-coordinate and decrease the width of dest.
   *  1. Increase the y-coordinate and decrease the height of dest.
   *  2. Decrease the width of dest.
   *  3. Decrease the height of dest.
   * We will chose the option which leaves the greatest area.
   * Note that negative areas are possible.
   */

  /* 0 */
  boxes[0] = *dest;
  boxes[0].x = src->x + src->width;
  boxes[0].width = dest->x + dest->width - boxes[0].x;

  /* 1 */
  boxes[1] = *dest;
  boxes[1].y = src->y + src->height;
  boxes[1].height = dest->y + dest->height - boxes[1].y;

  /* 2 */
  boxes[2] = *dest;
  boxes[2].width = src->x - dest->x;

  /* 3 */
  boxes[3] = *dest;
  boxes[3].height = src->y - dest->y;

  /* 0 and 1, winner in 0. */
  if (boxes[0].width * boxes[0].height < boxes[1].width * boxes[1].height) {
    boxes[0] = boxes[1];
  }

  /* 2 and 3, winner in 2. */
  if (boxes[2].width * boxes[2].height < boxes[3].width * boxes[3].height) {
    boxes[2] = boxes[3];
  }

  /* 0 and 2, winner in dest. */
  if (boxes[0].width * boxes[0].height < boxes[2].width * boxes[2].height) {
    *dest = boxes[2];
  } else {
    *dest = boxes[0];
  }

}

/** Subtract tray area from the bounding box. */
void SubtractTrayBounds(BoundingBox *box, unsigned int layer) {
  const TrayType *tp;
  BoundingBox src;
  BoundingBox last;
  for (tp = GetTrays(); tp; tp = tp->next) {

    if (tp->layer > layer && tp->autoHide == THIDE_OFF) {

      src.x = tp->x;
      src.y = tp->y;
      src.width = tp->width;
      src.height = tp->height;
      if (src.x < 0) {
        src.width += src.x;
        src.x = 0;
      }
      if (src.y < 0) {
        src.height += src.y;
        src.y = 0;
      }

      last = *box;
      SubtractBounds(&src, box);
      if (box->width * box->height <= 0) {
        *box = last;
        break;
      }

    }

  }
}

/** Remove struts from the bounding box. */
void SubtractStrutBounds(BoundingBox *box, const ClientNode *np) {

  Strut *sp;
  BoundingBox last;

  for (sp = struts; sp; sp = sp->next) {
    if (np != NULL && sp->client == np) {
      continue;
    }
    if (IsClientOnCurrentDesktop(sp->client)) {
      last = *box;
      SubtractBounds(&sp->box, box);
      if (box->width * box->height <= 0) {
        *box = last;
        break;
      }
    }
  }

}

/** Centered placement. */
void ClientNode::CenterClient(const BoundingBox *box) {
  this->x = box->x + (box->width / 2) - (this->width / 2);
  this->y = box->y + (box->height / 2) - (this->height / 2);
  this->ConstrainSize();
  this->ConstrainPosition();
}

/** Compare two integers. */
int IntComparator(const void *a, const void *b) {
  const int ia = *(const int*) a;
  const int ib = *(const int*) b;
  return ia - ib;
}

/** Attempt to place the client at the specified coordinates. */
int ClientNode::TryTileClient(const BoundingBox *box, int x, int y) {
  const ClientNode *tp;
  int layer;
  int north, south, east, west;
  int x1, x2, y1, y2;
  int ox1, ox2, oy1, oy2;
  int overlap;

  /* Set the client position. */
  Border::GetBorderSize(&this->state, &north, &south, &east, &west);
  this->x = x + west;
  this->y = y + north;
  this->ConstrainSize();
  this->ConstrainPosition();

  /* Get the client boundaries. */
  x1 = this->x - west;
  x2 = this->x + this->width + east;
  y1 = this->y - north;
  y2 = this->y + this->height + south;

  overlap = 0;

  /* Return maximum cost for window outside bounding box. */
  if (x1 < box->x || x2 > box->x + box->width || y1 < box->y || y2 > box->y + box->height) {
    return INT_MAX;
  }

  /* Loop over each client. */
  for (layer = this->getState()->layer; layer < LAYER_COUNT; layer++) {
    for (tp = nodes[layer]; tp; tp = tp->next) {

      /* Skip clients that aren't visible. */
      if (!IsClientOnCurrentDesktop(tp)) {
        continue;
      }
      if (!(tp->state.status & STAT_MAPPED)) {
        continue;
      }
      if (tp == this) {
        continue;
      }

      /* Get the boundaries of the other client. */
      Border::GetBorderSize(tp->getState(), &north, &south, &east, &west);
      ox1 = tp->x - west;
      ox2 = tp->x + tp->width + east;
      oy1 = tp->y - north;
      oy2 = tp->y + tp->height + south;

      /* Check for an overlap. */
      if (x2 <= ox1 || x1 >= ox2) {
        continue;
      }
      if (y2 <= oy1 || y1 >= oy2) {
        continue;
      }
      overlap += (Min(ox2, x2) - Max(ox1, x1)) * (Min(oy2, y2) - Max(oy1, y1));
    }
  }

  return overlap;
}

/** Tiled placement. */
char ClientNode::TileClient(const BoundingBox *box) {

  const ClientNode *tp;
  int layer;
  int north, south, east, west;
  int i, j;
  int count;
  int *xs;
  int *ys;
  int leastOverlap;
  int bestx, besty;

  /* Count insertion points, including bounding box edges. */
  count = 2;
  for (layer = this->getState()->layer; layer < LAYER_COUNT; layer++) {
    for (tp = nodes[layer]; tp; tp = tp->next) {
      if (!IsClientOnCurrentDesktop(tp)) {
        continue;
      }
      if (!(tp->state.status & STAT_MAPPED)) {
        continue;
      }
      if (tp == this) {
        continue;
      }
      count += 2;
    }
  }

  /* Allocate space for the points. */
  xs = AllocateStack(sizeof(int) * count);
  ys = AllocateStack(sizeof(int) * count);

  /* Insert points. */
  xs[0] = box->x;
  ys[0] = box->y;
  count = 1;
  for (layer = this->getState()->layer; layer < LAYER_COUNT; layer++) {
    for (tp = nodes[layer]; tp; tp = tp->next) {
      if (!IsClientOnCurrentDesktop(tp)) {
        continue;
      }
      if (!(tp->state.status & STAT_MAPPED)) {
        continue;
      }
      if (tp == this) {
        continue;
      }
      Border::GetBorderSize(&tp->state, &north, &south, &east, &west);
      xs[count + 0] = tp->x - west;
      xs[count + 1] = tp->x + tp->width + east;
      ys[count + 0] = tp->y - north;
      ys[count + 1] = tp->y + tp->height + south;
      count += 2;
    }
  }

  /* Try placing at lower right edge of box, too. */
  Border::GetBorderSize(&this->state, &north, &south, &east, &west);
  xs[count] = box->x + box->width - this->width - east - west;
  ys[count] = box->y + box->height - this->height - north - south;
  count += 1;

  /* Sort the points. */
  qsort(xs, count, sizeof(int), IntComparator);
  qsort(ys, count, sizeof(int), IntComparator);

  /* Try all possible positions. */
  leastOverlap = INT_MAX;
  bestx = xs[0];
  besty = ys[0];
  for (i = 0; i < count; i++) {
    for (j = 0; j < count; j++) {
      const int overlap = this->TryTileClient(box, xs[i], ys[j]);
      if (overlap < leastOverlap) {
        leastOverlap = overlap;
        bestx = xs[i];
        besty = ys[j];
        if (overlap == 0) {
          break;
        }
      }
    }
  }

  ReleaseStack(xs);
  ReleaseStack(ys);

  if (leastOverlap < INT_MAX) {
    /* Set the client position. */
    Border::GetBorderSize(&this->state, &north, &south, &east, &west);
    this->x = bestx + west;
    this->y = besty + north;
    this->ConstrainSize();
    this->ConstrainPosition();
    return 1;
  } else {
    /* Tiled placement failed. */
    return 0;
  }
}

/** Cascade placement. */
void ClientNode::CascadeClient(const BoundingBox *box) {
  const ScreenType *sp;
  const unsigned titleHeight = Border::GetTitleHeight();
  int north, south, east, west;
  int cascadeIndex;
  char overflow;

  Border::GetBorderSize(&this->state, &north, &south, &east, &west);
  sp = GetMouseScreen();
  cascadeIndex = sp->index * settings.desktopCount + currentDesktop;

  /* Set the cascaded location. */
  this->x = box->x + west + cascadeOffsets[cascadeIndex];
  this->y = box->y + north + cascadeOffsets[cascadeIndex];
  cascadeOffsets[cascadeIndex] += settings.borderWidth + titleHeight;

  /* Check for cascade overflow. */
  overflow = 0;
  if (this->x + this->width - box->x > box->width) {
    overflow = 1;
  } else if (this->y + this->height - box->y > box->height) {
    overflow = 1;
  }

  if (overflow) {
    cascadeOffsets[cascadeIndex] = settings.borderWidth + titleHeight;
    this->x = box->x + west + cascadeOffsets[cascadeIndex];
    this->y = box->y + north + cascadeOffsets[cascadeIndex];

    /* Check for client overflow and update cascade position. */
    if (this->x + this->width - box->x > box->width) {
      this->x = box->x + west;
    } else if (this->y + this->height - box->y > box->height) {
      this->y = box->y + north;
    } else {
      cascadeOffsets[cascadeIndex] += settings.borderWidth + titleHeight;
    }
  }

  this->ConstrainSize();
  this->ConstrainPosition();

}

/** Place a client on the screen. */
void PlaceClient(ClientNode *np, char alreadyMapped) {

  BoundingBox box;
  const ScreenType *sp;

  Assert(np);

  if (alreadyMapped || (np->getState()->status & STAT_POSITION)
      || (!(np->getState()->status & STAT_PIGNORE) && (np->getSizeFlags() & (PPosition | USPosition)))) {

    np->GravitateClient(0);
    if (!alreadyMapped) {
      np->ConstrainSize();
      np->ConstrainPosition();
    }

  } else {

    sp = GetMouseScreen();
    GetScreenBounds(sp, &box);
    SubtractTrayBounds(&box, np->getState()->layer);
    SubtractStrutBounds(&box, np);

    /* If tiled is specified, first attempt to use tiled placement. */
    if (np->getState()->status & STAT_TILED) {
      if (np->TileClient(&box)) {
        return;
      }
    }

    /* Either tiled placement failed or was not specified. */
    if (np->getState()->status & STAT_CENTERED) {
      np->CenterClient(&box);
    } else {
      np->CascadeClient(&box);
    }

  }

}

/** Constrain the size of the client. */
char ClientNode::ConstrainSize() {

  BoundingBox box;
  const ScreenType *sp;
  int north, south, east, west;
  const int oldWidth = this->width;
  const int oldHeight = this->height;

  /* First we make sure the window isn't larger than the program allows.
   * We do this here to avoid moving the window below.
   */
  this->width = Min(this->width, this->maxWidth);
  this->height = Min(this->height, this->maxHeight);

  /* Constrain the width if necessary. */
  sp = GetCurrentScreen(this->x, this->y);
  GetScreenBounds(sp, &box);
  SubtractTrayBounds(&box, this->state.layer);
  SubtractStrutBounds(&box, this);
  Border::GetBorderSize(&this->state, &north, &south, &east, &west);
  if (this->width + east + west > sp->width) {
    box.x += west;
    box.width -= east + west;
    if (box.width > this->maxWidth) {
      box.width = this->maxWidth;
    }
    if (box.width > this->width) {
      box.width = this->width;
    }
    this->x = box.x;
    this->width = box.width - (box.width % this->xinc);
  }

  /* Constrain the height if necessary. */
  if (this->height + north + south > sp->height) {
    box.y += north;
    box.height -= north + south;
    if (box.height > this->maxHeight) {
      box.height = this->maxHeight;
    }
    if (box.height > this->height) {
      box.height = this->height;
    }
    this->y = box.y;
    this->height = box.height - (box.height % this->yinc);
  }

  /* If the program has a minimum constraint, we apply that here.
   * Note that this could cause the window to overlap something. */
  this->width = Max(this->width, this->minWidth);
  this->height = Max(this->height, this->minHeight);

  /* Fix the aspect ratio. */
  if (this->sizeFlags & PAspect) {
    if (this->width * this->aspect.miny < this->height * this->aspect.minx) {
      this->height = (this->width * this->aspect.miny) / this->aspect.minx;
    }
    if (this->width * this->aspect.maxy > this->height * this->aspect.maxx) {
      this->width = (this->height * this->aspect.maxx) / this->aspect.maxy;
    }
  }

  if (this->width != oldWidth || this->height != oldHeight) {
    return 1;
  } else {
    return 0;
  }

}

/** Constrain the position of a client. */
void ClientNode::ConstrainPosition() {

  BoundingBox box;
  int north, south, east, west;

  /* Get the bounds for placement. */
  box.x = 0;
  box.y = 0;
  box.width = rootWidth;
  box.height = rootHeight;
  SubtractTrayBounds(&box, this->state.layer);
  SubtractStrutBounds(&box, this);

  /* Fix the position. */
  Border::GetBorderSize(&this->state, &north, &south, &east, &west);
  if (this->x + this->width + east + west > box.x + box.width) {
    this->x = box.x + box.width - this->width - east;
  }
  if (this->y + this->height + north + south > box.y + box.height) {
    this->y = box.y + box.height - this->height - south;
  }
  if (this->x < box.x + west) {
    this->x = box.x + west;
  }
  if (this->y < box.y + north) {
    this->y = box.y + north;
  }

}

/** Place a maximized client on the screen. */
void ClientNode::PlaceMaximizedClient(MaxFlags flags) {
  BoundingBox box;
  const ScreenType *sp;
  int north, south, east, west;

  this->oldx = this->x;
  this->oldy = this->y;
  this->oldWidth = this->width;
  this->oldHeight = this->height;
  this->state.maxFlags = flags;

  Border::GetBorderSize(&this->state, &north, &south, &east, &west);

  sp = GetCurrentScreen(this->x + (east + west + this->width) / 2, this->y + (north + south + this->height) / 2);
  GetScreenBounds(sp, &box);
  if (!(flags & (MAX_HORIZ | MAX_LEFT | MAX_RIGHT))) {
    box.x = this->x;
    box.width = this->width;
  }
  if (!(flags & (MAX_VERT | MAX_TOP | MAX_BOTTOM))) {
    box.y = this->y;
    box.height = this->height;
  }
  SubtractTrayBounds(&box, this->state.layer);
  SubtractStrutBounds(&box, this);

  if (box.width > this->maxWidth) {
    box.width = this->maxWidth;
  }
  if (box.height > this->maxHeight) {
    box.height = this->maxHeight;
  }

  if (this->sizeFlags & PAspect) {
    if (box.width * this->aspect.miny < box.height * this->aspect.minx) {
      box.height = (box.width * this->aspect.miny) / this->aspect.minx;
    }
    if (box.width * this->aspect.maxy > box.height * this->aspect.maxx) {
      box.width = (box.height * this->aspect.maxx) / this->aspect.maxy;
    }
  }

  /* If maximizing horizontally, update width. */
  if (flags & MAX_HORIZ) {
    this->x = box.x + west;
    this->width = box.width - east - west;
    if (!(this->state.status & STAT_IIGNORE)) {
      this->width -= ((this->width - this->baseWidth) % this->xinc);
    }
  } else if (flags & MAX_LEFT) {
    this->x = box.x + west;
    this->width = box.width / 2 - east - west;
    if (!(this->state.status & STAT_IIGNORE)) {
      this->width -= ((this->width - this->baseWidth) % this->xinc);
    }
  } else if (flags & MAX_RIGHT) {
    this->x = box.x + box.width / 2 + west;
    this->width = box.width / 2 - east - west;
    if (!(this->state.status & STAT_IIGNORE)) {
      this->width -= ((this->width - this->baseWidth) % this->xinc);
    }
  }

  /* If maximizing vertically, update height. */
  if (flags & MAX_VERT) {
    this->y = box.y + north;
    this->height = box.height - north - south;
    if (!(this->state.status & STAT_IIGNORE)) {
      this->height -= ((this->height - this->baseHeight) % this->yinc);
    }
  } else if (flags & MAX_TOP) {
    this->y = box.y + north;
    this->height = box.height / 2 - north - south;
    if (!(this->state.status & STAT_IIGNORE)) {
      this->height -= ((this->height - this->baseHeight) % this->yinc);
    }
  } else if (flags & MAX_BOTTOM) {
    this->y = box.y + box.height / 2 + north;
    this->height = box.height / 2 - north - south;
    if (!(this->state.status & STAT_IIGNORE)) {
      this->height -= ((this->height - this->baseHeight) % this->yinc);
    }
  }

}

/** Determine which way to move the client for the border. */
void GetGravityDelta(const ClientNode *np, int gravity, int *x, int *y) {
  int north, south, east, west;
  Border::GetBorderSize(np->getState(), &north, &south, &east, &west);
  switch (gravity) {
  case NorthWestGravity:
    *y = -north;
    *x = -west;
    break;
  case NorthGravity:
    *y = -north;
    *x = (west - east) / 2;
    break;
  case NorthEastGravity:
    *y = -north;
    *x = west;
    break;
  case WestGravity:
    *x = -west;
    *y = (north - south) / 2;
    break;
  case CenterGravity:
    *y = (north - south) / 2;
    *x = (west - east) / 2;
    break;
  case EastGravity:
    *x = west;
    *y = (north - south) / 2;
    break;
  case SouthWestGravity:
    *y = south;
    *x = -west;
    break;
  case SouthGravity:
    *x = (west - east) / 2;
    *y = south;
    break;
  case SouthEastGravity:
    *y = south;
    *x = west;
    break;
  default: /* Static */
    *x = 0;
    *y = 0;
    break;
  }
}

/** Move the window in the specified direction for reparenting. */
void ClientNode::GravitateClient(char negate) {
  int deltax, deltay;
  GetGravityDelta(this, this->gravity, &deltax, &deltay);
  if (negate) {
    this->x += deltax;
    this->y += deltay;
  } else {
    this->x -= deltax;
    this->y -= deltay;
  }
}

/** Set _NET_WORKAREA. */
void SetWorkarea(void) {
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

  SubtractTrayBounds(&box, LAYER_NORMAL);
  SubtractStrutBounds(&box, NULL);

  for (x = 0; x < settings.desktopCount; x++) {
    array[x * 4 + 0] = box.x;
    array[x * 4 + 1] = box.y;
    array[x * 4 + 2] = box.width;
    array[x * 4 + 3] = box.height;
  }
  JXChangeProperty(display, rootWindow, atoms[ATOM_NET_WORKAREA], XA_CARDINAL, 32, PropModeReplace,
      (unsigned char* )array, settings.desktopCount * 4);

  ReleaseStack(array);
}
