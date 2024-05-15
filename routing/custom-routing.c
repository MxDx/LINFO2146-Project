#include "custom-routing.h"

#define LOG_MODULE "Routing"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
static child_t children[16];
static uint8_t children_count = 0;

/*---------------------------------------------------------------------------*/
void set_parent(const linkaddr_t* parent_addr, uint8_t type, signed char rssi, parent_t* parent, uint8_t node_type) {
  linkaddr_copy(&parent->parent_addr, parent_addr);
  type_parent = type;
  parent->type = type;
  parent->rssi = rssi;

  LOG_INFO("Parent address: ");
  LOG_INFO_LLADDR(&parent->parent_addr);
  LOG_INFO_("\n");
  LOG_INFO("Parent type: %u\n", type);
  LOG_INFO("Parent rssi: %d\n", rssi);

  // Sending setup ack to the parent
  LOG_INFO("Sending setup ack control packet\n");
  LOG_INFO("Node type: %u\n", node_type);
  uint8_t data[sizeof(linkaddr_t) + 1];
  data[0] = node_type;
  memcpy(data + 1, &linkaddr_node_addr, sizeof(linkaddr_t));
  control_packet_send(node_type, &parent->parent_addr, SETUP_ACK, sizeof(linkaddr_t) + 1, data);
}

void set_child(const linkaddr_t* src, uint8_t* data) {
  child_t new_child;
  new_child.addr = ((linkaddr_t*) (data + 2))[0];
  new_child.from = *src;
  new_child.type = data[1];
  children[children_count] = new_child;
  children_count++;
}

void get_children(const linkaddr_t* src, linkaddr_t* nexthop) {
  for (uint8_t i = 0; i < children_count; i++) {
    if (linkaddr_cmp(&children[i].addr, src)) {
      *nexthop = children[i].from;
      return;
    }
  }
}

void send_child(child_t child, uint8_t node_type, parent_t* parent) {
  uint8_t data[sizeof(linkaddr_t) + 1];
  data[0] = child.type;
  memcpy(data + 1, &child.addr, sizeof(linkaddr_t));
  control_packet_send(node_type, &parent->parent_addr, SETUP_ACK, sizeof(linkaddr_t) + 1, data);
}

uint8_t not_setup() {
  return setup == 0;
}

void init_node() {
  LOG_INFO("Sending setup control packet (node)\n");
  LOG_INFO("To: ");
  LOG_INFO_LLADDR(NULL);
  LOG_INFO_("\n");
  control_packet_send(NODE, NULL, SETUP, 0, NULL);
}

void init_sub_gateway() {
  LOG_INFO("Sending setup control packet (sub-gateway)\n");
  LOG_INFO("To: ");
  LOG_INFO_LLADDR(NULL);
  LOG_INFO_("\n");
  control_packet_send(SUB_GATEWAY, NULL, SETUP, 0, NULL);
}

