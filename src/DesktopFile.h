/*
 * DesktopFile.h
 *
 *  Created on: Aug 9, 2020
 *      Author: nick
 */

#ifndef SRC_DESKTOPFILE_H_
#define SRC_DESKTOPFILE_H_

#include <vector>

class DesktopFile {
public:
  DesktopFile(const char* file);
  virtual ~DesktopFile();

  const char* getFile();

private:
  std::string _file;

public:
  static DesktopFile *parse(const char* file);

private:
  static DesktopFile *get(const char* file);
  //TODO: replace with hashmap
  static std::vector<DesktopFile*> files;
};

#endif /* SRC_DESKTOPFILE_H_ */
