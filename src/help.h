/**
 * @file help.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the help functions.
 *
 */

#ifndef HELP_H
#define HELP_H

class Help {
public:
	/** Display program name, version, and compiled options . */
	static void DisplayAbout(void);

	/** Display compiled options. */
	static void DisplayCompileOptions(void);

	/** Display all help. */
	static void DisplayHelp(void);

	static void DisplayUsage(void);
};

#endif /* HELP_H */

