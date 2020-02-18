/*
 * Flex.h
 *
 *  Created on: Dec 4, 2019
 *      Author: nick
 */

#ifndef SRC_FLEX_H_
#define SRC_FLEX_H_

#include "color.h"
#include <vector>

class Flex {
public:
  void Draw();

private:
  Flex();
  virtual ~Flex();

private:
  ColorName color;
  Window window;

public:
  static Flex* Create();
  static void DestroyFlexes();
  static void DrawAll();

private: /* static section */
  static std::vector<Flex*> flexes;
};

#endif /* SRC_FLEX_H_ */
