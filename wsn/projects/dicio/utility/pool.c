/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * pool.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <pool.h>

/*** SEQUENCE POOL OPERATIONS ***/
// clear_pool - clear the entire pool
void inline clear_pool(pool_t *pool) {
    volatile uint8_t pool_size = pool->size; 
    for(uint8_t i = 0; i < pool_size; i++) {
        pool->data_vals[i] = 0;
    }
    pool->size = 0;
}

// in_pool - determine if node_address is in the sequence pool
int8_t inline in_pool(pool_t *pool, uint8_t node_address) {
    volatile uint8_t pool_size = pool->size;
    for(uint8_t i = 0; i < pool_size; i++) {
        if(pool->node_id[i] == node_address) {
            return (int8_t)1;
        }
    }
    return (int8_t)-1;
}

// decrement_all - decrement every index in the pool
void inline decrement_all(pool_t *pool) {
    volatile uint8_t pool_size = pool->size;
    for(uint8_t i = 0; i < pool_size; i++) {
        if(ALIVE_LIMIT < pool->data_vals[i]) {
            pool->data_vals[i]--;
        }
    }
}

// get_pool_index - return the index of the node_address in the sequence pool
int8_t inline get_pool_index(pool_t *pool, uint8_t node_address) {
    volatile uint8_t pool_size = pool->size;
    for(uint8_t i = 0; i < pool_size; i++) {
        if(pool->node_id[i] == node_address) {
            return i;
        }
    }
    return -1;
}

// get_data_val - get the data value out of the pool
uint16_t inline get_data_val(pool_t *pool, uint8_t node_address) {
    int8_t index = get_pool_index(pool, node_address);
    if(index != -1) {
        return pool->data_vals[index];
    }
    return 0;
}

// add_to_pool - add a new item to the sequence pool
int8_t inline add_to_pool(pool_t *pool, uint8_t node_address, uint16_t data_val) {
    volatile uint8_t pool_size = pool->size;
    volatile int8_t in_pool_result = in_pool(pool, node_address);
    if((MAX_POOL > pool_size) && (NOT_IN_POOL == in_pool_result)) {
        uint8_t index = pool->size;
        pool->size++;
        pool->node_id[index] = node_address;
        pool->data_vals[index] = data_val;
        return 1;
    }
    return -1;
}

// update_pool - update the sequence pool with new sequence number
int8_t inline update_pool(pool_t *pool, uint8_t node_address, uint16_t data_val) {
    int8_t index = get_pool_index(pool, node_address);
    if(index >= 0) {
        pool->data_vals[index] = data_val;
        return 1;
    }
    return -1;
}
