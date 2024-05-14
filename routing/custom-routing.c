#include "custom-routing.h"

#define LOG_MODULE "Routing"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */

/*---------------------------------------------------------------------------*/
parent_t* get_parent() {
  return parent;
}

void set_parent(const linkaddr_t* parent_addr, uint8_t type, signed char rssi) {
  linkaddr_copy(parent->parent_addr, parent_addr);
  type_parent = type;
  parent->rssi = rssi;

  LOG_INFO("Parent address: ");
  LOG_INFO_LLADDR(parent_addr);
  LOG_INFO_("\n");
  LOG_INFO("Parent type: %u\n", type);
  LOG_INFO("Parent rssi: %d\n", rssi);
}

uint8_t not_setup() {
  LOG_INFO("Setup: %u\n", setup);
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
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void build_data_header(data_packet_t* data_packet, uint16_t len_topic, uint16_t len_data, char* topic, char* data) {
  data_header_t* header = malloc(sizeof(data_header_t));
  header->type = DATA;
  header->len_topic = len_topic & 0x7FFF;
  header->len_data = len_data;

  data_packet->header = header;
  data_packet->topic = topic;
  data_packet->data = data;
}

void packing_data_packet(data_packet_t* data_packet, uint8_t* data) {
  data[0] = 0;
  data[0] = data_packet->header->type << 7;

  /* Setting the first 2 bytes to be the length of topic wihtout erasing the first bit */
  data[0] |= (data_packet->header->len_topic & 0x7F) >> 8;
  data[1] = data_packet->header->len_topic;
  data[2] = data_packet->header->len_data >> 8;
  data[3] = data_packet->header->len_data;

  /* Setting the rest of the data to be the data_packet->topic and data_packet->data pointers */
  memcpy(data + 4, data_packet->topic, data_packet->header->len_topic);
  memcpy(data + 4 + data_packet->header->len_topic, data_packet->data, data_packet->header->len_data);
}

void process_data_packet(const uint8_t *input_data, uint16_t len, data_packet_t* data_packet) {
  if (len == 0) {
    return;
  }

  data_header_t* header = malloc(sizeof(data_header_t));

  /* Extracting the first byte of the data */
  header->type = input_data[0] >> 7;

  /* Extracting the first 2 bytes of the data */
  header->len_topic = (input_data[0] & 0x7F) << 8;
  header->len_topic |= input_data[1];
  header->len_data = input_data[2] << 8;
  header->len_data |= input_data[3];

  /* Extracting the topic and data */
  char* data_topic = malloc(sizeof(char)*header->len_topic + 1);
  char* data = malloc(sizeof(char)*header->len_data + 1);
  memcpy(data_topic, input_data + 4, header->len_topic);
  memcpy(data, input_data + 4 + header->len_topic, header->len_data);

  data_topic[header->len_topic] = '\0';
  data[header->len_data] = '\0';

  data_packet->header = header;
  data_packet->topic = data_topic;
  data_packet->data = data;

  print_data_packet(data_packet);
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void send_data_packet(uint16_t len_topic, uint16_t len_data, char* topic, char* input_data) {
  data_packet_t* data_packet = malloc(sizeof(data_packet_t));
  build_data_header(data_packet, len_topic, len_data, topic, input_data);

  LOG_INFO("Sending data packet\n");
  print_data_packet(data_packet);

  uint8_t* data = malloc(sizeof(uint8_t)*(len_topic + len_data + 4));
  packing_data_packet(data_packet, data);

  nullnet_buf = (uint8_t *)data;
  nullnet_len = 4 + len_topic + len_data;

  const linkaddr_t* dest = parent->parent_addr;
  LOG_INFO("Sending data packet to: ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO_("\n");
  NETSTACK_NETWORK.output(dest);
}

void forward_data_packet(const void *data, uint16_t len) {
  LOG_INFO("Forwarding data packet\n");
  nullnet_buf = (uint8_t *)data;
  nullnet_len = len;

  const linkaddr_t* dest = parent->parent_addr;
  LOG_INFO("Forwarding data packet to: ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO_("\n");
  NETSTACK_NETWORK.output(dest);
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
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

  LOG_INFO("Sending control packet to: ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO_("\n");

  NETSTACK_NETWORK.output(dest);
}

void check_parent_node(const linkaddr_t* src, uint8_t node_type, parent_t* parent_addr) {
  /* Create new possible parent */
  signed char rssi = cc2420_last_rssi;
  LOG_INFO("type_parent: %u\n", type_parent);

  if (parent_addr == NULL) {
    setup = 1;
    parent->parent_addr = malloc(sizeof(linkaddr_t));
    set_parent(src, node_type, rssi);
    LOG_INFO("First parent setup\n");
    return;
  }

  /* If the new parent is better than the current one */
  if (parent_addr->parent_addr < src) {
    set_parent(src, node_type, rssi);
    LOG_INFO("Better parent found\n");
    return;
  }

  /* If the new parent is the same as the current one */
  if (
      type_parent == node_type &&
      parent_addr->rssi < rssi
      ) 
  {
    set_parent(src, node_type, rssi);
    LOG_INFO("Better parent found\n");
    return;
  }
  LOG_INFO("Parent poopoo\n");
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void process_node_packet(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest, uint8_t* packet_type) {
  if (len == 0) {
    LOG_INFO("Empty packet\n");
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
      check_parent_node(src, header->node_type, parent);
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

    check_parent_node(src, header->node_type, parent);
    return;
  }

  if (*packet_type == DATA) {
    LOG_INFO("Forwarding data packet\n");
    forward_data_packet(data, len);
    return;
  }
}

void process_gateway_packet(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest, uint8_t* packet_type) {
  LOG_INFO("Processing gateway packet\n");
  if (len == 0) {
    LOG_INFO("Empty packet\n");
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
/*---------------------------------------------------------------------------*/

void print_data_packet(data_packet_t* data_packet) {
  LOG_INFO("Data packet\n");
  LOG_INFO("Type: %u\n", data_packet->header->type);
  LOG_INFO("Length of topic: %u\n", data_packet->header->len_topic);
  LOG_INFO("Length of data: %u\n", data_packet->header->len_data);
  LOG_INFO("Topic: %s\n", data_packet->topic);
  LOG_INFO("Data: %s\n", data_packet->data);
}

void print_control_packet(control_packet_t* control_packet) {
  LOG_INFO("Control packet\n");
  LOG_INFO("Type: %u\n", control_packet->header->type);
  LOG_INFO("Node type: %u\n", control_packet->header->node_type);
  LOG_INFO("Response type: %u\n", control_packet->header->response_type);
  LOG_INFO("Data: %u\n", control_packet->data);
}