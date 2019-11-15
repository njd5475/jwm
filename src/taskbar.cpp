/**
 * @file taskbar.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Task list tray component.
 *
 */

#include "jwm.h"
#include "taskbar.h"
#include "tray.h"
#include "timing.h"
#include "main.h"
#include "client.h"
#include "clientlist.h"
#include "color.h"
#include "popup.h"
#include "button.h"
#include "cursor.h"
#include "icon.h"
#include "error.h"
#include "winmenu.h"
#include "screen.h"
#include "settings.h"
#include "event.h"
#include "misc.h"
#include "DesktopEnvironment.h"

/** Initialize task bar data. */
void TaskBarType::InitializeTaskBar(void) {
  bars = NULL;
  taskEntries = NULL;
  taskEntriesTail = NULL;
}

/** Shutdown the task bar. */
void TaskBarType::ShutdownTaskBar(void) {
  TaskBarType *bp;
  for (bp = bars; bp; bp = bp->next) {
    JXFreePixmap(display, bp->buffer);
  }
}

/** Destroy task bar data. */
void TaskBarType::DestroyTaskBar(void) {
  TaskBarType *bp;
  while (bars) {
    bp = bars->next;
    _UnregisterCallback(SignalTaskbar, bars);
    Release(bars);
    bars = bp;
  }
}

/** Create a new task bar tray component. */
TaskBarType::TaskBarType() {
  this->next = bars;
  bars = this;
  this->itemHeight = 0;
  this->itemWidth = 0;
  this->userHeight = 0;
  this->maxItemWidth = 0;
  this->layout = LAYOUT_HORIZONTAL;
  this->labeled = 1;
  this->mousex = -settings.doubleClickDelta;
  this->mousey = -settings.doubleClickDelta;
  this->mouseTime.seconds = 0;
  this->mouseTime.ms = 0;
  this->pixmap = JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(), rootDepth);
  this->buffer = this->pixmap;
  TrayType::ClearTrayDrawable(this);
  _RegisterCallback(settings.popupDelay / 2, SignalTaskbar, this);
}

void TaskBarType::Create() {

}

/** Set the size of a task bar tray component. */
void TaskBarType::SetSize(int width, int height) {
  if (width == 0) {
    this->layout = LAYOUT_HORIZONTAL;
  } else if (height == 0) {
    this->layout = LAYOUT_VERTICAL;
  } else if (width > height) {
    this->layout = LAYOUT_HORIZONTAL;
  } else {
    this->layout = LAYOUT_VERTICAL;
  }
}

/** Resize a task bar tray component. */
void TaskBarType::Resize() {
  if (this->buffer != None) {
    JXFreePixmap(display, this->buffer);
  }
  this->pixmap = JXCreatePixmap(display, rootWindow, this->getWidth(), this->getHeight(), rootDepth);
  this->buffer = this->pixmap;
  TrayType::ClearTrayDrawable(this);
}

/** Determine the size of items in the task bar. */
void TaskBarType::ComputeItemSize() {
  if (this->layout == LAYOUT_VERTICAL) {

    if (this->userHeight > 0) {
      this->itemHeight = this->userHeight;
    } else {
      this->itemHeight = Fonts::GetStringHeight(FONT_TASKLIST) + 12;
    }
    this->itemWidth = this->getWidth();

  } else {

    TaskEntry *ep;
    unsigned itemCount = 0;

    this->itemHeight = this->getHeight();
    for (ep = taskEntries; ep; ep = ep->next) {
      if (ShouldShowEntry(ep)) {
        itemCount += 1;
      }
    }
    if (itemCount == 0) {
      return;
    }

    this->itemWidth = Max(1, this->getWidth() / itemCount);
    if (!this->labeled) {
      this->itemWidth = Min(this->itemHeight, this->itemWidth);
    }
    if (this->maxItemWidth > 0) {
      this->itemWidth = Min(this->maxItemWidth, this->itemWidth);
    }
  }
}

