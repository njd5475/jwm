/**
 * @file clientlist.c
 * @author Joe Wingbermuehle
 * @date 2007
 *
 * @brief Functions to manage lists of clients.
 *
 */

#include "nwm.h"
#include "clientlist.h"
#include "client.h"
#include "event.h"
#include "settings.h"

using namespace std;

//TODO: This really needs to be encapsulated.
vector<Client*> ClientList::nodes[LAYER_COUNT];

static Window *windowStack = NULL; /**< Image of the window stack. */
static int windowStackSize = 0; /**< Size of the image. */
static int windowStackCurrent = 0; /**< Current location in the image. */
static char walkingWindows = 0; /**< Are we walking windows? */
static char wasMinimized = 0; /**< Was the current window minimized? */

/** Determine if a client is allowed focus. */
char ClientList::ShouldFocus(const Client *np, char current) {

  /* Only display clients on the current desktop or clients that are sticky. */
  if (!settings.listAllTasks || current) {
    if (!IsClientOnCurrentDesktop(np)) {
      return 0;
    }
  }

  /* Don't display a client if it doesn't want to be displayed. */
  if (np->shouldSkipInTaskList()) {
    return 0;
  }

  /* Don't display a client on the tray if it has an owner. */
  if (np->getOwner() != None) {
    return 0;
  }

  if (!(np->isStatus(STAT_MAPPED | STAT_MINIMIZED | STAT_SHADED))) {
    return 0;
  }

  return 1;

}

/** Start walking windows in client list order. */
void ClientList::StartWindowWalk(void) {
  JXGrabKeyboard(display, rootWindow, False, GrabModeAsync, GrabModeAsync,
      CurrentTime);
  walkingWindows = 1;
}

/** Start walking the window stack. */
void ClientList::StartWindowStackWalk(void) {

  /* Get an image of the window stack.
   * Here we get the Window IDs rather than client pointers so
   * clients can be added/removed without disrupting the stack walk.
   */
  int layer;
  int count;

  /* If we are already walking the stack, just return. */
  if (windowStack != NULL) {
    return;
  }

  /* First determine how much space to allocate for windows. */
  count = 0;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      Client *client = nodes[x][at];
      if (ShouldFocus(client, 1)) {
        ++count;
      }
    }
  }

  /* If there were no windows to walk, don't even start. */
  if (count == 0) {
    return;
  }

  /* Allocate space for the windows. */
  windowStack = new Window[count];

  /* Copy windows into the array. */
  windowStackSize = 0;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      Client *client = nodes[x][at];
      if (ShouldFocus(client, 1)) {
        windowStack[windowStackSize++] = client->getWindow();
      }
    }
  }

  Assert(windowStackSize == count);

  windowStackCurrent = 0;

  JXGrabKeyboard(display, rootWindow, False, GrabModeAsync, GrabModeAsync,
      CurrentTime);

  walkingWindows = 1;
  wasMinimized = 0;

}

/** Move to the next window in the window stack. */
void ClientList::WalkWindowStack(char forward) {

  Client *np;

  if (windowStack != NULL) {
    int x;

    if (wasMinimized) {
      np = Client::FindClientByWindow(windowStack[windowStackCurrent]);
      if (np) {
        np->MinimizeClient(1);
      }
    }

    /* Loop until we either raise a window or go through them all. */
    for (x = 0; x < windowStackSize; x++) {

      /* Move to the next/previous window (wrap if needed). */
      if (forward) {
        windowStackCurrent = (windowStackCurrent + 1) % windowStackSize;
      } else {
        if (windowStackCurrent == 0) {
          windowStackCurrent = windowStackSize;
        }
        windowStackCurrent -= 1;
      }

      /* Look up the window. */
      np = Client::FindClientByWindow(windowStack[windowStackCurrent]);

      /* Skip this window if it no longer exists or is currently in
       * a state that doesn't allow focus.
       */
      if (np == NULL || !ShouldFocus(np, 1)
          || (np->isActive())) {
        continue;
      }

      /* Show the window.
       * Only when the walk completes do we update the stacking order. */
      Client::RestackClients();
      if (np->isMinimized()) {
        np->RestoreClient(1);
        wasMinimized = 1;
      } else {
        wasMinimized = 0;
      }
      JXRaiseWindow(display,
          np->getParent() ? np->getParent() : np->getWindow());
      np->keyboardFocus();
      break;

    }

  }

}

