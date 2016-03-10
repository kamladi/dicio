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

// ENUMS
typedef enum {
  STATE_INIT_NEIGHBOR,
  STATE_INIT_SENSOR,
  STATE_SET_SENSOR_POLL,
  STATE_SET_NEIGHBOR_UPDATE,
  STATE_PRINT_MAP,
  STATE_PRINT_DATA,
  STATE_WAIT
} states;

// TASKS
nrk_task_type UI_TASK, RECEIVE_TASK, TRANSMIT_TASK;
NRK_STK ui_task_stack[NRK_APP_STACKSIZE];
NRK_STK receive_task_stack[NRK_APP_STACKSIZE];
NRK_STK transmit_task_stack[NRK_APP_STACKSIZE];
void ui_task (void);
void receive_task(void);
void transmit_task(void);
void nrk_create_taskset ();

// BUFFERS
char rx_buf[RF_MAX_PAYLOAD_SIZE];
char tx_buf[RF_MAX_PAYLOAD_SIZE];
char ui_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t ui_index = 0;

// DRIVERS 
void nrk_register_drivers();

// PERIOD VALUES/SEMAPHORES
uint8_t neighbor_update_rate = DEFAULT_NEIGHBOR_RATE;
uint8_t sensor_update_rate = DEFAULT_SENSOR_RATE;
nrk_sem_t *neighbor_mux;
nrk_sem_t *sensor_mux;

// NEIGHBOR VALUES/SEMAPHORES
sequence_pool_t seq_pool;

// LIGHT VALUES/SEMAPHORE
light_pool_t light_pool;
nrk_sem_t *light_pool_mux;

// NEIGHBOR GRAPH/SEMAPHORE
neighbor_graph_t neighbor_graph;
nrk_sem_t *neighbor_graph_mux;

// GLOBAL FLAG/SEMAPHORE
uint8_t print_incoming;
nrk_sem_t *print_mux;

// PACKET
packet tx_packet;
uint16_t seq_num = 0;

int main ()
{
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_init ();

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);
  
  sensor_mux = nrk_sem_create(1, 5);
  neighbor_mux = nrk_sem_create(1,5);
  light_pool_mux = nrk_sem_create(1, 5);
  neighbor_graph_mux = nrk_sem_create(1, 5);
  print_mux = nrk_sem_create(1, 5);
  
  print_incoming = 0;
  
  tx_packet.type = MSG_GATEWAY;
  tx_packet.source_id = MAC_ADDR;
  tx_packet.seq_num = seq_num;
  tx_packet.num_hops = 0;

  nrk_time_set (0, 0);

  bmac_task_config ();

  nrk_create_taskset ();
  bmac_init (13);
  nrk_start ();

  return 0;
}

/** 
 * add_to_ui_buffer - add char to the ui buffer 
 */
void add_to_ui_buffer(char c) {
  ui_buf[ui_index] = c;
  printf("%c", c);
  ui_index++;
}


/**
 * get_user_input - get input from command line
 */
uint8_t get_user_input() {
  char option;
  while(nrk_uart_data_ready(NRK_DEFAULT_UART)) {
    option = getchar();
    add_to_ui_buffer(option);
    if(option == '\r') {
      add_to_ui_buffer('\n');
      nrk_kprintf(PSTR("\n"));
      return 1;    
    }
  }
  return 0;
}

/** 
 * clear_ui_buf - clear the UI buffe
 */
void clear_ui_buf() {
  for(uint8_t i = 0; i < ui_index; i++) {
    ui_buf[i] = '\0';
  }
  ui_index = 0;
}

/** 
 * turnUItoUint - convert user input to a uint duration of seconds. 
 * 
 * NOTE: The range of input values is bounded by MIN_RATE and MAX_RATE. 
 *  Thus, this function is not general, but only for use in this context.
 */
int8_t turnUItoUint() {
  // inital declarations
  uint16_t make = 0;
  int16_t temp = 0;
  
  // loop through entire useful ui_buf (-2 for '\r' and '\n')
  for(uint8_t i = 0; i < (ui_index - 2); i++) {
    temp = ui_buf[i] - '0';
    if((temp < 0) || (temp > 9)) {
      return -1;
    } else {
      make *= 10;
      make += temp;  
      if(make > MAX_RATE) {
        return -1;
      }
    }
  }
  return (int8_t)make;
}


/**
 * set_neighbor_rate - helper function to set the neighbor rate dynamically from 
 *  the terminal gui.
 * 
 * NOTE: This function requires use of a semaphore to ensure the correct access
 *  of a global variable.
 */
