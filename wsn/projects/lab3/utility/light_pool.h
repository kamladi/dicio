/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * light_pool.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#ifndef __light_pool_h
#define __light_pool_h

#include <type_defs.h>

/*** LIGHT POOL OPERATIONS ***/
int8_t in_light_pool(light_pool_t *lp, uint8_t node_address);
int8_t get_light_pool_index(light_pool_t *lp, uint8_t node_address);
int8_t add_to_light_pool(light_pool_t *lp, uint8_t node_address, uint16_t light_value);
int8_t update_light_pool(light_pool_t *lp, uint8_t node_address, uint16_t light_value);
void print_light_pool(light_pool_t *lp);

#endif
