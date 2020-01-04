/**
 * @file move.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for client window move functions.
 *
 */

#ifndef MOVE_H
#define MOVE_H

#include "border.h"
#include "timing.h"
class ClientNode;

extern char shouldStopMove;
extern char atLeft;
extern char atRight;
extern char atBottom;
extern char atTop;
extern char atSideFirst;
extern ClientNode *currentClient;
extern TimeType moveTime;

/** Move a client window.
 * @param np The client to move.
 * @param startx The starting mouse x-coordinate (window relative).
 * @param starty The starting mouse y-coordinate (window relative).
 * @return 1 if the client moved, 0 otherwise.
 */
char MoveClient(struct ClientNode *np, int startx, int starty);

/** Move a client window using the keyboard (mouse optional).
 * @param np The client to move.
 * @return 1 if the client moved, 0 otherwise.
 */
char MoveClientKeyboard(struct ClientNode *np);

void GetClientRectangle(ClientNode *np, RectangleType *r);

char CheckLeftValid(const RectangleType *client, const RectangleType *other, const RectangleType *left);

char CheckRightValid(const RectangleType *client, const RectangleType *other, const RectangleType *right);

char CheckTopValid(const RectangleType *client, const RectangleType *other, const RectangleType *top);

char CheckBottomValid(const RectangleType *client, const RectangleType *other, const RectangleType *bottom);

char CheckOverlapTopBottom(const RectangleType *a, const RectangleType *b);

char CheckOverlapLeftRight(const RectangleType *a, const RectangleType *b);

char ShouldSnap(ClientNode *np);

void SignalMove(const TimeType *now, int x, int y, Window w, void *data);

void UpdateDesktop(const TimeType *now);

void DoSnap(ClientNode *np);

void MoveController(int wasDestroyed);

#endif /* MOVE_H */

