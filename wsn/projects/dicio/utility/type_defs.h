/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * type_defs.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#ifndef __type_defs_h	
#define __type_defs_h

/*** INCLUDE STATEMENTS ***/
#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ff_basic_sensor.h>


/*** DEFINE STATEMENTS ***/
// boolean replacements
#define NONE 0
#define NODE_FOUND 1
#define MSG_NOT_SENT 0
#define MSG_SENT 1
#define NOT_SET 0
#define SET 1
#define INVALID 2
#define PRINT2TERM 1
#define BLINKLEDS 1
#define SERV_MSG_RECEIVED 1
#define SERV_MSG_INCOMPLETE 0
#define SAMPLE_SENSOR 0
#define ON 1
#define OFF 0

// buffers/messages
#define MAX_BUF_SIZE 24
#define MAX_PAYLOAD_SIZE 8
#define MAX_NEIGHBOR_BUF_SIZE 4
#define MAX_NUM_HOPS 3
#define MAX_PACKET_BUFFER 8
#define GATEWAY_ID 1

// tables/pools
#define MAX_NEIGHBOR_TABLE 3
#define MAX_POOL 8
#define MAX_GRAPH 8

// payload indexes
#define CMD_ID_INDEX 0
#define CMD_NODE_ID_INDEX 2
#define CMD_ACT_INDEX 3
#define CMDG_ID_INDEX 0
#define CMDG_GROUP_INDEX 2
#define CMDG_ACTION_INDEX 3
#define CMDACK_ID_INDEX 0
#define DATA_PWR_INDEX 0
#define DATA_TEMP_INDEX 2
#define DATA_LIGHT_INDEX 4
#define HANDACK_NODE_ID_INDEX 0

#define COIL_1_OUT NRK_PORTB_6
#define COIL_2_OUT NRK_PORTB_7
#define BTN_IN NRK_PORTE_6

/*** ENUMERATIONS ***/
typedef enum {
  // NOTE: These are the messages that will be used by Dicio. The
  //  ennumeration values should be updated when when parser.c and 
  //  assembeler.c are updated.
  MSG_NO_MESSAGE = 0,
  MSG_GATEWAY = 3,
  MSG_DATA = 5,
  MSG_CMD = 6,
  MSG_CMDACK = 7,
  MSG_HAND = 8,
  MSG_HANDACK = 9, 
} msg_type;

/**
 * sequence_pool_t struct - hold all neighbor id's and the last seen sequence 
 *  number for that neighbor
 * 
 * @param size - number of seen neighbors
 * @param neighbor_id - array of neighbor ids
 * @param seq_num - array of the last seen sequence numbers for each neighbor. 
 *    NOTE: maps directly to neighbor_id array
 */
typedef struct {
  uint8_t size;
  uint8_t node_id[MAX_NEIGHBOR_TABLE];
  uint16_t data_vals[MAX_NEIGHBOR_TABLE];
} pool_t;

/**
 * paket struct - defines a network packet
 * 
 * @param source_id - sending node
 * @param seq_num - message sequence number of the origin node
 * @param num_hops - number of hops this message has taken
 * @param light_value - value of light sensor
 * @param neighbor_table - neighbor_table of the sending node
 * 
 */
typedef struct{
  uint8_t source_id;
  msg_type type;
  uint16_t seq_num;
  uint8_t num_hops;
  uint8_t payload[MAX_PAYLOAD_SIZE];
} packet;

typedef struct{
  packet buffer[MAX_PACKET_BUFFER];
  uint8_t front;
  uint8_t back;
  uint8_t size;
} packet_queue;

typedef struct {
  uint16_t pwr_val;
  uint16_t temp_val;
  uint16_t light_val;
} sensor_packet;

#endif