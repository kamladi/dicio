/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * main.c (node)
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

/*** INCLUDE STATEMENTS ***/
// standard nrk
#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ff_basic_sensor.h>
// this package
#include <assembler.h>
#include <light_pool.h>
#include <neighbors.h>
#include <parser.h>
#include <sample_sensor.h>
#include <sequence_pool.h>

/*** DEFINE STATMENTS **/
#define MAC_ADDR  0x0007

// declare node's neighbor table
neighbor_table_t local_neighbor_table;
uint8_t neighbor_count; // count of items in neighbor table
uint16_t local_seq_count;
sequence_pool_t seq_pool;

/* The duration of the task might be shorter than sample periods.
  Therefore the start times need to persist through several task periods.
-> Add logic to get start time for a new period.
*/
nrk_time_t sensor_time_limit;
nrk_time_t neighbor_time_limit;


/*** TASKS **/
nrk_task_type HOP_TASK;
NRK_STK hop_task_stack[NRK_APP_STACKSIZE];
void hop_task (void);

void nrk_create_taskset ();

/*** BUFFER INSTANTIATION ***/
uint8_t hop_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t cmd[RF_MAX_PAYLOAD_SIZE];

/*** GLOBAL VARIABLES **/
uint16_t light_baseline;

void nrk_register_drivers();

int main ()
{
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_init ();

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

  nrk_time_set (0, 0);

  bmac_task_config ();

  nrk_register_drivers();
  nrk_create_taskset ();
  nrk_start ();

  return 0;
}

/**
 * receive_message - receive a message and return the type of message received
 * FORMAT [msg_type][id][seq_num][hop_num][neighbor][sens_value]
 * @returns message_type for node to act on
 */

void receive_message(packet *new_packet)
{
  // local variable declarations
  uint8_t *rx_cpy;
  uint16_t node_id;
  int8_t rssi;
  uint8_t len = 0;

  msg_type return_message;

  node_id = 0;

  // Get the RX packet -> if not ready then return as such
  nrk_led_set (ORANGE_LED);
  if(bmac_rx_pkt_ready()) 
  {
    // get packet and store in rx_cpy
    rx_cpy = bmac_rx_pkt_get(&len, &rssi);
    printf("RX: %s\r\n", rx_cpy);
  
    parse_msg(new_packet, rx_cpy, len);
  
    print_packet(new_packet);
    if(new_packet->type == MSG_GATEWAY)
    {
      // update new timer parameters
      sensor_time_limit.secs = new_packet->sensor_sample_rate;
      neighbor_time_limit.secs = new_packet->neighbor_update_rate;
    }
  
    // check to see if packet is from the originator (hop = 0), if so.. add him to your table he's your new neighbor!
    if(new_packet->num_hops == 0)
      {
      print_neighbor_table(&local_neighbor_table);
      // now check to see if the source_id of packet is in local neighbor table
      int8_t source_node_in_neighbor_table = in_neighbor_table(&local_neighbor_table, new_packet->source_id);
      
      if(source_node_in_neighbor_table == -1)
      {
        // source node for this packet has not been seen before,
        // add to local neighbor table.
        neighbor_t new_neighbor;
        new_neighbor.id = new_packet->source_id;
        add_neighbor(&local_neighbor_table, new_neighbor);
      }
      
      print_neighbor_table(&local_neighbor_table);
  
      nrk_led_clr(ORANGE_LED);
      // Release the RX buffer so future packets can arrive
      memset(rx_cpy, 0, sizeof(rx_cpy));
    }
    bmac_rx_pkt_release ();
  }
  return;
}


/*****************************        hop_task function loop               *************************
       *
****************************************************************************************************/


