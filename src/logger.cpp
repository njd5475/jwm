/*
 * logger.cpp
 *
 *  Created on: Mar 23, 2019
 *      Author: nick
 */

#include "logger.h"
#include "LoggerListener.h"

#include <algorithm>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

std::vector<FILE*> Logger::files;
std::vector<LoggerListener*> Logger::listeners;

Logger::Logger() {

}

Logger::~Logger() {

}

void Logger::AddFile(const char *name) {
	FILE *fd = fopen(name, "a");
	if(fd == NULL) {
		printf("Error: Could not open %s\n", name);
	}else{
		Logger::files.push_back(fd);
	}
}

#undef Log
void Logger::Log(const char *message) {
	if (!message)
		return;
	std::vector<FILE*>::iterator it;
	for (it = files.begin(); it != files.end(); ++it) {
		fprintf((*it), message);
		fflush((*it));
	}
	NotifyListeners(message);
}

void Logger::Close() {
	std::vector<FILE*>::iterator it;
	for (it = files.begin(); it != files.end(); ++it) {
		fclose((*it));
	}
}

void Logger::EnableStandardOut() {
	files.push_back(stdout);
}

void Logger::AddListener(LoggerListener *listener) {
	listeners.push_back(listener);
}

void Logger::NotifyListeners(const char* message) {
	std::vector<LoggerListener*>::iterator it;
	for(it = listeners.begin(); it != listeners.end(); ++it) {
		(*it)->log(message);
	}
}

void Logger::RemoveListener(LoggerListener *listener) {
  std::vector<LoggerListener*>::iterator it = std::find(listeners.begin(), listeners.end(), listener);
  if(it != listeners.end()) {
    listeners.erase(it);
  }
}

