#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h>
#include "routing/custom-routing.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define IS_NODE 1

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)
#define MAX_LIGHT_INTENSITY 255

/*---------------------------------------------------------------------------*/
PROCESS(light_sensor_process, "Light sensor process");
AUTOSTART_PROCESSES(&light_sensor_process);

static parent_t parent;
static uint8_t light_intensity;

// Function to generate random light intensity
int generate_light_intensity() {
    return rand() % (MAX_LIGHT_INTENSITY + 1);
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  packet_t packet;
  process_packet(data, len, &packet);

  LOG_INFO("Received packet\n");
  LOG_INFO("From: ");
  LOG_INFO_LLADDR(&packet.src);
  LOG_INFO_("\n");
  LOG_INFO("To: ");
  LOG_INFO_LLADDR(&packet.dest);
  LOG_INFO_("\n");
  
  if (
    !linkaddr_cmp(&packet.dest, &linkaddr_node_addr) &&
    !linkaddr_cmp(&packet.dest, &null_addr)
  ) {
    LOG_INFO("Ignoring packet not for me\n");
    return;
  }

  uint8_t data_cpy[len];
  memcpy(data_cpy, data, len);

  uint8_t packet_type;
  process_node_packet(data_cpy, len, &packet.src, &packet.dest, &packet_type, &parent, LIGHT_SENSOR_GROUP);
  LOG_INFO("Received packet\n");

  if (packet_type == DATA) {
    data_packet_t data_packet;
    process_data_packet(data_cpy, len, &data_packet);
    if (data_packet.header.multicast_group == LIGHT_SENSOR_GROUP && data_packet.header.mobile_flags == DATA_QUERY) {
      LOG_INFO("Received mobile query\n");

      char* topic = "light";
      uint16_t len_topic = strlen(topic);
      char* light_intensity_str = malloc(sizeof(char) * 4);
      sprintf(light_intensity_str, "%d", light_intensity);
      uint16_t len_data = strlen(light_intensity_str);


      uint64_t len_data_packet = data_packet.header.len_data + data_packet.header.len_topic + LEN_DATA_HEADER + sizeof(linkaddr_t);
      uint8_t new_data[len_data_packet];

      //modif data packet
      build_data_header(&data_packet, 1, UNICAST_GROUP, len_topic, len_data, topic, light_intensity_str, &packet.src, DATA_RESPONSE);

      packing_data_packet(&data_packet, new_data);
      uint8_t output[len_data_packet + LEN_HEADER];
      packing_packet(output, &linkaddr_node_addr, &parent.parent_addr, new_data, len_data_packet);
      forward_data_packet(output, len_data_packet + LEN_HEADER, &parent);

      free(light_intensity_str);
    }

    free(data_packet.data);
    free(data_packet.topic); 
  }


}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(light_sensor_process, ev, data)
{
  static struct etimer periodic_timer;
  // If the parent is already allocated we don't need to allocate it again
  // if (node_type == NULL) {
  //   node_type = malloc(sizeof(uint8_t));
  //   *node_type = NODE;
  // }
  setup = 0;
  type_parent = NODE;

  PROCESS_BEGIN();

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  
  static struct etimer periodic_timer_setup;

  etimer_set(&periodic_timer, SEND_INTERVAL);
  etimer_set(&periodic_timer_setup, SEND_INTERVAL);
  while(1) {
    while (not_setup()) {
      etimer_reset(&periodic_timer_setup);
      etimer_reset(&periodic_timer);
      init_node();
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_setup));
    }
    etimer_reset(&periodic_timer);
    etimer_reset(&periodic_timer_setup);
    // Sending random light sensor data
    char* topic = "light";
    uint16_t len_topic = strlen(topic);

    light_intensity = generate_light_intensity();
    // Turn the light intensity into a string
    char* light_intensity_str = malloc(sizeof(char) * 4);
    sprintf(light_intensity_str, "%d", light_intensity);
    uint16_t len_data = strlen(light_intensity_str);

    send_data_packet(1, UNICAST_GROUP, len_topic, len_data, topic, light_intensity_str, &parent.parent_addr, 1, NOT_MOBILE);    
    LOG_INFO("Packet sent\n");
    free(light_intensity_str);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  }
  LOG_INFO("Light sensor process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
