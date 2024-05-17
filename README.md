# LINFO2146 - Project

## Introduction

This project is a collection of programs for controlling and managing a network of IoT devices. It includes a gateway, nodes, and sub-gateways, as well as specific devices like light sensors, irrigation valves, and light bulbs. The project is built on the Contiki-NG operating system and uses custom routing for network communication.

## Project Structure

- `gateway.c`, `node.c`, `sub-gateway.c`, `node-light-sensor.c`, `irrigation-valve.c`, `light-bulb.c`, `mobile.c`: These are the main files for each device type in the network. They include the main processes for each device and define the send intervals and keep alive intervals.

- `routing/custom-routing.c` and `routing/custom-routing.h`: These files implement the custom routing used for network communication. They define several structures for control headers, control packets, data headers, and data packets.

- `project-conf.h`: This file contains the configuration for the project, including log levels.

- `Makefile`: This file defines the build process for the project. It specifies the Contiki-NG operating system and the NullNet network layer.

- `dev/`: This directory contains device-specific files, such as `cc2420.c` and `cc2420.h`.

Please refer to the individual files for more detailed information about each part of the project.