uint16_t light;
void hop_task ()
{
  // local variable instantiation
  int8_t val;
  msg_type received;
  // new packet is the packet received (parsed into type packet)
  packet new_packet;
  uint8_t new_node = NONE;
  int8_t in_seq_pool;
  uint16_t local_seq_num;

  // init bmac on channel 13
  bmac_init (13);

  // This sets the next RX buffer.
  // This can be called at anytime before releasing the packet
  // if you wish to do a zero-copy buffer switch
  bmac_rx_pkt_set_buffer(rx_buf, RF_MAX_PAYLOAD_SIZE);

    //Timer management
  nrk_time_t neighbor_start_time, current_time;
  nrk_time_t sensor_start_time;

  sensor_time_limit.secs = 10;
  neighbor_time_limit.secs = 15;

  //init_neighbor_table(&local_neighbor_table);
  while(1)
  {
   /*******************************     Section to handle received message         *************************
    *
    ****************************************************************************************************/
    receive_message(&new_packet);
    
    // check to our sequence pool to see if we have already received this packet
    in_seq_pool = in_sequence_pool(&seq_pool, new_packet.source_id);
    if((in_seq_pool == -1) && (new_packet.source_id != MAC_ADDR)) {
      add_to_sequence_pool(&seq_pool, new_packet.source_id, new_packet.seq_num);
      new_node = NODE_FOUND;
    }
    
    local_seq_num = get_sequence_number(&seq_pool, new_packet.source_id);
    if((new_packet.source_id != MAC_ADDR) && (new_packet.source_id != GATEWAY_ID) &&
        ((new_packet.seq_num > local_seq_num) || (new_node == NODE_FOUND))) {
      
      // update the sequence pool
      update_sequence_pool(&seq_pool, new_packet.source_id, new_packet.seq_num);
      new_node = NONE;

      // increase the hop counter and forward the packet
      new_packet.num_hops += 1;      // now forward the packet!
      assemble_packet(&tx_buf,&new_packet);
      val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);      
    }

   /*************************** Section to check intervals and send local values  *************************
         *
  *********************************************************************************************************/
    // get the current tick time for comparison
    nrk_time_get(&current_time);

    nrk_status_t status;
    nrk_time_t delta_sensor;
    nrk_time_t delta_neighbor;
    status = nrk_time_sub (&delta_sensor,current_time, sensor_start_time);
    uint16_t difference_s = delta_sensor.secs;
    if(delta_sensor.secs >= sensor_time_limit.secs)
    {
      light = sample_light();

      // now send the value
      sprintf (tx_buf, "[%d][%d][%d][%d][%d]", MSG_NODE_SENSOR_VALUE, MAC_ADDR, local_seq_count, 0, light);
      val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);

      local_seq_count ++;
      nrk_time_get(&sensor_start_time);
    }

    nrk_time_get(&current_time);
    status = nrk_time_sub (&delta_neighbor,current_time, neighbor_start_time);
    uint16_t difference_n = delta_neighbor.secs;
    //printf("current time %d \r\n", current_time.secs);
    if(delta_neighbor.secs >= neighbor_time_limit.secs)
    {
      // send neighbortable
      sprintf (tx_buf, "[%d][%d][%d][%d][%d][%d,%d,%d]", MSG_NODE_NEIGHBORS, MAC_ADDR, local_seq_count,
      0, light, local_neighbor_table.neighbors[0].id, local_neighbor_table.neighbors[1].id, local_neighbor_table.neighbors[2].id);
      val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);
      
      // Reset neighbor table
      init_neighbor_table(&local_neighbor_table);
      
      local_seq_count ++;
      nrk_time_get(&neighbor_start_time);
    }

    nrk_wait_until_next_period();
  }
}

void nrk_create_taskset ()
{
  nrk_task_set_stk( &HOP_TASK, hop_task_stack, NRK_APP_STACKSIZE);
  HOP_TASK.task = hop_task;
  HOP_TASK.prio = 2;
  HOP_TASK.FirstActivation = TRUE;
  HOP_TASK.Type = BASIC_TASK;
  HOP_TASK.SchType = PREEMPTIVE;
  HOP_TASK.period.secs = 1;
  HOP_TASK.period.nano_secs = 0;
  HOP_TASK.cpu_reserve.secs = 1;
  HOP_TASK.cpu_reserve.nano_secs = 0;
  HOP_TASK.offset.secs = 0;
  HOP_TASK.offset.nano_secs = 0;
  nrk_activate_task (&HOP_TASK);

  nrk_kprintf( PSTR("Create Done\r\n") );
}
void nrk_register_drivers()
{
  int8_t val;

  // Register the Basic FireFly Sensor device driver
  // Make sure to add:
  //     #define NRK_MAX_DRIVER_CNT
  //     in nrk_cfg.h
  // Make sure to add:
  //     SRC += $(ROOT_DIR)/src/drivers/platform/$(PLATFORM_TYPE)/source/ff_basic_sensor.c
  //     in makefile
  val=nrk_register_driver( &dev_manager_ff3_sensors,FIREFLY_3_SENSOR_BASIC);
  if(val==NRK_ERROR) nrk_kprintf( PSTR("Failed to load my ADC driver\r\n") );

}