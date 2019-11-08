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
  static void EnableStandardOut();

private:
  static std::vector<FILE*> files;
};

/** Initialize data structures.
 * This is called before the X connection is opened.
 */
#define Log(x) Logger::Log(x);
#define ILog(fn) \
  Logger::Log(#fn "\n");\
  fn();

#endif /* SRC_LOGGER_H_ */
