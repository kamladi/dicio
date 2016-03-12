/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * assembler.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#include <assembler.h>

void assemble_packet(uint8_t *tx_buf, packet *tx)
{
    switch(tx->type)
    {
        case MSG_NODE_SENSOR_VALUE:
        {
            sprintf (tx_buf, "[%d][%d][%d][%d][%d]", tx->type, tx->source_id, tx->seq_num, 
            tx->num_hops, tx->light_value);
            break;
        }
        
        case MSG_GATEWAY:
        {
          sprintf (tx_buf, "[%d][%d][%d][%d][%d][%d]", tx->type, tx->source_id, tx->seq_num, 
          tx->num_hops, tx->sensor_sample_rate, tx->neighbor_update_rate);
            break;
        }
    }
}