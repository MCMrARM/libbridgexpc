#ifndef BRIDGE_CONNECTION_LIBEVENT_H
#define BRIDGE_CONNECTION_LIBEVENT_H

#include "connection.h"

#include <event2/bufferevent.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bridge_xpc_libevent_connection {
    struct bridge_xpc_connection conn;
    struct bufferevent *bev;
    struct evbuffer *out;
};

struct bridge_xpc_libevent_connection *bridge_xpc_libevent_connection_create(struct event_base *evbase);

#ifdef __cplusplus
}
#endif

#endif //BRIDGE_CONNECTION_LIBEVENT_H
