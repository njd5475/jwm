/**
 * @file pager.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Pager tray component.
 *
 */

#ifndef PAGER_H
#define PAGER_H

#include "timing.h"
#include "client.h"
#include "TrayComponent.h"

/** Structure to represent a pager tray component. */
class Pager : public TrayComponent {
private: /* statics */

  static std::vector<Pager*> pagers;
  static char shouldStopMove;
  static void PagerMoveController(int wasDestroyed);
  static void SignalPager(const TimeType *now, int x, int y, Window w, void *data);

private:

  int deskWidth; /**< Width of a desktop. */
  int deskHeight; /**< Height of a desktop. */

  int scalex; /**< Horizontal scale factor (fixed point). */
  int scaley; /**< Vertical scale factor (fixed point). */
  char labeled; /**< Set to label the pager. */

  Pixmap buffer; /**< Buffer for rendering the pager. */

  TimeType mouseTime; /**< Timestamp of last mouse movement. */
  int mousex, mousey; /**< Coordinates of last mouse location. */

  void Create();
  void Draw(Graphics *g);
  void SetSize(int width, int height);
  int GetPagerDesktop(int x, int y);

  void ProcessButtonPress(int x, int y, int mask);
  void ProcessButtonRelease(int x, int y, int mask);
  void ProcessMotionEvent(int x, int y, int mask);

  void StartPagerMove(int x, int y);
  void StopPagerMove(ClientNode *np, int x, int y, int desktop, MaxFlags maxFlags);

  void DrawPagerClient(ClientNode *np);

public:
  virtual void Draw();
  virtual ~Pager();

private:
  Pager(char labelled, Tray *tray, TrayComponent *parent);

public:
  static TrayComponent *CreatePager(char labeled, Tray * tray, TrayComponent *parent);
  /*@{*/
  static void InitializePager() {}
  static void StartupPager() {}
  static void ShutdownPager(void);
  static void DestroyPager(void);
  /*@}*/

  /** Update pagers. */
  static void UpdatePager(void);
};

#endif /* PAGER_H */

