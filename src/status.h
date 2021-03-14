/**
 * @file status.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the status functions.
 *
 */

#ifndef STATUS_H
#define STATUS_H

class Client;

/** Create a move status window.
 * @param np The client to be moved.
 */
void CreateMoveWindow(struct Client *np);

/** Update a move status window.
 * @param np The client being moved.
 */
void UpdateMoveWindow(struct Client *np);

/** Destroy a move status window. */
void DestroyMoveWindow(void);

/** Create a resize status window.
 * @param np The client being resized.
 */
void CreateResizeWindow(struct Client *np);

/** Update a resize status window.
 * @param np The client being resized.
 * @param gwidth The width to display.
 * @param gheight The height to display.
 */
void UpdateResizeWindow(struct Client *np, int gwidth, int gheight);

/** Destroy a resize status window. */
void DestroyResizeWindow(void);

#endif /* STATUS_H */