uint8_t set_neighbor_rate() {
  int8_t uiUint;
  
  // prompt
  if(get_user_input()) {
    // analyze input
    uiUint = turnUItoUint();
    if((uiUint < MIN_RATE) || (uiUint > MAX_RATE)) {
      nrk_kprintf(PSTR("I'm very sorry, but that is an invalid input.\r\n# "));
      clear_ui_buf();
      return INVALID;
    } else {
      printf("Neighbor update rate set has been set to %d seconds!\r\n", uiUint);
      
      // ATOMIC ACCESS of global neighbor_update_rate
      nrk_sem_pend(neighbor_mux);
      neighbor_update_rate = uiUint;
      nrk_sem_post(neighbor_mux);
      clear_ui_buf();
      return SET;
    }
  }
  return NOT_SET;
}

/**
 * set sensor rate - helper function to set the sensor rate dynamically from the 
 *  terminal gui.
 * 
 * NOTE: This function requires use of a semaphore to ensure the correct access 
 *  of a global variable.
 */
uint8_t set_sensor_rate() {
  int8_t uiUint;
  
  // prompt
  if(get_user_input()) {
    // analyze inputr
    uiUint = turnUItoUint();
    if((uiUint < MIN_RATE) || (uiUint > MAX_RATE)) {
      nrk_kprintf(PSTR("I'm very sorry, but that is an invalid input.\r\n# "));
      clear_ui_buf();
      return INVALID;
    } else {
      printf("Sensor update rate set has been set to %d seconds!\r\n", uiUint);
      
      // ATOMIC ACCESS to global sensor_update_rate
      nrk_sem_pend(sensor_mux);
      sensor_update_rate = uiUint;
      nrk_sem_post(sensor_mux);
      
      clear_ui_buf();
      return SET;
    }
  }
  return NOT_SET;
}

/** 
 * transmit_task - this task forms a message in the TX buffer and sends it
 * to all listening nodes.
 * 
 * NOTE: This function requires use of a semaphore to ensure the correct access 
 *  of a global variable.
 */
void transmit_task() {
  /** TEMP LED STUFF **/
  nrk_led_clr (2);
  nrk_led_clr (3);
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
    /** TEMP LED STUFF **/
    i++;
    i%=4;
    if(i < 2) {
      nrk_led_set(i+2);
    } else {
      nrk_led_clr(i);
    }
    
    // ATOMIC ACCESS to global sensor_update_rate
    nrk_sem_pend(sensor_mux);
    tx_packet.sensor_sample_rate = sensor_update_rate;
    nrk_sem_post(sensor_mux);
    
    // ATOMIC ACCESS to global neighbor_update_rate
    nrk_sem_pend(neighbor_mux);
    tx_packet.neighbor_update_rate = neighbor_update_rate;
    nrk_sem_post(neighbor_mux);
    
    // assemble the rest of the packet
    seq_num++;
    tx_packet.seq_num = seq_num;
    assemble_packet(&tx_buf, &tx_packet);

    // send the packet
    val = bmac_tx_pkt_nonblocking(tx_buf, strlen(tx_buf));
    ret = nrk_event_wait (SIG(tx_done_signal));
    
    // Just check to be sure signal is okay
    if(ret & SIG(tx_done_signal) == 0 ) {
      nrk_kprintf (PSTR ("TX done signal error\r\n"));
    }
    nrk_wait_until_next_period();
  }
}

/**
 * receive_task - receives a message and adds the information to either 
 *  (1) the light data (2) the neighbor data or (3) both.
 */
