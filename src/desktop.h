/**
 * @file desktop.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the desktop management functions.
 *
 */

#ifndef DESKTOP_H
#define DESKTOP_H

#include <vector>
struct MenuType;

class Desktops {
public:
	Desktops();
	virtual ~Desktops();

	/*@{*/
	static void _InitializeDesktops() {}
	static void _StartupDesktops(void);
	static void _ShutdownDesktops() {}
	static void _DestroyDesktops(void);
	/*@}*/

	/** Get a relative desktop. */
	static unsigned int _GetRightDesktop(unsigned int desktop);
	static unsigned int _GetLeftDesktop(unsigned int desktop);
	static unsigned int _GetAboveDesktop(unsigned int desktop);
	static unsigned int _GetBelowDesktop(unsigned int desktop);

	/** Switch to a relative desktop. */
	static char _RightDesktop(void);
	static char _LeftDesktop(void);
	static char _AboveDesktop(void);
	static char _BelowDesktop(void);

	/** Switch to a specific desktop.
	 * @param desktop The desktop to show (0 based).
	 */
	static void _ChangeDesktop(unsigned int desktop);

	/** Toggle the "show desktop" state.
	 * This will either minimize or restore all items on the current desktop.
	 */
	static void _ShowDesktop(void);

	/** Create a menu containing a list of desktops.
	 * @param mask A bit mask of desktops to highlight.
	 * @param context Context to pass the action handler.
	 * @return A menu containing all the desktops.
	 */
	static struct Menu *_CreateDesktopMenu(unsigned int mask, void *context);

	/** Create a menu containing a list of desktops.
	 * @param mask Mask to OR onto the action.
	 * @param context Context to pass the action handler.
	 * @return A menu containing all the desktops.
	 */
	static struct Menu *_CreateSendtoMenu(unsigned char mask, void *context);

	/** Set the name of a desktop.
	 * This is called before startup.
	 * @param desktop The desktop to name (0 based).
	 * @param str The name to assign.
	 */
	static void _SetDesktopName(unsigned int desktop, const char *str);

	/** Get the name of a desktop.
	 * @param desktop The desktop (0 based).
	 * @return The name of the desktop.
	 */
	static const char *_GetDesktopName(unsigned int desktop);

private:
	static std::vector<const char*> names;
	static std::vector<bool> showing;
};

#endif
