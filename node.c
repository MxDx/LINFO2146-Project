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
#define KEEP_ALIVE_INTERVAL (30 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "Node process");
AUTOSTART_PROCESSES(&node_process);

parent_t parent;

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
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer periodic_timer;
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

  etimer_set(&periodic_timer_setup, SEND_INTERVAL);
  etimer_set(&periodic_timer, KEEP_ALIVE_INTERVAL);
  while(1) {
    while (not_setup()) {
      etimer_reset(&periodic_timer_setup);
      etimer_reset(&periodic_timer);
      init_node();
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_setup));
    }
    etimer_reset(&periodic_timer);
    etimer_reset(&periodic_timer_setup);
  
    LOG_INFO("Running....\n");
    print_children();
    keep_alive(&parent, "sub_gateway");
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  }
  LOG_INFO("Node process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