/** Stop walking the window stack or client list. */
void ClientList::StopWindowWalk(void) {

  Client *np;

  /* Raise the selected window and free the window array. */
  if (windowStack != NULL) {

    /* Look up the current window. */
    np = Client::FindClientByWindow(windowStack[windowStackCurrent]);
    if (np) {
      if (np->isMinimized()) {
        np->RestoreClient(1);
      } else {
        np->RaiseClient();
      }
    }

    Release(windowStack);
    windowStack = NULL;

    windowStackSize = 0;
    windowStackCurrent = 0;

  }

  if (walkingWindows) {
    JXUngrabKeyboard(display, CurrentTime);
    walkingWindows = 0;
  }

}

/** Focus the next client in the stacking order. */
void ClientList::FocusNextStacked(Client *client) {

  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      client = nodes[x][at];
      if ((client->isStatus(STAT_MAPPED | STAT_SHADED))
          && !(client->isHidden())) {
        client->keyboardFocus();
        return;
      }
    }
  }

  // focus first client in thats mapped shaded and not hidden only top-most layer that has clients

  for (int x = client->getLayer() - 1; x >= FIRST_LAYER; x--) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      client = nodes[x][at];
      if ((client->isStatus(STAT_MAPPED | STAT_SHADED))
          && !(client->isHidden())) {
        client->keyboardFocus();
        return;
      }
    }
  }

  /* No client to focus. */
  JXSetInputFocus(display, rootWindow, RevertToParent, Events::eventTime);

}

void ClientList::UngrabKeys() {
  Client *client;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      client = nodes[x][at];
      JXUngrabKey(display, AnyKey, AnyModifier, client->getWindow());
    }
  }
}

void ClientList::DrawBorders() {
  Client *client;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      client = nodes[x][at];
      client->DrawBorder();
    }
  }
}

void ClientList::Initialize() {

}

void ClientList::Shutdown() {
  Client *client;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
//      client = nodes[x][at];
//      client->RaiseClient();
//      client->setDelete();
//      client->DeleteClient();
//      client->RemoveClient();

    }
  }
}

vector<Client*> ClientList::GetLayerList(unsigned int layer) {
  return nodes[layer];
}

void ClientList::InsertAt(Client *node) {
  nodes[node->getLayer()].push_back(node);
}

void ClientList::MinimizeTransientWindows(Client *client, bool lower) {
//	int x = 0;
//	ClientNode *tp;
//	for (x = 0; x < LAYER_COUNT; x++) {
//		tp = nodes[x];
//		while (tp) {
//			ClientNode *next = tp->next;
//			if (tp->owner == owner && (tp->state.isStatus(STAT_MAPPED | STAT_SHADED))
//					&& !(tp->state.isMinimized())) {
//				tp->MinimizeTransients(lower);
//			}
//			tp = next;
//		}
//	} // Minimize children
  vector<Client*> children = GetChildren(client->getWindow());
  for (int i = 0; i < children.size(); ++i) {
    Client *child = children[i];
    if ((child->isStatus(STAT_MAPPED | STAT_SHADED))
        && !(child->isMinimized())) {
      child->MinimizeTransients(lower);
    }
  }

  /* Focus the next window. */
  if (client->isActive()) {
    FocusNextStacked(client);
  }

  if (lower) {
    /* Move this client to the end of the layer list. */
//		if (nodeTail[this->state.getLayer()] != this) {
//			if (this->prev) {
//				this->prev->next = this->next;
//			} else {
//				nodes[this->state.getLayer()] = this->next;
//			}
//			this->next->prev = this->prev;
//			tp = nodeTail[this->state.getLayer()];
//			nodeTail[this->state.getLayer()] = this;
//			tp->next = this;
//			this->prev = tp;
//			this->next = NULL;
//		}
    RemoveFrom(client);
    InsertAt(client);
  }
}

