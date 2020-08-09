/**
 * @file screen.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Screen functions.
 *
 * Note that screen here refers to physical monitors. Screens are
 * determined using the xinerama extension (if available). There will
 * always be at least one screen.
 *
 */

#include "nwm.h"
#include "screen.h"
#include "main.h"
#include "cursor.h"
#include "misc.h"

static ScreenType *screens = NULL;
static int screenCount;

/** Startup screens. */
void Screens::StartupScreens(void) {
#ifdef USE_XINERAMA

	XineramaScreenInfo *info;
	int x;

	if (XineramaIsActive(display)) {

		info = XineramaQueryScreens(display, &screenCount);

		screens = new ScreenType[screenCount];
		for (x = 0; x < screenCount; x++) {
			screens[x].index = x;
			screens[x].x = info[x].x_org;
			screens[x].y = info[x].y_org;
			screens[x].width = info[x].width;
			screens[x].height = info[x].height;
		}

		JXFree(info);

	} else {

		screenCount = 1;
		screens = new ScreenType;
		screens->index = 0;
		screens->x = 0;
		screens->y = 0;
		screens->width = rootWidth;
		screens->height = rootHeight;

	}

#else

   screenCount = 1;
   screens = Allocate(sizeof(ScreenType));
   screens->index = 0;
   screens->x = 0;
   screens->y = 0;
   screens->width = rootWidth;
   screens->height = rootHeight;

#endif /* USE_XINERAMA */
}

/** Shutdown screens. */
void Screens::ShutdownScreens(void) {
	if (screens) {
		delete[](screens);
		screens = NULL;
	}
}

/** Get the screen given global screen coordinates. */
const ScreenType* Screens::GetCurrentScreen(int x, int y) {

	ScreenType *sp;
	int index;

	x = Max(0, x);
	x = Min(x, rootWidth - 1);
	y = Max(0, y);
	y = Min(y, rootHeight - 1);
	for (index = 1; index < screenCount; index++) {
		sp = &screens[index];
		if (x >= sp->x && x < sp->x + sp->width) {
			if (y >= sp->y && y < sp->y + sp->height) {
				return sp;
			}
		}
	}

	return &screens[0];

}

/** Get the screen the mouse is currently on. */
const ScreenType* Screens::GetMouseScreen(void) {
#ifdef USE_XINERAMA

	Window w;
	int x, y;

	Cursors::GetMousePosition(&x, &y, &w);
	return GetCurrentScreen(x, y);

#else

   return &screens[0];

#endif
}

/** Get data for a screen. */
const ScreenType* Screens::GetScreen(int index) {

	Assert(index >= 0);
	Assert(index < screenCount);

	return &screens[index];

}

/** Get the number of screens. */
int Screens::GetScreenCount(void) {
	return screenCount;
}

