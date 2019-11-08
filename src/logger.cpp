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

std::vector<FILE*> Logger::files;

Logger::Logger() {
	// TODO Auto-generated constructor stub

}

Logger::~Logger() {
	// TODO Auto-generated destructor stub
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

