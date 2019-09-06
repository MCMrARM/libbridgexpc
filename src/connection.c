#include <bridgexpc/connection.h>
#include <sys/param.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <malloc.h>

static int _bridge_xpc_send_hello(struct bridge_xpc_connection *conn);
static void _bridge_xpc_connection_process_recv_msg(struct bridge_xpc_connection *conn,
        struct bridge_xpc_header *header, const uint8_t *data);

void bridge_xpc_connection_init(struct bridge_xpc_connection *conn, struct bridge_xpc_connection_callbacks *cbs,
        void *transport_data) {
    conn->cbs = *cbs;
    conn->transport_data = transport_data;
    conn->recv_header_pos = 0;
    conn->recv_data_pos = 0;
    conn->recv_data = NULL;
}

void bridge_xpc_connection_notify_connected(struct bridge_xpc_connection *conn) {
    _bridge_xpc_send_hello(conn);
}

void bridge_xpc_connection_process_recv(struct bridge_xpc_connection *conn, const uint8_t *data, size_t len) {
    size_t tlen;
    struct bridge_xpc_header *header = (struct bridge_xpc_header *) conn->recv_header_data;
    while (len > 0) {
        // Read header
        if (conn->recv_header_pos < sizeof(struct bridge_xpc_header)) {
            if (conn->recv_header_pos == 0 && len >= sizeof(struct bridge_xpc_header)) { // 99% of cases
                *((struct bridge_xpc_header *) conn->recv_header_data) = *((struct bridge_xpc_header *) data);
                conn->recv_header_pos = sizeof(struct bridge_xpc_header);
                data += sizeof(struct bridge_xpc_header);
                len -= sizeof(struct bridge_xpc_header);
            } else {
                tlen = MIN(sizeof(struct bridge_xpc_header) - conn->recv_header_pos, len);
                memcpy(&conn->recv_header_data[conn->recv_header_pos], data, tlen);
                conn->recv_header_pos += tlen;
                data += tlen;
                len -= tlen;
                if (conn->recv_header_pos < sizeof(struct bridge_xpc_header))
                    break; // Incomplete, not enough data to fill the remaining space
            }

            // Validate the header
            if (header->magic != BRIDGE_XPC_MAGIC) {
                fprintf(stderr, "bridgexpc: recv bad magic %" PRIx32, header->magic);
                return;
            }
            if (header->version != BRIDGE_XPC_VERSION) {
                fprintf(stderr, "bridgexpc: recv bad version %" PRIx32, header->version);
                return;
            }
            if (header->length > 0x10000) {
                fprintf(stderr, "bridgexpc: recv message too long: %" PRIu64 "\n", header->length);
                return;
            }
            // printf("bridgexpc: starting data read of %" PRIu64 "\n", header->length);
        }

        // Read the data
        if (conn->recv_data_pos == 0 && len >= header->length) {
            // No need to copy the data, it's all in the buffer
            assert(conn->recv_data == NULL);
            _bridge_xpc_connection_process_recv_msg(conn, header, data);
            data += header->length;
            len -= header->length;
        } else if (len > 0) {
            if (!conn->recv_data)
                conn->recv_data = malloc(header->length);
            tlen = MIN(len, header->length - conn->recv_data_pos);
            memcpy(&conn->recv_data[conn->recv_data_pos], data, tlen);
            data += tlen;
            len -= tlen;
            conn->recv_data_pos += tlen;

            if (conn->recv_data_pos != header->length)
                break; // Incomplete

            _bridge_xpc_connection_process_recv_msg(conn, header, conn->recv_data);
        }

        // Message complete, prepare for next message
        conn->recv_header_pos = 0;
        if (conn->recv_data)
            free(conn->recv_data);
        conn->recv_data_pos = 0;
    }
}

int bridge_xpc_connection_send_raw(struct bridge_xpc_connection *conn, int type, const uint8_t *data, size_t len,
        bool transfer_data_ownership) {
    struct bridge_xpc_header header;
    header.magic = BRIDGE_XPC_MAGIC;
    header.version = BRIDGE_XPC_VERSION;
    header.type = type;
    header.length = len;
    return conn->cbs.write(conn, (uint8_t *) &header, sizeof(header), data, len, transfer_data_ownership);
}

int bridge_xpc_connection_send(struct bridge_xpc_connection *conn, plist_t data) {
    uint32_t len;
    char *bin_data;
    plist_to_xml(data, &bin_data, &len);
    printf("bridgexpc: Send plist: %s\n", bin_data);
    free(bin_data);

    plist_to_bin(data, &bin_data, &len);
    return bridge_xpc_connection_send_raw(conn, BRIDGE_XPC_DATA, (uint8_t *) bin_data, len, true);
}

static int _bridge_xpc_send_hello(struct bridge_xpc_connection *conn) {
    // The remote doesn't parse the JSON anyways so there's no reason for us to try harder than this
    printf("bridgexpc: Send hello: {}\n");
    return bridge_xpc_connection_send_raw(conn, BRIDGE_XPC_HELLO, (const uint8_t *) "{}", 2, false);
}

static void _bridge_xpc_connection_process_recv_msg(struct bridge_xpc_connection *conn,
        struct bridge_xpc_header *header, const uint8_t *data) {
    if (header->type == BRIDGE_XPC_HELLO) {
        printf("bridgexpc: Got remote hello: %.*s\n", (int) header->length, data);
    } else if (header->type == BRIDGE_XPC_DATA) {
        plist_t pdata;
        uint32_t len;
        char *str_data;
        plist_from_bin((const char *) data, header->length, &pdata);

        plist_to_xml(pdata, &str_data, &len);
        printf("bridgexpc: Got remote plist: %s\n", str_data);
        free(str_data);
    }
}