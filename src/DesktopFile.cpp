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

#include "DesktopFile.h"

DesktopFile::DesktopFile(const char* file) : _file(file) {

}

DesktopFile::~DesktopFile() {
  // TODO Auto-generated destructor stub
}

const char* DesktopFile::getFile() {
  return _file.c_str();
}

using namespace std;

vector<DesktopFile*> DesktopFile::files;

DesktopFile* DesktopFile::parse(const char *file) {
  DesktopFile* toRet = get(file);
  if(toRet) {
    return toRet;
  }

  fstream newfile;
  newfile.open(file, ios::in); //open a file to perform read operation using file object
  if (newfile.is_open()) {   //checking whether the file is open
    string tp;
    while (getline(newfile, tp)) { //read data from file object and put it into string.
      //cout << tp << "\n"; //print the data of the string
    }
    newfile.close(); //close the file object.
  }

  DesktopFile *dfile = new DesktopFile(file);

  files.push_back(dfile);

  return dfile;
}

DesktopFile* DesktopFile::get(const char* file) {
  //TODO: use hash lookup for this instead of iterating a search
  for(auto df : files) {
    if(strcmp(df->getFile(), file) == 0) {
      return df;
    }
  }

  return NULL;
}

