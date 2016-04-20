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

void inline clear_pool(pool_t *pool);
int8_t inline in_pool(pool_t *pool, uint8_t node_address);
void inline decrement_all(pool_t *pool);
int8_t inline get_pool_index(pool_t *pool, uint8_t node_address);
uint16_t inline get_data_val(pool_t *pool, uint8_t node_address);
int8_t inline add_to_pool(pool_t *pool, uint8_t node_address, uint16_t data_val);
int8_t inline update_pool(pool_t *pool, uint8_t node_address, uint16_t data_val);

#endif