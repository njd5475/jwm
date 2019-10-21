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

/** Bounding box. */
typedef struct BoundingBox {
	int x; /**< x-coordinate of the bounding box. */
	int y; /**< y-coordinate of the bounding box. */
	int width; /**< Width of the bounding box. */
	int height; /**< Height of the bounding box. */
} BoundingBox;

typedef struct Strut {
	ClientNode *client;
	BoundingBox box;
	struct Strut *next;
} Strut;

class Places : public ClientNode {

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
	static void RemoveClientStrut(ClientNode *np);

	/** Read struts associated with a client.
	 * @param np The client.
	 */
	static void ReadClientStrut(ClientNode *np);

	/** Place a client on the screen.
	 * @param np The client to place.
	 * @param alreadyMapped 1 if already mapped, 0 if unmapped.
	 */
	static void PlaceClient(ClientNode *np, char alreadyMapped);

	/** Place a maximized client on the screen.
	 * @param np The client to place.
	 * @param flags The type of maximization to perform.
	 */
	void PlaceMaximizedClient(ClientNode *np, MaxFlags flags);

	/** Move a client window for a border.
	 * @param np The client.
	 * @param negate 0 to gravitate for a border, 1 to gravitate for no border.
	 */
	static void GravitateClient(ClientNode *np, char negate);


	char TileClient(const BoundingBox *box);
	void CascadeClient(const BoundingBox *box);

	/** Constrain the size of a client.
	 * @param np The client.
	 * @return 1 if the size changed, 0 otherwise.
	 */
	char ConstrainSize();

	/** Constrain the position of a client.
	 * @param np The  client.
	 */
	void ConstrainPosition();

	/** Get the bounding box for the screen.
	 * @param sp A pointer to the screen whose bounds to get.
	 * @param box The bounding box for the screen.
	 */
	static void GetScreenBounds(const struct ScreenType *sp, BoundingBox *box);
private:

	static Strut *struts = NULL;

	/* desktopCount x screenCount */
	/* Note that we assume x and y are 0 based for all screens here. */
	static int *cascadeOffsets = NULL;

	static char DoRemoveClientStrut(ClientNode *np);
	static void InsertStrut(const BoundingBox *box, ClientNode *np);
	static int IntComparator(const void *a, const void *b);

	static void SubtractStrutBounds(BoundingBox *box, const ClientNode *np);
	static void SubtractBounds(const BoundingBox *src, BoundingBox *dest);
	static void SubtractTrayBounds(BoundingBox *box, unsigned int layer);
	static void SetWorkarea(void);

};

#endif /* PLACE_H */

