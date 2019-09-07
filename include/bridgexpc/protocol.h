#ifndef BRDIGE_PROTOCOL_H
#define BRDIGE_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#define BRIDGE_XPC_MAGIC 0xB892
#define BRIDGE_XPC_VERSION 1

struct bridge_xpc_header {
    uint16_t magic;
    uint16_t version;
    uint32_t type;
    uint64_t length;
};

enum bridge_xpc_message_type {
    BRIDGE_XPC_HELLO = 1,
    BRIDGE_XPC_DATA = 2
};

#ifdef __cplusplus
}
#endif

#endif //BRDIGE_PROTOCOL_H
