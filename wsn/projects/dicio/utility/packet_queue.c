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
		/*
		for(uint8_t i = 0; i < MAX_PAYLOAD_SIZE; i++) {
			pq->buffer[pq->back].payload[i]	= p->payload[i];
		}*/
		pq->buffer[pq->back].payload[0]	= p->payload[0];
		pq->buffer[pq->back].payload[1]	= p->payload[1];
		pq->buffer[pq->back].payload[2]	= p->payload[2];
		pq->buffer[pq->back].payload[3]	= p->payload[3];
		pq->buffer[pq->back].payload[4]	= p->payload[4];
		pq->buffer[pq->back].payload[5]	= p->payload[5];
		pq->buffer[pq->back].payload[6]	= p->payload[6];
		pq->buffer[pq->back].payload[7]	= p->payload[7];


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
		/*for(uint8_t i = 0; i < MAX_PAYLOAD_SIZE; i++) {
			p->payload[i] = pq->buffer[pq->front].payload[i];
		}*/
		p->payload[0] = pq->buffer[pq->front].payload[0];
		p->payload[1] = pq->buffer[pq->front].payload[1];
		p->payload[2] = pq->buffer[pq->front].payload[2];
		p->payload[3] = pq->buffer[pq->front].payload[3];
		p->payload[4] = pq->buffer[pq->front].payload[4];
		p->payload[5] = pq->buffer[pq->front].payload[5];
		p->payload[6] = pq->buffer[pq->front].payload[6];
		p->payload[7] = pq->buffer[pq->front].payload[7];


		// inrement the front properly
		pq->front++;
		pq->front %= MAX_PACKET_BUFFER;	
	}
}

// atomic_size - get the size of the queue atomically
uint8_t atomic_size(packet_queue *pq, nrk_sem_t *mux) {
	uint8_t toReturn;
	nrk_sem_pend(mux); {
		toReturn = pq->size;
	}
	nrk_sem_post(mux);
	return toReturn;
}

// atomic_push - push onto the queue atomically
void atomic_push(packet_queue *pq, packet *p, nrk_sem_t *mux) {
  nrk_sem_pend(mux); {
  	push(pq, p);
  }
  nrk_sem_post(mux);
}

// atomic_pop - pop onto the queue atomically
void atomic_pop(packet_queue *pq, packet *p, nrk_sem_t *mux) {
	nrk_sem_pend(mux); {
		pop(pq, p);
	}
	nrk_sem_post(mux);
}
