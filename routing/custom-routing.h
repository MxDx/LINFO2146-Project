#ifndef CUSTOM_ROUTING_H

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h>
#include "dev/cc2420.h"
#include "sys/log.h"

#define DATA 1
#define CONTROL 0

#define GATEWAY 0b10
#define SUB_GATEWAY 0b01
#define NODE 0b00

#define SETUP 0b00
#define RESPONSE 0b01
#define SETUP_ACK 0b10

typedef struct {
    linkaddr_t addr;
    linkaddr_t from;
    uint8_t type;
} child_t;

/* Structure for parent nodes
    - parent_addr: address of the parent node
    - rssi: signal strength of the parent node
    - type: type of the parent node /!\ UNUSED
            it does not work with it (for some reason)
*/
typedef struct {
    linkaddr_t parent_addr;
    signed char rssi;
    uint8_t type;
} parent_t;


/* Structure for control headers
    - type: type of the packet
    - node_type: type of the node
    - response_type: type of the response
*/
typedef struct {
    uint8_t type;
    uint8_t node_type;
    uint8_t response_type;
} control_header_t;


/* Structure for control packets
    - header: control header
    - data: data of the packet
*/
typedef struct {
    control_header_t* header;
    void* data;
} control_packet_t;

/* Structure for data headers
    - len_topic: length of the topic
    - len_data: length of the data
*/
typedef struct {
    uint8_t type;
    uint16_t len_topic;
    uint16_t len_data;
} data_header_t;

/* Structure for data packets
    - header: control header
    - topic: topic of the data
    - data: data of the packet
*/
typedef struct {
    data_header_t header;
    char* topic;
    char* data;
} data_packet_t;

typedef struct {
    linkaddr_t src;
    linkaddr_t dest;
    void* packet;
} packet_t;


