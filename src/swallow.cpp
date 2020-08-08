/**
 * @file swallow.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Swallow tray component.
 *
 */

#include "jwm.h"
#include "swallow.h"
#include "main.h"
#include "tray.h"
#include "error.h"
#include "command.h"
#include "color.h"
#include "client.h"
#include "misc.h"

std::stack<SwallowNode*> SwallowNode::pendingNodes;
std::vector<SwallowNode*> SwallowNode::nodes;
std::vector<SwallowNode*> SwallowNode::swallowNodes;

/** Start swallow processing. */
void SwallowNode::StartupSwallow(void) {
  if(!pendingNodes.empty()) {
    fprintf(stderr, "WARNING: Swallow has pending nodes for some reason");

  }
//    if (np->command) {
//      Commands::RunCommand(np->command);
//    }
}

/** Destroy swallow data. */
void SwallowNode::DestroySwallow(void) {
  for(auto node : nodes) {
    delete node;
  }
  nodes.clear();
}

/** Release a linked list of swallow nodes. */
void SwallowNode::ReleaseNodes(SwallowNode *nodes) {
  while (nodes) {
  }
}

/** Create a swallowed application tray component. */
SwallowNode::SwallowNode(const char *name, const char *command, int width, int height, Tray *tray, TrayComponent *parent) : TrayComponent(tray, parent) {
  this->border = 0;
  if (JUNLIKELY(!name)) {
    Warning(_("cannot swallow a client with no name"));
    return;
  }

  this->name = CopyString(name);
  this->command = CopyString(command);


  if (width) {
    this->requestedWidth = width;
    this->userWidth = 1;
  } else {
    this->requestedWidth = 1;
    this->userWidth = 0;
  }
  if (height) {
    this->requestedHeight = height;
    this->userHeight = 1;
  } else {
    this->requestedHeight = 1;
    this->userHeight = 0;
  }

  Events::registerHandler(this);
  pendingNodes.push(this);
}

SwallowNode::~SwallowNode() {
  Assert(this->name);
  Release(this->name);
  if (this->command) {
    Release(this->command);
  }
}

/** Process an event on a swallowed window. */
bool SwallowNode::process(const XEvent *event) {

  int width, height;

  if (event->xany.window == this->window) {
    switch (event->type) {
    case DestroyNotify:
      this->window = None;
      this->requestedWidth = 1;
      this->requestedHeight = 1;
      this->getTray()->ResizeTray();
      break;
    case ResizeRequest:
      this->requestedWidth = event->xresizerequest.width + this->border * 2;
      this->requestedHeight = event->xresizerequest.height + this->border * 2;
      this->getTray()->ResizeTray();
      break;
    case ConfigureNotify:
      /* I don't think this should be necessary, but somehow
       * resize requests slip by sometimes... */
      width = event->xconfigure.width + this->border * 2;
      height = event->xconfigure.height + this->border * 2;
      if (width != this->getRequestedWidth()
          && height != this->getRequestedHeight()) {
        this->requestedWidth = width;
        this->requestedHeight = height;
        this->getTray()->ResizeTray();
      }
      break;
    default:
      break;
    }
    return true;
  }

  return false;

}

void SwallowNode::Create() {

}

void SwallowNode::Draw(Graphics *g) {

}

/** Handle a tray resize. */
void SwallowNode::Resize() {
  TrayComponent::Resize();
  if (this->window != None) {
    const unsigned int width = this->getWidth() - this->border * 2;
    const unsigned int height = this->getHeight() - this->border * 2;
    JXResizeWindow(display, this->window, width, height);
  }

}

/** Destroy a swallow tray component. */
void SwallowNode::Destroy() {

  /* Destroy the window if there is one. */
  if (this->window) {

    JXReparentWindow(display, this->window, rootWindow, 0, 0);
    JXRemoveFromSaveSet(display, this->window);

    if (Hints::IsDeleteAtomSet(this->window)) {
      ClientNode::SendClientMessage(this->window, ATOM_WM_PROTOCOLS,
          ATOM_WM_DELETE_WINDOW);
    } else {
      JXKillClient(display, this->window);
    }

  }

}

/** Determine if this is a window to be swallowed, if it is, swallow it. */
char SwallowNode::CheckSwallowMap(Window win) {

  SwallowNode **npp;
  XClassHint hint;
  XWindowAttributes attr;
  char result;

  /* Return if there are no programs left to swallow. */
  if (pendingNodes.empty()) {
    return 0;
  }

  /* Get the name of the window. */
  if (JXGetClassHint(display, win, &hint) == 0) {
    return 0;
  }

  /* Check if we should swallow this window. */
  result = 0;
  std::vector<SwallowNode*> toRemove;
  while(!pendingNodes.empty()) {

    SwallowNode *np = pendingNodes.top();
    pendingNodes.pop();
    Assert(np->getTray()->getWindow() != None);

    if (!strcmp(hint.res_name, np->name)) {

      /* Swallow the window. */
      JXSelectInput(display, win, StructureNotifyMask | ResizeRedirectMask);
      JXAddToSaveSet(display, win);
      JXSetWindowBorder(display, win, Colors::lookupColor(COLOR_TRAY_BG2));
      JXReparentWindow(display, win, np->getTray()->getWindow(), 0, 0);
      JXMapRaised(display, win);
      np->window = win;

      /* Remove this node from the pendingNodes list and place it
       * on the swallowNodes list. */
      toRemove.push_back(np);
      swallowNodes.push_back(np);

      /* Update the size. */
      JXGetWindowAttributes(display, win, &attr);
      np->border = attr.border_width;
      int newWidth = np->getWidth(), newHeight = np->getHeight();
      if (!np->userWidth) {
        newWidth = attr.width + 2 * np->border;
      }
      if (!np->userHeight) {
        newHeight = attr.height + 2 * np->border;
      }
      np->requestNewSize(newWidth, newHeight);

      np->getTray()->ResizeTray();
      result = 1;

      break;
    }

  }

  JXFree(hint.res_name);
  JXFree(hint.res_class);

  return result;

}

/** Determine if there are swallow processes pending. */
char SwallowNode::IsSwallowPending(void) {
  return !pendingNodes.empty();
}

