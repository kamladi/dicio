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
#include <packet_queue.h>
#include <parser.h>
#include <pool.h>
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
void hand_task(void);
void nrk_create_taskset ();
void nrk_register_drivers();

// TASKS
nrk_task_type RX_NODE_TASK;
nrk_task_type RX_SERV_TASK;
nrk_task_type TX_CMD_TASK;
nrk_task_type TX_NODE_TASK;
nrk_task_type TX_SERV_TASK;
nrk_task_type HAND_TASK;

// TASK STACKS
NRK_STK rx_node_task_stack[NRK_APP_STACKSIZE];
NRK_STK rx_serv_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_cmd_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_node_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_serv_task_stack[NRK_APP_STACKSIZE];
NRK_STK hand_task_stack[NRK_APP_STACKSIZE];

// BUFFERS
uint8_t g_net_rx_buf[RF_MAX_PAYLOAD_SIZE];

uint8_t g_serv_rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t g_serv_rx_index = 0;

uint8_t g_net_tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t g_net_tx_index = 0;
nrk_sem_t* g_net_tx_buf_mux
;
uint8_t g_serv_tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t g_serv_tx_index = 0;

// QUEUES
packet_queue g_cmd_tx_queue;
nrk_sem_t* g_cmd_tx_queue_mux;
packet_queue g_node_tx_queue;
nrk_sem_t* g_node_tx_queue_mux;
packet_queue g_serv_tx_queue;
nrk_sem_t* g_serv_tx_queue_mux;
packet_queue g_hand_rx_queue;
nrk_sem_t* g_hand_rx_queue_mux;

// SEQUENCE POOLS/NUMBER
pool_t g_seq_pool;
uint16_t g_seq_num = 0;
uint16_t g_server_seq_num = 0;

