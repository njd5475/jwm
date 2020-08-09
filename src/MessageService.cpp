/*
 * MessageService.cpp
 *
 *  Created on: Jul 26, 2020
 *      Author: nick
 */

#include "jwm.h"
#include "MessageService.h"
#include "command.h"
#include "misc.h"

#include <string.h>
#include <string>
#include <stdio.h>
// Docs: https://dbus.freedesktop.org/doc/api/html/
//#include <dbus/dbus.h>

MessageService::MessageService() {
  // TODO Auto-generated constructor stub

}

MessageService::~MessageService() {
  // TODO Auto-generated destructor stub
}

long MessageService::getShutdownTime() {
  std::string timeStr = getShutdownProperty();
  long time = atol(timeStr.c_str());
  return time;
}

std::string MessageService::getShutdownProperty() {
  char *property = getProperty("org.freedesktop.login1",
      "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
      "ScheduledShutdown");

  using namespace std;

  vector<string> params = splitStr(property, " ");

  if(params.size() >= 2) {
    return params[1];
  }

  return "";
}

char* MessageService::getProperty(const char *service, const char *object,
    const char *interface, const char *property) {
  char buf[256];
  sprintf(buf, "busctl get-property %s %s %s %s", service, object, interface,
      property);

  return Commands::ReadFromProcess(buf, 1000);
}

