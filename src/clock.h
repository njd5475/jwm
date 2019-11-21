/**
 * @file clock.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Clock tray component.
 *
 */

#ifndef CLOCK_H
#define CLOCK_H

#include "timing.h"
#include "TrayComponent.h"

/** Structure to respresent a clock tray component. */
class ClockType : public TrayComponent {
public:
  ClockType(const char *format, const char *zone, int width, int height,  Tray *tray, TrayComponent *parent);
  virtual ~ClockType() {}
private:
  char *format; /**< The time format to use. */
  char *zone; /**< The time zone to use (NULL = local). */
  //struct ActionNode *actions; /**< Actions */
  TimeType lastTime; /**< Currently displayed time. */

  /* The following are used to control popups. */
  int mousex; /**< Last mouse x-coordinate. */
  int mousey; /**< Last mouse y-coordinate. */
  TimeType mouseTime; /**< Time of the last mouse motion. */

  int userWidth; /**< User-specified clock width (or 0). */

  struct ClockType *next; /**< Next clock in the list. */

  /** The default time format to use. */
  static const char *DEFAULT_FORMAT;

  static ClockType *clocks;

public:

  virtual void Create();
  virtual void Resize();
  virtual void Destroy();
  virtual void ProcessButtonPress(int x, int y, int button);
  virtual void ProcessButtonRelease(int x, int y, int button);
  virtual void ProcessMotionEvent(int x, int y, int mask);
  virtual void Draw(Graphics *g);

  void DrawClock(const TimeType *now);

  static void SignalClock(const struct TimeType *now, int x, int y, Window w, void *data);

  /*@{*/
  void InitializeClock(void);
  void StartupClock(void);
  static void ShutdownClock() {}
  void DestroyClock(void);
  /*@}*/

  /** Add an action to a clock.
   * @param cp The clock.
   * @param action The action to take.
   * @param mask The mouse button mask.
   */
  static void AddClockAction(struct TrayComponent *cp, const char *action, int mask);

};

#endif /* CLOCK_H */
