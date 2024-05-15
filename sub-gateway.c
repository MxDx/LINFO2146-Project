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

parent_t* parent;

// static linkaddr_t parent_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

/*---------------------------------------------------------------------------*/

void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{

  LOG_INFO("Received packet\n");

  uint8_t* packet_type = malloc(sizeof(uint8_t));

  linkaddr_t* src_addr = malloc(sizeof(linkaddr_t));
  linkaddr_t* dest_addr = malloc(sizeof(linkaddr_t));
  void* data_cpy = malloc(len);
  memcpy(src_addr, src, sizeof(linkaddr_t));
  memcpy(dest_addr, dest, sizeof(linkaddr_t));
  memcpy(data_cpy, data, len);

  process_sub_gateway_packet(data_cpy, len, src_addr, dest_addr, packet_type, parent);

  if (*packet_type == DATA) {
    LOG_INFO("Received data packet\n");
    forward_data_packet(data, len, parent);
  } 
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gateway_process, ev, data)
{
  static struct etimer periodic_timer;
  if (parent == NULL) {
    parent = malloc(sizeof(parent_t));
    parent->parent_addr = malloc(sizeof(linkaddr_t));
  }

  setup = 0;
  type_parent = SUB_GATEWAY;

  PROCESS_BEGIN();

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  
  static struct etimer periodic_timer_setup;
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
  LOG_INFO("Gateway process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
