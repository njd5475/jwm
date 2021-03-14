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

#include "client.h"

class ClientList {
private:
	/** Client windows in linked lists for each layer. */
	static std::vector<Client*> nodes[LAYER_COUNT];

public:
	/** Determine if a client is on the current desktop.
	 * @param np The client.
	 * @return 1 if on the current desktop, 0 otherwise.
	 */
#define IsClientOnCurrentDesktop( np ) \
   ((np->getDesktop() == currentDesktop) \
      || (np->isSticky()))

	/** Determine if a client is allowed focus.
	 * @param np The client.
	 * @param current 1 if only showing clients on the current desktop.
	 * @return 1 if focus is allowed, 0 otherwise.
	 */
	static char ShouldFocus(const Client *np, char current);

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
	static void FocusNextStacked(Client *np);

	static void UngrabKeys();

	static void DrawBorders();

	static void Initialize();

	static void Shutdown();

	static std::vector<Client*> GetLayerList(unsigned int layer);
	static void InsertAt(Client *node);
	static void RemoveFrom(Client* node);
	static void MinimizeTransientWindows(Client* client, bool lower);
	static void ChangeLayer(Client *node, unsigned int layer);
	static void BringToTopOfLayer(Client *node);
	static void InsertBefore(Client* anchor, Client *toInsert);
	static void InsertAfter(Client* anchor, Client *toInsert);
	static void InsertFirst(Client* toInsert);
	static bool InsertRelative(Client* toInsert, Window above, int detail);
	static std::vector<Client*> GetList();

	static std::vector<Client*> GetChildren(Window owner);
	static std::vector<Client*> GetSelfAndChildren(Client* owner);
	static std::vector<Client*> GetMappedDesktopClients();

private:
	static std::vector<Client*>::iterator find(Client *node, unsigned int *layer);
};

#endif /* CLIENTLIST_H */
