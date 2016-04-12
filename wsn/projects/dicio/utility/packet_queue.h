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
void atomicPush(packet_queue *pq, packet *p, nrk_sem_t *mux);
void pop(packet_queue* pq, packet* p);
void atomicPop(packet_queue *pq, packet *p, nrk_sem_t *mux);

#endif