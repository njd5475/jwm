/*
 * ApplicationService.cpp
 *
 *  Created on: Jul 20, 2020
 *      Author: nick
 */

#include "stdlib.h"
#include "logger.h"
#include "ApplicationsSystemComponent.h"
#include "DesktopFile.h"

#include "misc.h"
#include <vector>
#include <string>
#include <iostream>
#include <string.h>
#include <experimental/filesystem> // http://en.cppreference.com/w/cpp/experimental/fs

using namespace std;
namespace stdfs = experimental::filesystem;

vector<string> getFilenames(experimental::filesystem::path path) {
  vector<string> filenames;

  // http://en.cppreference.com/w/cpp/experimental/fs/directory_iterator
  const stdfs::directory_iterator end { };

  for (stdfs::directory_iterator iter { path }; iter != end; ++iter) {
    if (stdfs::is_regular_file(*iter)) {// comment out if all names (names of directories tc.) are required
      filenames.push_back(iter->path().string());
    } else if (stdfs::is_symlink(*iter)) {
    } else if (stdfs::is_directory(*iter)) {
      for(auto file : getFilenames(*iter)) {
        filenames.push_back(file);
      }
    }
  }

  return filenames;
}

ApplicationsSystemComponent::ApplicationsSystemComponent() {
  // TODO Auto-generated constructor stub

}

ApplicationsSystemComponent::~ApplicationsSystemComponent() {
  // TODO Auto-generated destructor stub
}

void ApplicationsSystemComponent::initialize() {
  vLog("PATH is %s\n", getenv("PATH"));
  vector<string> pathDirs = splitStr(getenv("PATH"), ":");
  for (auto dirStr : pathDirs) {
    vLog("Found path dir %s\n", dirStr.c_str());
    vector<string> filesInDir = getFilenames(dirStr);
    for(auto file : filesInDir) {
      files.push_back(file);
    }
  }


  for(auto file : getFilenames("/usr/share/applications")) {
    files.push_back(file);
    const char* ext = stdfs::path(file).extension().string().c_str();
    if(strcmp(".desktop", ext) == 0) {
      vLog("Found desktop file %s\n", file.c_str());
      DesktopFile::parse(file.c_str());
    }
  }

  for(auto file : DesktopFile::getDesktopFiles()) {
    printf("Found desktop entry %s\n", file->getName());
  }

  int count = files.size();
  vLog("Found %d files\n", count);
}

void ApplicationsSystemComponent::start() {

}

void ApplicationsSystemComponent::stop() {

}

void ApplicationsSystemComponent::destroy() {

}