vector<Client*> ClientList::GetChildren(Window owner) {
  vector<Client*> children;

  Client *theChild;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int child = 0; child < nodes[x].size(); ++child) {
      theChild = nodes[x][child];
      if (theChild->getOwner() == owner) {
        children.push_back(theChild);
      }
    }
  }
  return children;
}

void ClientList::ChangeLayer(Client *node, unsigned int layer) {
  /* Remove from the old node list */
  RemoveFrom(node);

  /* Set the new layer */
  node->setLayer(layer);

  /* Insert into the new node list */
  InsertAt(node);

  Hints::WriteState(node);
}

std::vector<Client*> ClientList::GetSelfAndChildren(Client *owner) {
  vector<Client*> all;

  Client *client;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      client = nodes[x][at];
      if (client == owner || client->getWindow() == owner->getWindow()) {
        all.push_back(client);
      }
    }
  }
  return all;
}

vector<Client*> ClientList::GetMappedDesktopClients() {
  vector<Client*> all;

  Client *client;
  for (int x = 0; x < LAYER_COUNT; x++) {
    for (int at = 0; at < nodes[x].size(); ++at) {
      client = nodes[x][at];
      if (IsClientOnCurrentDesktop(client) && client->isMapped()) {
        all.push_back(client);
      }
    }
  }
  return all;
}

void ClientList::BringToTopOfLayer(Client *node) {
  RemoveFrom(node);

  nodes[node->getLayer()].insert(nodes[node->getLayer()].begin(), node);
}

vector<Client*>::iterator ClientList::find(Client *node,
    unsigned int *layer) {
  Client *client;
  for (int x = 0; x < LAYER_COUNT; x++) {
    vector<Client*>::iterator it = nodes[x].begin();
    for (; it != nodes[x].end(); ++it) {
      client = (*it);
      if (node == client) {
        *layer = x;
        return it;
      }
    }
  }
  *layer = -1;
  return vector<Client*>::iterator();
}

bool ClientList::InsertRelative(Client *node, Window above, int detail) {
  /* Insert relative to some other window. */
  bool found = false;
  bool inserted = false;
  Client *tp;
  vector<Client*> children = nodes[node->getLayer()];
  for (int i = 0; i < children.size(); ++i) {
    tp = children[i];
    if (tp == node) {
      found = true;
    } else if (tp->getWindow() == above) {
      bool insert_before = 0;
      inserted = 1;
      switch (detail) {
      case Above:
      case TopIf:
        insert_before = 1;
        break;
      case Below:
      case BottomIf:
        insert_before = 0;
        break;
      case Opposite:
        insert_before = !found;
        break;
      }
      if (insert_before) {
        /* Insert before this window. */
        InsertBefore(node, tp);
      } else {
        /* Insert after this window. */
        InsertAfter(node, tp);
      }
      break;
    }
  }

  return found;
}

vector<Client*> ClientList::GetList() {
  vector<Client*> all;
  for (int l = 0; l < LAYER_COUNT; ++l) {
    for (int x = 0; x < nodes[l].size(); ++x) {
      all.push_back(nodes[l][x]);
    }
  }
  return all;
}

void ClientList::RemoveFrom(Client *node) {
  vector<Client*>::iterator found;
  unsigned int layer = -1;
  found = find(node, &layer);
  if (layer != -1) {
    if (found != nodes[layer].begin()) {
      found--;
    }
    nodes[layer].erase(found);
  }
}

void ClientList::InsertBefore(Client *anchor, Client *toInsert) {
  vector<Client*>::iterator found;
  unsigned int layer = -1;
  found = find(anchor, &layer);
  if (layer != -1) {
    if (found != nodes[layer].begin()) {
      found--;
    }
    nodes[layer].insert(found, toInsert);
  }
}

void ClientList::InsertAfter(Client *anchor, Client *toInsert) {
  vector<Client*>::iterator found;
  unsigned int layer = -1;
  found = find(anchor, &layer);
  if (layer != -1) {
    nodes[layer].insert(found, toInsert);
  }
}

void ClientList::InsertFirst(Client* toInsert) {
  nodes[toInsert->getLayer()].insert(nodes[toInsert->getLayer()].begin(), toInsert);
}
