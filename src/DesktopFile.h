/*
 * DesktopFile.h
 *
 *  Created on: Aug 9, 2020
 *      Author: nick
 */

#ifndef SRC_DESKTOPFILE_H_
#define SRC_DESKTOPFILE_H_

#include <vector>
#include "HashMap.h"

class Image;
class Icon;

class DesktopFile {
public:
  DesktopFile(const char* file);
  virtual ~DesktopFile();

  const char* getFile();

  const char* getName();
  const char* getExec();
  const char* getKeywords();
  Image* getImage();
  Icon* getIcon();

private:
  void addKeywords(const char* lang, const char* value);
  void addName(const char* lang, const char* value);
  void setImage(Image* image);
  void setIcon(const char* iconName);
  void setExec(const char* exec);

private:
  std::string _file;
  HashMap<const char*> _names;
  HashMap<const char*> _keywords;
  const char* _exec;
  Image *_image;
  const char *_icon;

public:
  static DesktopFile *parse(const char* file);
  static std::vector<DesktopFile*> getDesktopFiles();

private:
  static DesktopFile *get(const char* file);
  //TODO: replace with hashmap
  static HashMap<DesktopFile*> files;
};

#endif /* SRC_DESKTOPFILE_H_ */
