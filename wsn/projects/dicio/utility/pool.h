/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * pool.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#ifndef __pool_h	
#define __pool_h

#include <type_defs.h>

/*** SEQUENCE POOL OPERATIONS ***/
int8_t in_pool(pool_t *pool, uint8_t node_address);
int8_t get_pool_index(pool_t *pool, uint8_t node_address);
int8_t add_to_pool(pool_t *pool, uint8_t node_address, uint16_t data_val);
int8_t update_pool(pool_t *pool, uint8_t node_address, uint16_t data_val);
uint16_t get_data_val(pool_t *pool, uint8_t node_address);

#endif