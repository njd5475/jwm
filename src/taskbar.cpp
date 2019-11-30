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
void TaskBar::InitializeTaskBar(void) {

}

/** Shutdown the task bar. */
void TaskBar::ShutdownTaskBar(void) {
  for (int b = 0; b < bars.size(); ++b) {
    TaskBar *bar = bars[b];
    delete bar;
  }
}

TaskBar::~TaskBar() {
  JXFreePixmap(display, this->buffer);
}

TaskBar* TaskBar::Create(Tray *tray, TrayComponent *parent) {
  TaskBar *bar = new TaskBar(tray, parent);
  bars.push_back(bar);
  return bar;
}

/** Destroy task bar data. */
void TaskBar::DestroyTaskBar(void) {
  std::vector<TaskBar*>::iterator it;
  while (!bars.empty()) {
    it = bars.begin();
    Release((*it));
    bars.erase(it);
  }
}

void TaskBar::Draw(Graphics *g) {

}

/** Create a new task bar tray component. */
TaskBar::TaskBar(Tray *tray, TrayComponent *parent) :
    TrayComponent(tray, parent) {
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
  this->pixmap = JXCreatePixmap(display, rootWindow, this->getWidth(),
      this->getHeight(), rootDepth);
  this->buffer = this->pixmap;
  Tray::ClearTrayDrawable(this);
  _RegisterCallback(settings.popupDelay / 2, SignalTaskbar, this);
}

void TaskBar::Create() {

}

