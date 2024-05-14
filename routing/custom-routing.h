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


// Routing functions

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
 * @brief Send a control packet to a destination
 * 
 * @param node_type type of the node
 * @param dest destination address
 * @param response_type type of the response
 */
void control_packet_send(uint8_t node_type, const linkaddr_t* dest, uint8_t response_type); 

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