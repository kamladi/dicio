/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * sequence_pool.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#ifndef __sequence_pool_h	
#define __sequence_pool_h

#include <type_defs.h>

/*** SEQUENCE POOL OPERATIONS ***/
int8_t in_sequence_pool(sequence_pool_t *sp, uint8_t node_address);
int8_t get_sequence_pool_index(sequence_pool_t *sp, uint8_t node_address);
int8_t add_to_sequence_pool(sequence_pool_t *sp, uint8_t node_address, uint16_t seq_num);
int8_t update_sequence_pool(sequence_pool_t *sp, uint8_t node_address, uint16_t seq_num);
uint16_t get_sequence_number(sequence_pool_t *sp, uint8_t node_address);

#endif