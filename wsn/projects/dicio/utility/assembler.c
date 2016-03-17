/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * assembler.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#include <assembler.h>


/*
Assemble packet to go to the server.
Right now the server is looking for ":"
*/
void assemble_serv_packet(uint8_t *tx_buf, packet *tx)
{
    switch(tx->type)
    {
        case MSG_CMD:
        {
            // msg_cmd from server has num_hops of 0.
            // cmd type message will only have "ON/OFF" payload value.
            // Will this ever happen? 
            sprintf(tx_buf, "%d:%d:%d:%d:%d", tx->source_id, tx->seq_num, tx->type, tx->num_hops, tx->payload[0]);
            break;
        }

        case MSG_DATA:
        {
            // NEED TO TEST
            sprintf(tx_buf, "%d:%d:%d:%d:%d,%d,%d", tx->source_id, tx->seq_num, tx->type, tx->num_hops, 
                (uint16_t)tx->payload[DATA_PWR_INDEX], (uint16_t)tx->payload[DATA_TEMP_INDEX],
                 (uint16_t)tx->payload[DATA_LIGHT_INDEX]);
        }
    }
}

/*
Assemble packet to go to the network.
Use network format.
*/
uint8_t assemble_packet(uint8_t *tx_buf, packet *tx)
{
    uint8_t length = 0;
    switch(tx->type)
    {
        case MSG_CMD:
        {
            length = 8;
            // msg_cmd from server has hop_num of 0.
            // cmd type message will only have "ON/OFF" payload value.
            tx_buf[0] = tx->source_id;
            tx_buf[1] = tx->seq_num;
            tx_buf[2] = tx->type;
            tx_buf[3] = tx->num_hops;
            tx_buf[4] = tx->payload[0];
            tx_buf[5] = tx->payload[1];
            tx_buf[6] = tx->payload[2];
            tx_buf[7] = tx->payload[3];
            break;
        }

        case MSG_DATA:
        {
            length = 10;
            // NEED TO TEST/DEVELOP!!
            // MISSING NUMBER HOPS
            // msg_cmd from server has hop_num of 0.
            // cmd type message will only have "ON/OFF" payload value.
            tx_buf[0] = tx->source_id;
            tx_buf[1] = tx->seq_num;
            tx_buf[2] = tx->type;
            tx_buf[3] = tx->num_hops;
            tx_buf[4] = tx->payload[0];
            tx_buf[5] = tx->payload[1];
            tx_buf[6] = tx->payload[2];
            tx_buf[7] = tx->payload[3];
            tx_buf[8] = tx->payload[4];
            tx_buf[9] = tx->payload[5];
        }
    }
    return length;
}