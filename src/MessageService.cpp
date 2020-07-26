/*
 * MessageService.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author: nick
 */

#include "jwm.h"
#include "MessageService.h"

// Docs: https://dbus.freedesktop.org/doc/api/html/
#include <dbus/dbus.h>

DBusConnection *MessageService::__conn = NULL;

MessageService::MessageService() {
  // TODO Auto-generated constructor stub

}

MessageService::~MessageService() {
  // TODO Auto-generated destructor stub
}

DBusConnection* MessageService::getConnection() {
  if (__conn) {
    return __conn;
  }

  DBusError err;
  dbus_error_init(&err);

  __conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "DBus Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (NULL == __conn) {
    exit(1);
  }

  return __conn;
}

void MessageService::requestName() {
  DBusError err;
  dbus_error_init(&err);
  // request a name on the bus
  int ret = dbus_bus_request_name(__conn, "test.method.server",
  DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "DBus Name Error (%s)\n", err.message);
    dbus_error_free(&err);
  }
  if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
    exit(1);
  }
}

void MessageService::sendSignal(const char* signal) {
  dbus_uint32_t serial = 0; // unique number to associate replies with requests
  DBusMessage *msg;
  DBusMessageIter args;

  // create a signal and check for errors
  msg = dbus_message_new_signal("/test/signal/Object", // object name of the signal
      "test.signal.Type", // interface name of the signal
      "Test"); // name of the signal
  if (NULL == msg) {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

  // append arguments onto signal
  dbus_message_iter_init_append(msg, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, (void*)signal)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }

  // send the message and flush the connection
  if (!dbus_connection_send(__conn, msg, &serial)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  dbus_connection_flush(__conn);

  // free the message
  dbus_message_unref(msg);
}

void MessageService::callMethod(const char *busName, const char *path,
    const char *interface, const char *method, const char* param) {
  DBusMessage *msg;
  DBusMessageIter args;
  DBusPendingCall *pending;

  msg = dbus_message_new_method_call(busName, // target for the method call
      path, // object to call on
      interface, // interface to call on
      method); // method name
  if (NULL == msg) {
    fprintf(stderr, "Message Null\n");
    exit(1);
  }

  // append arguments
  dbus_message_iter_init_append(msg, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, (void*)param)) {
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }

  // send message and get a handle for a reply
  if (!dbus_connection_send_with_reply(__conn, msg, &pending, -1)) { // -1 is default timeout
    fprintf(stderr, "Out Of Memory!\n");
    exit(1);
  }
  if (NULL == pending) {
    fprintf(stderr, "Pending Call Null\n");
    exit(1);
  }
  dbus_connection_flush(__conn);

  // free message
  dbus_message_unref(msg);
}

void MessageService::waitForResponse(DBusPendingCall *pending) {
  bool stat;
  dbus_uint32_t level;

  // block until we receive a reply
  dbus_pending_call_block(pending);

  // get the reply message
  DBusMessage *msg = dbus_pending_call_steal_reply(pending);
  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);
  if (NULL == msg) {
    fprintf(stderr, "Reply Null\n");
    exit(1);
  }
  // free the pending message handle
  dbus_pending_call_unref(pending);

  // read the parameters
  if (!dbus_message_iter_init(msg, &args))
    fprintf(stderr, "Message has no arguments!\n");
  else if (DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&args))
    fprintf(stderr, "Argument is not boolean!\n");
  else
    dbus_message_iter_get_basic(&args, &stat);

  if (!dbus_message_iter_next(&args))
    fprintf(stderr, "Message has too few arguments!\n");
  else if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args))
    fprintf(stderr, "Argument is not int!\n");
  else
    dbus_message_iter_get_basic(&args, &level);

  printf("Got Reply: %d, %d\n", stat, level);

  // free reply and close connection
  dbus_message_unref(msg);
}

void MessageService::addRule() {
  DBusError err;
  dbus_error_init(&err);
  // add a rule for which messages we want to see
  dbus_bus_add_match(__conn, "type='signal',interface='test.signal.Type'",
      &err); // see signals from the given interface
  dbus_connection_flush(__conn);
  if (dbus_error_is_set(&err)) {
    fprintf(stderr, "Match Error (%s)\n", err.message);
    exit(1);
  }
}

void MessageService::receiveSignals() {
  // loop listening for signals being emmitted
  while (true) {

    // non blocking read of the next available message
    dbus_connection_read_write(getConnection(), 0);
    DBusMessage *msg = dbus_connection_pop_message(getConnection());
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    char buf[255];
    memset((void*)buf, 0, sizeof(buf));

    // loop again if we haven't read a message
    if (NULL == msg) {
      sleep(1);
      continue;
    }

    // check if the message is a signal from the correct interface and with the correct name
    if (dbus_message_is_signal(msg, "test.signal.Type", "Test")) {
      // read the parameters
      if (!dbus_message_iter_init(msg, &args))
        fprintf(stderr, "Message has no arguments!\n");
      else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
        fprintf(stderr, "Argument is not string!\n");
      else {
        dbus_message_iter_get_basic(&args, (void*)buf);
        printf("Got Signal with value %s\n", buf);
      }
    }

    // free the message
    dbus_message_unref(msg);
  }
}

void MessageService::exposeMethod() {
  // loop, testing for new messages
  while (true) {
    // non blocking read of the next available message
    dbus_connection_read_write(getConnection(), 0);
    DBusMessage *msg = dbus_connection_pop_message(getConnection());

    // loop again if we haven't got a message
    if (NULL == msg) {
      sleep(1);
      continue;
    }

    // check this is a method call for the right interface and method
    if (dbus_message_is_method_call(msg, "test.method.Type", "Method"))
      reply_to_method_call(msg, getConnection());

    // free the message
    dbus_message_unref (msg);
  }
}

void MessageService::reply_to_method_call(DBusMessage* msg, DBusConnection* conn)
{
   DBusMessage* reply;
   DBusMessageIter args;
   bool stat = true;
   dbus_uint32_t level = 21614;
   dbus_uint32_t serial = 0;
   char* param = "";

   // read the arguments
   if (!dbus_message_iter_init(msg, &args))
      fprintf(stderr, "Message has no arguments!\n");
   else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
      fprintf(stderr, "Argument is not string!\n");
   else
      dbus_message_iter_get_basic(&args, &param);
   printf("Method called with %s\n", param);

   // create a reply from the message
   reply = dbus_message_new_method_return(msg);

   // add the arguments to the reply
   dbus_message_iter_init_append(reply, &args);
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &stat)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &level)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }

   // send the reply && flush the connection
   if (!dbus_connection_send(conn, reply, &serial)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }
   dbus_connection_flush(getConnection());

   // free the reply
   dbus_message_unref(reply);
}

void MessageService::cleanupMessageService() {
  dbus_connection_close(__conn);
}
