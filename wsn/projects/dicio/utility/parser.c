/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * parser.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#include <parser.h>

void print_packet(packet *p)
{
    printf("[source_id: %d]", p->source_id);
    printf("[seq_num: %d]", p->seq_num);
    printf("[msg_type: %d]", p->type);
    printf("[num_hops: %d]", p->num_hops);
    switch(p->type)
    {
        case MSG_CMD:
        {
            printf("[payload:%d,%d,%d]\r\n", (uint16_t)p->payload[0], p->payload[2], p->payload[3]);
            break;
        }

        case MSG_DATA:
        {
            printf("[payload:%d, %d, %d]\r\n", (uint16_t)p->payload[DATA_PWR_INDEX], 
                (uint16_t)p->payload[DATA_TEMP_INDEX],(uint16_t)p->payload[DATA_LIGHT_INDEX]);
            break;
        }

        case MSG_CMDACK:
        {
            break;
        }

        case MSG_HAND:
        {
            printf("[payload:]\r\n");
            // what is the payload going to look like here?
            break;
        }

        case MSG_HANDACK:
        {
            printf("[payload:%d]\r\n", p->payload[HANDACK_NODE_ID_INDEX]);
            break;
        }

        default:{
        }
    }
}

/*
Function : parse_serv_msg(packet *parsed_buf, uint8_t *src, uint8_t len, msg_type type)

Input parameters:
parsed_packet - pointer to the output packet struct.
src - the pointer to the received data buffer
len - the length of the received data buffer
msg_type - the type of message

packet format:
mac_addr:seq_num:msg_id:hop_num:payload
*/
void parse_serv_msg(packet *parsed_packet, uint8_t *src, uint8_t len)
{
    uint8_t pos = 0;
    uint8_t item = 0;
    uint8_t payload_index = 0;
    uint8_t temp_buf[MAX_BUF_SIZE];
    uint8_t temp_payload[MAX_PAYLOAD_SIZE];
    uint16_t value = 0;

    for (int x = 0; x < len; x ++){
        if(src[x] == ':')
        {
            value = atoi(temp_buf);
            //printf("parsed value = %d \r\n", value);
            switch(item)
            {
                case 0: // mac_addr
                {
                    parsed_packet->source_id = value;
                    break;
                }

                case 1: // seq_num
                {              
                     // get seq_num
                     parsed_packet->seq_num = value;
                    break;
                }

                case 2: // msg_id
                {
                    // get message type
                    parsed_packet->type = value;
                    break;
                }

                case 3: // hop_num
                {
                    // get message hop number
                     parsed_packet->num_hops = value;
                    break;
                }
            }
            // clear buffer
            for(uint8_t i = 0; i < pos; i ++)
            {
                temp_buf[i] = 0;
            }
            item += 1;
            pos = 0;
        }
        else{
            temp_buf[pos] = src[x];
            pos += 1;
        }

    }
    /*
    Payload has not been parsed into packet.
    Once the loop has gone through the length of the message, 
    the payload will be stored in temp_buf. 
    Need to parse payload depending on message type
    */
    item = 0;
    switch(parsed_packet->type)
    {
        case MSG_CMD:
        {
            for (int x = 0; x < MAX_PAYLOAD_SIZE; x ++){
                if(temp_buf[x] == ',')
                {
                    value = atoi(temp_payload);
                    switch(item)
                    {
                        case 0: // cmd_id
                        {
                            parsed_packet->payload[CMD_ID_INDEX] = value;
                            break;
                        }

                        case 1: // destination
                        {              
                             // get seq_num
                             parsed_packet->payload[CMD_NODE_ID_INDEX] = value;
                            break;
                        }

                        case 2: // action
                        {
                            // get message type
                            parsed_packet->payload[CMD_ACT_INDEX] = value;
                            break;
                        }
                    }
                    // clear buffer
                    for(uint8_t i = 0; i < payload_index; i ++)
                    {
                        temp_payload[i] = 0;
                    }
                    item += 1;
                    payload_index = 0;
                }
                else{
                    temp_payload[payload_index] = temp_buf[x];
                    payload_index += 1;
                }

            }
            break;
        }

        case MSG_DATA:
        {
            // should not be getting DATA type messages from server
            break;
        }

        case MSG_CMDACK:
        {
            // should not be getting command ack messages from server
            break;
        }

        case MSG_HAND:
        {
            // what is the payload going to look like here?
            break;
        }

        default:{
            printf("invalid msg_type \r\n");
        }
    }
}




/*
Function : parse_msg(packet *parsed_buf, uint8_t *src, uint8_t len, msg_type type)

Input parameters:
parsed_packet - pointer to the output packet struct.
src - the pointer to the received data buffer
len - the length of the received data buffer
msg_type - the type of message
*/
void parse_msg(packet *parsed_packet, uint8_t *src, uint8_t len)
{
    uint8_t pos = 0;
    uint8_t item = 0;
    uint8_t temp_buf[MAX_BUF_SIZE];

    parsed_packet->source_id = src[0];
    parsed_packet->seq_num = src[1];
    parsed_packet->type = src[2];
    parsed_packet->num_hops = src[3];

    /*
    Payload has not been parsed into packet.
    Once the loop has gone through the length of the message, 
    the payload will be stored in temp_buf. 
    Need to parse payload depending on message type
    */
    switch(parsed_packet->type)
    {
        case MSG_CMD:
        {
            parsed_packet->payload[0] = src[4];
            parsed_packet->payload[1] = src[5];
            parsed_packet->payload[2] = src[6];
            parsed_packet->payload[3] = src[7];
            break;
        }

        case MSG_DATA:
        {
            parsed_packet->payload[DATA_PWR_INDEX] = (uint16_t)src[4];
            parsed_packet->payload[DATA_TEMP_INDEX] = (uint16_t)src[6];
            parsed_packet->payload[DATA_LIGHT_INDEX] = (uint16_t)src[8];
            //printf("payload:%d,%d,%d\r\n", src[4],src[6],src[8]);
            break;
        }

        case MSG_CMDACK:
        {
            parsed_packet->payload[CMDACK_ID_INDEX] = (uint16_t)src[4];
            break;
        }

        case MSG_HAND:
        {
            // no information is stored in payload
            break;
        }

        case MSG_HANDACK: // received hand ack
        {
            parsed_packet->payload[HANDACK_NODE_ID_INDEX] = (uint16_t)src[4];
            // no need to do anything...?
            break;
        }

        default:{
            printf("invalid msg_type \r\n");
        }
    }
}