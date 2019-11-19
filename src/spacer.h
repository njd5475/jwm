/**
 * @file spacer.h
 * @author Joe Wingbermuehle
 * @date 2011
 *
 * @brief Spacer tray component.
 *
 */
#ifndef SPACER_H
#define SPACER_H

#include "TrayComponent.h"

/** Create a spacer tray component.
 * @param width Minimum width.
 * @param height Minimum height.
 * @return A new spacer tray component.
 */

class Spacer : public TrayComponent {
public:
  Spacer(int width, int height);

  void Create();
  void Destroy();
  void SetSize(int width, int height);
  void Resize();
  void Draw(Graphics *) {}
};

#endif /* SPACER_H */

