/*
 * ApplicationService.cpp
 *
 *  Created on: Jul 20, 2020
 *      Author: nick
 */

#include "stdlib.h"
#include "logger.h"
#include "ApplicationsSystemComponent.h"

#include "misc.h"
#include <vector>
#include <string>
#include <iostream>
//#include <filesystem>

//namespace fs = std::filesystem;
using namespace std;

ApplicationsSystemComponent::ApplicationsSystemComponent() {
  // TODO Auto-generated constructor stub

}

ApplicationsSystemComponent::~ApplicationsSystemComponent() {
  // TODO Auto-generated destructor stub
}

void ApplicationsSystemComponent::initialize() {
  vLog("PATH is %s\n", std::getenv("PATH"));
  vector<string> pathDirs = splitStr(std::getenv("PATH"), ":");
  for(auto dirStr : pathDirs) {
    vLog("Found path dir %s\n", dirStr.c_str());
//    std::string path = "/path/to/directory";
//        for (const auto & entry : fs::directory_iterator(path))
//            std::cout << entry.path() << std::endl;
  }
}

void ApplicationsSystemComponent::start() {

}

void ApplicationsSystemComponent::stop() {

}

void ApplicationsSystemComponent::destroy() {

}

