/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * light_pool.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#include <light_pool.h>


/*** LIGHT POOL OPERATIONS ***/
/**
 * id_in_light_pool:
 *  - determine if node_address is in the light pool
 *
 * @param lp - sequence pool to search
 * @param node_address - node to search for
 * @returns '1' if id found, '-1' otherwise
 */
int8_t in_light_pool(light_pool_t *lp, uint8_t node_address) {
    for(uint8_t i = 0; i < lp->size; i++) {
        if(lp->node_id[i] == node_address) {
            return 1;
        }
    }
    return -1;
}

/**
 * get_light_pool_index:
 *  - return the index of the node_address in the light pool
 *
 * @param lp - light pool to be searched
 * @param node_address - node to search for
 * @returns index of node_address if found, '-1' otherwise
 */
int8_t get_light_pool_index(light_pool_t *lp, uint8_t node_address) {
    for(uint8_t i = 0; i < lp->size; i++) {
        if(lp->node_id[i] == node_address) {
            return i;
        }
    }
    return -1;
}

/**
 * add_to_light_pool:
 *  - add a new item to the light pool
 *
 * @param lp - light pool to which new entry will be added
 * @param node_address - address of the node to be added
 * @param light_value - light value of node to be added
 * @returns '1' if add was successful, '-1' otherwise
 */
int8_t add_to_light_pool(light_pool_t *lp, uint8_t node_address, uint16_t light_value) {
    if((lp->size < MAX_POOL) && (in_light_pool(lp, node_address) == -1)) {
        uint8_t index = lp->size;
        lp->size++;
        lp->node_id[index] = node_address;
        lp->light_values[index] = light_value;
        return 1;
    }
    return -1;
}

/**
 * update_light_pool:
 *  - update the light pool with new light value
 *
 * @param lp - light pool to be updated
 * @param node_address - node whose light value needs updating
 * @param light_value - new light value of node_address
 * @returns '1' if update was successful, '-1' otherwise
 */
int8_t update_light_pool(light_pool_t *lp, uint8_t node_address, uint16_t light_value) {
    int8_t in_lp, index;
    
    in_lp = in_light_pool(lp, node_address);
    if(in_lp == -1) {
        add_to_light_pool(lp, node_address, light_value);
        return 1;
    } else {
        index = get_light_pool_index(lp, node_address);
        if(index >= 0) {
            lp->light_values[index] = light_value;
            return 1;
        }        
    }

    return -1;
}

void print_light_pool(light_pool_t *lp) {
    nrk_kprintf(PSTR("\r\nCurrent Light Values:\r\n"));
    for(uint8_t k = 0; k < lp->size; k++) {
      printf("  -> id: %d, value: %d\r\n", lp->node_id[k], lp->light_values[k]);
    }
}