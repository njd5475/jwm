
#ifndef CLIENT_LISTENER_H
#define CLIENT_LISTENER_H

class Client;

class ClientListener {
  public:
    ClientListener() {}
    virtual ~ClientListener() {}

    virtual void newClient(Client* node) = 0;
};

#endif