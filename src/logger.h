/*
 * logger.h
 *
 *  Created on: Mar 23, 2019
 *      Author: nick
 */

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include <stdio.h>
#include <vector>

class Logger {
public:
  Logger();
  virtual ~Logger();

  static void AddFile(const char* name);
  static void Log(const char* message);
  static void Close();

private:
  static std::vector<int> files;
};

#endif /* SRC_LOGGER_H_ */
