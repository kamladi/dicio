/*
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * main.c (gateway)
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

// INCLUDES
// standard nrk 
#include <nrk.h>
#include <nrk_events.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>
// this package
#include <assembler.h>
#include <light_pool.h>
#include <neighbors.h>
#include <packet_queue.h>
#include <parser.h>
#include <sequence_pool.h>
#include <type_defs.h>

// DEFINES
#define MAC_ADDR 2

// FUNCTION DECLARATIONS
uint8_t get_server_input(void);
void clear_serv_buf();
void rx_msg_task(void);
void tx_cmd_task(void);
void tx_data_task(void);
void tx_serv_task(void);
void nrk_create_taskset ();

// STATE ENUM
typedef enum {
  STATE_ON,
  STATE_OFF, 
} act_state;

// TASKS
nrk_task_type RX_MSG_TASK;
nrk_task_type TX_CMD_TASK;
nrk_task_type TX_DATA_TASK;
nrk_task_type SAMPLE_TASK;
nrk_task_type ACTUATE_TASK;

// TASK STACKS
NRK_STK rx_msg_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_cmd_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_data_task_stack[NRK_APP_STACKSIZE];
NRK_STK sample_task_stack[NRK_APP_STACKSIZE];
NRK_STK actuate_task_stack[NRK_APP_STACKSIZE];

// CURRENT STATE
act_state curr_state = STATE_OFF;

// BUFFERS
uint8_t net_rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t net_tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t net_tx_index = 0;
nrk_sem_t* net_tx_buf_mux;

// QUEUES / MUTEXES
packet_queue act_queue;
nrk_sem_t* act_queue_mux;
uint16_t last_command = 0;

packet_queue cmd_tx_queue;
nrk_sem_t* cmd_tx_queue_mux;

packet_queue data_tx_queue;
nrk_sem_t* data_tx_queue_mux;

packet_queue hand_rx_queue;
nrk_sem_t* hand_rx_queue_mux;

// SENSOR VALUES
uint8_t pwr_period;
uint8_t temp_period;
uint8_t light_period;
sensor_packet sensor_pkt;

// DRIVERS 
void nrk_register_drivers();

// SEQUENCE POOLS/NUMBER
sequence_pool_t seq_pool;
uint16_t seq_num = 0;
nrk_sem_t* seq_num_mux;

// GLOBAL FLAG
uint8_t print_incoming;
uint8_t blink_leds;

int main () {
  // setup ports/uart
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);
  nrk_init ();

  // clear all LEDs
  nrk_led_clr(0);
  nrk_led_clr(1);
  nrk_led_clr(2);
  nrk_led_clr(3);
    
  // print flag
  print_incoming = 1;
  blink_leds = 1;

  // mutexs
  net_tx_buf_mux    = nrk_sem_create(1, 6);
  act_queue_mux     = nrk_sem_create(1, 6);
  cmd_tx_queue_mux  = nrk_sem_create(1, 6);
  data_tx_queue_mux = nrk_sem_create(1, 6);
  hand_rx_queue_mux = nrk_sem_create(1, 6);
  seq_num_mux       = nrk_sem_create(1, 6);

  // sensor periods (in seconds)
  pwr_period = 30;
  temp_period = 35;
  light_period = 40;

  // packet queues
  packet_queue_init(&act_queue);
  packet_queue_init(&cmd_tx_queue);
  packet_queue_init(&data_tx_queue);
  packet_queue_init(&hand_rx_queue);

  // start running
  nrk_time_set(0, 0);
  bmac_task_config();
  nrk_create_taskset();
  bmac_init(13);
  nrk_start();
  return 0;
}

void clear_tx_buf(){
  for(uint8_t i = 0; i < net_tx_index; i++)
  {
    net_tx_buf[i] = 0;
  }
  net_tx_index = 0;
}

/**
 * rx_msg_task() - 
 *  receive messages from the network
 */
