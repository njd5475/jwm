/*
 * MessageService.h
 *
 *  Created on: Jul 26, 2020
 *      Author: nick
 */

#ifndef SRC_MESSAGESERVICE_H_
#define SRC_MESSAGESERVICE_H_

struct DBusConnection;
struct DBusPendingCall;
struct DBusMessage;

class MessageService {
public:
  MessageService();
  virtual ~MessageService();

public:
  static void sendSignal(const char* signal);
  static void callMethod(const char* busName, const char* path, const char* interface, const char* method, const char* param);
  static void waitForResponse(DBusPendingCall *pending);
  static void receiveSignals();
  static void exposeMethod();
  static void addRule();
  static void cleanupMessageService();
  static void reply_to_method_call(DBusMessage* msg, DBusConnection* conn);

private:
  static DBusConnection *__conn;
  static void requestName();
  static DBusConnection *getConnection();
};

#endif /* SRC_MESSAGESERVICE_H_ */
