/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * main.c (node)
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

// INCLUDES
// standard nrk
#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <adc_driver.h>
#include <bmac.h>
// this package
#include <adc.h>
#include <assembler.h>
#include <dicio_spi.h>
#include <power_sensor.h>
#include <packet_queue.h>
#include <parser.h>
#include <pool.h>
#include <type_defs.h>

// DEFINES
#define MAC_ADDR 3
#define HARDWARE_REV 0xD1C10000

// FUNCTION DECLARATIONS
int main(void);
void clear_tx_buf(void);
void rx_msg_task(void);
void tx_cmd_task(void);
void tx_data_task(void);
void sample_task(void);
void button_task(void);
void actuate_task(void);
void heartbeat_task(void);
void nrk_set_gpio(void);
uint8_t get_global_outlet_state();
void update_global_outlet_state(uint8_t new_state);
void nrk_register_drivers(void);
void nrk_create_taskset(void);

// STATE ENUM (used in actuate task)
typedef enum {
  STATE_ON,
  STATE_OFF,
  STATE_ACT_OFF,
  STATE_ACT_ON,
  STATE_ACK_OFF,
  STATE_ACK_ON
} act_state;

typedef enum {
  STATE_SNIFF,
  STATE_WAIT,
} button_pressed_state;

// TASKS
nrk_task_type RX_MSG_TASK;
nrk_task_type TX_CMD_TASK;
nrk_task_type TX_DATA_TASK;
nrk_task_type SAMPLE_TASK;
nrk_task_type ACTUATE_TASK;
nrk_task_type BUTTON_TASK;
nrk_task_type HEARTBEAT_TASK;

// TASK STACKS
NRK_STK rx_msg_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_cmd_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_data_task_stack[NRK_APP_STACKSIZE];
NRK_STK sample_task_stack[NRK_APP_STACKSIZE];
NRK_STK actuate_task_stack[NRK_APP_STACKSIZE];
NRK_STK button_task_stack[NRK_APP_STACKSIZE];
NRK_STK heartbeat_task_stack[NRK_APP_STACKSIZE];

// BUFFERS
uint8_t g_net_rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t g_net_tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t g_net_tx_index = 0;
nrk_sem_t* g_net_tx_buf_mux;

// QUEUES
packet_queue g_act_queue;
nrk_sem_t* g_act_queue_mux;
uint16_t g_last_command = 0;

packet_queue g_cmd_tx_queue;
nrk_sem_t* g_cmd_tx_queue_mux;

packet_queue g_data_tx_queue;
nrk_sem_t* g_data_tx_queue_mux;

// SENSOR VALUES
uint8_t g_adc_fd;
uint8_t g_pwr_period;
uint8_t g_temp_period;
uint8_t g_light_period;
sensor_packet g_sensor_pkt;

// SEQUENCE POOLS/NUMBER
pool_t g_seq_pool;
uint16_t g_seq_num = 0;
nrk_sem_t* g_seq_num_mux;

// WATCHDOG TIMER
int8_t g_net_watchdog = HEART_FACTOR;
nrk_sem_t* g_net_watchdog_mux;

// GLOBAL FLAGS
uint8_t g_print_incoming;
uint8_t g_network_joined;
nrk_sem_t* g_network_joined_mux;
uint8_t g_global_outlet_state;
nrk_sem_t *g_global_outlet_state_mux;
uint8_t g_button_pressed;
nrk_sem_t* g_button_pressed_mux;

