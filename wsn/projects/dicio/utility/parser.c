/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * parser.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <parser.h>

void print_packet(packet *p)
{
    printf("[source_id: %d]", p->source_id);
    printf("[seq_num: %d]", (uint16_t)p->seq_num);
    printf("[msg_type: %d]", p->type);
    printf("[num_hops: %d]", p->num_hops);
    uint8_t *payload = p->payload;
    switch(p->type)
    {
        case MSG_CMD:
        {
            uint16_t cmdId = (payload[CMD_ID_INDEX] << 8) | payload[CMD_ID_INDEX+1];
            printf("[payload:%d,%d,%d]\r\n", cmdId,
                payload[CMD_NODE_ID_INDEX], payload[CMD_ACT_INDEX]);
            break;
        }

        case MSG_DATA:
        {
            uint16_t power = (payload[DATA_PWR_INDEX] << 8) | payload[DATA_PWR_INDEX+1];
            uint16_t temp = (payload[DATA_TEMP_INDEX] << 8) | payload[DATA_TEMP_INDEX+1];
            uint16_t light = (payload[DATA_LIGHT_INDEX] << 8) | payload[DATA_LIGHT_INDEX+1];
            printf("[payload:%d, %d, %d, %d]\r\n", power, temp, light, payload[DATA_STATE_INDEX]);
            break;
        }

        case MSG_CMDACK:
        {
            printf("[payload:%d, %d]\r\n",
                (uint16_t)payload[CMDACK_ID_INDEX], payload[CMDACK_STATE_INDEX]);
            break;
        }

        case MSG_HAND:
        {
            printf("[payload:%d, %d, %d, %d]\r\n", payload[HAND_CONFIG_ID_INDEX], payload[HAND_CONFIG_ID_INDEX + 1],
                payload[HAND_CONFIG_ID_INDEX + 2], payload[HAND_CONFIG_ID_INDEX + 3]);
            // what is the payload going to look like here?
            break;
        }

        case MSG_HANDACK:
        {
            printf("[payload:%d, %d, %d, %d, %d]\r\n", payload[HANDACK_NODE_ID_INDEX], payload[HANDACK_CONFIG_ID_INDEX],
                payload[HANDACK_CONFIG_ID_INDEX + 1], payload[HANDACK_CONFIG_ID_INDEX + 2], payload[HANDACK_CONFIG_ID_INDEX + 3]);
            break;
        }

        default:{
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
    parsed_packet->seq_num = (src[1] << 8) | (src[2]);
    parsed_packet->type = src[3];
    parsed_packet->num_hops = src[4];

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
            parsed_packet->payload[CMD_ID_INDEX] = src[5];
            parsed_packet->payload[CMD_ID_INDEX+1] = src[6];
            parsed_packet->payload[CMD_NODE_ID_INDEX] = src[7];
            parsed_packet->payload[CMD_ACT_INDEX] = src[8];
            break;
        }

        case MSG_DATA:
        {
            parsed_packet->payload[DATA_PWR_INDEX] = src[5];
            parsed_packet->payload[DATA_PWR_INDEX+1] = src[6];
            parsed_packet->payload[DATA_TEMP_INDEX] = src[7];
            parsed_packet->payload[DATA_TEMP_INDEX+1] = src[8];
            parsed_packet->payload[DATA_LIGHT_INDEX] = src[9];
            parsed_packet->payload[DATA_LIGHT_INDEX+1] = src[10];
            parsed_packet->payload[DATA_STATE_INDEX]   = src[11];
            //printf("payload:%d,%d,%d\r\n", src[4],src[6],src[8]);
            break;
        }

        case MSG_CMDACK:
        {
            parsed_packet->payload[CMDACK_ID_INDEX] = src[5];
            parsed_packet->payload[CMDACK_ID_INDEX+1] = src[6];
            parsed_packet->payload[CMDACK_STATE_INDEX] = src[7];
            break;
        }

        case MSG_HAND:
        {
            parsed_packet->payload[HAND_CONFIG_ID_INDEX] = src[5];
            parsed_packet->payload[HAND_CONFIG_ID_INDEX+1] = src[6];
            parsed_packet->payload[HAND_CONFIG_ID_INDEX+2] = src[7];
            parsed_packet->payload[HAND_CONFIG_ID_INDEX+3] = src[8];
            // no information is stored in payload
            break;
        }

        case MSG_HANDACK: // received hand ack
        {
            parsed_packet->payload[HANDACK_NODE_ID_INDEX] = src[5];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX] = src[6];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX+1] = src[7];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX+2] = src[8];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX+3] = src[9];
            break;
        }

        default:{
            printf("invalid msg_type \r\n");
        }
    }
}
