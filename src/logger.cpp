/*
 * logger.cpp
 *
 *  Created on: Mar 23, 2019
 *      Author: nick
 */

#include "logger.h"

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

std::vector<int> Logger::files;

Logger::Logger() {
  // TODO Auto-generated constructor stub

}

Logger::~Logger() {
  // TODO Auto-generated destructor stub
}

void Logger::AddFile(const char *name) {
  int fd = open(name, O_CREAT | O_WRONLY | O_APPEND);
  Logger::files.push_back(fd);
}

void Logger::Log(const char* message) {
  if(!message) return;
  const char* saved = strdup(message);
  int len = strlen(saved) + 1;
  std::vector<int>::iterator it;
  int written = 0;
  for(it = files.begin(); it != files.end(); ++it) {
	do {
		written += write((*it), saved, len);
	}while(written < len);
  }
  delete saved;
}

void Logger::Close() {
  std::vector<int>::iterator it;
  for(it = files.begin(); it != files.end(); ++it) {
    close((*it));
  }
}

