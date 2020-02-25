/*
 * KeyEventHandler.h
 *
 *  Created on: Feb 18, 2020
 *      Author: nick
 */

#ifndef SRC_KEYEVENTHANDLER_H_
#define SRC_KEYEVENTHANDLER_H_

class KeyEventHandler {
public:
  KeyEventHandler();
  virtual ~KeyEventHandler();

  virtual void handle() = 0;
};

#endif /* SRC_KEYEVENTHANDLER_H_ */