// static parent_t* parent;
static uint8_t setup = 0;
// Maybe not needed
static uint8_t type_parent = 0;
static linkaddr_t null_addr =         {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
static uint8_t node_type = 0;


/* Routing functions */ 
/*---------------------------------------------------------------------------*/


/**
 * @brief Set the parent address
 * 
 * @param parent_addr parent address
 * @param rssi signal strength
 * @param type type of the parent node
 * @param parent parent node
 */
void set_parent(const linkaddr_t* parent_addr, uint8_t type, signed char rssi, parent_t* parent);

void set_child(const linkaddr_t* src, uint8_t* data);

/**
 * @brief Send child to parent
 * 
 * @param child child node
 * @param node_type type of the node
 * @param parent parent node
 */
void send_child(child_t child, uint8_t node_type, parent_t* parent);

/**
 * @brief Check if the node is not setup
 * 
 * @return uint8_t 1 if the node is not setup, 0 otherwise
 */
uint8_t not_setup(); 

/**
 * @brief Initialize the node
 * 
 */
void init_node();

/**
 * @brief Initialize the gateway
 * 
 */
void init_sub_gateway();

/**
 * @brief Initialize the gateway
 * 
 */
void init_gateway();

/**
 * @brief Send a packet to a destination
 * 
 * @param output output packet
 * @param src source address
 * @param dest destination address
 * @param packet packet to send
 * @param len_packet length of the packet
 */
void packing_packet(uint8_t* output, linkaddr_t* src, linkaddr_t* dest, uint8_t* packet, uint16_t len_packet);

/**
 * @brief Process a packet and determine its type
 * 
 * @param input_data packet data
 * @param len packet length
 * @param packet packet pointer to store the type of the packet
 */
void process_packet(const uint8_t* input_data, uint16_t len, packet_t* packet);

/**
 * @brief Build the data header of a packet
 * 
 * @param data_packet data packet pointer to store the header
 * @param len_topic length of the topic
 * @param len_data length of the data
 * @param topic topic of the data
 * @param data data of the packet
 */
void build_data_header(data_packet_t* data_packet, uint16_t len_topic, uint16_t len_data, char* topic, char* data);

/**
 * @brief Process the data header of a packet
 * 
 * @param input_data packet data
 * @param len packet length
 * @param data_packet data packet pointer to store the header
 */
void process_data_packet(const uint8_t *input_data, uint16_t len, data_packet_t* data_packet);

/**
 * @brief Send a data packet to the parent node
 * 
 * @param len_topic length of the topic
 * @param len_data length of the data
 * @param topic topic of the data
 * @param data data of the packet
 * @param parent parent node
 */
void send_data_packet(uint16_t len_topic, uint16_t len_data, char* topic, char* data, parent_t* parent);

/**
 * @brief Forward a data packet to the parent node
 * 
 * @param data data of the packet
 * @param len length of the data
 * @param parent parent node
 */
void forward_data_packet(const void *data, uint16_t len, parent_t* parent);

/**
 * @brief Build the control header of a packet
 * 
 * @param control_header control header pointer to store the header
 * @param node_type type of the node
 * @param response_type type of the response
 * @param dest destination address
 */
void build_control_header(control_header_t* control_header, uint8_t node_type, uint8_t response_type, linkaddr_t* dest);

/**
 * @brief Pack the control packet
 * 
 * @param control_packet control packet pointer
 * @param data data of the packet
 * @param len_of_data length of the data
 */
void packing_control_packet(control_packet_t* control_packet, uint8_t* data, uint16_t len_of_data);

/**
 * @brief Process the control header of a packet
 * 
 * @param data packet data
 * @param len packet length
 * @param control_header control header pointer to store the header
 */
void process_control_header(const uint8_t *data, uint16_t len, control_header_t* control_header);

/**
 * @brief Send a control packet to a destination
 * 
 * @param node_type type of the node
 * @param dest destination address
 * @param response_type type of the response
 * @param len_of_data length of the data
 */
void control_packet_send(uint8_t node_type, linkaddr_t* dest, uint8_t response_type, uint16_t len_of_data, void* control_data); 

/**
 * @brief Check if the parent node is better than the current one and update it,
 *        only for the nodes
 * 
 * @param src source address
 * @param node_type type of the node
 * @param parent_addr current parent node
 * @param setup setup flag
 * @param parent parent node
 */
void check_parent_node(const linkaddr_t* src, uint8_t node_type, parent_t* parent);

/**
 * @brief Process a packet and determine its type, if it is a control
 *       packet, it will process it
 * @param data packet data
 * @param len packet length
 * @param src source address
 * @param dest destination address
 * @param packet_type packet type pointer to store the type of the packet
 * @param parent parent node
*/
void process_node_packet(const void *data, uint16_t len, linkaddr_t *src, linkaddr_t *dest, uint8_t* packet_type, parent_t* parent);

/**
 * @brief Process a packet and determine its type, if it is a control
 *       packet, it will process it
 * @param data packet data
 * @param len packet length
 * @param src source address
 * @param dest destination address
 * @param packet_type packet type pointer to store the type of the packet
 * @param parent parent node
*/
void process_sub_gateway_packet(const uint8_t* data, uint16_t len, linkaddr_t *src, linkaddr_t *dest, uint8_t* packet_type, parent_t* parent);

/**
 * @brief Process a packet and determine its type, if it is a control
 *       packet, it will process it
 * @param data packet data
 * @param len packet length
 * @param src source address
 * @param dest destination address
 * @param packet_type packet type pointer to store the type of the packet
*/
void process_gateway_packet(const void *data, uint16_t len, linkaddr_t *src, linkaddr_t *dest, uint8_t* packet_type);

/*---------------------------------------------------------------------------*/
/* Debug functions */

/**
 * @brief Print the control header of a packet
 * 
 * @param control_header control header to print
 */
void print_data_packet(data_packet_t* data_packet); 

/**
 * @brief Print the control header of a packet
 * 
 * @param control_header control header to print
 */
void print_control_header(control_header_t* control_header);

/**
 * @brief Print the children of a node
 */
void print_children();


#endif /* CUSTOM_ROUTING_H */