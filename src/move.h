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

extern char atLeft;
extern char atRight;
extern char atBottom;
extern char atTop;
extern char atSideFirst;
extern ClientNode *currentClient;
extern TimeType moveTime;

/** Move a client window using the keyboard (mouse optional).
 * @param np The client to move.
 * @return 1 if the client moved, 0 otherwise.
 */
char MoveClientKeyboard(struct ClientNode *np);

void GetClientRectangle(ClientNode *np, ClientRectangle *r);

char CheckLeftValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *left);

char CheckRightValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *right);

char CheckTopValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *top);

char CheckBottomValid(const ClientRectangle *client, const ClientRectangle *other, const ClientRectangle *bottom);

char CheckOverlapTopBottom(const ClientRectangle *a, const ClientRectangle *b);

char CheckOverlapLeftRight(const ClientRectangle *a, const ClientRectangle *b);

char ShouldSnap(ClientNode *np);

void SignalMove(const TimeType *now, int x, int y, Window w, void *data);

void UpdateDesktop(const TimeType *now);

void DoSnap(ClientNode *np);

#endif /* MOVE_H */