void receive_task() {
  /** TEMP LED STUFF **/
  nrk_led_clr (0);
  nrk_led_clr (1);
  uint8_t i = 0;
  
  // local variable instantiation
  packet rx_packet;
  uint8_t len, rssi;
  uint8_t *local_buf;
  int8_t in_seq_pool;
  uint16_t local_seq_num;
  uint8_t new_node = NONE;
  uint8_t print;


  bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);
  
  // Wait until bmac has started. This should be called by all tasks using bmac that do not call bmac_init()
  while (!bmac_started ())
    nrk_wait_until_next_period ();
  
  while(1) {
    /** LED indication for debug **/
    i++;
    i%=4;
    if(i < 2) {
      nrk_led_set(i);
    } else {
      nrk_led_clr(i-2);
    }
    
    // if there is a packet available
    if(bmac_rx_pkt_ready()) {
      // get the packet, parse and release
      parse_msg(&rx_packet, &rx_buf, len);
      local_buf = bmac_rx_pkt_get(&len, &rssi);
      
      nrk_sem_pend(print_mux);
        print = print_incoming;
      nrk_sem_post(print_mux);
      if(print == 1) {
        printf ("RX: ");
        for (uint8_t j = 0; j < len; j++)
          printf ("%c", rx_buf[j]);
        printf("\r\n");        
      }
      
      bmac_rx_pkt_release ();  

      // check to see if this node is in the sequence pool, if not then add it
      in_seq_pool = in_sequence_pool(&seq_pool, rx_packet.source_id);
      if((in_seq_pool == -1) && (rx_packet.source_id != MAC_ADDR)) {
        add_to_sequence_pool(&seq_pool, rx_packet.source_id, rx_packet.seq_num);
        new_node = NODE_FOUND;
      }
    
      // determine if we should parse this packet based on the sequence number
      local_seq_num = get_sequence_number(&seq_pool, rx_packet.source_id);
      if((rx_packet.source_id != MAC_ADDR) && 
          ((rx_packet.seq_num > local_seq_num) || (new_node == NODE_FOUND))) {
        
        // update the sequence pool and reset the new_node flag
        update_sequence_pool(&seq_pool, rx_packet.source_id, rx_packet.seq_num);
        new_node = NONE;
        
        // update neighbor_graph/light_pool based on the current message type 
        switch(rx_packet.type) {
          case MSG_NODE_NEIGHBORS:
            nrk_sem_pend(neighbor_graph_mux);
            update_neighbor_graph(&neighbor_graph, &(rx_packet.neighbor_table));
            nrk_sem_post(neighbor_graph_mux);
            // NOTE: Intentional fall through - also should update light value
          case MSG_NODE_SENSOR_VALUE:
            nrk_sem_pend(light_pool_mux);
            update_light_pool(&light_pool, rx_packet.source_id, rx_packet.light_value);
            nrk_sem_post(light_pool_mux);
            break;
          case MSG_NO_MESSAGE:
          case MSG_GATEWAY:
          default:
            // do nothing
            break;
        }
      }
    }
    nrk_wait_until_next_period();
  }
}

/** 
 * ui_task - implements a user interface
 */