int main() {
  packet act_packet;

  // setup ports/uart
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);
  SPI_Init();
  pwr_init();

  nrk_init ();
  nrk_time_set (0, 0);

  // clear all LEDs
  nrk_led_clr(0);
  nrk_led_clr(1);
  nrk_led_clr(2);
  nrk_led_clr(3);

  // flags
  g_print_incoming      = TRUE;
  g_network_joined      = FALSE;
  g_global_outlet_state = OFF;
  g_button_pressed  = FALSE;

  // mutexs
  g_net_tx_buf_mux          = nrk_sem_create(1, 7);
  g_act_queue_mux           = nrk_sem_create(1, 7);
  g_cmd_tx_queue_mux        = nrk_sem_create(1, 7);
  g_data_tx_queue_mux       = nrk_sem_create(1, 7);
  g_seq_num_mux             = nrk_sem_create(1, 7);
  g_network_joined_mux      = nrk_sem_create(1, 7);
  g_global_outlet_state_mux = nrk_sem_create(1, 7);
  g_button_pressed_mux      = nrk_sem_create(1, 7);
  g_net_watchdog_mux       = nrk_sem_create(1, 7);

  // sensor periods (in seconds / 2)
  g_pwr_period = 1;
  g_temp_period = 2;
  g_light_period = 10;

  // packet queues
  packet_queue_init(&g_act_queue);
  packet_queue_init(&g_cmd_tx_queue);
  packet_queue_init(&g_data_tx_queue);

  // ensure node is initially set to "OFF"
  act_packet.source_id = MAC_ADDR;
  act_packet.type = MSG_CMD;
  act_packet.seq_num = 0;
  act_packet.num_hops = 0;
  act_packet.payload[CMD_ID_INDEX] = (uint16_t)0;
  act_packet.payload[CMD_NODE_ID_INDEX] = MAC_ADDR;
  act_packet.payload[CMD_ACT_INDEX] = OFF;
  nrk_sem_pend(g_act_queue_mux); {
    push(&g_act_queue, &act_packet);
    push(&g_act_queue, &act_packet);
  }
  nrk_sem_post(g_act_queue_mux);

  // initialize bmac
  bmac_task_config ();
  bmac_init (13);

  nrk_register_drivers();
  nrk_set_gpio();
  nrk_create_taskset();
  nrk_start ();

  return 0;
}

/**
 * Wrapper function to control access to retrieving the global outlet state
 * variable.
 * @return uint8_t current global outlet state (either ON or OFF)
 */
uint8_t get_global_outlet_state() {
  uint8_t outlet_state = OFF;
  nrk_sem_pend(g_global_outlet_state_mux); {
    outlet_state = g_global_outlet_state;
  }
  nrk_sem_post(g_global_outlet_state_mux);
  return outlet_state;
}

/**
 * Wrapper function to control access to setting the global outlet state
 * variable.
 * @param new_state uint8_t (either ON or OFF)
 */
void update_global_outlet_state(uint8_t new_state) {
  nrk_sem_pend(g_global_outlet_state_mux); {
    g_global_outlet_state = new_state;
  }
  nrk_sem_post(g_global_outlet_state_mux);
}

