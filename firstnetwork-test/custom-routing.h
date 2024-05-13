#define DATA 1
#define CONTROL 0

#define GATEWAY 1
#define NODE 0

#define SETUP 0b0
#define RESPONSE 0b1

/* Structure for parent nodes
    - parent_addr: address of the parent node
    - rssi: signal strength of the parent node
    - type: type of the parent node
*/
typedef struct {
    linkaddr_t* parent_addr;
    signed char rssi;
    uint8_t type;
} parent_t;

/* Structure for control packets
    - type: two bit field to indicate the type of the packet
*/
typedef struct {
    uint8_t type;
    uint8_t data;
} control_packet_t;