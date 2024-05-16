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


  uint8_t packet_type;
  process_node_packet(data, len, &packet.src, &packet.dest, &packet_type, &parent, UNICAST_GROUP);
  LOG_INFO("Received packet\n");
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

    uint8_t light_intensity = generate_light_intensity();
    // Turn the light intensity into a string
    char* light_intensity_str = malloc(sizeof(char) * 4);
    sprintf(light_intensity_str, "%d", light_intensity);
    uint16_t len_data = strlen(light_intensity_str);

    send_data_packet(1, UNICAST_GROUP, len_topic, len_data, topic, light_intensity_str, &parent.parent_addr, 1);    
    LOG_INFO("Packet sent\n");

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  }
  LOG_INFO("Light sensor process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
