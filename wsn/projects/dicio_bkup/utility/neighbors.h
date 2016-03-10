/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * neighbors.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#ifndef __neighbors_h
#define __neighbors_h

#include <type_defs.h>

/*** NEIGHBOR TABLE OPERATIONS ***/
void init_neighbor_table(neighbor_table_t *nt);
int8_t in_neighbor_table(neighbor_table_t *nt, uint8_t node_address);
int8_t get_neighbor_table_index(neighbor_table_t *nt, uint8_t node_address);
void add_neighbor(neighbor_table_t *nt, neighbor_t new_neighbor);
int8_t add_to_neighbor_table(neighbor_table_t *nt, neighbor_t new_neighbor);
void print_neighbor_table(neighbor_table_t *nt);

/*** NEIGHBOR GRAPH OPERATIONS ***/
int8_t in_neighbor_graph(neighbor_graph_t *ng, uint8_t node_address);
int8_t get_neighbor_graph_index(neighbor_graph_t *ng, uint8_t node_address);
int8_t add_to_neighbor_graph(neighbor_graph_t *ng, neighbor_table_t *new_table);
void print_neighbor_graph(neighbor_graph_t *ng);

#endif
