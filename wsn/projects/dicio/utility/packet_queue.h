

#ifndef __packet_queue_h
#define __packet_queue_h

#include <type_defs.h>

void packet_queue_init(packet_queue* pq);
void push(packet_queue* pq, packet* p);
void pop(packet_queue* pq, packet* p);

#endif