void rx_msg_task() {
  // local variable instantiation
  uint8_t LED_FLAG = 0;
  packet rx_packet;
  uint8_t len, rssi;
  uint8_t *local_buf;
  int8_t in_seq_pool;
  uint16_t local_seq_num;
  uint8_t new_node = NONE;

  // initialize network receive buffer
  bmac_rx_pkt_set_buffer(net_rx_buf, RF_MAX_PAYLOAD_SIZE);
  
  // Wait until bmac has started. This should be called by all tasks using bmac that do not call bmac_init()
  while (!bmac_started ()) {
    nrk_wait_until_next_period ();
  }
  
  // loop forever
  while(1) {
    // LED blinking - for debug
    if(blink_leds == BLINKLEDS) {
      LED_FLAG++;
      LED_FLAG%=2;
      if(LED_FLAG == 0) {
        nrk_led_set(0);
      } else {
        nrk_led_clr(0);
      }      
    }

    // only execute if there is a packet available
    if(bmac_rx_pkt_ready()) {
      // get the packet, parse and release
      parse_msg(&rx_packet, &net_rx_buf, len);
      local_buf = bmac_rx_pkt_get(&len, &rssi);
      bmac_rx_pkt_release ();  
      
      // print incoming packet if appropriate
      if(print_incoming == 1) {
        nrk_kprintf (PSTR ("rx pkt:\r\n"));
        print_packet(&rx_packet);     
      }
      
      // only receive the message if it's not from the gateway
      //  NOTE: this is required because the gateway will hear re-transmitted packets 
      //    originally from itself.
      if(rx_packet.source_id != MAC_ADDR) {

        // check to see if this node is in the sequence pool, if not then add it
        in_seq_pool = in_sequence_pool(&seq_pool, rx_packet.source_id);
        if(in_seq_pool == -1) {
          add_to_sequence_pool(&seq_pool, rx_packet.source_id, rx_packet.seq_num);
          new_node = NODE_FOUND;
        }
      
        // determine if we should act on this packet based on the sequence number
        local_seq_num = get_sequence_number(&seq_pool, rx_packet.source_id);
        if((rx_packet.seq_num > local_seq_num) || (new_node == NODE_FOUND)) {
          
          // update the sequence pool and reset the new_node flag
          update_sequence_pool(&seq_pool, rx_packet.source_id, rx_packet.seq_num);
          new_node = NONE;
          
          // put the message in the right queue based on the type
          switch(rx_packet.type) {
            case MSG_CMD:
            {
              /*
              command received -- either act or forward
              Payload format: [NN][D][A]
              NN: Command ID
              D: Destination Address
              A: Action!!!
              
              if command is for this node and hasn't been received yet, add it
              to the action queue. Otherwise, add it to the cmd_tx queue for 
              forwarding to other nodes.
                */
              if((last_command < (uint16_t)rx_packet.payload[CMD_ID_INDEX]) &&
                  (rx_packet.payload[CMD_NODE_ID_INDEX] == MAC_ADDR)) 
              {
                nrk_kprintf (PSTR ("packet for me!\r\n"));
                last_command = (uint16_t)rx_packet.payload[CMD_ID_INDEX]; // need to cast again here right?
                nrk_sem_pend(act_queue_mux);
                push(&act_queue, &rx_packet);
                nrk_sem_post(act_queue_mux);
              } 
              else 
              {
                nrk_sem_pend(cmd_tx_queue_mux);
                push(&cmd_tx_queue, &rx_packet);
                nrk_sem_post(cmd_tx_queue_mux);
              }
              break;
            }
            // command act received -- forward to the server
            case MSG_CMDACK:
              nrk_sem_pend(cmd_tx_queue_mux);
              push(&cmd_tx_queue, &rx_packet);
              nrk_sem_post(cmd_tx_queue_mux);
              break;
            // data received or command ack received -> forward to server
            case MSG_DATA:
              nrk_sem_pend(data_tx_queue_mux);
              push(&data_tx_queue, &rx_packet);
              nrk_sem_post(data_tx_queue_mux);
              break;
            // handshake message recieved -> deal with in handshake function
            case MSG_HAND:
              nrk_sem_pend(hand_rx_queue_mux);
              push(&hand_rx_queue, &rx_packet);
              nrk_sem_post(hand_rx_queue_mux);
              break;
            // gateway message -> for future expansion
            case MSG_GATEWAY:
              // do nothing...no messages have been defined with this type yet
              break;
            // no message
            case MSG_NO_MESSAGE:
              // do nothing. 
              // NOTE: this is a valid case. If the message is not 'parsible' then it can be
              //  given a 'NO_MESSAGE' type.
              break;
            default:
              // do nothing
              // NOTICE: really this should never happen. Eventually, throw and error here.
              break;
          }
        }        
      }
    }
    nrk_wait_until_next_period();
  }
}

