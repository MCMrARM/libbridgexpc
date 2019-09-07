#ifndef BRIDGE_CONNECTION_H
#define BRIDGE_CONNECTION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <plist/plist.h>
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bridge_xpc_connection;
struct bridge_xpc_connection_callbacks {
    void (*connected)(struct bridge_xpc_connection *conn, void *userdata);
    void (*message_received)(struct bridge_xpc_connection *conn, plist_t data, void *userdata);
};
struct bridge_xpc_connection_transport_callbacks {
    int (*write)(struct bridge_xpc_connection *conn, const uint8_t *header_data, size_t header_length,
            const uint8_t *data, size_t len, bool transfer_data_ownership);
};
struct bridge_xpc_connection {
    struct bridge_xpc_connection_callbacks cbs;
    void *userdata;
    struct bridge_xpc_connection_transport_callbacks transport_cbs;
    void *transport_data;

    size_t recv_header_pos;
    uint8_t recv_header_data[sizeof(struct bridge_xpc_header)];
    uint8_t *recv_data;
    size_t recv_data_pos;
};


void bridge_xpc_connection_init(struct bridge_xpc_connection *conn,
        struct bridge_xpc_connection_transport_callbacks *transport_cbs, void *transport_data);

void bridge_xpc_connection_set_callbacks(struct bridge_xpc_connection *conn,
        struct bridge_xpc_connection_callbacks *cbs, void *userdata);

void bridge_xpc_connection_notify_connected(struct bridge_xpc_connection *conn);

void bridge_xpc_connection_process_recv(struct bridge_xpc_connection *conn, const uint8_t *data, size_t len);

int bridge_xpc_connection_send_raw(struct bridge_xpc_connection *conn, int type, const uint8_t *data, size_t len,
        bool transfer_data_ownership);

int bridge_xpc_connection_send(struct bridge_xpc_connection *conn, plist_t data);

#ifdef __cplusplus
}
#endif

#endif //BRIDGE_CONNECTION_H