/** Check if all clients in this grou are on the top of their layer. */
char TaskBarType::IsGroupOnTop(const TaskEntry *entry) {
  ClientEntry *cp;
  int layer;

  for (layer = FIRST_LAYER; layer <= LAST_LAYER; layer++) {
    ClientNode *np;
    char foundOther = 0;
    for (np = nodes[layer]; np; np = np->getNext()) {
      char found = 0;
      if (!IsClientOnCurrentDesktop(np)) {
        continue;
      }
      for (cp = entry->clients; cp; cp = cp->next) {
        if (np == cp->client) {
          if (foundOther) {
            return 0;
          }
          found = 1;
          break;
        }
      }
      foundOther = !found;
    }
  }
  return 1;
}

/** Process a task list button event. */
void TaskBarType::ProcessTaskButtonEvent(int x, int y, int mask) {
  TaskEntry *entry = this->GetEntry(x, y);

  if (entry) {
    ClientEntry *cp;
    ClientNode *focused = NULL;
    char hasActive = 0;
    char allTop;

    switch (mask) {
    case Button1: /* Raise or minimize items in this group. */

      allTop = IsGroupOnTop(entry);
      if (allTop) {
        for (cp = entry->clients; cp; cp = cp->next) {
          int layer;
          if (cp->client->getState()->getStatus() & STAT_MINIMIZED) {
            continue;
          } else if (!ShouldFocus(cp->client, 0)) {
            continue;
          }
          for (layer = LAST_LAYER; layer >= FIRST_LAYER; layer--) {
            ClientNode *np;
            for (np = nodes[layer]; np; np = np->getNext()) {
              if (np->getState()->getStatus() & STAT_MINIMIZED) {
                continue;
              } else if (!ShouldFocus(np, 0)) {
                continue;
              }
              if (np == cp->client) {
                const char isActive = (np->getState()->getStatus() & STAT_ACTIVE) && IsClientOnCurrentDesktop(np);
                if (isActive) {
                  focused = np;
                }
                if (!(cp->client->getState()->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS)) || isActive) {
                  hasActive = 1;
                }
              }
              if (hasActive) {
                goto FoundActiveAndTop;
              }
            }
          }
        }
      }
      FoundActiveAndTop: if (hasActive && allTop) {
        /* The client is already active.
         * Here we switch desktops if the client is on another
         * desktop too. Otherwise we minimize the client.
         */
        ClientNode *nextClient = NULL;
        int i;

        /* Try to find a client on a different desktop. */
        for (i = 0; i < settings.desktopCount - 1; i++) {
          const int target = (currentDesktop + i + 1) % settings.desktopCount;
          for (cp = entry->clients; cp; cp = cp->next) {
            ClientNode *np = cp->client;
            if (!ShouldFocus(np, 0)) {
              continue;
            } else if (np->getState()->getStatus() & STAT_STICKY) {
              continue;
            } else if (np->getState()->getDesktop() == target) {
              if (!nextClient || np->getState()->getStatus() & STAT_ACTIVE) {
                nextClient = np;
              }
            }
          }
          if (nextClient) {
            break;
          }
        }
        /* Focus the next client or minimize the current group. */
        if (nextClient) {
          DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(nextClient->getState()->getDesktop());
          nextClient->RestoreClient(1);
        } else {
          MinimizeGroup(entry);
        }
      } else {
        /* The group was not currently on top, raise the group. */
        FocusGroup(entry);
        if (focused) {
          /* If the group already contained the active client,
           * ensure that the same client remains active.
           */
          focused->FocusClient();
        }
      }
      break;
    case Button3:
      ShowClientList(this, entry);
      break;
    case Button4:
      FocusPrevious();
      break;
    case Button5:
      FocusNext();
      break;
    default:
      break;
    }
  }

}

/** Minimize all clients in a group. */
void TaskBarType::MinimizeGroup(const TaskEntry *tp) {
  ClientEntry *cp;
  for (cp = tp->clients; cp; cp = cp->next) {
    if (ShouldFocus(cp->client, 1)) {
      cp->client->MinimizeClient(0);
    }
  }
}

