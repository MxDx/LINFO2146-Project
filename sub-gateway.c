#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h>

#define IS_GATEWAY 1
#include "routing/custom-routing.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define IS_GATEWAY 1

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(gateway_process, "Gateway process");
AUTOSTART_PROCESSES(&gateway_process);

parent_t parent;

// static linkaddr_t parent_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

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
    LOG_INFO("My linkaddr: ");
    LOG_INFO_LLADDR(&linkaddr_node_addr);
    LOG_INFO_("\n");
    LOG_INFO("Ignoring packet not for me\n");
    return;
  }


  uint8_t packet_type;
  process_sub_gateway_packet(data + 2*sizeof(linkaddr_t), len, &packet.src, &packet.dest, &packet_type, &parent);

  if (packet_type == DATA) {
    LOG_INFO("Received data packet\n");
    forward_data_packet(data, len, &parent);
  } 

}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gateway_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer periodic_timer_setup;
  // if (node_type == NULL) {
  //   node_type = malloc(sizeof(uint8_t));
  //   *node_type = SUB_GATEWAY;
  // }
  node_type = SUB_GATEWAY;

  setup = 0;
  type_parent = SUB_GATEWAY;

  PROCESS_BEGIN();

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  
  etimer_set(&periodic_timer_setup, SEND_INTERVAL);
  while (not_setup()) {
    etimer_reset(&periodic_timer_setup);
    LOG_INFO("Setup value: %u\n", setup);
    init_sub_gateway();
    LOG_INFO("Waiting for setup\n");
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_setup));
  }

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    LOG_INFO("Running....\n");
    etimer_reset(&periodic_timer);
  }
  LOG_INFO("Sub-gateway process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
