CONTIKI_PROJECT = gateway node node-light-sensor
all: $(CONTIKI_PROJECT)

MODULES_REL += ./routing

CONTIKI = ..

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC ?= MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_NULLNET
include $(CONTIKI)/Makefile.include
