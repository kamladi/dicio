/**
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
#include <parser.h>
#include <sequence_pool.h>

// DEFINES
#define MAC_ADDR 1
#define SERVER_MSG_INCOMPLETE 0
#define SERVER_MSG_RECEIVED 1
#define PRINT_ENABLED 1
#define PRINT_DISABLED 0
#define PRESENT 1
#define ABSENT -1

// TASK TYPES
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

// TASK DECLARATIONS
void rx_node_task(void);
void rx_serv_task(void);
void tx_cmd_task(void);
void tx_node_task(void);
void tx_serv_task(void);
void nrk_register_drivers(void);
void nrk_create_taskset(void);


// RECEIVE/TRANSMIT BUFFERS
uint8_t net_rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t serv_rx_buf[RF_MAX_PAYLOAD_SIZE];
uint16_t serv_rx_index = 0;
uint8_t net_tx_buf[RF_MAX_PAYLOAD_SIZE];
nrk_sem_t* net_tx_buf_mux;
uint8_t serv_tx_buf[RF_MAX_PAYLOAD_SIZE];
nrk_sem_t* serv_tx_buf_mux;

packet rx_packet;

// DRIVERS 

// NODE SEQUENCE POOL / SERVER SEQUENCE POOL
// 
sequence_pool_t seq_pool;
uint16_t serv_seq_num;

// LIGHT VALUES/SEMAPHORE
light_pool_t light_pool;
nrk_sem_t *light_pool_mux;

// NEIGHBOR GRAPH/SEMAPHORE
neighbor_graph_t neighbor_graph;
nrk_sem_t *neighbor_graph_mux;

// GLOBAL FLAG/SEMAPHORE
uint8_t print_enable;

// PACKET
packet tx_packet;
uint16_t seq_num = 0;

int main ()
{
  nrk_setup_ports ();
  nrk_setup_uart(UART_BAUDRATE_115K2);

  nrk_init ();

  nrk_led_clr(0);
  nrk_led_clr(1);
  nrk_led_clr(2);
  nrk_led_clr(3);
  
  nrk_time_set(0, 0);
  
  print_enable = 1;

  bmac_init(13);
  bmac_task_config();
  nrk_create_taskset();
  nrk_start();
  
  return 0;
}

void rx_node_task() {
  packet rx_packet;
  uint8_t len, rssi;
  uint8_t* local_buf;
  int8_t in_seq_pool;
  uint16_t local_seq_num;
  uint8_t new_node = NONE;
  uint8_t LED_FLAG = 0;

  bmac_rx_pkt_set_buffer (net_rx_buf, RF_MAX_PAYLOAD_SIZE);
  
  while (!bmac_started ())
    nrk_wait_until_next_period ();

  while(1) {
    // DEBUG LED FLASHING
    if(LED_FLAG == 0) {
      LED_FLAG = 1;
      nrk_led_set(0);
    } else {
      nrk_led_clr(0);
      LED_FLAG = 0;
    }
    
    // is there a packet available?
    if(bmac_rx_pkt_ready()) {
      // get the packet, parse and release
      parse_msg(&rx_packet, &net_rx_buf, len);
      local_buf = bmac_rx_pkt_get(&len, &rssi);
      bmac_rx_pkt_release ();  
      
      // print if appropriate
      if(print_enable == PRINT_ENABLED) {
        printf ("RX: ");
        for (uint8_t i = 0; i < len; i++)
          printf ("%c", local_buf[i]);
        printf("\r\n");        
      }
      
      if(rx_packet.source_id != MAC_ADDR) {
        // check to see if this node is in the sequence pool, if not then add it
        in_seq_pool = in_sequence_pool(&seq_pool, rx_packet.source_id);
        if(in_seq_pool == ABSENT) {
          add_to_sequence_pool(&seq_pool, rx_packet.source_id, rx_packet.seq_num);
          new_node = NODE_FOUND;
        }
      
        // determine if we should parse this packet based on the sequence number
        local_seq_num = get_sequence_number(&seq_pool, rx_packet.source_id);
        if((rx_packet.seq_num > local_seq_num) || (new_node == NODE_FOUND)) {
          // update the sequence pool and reset the new_node flag
          update_sequence_pool(&seq_pool, rx_packet.source_id, rx_packet.seq_num);
          new_node = NONE;
          
          // switch on packet type
          switch(rx_packet.type) {
            case MSG_NODE_SENSOR_VALUE:
              nrk_kprintf(PSTR("Deal with message data.\r\n"));
              break;
            case MSG_GATEWAY:
              nrk_kprintf(PSTR("Deal with command.\r\n"));
              break;
            default:
              nrk_kprintf(PSTR("Other type of command.\r\n"));
              break;            
          }          
        }
      }
    }
    nrk_wait_until_next_period();
  }
}


void clear_serv_rx_buf() {
  for(uint8_t i = 0; i < serv_rx_index; i++) {
    serv_rx_buf[i] = '\0';
  }
  serv_rx_index = 0;
}

uint8_t get_uart_input() {
  char option;
  while(nrk_uart_data_ready(NRK_DEFAULT_UART)) {
    option = getchar();
    serv_rx_buf[serv_rx_index] = option;
    printf("%c", option);
    serv_rx_index++;
    if(option == '\r') {
      serv_rx_buf[serv_rx_index] = '\n';
      serv_rx_index++;
      nrk_kprintf(PSTR("\n"));
      return SERVER_MSG_RECEIVED;    
    }
  }
  return SERVER_MSG_INCOMPLETE;
}

void rx_serv_task() {
  uint16_t rx_seq_num;
  nrk_sig_t uart_rx_signal = nrk_uart_rx_signal_get();
  nrk_signal_register(uart_rx_signal);
  
  serv_seq_num = 0;
  
  while(1) {
    // message received
    if(get_uart_input() == SERVER_MSG_RECEIVED) {
      // get sequence number out of message
      rx_seq_num = serv_seq_num + 1; // replace this with parsing in the future
      
      // only receive this message if the sequence number is higher than the last
      //  seen sequence number
      if(rx_seq_num > serv_seq_num) {
        nrk_kprintf(PSTR("command message received!!\r\n"));
        // TODO: add to cmd_tx_buf
      }
      clear_serv_rx_buf();
    }
    nrk_wait_until_next_period();    
  }
}

void tx_cmd_task() {
  uint8_t i = 0;
  while(1) {
    if(i == 0) {
      i = 1;
      nrk_led_set(1);
    } else {
      nrk_led_clr(1);
      i = 0;
    }
    nrk_wait_until_next_period();    
  }
}

void tx_node_task() {
  /*
  uint8_t i = 0;
  
  // local variable initialization
  uint16_t val;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  
  // Wait until bmac has started. This should be called by all tasks using bmac that do not call bmac_init()
  while (!bmac_started ())
    nrk_wait_until_next_period ();

  // Get and register the tx_done_signal to perform non-blocking transmits
  tx_done_signal = bmac_get_tx_done_signal ();
  nrk_signal_register (tx_done_signal);
  
  while(1) {
    // send the packet
    val = bmac_tx_pkt_nonblocking(tx_buf, strlen(tx_buf));
    ret = nrk_event_wait (SIG(tx_done_signal));
    
    // Just check to be sure signal is okay
    if(ret & SIG(tx_done_signal) == 0 ) {
      nrk_kprintf (PSTR ("TX done signal error\r\n"));
    }
    nrk_wait_until_next_period();
  }
  */
  uint8_t i = 0;
  while(1) {
    if(i == 0) {
      i = 1;
      nrk_led_set(2);
    } else {
      nrk_led_clr(2);
      i = 0;
    }
    nrk_wait_until_next_period();
  }
}

