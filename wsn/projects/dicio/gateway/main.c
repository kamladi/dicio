/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * main.c (gateway)
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

// silly change 
 
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
#define MAC_ADDR 1


// FUNCTION DECLARATIONS
uint8_t get_server_input(void);
void clear_serv_buf();
void rx_node_task(void);
void rx_serv_task(void);
void tx_cmd_task(void);
void tx_node_task(void);
void tx_serv_task(void);
void nrk_create_taskset ();

// TASKS
nrk_task_type RX_NODE_TASK;
nrk_task_type RX_SERV_TASK;
nrk_task_type TX_CMD_TASK;
nrk_task_type TX_NODE_TASK;
nrk_task_type TX_SERV_TASK;

// TASK STACKS
NRK_STK rx_node_task_stack[NRK_APP_STACKSIZE];
NRK_STK rx_serv_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_cmd_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_node_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_serv_task_stack[NRK_APP_STACKSIZE];

// BUFFERS
uint8_t net_rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t serv_rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t serv_rx_index = 0;
uint8_t net_tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t net_tx_index = 0;
nrk_sem_t* net_tx_buf_mux;
uint8_t serv_tx_buf[RF_MAX_PAYLOAD_SIZE];

// QUEUES
packet_queue cmd_tx_queue;
nrk_sem_t* cmd_tx_queue_mux;
packet_queue node_tx_queue;
nrk_sem_t* node_tx_queue_mux;
packet_queue serv_tx_queue;
nrk_sem_t* serv_tx_queue_mux;
packet_queue hand_rx_queue;
nrk_sem_t* hand_rx_queue_mux;

// DRIVERS 
void nrk_register_drivers();

// SEQUENCE POOLS/NUMBER
sequence_pool_t seq_pool;
uint16_t server_seq_num;
uint16_t seq_num = 0;

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
  cmd_tx_queue_mux  = nrk_sem_create(1, 6);
  node_tx_queue_mux = nrk_sem_create(1, 6);
  serv_tx_queue_mux = nrk_sem_create(1, 6);
  hand_rx_queue_mux = nrk_sem_create(1, 6);

  // packet queues
  packet_queue_init(&cmd_tx_queue);
  packet_queue_init(&node_tx_queue);
  packet_queue_init(&serv_tx_queue);
  packet_queue_init(&hand_rx_queue);

  nrk_time_set (0, 0);
  bmac_task_config();
  nrk_create_taskset();
  bmac_init (13);
  nrk_start ();
  return 0;
}

/**
 * get_server_input() - 
 *  get UART data from server - end of message noted by a '\r'
 */
uint8_t get_server_input() {
  uint8_t option;

  // loop until all bytes have been received
  while(nrk_uart_data_ready(NRK_DEFAULT_UART)) {

    // get UART byte and add to buffer
    option = getchar();

    // if there is room, add it to the buffer.
    if(serv_rx_index < (RF_MAX_PAYLOAD_SIZE -1)) {
      serv_rx_buf[serv_rx_index] = option;
      serv_rx_index++;      
    } 
    // if there is not room, clear the buffer and then add the new byte.
    else {
      clear_serv_buf();
      serv_rx_buf[serv_rx_index] = option;
      serv_rx_index++;
    }

    // print if appropriate
    if(print_incoming == PRINT2TERM) {
      printf("%c", option);
    }

    // message has been completed
    if(option == '\r') {
      serv_rx_buf[serv_rx_index] = '\n';
      serv_rx_index++;
      if(print_incoming == PRINT2TERM) {
        nrk_kprintf(PSTR("\n"));
      }
      return SERV_MSG_RECEIVED;    
    }
  }
  return SERV_MSG_INCOMPLETE;
}

/**
 * clear_serv_buf() - 
 *  clear the server buffer
 */
void clear_serv_buf() {
  for(uint8_t i = 0; i < serv_rx_index; i++) {
    serv_rx_buf[i] = '\0';
  }
  serv_rx_index = 0;
}

void clear_tx_buf(){
  for(uint8_t i = 0; i < net_tx_index; i++){
    net_tx_buf[i] = 0;
  }
  net_tx_index = 0;
}

/**
 * rx_node_task() - 
 *  receive messages from the network
 */
