/*
 * LoggerListener.h
 *
 *  Created on: Nov 20, 2019
 *      Author: nick
 */

#ifndef SRC_LOGGERLISTENER_H_
#define SRC_LOGGERLISTENER_H_

class LoggerListener {
public:
	LoggerListener() {}
	virtual ~LoggerListener() {}

	virtual void log(const char* message) = 0;
};

#endif /* SRC_LOGGERLISTENER_H_ */
