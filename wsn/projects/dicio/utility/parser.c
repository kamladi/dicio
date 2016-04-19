/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * parser.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <parser.h>

// print_packet - print packet to the terminal
void print_packet(packet *p)
{
    printf("[source_id: %d]", p->source_id);
    printf("[seq_num: %d]", (uint16_t)p->seq_num);
    printf("[msg_type: %d]", p->type);
    printf("[num_hops: %d]", p->num_hops);
    uint8_t *payload = p->payload;
    switch(p->type)
    {

        case MSG_NO_MESSAGE:
        {
            printf("[INVALID]\r\n");
            break;
        }
        case MSG_LOST:
        {
            printf("[%d]\r\n", 
                        payload[LOST_NODE_INDEX]);
            break;
        }

        case MSG_GATEWAY:
        {
            printf("[UNDEFINED]\r\n");
            break;
        }
        case MSG_DATA:
        {
            uint16_t data_pwr = ((payload[DATA_PWR_INDEX] << 8) | (payload[DATA_PWR_INDEX + 1]));
            uint16_t data_temp = ((payload[DATA_TEMP_INDEX] << 8) | (payload[DATA_TEMP_INDEX + 1]));
            uint16_t data_light = ((payload[DATA_LIGHT_INDEX] << 8) | (payload[DATA_LIGHT_INDEX + 1]));
            printf("[%d, %d, %d, %d]\r\n",
                        data_pwr, data_temp, data_light, 
                        payload[DATA_STATE_INDEX]);
            break;
        }
        case MSG_CMD:
        {
            uint16_t cmd_id = ((payload[0] << 8) | (payload[1]));
            printf("[%d, %d, %d\r\n]", cmd_id, payload[2], payload[3]);
            break;
        }
        case MSG_CMDACK:
        {
            uint16_t cmd_id = ((payload[CMDACK_CMDID_INDEX] << 8) | (payload[CMDACK_CMDID_INDEX + 1]));
            printf("[%d, %d]\r\n", 
                        cmd_id, 
                        payload[CMDACK_STATE_INDEX]);
            break;
        }
        case MSG_HAND:
        {
            printf("[%d, %d, %d, %d]\r\n", 
                        payload[HAND_CONFIG_ID_INDEX], 
                        payload[HAND_CONFIG_ID_INDEX + 1], 
                        payload[HAND_CONFIG_ID_INDEX + 2], 
                        payload[HAND_CONFIG_ID_INDEX + 3]);
            break;
        }
        
        case MSG_HANDACK:
        {
            printf("[%d, %d, %d, %d, %d]\r\n", payload[HANDACK_NODE_ID_INDEX], 
                        payload[HANDACK_CONFIG_ID_INDEX], 
                        payload[HANDACK_CONFIG_ID_INDEX + 1], 
                        payload[HANDACK_CONFIG_ID_INDEX + 2], 
                        payload[HANDACK_CONFIG_ID_INDEX + 3]);
            break;
        }
        case MSG_HEARTBEAT: {
            printf("\r\n"); 
            break;
        }
        default:{
            break;
        }
    }
}

// parse message - parse message src into parsed_packet
void parse_msg(packet *parsed_packet, uint8_t *src, uint8_t len)
{
    parsed_packet->source_id = src[HEADER_SRC_ID_INDEX];
    parsed_packet->seq_num = (src[HEADER_SEQ_NUM_INDEX] << 8) | (src[HEADER_SEQ_NUM_INDEX + 1]);
    parsed_packet->type = src[HEADER_TYPE_INDEX];
    parsed_packet->num_hops = src[HEADER_NUM_HOPS_INDEX];

    switch(parsed_packet->type)
    {
        case MSG_NO_MESSAGE: 
        {
            // undefined message - should throw an error
            break;
        }
        case MSG_LOST:
        {
            parsed_packet->payload[LOST_NODE_INDEX] = src[HEADER_SIZE];
            break;
        }
        case MSG_GATEWAY:
        {
            // undefined message
            break;
        }
        case MSG_DATA:
        {
            parsed_packet->payload[DATA_PWR_INDEX] = src[HEADER_SIZE];
            parsed_packet->payload[DATA_PWR_INDEX+1] = src[HEADER_SIZE + 1];
            parsed_packet->payload[DATA_TEMP_INDEX] = src[HEADER_SIZE + 2];
            parsed_packet->payload[DATA_TEMP_INDEX+1] = src[HEADER_SIZE + 3];
            parsed_packet->payload[DATA_LIGHT_INDEX] = src[HEADER_SIZE + 4];
            parsed_packet->payload[DATA_LIGHT_INDEX+1] = src[HEADER_SIZE + 5];
            parsed_packet->payload[DATA_STATE_INDEX]   = src[HEADER_SIZE + 6];
            break;
        }
        case MSG_CMD:
        {
            parsed_packet->payload[CMD_CMDID_INDEX] = src[HEADER_SIZE];
            parsed_packet->payload[CMD_CMDID_INDEX+1] = src[HEADER_SIZE + 1];
            parsed_packet->payload[CMD_NODE_ID_INDEX] = src[HEADER_SIZE + 2];
            parsed_packet->payload[CMD_ACT_INDEX] = src[HEADER_SIZE + 3];
            break;
        }
        case MSG_CMDACK:
        {
            parsed_packet->payload[CMDACK_CMDID_INDEX] = src[HEADER_SIZE];
            parsed_packet->payload[CMDACK_CMDID_INDEX+1] = src[HEADER_SIZE + 1];
            parsed_packet->payload[CMDACK_STATE_INDEX] = src[HEADER_SIZE + 2];
            break;
        }

        case MSG_HAND:
        {
            parsed_packet->payload[HAND_CONFIG_ID_INDEX] = src[HEADER_SIZE];
            parsed_packet->payload[HAND_CONFIG_ID_INDEX+1] = src[HEADER_SIZE + 1];
            parsed_packet->payload[HAND_CONFIG_ID_INDEX+2] = src[HEADER_SIZE + 2];
            parsed_packet->payload[HAND_CONFIG_ID_INDEX+3] = src[HEADER_SIZE + 3];
            break;
        }

        case MSG_HANDACK:
        {
            parsed_packet->payload[HANDACK_NODE_ID_INDEX] = src[HEADER_SIZE];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX] = src[HEADER_SIZE + 1];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX+1] = src[HEADER_SIZE + 2];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX+2] = src[HEADER_SIZE + 3];
            parsed_packet->payload[HANDACK_CONFIG_ID_INDEX+3] = src[HEADER_SIZE + 4];
            break;
        }

        case MSG_HEARTBEAT:
        {
            break;
        }

        default:{
            printf("invalid msg_type \r\n");
        }
    }
}
