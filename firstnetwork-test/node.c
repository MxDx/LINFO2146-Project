/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         NullNet broadcast example
 * \author
*         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h>
#include "custom-routing.h"
#include "dev/cc2420.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define IS_NODE 1

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

uint8_t setup = 0;

static parent_t* parent_addr;

/*---------------------------------------------------------------------------*/
void check_parent(const linkaddr_t* src, uint8_t node_type) {
  // Create new possible parent
  parent_t* new_parent = malloc(sizeof(parent_t));
  linkaddr_t src_addr;
  memcpy(&src_addr, src, sizeof(linkaddr_t));
  new_parent->parent_addr = &src_addr;
  new_parent->rssi = cc2420_last_rssi;
  new_parent->type = node_type;
  
  LOG_INFO("Received setup control packet\n");
  LOG_INFO("Parent address: ");
  LOG_INFO_LLADDR(new_parent->parent_addr);
  LOG_INFO_("\n");
  LOG_INFO("RSSI: %d\n", new_parent->rssi);

  if (parent_addr == NULL) {
    setup = 1;
    parent_addr = new_parent;
    LOG_INFO("First parent setup\n");
    return;
  }

  // If the new parent is better than the current one
  if (parent_addr->parent_addr < new_parent->parent_addr) {
    parent_addr = new_parent;
    LOG_INFO("Better parent found\n");
    return;
  }

  // If the new parent is the same as the current onej
  if (
      parent_addr->type == new_parent->type &&
      parent_addr->rssi < new_parent->rssi
      ) 
  {
    parent_addr = new_parent;
    LOG_INFO("Better parent found\n");
    return;
  }
  LOG_INFO("Parent poopoo\n");
}

void control_packet_send(const linkaddr_t* dest, uint8_t response_type) {
  control_packet_t control_packet;
  control_packet.type = CONTROL << 7;
  control_packet.type |= NODE << 6;
  control_packet.type |= response_type << 4;

  nullnet_buf = (uint8_t *)&control_packet;
  nullnet_len = sizeof(control_packet);
  LOG_INFO("Sending control packet to ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO_("\n");

  NETSTACK_NETWORK.output(dest);
}


void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  LOG_INFO("Received data of len %u\n", len);
  if (len == 0) {
    return;
  }

  uint8_t header = ((uint8_t *)data)[0];
  uint8_t packet_type = header >> 7;
  LOG_INFO("Packet type: %u\n", packet_type);

  if (packet_type == DATA) {
    LOG_INFO("Received data packet\n");
  } else if (packet_type == CONTROL) {
    LOG_INFO("Received control packet\n");
    uint8_t node_type = (header >> 6) & 0b1;
    uint8_t response_type = (header >> 4) & 0b1;

    // Sending back a control packet to the source
    // if setup is not done
    if (setup == 1 && response_type == SETUP) {
      LOG_INFO("Sending response control packet\n");
      control_packet_send(src, RESPONSE);
      return;
    }

    if (response_type == SETUP) {
      LOG_INFO("Ignoring packet\n");
      return;
    }

    check_parent(src, node_type);
  }
}

void init_node() {
  // control_packet_t control_packet;
  // control_packet.type = CONTROL << 7;
  // control_packet.type |= NODE << 6;
  // nullnet_buf = (uint8_t *)&control_packet;
  // nullnet_len = sizeof(control_packet);

  // LOG_INFO("Sending control packet (setup) with type %u\n", control_packet.type);
  // NETSTACK_NETWORK.output(NULL);
  control_packet_send(NULL, SETUP);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  // static unsigned count = 0;

  PROCESS_BEGIN();

  // RESPONSE FUNCTION
  nullnet_set_input_callback(input_callback);
  
  static struct etimer periodic_timer_setup;
  etimer_set(&periodic_timer_setup, SEND_INTERVAL);
  while (setup == 0) {
    etimer_reset(&periodic_timer_setup);
    init_node();
    LOG_INFO("Setup value is %u\n", setup);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_setup));
  }

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    // LOG_INFO("Sending %u to ", count);
    // LOG_INFO_LLADDR(NULL);
    // LOG_INFO_("\n");
    
    // memcpy(nullnet_buf, &count, sizeof(count));
    // nullnet_len = sizeof(count);

    // NETSTACK_NETWORK.output(NULL);
    // count++;
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
