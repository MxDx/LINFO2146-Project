#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h>
#include "dev/serial-line.h"
#include "cpu/msp430/dev/uart0.h"

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
    data_packet_t packet_data;
    process_data_packet(data, len, &packet_data);
      char *stringToReturn = malloc(sizeof(char) * 20);
      int barnNb;
      for (barnNb = 0; barnNb < barns_size; barnNb++) {
        if (linkaddr_cmp(&barns[barnNb], dest)) {
          break;
        }
      }
      sprintf(stringToReturn, "/%u/%s/=%s", barnNb, packet_data.topic, packet_data.data);
      printf("%s\n", stringToReturn);
      free(stringToReturn);

      /* /!\ freeing topic and data */
      free(packet_data.topic);
      free(packet_data.data);
  } 
}

void striping_data(char* message, char* barn_number, char* topic, char* data) {
  // get the first token
  barn_number = strtok(message, "/=");
  topic = strtok(NULL, "/=");
  data = strtok(NULL, "/=");
}

void decide_action(char* topic, char* data, uint8_t barn_number) {
  LOG_INFO("Topic: %s\n", topic);
  LOG_INFO("Data: %s\n", data);
  LOG_INFO("Barn number: %d\n", barn_number);
  if (strcmp(topic, "lights") == 0) {
    send_data_packet(0, LIGHT_BULB_GROUP, strlen(topic), strlen(data), topic, data, &barns[barn_number], 0, NOT_MOBILE);
  }

  if (strcmp(topic, "irrigation") == 0) {
    if (barn_number == 255) {
      for (int i = 0; i < barns_size; i++) {
        send_data_packet(0, IRRIGATION_GROUP, strlen(topic), strlen(data), topic, data, &barns[i], 0, NOT_MOBILE);
      }
    } else {
      send_data_packet(0, IRRIGATION_GROUP, strlen(topic), strlen(data), topic, data, &barns[barn_number], 0, NOT_MOBILE);
    }
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
  serial_line_init();
  uart0_set_input(serial_line_input_byte);

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  
  init_gateway();

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();

    if (ev == serial_line_event_message) {
      char* message = (char*) data;
      // LOG_INFO("Received message: %s\n", message);
      /* Get the topic and the data to send "/barn_number/topic=data" */
      char* barn_number = strtok(message, "/=");
      char* topic = strtok(NULL, "/=");
      char* data = strtok(NULL, "/=");

      int barn_number_int = atoi(barn_number);

      decide_action(topic, data, barn_number_int);
    }

    // if (etimer_expired(&periodic_timer)) {
    //   LOG_INFO("Running....\n");
    //   print_children();

    //   char* topic = "light";
    //   uint16_t len_topic = strlen(topic);
    //   char* data = "off";
    //   uint16_t len_data = strlen(data);

    //   linkaddr_t nexthop;
    //   get_multicast_children(LIGHT_BULB_GROUP, &nexthop, 0);

    //   send_data_packet(0, LIGHT_BULB_GROUP, len_topic, len_data, topic, data, &nexthop, 0);
    //   etimer_reset(&periodic_timer);
    // }
  }
  LOG_INFO("Gateway process ended\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
