enum {
    NODE_SENSOR_VALUE
    NODE_NEIGHBORS
    GATEWAY2NODE
} msg_type

/*********************
 * packet structure
 *********************/
struct packet {
    msg_type type
    uint8_t source_id
    uint8_t num_hops
    // if msg_type is NODE_SENSOR_VALUE
    uint16_t sensor_value

    // if msg_type is NODE_NEIGHBORS
    uint16_T sensor_value // just in case
    uint8_t num_neighbors
    uint8_t[] neighbors

    // if msg_type is GATEWAY2NODE
    uint16_t sensor_sample_rate
    uint16_t neighbor_update_Rate
}

/*********************
 * node shorthand code
 *********************/

getTime(sample_time)
getTime(neighbor_time)

packet = receive_msg()

// if msg from another node, add to neighbors, increment num_hops, forward
// if msg from gateway, update sample/neighbor duration variables
handle_packet()

if sample_time > sample_duration
	send_sensor_msg()

if neighbor_time > neighbor_duration
	send_neighbor_msg()


/*********************
 * gateway shorthand code
 *********************/
neighbor_graph
node_sensor_values

packet = receive_message()

// if sensor value msg, save to node_sensor_values
// if neighbor msg, save to neighbor_graph
handle_packet()

// get new update rates from gui
sensor_update_rate = get_sensor_rate_from_gui()
neighbor_update_rate = get_neighbor_rate_from_gui()

send_msg(sensor_update_rate)
send_msg(neighbor_update_rate)

// send neighbor graph and sensor values to gui
send_sensor_values_to_gui()
send_neighbor_graph_to_gui()