/** Raise all clients in a group and focus the top-most. */
void TaskBarType::FocusGroup(const TaskEntry *tp) {
  const char *className = tp->clients->client->getClassName();
  ClientNode **toRestore;
  const ClientEntry *cp;
  unsigned restoreCount;
  int i;
  char shouldSwitch;

  /* If there is no class name, then there will only be one client. */
  if (!className || !settings.groupTasks) {
    if (!(tp->clients->client->getState()->getStatus() & STAT_STICKY)) {
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(tp->clients->client->getState()->getDesktop());
    }
    tp->clients->client->RestoreClient(1);
    tp->clients->client->FocusClient();
    return;
  }

  /* If there is a client in the group on this desktop,
   * then we remain on the same desktop. */
  shouldSwitch = 1;
  for (cp = tp->clients; cp; cp = cp->next) {
    if (IsClientOnCurrentDesktop(cp->client)) {
      shouldSwitch = 0;
      break;
    }
  }

  /* Switch to the desktop of the top-most client in the group. */
  if (shouldSwitch) {
    for (i = 0; i < LAYER_COUNT; i++) {
      ClientNode *np;
      for (np = nodes[i]; np; np = np->getNext()) {
        if (np->getClassName() && !strcmp(np->getClassName(), className)) {
          if (ShouldFocus(np, 0)) {
            if (!(np->getState()->getStatus() & STAT_STICKY)) {
              DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(np->getState()->getDesktop());
            }
            break;
          }
        }
      }
    }
  }

  /* Build up the list of clients to restore in correct order. */
  toRestore = new ClientNode*[ClientNode::clientCount];
  restoreCount = 0;
  for (i = 0; i < LAYER_COUNT; i++) {
    ClientNode *np;
    for (np = nodes[i]; np; np = np->getNext()) {
      if (!ShouldFocus(np, 1)) {
        continue;
      }
      if (np->getClassName() && !strcmp(np->getClassName(), className)) {
        toRestore[restoreCount] = np;
        restoreCount += 1;
      }
    }
  }
  Assert(restoreCount <= clientCount);
  for (i = restoreCount - 1; i >= 0; i--) {
    toRestore[i]->RestoreClient(1);
  }
  for (i = 0; i < restoreCount; i++) {
    if (toRestore[i]->getState()->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS)) {
      toRestore[i]->FocusClient();
      break;
    }
  }
  ReleaseStack(toRestore);
}

/** Process a task list motion event. */
void TaskBarType::ProcessTaskMotionEvent(int x, int y, int mask) {
  this->mousex = this->getScreenX() + x;
  this->mousey = this->getScreenY() + y;
  GetCurrentTime(&this->mouseTime);
}

