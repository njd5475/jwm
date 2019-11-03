/**
 * @file grab.h
 * @author Joe Wingbermuehle
 * @date 2013
 *
 * @brief Header file for managing server grabs.
 *
 */

#ifndef GRAB_H
#define GRAB_H

class Grabs {
public:

	/** Grab the server and sync. */
	static void GrabServer(void);

	/** Ungrab the server. */
	static void UngrabServer(void);
private:
	static unsigned int grabCount;
};

#endif /* GRAB_H */

