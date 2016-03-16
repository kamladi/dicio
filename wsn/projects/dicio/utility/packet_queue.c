
#include <packet_queue.h>

void packet_queue_init(packet_queue* pq) {
	pq->front = 0;
	pq->back = 0;
	pq->size = 0;
}

// NEED TO test for all types of payload
void push(packet_queue* pq, packet* p) {
	if(pq->size < MAX_PACKET_BUFFER) {
		pq->size++;

		pq->buffer[pq->back].type 			= p->type;
		pq->buffer[pq->back].source_id 		= p->source_id;
		pq->buffer[pq->back].seq_num 		= p->seq_num;
		pq->buffer[pq->back].num_hops 		= p->num_hops;

		pq->buffer[pq->back].payload[0]	= p->payload[0];
		pq->buffer[pq->back].payload[1]	= p->payload[1];
		pq->buffer[pq->back].payload[2]	= p->payload[2];
		pq->buffer[pq->back].payload[3]	= p->payload[3];
		pq->buffer[pq->back].payload[4]	= p->payload[4];
		pq->buffer[pq->back].payload[5]	= p->payload[5];
		pq->buffer[pq->back].payload[6]	= p->payload[6];
		pq->buffer[pq->back].payload[7]	= p->payload[7];


		pq->back++;
		pq->back %= MAX_PACKET_BUFFER;
	}
	return;
}

// NEED To test for all types of payload
void pop(packet_queue* pq, packet* p) {
	if(pq->size > 0) {
		p->type 		= pq->buffer[pq->front].type;
		p->source_id 	= pq->buffer[pq->front].source_id;
		p->seq_num 		= pq->buffer[pq->front].seq_num;
		p->num_hops 	= pq->buffer[pq->front].num_hops;

		p->payload[0]	= pq->buffer[pq->front].payload[0];
		p->payload[1]	= pq->buffer[pq->front].payload[1];
		p->payload[2]	= pq->buffer[pq->front].payload[2];
		p->payload[3]	= pq->buffer[pq->front].payload[3];
		p->payload[4]	= pq->buffer[pq->front].payload[4];
		p->payload[5]	= pq->buffer[pq->front].payload[5];
		p->payload[6]	= pq->buffer[pq->front].payload[6];
		p->payload[7]	= pq->buffer[pq->front].payload[7];

		pq->size--;
		pq->front++;
		pq->front %= MAX_PACKET_BUFFER;	
	}
}
