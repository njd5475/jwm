/**
 * @file place.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for client placement functions.
 *
 */

#ifndef PLACE_H
#define PLACE_H

#include "client.h"

struct ScreenType;
struct TrayType;

class Places : public Client {

public:
	/*@{*/
	static void InitializePlacement() {}
	static void StartupPlacement(void);
	static void ShutdownPlacement(void);
	static void DestroyPlacement() {}
	/*@}*/

	/** Remove struts associated with a client.
	 * @param np The client.
	 */
	static void RemoveClientStrut(Client *np);

	/** Read struts associated with a client.
	 * @param np The client.
	 */
	static void ReadClientStrut(Client *np);

	/** Move a client window for a border.
	 * @param np The client.
	 * @param negate 0 to gravitate for a border, 1 to gravitate for no border.
	 */
	static void GravitateClient(Client *np, char negate);


	static int IntComparator(const void *a, const void *b);
	static void SetWorkarea(void);

};

#endif /* PLACE_H */

