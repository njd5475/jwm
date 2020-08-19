/*
 * DesktopFile.cpp
 *
 *  Created on: Aug 9, 2020
 *      Author: nick
 */

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include "misc.h"
#include "icon.h"
#include "image.h"
#include <experimental/filesystem>

#include "DesktopEnvironment.h"
#include "DesktopFile.h"

DesktopFile::DesktopFile(const char *file) :
    _file(file), _exec(NULL), _image(NULL), _icon(NULL) {
}

DesktopFile::~DesktopFile() {
  // TODO Auto-generated destructor stub
  if(_icon) {
    delete _icon;
  }
}

const char* DesktopFile::getFile() {
  return _file.c_str();
}

using namespace std;

HashMap<DesktopFile*> DesktopFile::files;

DesktopFile* DesktopFile::parse(const char *file) {
  DesktopFile *toRet = get(file);
  if (toRet) {
    return toRet;
  }

  toRet = new DesktopFile(file);

  fstream newfile;
  newfile.open(file, ios::in); //open a file to perform read operation using file object
  if (newfile.is_open()) {   //checking whether the file is open
    string tp;
    while (getline(newfile, tp)) { //read data from file object and put it into string.
      //cout << tp << "\n"; //print the data of the string
      char *link = strdup(tp.c_str());

      vector<string> split = splitStr(link, "=");

      delete link;

      if (split.size() >= 2) {

        const char* value = split[1].c_str();

        vector<string> keySplit = splitStr(strdup(split[0].c_str()), "[");

        if (keySplit.size() > 1) {
          std::string key = keySplit[0].c_str();
          const char *lang = keySplit[1].erase(keySplit[1].find_last_of(']')).c_str();

          if (key == "keywords") {
            toRet->addKeywords(lang, value);
          } else if (key == "Name") {
            toRet->addName(lang, value);
          } else if (key == "Exec") {
            toRet->setExec(value);
          } else if (key == "Icon") {
            std::string imagePath(value);
            Image *image = NULL;
            if(imagePath.rfind("/", 0) == 0) {
              //full path
              image = Image::LoadImage(value, 1, 1, NULL);
            }else{
              std::string imageFile(file);
              std::experimental::filesystem::path imagePath(file);
              imagePath.replace_filename(value);
              image = Image::LoadImage(imagePath.string().c_str(), 1, 1, NULL);
            }
            if(image == NULL) {
              toRet->setIcon(strdup(value));
            }
            toRet->setImage(image);
          }
        }
      }
    }
    newfile.close(); //close the file object.
  }

  files[file] = toRet;

  return toRet;
}

DesktopFile* DesktopFile::get(const char *file) {
  return files[file].value();
}

const char* DesktopFile::getName() {
  const char *locale = DesktopEnvironment::getLocale();
  const char *name = NULL;
  if (_names.has(locale)) {
    name = _names[DesktopEnvironment::getLocale()].value();
  } else {
    for(auto entry : _names.entries()) {
      std::string key = entry->key();
      if(key.rfind(locale, 0) == 0) {
        name = entry->value();
        break;
      }
    }
  }
  return name;
}

const char* DesktopFile::getKeywords() {
  return _keywords[DesktopEnvironment::getLocale()].value();
}

Image* DesktopFile::getImage() {
  return _image;
}

void DesktopFile::addKeywords(const char *lang, const char *value) {
  this->_keywords[strdup(lang)] = strdup(value);
}

void DesktopFile::addName(const char *lang, const char *value) {
  this->_names[strdup(lang)] = strdup(value);
}

void DesktopFile::setExec(const char *command) {
  this->_exec = strdup(command);
}

void DesktopFile::setImage(Image *image) {
  this->_image = image;
}

void DesktopFile::setIcon(const char *icon) {
  this->_icon = icon;
}

Icon* DesktopFile::getIcon() {
  return Icon::LoadNamedIcon(_icon, 0, 0);
}

vector<DesktopFile*> DesktopFile::getDesktopFiles() {
  vector<DesktopFile*> toRet;
// iterate over all entries in the hashmap
  for (auto entry : files.entries()) {
    toRet.push_back(entry->value());
  }
  return toRet;
}