/**
 * tx_cmd_task() -
 *  send all commands out to the network.
 */
void tx_cmd_task() {
  // local variable instantiation
  uint8_t LED_FLAG = 0;
  uint16_t val;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  packet tx_packet;
  uint8_t tx_cmd_queue_size;

  // Wait until bmac has started. This should be called by all tasks 
  //  using bmac that do not call bmac_init().
  while(!bmac_started()) {
    nrk_wait_until_next_period();
  }

  // Get and register the tx_done_signal to perform non-blocking transmits
  tx_done_signal = bmac_get_tx_done_signal();
  nrk_signal_register(tx_done_signal);

  // loop forever
  while(1){
    // LED blinking - for debug
    if(blink_leds == BLINKLEDS) {
      LED_FLAG++;
      LED_FLAG%=2;
      /*
      if(LED_FLAG == 0) {
        nrk_led_set(1);
      } else {
        nrk_led_clr(1);
      }  */    
    }

    // atomically get the queue size
    nrk_sem_pend(cmd_tx_queue_mux);
      tx_cmd_queue_size = cmd_tx_queue.size;
    nrk_sem_post(cmd_tx_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(cmd_tx_queue.size > 0)" was used a few bad things could happen
     *      (1) a mutex would be required around the entire loop - BAD IDEA
     *      (2) the queue could be added to while this loop is running, thus
     *        making the loop unbounded - BAD IDEA
     *      (3) the size the queue read and the actual size of the queue could be 
     *        incorrect due to preemtion - BAD IDEA
     *    Doing it this way bounds this loop to the maximum size of the queue
     *    at any given time, regardless of whether or not the queue has been 
     *    added to by another task.
     */
    for(uint8_t i = 0; i < tx_cmd_queue_size; i++) {
      // get a packet out of the queue.
      nrk_sem_pend(cmd_tx_queue_mux);
      pop(&cmd_tx_queue, &tx_packet);
      nrk_sem_post(cmd_tx_queue_mux);

      // NOTE: a mutex is required around the network transmit buffer because 
      //  tx_cmd_task() also uses it.
      nrk_sem_pend(net_tx_buf_mux);
      net_tx_index = assemble_packet(&net_tx_buf, &tx_packet);

      // send the packet
      val = bmac_tx_pkt_nonblocking(net_tx_buf, net_tx_index);
      ret = nrk_event_wait (SIG(tx_done_signal));

      printf("tx pkt: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
        net_tx_buf[0],net_tx_buf[1],net_tx_buf[2],net_tx_buf[3],net_tx_buf[4],
        net_tx_buf[5],net_tx_buf[6],net_tx_buf[7],net_tx_buf[8],net_tx_buf[9]);
      
      // Just check to be sure signal is okay
      if(ret & (SIG(tx_done_signal) == 0)) {
        nrk_kprintf (PSTR ("TX done signal error\r\n"));
      }
      clear_tx_buf();
      nrk_sem_post(net_tx_buf_mux);     
    }
    nrk_wait_until_next_period();
  }
}

/**
 * tx_data_task() -
 *  send standard messages out to the network (i.e. handshake messages, etc.)
 */
void tx_data_task() {
  // local variable initialization
  uint8_t LED_FLAG = 0;
  uint16_t val;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  packet tx_packet;
  uint8_t tx_data_queue_size;

  // Wait until bmac has started. This should be called by all tasks 
  //  using bmac that do not call bmac_init().
  while(!bmac_started ()) {
    nrk_wait_until_next_period ();
  }

  // Get and register the tx_done_signal to perform non-blocking transmits
  tx_done_signal = bmac_get_tx_done_signal();
  nrk_signal_register(tx_done_signal);
  
  while(1) {
    // LED blinking - for debug
    if(blink_leds == BLINKLEDS) {
      LED_FLAG++;
      LED_FLAG%=2;
      if(LED_FLAG == 0) {
        nrk_led_set(3);
      } else {
        nrk_led_clr(3);
      }      
    }
 
    // atomically get the queue size
    nrk_sem_pend(data_tx_queue_mux);
      tx_data_queue_size = data_tx_queue.size;
    nrk_sem_post(data_tx_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(node_tx_queue.size > 0)" was used a few bad things could happen
     *      (1) a mutex would be required around the entire loop - BAD IDEA
     *      (2) the queue could be added to while this loop is running, thus
     *        making the loop unbounded - BAD IDEA
     *      (3) the size the queue read and the actual size of the queue could be 
     *        incorrect due to preemtion - BAD IDEA
     *    Doing it this way bounds this loop to the maximum size of the queue
     *    at any given time, regardless of whether or not the queue has been 
     *    added to by another task.
     */
    for(uint8_t i = 0; i < tx_data_queue_size; i++) {
      // get a packet out of the queue.
      nrk_sem_pend(data_tx_queue_mux);
     // printf("Q front:%d\r\n", data_tx_queue.front);// for debug
      pop(&data_tx_queue, &tx_packet);
      nrk_sem_post(data_tx_queue_mux);

      if(print_incoming == 1){
        //nrk_kprintf (PSTR ("Asm pkt:\r\n"));
        //print_packet(&tx_packet);
      }

      // NOTE: a mutex is required around the network transmit buffer because 
      //  tx_cmd_task() also uses it.
      nrk_sem_pend(net_tx_buf_mux);
      net_tx_index = assemble_packet(&net_tx_buf, &tx_packet);

      // send the packet
      val = bmac_tx_pkt_nonblocking(net_tx_buf, net_tx_index);
      ret = nrk_event_wait (SIG(tx_done_signal));

      /*printf("tx pkt: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
        net_tx_buf[0],net_tx_buf[1],net_tx_buf[2],net_tx_buf[3],net_tx_buf[4],
        net_tx_buf[5],net_tx_buf[6],net_tx_buf[7],net_tx_buf[8],net_tx_buf[9]);*/

        //printf("index: %d\r\n", net_tx_index);

      // Just check to be sure signal is okay
      if(ret & (SIG(tx_done_signal) == 0)) {
        nrk_kprintf (PSTR ("TX done signal error\r\n"));
      }
      clear_tx_buf();
      nrk_sem_post(net_tx_buf_mux);     
    }
    nrk_wait_until_next_period();
  }
}


/**
 * sample_task() -
 *  sample any sensors that are supposed to be sampled.
 */
void sample_task() {
  // local variable instantiation
  uint8_t LED_FLAG = 0;
  uint8_t pwr_period_count = 0;
  uint8_t temp_period_count = 0;
  uint8_t light_period_count = 0;
  uint8_t sensor_sampled = FALSE;
  uint16_t local_pwr_val = 0;
  uint16_t local_temp_val = 0;
  uint16_t local_light_val = 0;
  packet tx_packet;

  // initialize sensor packet
  sensor_pkt.pwr_val = local_pwr_val;
  sensor_pkt.temp_val = local_temp_val;
  sensor_pkt.light_val = local_light_val;

  // initialize tx_packet
  tx_packet.source_id = MAC_ADDR;
  tx_packet.type = MSG_DATA;
  tx_packet.num_hops = 0;

  // loop forever
  while(1) {
    // LED blinking - for debug
    if(blink_leds == BLINKLEDS) {
      LED_FLAG++;
      LED_FLAG%=2;
      if(LED_FLAG == 0) {
        //nrk_led_set(2);
      } else {
        nrk_led_clr(2);
      }      
    }

    // update period counts
    pwr_period_count++;
    temp_period_count++;
    light_period_count++;
    pwr_period_count %= pwr_period;
    temp_period_count %= temp_period;
    light_period_count %= light_period;

    // sample power sensor if appropriate
    if(pwr_period_count == SAMPLE_SENSOR) {
      //TODO: SAMPLE POWER SENSOR
      local_pwr_val++;
      sensor_pkt.pwr_val = local_pwr_val;
      sensor_sampled = TRUE;
      pwr_period_count = 0;
    }

    // sample temperature sensor if appropriate
    if(temp_period_count == SAMPLE_SENSOR) {
      //TODO: SAMPLE TEMP SENSOR
      local_temp_val++;
      sensor_pkt.temp_val = local_temp_val;
      sensor_sampled = TRUE;
      temp_period_count = 0;
    }

    // sample light sensor if appropriate
    if(light_period_count == SAMPLE_SENSOR) {
      //TODO: SAMPLE LIGHT SENSOR
      local_light_val++;
      sensor_pkt.light_val = local_light_val;
      sensor_sampled = TRUE;
      light_period_count = 0;
    }

    // if a sensor has been sampled, send a packet out
    if(sensor_sampled == TRUE) {
      // update sequence number
      nrk_sem_post(seq_num_mux);
      seq_num++;
      tx_packet.seq_num = seq_num;
      nrk_sem_pend(seq_num_mux);

      // add data values to sensor packet
      tx_packet.payload[DATA_PWR_INDEX] = sensor_pkt.pwr_val;
      tx_packet.payload[DATA_TEMP_INDEX] = sensor_pkt.temp_val;
      tx_packet.payload[DATA_LIGHT_INDEX] = sensor_pkt.light_val;

      // add packet to data queue
      nrk_sem_pend(data_tx_queue_mux);
      push(&data_tx_queue, &tx_packet);
      nrk_sem_post(data_tx_queue_mux);

      //nrk_sem_pend(cmd_tx_queue_mux);
      //push(&cmd_tx_queue, &tx_packet);
      //nrk_sem_post(data_tx_queue_mux);

      // reset FLAG
      sensor_sampled = FALSE;
    }
    nrk_wait_until_next_period();
  }
}

/**
 * actuate_task() - 
 *  actuate any commands that have been received for this node.
 */
void actuate_task() {
  // local variable instantiation
  uint8_t LED_FLAG = 0;
  uint8_t act_queue_size;
  packet act_packet, tx_packet;
  uint8_t action, ack_required, act_required; 

  // initialize tx_packet
  tx_packet.source_id = MAC_ADDR;
  tx_packet.type = MSG_CMDACK;
  tx_packet.num_hops = 0;

  // loop forever
  while(1) {
    // LEDs for debug
    if(blink_leds == BLINKLEDS) {
      LED_FLAG++;
      LED_FLAG%=2;
      if(LED_FLAG == 0) {
        //nrk_led_set(1);
      } else {
        nrk_led_clr(1);
      }   
    }

    // get action queue size
    nrk_sem_post(act_queue_mux);
    act_queue_size = act_queue.size;
    nrk_sem_pend(act_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(act_queue.size > 0)" was used a few bad things could happen
     *      (1) a mutex would be required around the entire loop - BAD IDEA
     *      (2) the queue could be added to while this loop is running, thus
     *        making the loop unbounded - BAD IDEA
     *      (3) the size the queue read and the actual size of the queue could be 
     *        incorrect due to preemtion - BAD IDEA
     *    Doing it this way bounds this loop to the maximum size of the queue
     *    at any given time, regardless of whether or not the queue has been 
     *    added to by another task.
     */
    for(uint8_t i = 0; i < act_queue_size; i++) {
      // get packet out of the queue
      nrk_sem_post(act_queue_mux);
      pop(&act_queue, &act_packet);
      nrk_sem_pend(act_queue_mux); 

      // pull action out of packet
      action = act_packet.payload[CMD_ACT_INDEX];

      // switch on current state
      switch(curr_state) {
        // STATE_ON - actuate if required / acknowledge if required
        case STATE_ON:
          // action ON received -> ack but don't act
          if(action == ON) {
            ack_required = TRUE;
            act_required = FALSE;
            curr_state = STATE_ON;
          } 
          // action OFF received -> ack and act
          else if(action == OFF) {
            ack_required = TRUE;
            act_required = TRUE;
            curr_state = STATE_OFF;
          } 
          // this should never happen, but -> don't ack and don't act
          else {
            ack_required = FALSE;
            act_required = FALSE;
            curr_state = STATE_ON;
          }
          break;        
        // STATE_OFF - actuate if required / acknowledge if required
        case STATE_OFF:
          // action OFF received -> ack but don't act
          if(action == OFF) {
            ack_required = TRUE;
            act_required = FALSE;
            curr_state = STATE_OFF;
          } 
          // action ON received -> ack and act
          else if(action == ON) {
            ack_required = TRUE;
            act_required = TRUE;
            curr_state = STATE_ON;
          } 
          // this should never happen, but -> don't ack and don't act
          else {
            ack_required = FALSE;
            act_required = FALSE;
            curr_state = STATE_OFF;
          }
          break;
        default:
          // THIS SHOULD NEVER HAPPEN.
          break;
      }

      // action required -> complete actuation
      if(act_required == TRUE) {
        // TODO: ACTUATE.
        // dummy operation so the compiler doesn't yell.
        i = i;
      }

      // acknowledge required -> add ack packet to cmd_tx_queue
      if(ack_required == TRUE) {
        // update sequence number
        nrk_sem_pend(seq_num_mux);
        seq_num++;
        tx_packet.seq_num = seq_num;
        nrk_sem_post(seq_num_mux);

        // set payload
        tx_packet.payload[CMDACK_ID_INDEX] = act_packet.payload[CMD_ID_INDEX];

        // place message in the queue
        nrk_sem_pend(cmd_tx_queue_mux);
        push(&cmd_tx_queue, &tx_packet);
        nrk_sem_post(cmd_tx_queue_mux);
      }
    }
    nrk_wait_until_next_period();
  }
}

/**
 * nrk_create_taskset - create the tasks in this application
 * 
 * NOTE: task priority maps to importance. That is, priority(5) > priority(2).
 */
void nrk_create_taskset () {
  // PRIORITY 5
  RX_MSG_TASK.task = rx_msg_task;
  nrk_task_set_stk(&RX_MSG_TASK, rx_msg_task_stack, NRK_APP_STACKSIZE);
  RX_MSG_TASK.prio = 5;
  RX_MSG_TASK.FirstActivation = TRUE;
  RX_MSG_TASK.Type = BASIC_TASK;
  RX_MSG_TASK.SchType = PREEMPTIVE;
  RX_MSG_TASK.period.secs = 0;
  RX_MSG_TASK.period.nano_secs = 100*NANOS_PER_MS;
  RX_MSG_TASK.cpu_reserve.secs = 0;
  RX_MSG_TASK.cpu_reserve.nano_secs = 20*NANOS_PER_MS;
  RX_MSG_TASK.offset.secs = 0;
  RX_MSG_TASK.offset.nano_secs = 0;

  // PRIORITY 4
  ACTUATE_TASK.task = actuate_task;
  nrk_task_set_stk(&ACTUATE_TASK, actuate_task_stack, NRK_APP_STACKSIZE);
  ACTUATE_TASK.prio = 4;
  ACTUATE_TASK.FirstActivation = TRUE;
  ACTUATE_TASK.Type = BASIC_TASK;
  ACTUATE_TASK.SchType = PREEMPTIVE;
  ACTUATE_TASK.period.secs = 0;
  ACTUATE_TASK.period.nano_secs = 200*NANOS_PER_MS;
  ACTUATE_TASK.cpu_reserve.secs = 0;
  ACTUATE_TASK.cpu_reserve.nano_secs = 30*NANOS_PER_MS;
  ACTUATE_TASK.offset.secs = 0;
  ACTUATE_TASK.offset.nano_secs = 0;

  // PRIORITY 3
  TX_CMD_TASK.task = tx_cmd_task;
  nrk_task_set_stk(&TX_CMD_TASK, tx_cmd_task_stack, NRK_APP_STACKSIZE);
  TX_CMD_TASK.prio = 3;
  TX_CMD_TASK.FirstActivation = TRUE;
  TX_CMD_TASK.Type = BASIC_TASK;
  TX_CMD_TASK.SchType = PREEMPTIVE;
  TX_CMD_TASK.period.secs = 0;
  TX_CMD_TASK.period.nano_secs = 100*NANOS_PER_MS;
  TX_CMD_TASK.cpu_reserve.secs = 0;
  TX_CMD_TASK.cpu_reserve.nano_secs = 20*NANOS_PER_MS;
  TX_CMD_TASK.offset.secs = 0;
  TX_CMD_TASK.offset.nano_secs = 0;

  // PRIORITY 2
  SAMPLE_TASK.task = sample_task;
  nrk_task_set_stk(&SAMPLE_TASK, sample_task_stack, NRK_APP_STACKSIZE);
  SAMPLE_TASK.prio = 2;
  SAMPLE_TASK.FirstActivation = TRUE;
  SAMPLE_TASK.Type = BASIC_TASK;
  SAMPLE_TASK.SchType = PREEMPTIVE;
  SAMPLE_TASK.period.secs = 1;
  SAMPLE_TASK.period.nano_secs = 0;
  SAMPLE_TASK.cpu_reserve.secs = 0;
  SAMPLE_TASK.cpu_reserve.nano_secs = 200*NANOS_PER_MS;
  SAMPLE_TASK.offset.secs = 0;
  SAMPLE_TASK.offset.nano_secs = 0;

  // PRIORITY 1
  TX_DATA_TASK.task = tx_data_task;
  nrk_task_set_stk(&TX_DATA_TASK, tx_data_task_stack, NRK_APP_STACKSIZE);
  TX_DATA_TASK.prio = 1;
  TX_DATA_TASK.FirstActivation = TRUE;
  TX_DATA_TASK.Type = BASIC_TASK;
  TX_DATA_TASK.SchType = PREEMPTIVE;
  TX_DATA_TASK.period.secs = 5;
  TX_DATA_TASK.period.nano_secs = 0;
  TX_DATA_TASK.cpu_reserve.secs = 0;
  TX_DATA_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TX_DATA_TASK.offset.secs = 0;
  TX_DATA_TASK.offset.nano_secs = 0;

  nrk_activate_task(&RX_MSG_TASK); 
  nrk_activate_task(&TX_CMD_TASK);
  nrk_activate_task(&TX_DATA_TASK);
  nrk_activate_task(&SAMPLE_TASK);
  nrk_activate_task(&ACTUATE_TASK);

  nrk_kprintf(PSTR("Create done.\r\n"));
}