void tx_serv_task() {
  uint8_t i = 0;
  while(1) {
    if(i == 0) {
      i = 1;
      nrk_led_set(4);
    } else {
      nrk_led_clr(4);
      i = 0;
    }
    nrk_wait_until_next_period();    
  }
}

/**
 * nrk_create_taskset - create the tasks in this application
 * 
 * NOTE: task priority maps to importance. That is, priority(5) > priority(2).
 */
void nrk_create_taskset ()
{
  // PRIORITY ??
  RX_NODE_TASK.task = rx_node_task;
  nrk_task_set_stk(&RX_NODE_TASK, rx_node_task_stack, NRK_APP_STACKSIZE);
  RX_NODE_TASK.prio = 3;
  RX_NODE_TASK.FirstActivation = TRUE;
  RX_NODE_TASK.Type = BASIC_TASK;
  RX_NODE_TASK.SchType = PREEMPTIVE;
  RX_NODE_TASK.period.secs = 0;
  RX_NODE_TASK.period.nano_secs = 50*NANOS_PER_MS;
  RX_NODE_TASK.cpu_reserve.secs = 1;
  RX_NODE_TASK.cpu_reserve.nano_secs = 10*NANOS_PER_MS;
  RX_NODE_TASK.offset.secs = 0;
  RX_NODE_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_NODE_TASK);
  
  // PRIORITY ??
  RX_SERV_TASK.task = rx_serv_task;
  nrk_task_set_stk(&RX_SERV_TASK, rx_serv_task_stack, NRK_APP_STACKSIZE);
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
  
  // PRIORITY ??
  TX_CMD_TASK.task = tx_cmd_task;
  nrk_task_set_stk(&TX_CMD_TASK, tx_cmd_task_stack, NRK_APP_STACKSIZE);
  TX_CMD_TASK.prio = 5;
  TX_CMD_TASK.FirstActivation = TRUE;
  TX_CMD_TASK.Type = BASIC_TASK;
  TX_CMD_TASK.SchType = PREEMPTIVE;
  TX_CMD_TASK.period.secs = 0;
  TX_CMD_TASK.period.nano_secs = 100*NANOS_PER_MS;
  TX_CMD_TASK.cpu_reserve.secs = 0;
  TX_CMD_TASK.cpu_reserve.nano_secs = 20*NANOS_PER_MS;
  TX_CMD_TASK.offset.secs = 0;
  TX_CMD_TASK.offset.nano_secs = 0;
  nrk_activate_task(&TX_CMD_TASK);
  
  
  // PRIORITY ??
  TX_NODE_TASK.task = tx_node_task;
  nrk_task_set_stk(&TX_NODE_TASK, tx_node_task_stack, NRK_APP_STACKSIZE);
  TX_NODE_TASK.prio = 1;
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
  
  // PRIORITY ??
  TX_SERV_TASK.task = tx_serv_task;
  nrk_task_set_stk(&TX_SERV_TASK, tx_serv_task_stack, NRK_APP_STACKSIZE);
  TX_SERV_TASK.prio = 2;
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

