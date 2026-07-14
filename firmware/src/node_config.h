#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#ifndef NODE_ID
#define NODE_ID  1
#endif

#define NODE_ID_MIN          1
#define NODE_ID_MAX          100
#define NODE_ID_PHONE        0x00
#define NODE_ID_BROADCAST    0xFF

#define CLUSTER_SIZE         10
#define CLUSTER_ID           ((NODE_ID - 1) / CLUSTER_SIZE)
#define GATEWAY_NODE_ID      (CLUSTER_ID * CLUSTER_SIZE + 1)
#define IS_GATEWAY           (NODE_ID == GATEWAY_NODE_ID)
#define NEXT_GATEWAY_ID      (GATEWAY_NODE_ID + CLUSTER_SIZE)
#define PREV_GATEWAY_ID      (GATEWAY_NODE_ID - CLUSTER_SIZE)

#define GET_CLUSTER(nid)     (((nid) - 1) / CLUSTER_SIZE)
#define GET_GATEWAY(nid)     (GET_CLUSTER(nid) * CLUSTER_SIZE + 1)
#define IS_NODE_GATEWAY(nid) ((nid) == GET_GATEWAY(nid))

#endif
