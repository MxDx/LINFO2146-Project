CONTIKI_PROJECT = gateway node sub-gateway node-light-sensor irrigation-valve light-bulb mobile
all: $(CONTIKI_PROJECT)

MODULES_REL += ./routing

CONTIKI = /home/user/contiki-ng

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC ?= MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_NULLNET
include $(CONTIKI)/Makefile.include