void ui_task ()
{
  // get the UART signal and register it
  nrk_sig_t uart_rx_signal = nrk_uart_rx_signal_get();
  nrk_signal_register(uart_rx_signal);
  
  // intitial state information
  states cur_state = STATE_WAIT;
  states last_state = STATE_SET_NEIGHBOR_UPDATE;
  
  // boolean flags to bound print statements
  uint8_t neighbor_set = NOT_SET;
  uint8_t sensor_set = NOT_SET;

  // initial welcome statement
  nrk_kprintf(PSTR("Welcome!\r\n"));
  
  while (1) {
   switch(cur_state) {
      // SET_NEIGHTBOR_RATE - set the neighbor rate and return to the WAIT state
      case STATE_SET_NEIGHBOR_UPDATE:
        // state actions
        if(neighbor_set != SET) {
          if(last_state != cur_state) {
            nrk_kprintf(PSTR("\r\nPlease set the neighbor update rate. (1-30 seconds)\r\n# "));
          }
          neighbor_set = set_neighbor_rate();
        }
        
        // state transitions
        last_state = cur_state;
        if(neighbor_set == SET) {
          neighbor_set = NOT_SET; // reset flag
          cur_state = STATE_WAIT;          
        } else {
          cur_state = STATE_SET_NEIGHBOR_UPDATE;
        }
        break;
        
      // SET SENSOR POLL - set the sensor poll rate and return to the WAIT state
      case STATE_SET_SENSOR_POLL:
        if(sensor_set != SET) {
          if(last_state != cur_state) {
            nrk_kprintf(PSTR("\r\nPlease set the sensor update rate. (1-30 seconds)\r\n# "));
          }
          sensor_set = set_sensor_rate();
        }
        
        // state transitions
        last_state = cur_state;
        if(sensor_set == SET) {
          sensor_set = NOT_SET;
          cur_state = STATE_WAIT;
        } else {
          cur_state = STATE_SET_SENSOR_POLL;
        }
        break;
        
      // PRINT MAP - print the neighbor map and return to the WAIT state
      case STATE_PRINT_MAP:
        // state actions
        nrk_sem_pend(neighbor_graph_mux);
        print_neighbor_graph(&neighbor_graph);
        nrk_sem_post(neighbor_graph_mux);        
        // state transitions
        last_state = cur_state;
        cur_state = STATE_WAIT;
        break;
      
      // PRINT DATA - print light data and return to the WAIT state
      case STATE_PRINT_DATA:
        // state actions
        nrk_sem_pend(light_pool_mux);
        print_light_pool(&light_pool);
        nrk_sem_post(light_pool_mux);

        // state transitions
        last_state = cur_state;
        cur_state = STATE_WAIT;
        break;
      
      // WAIT - wait for user input and react
      case STATE_WAIT:
      default:
        // state output
        if(last_state != STATE_WAIT) {
          nrk_kprintf(PSTR("\r\nHere are your options...\r\n"));
          nrk_kprintf(PSTR("  n: update neighbor refresh rate.\r\n"));
          nrk_kprintf(PSTR("  s: update sensor refresh rate.\r\n"));
          nrk_kprintf(PSTR("  m: view current neighbor dependencies.\r\n"));
          nrk_kprintf(PSTR("  l: view current light values.\r\n"));
          nrk_kprintf(PSTR("  p: stream incoming packets.\r\n# "));
        }

        // state transitions
        last_state = cur_state;
        if(get_user_input()) {
          if(ui_index == 3) {
            if(ui_buf[0] == 'n') {
              cur_state = STATE_SET_NEIGHBOR_UPDATE;
            } else if (ui_buf[0] == 's') {
              cur_state = STATE_SET_SENSOR_POLL;
            } else if (ui_buf[0] == 'm') {
              cur_state = STATE_PRINT_MAP;
            } else if (ui_buf[0] == 'l') {
              cur_state = STATE_PRINT_DATA;
            } else if (ui_buf[0] == 'p') {
              nrk_sem_pend(print_mux);
              if(print_incoming == 1) {
                nrk_kprintf(PSTR("Streaming off.\r\n# "));
                print_incoming = 0;
              } else {
                nrk_kprintf(PSTR("Streaming on.\r\n"));
                print_incoming = 1;
              }
              nrk_sem_post(print_mux);
            } else {
              nrk_kprintf(PSTR("Invalid input.\r\n# "));
              cur_state = STATE_WAIT;
            }
          } else {
            nrk_kprintf(PSTR("Invalid input.\r\n# "));
            cur_state = STATE_WAIT;
          }
          clear_ui_buf();
        } else {
          cur_state = STATE_WAIT;
        }
        break;
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
  // PRIORITY 2
  TRANSMIT_TASK.task = transmit_task;
  nrk_task_set_stk( &TRANSMIT_TASK, transmit_task_stack, NRK_APP_STACKSIZE);
  TRANSMIT_TASK.prio = 2;
  TRANSMIT_TASK.FirstActivation = TRUE;
  TRANSMIT_TASK.Type = BASIC_TASK;
  TRANSMIT_TASK.SchType = PREEMPTIVE;
  TRANSMIT_TASK.period.secs = 5;
  TRANSMIT_TASK.period.nano_secs = 0;
  TRANSMIT_TASK.cpu_reserve.secs = 1;
  TRANSMIT_TASK.cpu_reserve.nano_secs = 0;
  TRANSMIT_TASK.offset.secs = 0;
  TRANSMIT_TASK.offset.nano_secs = 0;
  nrk_activate_task (&TRANSMIT_TASK);
  
  // PRIORITY 3
  UI_TASK.task = ui_task;
  nrk_task_set_stk( &UI_TASK, ui_task_stack, NRK_APP_STACKSIZE);
  UI_TASK.prio = 3;
  UI_TASK.FirstActivation = TRUE;
  UI_TASK.Type = BASIC_TASK;
  UI_TASK.SchType = PREEMPTIVE;
  UI_TASK.period.secs = 0;
  UI_TASK.period.nano_secs = 100*NANOS_PER_MS;
  UI_TASK.cpu_reserve.secs = 0;
  UI_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  UI_TASK.offset.secs = 0;
  UI_TASK.offset.nano_secs = 0;
  nrk_activate_task (&UI_TASK);
  
  // PRIORITY 4
  RECEIVE_TASK.task = receive_task;
  nrk_task_set_stk( &RECEIVE_TASK, receive_task_stack, NRK_APP_STACKSIZE);
  RECEIVE_TASK.prio = 4;
  RECEIVE_TASK.FirstActivation = TRUE;
  RECEIVE_TASK.Type = BASIC_TASK;
  RECEIVE_TASK.SchType = PREEMPTIVE;
  RECEIVE_TASK.period.secs = 0;
  RECEIVE_TASK.period.nano_secs = 10*NANOS_PER_MS;
  RECEIVE_TASK.cpu_reserve.secs = 0;
  RECEIVE_TASK.cpu_reserve.nano_secs = 10*NANOS_PER_MS;
  RECEIVE_TASK.offset.secs = 0;
  RECEIVE_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RECEIVE_TASK);
  
  nrk_kprintf(PSTR("Create done.\r\n"));
}