/** Show the menu associated with a task list item. */
void TaskBarType::ShowClientList(TaskBarType *bar, TaskEntry *tp) {
  Menu *menu;
  MenuItem *item;
  ClientEntry *cp;

  const ScreenType *sp;
  int x, y;
  Window w;

  if (settings.groupTasks) {

    menu = Menus::CreateMenu();

    item = Menus::CreateMenuItem(MENU_ITEM_NORMAL);
    item->name = CopyString(_("Close"));
    item->action.type = MA_CLOSE | MA_GROUP_MASK;
    item->action.context = tp;
    item->next = menu->items;
    menu->items = item;

    item = Menus::CreateMenuItem(MENU_ITEM_NORMAL);
    item->name = CopyString(_("Minimize"));
    item->action.type = MA_MINIMIZE | MA_GROUP_MASK;
    item->action.context = tp;
    item->next = menu->items;
    menu->items = item;

    item = Menus::CreateMenuItem(MENU_ITEM_NORMAL);
    item->name = CopyString(_("Restore"));
    item->action.type = MA_RESTORE | MA_GROUP_MASK;
    item->action.context = tp;
    item->next = menu->items;
    menu->items = item;

    item = Menus::CreateMenuItem(MENU_ITEM_SUBMENU);
    item->name = CopyString(_("Send To"));
    item->action.type = MA_SENDTO_MENU | MA_GROUP_MASK;
    item->action.context = tp;
    item->next = menu->items;
    menu->items = item;

    /* Load the separator and group actions. */
    item = Menus::CreateMenuItem(MENU_ITEM_SEPARATOR);
    item->next = menu->items;
    menu->items = item;

    /* Load the clients into the menu. */
    for (cp = tp->clients; cp; cp = cp->next) {
      if (!ShouldFocus(cp->client, 0)) {
        continue;
      }
      item = Menus::CreateMenuItem(MENU_ITEM_NORMAL);
      if (cp->client->getState()->getStatus() & STAT_MINIMIZED) {
        size_t len = 0;
        if (cp->client->getName()) {
          len = strlen(cp->client->getName());
        }
        item->name = new char[len + 3];
        item->name[0] = '[';
        memcpy(&item->name[1], cp->client->getName(), len);
        item->name[len + 1] = ']';
        item->name[len + 2] = 0;
      } else {
        item->name = CopyString(cp->client->getName());
      }
      item->icon = cp->client->getIcon() ? cp->client->getIcon() : Icons::GetDefaultIcon();
      item->action.type = MA_EXECUTE;
      item->action.context = cp->client;
      item->next = menu->items;
      menu->items = item;
    }
  } else {
    /* Not grouping clients. */
    menu = CreateWindowMenu(tp->clients->client);
  }

  /* Initialize and position the menu. */
  Menus::InitializeMenu(menu);
  sp = Screens::GetCurrentScreen(bar->cp->getScreenX(), bar->cp->getScreenY());
  Cursors::GetMousePosition(&x, &y, &w);
  if (bar->layout == LAYOUT_HORIZONTAL) {
    if (bar->cp->getScreenY() + bar->cp->getHeight() / 2 < sp->y + sp->height / 2) {
      /* Bottom of the screen: menus go up. */
      y = bar->cp->getScreenY() + bar->cp->getHeight();
    } else {
      /* Top of the screen: menus go down. */
      y = bar->cp->getScreenY() - menu->height;
    }
    x -= menu->width / 2;
    x = Max(x, sp->x);
  } else {
    if (bar->cp->getScreenX() + bar->cp->getWidth() / 2 < sp->x + sp->width / 2) {
      /* Left side: menus go right. */
      x = bar->cp->getScreenX() + bar->cp->getWidth();
    } else {
      /* Right side: menus go left. */
      x = bar->cp->getScreenX() - menu->width;
    }
    y -= menu->height / 2;
    y = Max(y, sp->y);
  }

  Menus::ShowMenu(menu, RunTaskBarCommand, x, y, 0);

  Menus::DestroyMenu(menu);

}

/** Run a menu action. */
void TaskBarType::RunTaskBarCommand(MenuAction *action, unsigned button) {
  ClientEntry *cp;

  if (action->type & MA_GROUP_MASK) {
    TaskEntry *tp = (TaskEntry*)action->context;
    for (cp = tp->clients; cp; cp = cp->next) {
      if (!ShouldFocus(cp->client, 0)) {
        continue;
      }
      switch (action->type & ~MA_GROUP_MASK) {
      case MA_SENDTO:
        cp->client->SetClientDesktop(action->value);
        break;
      case MA_CLOSE:
        cp->client->DeleteClient();
        break;
      case MA_RESTORE:
        cp->client->RestoreClient(0);
        break;
      case MA_MINIMIZE:
        cp->client->MinimizeClient(0);
        break;
      default:
        break;
      }
    }
  } else if (action->type == MA_EXECUTE) {
    if (button == Button3) {
      Window w;
      int x, y;
      Cursors::GetMousePosition(&x, &y, &w);
      ShowWindowMenu((ClientNode*)action->context, x, y, 0);
    } else {
      ClientNode *np = (ClientNode*)action->context;
      np->RestoreClient(1);
      np->FocusClient();
      Cursors::MoveMouse(np->getWindow(), np->getWidth() / 2, np->getHeight() / 2);
    }
  } else {
    RunWindowCommand(action, button);
  }
}

