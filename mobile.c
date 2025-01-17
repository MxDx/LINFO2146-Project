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
#define IS_MOBILE 1

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)
#define KEEP_ALIVE_INTERVAL (30 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(mobile_process, "Mobile process");
AUTOSTART_PROCESSES(&mobile_process);

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
  process_mobile_packet(data, len, &packet.src, &packet.dest, &packet_type, &parent);
  LOG_INFO("Received packet\n");
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mobile_process, ev, data)
{
  static struct etimer periodic_timer;

  setup = 0;
  type_parent = NODE;

  PROCESS_BEGIN();

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  
  static struct etimer periodic_timer_setup;
  static int nb_queries;

  while(1) {
    while (not_setup()) {
      etimer_set(&periodic_timer_setup, SEND_INTERVAL);
      init_node();
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_setup));
    }
    for (nb_queries = 0; nb_queries < 3; nb_queries++){
      etimer_set(&periodic_timer_setup, SEND_INTERVAL);
      char* topic = "mobile";
      uint16_t len_topic = strlen(topic);

      // /!\ Change the following lines to change the queried nodes 
      char* queried_target = malloc(sizeof(char)+1);
      *queried_target = LIGHT_SENSOR_GROUP;
      *(queried_target+1) = '\0';

      uint16_t len_data = sizeof(queried_target);
      send_data_packet(1, UNICAST_GROUP, len_topic, len_data, topic, queried_target, &parent.parent_addr, 0, DATA_QUERY);    
      LOG_INFO("Sending query packet\n");
      free(queried_target);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_setup));
      /* code */
    }
    
    etimer_set(&periodic_timer, KEEP_ALIVE_INTERVAL);
  
    LOG_INFO("Running....\n");
    keep_alive(&parent, "mob");
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  }
  LOG_INFO("Node process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
