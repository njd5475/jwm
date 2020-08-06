/*
 * MessageService.h
 *
 *  Created on: Jul 26, 2020
 *      Author: nick
 */

#ifndef SRC_MESSAGESERVICE_H_
#define SRC_MESSAGESERVICE_H_

#include <string>
#include <vector>

struct DBusConnection;
struct DBusPendingCall;
struct DBusMessage;
struct DBusPendingCall;

class MessageService {
public:
  MessageService();
  virtual ~MessageService();

public:
  static long getShutdownTime();
  static std::string getShutdownProperty();
  static char* getProperty(const char* service, const char* object, const char* interface, const char* property);

};

#endif /* SRC_MESSAGESERVICE_H_ */