/** Add a client to the task bar. */
void TaskBarType::AddClientToTaskBar(ClientNode *np) {
  TaskEntry *tp = NULL;
  ClientEntry *cp = new ClientEntry;
  cp->client = np;

  if (np->getClassName() && settings.groupTasks) {
    for (tp = taskEntries; tp; tp = tp->next) {
      const char *className = tp->clients->client->getClassName();
      if (className && !strcmp(np->getClassName(), className)) {
        break;
      }
    }
  }
  if (tp == NULL) {
    tp = new TaskEntry;
    tp->clients = NULL;
    tp->next = NULL;
    tp->prev = taskEntriesTail;
    if (taskEntriesTail) {
      taskEntriesTail->next = tp;
    } else {
      taskEntries = tp;
    }
    taskEntriesTail = tp;
  }

  cp->next = tp->clients;
  if (tp->clients) {
    tp->clients->prev = cp;
  }
  cp->prev = NULL;
  tp->clients = cp;

  _RequireTaskUpdate();
  UpdateNetClientList();

}

/** Remove a client from the task bar. */
void TaskBarType::RemoveClientFromTaskBar(ClientNode *np) {
  TaskEntry *tp;
  for (tp = taskEntries; tp; tp = tp->next) {
    ClientEntry *cp;
    for (cp = tp->clients; cp; cp = cp->next) {
      if (cp->client == np) {
        if (cp->prev) {
          cp->prev->next = cp->next;
        } else {
          tp->clients = cp->next;
        }
        if (cp->next) {
          cp->next->prev = cp->prev;
        }
        Release(cp);
        if (!tp->clients) {
          if (tp->prev) {
            tp->prev->next = tp->next;
          } else {
            taskEntries = tp->next;
          }
          if (tp->next) {
            tp->next->prev = tp->prev;
          } else {
            taskEntriesTail = tp->prev;
          }
          Release(tp);
        }
        _RequireTaskUpdate();
        UpdateNetClientList();
        return;
      }
    }
  }
}

/** Update all task bars. */
void TaskBarType::UpdateTaskBar(void) {
  int lastHeight = -1;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  TaskBarType *bar;
  for (bar = bars; bar; bar = bar->next) {
    if (bar->layout == LAYOUT_VERTICAL) {
      TaskEntry *taskEntry;
      lastHeight = bar->requestedHeight;
      if (bar->userHeight > 0) {
        bar->itemHeight = bar->userHeight;
      } else {
        bar->itemHeight = Fonts::GetStringHeight(FONT_TASKLIST) + 12;
      }
      bar->requestedHeight = 0;
      for (taskEntry = taskEntries; taskEntry; taskEntry = taskEntry->next) {
        if (bar->ShouldShowEntry(taskEntry)) {
          bar->requestedHeight += bar->itemHeight;
        }
      }
      bar->requestedHeight = Max(1, bar->requestedHeight);
      if (lastHeight != bar->requestedHeight) {
        bar->getTray()->ResizeTray();
      }
    }
    bar->ComputeItemSize();
    bar->Render();
  }
}

/** Signal task bar (for popups). */
void TaskBarType::SignalTaskbar(const TimeType *now, int x, int y, Window w, void *data) {

  TaskBarType *bp = (TaskBarType*) data;
  TaskEntry *ep;

  if (w == bp->getTray()->getWindow() && abs(bp->mousex - x) < settings.doubleClickDelta
      && abs(bp->mousey - y) < settings.doubleClickDelta) {
    if (GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
      ep = bp->GetEntry(x - bp->getScreenX(), y - bp->getScreenY());
      if (settings.groupTasks) {
        if (ep && ep->clients->client->getClassName()) {
        	Popups::ShowPopup(x, y, ep->clients->client->getClassName(), POPUP_TASK);
        }
      } else {
        if (ep && ep->clients->client->getName()) {
        	Popups::ShowPopup(x, y, ep->clients->client->getName(), POPUP_TASK);
        }
      }
    }
  }

}

