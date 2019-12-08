/**
 * @file clientlist.h
 * @author Joe Wingbermuehle
 * @date 2007
 *
 * @brief Functions to manage lists of clients.
 *
 */

#ifndef CLIENTLIST_H
#define CLIENTLIST_H

#include <vector>
#include "hint.h"
#include "ClientState.h"

class ClientNode;

class ClientList {
private:
	/** Client windows in linked lists for each layer. */
	static std::vector<ClientNode*> nodes[LAYER_COUNT];

public:
	/** Determine if a client is on the current desktop.
	 * @param np The client.
	 * @return 1 if on the current desktop, 0 otherwise.
	 */
#define IsClientOnCurrentDesktop( np ) \
   ((np->getState()->getDesktop() == currentDesktop) \
      || (np->getState()->isSticky()))

	/** Determine if a client is allowed focus.
	 * @param np The client.
	 * @param current 1 if only showing clients on the current desktop.
	 * @return 1 if focus is allowed, 0 otherwise.
	 */
	static char ShouldFocus(const ClientNode *np, char current);

	/** Start walking the window client list. */
	static void StartWindowWalk(void);

	/** Start walking the window stack. */
	static void StartWindowStackWalk();

	/** Move to the next/previous window in the window stack. */
	static void WalkWindowStack(char forward);

	/** Stop walking the window stack or client list. */
	static void StopWindowWalk(void);

	/** Set the keyboard focus to the next client.
	 * This is used to focus the next client in the stacking order.
	 * @param np The client before the client to focus.
	 */
	static void FocusNextStacked(ClientNode *np);

	static void UngrabKeys();

	static void DrawBorders();

	static void Initialize();

	static void Shutdown();

	static std::vector<ClientNode*> GetLayerList(unsigned int layer);
	static void InsertAt(ClientNode *node);
	static void RemoveFrom(ClientNode* node);
	static void MinimizeTransientWindows(ClientNode* client, bool lower);
	static void ChangeLayer(ClientNode *node, unsigned int layer);
	static void BringToTopOfLayer(ClientNode *node);
	static void InsertBefore(ClientNode* anchor, ClientNode *toInsert);
	static void InsertAfter(ClientNode* anchor, ClientNode *toInsert);
	static void InsertFirst(ClientNode* toInsert);
	static bool InsertRelative(ClientNode* toInsert, Window above, int detail);
	static std::vector<ClientNode*> GetList();

	static std::vector<ClientNode*> GetChildren(Window owner);
	static std::vector<ClientNode*> GetSelfAndChildren(ClientNode* owner);
	static std::vector<ClientNode*> GetMappedDesktopClients();

private:
	static std::vector<ClientNode*>::iterator find(ClientNode *node, unsigned int *layer);
};

#endif /* CLIENTLIST_H */
