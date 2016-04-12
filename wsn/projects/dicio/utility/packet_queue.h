/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * packet_queue.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#ifndef __packet_queue_h
#define __packet_queue_h

#include <type_defs.h>

void packet_queue_init(packet_queue* pq);
void push(packet_queue* pq, packet* p);
void pop(packet_queue* pq, packet* p);
uint8_t atomic_size(packet_queue *pq, nrk_sem_t *mux);
void atomic_push(packet_queue *pq, packet *p, nrk_sem_t *mux);
void atomic_pop(packet_queue *pq, packet *p, nrk_sem_t *mux);

#endif