/** Draw a specific task bar. */
void TaskBarType::Render() {
  TaskEntry *tp;
  char *displayName;
  ButtonNode button;
  int x, y;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  TrayType::ClearTrayDrawable(this);
  if (!taskEntries) {
    this->UpdateSpecificTray(this->getTray());
    return;
  }

  ResetButton(&button, this->getPixmap());
  button.border = settings.taskListDecorations == DECO_MOTIF;
  button.font = FONT_TASKLIST;
  button.height = this->itemHeight;
  button.width = this->itemWidth;
  button.text = NULL;

  x = 0;
  y = 0;
  for (tp = taskEntries; tp; tp = tp->next) {

    if (!ShouldShowEntry(tp)) {
      continue;
    }

    /* Check for an active or urgent window and count clients. */
    ClientEntry *cp;
    unsigned clientCount = 0;
    button.type = BUTTON_TASK;
    for (cp = tp->clients; cp; cp = cp->next) {
      if (ShouldFocus(cp->client, 0)) {
        const char flash = (cp->client->getState()->getStatus() & STAT_FLASH) != 0;
        const char active = (cp->client->getState()->getStatus() & STAT_ACTIVE) && IsClientOnCurrentDesktop(cp->client);
        if (flash || active) {
          if (button.type == BUTTON_TASK) {
            button.type = BUTTON_TASK_ACTIVE;
          } else {
            button.type = BUTTON_TASK;
          }
        }
        clientCount += 1;
      }
    }
    button.x = x;
    button.y = y;
    if (!tp->clients->client->getIcon()) {
      button.icon = Icons::GetDefaultIcon();
    } else {
      button.icon = tp->clients->client->getIcon();
    }
    displayName = NULL;
    if (this->labeled) {
      if (tp->clients->client->getClassName() && settings.groupTasks) {
        if (clientCount != 1) {
          const size_t len = strlen(tp->clients->client->getClassName()) + 16;
          displayName = new char[len];
          snprintf(displayName, len, "%s (%u)", tp->clients->client->getClassName(), clientCount);
          button.text = displayName;
        } else {
          button.text = tp->clients->client->getClassName();
        }
      } else {
        button.text = tp->clients->client->getName();
      }
    }
    DrawButton(&button);
    if (displayName) {
      Release(displayName);
    }

    if (this->layout == LAYOUT_HORIZONTAL) {
      x += this->itemWidth;
    } else {
      y += this->itemHeight;
    }
  }

  this->UpdateSpecificTray(this->getTray());

}

/** Focus the next client in the task bar. */
void TaskBarType::FocusNext(void) {
  TaskEntry *tp;

  /* Find the current entry. */
  for (tp = taskEntries; tp; tp = tp->next) {
    ClientEntry *cp;
    for (cp = tp->clients; cp; cp = cp->next) {
      if (cp->client->getState()->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS)) {
        if (ShouldFocus(cp->client, 1)) {
          if (cp->client->getState()->getStatus() & STAT_ACTIVE) {
            cp = cp->next;
            goto ClientFound;
          }
        }
      }
    }
  }
  ClientFound:

  /* Move to the next group. */
  if (tp) {
    do {
      tp = tp->next;
    } while (tp && !ShouldFocusEntry(tp));
  }
  if (!tp) {
    /* Wrap around; start at the beginning. */
    for (tp = taskEntries; tp; tp = tp->next) {
      if (ShouldFocusEntry(tp)) {
        break;
      }
    }
  }

  /* Focus the group if one exists. */
  if (tp) {
    FocusGroup(tp);
  }
}

/** Focus the previous client in the task bar. */
void TaskBarType::FocusPrevious(void) {
  TaskEntry *tp;

  /* Find the current entry. */
  for (tp = taskEntries; tp; tp = tp->next) {
    ClientEntry *cp;
    for (cp = tp->clients; cp; cp = cp->next) {
      if (cp->client->getState()->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS)) {
        if (ShouldFocus(cp->client, 1)) {
          if (cp->client->getState()->getStatus() & STAT_ACTIVE) {
            cp = cp->next;
            goto ClientFound;
          }
        }
      }
    }
  }

  ClientFound:

  /* Move to the previous group. */
  if (tp) {
    do {
      tp = tp->prev;
    } while (tp && !ShouldFocusEntry(tp));
  }
  if (!tp) {
    /* Wrap around; start at the end. */
    for (tp = taskEntriesTail; tp; tp = tp->prev) {
      if (ShouldFocusEntry(tp)) {
        break;
      }
    }
  }

  /* Focus the group if one exists. */
  if (tp) {
    FocusGroup(tp);
  }
}