// GLOBAL FLAG
uint8_t g_print_incoming;

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
  g_print_incoming = FALSE;

  // mutexs
  g_net_tx_buf_mux    = nrk_sem_create(1, 6);
  g_cmd_tx_queue_mux  = nrk_sem_create(1, 6);
  g_node_tx_queue_mux = nrk_sem_create(1, 6);
  g_serv_tx_queue_mux = nrk_sem_create(1, 6);
  g_hand_rx_queue_mux = nrk_sem_create(1, 6);

  // packet queues
  packet_queue_init(&g_cmd_tx_queue);
  packet_queue_init(&g_node_tx_queue);
  packet_queue_init(&g_serv_tx_queue);
  packet_queue_init(&g_hand_rx_queue);

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
    if(g_serv_rx_index < (RF_MAX_PAYLOAD_SIZE -1)) {
      g_serv_rx_buf[g_serv_rx_index] = option;
      g_serv_rx_index++;
    }
    // if there is not room, clear the buffer and then add the new byte.
    else {
      clear_serv_buf();
      g_serv_rx_buf[g_serv_rx_index] = option;
      g_serv_rx_index++;
    }

    // print if appropriate
    if(g_print_incoming == TRUE) {
      printf("!%d", option);
    }

    // message has been completed
    if(option == '\r') {
      g_serv_rx_buf[g_serv_rx_index] = '\n';
      g_serv_rx_index++;
      if(g_print_incoming == TRUE) {
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
  for(uint8_t i = 0; i < g_serv_rx_index; i++) {
    g_serv_rx_buf[i] = 0;
  }
  g_serv_rx_index = 0;
}

void clear_tx_buf(){
  for(uint8_t i = 0; i < g_net_tx_index; i++){
    g_net_tx_buf[i] = 0;
  }
  g_net_tx_index = 0;
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
  bmac_rx_pkt_set_buffer(g_net_rx_buf, RF_MAX_PAYLOAD_SIZE);

  // Wait until bmac has started. This should be called by all tasks using bmac that do not call bmac_init()
  while (!bmac_started ()){
    nrk_wait_until_next_period ();
  }

  // loop forever
  while(1) {
    // only execute if there is a packet available
    if(bmac_rx_pkt_ready()) {
      nrk_led_set(GREEN_LED);
      // get the packet, parse and release
      parse_msg(&rx_packet, &g_net_rx_buf, len);
      local_buf = bmac_rx_pkt_get(&len, &rssi);
      bmac_rx_pkt_release ();

      // print incoming packet if appropriate
      if(g_print_incoming == TRUE) {
        nrk_kprintf (PSTR ("rx:\r\n"));
        print_packet(&rx_packet);
      }



      // only receive the message if it's not from the gateway
      //  NOTE: this is required because the gateway will hear re-transmitted packets
      //    originally from itself.
      if(rx_packet.source_id != MAC_ADDR) {

        // check to see if this node is in the sequence pool, if not then add it
        in_seq_pool = in_pool(&g_seq_pool, rx_packet.source_id);
        if(in_seq_pool == -1) {
          add_to_pool(&g_seq_pool, rx_packet.source_id, rx_packet.seq_num);
          new_node = NODE_FOUND;
        }

        // determine if we should act on this packet based on the sequence number
        local_seq_num = get_data_val(&g_seq_pool, rx_packet.source_id);
        if((rx_packet.seq_num > local_seq_num) || (new_node == NODE_FOUND) || (rx_packet.type == MSG_HAND)) {

          // update the sequence pool and reset the new_node flag
          update_pool(&g_seq_pool, rx_packet.source_id, rx_packet.seq_num);
          new_node = NONE;

          // put the message in the right queue based on the type
          switch(rx_packet.type) {
            // command -> do nothing
            case MSG_CMD:
              // do nothing...commands are sent by the gateway, so there is no
              //  no need to pass it along.
              break;
            // command ack -> forward to server
            case MSG_CMDACK: {
              rx_packet.num_hops++;
              nrk_sem_pend(g_serv_tx_queue_mux); {
                push(&g_serv_tx_queue, &rx_packet);
              }
              nrk_sem_post(g_serv_tx_queue_mux);
              break;
            }
            // data received  -> forward to server
            case MSG_DATA: {
              nrk_sem_pend(g_serv_tx_queue_mux); {
                push(&g_serv_tx_queue, &rx_packet);
              }
              nrk_sem_post(g_serv_tx_queue_mux);
              break;
            }
            // handshake message recieved -> deal with in handshake function
            case MSG_HAND: {
              nrk_sem_pend(g_hand_rx_queue_mux); {
                push(&g_hand_rx_queue, &rx_packet);
              }
              nrk_sem_post(g_hand_rx_queue_mux);
              break;
            }

            case MSG_HANDACK: {
              // received an ACK. Do nothing?
              break;
            }
            // gateway message -> do nothing
            case MSG_GATEWAY:{
              // do nothing...no messages have been defined with this type yet
              break;
            }
            // no message
            case MSG_NO_MESSAGE: {
              // do nothing.
              // NOTE: this is a valid case. If the message is not 'parsible' then it can be
              //  given a 'NO_MESSAGE' type.
              break;
            }
            default: {
              // do nothing
              // NOTICE: really this should never happen. Eventually, throw and error here.
              break;
            }
          }
        }
      }
      nrk_led_clr(GREEN_LED);
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
    // only execute if a full server message has been received
    if(get_server_input() == SERV_MSG_RECEIVED) {
      nrk_led_set(BLUE_LED);

      // print message if appropriate
      if(g_print_incoming == TRUE) {
        printf("rx_serv:%s\r\n", g_serv_rx_buf);
      }

      // parse message
      parse_msg(&rx_packet, &g_serv_rx_buf, g_serv_rx_index);
      clear_serv_buf();
      print_packet(&rx_packet);

      /**
       * check sequence number to determine if the packet should be received
       * NOTE: This probably is unnecessary because the likelihood of an earlier
       *  message being delivered serially is very slim. However, one can imagine
       *  that if these were actual network messages that a delay would be much more
       *  probable. Thus, in the spirit of correctness and completeness, we will
       *  keep track of these sequence numbers.
       */
      //if(rx_packet.seq_num > g_server_seq_num) {
      // update local sequence number
      //  g_server_seq_num = rx_packet.seq_num;

      // Note: Right now the gateway is SETTING the server sequence number, making the
      //  assumption that all messages from the server are unique and in order.
      rx_packet.seq_num = g_server_seq_num;
      g_server_seq_num++;
      rx_packet.num_hops++;

      switch(rx_packet.type) {
        // command received
        case MSG_CMD: {
          rx_packet.num_hops++;
          nrk_sem_pend(g_cmd_tx_queue_mux); {
            push(&g_cmd_tx_queue, &rx_packet);
          }
          nrk_sem_post(g_cmd_tx_queue_mux);
          break;
        }
        case MSG_CMDACK:
        case MSG_DATA:
        case MSG_HAND:
        case MSG_GATEWAY:
         // do nothing
          // NOTICE: really, these should never happend. Eventually, throw an error here.
          break;
        case MSG_NO_MESSAGE: {
          // do nothing.
          // NOTE: this is a valid case. If the message is not 'parsible' then it can be
          //  given a 'NO_MESSAGE' type.
          break;
        }
        default: {
          // do nothing
          // NOTICE: really this should never happen. Eventually, throw an error here.
          break;
        }
      }
      nrk_led_clr(BLUE_LED);
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
    // atomically get the queue size
    nrk_sem_pend(g_cmd_tx_queue_mux); {
      tx_cmd_queue_size = g_cmd_tx_queue.size;
    }
    nrk_sem_post(g_cmd_tx_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(g_cmd_tx_queue.size > 0)" was used a few bad things could happen
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
      nrk_led_set(RED_LED);
      // get a packet out of the queue.
      nrk_sem_pend(g_cmd_tx_queue_mux); {
        pop(&g_cmd_tx_queue, &tx_packet);
      }
      nrk_sem_post(g_cmd_tx_queue_mux);

      // NOTE: a mutex is required around the network transmit buffer because
      //  tx_cmd_task() also uses it.
      nrk_sem_pend(g_net_tx_buf_mux); {
        g_net_tx_index = assemble_packet(&g_net_tx_buf, &tx_packet);

        // send the packet
        val = bmac_tx_pkt_nonblocking(g_net_tx_buf, g_net_tx_index);
        ret = nrk_event_wait (SIG(tx_done_signal));

        // Just check to be sure signal is okay
        if(ret & (SIG(tx_done_signal) == 0)) {
          nrk_kprintf (PSTR ("TX done signal error\r\n"));
        }
      }
      nrk_sem_post(g_net_tx_buf_mux);
      nrk_led_clr(RED_LED);
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
    // atomically get the queue size
    nrk_sem_pend(g_node_tx_queue_mux); {
      tx_node_queue_size = g_node_tx_queue.size;
    }
    nrk_sem_post(g_node_tx_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(g_node_tx_queue.size > 0)" was used a few bad things could happen
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
      nrk_led_set(RED_LED);
      // get a packet out of the queue.
      nrk_sem_pend(g_node_tx_queue_mux); {
        pop(&g_node_tx_queue, &tx_packet);
      }
      nrk_sem_post(g_node_tx_queue_mux);

      // NOTE: a mutex is required around the network transmit buffer because
      //  tx_cmd_task() also uses it.
      nrk_sem_pend(g_net_tx_buf_mux); {
      g_net_tx_index = assemble_packet(&g_net_tx_buf, &tx_packet);
        // send the packet
        val = bmac_tx_pkt_nonblocking(g_net_tx_buf, g_net_tx_index);
        ret = nrk_event_wait (SIG(tx_done_signal));

        // Just check to be sure signal is okay
        if(ret & (SIG(tx_done_signal) == 0)) {
          nrk_kprintf (PSTR ("TX done signal error\r\n"));
        }
      }
      nrk_sem_post(g_net_tx_buf_mux);
      nrk_led_clr(RED_LED);
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
    LED_FLAG += 1;
    LED_FLAG %= 2;
    if(LED_FLAG == 0) {
      nrk_led_set(ORANGE_LED);
    }
    else {
      nrk_led_clr(ORANGE_LED);
    }
    // atomically get the queue size
    nrk_sem_pend(g_serv_tx_queue_mux); {
      tx_serv_queue_size = g_serv_tx_queue.size;
    }
    nrk_sem_post(g_serv_tx_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(g_serv_tx_queue.size > 0)" was used a few bad things could happen
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
      nrk_sem_pend(g_serv_tx_queue_mux); {
        pop(&g_serv_tx_queue, &tx_packet);
      }
      nrk_sem_post(g_serv_tx_queue_mux);

      // NOTE: unlike tx_cmd_task() and tx_node_task(), no mutex is required around
      //  the sending buffer here because tx_serv_task() is the only task to use
      //  the serial transmitting buffer (g_serv_tx_buff);
      assemble_serv_packet(&g_serv_tx_buf, &tx_packet);

      // send the packet
      //for (uint8_t x = 0; x < g_serv_tx_index; x++)
      //{
        printf("%s", g_serv_tx_buf);
      //}
      printf("\r\n");
    }

    nrk_wait_until_next_period();
  }
}

void hand_task() {
  uint8_t hand_rx_size;
  packet rx_packet, tx_packet;
  uint8_t in_node_pool;

  tx_packet.source_id = MAC_ADDR;
  tx_packet.type = MSG_HANDACK;
  tx_packet.num_hops = 0;


  while(1) {
    // every iteration of this task will yield a new pool
    pool_t node_pool;

    // atomically get queue size
    nrk_sem_pend(g_hand_rx_queue_mux); {
      hand_rx_size = g_hand_rx_queue.size;
    }
    nrk_sem_post(g_hand_rx_queue_mux);

    /**
     * loop on queue size received above, and no more.
     *  NOTE: during this loop the queue can be added to. If, for instance,
     *    a "while(g_hand_rx_queue.size > 0)" was used a few bad things could happen
     *      (1) a mutex would be required around the entire loop - BAD IDEA
     *      (2) the queue could be added to while this loop is running, thus
     *        making the loop unbounded - BAD IDEA
     *      (3) the size the queue read and the actual size of the queue could be
     *        incorrect due to preemtion - BAD IDEA
     *    Doing it this way bounds this loop to the maximum size of the queue
     *    at any given time, regardless of whether or not the queue has been
     *    added to by another task.
     */
    for(uint8_t i = 0; i < hand_rx_size; i++) {
      // get a packet out of the queue.
      nrk_sem_pend(g_hand_rx_queue_mux); {
        pop(&g_hand_rx_queue, &rx_packet);
      }
      nrk_sem_post(g_hand_rx_queue_mux);
      print_packet(&rx_packet);

      // check to see if the node is in the pool. If so, send HANDACK. Otherwise, ignore
      //in_node_pool = in_pool(&node_pool, rx_packet.source_id);
      //if(in_node_pool == -1) {
       //add_to_pool(&node_pool, rx_packet.source_id, rx_packet.seq_num);

        // finish TX packet
        // NOTE: no mutex is required for seq_num because this is the only task that
        //  uses the gateway's sequence number.
        g_seq_num++;
        tx_packet.seq_num = g_seq_num;
        tx_packet.payload[HANDACK_NODE_ID_INDEX] = rx_packet.source_id;
        tx_packet.payload[HANDACK_CONFIG_ID_INDEX] = rx_packet.payload[HAND_CONFIG_ID_INDEX];
        tx_packet.payload[HANDACK_CONFIG_ID_INDEX + 1] = rx_packet.payload[HAND_CONFIG_ID_INDEX +1];
        tx_packet.payload[HANDACK_CONFIG_ID_INDEX + 2] = rx_packet.payload[HAND_CONFIG_ID_INDEX +2];
        tx_packet.payload[HANDACK_CONFIG_ID_INDEX + 3] = rx_packet.payload[HAND_CONFIG_ID_INDEX +3];

        print_packet(&tx_packet);

        // send response back to the node
        nrk_sem_pend(g_cmd_tx_queue_mux); {
          push(&g_cmd_tx_queue, &tx_packet);
        }
        nrk_sem_post(g_cmd_tx_queue_mux);

        // forward the "hello" message from the node to the server
        nrk_sem_pend(g_serv_tx_queue_mux); {
          push(&g_serv_tx_queue, &tx_packet);
        }
        nrk_sem_post(g_serv_tx_queue_mux);
      //}
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
  RX_NODE_TASK.task = rx_node_task;
  nrk_task_set_stk(&RX_NODE_TASK, rx_node_task_stack, NRK_APP_STACKSIZE);
  RX_NODE_TASK.prio = 6;
  RX_NODE_TASK.FirstActivation = TRUE;
  RX_NODE_TASK.Type = BASIC_TASK;
  RX_NODE_TASK.SchType = PREEMPTIVE;
  RX_NODE_TASK.period.secs = 0;
  RX_NODE_TASK.period.nano_secs = 50*NANOS_PER_MS;
  RX_NODE_TASK.cpu_reserve.secs = 0;
  RX_NODE_TASK.cpu_reserve.nano_secs = 10*NANOS_PER_MS;
  RX_NODE_TASK.offset.secs = 0;
  RX_NODE_TASK.offset.nano_secs = 0;

  RX_SERV_TASK.task = rx_serv_task;
  nrk_task_set_stk(&RX_SERV_TASK, rx_serv_task, NRK_APP_STACKSIZE);
  RX_SERV_TASK.prio = 5;
  RX_SERV_TASK.FirstActivation = TRUE;
  RX_SERV_TASK.Type = BASIC_TASK;
  RX_SERV_TASK.SchType = PREEMPTIVE;
  RX_SERV_TASK.period.secs = 0;
  RX_SERV_TASK.period.nano_secs = 50*NANOS_PER_MS;
  RX_SERV_TASK.cpu_reserve.secs = 0;
  RX_SERV_TASK.cpu_reserve.nano_secs = 10*NANOS_PER_MS;
  RX_SERV_TASK.offset.secs = 0;
  RX_SERV_TASK.offset.nano_secs = 0;

  TX_CMD_TASK.task = tx_cmd_task;
  nrk_task_set_stk(&TX_CMD_TASK, tx_cmd_task_stack, NRK_APP_STACKSIZE);
  TX_CMD_TASK.prio = 4;
  TX_CMD_TASK.FirstActivation = TRUE;
  TX_CMD_TASK.Type = BASIC_TASK;
  TX_CMD_TASK.SchType = PREEMPTIVE;
  TX_CMD_TASK.period.secs = 0;
  TX_CMD_TASK.period.nano_secs = 150*NANOS_PER_MS;
  TX_CMD_TASK.cpu_reserve.secs = 0;
  TX_CMD_TASK.cpu_reserve.nano_secs = 50*NANOS_PER_MS;
  TX_CMD_TASK.offset.secs = 0;
  TX_CMD_TASK.offset.nano_secs = 0;

  TX_NODE_TASK.task = tx_node_task;
  nrk_task_set_stk(&TX_NODE_TASK, tx_node_task_stack, NRK_APP_STACKSIZE);
  TX_NODE_TASK.prio = 3;
  TX_NODE_TASK.FirstActivation = TRUE;
  TX_NODE_TASK.Type = BASIC_TASK;
  TX_NODE_TASK.SchType = PREEMPTIVE;
  TX_NODE_TASK.period.secs = 5;
  TX_NODE_TASK.period.nano_secs = 0;
  TX_NODE_TASK.cpu_reserve.secs = 0;
  TX_NODE_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TX_NODE_TASK.offset.secs = 0;
  TX_NODE_TASK.offset.nano_secs = 0;

  TX_SERV_TASK.task = tx_serv_task;
  nrk_task_set_stk(&TX_SERV_TASK, tx_serv_task_stack, NRK_APP_STACKSIZE);
  TX_SERV_TASK.prio = 2;
  TX_SERV_TASK.FirstActivation = TRUE;
  TX_SERV_TASK.Type = BASIC_TASK;
  TX_SERV_TASK.SchType = PREEMPTIVE;
  TX_SERV_TASK.period.secs = 1;
  TX_SERV_TASK.period.nano_secs = 0;
  TX_SERV_TASK.cpu_reserve.secs = 0;
  TX_SERV_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TX_SERV_TASK.offset.secs = 0;
  TX_SERV_TASK.offset.nano_secs = 0;

  HAND_TASK.task = hand_task;
  nrk_task_set_stk(&HAND_TASK, hand_task_stack, NRK_APP_STACKSIZE);
  HAND_TASK.prio = 1;
  HAND_TASK.FirstActivation = TRUE;
  HAND_TASK.Type = BASIC_TASK;
  HAND_TASK.SchType = PREEMPTIVE;
  HAND_TASK.period.secs = 5;
  HAND_TASK.period.nano_secs = 0;
  HAND_TASK.cpu_reserve.secs = 0;
  HAND_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  HAND_TASK.offset.secs = 0;
  HAND_TASK.offset.nano_secs = 0;

  nrk_activate_task(&RX_NODE_TASK);
  nrk_activate_task(&RX_SERV_TASK);
  nrk_activate_task(&TX_CMD_TASK);
  nrk_activate_task(&TX_NODE_TASK);
  nrk_activate_task(&TX_SERV_TASK);
  nrk_activate_task(&HAND_TASK);

  nrk_kprintf(PSTR("Create done.\r\n"));
}

