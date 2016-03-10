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

// rates/timers
#define DEFAULT_SENSOR_RATE 2
#define DEFAULT_NEIGHBOR_RATE 10
#define MIN_RATE 1
#define MAX_RATE 30

// buffers/messages
#define MAX_BUF_SIZE 24
#define MAX_NEIGHBOR_BUF_SIZE 4
#define MAX_NUM_HOPS 3
#define GATEWAY_ID 1

// tables/pools
#define MAX_NEIGHBOR_TABLE 3
#define MAX_POOL 8
#define MAX_GRAPH 8

/*** ENUMERATIONS ***/
typedef enum
{
  MSG_NO_MESSAGE = 0,
  MSG_NODE_SENSOR_VALUE = 1,
  MSG_NODE_NEIGHBORS = 2,
  MSG_GATEWAY = 3,
} msg_type;

/**
 * light_pool_t struct - hold all 
 * 
 * @param size - number of nodes in the light pool
 * @param node_id - array of node ids
 * @param light_values - array of last seen light values for each node 
 *    NOTE: maps directly to node_id array
 */
typedef struct {
  uint8_t size;
  uint8_t node_id[MAX_NEIGHBOR_TABLE];
  uint16_t light_values[MAX_NEIGHBOR_TABLE];
} light_pool_t;

/** 
 * neighbor_t struct - defines a neighbor struct
 * 
 * NOTES: can expand with RSSI, etc.
 */
typedef struct{
  uint8_t id;
} neighbor_t;

/** 
 * neighbor_table_t struct - defines a neighbor table
 * 
 * @param size - size of the neighbor table
 * @param origin - address of the node who is established this neighbor table
 * @param neighbors - array of neighbors
 */
typedef struct {
  uint8_t size;
  uint8_t origin;
  neighbor_t neighbors[MAX_NEIGHBOR_TABLE];
} neighbor_table_t;

/**
 * neighbor_graph_t - defines a neighbor graph
 * 
 * @param size - size of the neighbor graph (i.e. number of nodes in the systsem)
 * @param neighbor_tables - all neighbor tables from the system.
 */
typedef struct 
{
  uint8_t size;
  neighbor_table_t neighbor_tables[MAX_NEIGHBOR_TABLE];
} neighbor_graph_t; 

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
  uint16_t seq_nums[MAX_NEIGHBOR_TABLE];
} sequence_pool_t;

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
  msg_type type;
  uint8_t source_id;
  uint16_t seq_num;
  uint8_t num_hops;
  
  // if msg_type is NODE_NEIGHBORS
  uint16_t light_value; // just in case
  neighbor_table_t neighbor_table;
  
  // if msg_type is GATEWAY2NODE
  uint16_t sensor_sample_rate;
  uint16_t neighbor_update_rate;
} packet;

#endif