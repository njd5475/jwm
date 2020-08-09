/**
 * @file grab.c
 * @author Joe Wingbermuehle
 * @date 2013
 *
 * @brief Functions for managing server grabs.
 *
 */

#include "nwm.h"
#include "main.h"
#include "grab.h"

unsigned int Grabs::grabCount = 0;

/** Grab the server and sync. */
void Grabs::GrabServer(void) {
	if (grabCount == 0) {
		JXGrabServer(display);
		JXSync(display, False);
	}
	grabCount += 1;
}

/** Ungrab the server. */
void Grabs::UngrabServer(void) {
	Assert(grabCount > 0);
	grabCount -= 1;
	if (grabCount == 0) {
		JXUngrabServer(display);
	}
}

