
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
static linkaddr_t barns[16];
static int barns_size = 0;

// static linkaddr_t parent_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

/*---------------------------------------------------------------------------*/

void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  LOG_INFO("Received packet\n");
  packet_t packet;
  process_packet(data, len, &packet);

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
  process_gateway_packet(data + 2*sizeof(linkaddr_t), len, &packet.src, &packet.dest, &packet_type, barns, &barns_size);

  if (packet_type == DATA) {
    data_packet_t * packet_data = malloc(sizeof(data_packet_t));
    process_data_packet(data, len, packet_data);
    //if (!strcmp(packet_data.topic, "keepalive")){
      char *stringToReturn = malloc(sizeof(char) * 20);
      int barnNb;
      for (barnNb = 0; barnNb < barns_size; barnNb++) {
        if (linkaddr_cmp(&barns[barnNb], src)) {
          break;
        }
      }
      barnNb--;
      LOG_INFO("pute : %s\n", packet_data->topic);
      sprintf(stringToReturn, "/%u/%s", barnNb, packet_data->topic);
      LOG_INFO("received data from subject : %s\n", stringToReturn);
      free(stringToReturn);
      free(packet_data);
    //}
    //sprintf(dest, string, value);
    LOG_INFO("Received data packet\n");
  } 
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gateway_process, ev, data)
{
  static struct etimer periodic_timer;
  // if (node_type == NULL) {
  //   node_type = malloc(sizeof(uint8_t));
  //   *node_type = SUB_GATEWAY;
  // }
  setup = 0;
  type_parent = GATEWAY;

  PROCESS_BEGIN();

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  
  init_gateway();

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    LOG_INFO("Running....\n");
    print_children();
    char* topic = "light";
    uint16_t len_topic = strlen(topic);
    char* data = "off";
    uint16_t len_data = strlen(data);

    linkaddr_t nexthop;
    get_multicast_children(LIGHT_BULB_GROUP, &nexthop, 0);

    send_data_packet(0, LIGHT_BULB_GROUP, len_topic, len_data, topic, data, &nexthop, 0);
    etimer_reset(&periodic_timer);
  }
  LOG_INFO("Gateway process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