void clear_tx_buf(){
  for(uint8_t i = 0; i < g_net_tx_index; i++) {
    g_net_tx_buf[i] = 0;
  }
  g_net_tx_index = 0;
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
  uint8_t local_network_joined = FALSE;

  printf("rx_msg_task PID: %d.\r\n", nrk_get_pid());
  // initialize network receive buffer
  bmac_rx_pkt_set_buffer(g_net_rx_buf, RF_MAX_PAYLOAD_SIZE);

  // Wait until bmac has started. This should be called by all tasks using bmac that do not call bmac_init()
  while (!bmac_started ()) {
    nrk_wait_until_next_period ();
  }

  // loop forever
  while(1) {
    // only execute if there is a packet available
    if(bmac_rx_pkt_ready()) {
      nrk_led_set(BLUE_LED);
      // get the packet, parse and release
      parse_msg(&rx_packet, &g_net_rx_buf, len);
      local_buf = bmac_rx_pkt_get(&len, &rssi);
      bmac_rx_pkt_release ();

      // print incoming packet if appropriate
      if(g_print_incoming == TRUE) {
        print_packet(&rx_packet);
      }

      // only receive the message if it's not from this node
      //  NOTE: this is required because the node will hear re-transmitted packets
      //    originally from itself.
      if(rx_packet.source_id != MAC_ADDR) {
        // execute the normal sequence of events if the network has been joined
        if(local_network_joined == TRUE) {
          // check to see if this node is in the sequence pool, if not then add it
          in_seq_pool = in_pool(&g_seq_pool, rx_packet.source_id);
          if(in_seq_pool == -1) {
            add_to_pool(&g_seq_pool, rx_packet.source_id, rx_packet.seq_num);
            new_node = NODE_FOUND;
          }

          // determine if we should act on this packet based on the sequence number
          local_seq_num = get_data_val(&g_seq_pool, rx_packet.source_id);
          if((rx_packet.seq_num > local_seq_num) || (new_node == NODE_FOUND)) {

            // update the sequence pool and reset the new_node flag
            update_pool(&g_seq_pool, rx_packet.source_id, rx_packet.seq_num);
            new_node = NONE;

            // put the message in the right queue based on the type
            switch(rx_packet.type) {
              // no message
              case MSG_NO_MESSAGE: {
                // do nothing.
                // NOTE: this is a valid case. If the message is not 'parsible' then it can be
                //  given a 'NO_MESSAGE' type.
                break;
              }
              // gateway message -> for future expansion
              case MSG_GATEWAY: {
                // do nothing...no messages have been defined with this type yet
                break;
              }
              // data received -> forward to server
              case MSG_DATA: {
                rx_packet.num_hops++;
                nrk_sem_pend(g_data_tx_queue_mux); {
                  push(&g_data_tx_queue, &rx_packet);
                }
                nrk_sem_post(g_data_tx_queue_mux);
                break;
              }
              // command received -> forward or actuate
              case MSG_CMD: {
                // if command is for this node and hasn't been received yet, add it
                //  to the action queue. Otherwise, add it to the cmd_tx queue for
                //  forwarding to other nodes.
                if(rx_packet.payload[CMD_NODE_ID_INDEX] == MAC_ADDR) {
                  nrk_kprintf (PSTR ("Received command.\r\n"));
                  g_last_command = (uint16_t)rx_packet.payload[CMD_ID_INDEX]; // need to cast again here right?
                  nrk_sem_pend(g_act_queue_mux); {
                    push(&g_act_queue, &rx_packet);
                  }
                  nrk_sem_post(g_act_queue_mux);
                }
                else {
                  rx_packet.num_hops++;
                  nrk_sem_pend(g_cmd_tx_queue_mux); {
                    push(&g_cmd_tx_queue, &rx_packet);
                  }
                  nrk_sem_post(g_cmd_tx_queue_mux);
                }
                break;
              }
              // command ack received -> forward to the server
              case MSG_CMDACK: {
                rx_packet.num_hops++;
                nrk_sem_pend(g_cmd_tx_queue_mux); {
                  push(&g_cmd_tx_queue, &rx_packet);
                }
                nrk_sem_post(g_cmd_tx_queue_mux);
                break;
              }
              // handshake message -> forward to the server
              // NOTE: will only receive type MSG_HAND to forward
              case MSG_HAND: {
                rx_packet.num_hops++;
                nrk_sem_pend(g_data_tx_queue_mux); {
                  push(&g_data_tx_queue, &rx_packet);
                }
                nrk_sem_post(g_data_tx_queue_mux);
                break;
              }
              // handshake ack message -> forward to the server
              case MSG_HANDACK: {
                rx_packet.num_hops++;
                nrk_sem_pend(g_data_tx_queue_mux); {
                  push(&g_data_tx_queue, &rx_packet);
                }
                nrk_sem_post(g_data_tx_queue_mux);
                break;
              }
              // heartbeat message -> forward to the server and
              //  kick the watchdog counter
              case MSG_HEARTBEAT: {
                rx_packet.num_hops++;
                nrk_sem_pend(g_data_tx_queue_mux); {
                  push(&g_data_tx_queue, &rx_packet);
                }
                nrk_sem_post(g_data_tx_queue_mux);

                nrk_sem_pend(g_net_watchdog_mux); {
                  g_net_watchdog = HEART_FACTOR;
                }
                nrk_sem_post(g_net_watchdog_mux);
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
        // if the local_network_joined flag hasn't been set yet, check status
        else {
          // if a handshake ack has been received, then set the network joined flag. Otherwise, ignore.
          if((rx_packet.type == MSG_HANDACK) && (rx_packet.payload[HANDACK_NODE_ID_INDEX] == MAC_ADDR)) {
            nrk_sem_pend(g_network_joined_mux); {
              nrk_kprintf (PSTR ("Received HAND ACK.\r\n"));
              g_network_joined = TRUE;
              local_network_joined = g_network_joined;
            }
            nrk_sem_post(g_network_joined_mux);
          }
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
  uint8_t local_network_joined = FALSE;

  printf("tx_cmd_task pid %d\r\n", nrk_get_pid());

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
      nrk_led_set(ORANGE_LED);
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
          nrk_kprintf(PSTR("TX done signal error\r\n"));
        }
        clear_tx_buf();
      }
      nrk_sem_post(g_net_tx_buf_mux);
      nrk_led_clr(ORANGE_LED);
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
  uint8_t local_network_joined = FALSE;

  printf("tx_data_task PID: %d.\r\n", nrk_get_pid());

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
    nrk_sem_pend(g_data_tx_queue_mux); {
      tx_data_queue_size = g_data_tx_queue.size;
    }
    nrk_sem_post(g_data_tx_queue_mux);
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
      nrk_led_set(ORANGE_LED);
      // get a packet out of the queue.
      nrk_sem_pend(g_data_tx_queue_mux); {
        pop(&g_data_tx_queue, &tx_packet);
      }
      nrk_sem_post(g_data_tx_queue_mux);

      if(g_print_incoming == TRUE){
        //nrk_kprintf (PSTR ("Asm pkt:\r\n"));
        //print_packet(&tx_packet);
      }

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
        clear_tx_buf();
      }
      nrk_sem_post(g_net_tx_buf_mux);

      nrk_led_clr(ORANGE_LED);
    }
    nrk_wait_until_next_period();
  }
}

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
  packet tx_packet, hello_packet;
  uint8_t local_network_joined = FALSE;
  int8_t val, chan;
  uint16_t adc_buf[2];
  uint8_t pwr_rcvd[3];

  printf("sample_task PID: %d.\r\n", nrk_get_pid());

  // initialize sensor packet
  g_sensor_pkt.pwr_val = local_pwr_val;
  g_sensor_pkt.temp_val = local_temp_val;
  g_sensor_pkt.light_val = local_light_val;

  // initialize tx_packet
  tx_packet.source_id = MAC_ADDR;
  tx_packet.type = MSG_DATA;
  tx_packet.num_hops = 0;

  // initialize hello packet
  hello_packet.source_id = MAC_ADDR;
  hello_packet.type = MSG_HAND;
  hello_packet.num_hops = 0;
  hello_packet.payload[0] = (HARDWARE_REV >> 24) & 0xff;
  hello_packet.payload[1] = (HARDWARE_REV >> 16) & 0xff;
  hello_packet.payload[2] = (HARDWARE_REV >> 8) & 0xff;
  hello_packet.payload[3] = (HARDWARE_REV) & 0xff;

  print_packet(&hello_packet);

  // Open ADC device as read
  g_adc_fd = nrk_open(ADC_DEV_MANAGER,READ);
  if(g_adc_fd == NRK_ERROR) {
    nrk_kprintf(PSTR("Failed to open ADC driver\r\n"));
  }

  while (1) {
    if(local_network_joined == TRUE) {
      // update period counts
      pwr_period_count++;
      temp_period_count++;
      light_period_count++;
      pwr_period_count %= g_pwr_period;
      temp_period_count %= g_temp_period;
      light_period_count %= g_light_period;
      sensor_sampled = FALSE;

      // sample power sensor if appropriate
      if(pwr_period_count == SAMPLE_SENSOR) {
        // requrest temperature
        pwr_read(WATT, &pwr_rcvd);

        // pull out dinner location
        local_pwr_val = (pwr_rcvd[0] << 8) | pwr_rcvd[1];
        g_sensor_pkt.pwr_val = local_pwr_val;
        sensor_sampled = TRUE;

        // for debug -> print
        printf("%x\r\n", local_pwr_val);
      }

      // sample temperature sensor if appropriate
      if(temp_period_count == SAMPLE_SENSOR) {
        // SAMPLE TEMP SENSOR
        val = nrk_set_status(g_adc_fd,ADC_CHAN,CHAN_6);
        if(val == NRK_ERROR) {
          nrk_kprintf(PSTR("Failed to set ADC status\r\n"));
        } else {
          val = nrk_read(g_adc_fd, &adc_buf[0],2);
          if(val == NRK_ERROR)  {
            nrk_kprintf(PSTR("Failed to read ADC\r\n"));
          } else {
            local_temp_val = (uint16_t)adc_buf[0];
            g_sensor_pkt.temp_val = transform_temp(local_temp_val);
            printf("TEMP: %d\r\n", transform_temp(local_temp_val));
            sensor_sampled = TRUE;

          }
        }
      }

      // sample light sensor if appropriate
      if(light_period_count == SAMPLE_SENSOR) {
        //TODO: SAMPLE LIGHT SENSOR
        local_light_val++;
        g_sensor_pkt.light_val = local_light_val;
        sensor_sampled = TRUE;

        val = nrk_set_status(g_adc_fd,ADC_CHAN,CHAN_5);
        if(val == NRK_ERROR) {
          nrk_kprintf(PSTR("Failed to set ADC status\r\n"));
        } else {
          val = nrk_read(g_adc_fd, &adc_buf[1],2);
          if(val == NRK_ERROR){
            nrk_kprintf(PSTR("Failed to read ADC\r\n"));
          } else {
            local_light_val = (uint16_t)adc_buf[1];
            g_sensor_pkt.light_val = local_light_val;

            sensor_sampled = TRUE;
            printf("LIGHT: %d\r\n", local_light_val);
          }
        }
      }

      // if a sensor has been sampled, send a packet out
      if(sensor_sampled == TRUE) {
        // update sequence number
        nrk_sem_pend(g_seq_num_mux); {
          g_seq_num++;
          tx_packet.seq_num = g_seq_num;
        }
        nrk_sem_post(g_seq_num_mux);

        // add data values to sensor packet
        tx_packet.payload[DATA_PWR_INDEX] = g_sensor_pkt.pwr_val;
        tx_packet.payload[DATA_TEMP_INDEX] = g_sensor_pkt.temp_val;
        tx_packet.payload[DATA_LIGHT_INDEX] = g_sensor_pkt.light_val;
        tx_packet.payload[DATA_STATE_INDEX] = get_global_outlet_state();

        // add packet to data queue
        nrk_sem_pend(g_data_tx_queue_mux); {
          push(&g_data_tx_queue, &tx_packet);
        }
        nrk_sem_post(g_data_tx_queue_mux);
      }
    }
    // if the local_network_joined flag hasn't been set yet, check status
    else {
      nrk_sem_pend(g_network_joined_mux); {
        local_network_joined = g_network_joined;
        //local_network_joined = TRUE;
        //g_network_joined = TRUE;
      }
      nrk_sem_post(g_network_joined_mux);

      // if the network has not yet been joined, then add "Hello" message
      //  to the g_data_tx_queue
      if(local_network_joined == FALSE) {
        // update seq num
        nrk_sem_pend(g_seq_num_mux); {
          g_seq_num++;
          hello_packet.seq_num = g_seq_num;
        }
        nrk_sem_post(g_seq_num_mux);

        // push to queue
        nrk_sem_pend(g_cmd_tx_queue_mux); {
          push(&g_cmd_tx_queue, &hello_packet);
        }
        nrk_sem_post(g_cmd_tx_queue_mux);
      }
    }
    nrk_wait_until_next_period();
  }
}

void button_task() {
  button_pressed_state curr_state = STATE_SNIFF;
  uint8_t local_button_pressed = FALSE;

  printf("button_task PID: %d.\r\n", nrk_get_pid());


  while(1) {
    switch(curr_state) {
      // STATE_SNIFF:
      //    - if the button gets pressed set the flag and switch states
      //    - otherwise, keep sniffing
      case STATE_SNIFF: {
        // get current button_pressed state
        nrk_sem_pend(g_button_pressed_mux); {
          local_button_pressed = g_button_pressed;
        }
        nrk_sem_post(g_button_pressed_mux);

        // check button input
        if((nrk_gpio_get(BTN_IN) == BUTTON_PRESSED) && (local_button_pressed == FALSE)) {
          // set flag
          nrk_sem_pend(g_button_pressed_mux); {
            g_button_pressed = TRUE;
          }
          nrk_sem_post(g_button_pressed_mux);

          // switch states
          curr_state = STATE_WAIT;
        }
        // keep sniffing...
        else {
          curr_state = STATE_SNIFF;
        }
        break;
      }

      // STATE_WAIT:
      //  - wait for button to be released and switch states
      case STATE_WAIT: {
        // if the button is released - start sniffing
        if(nrk_gpio_get(BTN_IN) == BUTTON_RELEASED) {
          curr_state = STATE_SNIFF;
        }
        // otherwise
        else {
          curr_state = STATE_WAIT;
        }
        break;
      }
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
  uint8_t ACT_FLAG = 0;
  uint8_t act_queue_size;
  packet act_packet, tx_packet;
  uint8_t ack_required, act_required;
  int8_t action;
  uint8_t btn_val;
  uint8_t local_network_joined = FALSE;
  uint8_t local_button_pressed = FALSE;

  printf("actuate_task PID: %d.\r\n", nrk_get_pid());

  // CURRENT STATE
  // NOTE: set initially to "ON" so that when initial "OFF" commands come through
  //  they will actually be paid attention to.
  act_state curr_state = STATE_ON;

  // initialize tx_packet
  tx_packet.source_id = MAC_ADDR;
  tx_packet.type = MSG_CMDACK;
  tx_packet.num_hops = 0;

  // clear the GPIOs - these are ACTIVE LOW, i.e. this is not actuating
  nrk_gpio_set(ON_COIL);
  nrk_gpio_set(OFF_COIL);

  // loop forever
  while(1) {
    // get action queue size / reset action flag
    nrk_sem_pend(g_act_queue_mux); {
      act_queue_size = g_act_queue.size;
    }
    nrk_sem_post(g_act_queue_mux);
    action = ACT_NONE;

    // get button pressed
    nrk_sem_pend(g_button_pressed_mux); {
      local_button_pressed = g_button_pressed;
    }
    nrk_sem_post(g_act_queue_mux);

    switch(curr_state) {
      // STATE_OFF -
      //  - wait for a button press -> actuate ON
      //  - wait for an ON command -> actuate
      //  - wait for an OFF command -> send ACK
      case STATE_OFF: {
        // check button state
        if(local_button_pressed == TRUE) {
          action = ON;
        }
        // check act queue
        else if(act_queue_size > 0) {
          // get the action atomically
          nrk_sem_pend(g_act_queue_mux); {
            pop(&g_act_queue, &act_packet);
          }
          nrk_sem_post(g_act_queue_mux);
          action = act_packet.payload[CMD_ACT_INDEX];
        }
        // if the action is ON -> actuate
        if(action == ON) {
          curr_state = STATE_ACT_ON;
        }
        // if the action is OFF -> send ACK
        else if (action == OFF) {
          curr_state = STATE_ACK_OFF;
        }
        // if the action is something else...there is a problem
        else {
          curr_state = STATE_OFF;
        }
        break;
      }
      // STATE_ON -
      //  - wait for a buttton press -> actuate OFF
      //  - wait for an ON command -> send ACK
      //  - wait for an OFF command -> actuate
      case STATE_ON: {
        // check button state
        if(local_button_pressed == TRUE) {
          action = OFF;
        }
        // check act queue
        else if(act_queue_size > 0) {
          nrk_sem_pend(g_act_queue_mux); {
            pop(&g_act_queue, &act_packet);
          }
          nrk_sem_post(g_act_queue_mux);
          action = act_packet.payload[CMD_ACT_INDEX];
        }

        // if the action is ON -> send ACK
        if(action == ON) {
          curr_state = STATE_ACK_ON;
        }
        // if the action is OFF -> actuate
        else if(action == OFF) {
          curr_state = STATE_ACT_OFF;
        }
        // if the action is something else...there is a problem
        else {
          curr_state = STATE_ON;
        }
        break;
      }

      // STATE_ACT_OFF - actuate the OFF_COIL
      case STATE_ACT_OFF: {
        // ACTIVE LOW signal
        nrk_gpio_clr(OFF_COIL);
        curr_state = STATE_ACK_OFF;
        break;
      }

      // STATE_ACT_ON - actuate the ON_COIL
      case STATE_ACT_ON: {
        // ACTIVE_HIGH signal
        nrk_gpio_clr(ON_COIL);
        curr_state = STATE_ACK_ON;
        break;
      }

      // STATE_ACT_OFF -
      //  - clear the control signal
      //  - send ACK
      case STATE_ACK_OFF: {
        // clear control signal
        nrk_gpio_set(OFF_COIL);

        // send ack if the network has been joined
        if(local_network_joined == TRUE) {
          // update sequence number
          nrk_sem_pend(g_seq_num_mux); {
            g_seq_num++;
            tx_packet.seq_num = g_seq_num;
          }
          nrk_sem_post(g_seq_num_mux);

          // set payload
          tx_packet.payload[CMDACK_ID_INDEX] = (uint16_t)act_packet.payload[CMD_ID_INDEX];
          tx_packet.payload[CMDACK_STATE_INDEX] = OFF;

          // place message in the queue
          nrk_sem_pend(g_cmd_tx_queue_mux); {
            push(&g_cmd_tx_queue, &tx_packet);
          }

          nrk_sem_post(g_cmd_tx_queue_mux);
        }

        // update global outlet state
        update_global_outlet_state(OFF);

        // this will flag if the command just executed was from a physical button press
        //  if so, reset the global flag
        if(local_button_pressed = TRUE) {
          nrk_sem_pend(g_button_pressed_mux); {
            g_button_pressed = FALSE;
          }
          nrk_sem_post(g_button_pressed_mux);
        }

        // next state -> STATE_OFF
        curr_state = STATE_OFF;
        break;
      }

      // STATE_ACK_ON -
      //  - clear the control signal
      //  - send ACK
      case STATE_ACK_ON: {
        // clear control signal
        nrk_gpio_set(ON_COIL);

        // send ack if network has been joined
        if(local_network_joined == TRUE) {
          // update sequence number
          nrk_sem_pend(g_seq_num_mux); {
            g_seq_num++;
            tx_packet.seq_num = g_seq_num;
          }
          nrk_sem_post(g_seq_num_mux);

          // set payload
          tx_packet.payload[CMDACK_ID_INDEX] = (uint16_t)act_packet.payload[CMD_ID_INDEX];
          tx_packet.payload[CMDACK_STATE_INDEX] = ON;

          // place message in the queue
          nrk_sem_pend(g_cmd_tx_queue_mux); {
            push(&g_cmd_tx_queue, &tx_packet);
          }

          nrk_sem_post(g_cmd_tx_queue_mux);
        }

        // update global outlet state
        update_global_outlet_state(ON);

        // this will flag if the command just executed was from a physical button press
        //  if so, reset the global flag
        if(local_button_pressed = TRUE) {
          nrk_sem_pend(g_button_pressed_mux); {
            g_button_pressed = FALSE;
          }
          nrk_sem_post(g_button_pressed_mux);
        }

        // next state -> STATE_ON
        curr_state = STATE_ON;
        break;
      }
    }

    if(local_network_joined == FALSE) {
      // determine if the network has been joined
      nrk_sem_pend(g_network_joined_mux); {
        local_network_joined = g_network_joined;
      }
      nrk_sem_post(g_network_joined_mux);
    }
    nrk_wait_until_next_period();
  }
}

/**
 * heartbeat_task() - 
 *  updated heartbeat counter, check to see if the timer has been expired.
 */
void heartbeat_task() {
  int8_t local_watchdog;
  uint8_t local_network_joined = FALSE;

  printf("heartbeat_task PID: %d.\r\n", nrk_get_pid());

  // loop forever
  while(1) {
    // determine if the network has been joined
    nrk_sem_pend(g_network_joined_mux); {
      local_network_joined = g_network_joined;
    }
    nrk_sem_post(g_network_joined_mux);

    // if the network has been joined, decrement the counter.
    //  otherwise, wait.
    if(local_network_joined == TRUE) {
      // update watchdog timer
      nrk_sem_pend(g_net_watchdog_mux); {
        g_net_watchdog--;
        local_watchdog = g_net_watchdog;
      }
      nrk_sem_post(g_net_watchdog_mux);    

      // if the watchdog has been exceeded, set network joined flag to false
      if(local_watchdog <= 0) {
        nrk_sem_pend(g_network_joined_mux); {
          g_network_joined = FALSE;
        }
        nrk_sem_post(g_network_joined_mux);      
      }

      // clear red LED / set green LED
      nrk_led_clr(RED_LED);
      nrk_led_set(GREEN_LED);
    } else {
      // reset the watchdog timer
      nrk_sem_pend(g_net_watchdog_mux); {
        g_net_watchdog = HEART_FACTOR;
      }
      nrk_sem_post(g_net_watchdog_mux);  

      // set red LED / clear green LED
      nrk_led_set(RED_LED);
      nrk_led_clr(GREEN_LED);
    }
    nrk_wait_until_next_period();
  }
}

void SPI_Init() {
  SPI_MasterInit();
  SPI_SlaveInit(PWR_CS);
}

void nrk_set_gpio() {
  nrk_gpio_direction(ON_COIL, NRK_PIN_OUTPUT);
  nrk_gpio_direction(OFF_COIL, NRK_PIN_OUTPUT);
  nrk_gpio_direction(BTN_IN, NRK_PIN_INPUT);

  nrk_gpio_set(ON_COIL);
  nrk_gpio_set(OFF_COIL);
}

void nrk_register_drivers() {
  int8_t val;
    val = nrk_register_driver(&dev_manager_adc,ADC_DEV_MANAGER);
    if(val==NRK_ERROR)
      nrk_kprintf(PSTR("Failed to load my ADC driver\r\n"));
}

void nrk_create_taskset () {

  BUTTON_TASK.task = button_task;
  nrk_task_set_stk(&BUTTON_TASK, button_task_stack, NRK_APP_STACKSIZE);
  BUTTON_TASK.prio = 7;
  BUTTON_TASK.FirstActivation = TRUE;
  BUTTON_TASK.Type = BASIC_TASK;
  BUTTON_TASK.SchType = PREEMPTIVE;
  BUTTON_TASK.period.secs = 0;
  BUTTON_TASK.period.nano_secs = 50*NANOS_PER_MS;
  BUTTON_TASK.cpu_reserve.secs = 0;
  BUTTON_TASK.cpu_reserve.nano_secs = 10*NANOS_PER_MS;
  BUTTON_TASK.offset.secs = 0;
  BUTTON_TASK.offset.nano_secs = 0;

  RX_MSG_TASK.task = rx_msg_task;
  nrk_task_set_stk(&RX_MSG_TASK, rx_msg_task_stack, NRK_APP_STACKSIZE);
  RX_MSG_TASK.prio = 6;
  RX_MSG_TASK.FirstActivation = TRUE;
  RX_MSG_TASK.Type = BASIC_TASK;
  RX_MSG_TASK.SchType = PREEMPTIVE;
  RX_MSG_TASK.period.secs = 0;
  RX_MSG_TASK.period.nano_secs = 100*NANOS_PER_MS;
  RX_MSG_TASK.cpu_reserve.secs = 0;
  RX_MSG_TASK.cpu_reserve.nano_secs = 20*NANOS_PER_MS;
  RX_MSG_TASK.offset.secs = 0;
  RX_MSG_TASK.offset.nano_secs = 0;

  ACTUATE_TASK.task = actuate_task;
  nrk_task_set_stk(&ACTUATE_TASK, actuate_task_stack, NRK_APP_STACKSIZE);
  ACTUATE_TASK.prio = 5;
  ACTUATE_TASK.FirstActivation = TRUE;
  ACTUATE_TASK.Type = BASIC_TASK;
  ACTUATE_TASK.SchType = PREEMPTIVE;
  ACTUATE_TASK.period.secs = 0;
  ACTUATE_TASK.period.nano_secs = 200*NANOS_PER_MS;
  ACTUATE_TASK.cpu_reserve.secs = 0;
  ACTUATE_TASK.cpu_reserve.nano_secs = 20*NANOS_PER_MS;
  ACTUATE_TASK.offset.secs = 0;
  ACTUATE_TASK.offset.nano_secs = 0;

  TX_CMD_TASK.task = tx_cmd_task;
  nrk_task_set_stk(&TX_CMD_TASK, tx_cmd_task_stack, NRK_APP_STACKSIZE);
  TX_CMD_TASK.prio = 4;
  TX_CMD_TASK.FirstActivation = TRUE;
  TX_CMD_TASK.Type = BASIC_TASK;
  TX_CMD_TASK.SchType = PREEMPTIVE;
  TX_CMD_TASK.period.secs = 0;
  TX_CMD_TASK.period.nano_secs = 200*NANOS_PER_MS;
  TX_CMD_TASK.cpu_reserve.secs = 0;
  TX_CMD_TASK.cpu_reserve.nano_secs = 20*NANOS_PER_MS;
  TX_CMD_TASK.offset.secs = 0;
  TX_CMD_TASK.offset.nano_secs = 0;

  SAMPLE_TASK.task = sample_task;
  nrk_task_set_stk( &SAMPLE_TASK, sample_task_stack, NRK_APP_STACKSIZE);
  SAMPLE_TASK.prio = 3;
  SAMPLE_TASK.FirstActivation = TRUE;
  SAMPLE_TASK.Type = BASIC_TASK;
  SAMPLE_TASK.SchType = PREEMPTIVE;
  SAMPLE_TASK.period.secs = 2;
  SAMPLE_TASK.period.nano_secs = 0;
  SAMPLE_TASK.cpu_reserve.secs = 0;
  SAMPLE_TASK.cpu_reserve.nano_secs = 50*NANOS_PER_MS;
  SAMPLE_TASK.offset.secs = 0;
  SAMPLE_TASK.offset.nano_secs = 0;

  HEARTBEAT_TASK.task = heartbeat_task;
  nrk_task_set_stk(&HEARTBEAT_TASK, heartbeat_task_stack, NRK_APP_STACKSIZE);
  HEARTBEAT_TASK.prio = 2;
  HEARTBEAT_TASK.FirstActivation = TRUE;
  HEARTBEAT_TASK.Type = BASIC_TASK;
  HEARTBEAT_TASK.SchType = PREEMPTIVE;
  HEARTBEAT_TASK.period.secs = 5;
  HEARTBEAT_TASK.period.nano_secs = 0;
  HEARTBEAT_TASK.cpu_reserve.secs = 0;
  HEARTBEAT_TASK.cpu_reserve.nano_secs = 5*NANOS_PER_MS;
  HEARTBEAT_TASK.offset.secs = 0;
  HEARTBEAT_TASK.offset.nano_secs = 0;

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

  nrk_activate_task(&BUTTON_TASK);
  nrk_activate_task(&RX_MSG_TASK);
  nrk_activate_task(&ACTUATE_TASK);
  nrk_activate_task(&TX_CMD_TASK);
  nrk_activate_task(&SAMPLE_TASK);
  nrk_activate_task(&HEARTBEAT_TASK);
  nrk_activate_task(&TX_DATA_TASK);

  nrk_kprintf( PSTR("Create done\r\n") );
}

