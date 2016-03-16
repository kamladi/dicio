/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * neighbors.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#include <neighbors.h>

/*** NEIGHBOR TABLE OPERATIONS ***/
/**
 * init_neighbor_table
 */
void init_neighbor_table(neighbor_table_t *nt) {
    nt->size = 0;

    // need to actually force id values to 0 because when we send our the neighbor packet...
    // we send all 4 items. Therefore an old value can still be in [3] but the size could be 2.
    for(uint8_t i = 0; i < MAX_NEIGHBOR_TABLE; i++)
    {
        nt->neighbors[i].id = 0;
    }
}

/**
 * id_in_neighbor_table:
 *  - determine if node_address is already in the neighbor table
 *
 * @param nt - neighbor table to search
 * @param node_address - node to search for
 * @returns '1' if node is found, '-1' otherwise
 */
int8_t in_neighbor_table(neighbor_table_t *nt, uint8_t node_address) {
    for(uint8_t i = 0; i < nt->size; i++) {
        if(nt->neighbors[i].id == node_address) {
            return 1;
        }
    }
    return -1;
}

void add_neighbor(neighbor_table_t *nt, neighbor_t new_neighbor) {
    uint8_t in_nt = in_neighbor_table(nt, new_neighbor.id);
    printf("neighbor table size %d\r\n", nt->size);
    if((nt->size < MAX_NEIGHBOR_TABLE) && (in_nt != -1))
    {
        printf("new neighbor %d\r\n", new_neighbor.id);
        nt->neighbors[nt->size] = new_neighbor;
        nt->size++;
    }
}

/**
 * get_neighbor_table_index:
 *  - return the index of node_address in the neighbor table nt.
 *
 * @param nt - neighbor table to search
 * @param node_address - node address to be searched for
 * @returns index of neighbor if found, '-1' otherwise
 */
int8_t get_neighbor_table_index(neighbor_table_t *nt, uint8_t node_address) {
    for(uint8_t i = 0; i < nt->size; i++) {
        if(nt->neighbors[i].id == node_address) {
            return i;
        }
    }
    return -1;
}

/**
 * add_id_to_neighbor_table:
 *  - add new_neighbor to the neighbor table
 *
 * @returns '1' if neighbor added, '-1' otherwise
 */
/*int8_t add_to_neighbor_table(neighbor_table_t *nt, neighbor_t new_neighbor) {
    if((nt->size < MAX_NEIGHBOR_TABLE) && (in_neighbor_table(nt, new_neighbor.id) == -1)) {
        uint8_t index = nt->size;
        nt->size++;
        nt->neighbors[index].id = new_neighbor.id;
        return 1;
    }
    return -1;
}*

/**
 * Print a neighbor table:
 */
void print_neighbor_table(neighbor_table_t *nt) {
    printf("[");
    for (uint8_t i=0; i < nt->size; i++) {
        uint8_t node_id = nt->neighbors[i].id;
        printf("%d", node_id);
        if (i < (nt->size-1))
            printf(", ");
    }
    printf("]\r\n");
}

/*** NEIGHBOR GRAPH OPERATIONS ***/
/**
 * id_in_neighbor_graph:
 *  - determine if node_address is in the neighbor graph
 *
 * @param ng - neighbor graph to be searched
 * @param node_address - node address to be searched for
 * @returns '1' if the node is found, '-1' otherwise
 */
int8_t in_neighbor_graph(neighbor_graph_t *ng, uint8_t node_address) {
    for(uint8_t i = 0; i < ng->size; i++) {
        if(ng->neighbor_tables[i].origin == node_address) {
            return 1;
        }
    }
    return -1;
}

/**
 * get_neighbor_graph_index:
 *  - return the index of node_address in the neighbor graph
 *
 * @param ng - neighbor graph to be searched
 * @param node_adress - node to be searched for
 * @returns index of node if found, '-1' otherwise
 */
int8_t get_neighbor_graph_index(neighbor_graph_t *ng, uint8_t node_address) {
    for(uint8_t i = 0; i< ng->size; i++) {
        if(ng->neighbor_tables[i].origin == node_address) {
            return i;
        }
    }
    return -1;
}

/**
 * add_to_neighbor_graph:
 *  - add new table to the neighbor graph
 *
 * @param ng - neighbor graph to which to add
 * @param new_table - new neighbor table to add to the neighbor graph
 * @returns '1' if add was successful, '-1' otherwise
 */
int8_t add_to_neighbor_graph(neighbor_graph_t *ng, neighbor_table_t *nt) {
    //printf("     add %d to neighbor_graph\r\n", nt->origin);
    if((ng->size < MAX_GRAPH)) {
        uint8_t index = ng->size;
        ng->size++;
        ng->neighbor_tables[index].size = nt->size;
        ng->neighbor_tables[index].origin = nt->origin;
        ng->neighbor_tables[index].neighbors[0].id = nt->neighbors[0].id;
        ng->neighbor_tables[index].neighbors[1].id = nt->neighbors[1].id;
        ng->neighbor_tables[index].neighbors[2].id = nt->neighbors[2].id;
        return 1;
    }
    return -1;
}

/** 
 * update_neighbor_graph:
 *  - update the neighbor graph with the current information
 * 
 * @param ng - neighbor graph to update
 * @param nt - neighbor table to update
 * @returns '1' if update was successful, '-1' otherwise
 */
int8_t update_neighbor_graph(neighbor_graph_t *ng, neighbor_table_t *nt) {
    int8_t in_ng, index;
    
    in_ng = in_neighbor_graph(ng, nt->origin);
    if(in_ng == -1) {
        add_to_neighbor_graph(ng, nt);
        return 1;
    } else {
        index = get_neighbor_graph_index(ng, nt->origin);
        ng->neighbor_tables[index].size = nt->size;
        ng->neighbor_tables[index].origin = nt->origin;
        ng->neighbor_tables[index].neighbors[0].id = nt->neighbors[0].id;
        ng->neighbor_tables[index].neighbors[1].id = nt->neighbors[1].id;
        ng->neighbor_tables[index].neighbors[2].id = nt->neighbors[2].id;
        return 1;
    }
    return -1;
}

/**
 * print_neighbor_graph:
 *  - print the current neighbor information
 * 
 * @param ng - neighbor graph to print
 */
void print_neighbor_graph(neighbor_graph_t *ng) {
    neighbor_table_t nt;
    nrk_kprintf(PSTR("\r\nNeighbor Graph:\r\n"));
    for(uint8_t i = 0; i < ng->size; i++) {
        nt = ng->neighbor_tables[i];
        printf("   Origin: %d -> Neighbors: %d %d %d\r\n", nt.origin, nt.neighbors[0].id, nt.neighbors[1].id, nt.neighbors[2].id);
    }
}