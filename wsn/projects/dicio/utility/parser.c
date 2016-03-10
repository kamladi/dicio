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
    printf("msg_type: %d\r\n", p->type);
    printf("source_id: %d\r\n", p->source_id);
    printf("seq_num: %d\r\n", p->seq_num);
    printf("num_hops: %d\r\n", p->num_hops);
    switch(p->type)
    {
        case MSG_NODE_SENSOR_VALUE:
        {
            printf("light value: %d\r\n", p->light_value);
            break;
        }
        case MSG_GATEWAY:
        {
            printf("sample_rate: %d\r\n", p->sensor_sample_rate);
            printf("neighbor_rate: %d\r\n", p->neighbor_update_rate);
            break;
        }
    }
}

// start_index is index in input of location after opening bracket.
uint8_t parse_comma(char *output, char *input, uint8_t start_index)
{
    memset(output, 0, MAX_NEIGHBOR_BUF_SIZE);
    uint8_t pos = start_index;
    uint8_t item_length = 0;
    while(pos < MAX_BUF_SIZE && input[pos] != ',' && input[pos] != '\0')
    {
        item_length ++;
        pos ++;
    }
    //memcpy(output, input + start_index, item_length);
    strncpy(output, input + start_index, item_length);
    return pos;
}

// start_index is index in input of location after opening bracket.
uint8_t parse_bracket(char *output, char *input, uint8_t start_index)
{
    memset(output, 0, MAX_BUF_SIZE);
    uint8_t pos = start_index;
    uint8_t item_length = 0;
    while(pos < MAX_BUF_SIZE && input[pos] != ']' && input[pos] != '\0')
    {
        item_length ++;
        pos ++;
    }
    //memcpy(output, input + start_index, item_length);
    strncpy(output, input + start_index, item_length);
    return pos;
}

/*
Function : parse_msg(packet *parsed_buf, uint8_t *src, uint8_t len, msg_type type)

Input parameters:
parsed_packet - pointer to the output packet struct.
src - the pointer to the received data buffer
len - the length of the received data buffer
msg_type - the type of message
*/
void parse_msg(packet *parsed_packet, char *src, uint8_t len)
{
    uint8_t pos = 0;
    uint8_t item_length = 0;
    char temp_buf[MAX_BUF_SIZE];
    msg_type type = src[1] - '0';
    parsed_packet->type = type;
    
    // parse source id
    parsed_packet->source_id = (src[4] - '0');

    // parse sequence number
    pos = 6;
    item_length = 0;
    pos = parse_bracket(temp_buf, src, 7);
    parsed_packet->seq_num = atoi(temp_buf);

    // parse num_hops
    pos += 2; // skip ']['
    pos = parse_bracket(temp_buf, src, pos);
    parsed_packet->num_hops = atoi(temp_buf);
    
    switch(type)
    {
        case MSG_NODE_SENSOR_VALUE:
        {
            // parse light value
            pos += 2; // skip
            pos = parse_bracket(temp_buf, src, pos);
            parsed_packet->light_value = atoi(temp_buf);
            break;
        }

        case MSG_GATEWAY:
        {
            // parse sample_rate
            pos += 2;
            pos = parse_bracket(temp_buf, src, pos);
           // printf("sample_rate string %s \r\n", temp_buf);
            parsed_packet->sensor_sample_rate = atoi(temp_buf);
            
            // parse neighbor_rate
            pos += 2; // skip ']['
            pos = parse_bracket(temp_buf, src, pos);
            //printf("neighbor_rate string %s \r\n", temp_buf);
            parsed_packet->neighbor_update_rate = atoi(temp_buf);
            
            break;
        }

        default:{printf("invalid type\r\n");}
    }
}