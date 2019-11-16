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
#include "tray.h"

/** Structure to represent a pager tray component. */
class PagerType : public TrayComponentType {
private:

  int deskWidth; /**< Width of a desktop. */
  int deskHeight; /**< Height of a desktop. */
  int scalex; /**< Horizontal scale factor (fixed point). */
  int scaley; /**< Vertical scale factor (fixed point). */
  char labeled; /**< Set to label the pager. */

  Pixmap buffer; /**< Buffer for rendering the pager. */

  TimeType mouseTime; /**< Timestamp of last mouse movement. */
  int mousex, mousey; /**< Coordinates of last mouse location. */

  struct PagerType *next; /**< Next pager in the list. */

  static PagerType *pagers;

  static char shouldStopMove;

  void Create();

  void SetSize(int width, int height);

  int GetPagerDesktop(int x, int y);

  void ProcessPagerButtonEvent(TrayComponentType *cp, int x, int y, int mask);

  void ProcessPagerMotionEvent(TrayComponentType *cp, int x, int y, int mask);

  void StartPagerMove(TrayComponentType *cp, int x, int y);

  void StopPagerMove(ClientNode *np, int x, int y, int desktop, MaxFlags maxFlags);

  static void PagerMoveController(int wasDestroyed);

  void DrawPager();

  void DrawPagerClient(ClientNode *np);

  static void SignalPager(const TimeType *now, int x, int y, Window w, void *data);

public:
  PagerType(char labelled);
  virtual ~PagerType() {};
  /** Create a pager tray component.
   * @param labeled Set to label the pager.
   * @return A new pager tray component.
   */
  struct TrayComponentType *CreatePager(char labeled);
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

