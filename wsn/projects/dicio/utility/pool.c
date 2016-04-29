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
    // initialize
    uint8_t pool_size = pool->size; 

    // loop through all nodes
    for(uint8_t i = 0; i < pool_size; i++) {
        pool->data_vals[i] = 0;
    }
    pool->size = 0;
}

// in_pool - determine if node_address is in the sequence pool
int8_t inline in_pool(pool_t *pool, uint8_t node_address) {
    // initialize
    uint8_t node_id;
    uint8_t pool_size = pool->size;

    // loop through all nodes
    for(uint8_t i = 0; i < pool_size; i++) {
        node_id = pool->node[i];
        if(node_id == node_address) {
            return (int8_t)1;
        }
    }
    return (int8_t)-1;
}

// decrement_all - decrement every index in the pool
void inline decrement_all(pool_t *pool) {
    // initialize
    uint16_t data_val;
    uint8_t pool_size = pool->size;

    // loop through all nodes
    for(uint8_t i = 0; i < pool_size; i++) {
        data_val = pool->data_vals[i];
        if(ALIVE_LIMIT < data_vals) {
            pool->data_vals[i] = data_val-1;
        }
    }
}

// get_pool_index - return the index of the node_address in the sequence pool
int8_t inline get_pool_index(pool_t *pool, uint8_t node_address) {
    // initialize
    uint8_t node_id;
    uint8_t pool_size = pool->size;

    // loop through all nodes
    for(uint8_t i = 0; i < pool_size; i++) {
        node_id = pool->node_id[i];
        if(node_id == node_address) {
            return i;
        }
    }
    return -1;
}

// get_data_val - get the data value out of the pool
uint16_t inline get_data_val(pool_t *pool, uint8_t node_address) {
    // initialize
    int8_t index;
    uint16_t data_val;

    // get index
    index = get_pool_index(pool, node_address);

    // return on index
    if(index != -1) {
        data_val = pool->data_vals[index];
        return data_val;
    }
    return 0;
}

// add_to_pool - add a new item to the sequence pool
int8_t inline add_to_pool(pool_t *pool, uint8_t node_address, uint16_t data_val) {
    // initialize
    uint8_t pool_size = pool->size;
    uint8_t size = pool->size;
    int8_t in_pool_result = in_pool(pool, node_address);

    // if the pool is not full and this node is not in the pool then it can be added
    if((MAX_POOL > pool_size) && (NOT_IN_POOL == in_pool_result)) {
        pool->size = size+1;
        pool->node_id[size] = node_address;
        pool->data_vals[size] = data_val;
        return 1;
    }
    return -1;
}

// update_pool - update the sequence pool with new sequence number
int8_t inline update_pool(pool_t *pool, uint8_t node_address, uint16_t data_val) {
    // initialize
    int8_t index = get_pool_index(pool, node_address);

    // update node at position 'index'
    if(index >= 0) {
        pool->data_vals[index] = data_val;
        return 1;
    }
    return -1;
}
