/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * packet_queue.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <packet_queue.h>

// packet_queue_init - initialize a packet queue
void packet_queue_init(packet_queue* pq) {
	pq->front = 0;
	pq->back = 0;
	pq->size = 0;
}

// push - push a packet onto the queue
void push(packet_queue *pq, packet *p) {

	// only push if there is room in the queue
	if(pq->size < MAX_PACKET_BUFFER) {
		// increment the size
		pq->size++;

		// update the header of the packet
		pq->buffer[pq->back].type = p->type;
		pq->buffer[pq->back].source_id = p->source_id;
		pq->buffer[pq->back].seq_num = p->seq_num;
		pq->buffer[pq->back].num_hops = p->num_hops;

		// copy the payload
		for(uint8_t i = 0; i < MAX_PACKET_BUFFER; i++) {
			pq->buffer[pq->back].payload[i]	= p->payload[i];
		}

		// increment the back appropriately
		pq->back++;
		pq->back %= MAX_PACKET_BUFFER;
	}
	return;
}

// pop - pop something off of the queue 
void pop(packet_queue *pq, packet *p) {
	// only pop if there is something in the queue
	if(pq->size > 0) {
		// decrement the size
		pq->size--;

		// update the header of the packet
		p->type = pq->buffer[pq->front].type;
		p->source_id = pq->buffer[pq->front].source_id;
		p->seq_num = pq->buffer[pq->front].seq_num;
		p->num_hops = pq->buffer[pq->front].num_hops;

		// copy the payload
		for(uint8_t i = 0; i < MAX_PACKET_BUFFER; i++) {
			p->payload[i] = pq->buffer[pq->front].payload[i];
		}

		// inrement the front properly
		pq->front++;
		pq->front %= MAX_PACKET_BUFFER;	
	}
}
