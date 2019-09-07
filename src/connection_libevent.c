#include <bridgexpc/connection_libevent.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <netinet/tcp.h>
#include <malloc.h>

static void _bridge_xpc_ev_read_callback(struct bufferevent *bev, void *ptr) {
    struct bridge_xpc_libevent_connection *evconn = ptr;
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t datalen;
    unsigned char *data;

    while (true) {
        datalen = evbuffer_get_contiguous_space(input);
        if (datalen == 0)
            break;

        data = evbuffer_pullup(input, datalen);
        bridge_xpc_connection_process_recv(&evconn->conn, data, datalen);
        if (evbuffer_drain(input, datalen))
            fprintf(stderr, "bridge-xpc: evbuffer_drain failed\n");
    }
}

static void _bridge_xpc_ev_event_callback(struct bufferevent *bev, short events, void *ptr) {
    struct bridge_xpc_libevent_connection *evconn = ptr;
    if (events & BEV_EVENT_CONNECTED) {
        int fd = bufferevent_getfd(bev);
        int val;

        val = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val));

        bridge_xpc_connection_notify_connected(&evconn->conn);
    } else {
        // TODO: disconnect
    }
}

static void _bridge_xpc_evbuf_just_free(const void *data, size_t datalen, void *extra) {
    free((void *) data);
}

static int _bridge_xpc_libevent_connection_write(struct bridge_xpc_connection *conn,
        const uint8_t *header_data, size_t header_length,
        const uint8_t *data, size_t len, bool transfer_data_ownership) {
    int result;
    struct bridge_xpc_libevent_connection *evconn = conn->transport_data;
    if ((result = evbuffer_add(evconn->out, header_data, header_length)))
        return result;
    if (transfer_data_ownership) {
        if ((result = evbuffer_add_reference(evconn->out, data, len, _bridge_xpc_evbuf_just_free, NULL)))
            return result;
    } else {
        if ((result = evbuffer_add(evconn->out, data, len)))
            return result;
    }
    return 0;
}

struct bridge_xpc_libevent_connection *bridge_xpc_libevent_connection_create(struct event_base *evbase) {
    struct bridge_xpc_libevent_connection *evconn = malloc(sizeof(struct bridge_xpc_libevent_connection));
    struct bridge_xpc_connection_transport_callbacks tcbs;
    tcbs.write = _bridge_xpc_libevent_connection_write;
    bridge_xpc_connection_init(&evconn->conn, &tcbs, evconn);
    evconn->bev = bufferevent_socket_new(evbase, -1, BEV_OPT_DEFER_CALLBACKS | BEV_OPT_CLOSE_ON_FREE);
    bufferevent_enable(evconn->bev, EV_READ | EV_WRITE);
    bufferevent_setcb(evconn->bev, _bridge_xpc_ev_read_callback, NULL, _bridge_xpc_ev_event_callback,
                      evconn);
    evconn->out = bufferevent_get_output(evconn->bev);
    return evconn;
}