/** Set the size of a task bar tray component. */
void TaskBar::SetSize(int width, int height) {
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
void TaskBar::Resize() {
  TrayComponent::Resize();
  if (this->buffer != None) {
    JXFreePixmap(display, this->buffer);
  }
  this->pixmap = JXCreatePixmap(display, rootWindow, this->getWidth(),
      this->getHeight(), rootDepth);
  this->buffer = this->pixmap;
  Tray::ClearTrayDrawable(this);
}

/** Determine the size of items in the task bar. */
void TaskBar::ComputeItemSize() {
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
    for (int i = 0; i < taskEntries.size(); ++i) {
      ep = taskEntries[i];
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
char TaskBar::IsGroupOnTop(const TaskEntry *entry) {
  ClientEntry *cp;
  int layer;

  for (layer = FIRST_LAYER; layer <= LAST_LAYER; layer++) {
    ClientNode *np;
    char foundOther = 0;
    std::vector<ClientNode*> clients = ClientList::GetLayerList(layer);
    for (int i = 0; i < clients.size(); ++i) {
      np = clients[i];
      char found = 0;
      if (!IsClientOnCurrentDesktop(np)) {
        continue;
      }
      for (int c = 0; c < entry->clients.size(); ++c) {
        cp = entry->clients[c];
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
void TaskBar::ProcessButtonPress(int x, int y, int mask) {
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
        for (int c = 0; c < entry->clients.size(); ++c) {
          cp = entry->clients[c];
          int layer;
          if (cp->client->getStatus() & STAT_MINIMIZED) {
            continue;
          } else if (!ClientList::ShouldFocus(cp->client, 0)) {
            continue;
          }
          for (layer = LAST_LAYER; layer >= FIRST_LAYER; layer--) {
            ClientNode *np;
            std::vector<ClientNode*> inLayer = ClientList::GetLayerList(layer);
            for (int i = 0; i < inLayer.size(); ++i) {
              np = inLayer[i];
              if (np->getStatus() & STAT_MINIMIZED) {
                continue;
              } else if (!ClientList::ShouldFocus(np, 0)) {
                continue;
              }
              if (np == cp->client) {
                const char isActive = (np->getStatus() & STAT_ACTIVE)
                    && IsClientOnCurrentDesktop(np);
                if (isActive) {
                  focused = np;
                }
                if (!(cp->client->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS))
                    || isActive) {
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
          for (int c = 0; c < entry->clients.size(); ++c) {
            cp = entry->clients[c];
            ClientNode *np = cp->client;
            if (!ClientList::ShouldFocus(np, 0)) {
              continue;
            } else if (np->getStatus() & STAT_STICKY) {
              continue;
            } else if (np->getState()->getDesktop() == target) {
              if (!nextClient || (np->getStatus() & STAT_ACTIVE)) {
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
          DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(
              nextClient->getState()->getDesktop());
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
void TaskBar::MinimizeGroup(const TaskEntry *tp) {
  ClientEntry *cp;
  for (int c = 0; c < tp->clients.size(); ++c) {
    cp = tp->clients[c];
    if (ClientList::ShouldFocus(cp->client, 1)) {
      cp->client->MinimizeClient(0);
    }
  }
}

/** Raise all clients in a group and focus the top-most. */
void TaskBar::FocusGroup(const TaskEntry *tp) {
  const char *className = tp->clients[0]->client->getClassName();
  ClientNode **toRestore;
  const ClientEntry *cp;
  unsigned restoreCount;
  int i;
  char shouldSwitch;

  /* If there is no class name, then there will only be one client. */
  if (!className || !settings.groupTasks) {
    if (!(tp->clients[0]->client->getStatus() & STAT_STICKY)) {
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(
          tp->clients[0]->client->getState()->getDesktop());
    }
    tp->clients[0]->client->RestoreClient(1);
    tp->clients[0]->client->FocusClient();
    return;
  }

  /* If there is a client in the group on this desktop,
   * then we remain on the same desktop. */
  shouldSwitch = 1;
  for (int c = 0; c < tp->clients.size(); ++c) {
    cp = tp->clients[c];
    if (IsClientOnCurrentDesktop(cp->client)) {
      shouldSwitch = 0;
      break;
    }
  }

  /* Switch to the desktop of the top-most client in the group. */
  if (shouldSwitch) {
    for (i = 0; i < LAYER_COUNT; i++) {
      ClientNode *np;
      std::vector<ClientNode*> inLayer = ClientList::GetLayerList(i);
      for (int x = 0; x < inLayer.size(); ++x) {
        np = inLayer[x];
        if (np->getClassName() && !strcmp(np->getClassName(), className)) {
          if (ClientList::ShouldFocus(np, 0)) {
            if (!(np->getStatus() & STAT_STICKY)) {
              DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(
                  np->getState()->getDesktop());
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
    std::vector<ClientNode*> inLayer = ClientList::GetLayerList(i);
    for (int x = 0; x < inLayer.size(); ++x) {
      np = inLayer[x];
      if (!ClientList::ShouldFocus(np, 1)) {
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
    if (toRestore[i]->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS)) {
      toRestore[i]->FocusClient();
      break;
    }
  }
  ReleaseStack(toRestore);
}

/** Process a task list motion event. */
void TaskBar::ProcessMotionEvent(int x, int y, int mask) {
  this->mousex = this->getScreenX() + x;
  this->mousey = this->getScreenY() + y;
  GetCurrentTime(&this->mouseTime);
}

/** Show the menu associated with a task list item. */
void TaskBar::ShowClientList(TaskBar *bar, TaskEntry *tp) {
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
    for (int c = 0; c < tp->clients.size(); ++c) {
      cp = tp->clients[c];
      if (!ClientList::ShouldFocus(cp->client, 0)) {
        continue;
      }
      item = Menus::CreateMenuItem(MENU_ITEM_NORMAL);
      if (cp->client->getStatus() & STAT_MINIMIZED) {
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
      item->icon =
          cp->client->getIcon() ?
              cp->client->getIcon() : Icons::GetDefaultIcon();
      item->action.type = MA_EXECUTE;
      item->action.context = cp->client;
      item->next = menu->items;
      menu->items = item;
    }
  } else {
    /* Not grouping clients. */
    menu = CreateWindowMenu(tp->clients[0]->client);
  }

  /* Initialize and position the menu. */
  Menus::InitializeMenu(menu);
  sp = Screens::GetCurrentScreen(bar->getScreenX(), bar->getScreenY());
  Cursors::GetMousePosition(&x, &y, &w);
  if (bar->layout == LAYOUT_HORIZONTAL) {
    if (bar->getScreenY() + bar->getHeight() / 2 < sp->y + sp->height / 2) {
      /* Bottom of the screen: menus go up. */
      y = bar->getScreenY() + bar->getHeight();
    } else {
      /* Top of the screen: menus go down. */
      y = bar->getScreenY() - menu->height;
    }
    x -= menu->width / 2;
    x = Max(x, sp->x);
  } else {
    if (bar->getScreenX() + bar->getWidth() / 2 < sp->x + sp->width / 2) {
      /* Left side: menus go right. */
      x = bar->getScreenX() + bar->getWidth();
    } else {
      /* Right side: menus go left. */
      x = bar->getScreenX() - menu->width;
    }
    y -= menu->height / 2;
    y = Max(y, sp->y);
  }

  Menus::ShowMenu(menu, RunTaskBarCommand, x, y, 0);

  Menus::DestroyMenu(menu);

}

/** Run a menu action. */
void TaskBar::RunTaskBarCommand(MenuAction *action, unsigned button) {
  ClientEntry *cp;

  if (action->type & MA_GROUP_MASK) {
    TaskEntry *tp = (TaskEntry*) action->context;
    for (int c = 0; c < tp->clients.size(); ++c) {
      cp = tp->clients[c];
      if (!ClientList::ShouldFocus(cp->client, 0)) {
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
      ShowWindowMenu((ClientNode*) action->context, x, y, 0);
    } else {
      ClientNode *np = (ClientNode*) action->context;
      np->RestoreClient(1);
      np->FocusClient();
      Cursors::MoveMouse(np->getWindow(), np->getWidth() / 2,
          np->getHeight() / 2);
    }
  } else {
    RunWindowCommand(action, button);
  }
}

/** Add a client to the task bar. */
void TaskBar::AddClientToTaskBar(ClientNode *np) {
  TaskEntry *tp = NULL;
  ClientEntry *cp = new ClientEntry;
  cp->client = np;

  if (np->getClassName() && settings.groupTasks) {
    for (int i = 0; i < taskEntries.size(); ++i) {
      tp = taskEntries[i];
      const char *className = (*tp->clients.begin())->client->getClassName();
      if (className && !strcmp(np->getClassName(), className)) {
        break;
      }
    }
  }
  if (tp == NULL) {
    tp = new TaskEntry;
    taskEntries.push_back(tp);
  }

  _RequireTaskUpdate();
  UpdateNetClientList();

}

/** Remove a client from the task bar. */
void TaskBar::RemoveClientFromTaskBar(ClientNode *np) {
  TaskEntry *tp;
  std::vector<TaskEntry*>::iterator te;
  for (te = taskEntries.begin(); te != taskEntries.end(); ++te) {
    tp = *te;
    ClientEntry *cp;
    std::vector<ClientEntry*>::iterator it;
    for (it = tp->clients.begin(); it != tp->clients.end(); ++it) {
      cp = *it;
      if (cp->client == np) {
        tp->clients.erase(it);
        Release(cp);
        if (tp->clients.empty()) {
          taskEntries.erase(te);
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
void TaskBar::UpdateTaskBar(void) {
  int lastHeight = -1;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  TaskBar *bar;
  for (int b = 0; b < bars.size(); ++b) {
    bar = bars[b];
    if (bar->layout == LAYOUT_VERTICAL) {
      TaskEntry *taskEntry;
      lastHeight = bar->requestedHeight;
      if (bar->userHeight > 0) {
        bar->itemHeight = bar->userHeight;
      } else {
        bar->itemHeight = Fonts::GetStringHeight(FONT_TASKLIST) + 12;
      }
      bar->requestedHeight = 0;
      for (int i = 0; i < taskEntries.size(); ++i) {
        taskEntry = taskEntries[i];
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
void TaskBar::SignalTaskbar(const TimeType *now, int x, int y, Window w,
    void *data) {

  TaskBar *bp = (TaskBar*) data;
  TaskEntry *ep;

  if (w == bp->getTray()->getWindow()
      && abs(bp->mousex - x) < settings.doubleClickDelta
      && abs(bp->mousey - y) < settings.doubleClickDelta) {
    if (GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
      ep = bp->GetEntry(x - bp->getScreenX(), y - bp->getScreenY());
      if (settings.groupTasks) {
        if (ep && ep->clients[0]->client->getClassName()) {
          Popups::ShowPopup(x, y, ep->clients[0]->client->getClassName(),
          POPUP_TASK);
        }
      } else {
        if (ep && ep->clients[0]->client->getName()) {
          Popups::ShowPopup(x, y, ep->clients[0]->client->getName(),
          POPUP_TASK);
        }
      }
    }
  }

}

/** Draw a specific task bar. */
void TaskBar::Render() {
  TaskEntry *tp;
  char *displayName;
  Drawable drawable = this->getPixmap();
  bool border = false, fill = true;
  int x, y, width, height, xoffset, yoffset;
  IconNode *icon = NULL;
  const char *text = NULL;
  AlignmentType alignment = ALIGN_CENTER;
  FontType font = FONT_TASKLIST;
  ButtonType type;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  Tray::ClearTrayDrawable(this);
  if (taskEntries.empty()) {
    this->UpdateSpecificTray(this->getTray());
    return;
  }

  border = settings.taskListDecorations == DECO_MOTIF;
  font = FONT_TASKLIST;
  height = this->itemHeight;
  width = this->itemWidth;
  text = NULL;

  x = 0;
  y = 0;
  for (int i = 0; i < taskEntries.size(); ++i) {
    tp = taskEntries[i];

    if (!ShouldShowEntry(tp)) {
      continue;
    }

    /* Check for an active or urgent window and count clients. */
    ClientEntry *cp;
    unsigned clientCount = 0;
    type = BUTTON_TASK;
    for (int c = 0; c < tp->clients.size(); ++c) {
      cp = tp->clients[c];
      if (ClientList::ShouldFocus(cp->client, 0)) {
        const char flash = (cp->client->getStatus() & STAT_FLASH) != 0;
        const char active = (cp->client->getStatus() & STAT_ACTIVE)
            && IsClientOnCurrentDesktop(cp->client);
        if (flash || active) {
          if (type == BUTTON_TASK) {
            type = BUTTON_TASK_ACTIVE;
          } else {
            type = BUTTON_TASK;
          }
        }
        clientCount += 1;
      }
    }
    if (!tp->clients[0]->client->getIcon()) {
      icon = Icons::GetDefaultIcon();
    } else {
      icon = tp->clients[0]->client->getIcon();
    }
    displayName = NULL;
    if (this->labeled) {
      if (tp->clients[0]->client->getClassName() && settings.groupTasks) {
        if (clientCount != 1) {
          const size_t len = strlen(tp->clients[0]->client->getClassName())
              + 16;
          displayName = new char[len];
          snprintf(displayName, len, "%s (%u)",
              tp->clients[0]->client->getClassName(), clientCount);
          text = displayName;
        } else {
          text = tp->clients[0]->client->getClassName();
        }
      } else {
        text = tp->clients[0]->client->getName();
      }
    }
    DrawButton(type, alignment, font, text, fill, border, drawable, icon, x, y,
        width, height, xoffset, yoffset);
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
void TaskBar::FocusNext(void) {
  TaskEntry *tp;

  /* Find the current entry. */
  std::vector<TaskEntry*>::iterator it = taskEntries.begin();
  for (; it != taskEntries.end(); ++it) {
    tp = *it;
    ClientEntry *cp;
    bool found = false;
    for (int c = 0; c < tp->clients.size(); ++c) {
      cp = tp->clients[c];
      if ((cp->client->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS))
          && ClientList::ShouldFocus(cp->client, 1)
          && (cp->client->getStatus() & STAT_ACTIVE)) {
        found = true;
        break;
      }
    }

    if (found) {
      break;
    }
  }

  /* Move to the next group. */
  if (tp) {
    do {
      ++it;
      tp = *it;
    } while (it != taskEntries.end() && !ShouldFocusEntry(tp));
  }
  if (!tp) {
    /* Wrap around; start at the beginning. */
    for (int i = 0; i < taskEntries.size(); ++i) {
      tp = taskEntries[i];
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
void TaskBar::FocusPrevious(void) {
  TaskEntry *tp = NULL;

  /* Find the current entry. */
  std::vector<TaskEntry*>::iterator it = taskEntries.begin();
  for (; it != taskEntries.end(); ++it) {
    tp = *it;
    ClientEntry *cp = NULL;
    bool found = false;
    for (int c = 0; c < tp->clients.size(); ++c) {
      cp = tp->clients[c];
      if ((cp->client->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS))
          && ClientList::ShouldFocus(cp->client, 1)
          && (cp->client->getStatus() & STAT_ACTIVE)) {
        found = true;
        break;
      }
    }

    if(found) {
      break;
    }
  }

  /* Move to the previous group. */
  if (tp) {
    do {
      ++it;
      tp = *it;
    } while (it != taskEntries.end() && !ShouldFocusEntry(tp));
  }
  if (!tp) {
    /* Wrap around; start at the end. */
    for (std::vector<TaskEntry*>::reverse_iterator i = taskEntries.rbegin();
        i != taskEntries.rend(); ++i) {
      if (ShouldFocusEntry(*i)) {
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
char TaskBar::ShouldShowEntry(const TaskEntry *tp) {
  const ClientEntry *cp = NULL;
  for (int c = 0; c < tp->clients.size(); ++c) {
    cp = tp->clients[c];
    if (ClientList::ShouldFocus(cp->client, 0)) {
      return 1;
    }
  }
  return 0;
}

/** Determine if we should attempt to focus an entry. */
char TaskBar::ShouldFocusEntry(const TaskEntry *tp) {
  const ClientEntry *cp = NULL;
  for (int c = 0; c < tp->clients.size(); ++c) {
    cp = tp->clients[c];
    if (cp->client->getStatus() & (STAT_CANFOCUS | STAT_TAKEFOCUS)) {
      if (ClientList::ShouldFocus(cp->client, 1)) {
        return 1;
      }
    }
  }
  return 0;
}

std::vector<TaskBar*> TaskBar::bars;
std::vector<TaskEntry*> TaskBar::taskEntries;

/** Get the item associated with a coordinate on the task bar. */
TaskEntry* TaskBar::GetEntry(int x, int y) {
  TaskEntry *tp = NULL;
  int offset;

  offset = 0;
  for (int i = 0; i < taskEntries.size(); ++i) {
    tp = taskEntries[i];
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
void TaskBar::SetMaxTaskBarItemWidth(unsigned int itemWidth) {
  this->maxItemWidth = itemWidth;
}

/** Set the preferred height of the specified task bar. */
void TaskBar::SetTaskBarHeight(unsigned int itemHeight) {
  this->userHeight = itemHeight;
}

/** Set whether the label should be displayed. */
void TaskBar::SetTaskBarLabeled(TrayComponent *cp, char labeled) {
  TaskBar *bp = (TaskBar*) cp;
  bp->labeled = labeled;
}

/** Maintain the _NET_CLIENT_LIST[_STACKING] properties on the root. */
void TaskBar::UpdateNetClientList(void) {
  TaskEntry *tp;
  ClientNode *client;
  __uint32_t *windows;
  unsigned int count;
  int layer;

  /* Determine how much we need to allocate. */
  if (ClientNode::clientCount == 0) {
    windows = NULL;
  } else {
    windows = new __uint32_t [ClientNode::clientCount];
  }

  /* Set _NET_CLIENT_LIST */
  count = 0;
  for (int i = 0; i < taskEntries.size(); ++i) {
    tp = taskEntries[i];
    ClientEntry *cp;
    for (int c = 0; c < tp->clients.size(); ++c) {
      cp = tp->clients[c];
      windows[count] = (__uint32_t ) cp->client->getWindow(); // need to cast to 32 bits
      count += 1;
    }
  }
  Assert(count <= clientCount);
  JXChangeProperty(display, rootWindow, Hints::atoms[ATOM_NET_CLIENT_LIST],
      XA_WINDOW, 32, PropModeReplace, (unsigned char* )windows, count);

  /* Set _NET_CLIENT_LIST_STACKING */
  count = 0;
  for (layer = FIRST_LAYER; layer <= LAST_LAYER; layer++) {
    std::vector<ClientNode*> inLayer = ClientList::GetLayerList(layer);
    for (int x = 0; x < inLayer.size(); ++x) {
      client = inLayer[x];
      windows[count] = client->getWindow();
      count += 1;
    }
  }
  JXChangeProperty(display, rootWindow,
      Hints::atoms[ATOM_NET_CLIENT_LIST_STACKING], XA_WINDOW, 32,
      PropModeReplace, (unsigned char* )windows, count);

  if (windows != NULL) {
    ReleaseStack(windows);
  }

}
