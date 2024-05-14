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

#define GATEWAY 1
#define NODE 0

#define SETUP 0b0
#define RESPONSE 0b1


/* Structure for parent nodes
    - parent_addr: address of the parent node
    - rssi: signal strength of the parent node
    - type: type of the parent node /!\ UNUSED
            it does not work with it (for some reason)
*/
typedef struct {
    linkaddr_t* parent_addr;
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
    uint8_t data;
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
    data_header_t* header;
    char* topic;
    char* data;
} data_packet_t;

static parent_t* parent;
static uint8_t setup = 0;
static uint8_t type_parent = 0;

/* Routing functions */ 
/*---------------------------------------------------------------------------*/

/**
 * @brief Get the parent address
 * 
 * @return parent_t* parent address
 */
parent_t* get_parent();

/**
 * @brief Set the parent address
 * 
 * @param parent_addr parent address
 * @param rssi signal strength
 * @param type type of the parent node
 */
void set_parent(const linkaddr_t* parent_addr, uint8_t type, signed char rssi);

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
void init_gateway();

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
 * @brief Forward a data packet to the parent node
 * 
 * @param data data of the packet
 * @param len length of the data
 */
void forward_data_packet(const void *data, uint16_t len);

/**
 * @brief Send a data packet to the parent node
 * 
 * @param len_topic length of the topic
 * @param len_data length of the data
 * @param topic topic of the data
 * @param data data of the packet
 */
void send_data_packet(uint16_t len_topic, uint16_t len_data, char* topic, char* data);

/**
 * @brief Build the control header of a packet
 * 
 * @param control_header control header pointer to store the header
 * @param node_type type of the node
 * @param response_type type of the response
 */
void build_control_header(control_header_t* control_header, uint8_t node_type, uint8_t response_type);

/**
 * @brief Pack the control packet
 * 
 * @param control_packet control packet pointer
 * @param data data of the packet
 */
void packing_control_packet(control_packet_t* control_packet, uint8_t* data);

/**
 * @brief Process the control header of a packet
 * 
 * @param data packet data
 * @param len packet length
 * @param control_header control header pointer to store the header
 */
void process_control_header(const void *data, uint16_t len, control_header_t* control_header);

/**
 * @brief Send a control packet to a destination
 * 
 * @param node_type type of the node
 * @param dest destination address
 * @param response_type type of the response
 */
void control_packet_send(uint8_t node_type, const linkaddr_t* dest, uint8_t response_type, uint16_t len_of_data); 

/**
 * @brief Check if the parent node is better than the current one and update it,
 *        only for the nodes
 * 
 * @param src source address
 * @param node_type type of the node
 * @param parent_addr current parent node
 * @param setup setup flag
 */
void check_parent_node(const linkaddr_t* src, uint8_t node_type, parent_t* parent_addr);

/**
 * @brief Process a packet and determine its type, if it is a control
 *       packet, it will process it
 * @param data packet data
 * @param len packet length
 * @param src source address
 * @param dest destination address
 * @param packet_type packet type pointer to store the type of the packet
*/
void process_node_packet(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest, uint8_t* packet_type);

/**
 * @brief Process a packet and determine its type, if it is a control
 *       packet, it will process it
 * @param data packet data
 * @param len packet length
 * @param src source address
 * @param dest destination address
 * @param packet_type packet type pointer to store the type of the packet
*/
void process_gateway_packet(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest, uint8_t* packet_type);

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


#endif /* CUSTOM_ROUTING_H */