/** Determine if there is anything to show for the specified entry. */
char TaskBarType::ShouldShowEntry(const TaskEntry *tp) {
  const ClientEntry *cp;
  for (cp = tp->clients; cp; cp = cp->next) {
    if (ShouldFocus(cp->client, 0)) {
      return 1;
    }
  }
  return 0;
}

/** Determine if we should attempt to focus an entry. */
char TaskBarType::ShouldFocusEntry(const TaskEntry *tp) {
  const ClientEntry *cp;
  for (cp = tp->clients; cp; cp = cp->next) {
    if (cp->client->getState()->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS)) {
      if (ShouldFocus(cp->client, 1)) {
        return 1;
      }
    }
  }
  return 0;
}


TaskBarType *TaskBarType::bars = NULL;
TaskEntry *TaskBarType::taskEntries = NULL;
TaskEntry *TaskBarType::taskEntriesTail = NULL;

/** Get the item associated with a coordinate on the task bar. */
TaskEntry *TaskBarType::GetEntry(int x, int y) {
  TaskEntry *tp;
  int offset;

  offset = 0;
  for (tp = taskEntries; tp; tp = tp->next) {
    if (!ShouldShowEntry(tp)) {
      continue;
    }
    if (this->layout == LAYOUT_HORIZONTAL) {
      offset += this->itemWidth;
      if (x < offset) {
        return tp;
      }
    } else {
      offset += this->itemHeight;
      if (y < offset) {
        return tp;
      }
    }
  }

  return NULL;
}

/** Set the maximum width of an item in the task bar. */
void TaskBarType::SetMaxTaskBarItemWidth(const char *value) {
  int temp;

  Assert(value);

  temp = atoi(value);
  if (JUNLIKELY(temp < 0)) {
    Warning(_("invalid maxwidth for TaskList: %s"), value);
    return;
  }
  this->maxItemWidth = temp;
}

/** Set the preferred height of the specified task bar. */
void TaskBarType::SetTaskBarHeight(TrayComponentType *cp, const char *value) {
  TaskBarType *bp = (TaskBarType*) cp;
  int temp;

  temp = atoi(value);
  if (JUNLIKELY(temp < 0)) {
    Warning(_("invalid height for TaskList: %s"), value);
    return;
  }
  bp->userHeight = temp;
}

/** Set whether the label should be displayed. */
void TaskBarType::SetTaskBarLabeled(TrayComponentType *cp, char labeled) {
  TaskBarType *bp = (TaskBarType*) cp;
  bp->labeled = labeled;
}

/** Maintain the _NET_CLIENT_LIST[_STACKING] properties on the root. */
void TaskBarType::UpdateNetClientList(void) {
  TaskEntry *tp;
  ClientNode *client;
  Window *windows;
  unsigned int count;
  int layer;

  /* Determine how much we need to allocate. */
  if (ClientNode::clientCount == 0) {
    windows = NULL;
  } else {
    windows = new Window[ClientNode::clientCount];
  }

  /* Set _NET_CLIENT_LIST */
  count = 0;
  for (tp = taskEntries; tp; tp = tp->next) {
    ClientEntry *cp;
    for (cp = tp->clients; cp; cp = cp->next) {
      windows[count] = cp->client->getWindow();
      count += 1;
    }
  }
  Assert(count <= clientCount);
  JXChangeProperty(display, rootWindow, Hints::atoms[ATOM_NET_CLIENT_LIST], XA_WINDOW, 32, PropModeReplace,
      (unsigned char* )windows, count);

  /* Set _NET_CLIENT_LIST_STACKING */
  count = 0;
  for (layer = FIRST_LAYER; layer <= LAST_LAYER; layer++) {
    for (client = nodes[layer]; client; client = client->getNext()) {
      windows[count] = client->getWindow();
      count += 1;
    }
  }
  JXChangeProperty(display, rootWindow, Hints::atoms[ATOM_NET_CLIENT_LIST_STACKING], XA_WINDOW, 32, PropModeReplace,
      (unsigned char* )windows, count);

  if (windows != NULL) {
    ReleaseStack(windows);
  }

}