void rx_node_task() {
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
  while (!bmac_started ()){
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
      nrk_kprintf (PSTR ("pkt ready:\r\n"));
      // get the packet, parse and release
      parse_msg(&rx_packet, &net_rx_buf, len);
      local_buf = bmac_rx_pkt_get(&len, &rssi);
      
      // print incoming packet if appropriate
      if(print_incoming == 1) {
        nrk_kprintf (PSTR ("rx:\r\n"));
        print_packet(&rx_packet);     
      }

       bmac_rx_pkt_release ();  
      
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
            // data received or command ack received -> forward to server
            case MSG_DATA:
            {
              nrk_sem_pend(serv_tx_queue_mux);
              push(&serv_tx_queue, &rx_packet);
              nrk_sem_post(serv_tx_queue_mux);
              break;
            }
            case MSG_CMDACK:
            {
              rx_packet.num_hops ++;
              nrk_sem_pend(serv_tx_queue_mux);
              push(&serv_tx_queue, &rx_packet);
              nrk_sem_post(serv_tx_queue_mux);
              break;
            }
            // handshake message recieved -> deal with in handshake function
            case MSG_HAND:
              nrk_sem_pend(hand_rx_queue_mux);
              push(&hand_rx_queue, &rx_packet);
              nrk_sem_post(hand_rx_queue_mux);
              break;
            case MSG_NO_MESSAGE:
              // do nothing. 
              // NOTE: this is a valid case. If the message is not 'parsible' then it can be
              //  given a 'NO_MESSAGE' type.
              break;
            case MSG_GATEWAY:
            case MSG_CMD:
              // do nothing...commands are sent by the gateway, so there is no
              //  no need to pass it along.
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
 * rx_serv_task() - 
 *  receive a message from the server
 */
void rx_serv_task() {
  // local variable instantiation
  uint8_t LED_FLAG = 0;
  packet rx_packet;

  // get the UART signal and register it
  nrk_sig_t uart_rx_signal = nrk_uart_rx_signal_get();
  nrk_signal_register(uart_rx_signal);
  
  // loop forever
  while (1) {
    // LED blinking - for debug
    if(blink_leds == BLINKLEDS) {
      LED_FLAG++;
      LED_FLAG%=2;
      if(LED_FLAG == 0) {
        nrk_led_set(1);
      } else {
        nrk_led_clr(1);
      }      
    }

    // only execute if a full server message has been received
    if(get_server_input() == SERV_MSG_RECEIVED) {

      // print message if appropriate
      if(print_incoming == 1) {
        printf("%s", serv_rx_buf);     
      }

      // parse message
      parse_serv_msg(&rx_packet, &serv_rx_buf, serv_rx_index);
      clear_serv_buf();

      // check sequence number to determine if the packet should be received
      // NOTE: This probably is unnecessary because the likelihood of an earlier
      //  message being delivered serially is very slim. However, one can imagine
      //  that if these were actual network messages that a delay would be much more
      //  probable. Thus, in the spirit of correctness and completeness, we will
      //  keep track of these sequence numbers.
      if(rx_packet.seq_num > server_seq_num) {

        // update local sequence number
        server_seq_num = rx_packet.seq_num;


        switch(rx_packet.type) {
          // if a command
          case MSG_CMD:
          {
            //nrk_kprintf (PSTR ("push to Q\r\n")); // for debugging
            nrk_sem_pend(cmd_tx_queue_mux);
            push(&cmd_tx_queue, &rx_packet);
            nrk_sem_post(cmd_tx_queue_mux);
            break;
          }
          case MSG_NO_MESSAGE:
            // do nothing. 
            // NOTE: this is a valid case. If the message is not 'parsible' then it can be
            //  given a 'NO_MESSAGE' type.
            break;
          case MSG_GATEWAY:
          case MSG_DATA:
          case MSG_CMDACK:
          case MSG_HAND: 
            // do nothing
            // NOTICE: really, this should never happend. Eventually, throw an error here.
            break;  
          default:
            // do nothing
            // NOTICE: really this should never happen. Eventually, throw an error here.
            break;
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
      if(LED_FLAG == 0) {
        nrk_led_set(2);
      } else {
        nrk_led_clr(2);
      }      
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
      if(print_incoming == 1){
       nrk_kprintf (PSTR ("asm pkt:\r\n"));
      }

      // send the packet
      val = bmac_tx_pkt_nonblocking(net_tx_buf, net_tx_index);
      ret = nrk_event_wait (SIG(tx_done_signal));
      
      // Just check to be sure signal is okay
      if(ret & (SIG(tx_done_signal) == 0)) {
        nrk_kprintf (PSTR ("TX done signal error\r\n"));
      }
      nrk_sem_post(net_tx_buf_mux);     
    }
    nrk_wait_until_next_period();
  }
}

/**
 * tx_node_task() -
 *  send standard messages out to the network (i.e. handshake messages, etc.)
 */
void tx_node_task() {
  // local variable initialization
  uint8_t LED_FLAG = 0;
  uint16_t val;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  packet tx_packet;
  uint8_t tx_node_queue_size;
  
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
    nrk_sem_pend(node_tx_queue_mux);
      tx_node_queue_size = node_tx_queue.size;
    nrk_sem_post(node_tx_queue_mux);

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
    for(uint8_t i = 0; i < tx_node_queue_size; i++) {
      // get a packet out of the queue.
      nrk_sem_pend(node_tx_queue_mux);
      pop(&node_tx_queue, &tx_packet);
      nrk_sem_post(node_tx_queue_mux);

      // NOTE: a mutex is required around the network transmit buffer because 
      //  tx_cmd_task() also uses it.
      nrk_sem_pend(net_tx_buf_mux);
      assemble_packet(&net_tx_buf, &tx_packet);
      // send the packet
      val = bmac_tx_pkt_nonblocking(net_tx_buf, strlen(net_tx_buf));
      ret = nrk_event_wait (SIG(tx_done_signal));
      
      // Just check to be sure signal is okay
      if(ret & (SIG(tx_done_signal) == 0)) {
        nrk_kprintf (PSTR ("TX done signal error\r\n"));
      }
      nrk_sem_post(net_tx_buf_mux);     
    }
    nrk_wait_until_next_period();
  }
}

/**
 * tx_serv_task() _
 *  send messages to the server
 */
void tx_serv_task() {
  // local variable initialization
  uint8_t LED_FLAG = 0;
  uint8_t tx_serv_queue_size;
  packet tx_packet;

  while(1) {

    if(blink_leds == BLINKLEDS) {
      LED_FLAG++;
      LED_FLAG%=2;
      if(LED_FLAG == 0) {
        nrk_kprintf(PSTR("SERV LED ON\r\n"));
      } else {
        nrk_kprintf(PSTR("SERV LED OFF\r\n"));
      }      
    }

    // atomically get the queue size
    nrk_sem_pend(serv_tx_queue_mux);
      tx_serv_queue_size = serv_tx_queue.size;
    nrk_sem_post(serv_tx_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(serv_tx_queue.size > 0)" was used a few bad things could happen
     *      (1) a mutex would be required around the entire loop - BAD IDEA
     *      (2) the queue could be added to while this loop is running, thus
     *        making the loop unbounded - BAD IDEA
     *      (3) the size the queue read and the actual size of the queue could be 
     *        incorrect due to preemtion - BAD IDEA
     *    Doing it this way bounds this loop to the maximum size of the queue
     *    at any given time, regardless of whether or not the queue has been 
     *    added to by another task.
     */
    for(uint8_t i = 0; i < tx_serv_queue_size; i++) {
      // get a packet out of the queue.
      nrk_sem_pend(serv_tx_queue_mux);
      pop(&serv_tx_queue, &tx_packet);
      nrk_sem_post(serv_tx_queue_mux);

      // NOTE: unlike tx_cmd_task() and tx_node_task(), no mutex is required around
      //  the sending buffer here because tx_serv_task() is the only task to use
      //  the serial transmitting buffer (serv_tx_buff);
      assemble_serv_packet(&serv_tx_buf, &tx_packet);

      // send the packet
      printf("%s\r\n", serv_tx_buf);
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
  // PRIORITY 5 - HIGHEST PRIORITY
  RX_NODE_TASK.task = rx_node_task;
  nrk_task_set_stk(&RX_NODE_TASK, rx_node_task_stack, NRK_APP_STACKSIZE);
  RX_NODE_TASK.prio = 5;
  RX_NODE_TASK.FirstActivation = TRUE;
  RX_NODE_TASK.Type = BASIC_TASK;
  RX_NODE_TASK.SchType = PREEMPTIVE;
  RX_NODE_TASK.period.secs = 0;
  RX_NODE_TASK.period.nano_secs = 50*NANOS_PER_MS;
  RX_NODE_TASK.cpu_reserve.secs = 0;
  RX_NODE_TASK.cpu_reserve.nano_secs = 10*NANOS_PER_MS;
  RX_NODE_TASK.offset.secs = 0;
  RX_NODE_TASK.offset.nano_secs = 0;
  nrk_activate_task(&RX_NODE_TASK);

  // PRIORITY 4 - SECOND HIGHEST PRIORITY
  RX_SERV_TASK.task = rx_serv_task;
  nrk_task_set_stk(&RX_SERV_TASK, rx_serv_task, NRK_APP_STACKSIZE);
  RX_SERV_TASK.prio = 4;
  RX_SERV_TASK.FirstActivation = TRUE;
  RX_SERV_TASK.Type = BASIC_TASK;
  RX_SERV_TASK.SchType = PREEMPTIVE;
  RX_SERV_TASK.period.secs = 0;
  RX_SERV_TASK.period.nano_secs = 50*NANOS_PER_MS;
  RX_SERV_TASK.cpu_reserve.secs = 0;
  RX_SERV_TASK.cpu_reserve.nano_secs = 10*NANOS_PER_MS;
  RX_SERV_TASK.offset.secs = 0;
  RX_SERV_TASK.offset.nano_secs = 0;
  nrk_activate_task(&RX_SERV_TASK);
  
  // PRIORITY 3 - MIDDLE PRIORITY
  TX_CMD_TASK.task = tx_cmd_task;
  nrk_task_set_stk(&TX_CMD_TASK, tx_cmd_task_stack, NRK_APP_STACKSIZE);
  TX_CMD_TASK.prio = 3;
  TX_CMD_TASK.FirstActivation = TRUE;
  TX_CMD_TASK.Type = BASIC_TASK;
  TX_CMD_TASK.SchType = PREEMPTIVE;
  TX_CMD_TASK.period.secs = 0;
  TX_CMD_TASK.period.nano_secs = 150*NANOS_PER_MS;
  TX_CMD_TASK.cpu_reserve.secs = 0;
  TX_CMD_TASK.cpu_reserve.nano_secs = 50*NANOS_PER_MS;
  TX_CMD_TASK.offset.secs = 0;
  TX_CMD_TASK.offset.nano_secs = 0;
  nrk_activate_task(&TX_CMD_TASK);


  // PRIORITY 2 - SECOND LOWEST PRIORITY
  TX_NODE_TASK.task = tx_node_task;
  nrk_task_set_stk(&TX_NODE_TASK, tx_node_task_stack, NRK_APP_STACKSIZE);
  TX_NODE_TASK.prio = 2;
  TX_NODE_TASK.FirstActivation = TRUE;
  TX_NODE_TASK.Type = BASIC_TASK;
  TX_NODE_TASK.SchType = PREEMPTIVE;
  TX_NODE_TASK.period.secs = 5;
  TX_NODE_TASK.period.nano_secs = 0;
  TX_NODE_TASK.cpu_reserve.secs = 0;
  TX_NODE_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TX_NODE_TASK.offset.secs = 0;
  TX_NODE_TASK.offset.nano_secs = 0;
  nrk_activate_task(&TX_NODE_TASK);

  // PRIORITY 1 - LOWEST PRIORITY
  TX_SERV_TASK.task = tx_serv_task;
  nrk_task_set_stk(&TX_SERV_TASK, tx_serv_task_stack, NRK_APP_STACKSIZE);
  TX_SERV_TASK.prio = 1;
  TX_SERV_TASK.FirstActivation = TRUE;
  TX_SERV_TASK.Type = BASIC_TASK;
  TX_SERV_TASK.SchType = PREEMPTIVE;
  TX_SERV_TASK.period.secs = 5;
  TX_SERV_TASK.period.nano_secs = 0;
  TX_SERV_TASK.cpu_reserve.secs = 0;
  TX_SERV_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TX_SERV_TASK.offset.secs = 0;
  TX_SERV_TASK.offset.nano_secs = 0;
  nrk_activate_task(&TX_SERV_TASK);
  
  
  nrk_kprintf(PSTR("Create done.\r\n"));
}

