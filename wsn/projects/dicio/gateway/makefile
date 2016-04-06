## 18-748 Wireless Sensor Networks
## Spring 2016
## Dicio - A Smart Outlet Mesh Network
## makefile (gateway)
## Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.

# Platform name  cc2420DK, firefly, micaZ
#PLATFORM = firefly2_2
PLATFORM = firefly3
#PLATFORM = firefly2_3


# Target file name (without extension).
TARGET = main

# Set the Port that you programmer is connected to 
PROGRAMMING_PORT = /dev/ttyUSB0 # Default FireFly port 
# PROGRAMMING_PORT = /dev/ttyUSB0 # Default micaZ port 

# Set this such that the nano-RK directory is the base path
ROOT_DIR = ../../..

# Set platform specific defines 
# The following will be defined based on the PLATFORM variable:
# PROG_TYPE  (e.g. avrdude, or uisp)
# MCU (e.g. atmega32, atmega128, atmega1281) 
# RADIO (e.g. cc2420)
include $(ROOT_DIR)/include/platform.mk


SRC = $(TARGET).c

# Add extra source files. 
# For example:
SRC += $(ROOT_DIR)/src/net/bmac/$(RADIO)/bmac.c
SRC += $(ROOT_DIR)/projects/dicio/utility/assembler.c
SRC += $(ROOT_DIR)/projects/dicio/utility/packet_queue.c
SRC += $(ROOT_DIR)/projects/dicio/utility/parser.c
SRC += $(ROOT_DIR)/projects/dicio/utility/pool.c

# Add extra includes files. 
# For example:
EXTRAINCDIRS =
EXTRAINCDIRS += $(ROOT_DIR)/src/net/bmac
EXTRAINCDIRS += $(ROOT_DIR)/projects/dicio/utility

#  This is where the final compile and download happens
include $(ROOT_DIR)/include/platform/$(PLATFORM)/common.mk