void init_gateway() {
  LOG_INFO("Sending setup control packet (gateway)\n");
  LOG_INFO("To: ");
  LOG_INFO_LLADDR(NULL);
  LOG_INFO_("\n");
  control_packet_send(GATEWAY, NULL, SETUP, 0, NULL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
void packing_packet(uint8_t* output, linkaddr_t* src, linkaddr_t* dest, uint8_t* packet, uint16_t len_packet) {
  /* Adding the src and dest at the beginning of the packet */
  memcpy(output, src, sizeof(linkaddr_t));
  memcpy(output + sizeof(linkaddr_t), dest, sizeof(linkaddr_t));

  /* Adding the packet */
  memcpy(output + 2*sizeof(linkaddr_t), packet, len_packet);
}

void process_packet(const uint8_t* input_data, uint16_t len, packet_t* packet) {
  if (len == 0) {
    return;
  }

  /* Extracting the source and destination addresses */
  memcpy(&packet->src, input_data, sizeof(linkaddr_t));
  memcpy(&packet->dest, input_data + sizeof(linkaddr_t), sizeof(linkaddr_t));
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void build_data_header(data_packet_t* data_packet, uint16_t len_topic, uint16_t len_data, char* topic, char* data) {
  data_header_t header;
  header.type = DATA;
  LOG_INFO("Len topic: %u\n", len_topic);
  header.len_topic = len_topic & 0x7FFF;
  LOG_INFO("Len topic after: %u\n", header.len_topic);
  header.len_data = len_data;

  data_packet->header = header;
  data_packet->topic = topic;
  data_packet->data = data;
}

void packing_data_packet(data_packet_t* data_packet, uint8_t* data) {
  data[0] = 0;
  data[0] = data_packet->header.type << 7;

  /* Setting the first 2 bytes to be the length of topic wihtout erasing the first bit */
  data[0] |= (data_packet->header.len_topic & 0x7F) >> 8;
  data[1] = data_packet->header.len_topic;
  data[2] = data_packet->header.len_data >> 8;
  data[3] = data_packet->header.len_data;

  /* Setting the rest of the data to be the data_packet->topic and data_packet->data pointers */
  memcpy(data + 4, data_packet->topic, data_packet->header.len_topic);
  memcpy(data + 4 + data_packet->header.len_topic, data_packet->data, data_packet->header.len_data);
}

void process_data_packet(const uint8_t *input_data, uint16_t len, data_packet_t* data_packet) {
  if (len == 0) {
    return;
  }

  uint16_t start_of_data = sizeof(linkaddr_t)*2;

  data_header_t header;

  /* Extracting the first byte of the data */
  header.type = input_data[start_of_data] >> 7;

  /* Extracting the first 2 bytes of the data */
  header.len_topic = (input_data[start_of_data] & 0x7F) << 8;
  header.len_topic |= input_data[start_of_data + 1];
  header.len_data = input_data[start_of_data + 2] << 8;
  header.len_data |= input_data[start_of_data + 3];

  /* Extracting the topic and data */
  char* data_topic = malloc(sizeof(char)*header.len_topic + 1);
  char* data = malloc(sizeof(char)*header.len_data + 1);
  memcpy(data_topic, input_data + start_of_data + 4, header.len_topic);
  memcpy(data, input_data + start_of_data + 4 + header.len_topic, header.len_data);

  data_topic[header.len_topic] = '\0';
  data[header.len_data] = '\0';

  data_packet->header = header;
  data_packet->topic = data_topic;
  data_packet->data = data;

  print_data_packet(data_packet);
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void send_data_packet(uint16_t len_topic, uint16_t len_data, char* topic, char* input_data, parent_t* parent) {
  data_packet_t data_packet;
  build_data_header(&data_packet, len_topic, len_data, topic, input_data);
  
  LOG_INFO("Len topic: %u\n", data_packet.header.len_topic);
  LOG_INFO("Sending data packet\n");
  print_data_packet(&data_packet);

  // uint8_t* data = malloc(sizeof(uint8_t)*(len_topic + len_data + 4));
  uint8_t data[8];
  LOG_INFO("Before packing data packet\n");
  packing_data_packet(&data_packet, data);

  // uint8_t* output = malloc(sizeof(uint8_t)*(len_topic + len_data + 4) + 2*sizeof(linkaddr_t));
  uint8_t output[16];
  LOG_INFO("Before packing\n");
  packing_packet(output, &linkaddr_node_addr, &parent->parent_addr, data, len_topic + len_data + 4 + 2*sizeof(linkaddr_t));

  nullnet_buf = output;
  nullnet_len = 2*sizeof(linkaddr_t) + 4 + len_topic + len_data;

  const linkaddr_t dest = parent->parent_addr;
  LOG_INFO("Sending data packet to: ");
  LOG_INFO_LLADDR(&dest);
  LOG_INFO_("\n");
  NETSTACK_NETWORK.output(&dest);

  free(data_packet.topic);
  free(data_packet.data);

  // free(data);
  // free(output);
}

void forward_data_packet(const void *data, uint16_t len, parent_t* parent) {
  /* Changing the dest value to the address of the parent */
  ((linkaddr_t*)data)[1] = parent->parent_addr;
  
  nullnet_buf = (uint8_t *)data;
  nullnet_len = len;

  const linkaddr_t dest = parent->parent_addr;
  LOG_INFO("Forwarding data packet to: ");
  LOG_INFO_LLADDR(&dest);
  LOG_INFO_("\n");
  NETSTACK_NETWORK.output(&dest);
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void build_control_header(control_header_t* control_header, uint8_t node_type, uint8_t response_type, linkaddr_t* dest) {
  control_header->type = CONTROL;
  control_header->node_type = node_type;
  control_header->response_type = response_type;
}

void packing_control_packet(control_packet_t* control_packet, uint8_t* data, uint16_t len_of_data) {
  data[0] = control_packet->header->type << 7;
  data[0] |= control_packet->header->node_type << 5;
  data[0] |= control_packet->header->response_type << 3;

  /* Adding the data */
  memcpy(data + 1, control_packet->data, len_of_data);
}

void process_control_header(const uint8_t *data, uint16_t len, control_header_t* control_header) {
  if (len == 0) {
    LOG_INFO("Empty packet\n");
    return;
  }

  uint8_t header = ((uint8_t *)data)[0];
  control_header->type = header >> 7;
  control_header->node_type = (header >> 5) & 0b11;
  control_header->response_type = (header >> 3) & 0b11;
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void control_packet_send(uint8_t node_type, linkaddr_t* dest, uint8_t response_type, uint16_t len_of_data, void* control_data) {
  control_packet_t control_packet;
  control_header_t header;
  build_control_header(&header, node_type, response_type, dest);
  control_packet.header = &header;
  control_packet.data = control_data;

  LOG_INFO("Sending control packet\n");
  LOG_INFO("Node type: %u\n", header.node_type);
  LOG_INFO("Response type: %u\n", header.response_type);

  uint8_t data[8];
  packing_control_packet(&control_packet, data, len_of_data);

  uint8_t output[16];
  if (dest == NULL) {
    packing_packet(output, &linkaddr_node_addr, &null_addr, data, len_of_data + 1 + 2*sizeof(linkaddr_t));
    LOG_INFO("Sending control packet to null\n");
    LOG_INFO("Address: ");
    LOG_INFO_LLADDR(&null_addr);
    LOG_INFO_("\n");
  } else {
    packing_packet(output, &linkaddr_node_addr, dest, data, len_of_data + 1 + 2*sizeof(linkaddr_t));
  }

  // memcpy(nullnet_buf, packet_space, 2*sizeof(linkaddr_t) + len_of_data + 1);
  nullnet_buf = output;
  nullnet_len = sizeof(linkaddr_t)*2 + len_of_data + 1;

  LOG_INFO("Sending control packet to: ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO_("\n");

  NETSTACK_NETWORK.output(dest);

  /* Freeing everything */
  // free(data);
  // free(output);
}

void check_parent_node(const linkaddr_t* src, uint8_t node_type, parent_t* parent) {
  /* Create new possible parent */
  signed char rssi = cc2420_last_rssi;
  LOG_INFO("type_parent: %u\n", type_parent);

  if (node_type == GATEWAY) {
    LOG_INFO("Ignoring gateway\n");
    return;
  }

  if (not_setup()) {
    setup = 1;
    set_parent(src, node_type, rssi, parent, NODE);
    LOG_INFO("First parent setup\n");
    return;
  }

  /* If the new parent is better than the current one */
  if (parent->type < node_type) {
    set_parent(src, node_type, rssi, parent, NODE);
    LOG_INFO("Better parent found\n");
    return;
  }

  /* If the new parent is the same as the current one */
  if (
      type_parent == node_type &&
      parent->rssi < rssi
      ) 
  {
    set_parent(src, node_type, rssi, parent, NODE);
    LOG_INFO("Better parent found\n");
    return;
  }
  LOG_INFO("Parent poopoo\n");
}

void check_parent_sub_gateway(const linkaddr_t* src, uint8_t node_type, parent_t* parent) {
  /* Create new possible parent */
  signed char rssi = cc2420_last_rssi;
  LOG_INFO("type_parent: %u\n", type_parent);

  if (node_type != GATEWAY) {
    LOG_INFO("Ignoring not a gateway\n");
    return;
  }

  if (not_setup()) {
    setup = 1;
    set_parent(src, node_type, rssi, parent, SUB_GATEWAY);
    LOG_INFO("First parent setup\n");
    return;
  }

  /* If the new parent is the same as the current one */
  if (parent->rssi < rssi) 
  {
    set_parent(src, node_type, rssi, parent, SUB_GATEWAY);
    LOG_INFO("Better parent found\n");
    return;
  }
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void process_node_packet(const void *data, uint16_t len, linkaddr_t *src, linkaddr_t *dest, uint8_t* packet_type, parent_t* parent) {
  if (len == 0) {
    LOG_INFO("Empty packet\n");
    return;
  }

  uint8_t head = ((uint8_t *)data)[0];
  *packet_type = head >> 7;
  LOG_INFO("Packet type: %u\n", *packet_type);

  if (*packet_type == CONTROL) {
    LOG_INFO("Received control packet\n");
    control_header_t header;
    process_control_header(data, len, &header);
    LOG_INFO("Node type: %u\n", header.node_type);
    LOG_INFO("Response type: %u\n", header.response_type);


    if (header.node_type == SUB_GATEWAY && header.response_type == RESPONSE) {
      check_parent_node(src, header.node_type, parent);
      return;
    }

    /* Sending back a control packet to the source
     * if setup is not done
    */
    if (!not_setup() && header.response_type == SETUP) {
      LOG_INFO("Sending response control packet\n");
      control_packet_send(NODE, src, RESPONSE, 0, NULL);
      return;
    }

    if (header.response_type == SETUP) {
      LOG_INFO("Ignoring packet, node not setup\n");
      return;
    }

    if (header.response_type == RESPONSE) {
      LOG_INFO("Received response control packet\n");
      check_parent_node(src, header.node_type, parent);
      return;
    }

    if (header.response_type == SETUP_ACK) {
      set_child(src, (uint8_t*)data);
      child_t new_child = children[children_count - 1];

      /* Forwarding child to gateway */
      send_child(new_child, NODE, parent);

      LOG_INFO("Received setup ack control packet\n");
      LOG_INFO("New children at address: ");
      LOG_INFO_LLADDR(&new_child.addr);
      LOG_INFO("\n");
      LOG_INFO("From: ");
      LOG_INFO_LLADDR(&new_child.from);
      LOG_INFO("\n");
      return;
    }

    return;
  }

  if (not_setup()) {
    LOG_INFO("Ignoring data packet, node not setup\n");
    return;
  }

  if (*packet_type == DATA) {
    forward_data_packet(data, len, parent);
    return;
  }
}

void process_sub_gateway_packet(const uint8_t* data, uint16_t len, linkaddr_t *src, linkaddr_t *dest, uint8_t* packet_type, parent_t* parent) {
  LOG_INFO("Processing sub gateway packet\n");
  if (len == 0) {
    LOG_INFO("Empty packet\n");
    return;
  }

  uint8_t head = ((uint8_t *)data)[0];
  uint8_t type = head >> 7;
  memcpy(packet_type, &type, sizeof(uint8_t));
  LOG_INFO("Packet type: %u\n", *packet_type);

  if (*packet_type == CONTROL) {
    LOG_INFO("Received control packet\n");
    control_header_t header;
    LOG_INFO("Processing control header\n");
    process_control_header(data, len, &header);
    LOG_INFO("Node type: %u\n", header.node_type);
    LOG_INFO("Response type: %u\n", header.response_type);

    if (header.response_type == SETUP_ACK) {
      set_child(src, (uint8_t*)data);
      child_t new_child = children[children_count - 1];

      /* Forwarding child to gateway */
      send_child(new_child, SUB_GATEWAY, parent);

      LOG_INFO("Received setup ack control packet\n");
      LOG_INFO("New children at address: ");
      LOG_INFO_LLADDR(&new_child.addr);
      LOG_INFO("\n");
      LOG_INFO("From: ");
      LOG_INFO_LLADDR(&new_child.from);
      LOG_INFO("\n");
      return;
    }

    if (header.response_type == DATA_ACK) {
      process_data_ack(data, src);
    }

    if (header.response_type <= 1 && header.node_type == GATEWAY) {
      check_parent_sub_gateway(src, header.node_type, parent);
      return;
    }

    if (!not_setup() && header.response_type == SETUP) {
      LOG_INFO("Sending back a control packet\n");
      control_packet_send(SUB_GATEWAY, src, RESPONSE, 0, NULL);
      return;
    }
  }
}

void process_data_ack(const uint8_t* data, linkaddr_t* src){
  linkaddr_t dest =*((linkaddr_t*) (data+1));
  if (linkaddr_cmp(dest, link)){
    LOG_INFO("Ack reached destination\n");
    return;
  }
  linkaddr_t nexthop;
  get_children(&dest, &nexthop);
  if (linkaddr_cmp(&nexthop, NULL)) {
    LOG_INFO("No children found\n");
    return;
  }
  
  control_packet_send(0, &nexthop, DATA_ACK, sizeof(linkaddr_t), &dest);

}

void process_gateway_packet(const void *data, uint16_t len, linkaddr_t *src, linkaddr_t *dest, uint8_t* packet_type) {
  LOG_INFO("Processing gateway packet\n");
  if (len == 0) {
    LOG_INFO("Empty packet\n");
    return;
  }

  uint8_t head = ((uint8_t *)data)[0];
  *packet_type = head >> 7;
  LOG_INFO("Packet type: %u\n", *packet_type);

  if (*packet_type == CONTROL) {

    control_header_t header; 
    process_control_header(data, len, &header);
    LOG_INFO("Node type: %u\n", header.node_type);
    LOG_INFO("Response type: %u\n", header.response_type);

    if (header.node_type != SUB_GATEWAY) {
      LOG_INFO("Ignoring control packet, not from sub-gateway\n");
      return;
    }

    if (header.response_type == SETUP_ACK) {
      set_child(src, (uint8_t*)data);
      child_t new_child = children[children_count - 1];
      LOG_INFO("Received setup ack control packet\n");
      LOG_INFO("New children at address: ");
      LOG_INFO_LLADDR(&new_child.addr);
      LOG_INFO("\n");
      LOG_INFO("From: ");
      LOG_INFO_LLADDR(&new_child.from);
      LOG_INFO("\n");
      LOG_INFO("New child type %u\n", new_child.type);
      return;
    }

    if (header.response_type == SETUP) {
      LOG_INFO("Sending back a control packet\n");
      control_packet_send(GATEWAY, src, RESPONSE, 0, NULL);
      return;
    }
  }

  if (*packet_type == DATA) {
    LOG_INFO("Received data packet\n");
    linkaddr_t nexthop;
    get_children(src, &nexthop);
    if (linkaddr_cmp(&nexthop, NULL)) {
      LOG_INFO("No children found\n");
      return;
    }

    control_packet_send(GATEWAY, &nexthop, DATA_ACK, sizeof(linkaddr_t), src);

  }
}
/*---------------------------------------------------------------------------*/





/*---------------------------------------------------------------------------*/
void print_data_packet(data_packet_t* data_packet) {
  LOG_INFO("Data packet\n");
  LOG_INFO("Type: %u\n", data_packet->header.type);
  LOG_INFO("Length of topic: %u\n", data_packet->header.len_topic);
  LOG_INFO("Length of data: %u\n", data_packet->header.len_data);
  LOG_INFO("Topic: %s\n", data_packet->topic);
  LOG_INFO("Data: %s\n", data_packet->data);
}

void print_control_packet(control_packet_t* control_packet) {
  LOG_INFO("Control packet\n");
  LOG_INFO("Type: %u\n", control_packet->header->type);
  LOG_INFO("Node type: %u\n", control_packet->header->node_type);
  LOG_INFO("Response type: %u\n", control_packet->header->response_type);
  LOG_INFO("Data: %p\n", control_packet->data);
}

void print_children() {
  LOG_INFO("Children\n");
  for (uint8_t i = 0; i < children_count; i++) {
    LOG_INFO("Child %u\n", i);
    LOG_INFO("Address: ");
    LOG_INFO_LLADDR(&children[i].addr);
    LOG_INFO("\n");
    LOG_INFO("From: ");
    LOG_INFO_LLADDR(&children[i].from);
    LOG_INFO("\n");
    LOG_INFO("Type: %u\n", children[i].type);
  }
}