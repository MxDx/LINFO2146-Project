#include "custom-routing.h"

#define LOG_MODULE "Routing"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
uint8_t setup = 0;
static parent_t* parent_addr;

uint8_t not_setup() {
  return setup == 0;
}

void init_node() {
  LOG_INFO("Sending setup control packet (node)\n");
  LOG_INFO("To: ");
  LOG_INFO_LLADDR(NULL);
  LOG_INFO_("\n");
  control_packet_send(NODE, NULL, SETUP, 0);
}

void init_gateway() {
  LOG_INFO("Sending setup control packet (gateway)\n");
  LOG_INFO("To: ");
  LOG_INFO_LLADDR(NULL);
  LOG_INFO_("\n");
  control_packet_send(GATEWAY, NULL, SETUP, 0);
}

void build_control_header(control_header_t* control_header, uint8_t node_type, uint8_t response_type) {
  control_header->type = CONTROL;
  control_header->node_type = node_type;
  control_header->response_type = response_type;
}

void packing_control_packet(control_packet_t* control_packet, uint8_t* data) {
  data[0] = control_packet->header->type << 7;
  data[0] |= control_packet->header->node_type << 6; 
  data[0] |= control_packet->header->response_type << 4;

  /* Setting the rest of the data to be the control_packet->data pointer */
  data[1] = control_packet->data;
}

void process_control_header(const void *data, uint16_t len, control_header_t* control_header) {
  if (len == 0) {
    return;
  }

  uint8_t header = ((uint8_t *)data)[0];
  control_header->type = header >> 7;
  control_header->node_type = (header >> 6) & 0b1;
  control_header->response_type = (header >> 4) & 0b1;
}

void control_packet_send(uint8_t node_type, const linkaddr_t* dest, uint8_t response_type, uint16_t len_of_data) {
  control_packet_t* control_packet = malloc(sizeof(control_packet_t));
  control_header_t* header = malloc(sizeof(control_header_t));
  build_control_header(header, node_type, response_type);
  control_packet->header = header;
  LOG_INFO("Sending control packet\n");
  LOG_INFO("Node type: %u\n", header->node_type);
  LOG_INFO("Response type: %u\n", header->response_type);

  uint8_t* data = malloc(sizeof(uint8_t)*(len_of_data + 1));  
  packing_control_packet(control_packet, data);
  nullnet_buf = (uint8_t *)data;
  nullnet_len = sizeof(data);

  NETSTACK_NETWORK.output(dest);
}

void check_parent_node(const linkaddr_t* src, uint8_t node_type, parent_t* parent_addr) {
  /* Create new possible parent */
  parent_t* new_parent = malloc(sizeof(parent_t));
  linkaddr_t src_addr;
  memcpy(&src_addr, src, sizeof(linkaddr_t));
  new_parent->parent_addr = &src_addr;
  new_parent->rssi = cc2420_last_rssi;
  new_parent->type = node_type;
  
  LOG_INFO("Received setup control packet\n");
  LOG_INFO("Parent address: ");
  LOG_INFO_LLADDR(new_parent->parent_addr);
  LOG_INFO_("\n");
  LOG_INFO("RSSI: %d\n", new_parent->rssi);

  if (parent_addr == NULL) {
    setup = 1;
    parent_addr = new_parent;
    LOG_INFO("First parent setup\n");
    return;
  }

  /* If the new parent is better than the current one */
  if (parent_addr->parent_addr < new_parent->parent_addr) {
    parent_addr = new_parent;
    LOG_INFO("Better parent found\n");
    return;
  }

  /* If the new parent is the same as the current one */
  if (
      parent_addr->type == new_parent->type &&
      parent_addr->rssi < new_parent->rssi
      ) 
  {
    parent_addr = new_parent;
    LOG_INFO("Better parent found\n");
    return;
  }
  LOG_INFO("Parent poopoo\n");
}

void process_node_packet(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest, uint8_t* packet_type) {
  if (len == 0) {
    return;
  }

  uint8_t head = ((uint8_t *)data)[0];
  *packet_type = head >> 7;
  LOG_INFO("Packet type: %u\n", *packet_type);

  if (*packet_type == CONTROL) {
    LOG_INFO("Received control packet\n");
    control_header_t* header = malloc(sizeof(control_header_t));
    process_control_header(data, len, header);
    LOG_INFO("Node type: %u\n", header->node_type);
    LOG_INFO("Response type: %u\n", header->response_type);

    if (header->node_type == GATEWAY) {
      check_parent_node(src, header->node_type, parent_addr);
      return;
    }

    /* Sending back a control packet to the source
     * if setup is not done
    */
    if (!not_setup() && header->response_type == SETUP) {
      LOG_INFO("Sending response control packet\n");
      control_packet_send(NODE, src, RESPONSE, 0);
      return;
    }

    if (header->response_type == SETUP) {
      LOG_INFO("Ignoring packet, node not setup\n");
      return;
    }

    check_parent_node(src, header->node_type, parent_addr);
  }
}

void process_gateway_packet(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest, uint8_t* packet_type) {
  if (len == 0) {
    return;
  }

  uint8_t head = ((uint8_t *)data)[0];
  *packet_type = head >> 7;
  LOG_INFO("Packet type: %u\n", *packet_type);

  if (*packet_type == CONTROL) {
    LOG_INFO("Received control packet\n");
    LOG_INFO("Sending back a control packet\n");
    control_packet_send(GATEWAY, src, RESPONSE, 0);
  }
}
