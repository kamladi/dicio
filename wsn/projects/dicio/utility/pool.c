/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * pool.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <pool.h>

/*** SEQUENCE POOL OPERATIONS ***/
/**
 * in_pool:
 *  - determine if node_address is in the sequence pool
 *
 * @param pool - sequence pool to search
 * @param node_address - node to search for
 * @returns '1' if id found, '-1' otherwise
 */
int8_t in_pool(pool_t *pool, uint8_t node_address) {
    for(uint8_t i = 0; i < pool->size; i++) {
        if(pool->node_id[i] == node_address) {
            return (int8_t)1;
        }
    }
    return (int8_t)-1;
}

void clear_pool(pool_t *pool) { 
    for(uint8_t i = 0; i < pool->size; i++) {
        pool->data_vals[i] = 0;
    }
}

/**
 * get_pool_index:
 *  - return the index of teh node_address in the sequence pool
 *
 * @param pool - sequence pool to be searched
 * @param node_address - node to search for
 * @returns index of node_address if found, '-1' otherwise
 */
int8_t get_pool_index(pool_t *pool, uint8_t node_address) {
    for(uint8_t i = 0; i < pool->size; i++) {
        if(pool->node_id[i] == node_address) {
            return i;
        }
    }
    return -1;
}

uint16_t get_data_val(pool_t *pool, uint8_t node_address) {
    int8_t index = get_pool_index(pool, node_address);
    if(index != -1) {
        return pool->data_vals[index];
    }
    return 0;
}

/**
 * add_to_pool:
 *  - add a new item to the sequence pool
 *
 * @param pool - sequence pool to which new entry will be added
 * @param node_address - address of the node to be added
 * @param data_val - sequence number of node to be added
 * @returns '1' if add was successful, '-1' otherwise
 */
int8_t add_to_pool(pool_t *pool, uint8_t node_address, uint16_t data_val) {
    if((pool->size < MAX_POOL) && (in_pool(pool, node_address) == -1)) {
        uint8_t index = pool->size;
        pool->size++;
        pool->node_id[index] = node_address;
        pool->data_vals[index] = data_val;
        return 1;
    }
    return -1;
}

/**
 * update_pool:
 *  - update the sequence pool with new sequence number
 *
 * @param pool - sequence pool to be updated
 * @param node_address - node whose sequence number needs updating
 * @param data_val - new sequence number of node_address
 * @returns '1' if update was successful, '-1' otherwise
 */
int8_t update_pool(pool_t *pool, uint8_t node_address, uint16_t data_val) {
    int8_t index = get_pool_index(pool, node_address);
    if(index >= 0) {
        pool->data_vals[index] = data_val;
        return 1;
    }
    return -1;
}
