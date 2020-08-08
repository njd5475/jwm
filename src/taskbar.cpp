/**
 * @file taskbar.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Task list tray component.
 *
 */

#include "taskbar.h"

#include <stddef.h>
#include <X11/X.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <utility>

#include "button.h"
#include "client.h"
#include "clientlist.h"
#include "cursor.h"
#include "debug.h"
#include "DesktopEnvironment.h"
#include "event.h"
#include "font.h"
#include "icon.h"
#include "jwm.h"
#include "jxlib.h"
#include "main.h"
#include "menu.h"
#include "misc.h"
#include "popup.h"
#include "screen.h"
#include "settings.h"
#include "timing.h"

using namespace std;
using ::TaskBar;
std::vector<TaskBar*> TaskBar::bars;
std::map<const char*, TaskBar::BarItem*> TaskBar::taskEntries;

/** Initialize task bar data. */
void TaskBar::InitializeTaskBar(void) {

}

/** Shutdown the task bar. */
void TaskBar::ShutdownTaskBar(void) {
  bars.clear();
}

TaskBar::~TaskBar() {
  Events::_UnregisterCallback(SignalTaskbar, this);
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
    bars.erase(it);
    Release((*it));
  }

  for (auto t : taskEntries) {
    delete t.second;
  }
  taskEntries.clear();
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
  Events::_RegisterCallback(settings.popupDelay / 2, SignalTaskbar, this);
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
  if (this->pixmap != None) {
    JXFreePixmap(display, this->pixmap);
  }
  this->pixmap = JXCreatePixmap(display, rootWindow, this->getWidth(),
      this->getHeight(), rootDepth);
  Tray::ClearTrayDrawable(this);
  this->UpdateSpecificTray(this->getTray());
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

    unsigned itemCount = 0;

    this->itemHeight = this->getHeight();
    for (auto &entry : taskEntries) {
      if (entry.second->ShouldFocusEntry()) {
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

/** Process a task list button event. */
void TaskBar::ProcessButtonPress(int x, int y, int mask) {
  BarItem *entry = this->GetEntry(x, y);

  if (entry) {
    ClientNode *focused = NULL;
    char hasActive = 0;
    char allTop;

    switch (mask) {
    case Button1: /* Raise or minimize items in this group. */

      allTop = entry->IsGroupOnTop();
      if (allTop) {
        for (auto client : entry->shouldFocus()) {
          if (!ClientList::ShouldFocus(client, 0)
              || (client->isMinimized())) {
            continue;
          }

          for (int layer = LAST_LAYER; layer >= FIRST_LAYER; layer--) {
            ClientNode *np;
            std::vector<ClientNode*> inLayer = ClientList::GetLayerList(layer);
            for (int i = 0; i < inLayer.size(); ++i) {
              np = inLayer[i];
              if (!ClientList::ShouldFocus(np, 0)
                  || (np->isMinimized())) {
                continue;
              }
              if (np == client) {
                const char isActive = (np->isActive())
                    && IsClientOnCurrentDesktop(np);
                if (isActive) {
                  focused = np;
                }
                if (!(client->isStatus(STAT_CANFOCUS | STAT_TAKEFOCUS))
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
          for (auto np : entry->shouldFocus()) {
            if (np->isSticky()) {
              continue;
            }

            if (np->getDesktop() == target) {
              if (!nextClient || (np->isActive())) {
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
              nextClient->getDesktop());
          nextClient->RestoreClient(1);
        } else {
          entry->MinimizeGroup();
        }
      } else {
        /* The group was not currently on top, raise the group. */
        entry->focusGroup();
        if (focused) {
          /* If the group already contained the active client,
           * ensure that the same client remains active.
           */
          focused->keyboardFocus();
        }
      }
      break;
    case Button3:
      entry->ShowClientList(this);
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
void TaskBar::BarItem::MinimizeGroup() {
  for (auto client : clients) {
    if (ClientList::ShouldFocus(client, 1)) {
      client->MinimizeClient(0);
    }
  }
}

/** Raise all clients in a group and focus the top-most. */
void TaskBar::BarItem::focusGroup() {
  ClientNode *client = (*clients.begin());
  const char *className = client->getClassName();
  ClientNode **toRestore;
  unsigned restoreCount;
  int i;

  /* If there is no class name, then there will only be one client. */
  if (!className || !settings.groupTasks) {
    if (!(client->isSticky())) {
      DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(
          client->getDesktop());
    }
    client->RestoreClient(1);
    client->keyboardFocus();
    return;
  }

  /* If there is a client in the group on this desktop,
   * then we remain on the same desktop. */
  bool shouldSwitch = true;
  for (auto client : clients) {
    if (IsClientOnCurrentDesktop(client)) {
      shouldSwitch = false;
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
            if (!(np->isSticky())) {
              DesktopEnvironment::DefaultEnvironment()->ChangeDesktop(
                  np->getDesktop());
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
  Assert(restoreCount <= ClientNode::clientCount);
  for (i = restoreCount - 1; i >= 0; i--) {
    toRestore[i]->RestoreClient(1);
  }
  for (i = 0; i < restoreCount; i++) {
    if (toRestore[i]->isStatus(STAT_CANFOCUS | STAT_TAKEFOCUS)) {
      toRestore[i]->keyboardFocus();
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

/** Add a client to the task bar. */
void TaskBar::AddClientToTaskBar(ClientNode *np) {
  BarItem *tp = NULL;
  if (np->getClassName() && settings.groupTasks) {
    for (auto &entry : taskEntries) {
      const char *className = entry.second->getClassName();
      if (className && !strcmp(np->getClassName(), className)) {
        break;
      }
    }
  }
  if (tp == NULL) {
    tp = new BarItem(np);
    taskEntries[tp->getClassName()] = tp;
  }

  Events::_RequireTaskUpdate();
  UpdateNetClientList();

}

/** Remove a client from the task bar. */
void TaskBar::RemoveClientFromTaskBar(ClientNode *np) {
  BarItem *tp;
  std::vector<BarItem*>::iterator te;
  for (auto &entry : taskEntries) {
    tp = entry.second;

    if (tp->RemoveClient(np)) {
      if (tp->empty()) {
        taskEntries.erase(entry.first);
        Release(tp);
      }
      Events::_RequireTaskUpdate();
      UpdateNetClientList();
      return;
    }
  }
}

/** Update all task bars. */
void TaskBar::UpdateTaskBar(void) {
  int lastHeight = -1;

  if (JUNLIKELY(shouldExit)) {
    return;
  }

  TaskBar *bar = NULL;
  for (int b = 0; b < bars.size(); ++b) {
    bar = bars[b];
    if (bar->layout == LAYOUT_VERTICAL) {
      lastHeight = bar->requestedHeight;
      if (bar->userHeight > 0) {
        bar->itemHeight = bar->userHeight;
      } else {
        bar->itemHeight = Fonts::GetStringHeight(FONT_TASKLIST) + 12;
      }
      bar->requestedHeight = 0;
      for (auto &entry : taskEntries) {
        if (entry.second->ShouldFocusEntry()) {
          bar->requestedHeight += bar->itemHeight;
        }
      }
      bar->requestedHeight = Max(1, bar->requestedHeight);
      if (lastHeight != bar->requestedHeight) {
        bar->getTray()->ResizeTray();
      }
    }
    bar->ComputeItemSize();
    bar->Draw();
  }
}

/** Signal task bar (for popups). */
void TaskBar::SignalTaskbar(const TimeType *now, int x, int y, Window w,
    void *data) {

  TaskBar *bp = (TaskBar*) data;
  BarItem *ep;

  if (w == bp->getTray()->getWindow()
      && abs(bp->mousex - x) < settings.doubleClickDelta
      && abs(bp->mousey - y) < settings.doubleClickDelta) {
    if (GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
      ep = bp->GetEntry(x - bp->getScreenX(), y - bp->getScreenY());
      if (settings.groupTasks) {
        if (ep && ep->getClassName()) {
          Popups::ShowPopup(x, y, ep->getClassName(), POPUP_TASK);
        }
      } else {
        if (ep && ep->getName()) {
          Popups::ShowPopup(x, y, ep->getName(), POPUP_TASK);
        }
      }
    }
  }

}

/** Draw a specific task bar. */
void TaskBar::Draw() {
  char *displayName;
  Drawable drawable = this->getPixmap();
  bool border = false, fill = true;
  int x, y, width, height, xoffset, yoffset;
  Icon *icon = NULL;
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
  for (const auto &entry : taskEntries) {
    BarItem *client = entry.second;

    if (!client->ShouldFocusEntry()) {
      continue;
    }

    /* Check for an active or urgent window and count clients. */
    unsigned clientCount = client->activeCount();
    type = BUTTON_TASK;
    if (!client->getIcon()) {
      icon = Icon::GetDefaultIcon();
    } else {
      icon = client->getIcon();
    }
    displayName = NULL;
    if (this->labeled) {
      if (client->getClassName() && settings.groupTasks) {
        if (clientCount != 1) {
          const size_t len = strlen(client->getClassName()) + 16;
          displayName = new char[len];
          snprintf(displayName, len, "%s (%u)", client->getClassName(),
              clientCount);
          text = displayName;
        } else {
          text = client->getClassName();
        }
      } else {
        text = client->getName();
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
  BarItem *tp;

  /* Find the current entry. */
  for (auto &entry : taskEntries) {
    tp = entry.second;

    if (tp->hasActiveClient()) {
      break;
    }
  }

  /* Focus the group if one exists. */
  if (tp) {
    tp->focusGroup();
  }
}

/** Focus the previous client in the task bar. */
void TaskBar::FocusPrevious(void) {
  BarItem *tp = NULL;

  /* Find the current entry. */
  std::map<const char*, BarItem*>::iterator it = taskEntries.begin();
  for (; it != taskEntries.end(); ++it) {
    tp = it->second;

    if (it->second->hasActiveClient()) {
      break;
    }
  }

  /* Move to the previous group. */
  if (tp) {
    do {
      ++it;
      tp = it->second;
    } while (it != taskEntries.end() && !tp->ShouldFocusEntry());
  }
  if (!tp) {
    /* Wrap around; start at the end. */
    for (std::map<const char*, BarItem*>::reverse_iterator i =
        taskEntries.rbegin(); i != taskEntries.rend(); ++i) {
      if (i->second->ShouldFocusEntry()) {
        break;
      }
    }
  }

  /* Focus the group if one exists. */
  if (tp) {
    tp->focusGroup();
  }
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
  if (ClientNode::clientCount == 0) {
    return;
  }

//  /* Set _NET_CLIENT_LIST */
//  vector<__uint32_t> windows;
//
//  for (const auto &entry : taskEntries) {
//    BarItem *tp = entry.second;
//
//    vector<Window> clientWindows = tp->getClientWindows();
//    windows.insert(windows.end(), clientWindows.begin(), clientWindows.end());
//  }
//
//  JXChangeProperty(display, rootWindow, Hints::atoms[ATOM_NET_CLIENT_LIST],
//      XA_WINDOW, 32, PropModeReplace, (unsigned char* )windows.data(),
//      windows.size());
//
//  /* Set _NET_CLIENT_LIST_STACKING */
//  for (unsigned layer = FIRST_LAYER; layer <= LAST_LAYER; layer++) {
//    for (auto client : ClientList::GetLayerList(layer)) {
//      windows.push_back((__uint32_t ) client->getWindow());
//    }
//  }
//  JXChangeProperty(display, rootWindow,
//      Hints::atoms[ATOM_NET_CLIENT_LIST_STACKING], XA_WINDOW, 32,
//      PropModeReplace, (unsigned char* )windows.data(), windows.size());
//
//  if (!windows.empty()) {
//    ReleaseStack(windows);
//  }

}

unsigned int TaskBar::BarItem::activeCount() {
  int count = 0;
  for (auto client : clients) {
    if (ClientList::ShouldFocus(client, 0)
        && ((client->shouldFlash()) != 0
            || ((client->isActive())
                && IsClientOnCurrentDesktop(client)))) {
      count += 1;
    }
  }
  return count;
}

unsigned int TaskBar::BarItem::focusCount() {
  int count = 0;
  for (auto client : clients) {
    if (ClientList::ShouldFocus(client, 0)) {
      count += 1;
    }
  }
  return count;
}

bool TaskBar::BarItem::hasActiveClient() {
  for (auto client : clients) {
    if ((client->isStatus(STAT_CANFOCUS | STAT_TAKEFOCUS))
        && ClientList::ShouldFocus(client, 1)
        && (client->isActive())) {
      return true;
      break;
    }
  }
  return false;
}

/** Determine if we should attempt to focus an entry. */
bool TaskBar::BarItem::ShouldFocusEntry() {
  for (auto client : clients) {
    if (client->isStatus(STAT_CANFOCUS | STAT_TAKEFOCUS)) {
      if (ClientList::ShouldFocus(client, 1)) {
        return true;
      }
    }
  }
  return false;
}

/** Check if all clients in this grou are on the top of their layer. */
bool TaskBar::BarItem::IsGroupOnTop() {
  for (auto client : ClientList::GetLayerList(FIRST_LAYER)) {
    bool found = false;
    if (!IsClientOnCurrentDesktop(client)) {
      continue;
    }
    for (auto barClient : clients) {
      if (barClient == client) {
        return true;
      }
    }
  }

  return false;
}

/** Get the item associated with a coordinate on the task bar. */
TaskBar::BarItem* TaskBar::GetEntry(int x, int y) {
  BarItem *tp = NULL;
  int offset;

  offset = 0;
  for (auto &entry : taskEntries) {
    tp = entry.second;
    if (!tp->ShouldFocusEntry()) {
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

vector<Window> TaskBar::BarItem::getClientWindows() {
  vector<Window> windows;
  for (auto client : clients) {
    windows.push_back(client->getWindow());
  }
  return windows;
}

vector<ClientNode*> TaskBar::BarItem::shouldFocus() {
  vector<ClientNode*> shouldFocus;
  for (auto client : clients) {
    if (ClientList::ShouldFocus(client, 0)) {
      shouldFocus.push_back(client);
    }
  }
  return shouldFocus;
}

/** Show the menu associated with a task list item. */
void TaskBar::BarItem::ShowClientList(TaskBar *bar) {

  const ScreenType *sp;
  int x, y;
  Window w;

  if (settings.groupTasks) {

    /* Load the clients into the menu. */
    for (auto client : this->shouldFocus()) {
      char* name;
      if (client->isMinimized()) {
        size_t len = 0;
        if (client->getName()) {
          len = strlen(client->getName());
        }
        name = new char[len + 3];
        name[0] = '[';
        memcpy(&name[1], client->getName(), len);
        name[len + 1] = ']';
        name[len + 2] = 0;
      } else {
        name = CopyString(client->getName());
      }
      Icon* node = (client->getIcon() ? client->getIcon() : Icon::GetDefaultIcon());
      char* iconName = NULL;
      if(node) {
        iconName = node->getName();
      }
    }
  } else {
    Log("NOT IMPLEMENTED: Not grouping clients should create window menu");
  }

  /* Initialize and position the menu. */
  sp = Screens::GetCurrentScreen(bar->getScreenX(), bar->getScreenY());
  Cursors::GetMousePosition(&x, &y, &w);
  if (bar->layout == LAYOUT_HORIZONTAL) {
    if (bar->getScreenY() + bar->getHeight() / 2 < sp->y + sp->height / 2) {
      /* Bottom of the screen: menus go up. */
      y = bar->getScreenY() + bar->getHeight();
    } else {
      /* Top of the screen: menus go down. */

    }
    x = Max(x, sp->x);
  } else {
    if (bar->getScreenX() + bar->getWidth() / 2 < sp->x + sp->width / 2) {
      /* Left side: menus go right. */
      x = bar->getScreenX() + bar->getWidth();
    } else {
    }
    y = Max(y, sp->y);
  }

}

const char* TaskBar::BarItem::getClassName() {
  return (*clients.begin())->getClassName();
}

const char* TaskBar::BarItem::getName() {
  return (*clients.begin())->getName();
}

Icon* TaskBar::BarItem::getIcon() {
  return (*clients.begin())->getIcon();
}

bool TaskBar::BarItem::empty() {
  return clients.empty();
}
bool TaskBar::BarItem::RemoveClient(ClientNode *np) {
  vector<ClientNode*>::iterator pos;
  for (pos = clients.begin(); pos != clients.end(); ++pos) {
    if (np == (*pos)) {
      clients.erase(pos);
      return true;
    }
  }

  return false;
}

TaskBar::BarItem::BarItem(ClientNode *atLeastOne) {
  Assert(atLeastOne);
  clients.push_back(atLeastOne);
}

TaskBar::BarItem::~BarItem() {
}

