
#include <packet_queue.h>

void packet_queue_init(packet_queue* pq) {
	pq->front = 0;
	pq->back = 0;
	pq->size = 0;
}

void push(packet_queue* pq, packet* p) {
	if(pq->size < MAX_PACKET_BUFFER) {
		pq->size++;

		pq->buffer[pq->back].type 			= p->type;
		pq->buffer[pq->back].source_id 		= p->source_id;
		pq->buffer[pq->back].seq_num 		= p->seq_num;
		pq->buffer[pq->back].num_hops 		= p->num_hops;
		pq->buffer[pq->back].light_value 	= p->light_value;
		pq->buffer[pq->back].sensor_sample_rate 	= p->sensor_sample_rate;
		pq->buffer[pq->back].neighbor_update_rate 	= p->neighbor_update_rate;

		pq->back++;
		pq->back %= MAX_PACKET_BUFFER;
	}
	return;
}

void pop(packet_queue* pq, packet* p) {
	if(pq->size > 0) {
		p->type 		= pq->buffer[pq->front].type;
		p->source_id 	= pq->buffer[pq->front].source_id;
		p->seq_num 		= pq->buffer[pq->front].seq_num;
		p->num_hops 	= pq->buffer[pq->front].num_hops;
		p->light_value 	= pq->buffer[pq->front].light_value;
		p->sensor_sample_rate 		= pq->buffer[pq->front].sensor_sample_rate;
		p->neighbor_update_rate 	= pq->buffer[pq->front].neighbor_update_rate;	

		pq->size--;
		pq->front++;
		pq->front %= MAX_PACKET_BUFFER;	
	}
}
