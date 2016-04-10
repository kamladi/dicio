/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
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
        // this should never happen...TODO: throw an error
        case MSG_NO_MESSAGE:
        {
            break;
        }
        // lost node message - indicate the loss of a particular node in the system
        case MSG_LOST:
        {
            sprintf(tx_buf, "%d:%d:%d:%d:%d,", tx->source_id, tx->seq_num, tx->type, tx->num_hops,
                tx->payload[LOST_NODE_INDEX]);
            break;
        }
        // mesage from gateway -> undefined
        case MSG_GATEWAY:
        {
            break;
        }
        // data message 
        case MSG_DATA:
        {
            sprintf(tx_buf, "%d:%d:%d:%d:%d,%d,%d,%d", tx->source_id, (uint16_t)tx->seq_num, tx->type, tx->num_hops, 
                (uint16_t)tx->payload[DATA_PWR_INDEX], (uint16_t)tx->payload[DATA_TEMP_INDEX],
                 (uint16_t)tx->payload[DATA_LIGHT_INDEX], tx->payload[DATA_STATE_INDEX]);
            break;
        }
        // command message ... this will never happen. (Commands come from the server!)
        case MSG_CMD:
        {
            break;
        }
        // command acknowledgement
        case MSG_CMDACK:
        {
            sprintf(tx_buf, "%d:%d:%d:%d:%d,%d", tx->source_id, tx->seq_num, tx->type, tx->num_hops,
                (uint16_t)tx->payload[CMDACK_CMDID_INDEX], tx->payload[CMDACK_STATE_INDEX]);
            break;
        }
        // handshake request ... this will never happend (Server only receives HANDACKs)
        case MSG_HAND:
        {
            break;
        }
        // hanshake acknowledgement - sent to the server to indicate a new node
        case MSG_HANDACK:
        {
            uint16_t hardware_config_1 = ((tx->payload[HANDACK_CONFIG_ID_INDEX] << 8) | (tx->payload[HANDACK_CONFIG_ID_INDEX+1]));
            uint16_t hardware_config_2 = ((tx->payload[HANDACK_CONFIG_ID_INDEX+2] << 8) | tx->payload[HANDACK_CONFIG_ID_INDEX + 3]);
            
            sprintf(tx_buf, "%d:%d:%d:%d:%d,%u,%u", tx->source_id, (uint16_t)tx->seq_num, tx->type, tx->num_hops,
                tx->payload[HANDACK_NODE_ID_INDEX], hardware_config_1, hardware_config_2);
            break;
        }
        // heart beat message - so the server knows the gateway is still alive
        case MSG_HEARTBEAT:
        {
            sprintf(tx_buf, "%d:%d:%d:%d:,", tx->source_id, tx->seq_num, tx->type, tx->num_hops);
            break;
        }
        default:
            break;
    }
}

/*
Assemble packet to go to the network.
Use network format.
*/
uint8_t assemble_packet(uint8_t *tx_buf, packet *tx)
{
    // common to all packets 
    uint8_t length = 0;
    tx_buf[0] = tx->source_id;
    tx_buf[1] = (tx->seq_num >> 8) & 0xff;
    tx_buf[2] = tx->seq_num & 0xff;
    tx_buf[3] = tx->type;
    tx_buf[4] = tx->num_hops;

    switch(tx->type)
    {
        // this should never happen....TODO: Throw an error.
        case MSG_NO_MESSAGE: 
        {
            break;
        }
        // message from the gateway -> undefined
        case MSG_GATEWAY: 
        {
            break;
        }
        // lost node message - this will never happen. This message only gets sent to the server
        case MSG_LOST: 
        {
            break;
        }
        // data message - to get data back to the server
        case MSG_DATA:
        {
            length = 12;
            // power value (2 bytes)
            tx_buf[HEADER_SIZE] = tx->payload[DATA_PWR_INDEX];
            tx_buf[HEADER_SIZE + 1] = tx->payload[DATA_PWR_INDEX + 1];
            // temperature value (2 bytes)
            tx_buf[HEADER_SIZE + 2] = tx->payload[DATA_TEMP_INDEX];
            tx_buf[HEADER_SIZE + 3] = tx->payload[DATA_TEMP_INDEX + 1];
            // light value (2 bytes)
            tx_buf[HEADER_SIZE + 4] = tx->payload[DATA_LIGHT_INDEX];
            tx_buf[HEADER_SIZE + 5] = tx->payload[DATA_LIGHT_INDEX + 1];
            // current state (ON/OFF) (1 byte)
            tx_buf[HEADER_SIZE + 6] = tx->payload[DATA_STATE_INDEX];
            break;
        }
        // command message - sent out to a particular node with a particular command ID
        case MSG_CMD:
        {
            length = 9;
            // command ID (2 bytes)
            tx_buf[HEADER_SIZE] = tx->payload[CMD_CMDID_INDEX];
            tx_buf[HEADER_SIZE + 1] = tx->payload[CMD_CMDID_INDEX + 1];
            // node ID (1 byte)
            tx_buf[HEADER_SIZE + 2] = tx->payload[CMD_NODE_ID_INDEX];
            // action (1 byte)
            tx_buf[HEADER_SIZE + 3] = tx->payload[CMD_ACT_INDEX];
            break;
        }
        // command acknowledgment - send from a node back to the server to confirm actuation 
        case MSG_CMDACK:
        {
            length = 8;
            // command ID (2 bytes)
            tx_buf[HEADER_SIZE] = tx->payload[CMDACK_CMDID_INDEX];
            tx_buf[HEADER_SIZE + 1] = tx->payload[CMDACK_CMDID_INDEX + 1];
            // node state (1 byte)
            tx_buf[HEADER_SIZE + 2] = tx->payload[CMDACK_STATE_INDEX];
            break;
        }
        // handshake request - send to the gateway to gain access to the network
        case MSG_HAND:
        {
            length = 9;
            // hardware configuration (4 bytes)
            tx_buf[HEADER_SIZE] = tx->payload[HAND_CONFIG_ID_INDEX];
            tx_buf[HEADER_SIZE + 1] = tx->payload[HAND_CONFIG_ID_INDEX + 1];
            tx_buf[HEADER_SIZE + 2] = tx->payload[HAND_CONFIG_ID_INDEX + 2];
            tx_buf[HEADER_SIZE + 3] = tx->payload[HAND_CONFIG_ID_INDEX + 3];
            break;
        }

        // handshake acknowledgement - response to handshake request
        case MSG_HANDACK:
        {
            length = 10;
            // MACADDR of the requesting node (1 byte)
            tx_buf[HEADER_SIZE] = tx->payload[HANDACK_NODE_ID_INDEX];
            // handware configuration (4 bytes)
            tx_buf[HEADER_SIZE + 1] = tx->payload[HANDACK_CONFIG_ID_INDEX];
            tx_buf[HEADER_SIZE + 2] = tx->payload[HANDACK_CONFIG_ID_INDEX + 1];
            tx_buf[HEADER_SIZE + 3] = tx->payload[HANDACK_CONFIG_ID_INDEX + 2];
            tx_buf[HEADER_SIZE + 4] = tx->payload[HANDACK_CONFIG_ID_INDEX + 3];
            break;
        }
        // heartbeat message - from gateway to nodes so the nodes know they 
        //  are still part of the system
        case MSG_HEARTBEAT:
        {
            // no payload!
            length = 5;
            break;
        }
        default:
            break;
    }
    return length;
}
