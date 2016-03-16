/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * sequence_pool.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
#include <sequence_pool.h>

/*** SEQUENCE POOL OPERATIONS ***/
/**
 * in_sequence_pool:
 *  - determine if node_address is in the sequence pool
 *
 * @param sp - sequence pool to search
 * @param node_address - node to search for
 * @returns '1' if id found, '-1' otherwise
 */
int8_t in_sequence_pool(sequence_pool_t *sp, uint8_t node_address) {
    for(uint8_t i = 0; i < sp->size; i++) {
        if(sp->node_id[i] == node_address) {
            return (int8_t)1;
        }
    }
    return (int8_t)-1;
}

/**
 * get_sequence_pool_index:
 *  - return the index of teh node_address in the sequence pool
 *
 * @param sp - sequence pool to be searched
 * @param node_address - node to search for
 * @returns index of node_address if found, '-1' otherwise
 */
int8_t get_sequence_pool_index(sequence_pool_t *sp, uint8_t node_address) {
    for(uint8_t i = 0; i < sp->size; i++) {
        if(sp->node_id[i] == node_address) {
            return i;
        }
    }
    return -1;
}

uint16_t get_sequence_number(sequence_pool_t *sp, uint8_t node_address) {
    int8_t index = get_sequence_pool_index(sp, node_address);
    if(index != -1) {
        return sp->seq_nums[index];
    }
    return 0;
}

/**
 * add_to_sequence_pool:
 *  - add a new item to the sequence pool
 *
 * @param sp - sequence pool to which new entry will be added
 * @param node_address - address of the node to be added
 * @param seq_num - sequence number of node to be added
 * @returns '1' if add was successful, '-1' otherwise
 */
int8_t add_to_sequence_pool(sequence_pool_t *sp, uint8_t node_address, uint16_t seq_num) {
    if((sp->size < MAX_POOL) && (in_sequence_pool(sp, node_address) == -1)) {
        uint8_t index = sp->size;
        sp->size++;
        sp->node_id[index] = node_address;
        sp->seq_nums[index] = seq_num;
        return 1;
    }
    return -1;
}

/**
 * update_sequence_pool:
 *  - update the sequence pool with new sequence number
 *
 * @param sp - sequence pool to be updated
 * @param node_address - node whose sequence number needs updating
 * @param seq_num - new sequence number of node_address
 * @returns '1' if update was successful, '-1' otherwise
 */
int8_t update_sequence_pool(sequence_pool_t *sp, uint8_t node_address, uint16_t seq_num) {
    int8_t index = get_sequence_pool_index(sp, node_address);
    if(index >= 0) {
        sp->seq_nums[index] = seq_num;
        return 1;
    }
    return -1;
}
