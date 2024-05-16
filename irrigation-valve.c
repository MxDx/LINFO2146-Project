#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h>
#include "routing/custom-routing.h"
#include "dev/leds.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define IS_NODE 1

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)
#define KEEP_ALIVE_INTERVAL (30 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "Node process");
AUTOSTART_PROCESSES(&node_process);

parent_t parent;
static struct ctimer irrigation_timer; 

/*---------------------------------------------------------------------------*/

static void timer_callback() {
  LOG_INFO("Irrigation ended\n");
  leds_off(LEDS_YELLOW);
}

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
  process_node_packet(data, len, &packet.src, &packet.dest, &packet_type, &parent, IRRIGATION_GROUP);

  if (packet_type == DATA) {
    data_packet_t data_packet;
    process_data_packet(data_cpy, len, &data_packet);
    if (strcmp(data_packet.topic, "irrigation") == 0) {
      int irrigation_time = atoi(data_packet.data);
      LOG_INFO("Irrigating for %d seconds\n", irrigation_time);
      leds_on(LEDS_YELLOW);
      LOG_INFO("Sending ack\n");
      char* topic = "ack_irrigation";
      /* data : ack?irrigation_time */
      // size of maximum int is 11 + 1 for the '\0'
      char* data = malloc(12 + 4);
      sprintf(data, "time:%d", irrigation_time);
      send_data_packet(1, UNICAST_GROUP, strlen(topic), strlen(data), topic, data, &parent.parent_addr, 0);
      ctimer_set(&irrigation_timer, irrigation_time * CLOCK_SECOND, timer_callback, NULL);
      free(data);
    }
    
    /* /!\ Freeing the topic and data */
    free(data_packet.topic);
    free(data_packet.data);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer periodic_timer_setup;
  
  setup = 0;
  type_parent = NODE;

  PROCESS_BEGIN();

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  etimer_set(&periodic_timer, KEEP_ALIVE_INTERVAL);
  etimer_set(&periodic_timer_setup, SEND_INTERVAL);
  init_node();

  while(1) {
    PROCESS_YIELD();
    if (ev == PROCESS_EVENT_TIMER && etimer_expired(&periodic_timer_setup)) {
      if (not_setup()) {
        init_node();
        etimer_set(&periodic_timer_setup, SEND_INTERVAL);
      }
    }

    if (ev == PROCESS_EVENT_TIMER && etimer_expired(&periodic_timer)) {
      if (not_setup()) {
        init_node();
        etimer_set(&periodic_timer_setup, SEND_INTERVAL);
      } else {
        keep_alive(&parent, "irrigation");
        etimer_set(&periodic_timer, KEEP_ALIVE_INTERVAL);
      }
    }
  }
  LOG_INFO("Node process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
