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
#include <nrk_sw_wdt.h>
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
// helper functions
uint8_t inline atomic_size(packet_queue *pq, nrk_sem_t *mux);
void inline atomic_push(packet_queue *pq, packet *p, nrk_sem_t *mux);
void inline atomic_pop(packet_queue *pq, packet *p, nrk_sem_t *mux);
uint16_t inline atomic_increment_seq_num();
uint8_t inline atomic_outlet_state();
void inline atomic_update_outlet_state(uint8_t new_state);
uint8_t inline atomic_network_joined();
void inline atomic_update_network_joined(uint8_t update);
uint8_t inline atomic_button_pressed();
void inline atomic_update_button_pressed(uint8_t update);
uint8_t inline atomic_decrement_watchdog();
uint8_t inline atomic_kick_watchdog();
void inline tx_cmds(void);
void inline tx_data(void);
void inline clear_tx_buf(void);

// tasks
void rx_msg_task(void);
void tx_net_task(void);
void sample_task(void);
void button_task(void);
void actuate_task(void);
void heartbeat_task(void);

// configuration functions
void inline SPI_Init();
void inline nrk_set_gpio(void);
void inline nrk_register_drivers(void);
void inline nrk_create_taskset(void);

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
nrk_task_type TX_NET_TASK;
nrk_task_type SAMPLE_TASK;
nrk_task_type ACTUATE_TASK;
nrk_task_type BUTTON_TASK;
nrk_task_type HEARTBEAT_TASK;

// TASK STACKS
NRK_STK rx_msg_task_stack[NRK_APP_STACKSIZE];
NRK_STK tx_net_task_stack[NRK_APP_STACKSIZE*4];
NRK_STK sample_task_stack[NRK_APP_STACKSIZE*2];
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
uint8_t g_atmega_adc_fd;
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
uint8_t g_verbose;
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
  g_verbose = TRUE;
  g_network_joined = FALSE;
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
  g_net_watchdog_mux        = nrk_sem_create(1, 7);

  // sensor periods (in seconds / 2)
  g_pwr_period = 5;
  g_temp_period = 10;
  g_light_period = 15;

  // packet queues
  packet_queue_init(&g_act_queue);
  packet_queue_init(&g_cmd_tx_queue);
  packet_queue_init(&g_data_tx_queue);

  // ensure node is initially set to "OFF"
  act_packet.source_id = MAC_ADDR;
  act_packet.type = MSG_CMD;
  act_packet.seq_num = 0;
  act_packet.num_hops = 0;
  act_packet.payload[CMD_CMDID_INDEX] = (uint16_t)0;
  act_packet.payload[CMD_NODE_ID_INDEX] = MAC_ADDR;
  act_packet.payload[CMD_ACT_INDEX] = OFF;
  atomic_push(&g_act_queue, &act_packet, g_act_queue_mux);

  // initialize bmac
  bmac_task_config ();
  bmac_init (13);

  nrk_register_drivers();
  nrk_set_gpio();
  nrk_create_taskset();
  nrk_start ();

  return 0;
}

/***** HELPER FUNCTIONS *****/
uint8_t inline atomic_size(packet_queue *pq, nrk_sem_t *mux) {
  uint8_t toReturn;
  nrk_sem_pend(mux); {
    toReturn = pq->size;
  }
  nrk_sem_post(mux);
  return toReturn;
}

// atomic_push - push onto the queue atomically
void inline atomic_push(packet_queue *pq, packet *p, nrk_sem_t *mux) {
  nrk_sem_pend(mux); {
    push(pq, p);
  }
  nrk_sem_post(mux);
}

// atomic_pop - pop onto the queue atomically
void inline atomic_pop(packet_queue *pq, packet *p, nrk_sem_t *mux) {
  nrk_sem_pend(mux); {
    pop(pq, p);
  }
  nrk_sem_post(mux);
}

// atomic_increment_seq_num - increment sequence number atomically and return
uint16_t inline atomic_increment_seq_num() {
  uint16_t returnVal;
  nrk_sem_pend(g_seq_num_mux); {
    g_seq_num++;
    returnVal = g_seq_num;
  }
  nrk_sem_post(g_seq_num_mux);
  return returnVal;
}

// atomic_outlet_state - atomically return outlet state
uint8_t inline atomic_outlet_state() {
  uint8_t outlet_state = OFF;
  nrk_sem_pend(g_global_outlet_state_mux); {
    outlet_state = g_global_outlet_state;
  }
  nrk_sem_post(g_global_outlet_state_mux);
  return outlet_state;
}

// atomic_update_outlet_state - atomically update outlet state
void inline atomic_update_outlet_state(uint8_t update) {
  nrk_sem_pend(g_global_outlet_state_mux); {
    g_global_outlet_state = update;
  }
  nrk_sem_post(g_global_outlet_state_mux);
}

// atomic_network_joined - atomically return the network status
uint8_t inline atomic_network_joined() {
  uint8_t returnVal;
  nrk_sem_pend(g_network_joined_mux); { 
    returnVal = g_network_joined;
  }
  nrk_sem_post(g_network_joined_mux); 
  return returnVal; 
}

// atomic_update_network_joined
void inline atomic_update_network_joined(uint8_t update) {
  nrk_sem_pend(g_network_joined_mux); { 
    g_network_joined = update;
  }
  nrk_sem_post(g_network_joined_mux);  
}

// atomic_button_pressed - atomically return the button_pressed flag
uint8_t inline atomic_button_pressed() {
  uint8_t returnVal;
  nrk_sem_pend(g_button_pressed_mux); {
    returnVal = g_button_pressed;
  }
  nrk_sem_post(g_button_pressed_mux);
  return returnVal;  
}

// atomic_update_button_pressed - atomically update button_pressed flag
void inline atomic_update_button_pressed(uint8_t update) {
  nrk_sem_pend(g_button_pressed_mux); {
    g_button_pressed = update;
  }
  nrk_sem_post(g_button_pressed_mux);  
}

// atomic_decrement_watchdog - atomically decrement watchdog timer
uint8_t inline atomic_decrement_watchdog() {
  uint8_t returnVal;
  nrk_sem_pend(g_net_watchdog_mux); {
    g_net_watchdog--;
    returnVal = g_net_watchdog;
  }
  nrk_sem_post(g_net_watchdog_mux);  
  return returnVal;
}

// atomic_kick_watchdog - atomically kick the watchdog timer
uint8_t inline atomic_kick_watchdog() {
  nrk_sem_pend(g_net_watchdog_mux); {
    g_net_watchdog = HEART_FACTOR;
  }
  nrk_sem_post(g_net_watchdog_mux);  
  return HEART_FACTOR;  
}

// tx_cmds() - send all commands out to the network.
void inline tx_cmds() {
  // local variable instantiation
  packet tx_packet;
  uint8_t local_tx_cmd_queue_size;
  int8_t val = 0;

  // atomically get the queue size
  local_tx_cmd_queue_size = atomic_size(&g_cmd_tx_queue, g_cmd_tx_queue_mux);

  // print out task header
  if((g_verbose == TRUE) && (local_tx_cmd_queue_size > 0)) {
    nrk_kprintf(PSTR("tx_cmds...\r\n"));
  }

  // loop on queue size received above, and no more.
  for(uint8_t i = 0; i < local_tx_cmd_queue_size; i++) {
    nrk_led_set(ORANGE_LED);
    // get a packet out of the queue.
    atomic_pop(&g_cmd_tx_queue, &tx_packet, g_cmd_tx_queue_mux);
    // NOTE: a mutex is required around the network transmit buffer because
    //  tx_cmd_task() also uses it.
    nrk_sem_pend(g_net_tx_buf_mux); {
      g_net_tx_index = assemble_packet((uint8_t *)&g_net_tx_buf, &tx_packet);
      if (g_verbose == TRUE) {
        nrk_kprintf(PSTR("TX: "));
        print_packet(&tx_packet);
      }
      // send the packet
      val = bmac_tx_pkt(g_net_tx_buf, g_net_tx_index);
      if(val==NRK_OK){ 
      }
      else {
        nrk_kprintf( PSTR( "NO ack or Reserve Violated!\r\n" ));
      }

      clear_tx_buf();
    }
    nrk_sem_post(g_net_tx_buf_mux);
    nrk_led_clr(ORANGE_LED);
  }
  return;
}

// tx_data_task() - send standard messages out to the network (i.e. handshake messages, etc.)
void inline tx_data() {
  // local variable initialization
  packet tx_packet;
  uint8_t local_tx_data_queue_size;
  int8_t val = 0;
  uint8_t sent_heart = FALSE;
  uint8_t to_send;

  // atomically get the queue size
  local_tx_data_queue_size = atomic_size(&g_data_tx_queue, g_data_tx_queue_mux);

  // print out task header
  if((g_verbose == TRUE) && (local_tx_data_queue_size > 0)){
    nrk_kprintf(PSTR("tx_data...\r\n"));
  }

  // loop on queue size received above, and no more.
  for(uint8_t i = 0; i < local_tx_data_queue_size; i++) {
    nrk_led_set(ORANGE_LED);
    // get a packet out of the queue.
    atomic_pop(&g_data_tx_queue, &tx_packet, g_data_tx_queue_mux);

    // ONLY send one heartbeat per iteration.
    if((tx_packet.type == MSG_HEARTBEAT) && (sent_heart == TRUE)) {
      to_send = FALSE;
    } else {
      to_send = TRUE;
    }

    if (to_send == TRUE) {
      // assemble tx buf and send message
      nrk_sem_pend(g_net_tx_buf_mux); {
        g_net_tx_index = assemble_packet((uint8_t *)&g_net_tx_buf, &tx_packet);

        // print out packet
        if (g_verbose == TRUE) {
          nrk_kprintf(PSTR("TX: "));
          print_packet(&tx_packet);
        }

        // send the packet
        val = bmac_tx_pkt(g_net_tx_buf, g_net_tx_index);
        if(val==NRK_OK){ 
        }
        else {
          nrk_kprintf( PSTR( "NO ack or Reserve Violated!\r\n" ));
        }

        // set sent_heart flag
        if(tx_packet.type == MSG_HEARTBEAT) {
          sent_heart = TRUE;
        }

        // clear the buffer
        clear_tx_buf();
      }
      nrk_sem_post(g_net_tx_buf_mux);

      nrk_led_clr(ORANGE_LED);
    }
  }

  return;
}

void inline clear_tx_buf(){
  for(uint8_t i = 0; i < g_net_tx_index; i++) {
    g_net_tx_buf[i] = 0;
  }
  g_net_tx_index = 0;
}

// rx_msg_task() - receive messages from the network
void rx_msg_task() {
  // local variable instantiation
  packet rx_packet;
  uint8_t len;
  int8_t rssi;
  int8_t in_seq_pool;
  uint16_t local_seq_num;
  uint8_t new_node = NONE;
  uint8_t local_network_joined = FALSE;

  printf("rx_msg PID: %d.\r\n", nrk_get_pid());

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

      // get the packet, parse and release !!!!!!!!!!!!!!!!!!
      bmac_rx_pkt_get(&len, &rssi);

      parse_msg(&rx_packet, (uint8_t *)&g_net_rx_buf, len);

      bmac_rx_pkt_release ();

      // print incoming packet if appropriate
      if(g_verbose == TRUE) {
        nrk_kprintf(PSTR("RX: "));
        print_packet(&rx_packet);
      }

      // only receive the message if it's not from this node
      if(rx_packet.source_id != MAC_ADDR) {
        // determine if the network has been joined
        local_network_joined = atomic_network_joined();

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
          if((rx_packet.seq_num > local_seq_num) || (new_node == NODE_FOUND) || (rx_packet.type = MSG_HAND)) {

            // update the sequence pool and reset the new_node flag
            update_pool(&g_seq_pool, rx_packet.source_id, rx_packet.seq_num);
            new_node = NONE;

            // put the message in the right queue based on the type
            switch(rx_packet.type) {
              // no message
              case MSG_NO_MESSAGE: {
                // do nothing.
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
                atomic_push(&g_data_tx_queue, &rx_packet, g_data_tx_queue_mux);
                break;
              }
              // command received -> forward or actuate
              case MSG_CMD: {
                // if command is for this node and hasn't been received yet, add it
                //  to the action queue. Otherwise, add it to the cmd_tx queue for
                //  forwarding to other nodes.
                if(rx_packet.payload[CMD_NODE_ID_INDEX] == MAC_ADDR) {
                  g_last_command = (uint16_t)rx_packet.payload[CMD_CMDID_INDEX]; // need to cast again here right?
                  atomic_push(&g_act_queue, &rx_packet, g_act_queue_mux);
                  if (g_verbose == TRUE) {
                    nrk_kprintf(PSTR("Received command ^^^\r\n"));
                  }
                }
                else {
                  rx_packet.num_hops++;
                  atomic_push(&g_cmd_tx_queue, &rx_packet, g_cmd_tx_queue_mux);
                }
                break;
              }
              // command ack received -> forward to the server
              case MSG_CMDACK: {
                rx_packet.num_hops++;
                atomic_push(&g_cmd_tx_queue, &rx_packet, g_cmd_tx_queue_mux);
                break;
              }
              // handshake message -> forward to the server
              // NOTE: will only receive type MSG_HAND to forward
              case MSG_HAND: {
                rx_packet.num_hops++;
                atomic_push(&g_data_tx_queue, &rx_packet, g_data_tx_queue_mux);
                break;
              }
              // handshake ack message -> forward to the server
              case MSG_HANDACK: {
                rx_packet.num_hops++;
                atomic_push(&g_data_tx_queue, &rx_packet, g_data_tx_queue_mux);
                break;
              }
              // heartbeat message -> forward to the server and
              //  kick the watchdog counter
              case MSG_HEARTBEAT: {
                rx_packet.num_hops++;
                atomic_push(&g_data_tx_queue, &rx_packet, g_data_tx_queue_mux);
                atomic_kick_watchdog();
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
          // clear the pool if we're not in the network
          clear_pool(&g_seq_pool);

          // if a handshake ack has been received, then set the network joined flag. Otherwise, ignore.
          if((rx_packet.type == MSG_HANDACK) && (rx_packet.payload[HANDACK_NODE_ID_INDEX] == MAC_ADDR)) {
            atomic_update_network_joined(TRUE);
            local_network_joined = atomic_network_joined();
          }
        }
      }
      nrk_led_clr(BLUE_LED);
    }
    nrk_wait_until_next_period();
  }
}

// net_tx_task - send network messages
void tx_net_task() {
  uint8_t counter = 0;
  uint8_t tx_cmd_flag;
  uint8_t tx_data_flag;

  printf("tx_net PID: %d.\r\n", nrk_get_pid());

  // setup soft watchdog variables
  // nrk_time_t soft_watchdog_period;
  // int8_t v;
  // soft_watchdog_period.secs = 1;
  // soft_watchdog_period.nano_secs = 0;
  // v = nrk_sw_wdt_init(0, &soft_watchdog_period, NULL);
  // nrk_sw_wdt_start(0);

  // Wait until bmac has started. This should be called by all tasks
  //  using bmac that do not call bmac_init().
  while(!bmac_started ()) {
    nrk_wait_until_next_period ();
  }

  // loop forever
  while(1) {
    //nrk_sw_wdt_update(0);
    // incrment counter and set flags
    counter++;
    tx_cmd_flag = counter % TX_CMD_FLAG;
    tx_data_flag = counter % TX_DATA_FLAG;

    // if commands should be transmitted, then call the tx_cmds() helper
    if (tx_cmd_flag == TRANSMIT) {
      tx_cmds();
    }

    // if data shoudl be transmitted, then call the tx_data() helper
    if (tx_data_flag == TRANSMIT) {
      tx_data();
      counter %= TX_DATA_FLAG;
    }
    nrk_wait_until_next_period();
  }
}

// sample_task - sample sensors
void sample_task() {
  // local variable instantiation
  uint8_t pwr_period_count = 0;
  uint8_t temp_period_count = 0;
  uint8_t light_period_count = 0;
  uint8_t sensor_sampled = FALSE;
  uint16_t local_pwr_val = 0;
  uint16_t local_temp_val = 0;
  uint16_t local_light_val = 0;
  packet tx_packet, hello_packet;
  uint8_t local_network_joined = FALSE;
  int8_t val;
  uint16_t adc_buf[2];
  uint8_t pwr_rcvd[3];
  uint8_t hw_rev;

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

  // get the hardware rev of this node
  hw_rev = GET_REV(HARDWARE_REV);

  // Open the ATMEGA ADC device as read
  g_atmega_adc_fd = nrk_open(ADC_DEV_MANAGER,READ);
  if(g_atmega_adc_fd == NRK_ERROR) {
    nrk_kprintf(PSTR("Failed to open ADC driver\r\n"));
  }   

  // loop forever
  while (1) {
    // check if the network has been joined
    local_network_joined = atomic_network_joined();


    // if the network has been joined then start sampling sensors
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
      if((pwr_period_count == SAMPLE_SENSOR) && (hw_rev == HW_REV0)) {
        // requrest temperature
        pwr_read(WATT, (uint8_t *)&pwr_rcvd);

        // pull out dinner location
        local_pwr_val = (pwr_rcvd[0] << 8) | pwr_rcvd[1];
        g_sensor_pkt.pwr_val = transform_pwr(local_pwr_val);
        sensor_sampled = TRUE;
      }

      // sample temperature sensor if appropriate
      if(temp_period_count == SAMPLE_SENSOR) {
        // sample analog temperature sensor via Atmega ADC
        if(hw_rev == HW_REV0) {
          val = nrk_set_status(g_atmega_adc_fd, ADC_CHAN, CHAN_6);
        } else if(hw_rev == HW_REV1) {
          val = nrk_set_status(g_atmega_adc_fd, ADC_CHAN, CHAN_4);
        }
        if(val == NRK_ERROR) {
          nrk_kprintf(PSTR("Failed to set ADC status\r\n"));
        } else {
          val = nrk_read(g_atmega_adc_fd, (uint8_t *)&adc_buf[0],2);
          if(val == NRK_ERROR)  {
            nrk_kprintf(PSTR("Failed to read ADC\r\n"));
          } else {
            local_temp_val = (uint16_t)adc_buf[0];
            g_sensor_pkt.temp_val = transform_temp(local_temp_val);
            sensor_sampled = TRUE;
          }
        }          
      }

      // sample light sensor if appropriate
      if((light_period_count == SAMPLE_SENSOR) && (hw_rev == HW_REV1)) {
        val = nrk_set_status(g_atmega_adc_fd, ADC_CHAN, CHAN_2);
        if(val == NRK_ERROR) {
          nrk_kprintf(PSTR("Failed to set ADC status\r\n"));
        } else {
          val = nrk_read(g_atmega_adc_fd, (uint8_t *)&adc_buf[0],2);
          if(val == NRK_ERROR)  {
            nrk_kprintf(PSTR("Failed to read ADC\r\n"));
          } else {
            local_light_val = (uint16_t)adc_buf[0];
            g_sensor_pkt.light_val = local_light_val;
            sensor_sampled = TRUE;
          }
        }
      }

      // if a sensor has been sampled, send a packet out
      if(sensor_sampled == TRUE) {
        // update sequence number
        tx_packet.seq_num = atomic_increment_seq_num();

        // add data values to sensor packet
        tx_packet.payload[DATA_PWR_INDEX] = (uint8_t)((g_sensor_pkt.pwr_val >> 8) & 0xFF);
        tx_packet.payload[DATA_PWR_INDEX + 1] = (uint8_t)(g_sensor_pkt.pwr_val& 0xFF);
        tx_packet.payload[DATA_TEMP_INDEX] = (uint8_t)((g_sensor_pkt.temp_val >> 8) & 0xFF);
        tx_packet.payload[DATA_TEMP_INDEX + 1] = (uint8_t)(g_sensor_pkt.temp_val & 0xFF);
        tx_packet.payload[DATA_LIGHT_INDEX] = (uint8_t)((g_sensor_pkt.light_val >> 8) & 0xFF);
        tx_packet.payload[DATA_LIGHT_INDEX + 1] = (uint8_t)(g_sensor_pkt.light_val & 0xFF);
        tx_packet.payload[DATA_STATE_INDEX] = atomic_outlet_state();

        // print the sensor info
        if(g_verbose == TRUE) {
          printf("P: %d, T: %d, L: %d\r\n", g_sensor_pkt.pwr_val, g_sensor_pkt.temp_val, g_sensor_pkt.light_val);
        }

        // add packet to data queue
        atomic_push(&g_data_tx_queue, &tx_packet, g_data_tx_queue_mux);
      }
    }
    // if the local_network_joined flag hasn't been set yet, send a hello packet
    else {
      // NETWORK JOINED DEBUG COMMAND
      //atomic_update_network_joined(TRUE);

      // update seq num
      hello_packet.seq_num = atomic_increment_seq_num();

      // push to queue
      atomic_push(&g_cmd_tx_queue, &hello_packet, g_cmd_tx_queue_mux);
    }
    nrk_wait_until_next_period();
  }
}

// button_task - check the state of the physical button
void button_task() {
  button_pressed_state curr_state = STATE_SNIFF;
  uint8_t local_button_pressed = FALSE;

  printf("button_task PID: %d.\r\n", nrk_get_pid());

  // loop forever
  while(1) {
    switch(curr_state) {
      // STATE_SNIFF:
      //    - if the button gets pressed set the flag and switch states
      //    - otherwise, keep sniffing
      case STATE_SNIFF: {
        // get current button_pressed state
        local_button_pressed = atomic_button_pressed();

        // check button input
        if((nrk_gpio_get(BTN_IN) == BUTTON_PRESSED) && (local_button_pressed == FALSE)) {
          // set flag
          atomic_update_button_pressed(TRUE);

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

// actuate_task() - actuate any commands that have been received for this node.
void actuate_task() {
  // local variable instantiation
  uint8_t act_queue_size;
  packet act_packet, tx_packet;
  int8_t action;
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
    // determine if the network has been joined
    local_network_joined = atomic_network_joined();

    // get action queue size / reset action flag
    act_queue_size = atomic_size(&g_act_queue, g_act_queue_mux);
    action = ACT_NONE;

    // get button pressed
    local_button_pressed = atomic_button_pressed();

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
          atomic_pop(&g_act_queue, &act_packet, g_act_queue_mux);
          action = act_packet.payload[CMD_ACT_INDEX];
          if(g_verbose == TRUE) {
            printf("ACT: %d\r\n", action);
          }
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
          atomic_pop(&g_act_queue, &act_packet, g_act_queue_mux);
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
          tx_packet.seq_num = atomic_increment_seq_num();

          // set payload
          tx_packet.payload[CMDACK_CMDID_INDEX] = (uint16_t)act_packet.payload[CMD_CMDID_INDEX];
          tx_packet.payload[CMDACK_STATE_INDEX] = OFF;

          // place message in the queue
          atomic_push(&g_cmd_tx_queue, &tx_packet, g_cmd_tx_queue_mux);
        }

        // update global outlet state
        atomic_update_outlet_state(OFF);

        // this will flag if the command just executed was from a physical button press
        //  if so, reset the global flag
        if(local_button_pressed == TRUE) {
          atomic_update_button_pressed(FALSE);
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
          tx_packet.seq_num = atomic_increment_seq_num();

          // set payload
          tx_packet.payload[CMDACK_CMDID_INDEX] = (uint16_t)act_packet.payload[CMD_CMDID_INDEX];
          tx_packet.payload[CMDACK_STATE_INDEX] = ON;

          // place message in the queue
          atomic_push(&g_cmd_tx_queue, &tx_packet, g_cmd_tx_queue_mux);
        }

        // update global outlet state
        atomic_update_outlet_state(ON);

        // this will flag if the command just executed was from a physical button press
        //  if so, reset the global flag
        if(local_button_pressed == TRUE) {
          atomic_update_button_pressed(FALSE);
        }

        // next state -> STATE_ON
        curr_state = STATE_ON;
        break;
      }
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
    local_network_joined = atomic_network_joined();

    // if the network has been joined, decrement the counter.
    //  otherwise, wait.
    if(local_network_joined == TRUE) {
      // update watchdog timer
      local_watchdog = atomic_decrement_watchdog();

      // if the watchdog has been exceeded, set network joined flag to false
      if(local_watchdog <= 0) {
        atomic_update_network_joined(FALSE);     
      }

      // clear red LED / set green LED
      nrk_led_clr(RED_LED);
      nrk_led_set(GREEN_LED);
    } else {
      // reset the watchdog timer
      local_watchdog = atomic_kick_watchdog();

      // set red LED / clear green LED
      nrk_led_set(RED_LED);
      nrk_led_clr(GREEN_LED);
    }
    nrk_wait_until_next_period();
  }
}

/**** CONFIGURATION ****/
void inline SPI_Init() {
  uint8_t hw_rev;

  SPI_MasterInit();

  // get the hardware rev of this node
  hw_rev = GET_REV(HARDWARE_REV);

  // Open the ATMEGA ADC device as read
  if(hw_rev == HW_REV0) {
    SPI_SlaveInit(PWR_CS);
  }
}

void inline nrk_set_gpio() {
  nrk_gpio_direction(ON_COIL, NRK_PIN_OUTPUT);
  nrk_gpio_direction(OFF_COIL, NRK_PIN_OUTPUT);
  nrk_gpio_direction(BTN_IN, NRK_PIN_INPUT);

  nrk_gpio_set(ON_COIL);
  nrk_gpio_set(OFF_COIL);
}

void inline nrk_register_drivers() {
  int8_t val;
    val = nrk_register_driver(&dev_manager_adc,ADC_DEV_MANAGER);
    if(val==NRK_ERROR)
      nrk_kprintf(PSTR("Failed to load my ADC driver\r\n"));
}

void inline nrk_create_taskset () {

  BUTTON_TASK.task = button_task;
  nrk_task_set_stk(&BUTTON_TASK, button_task_stack, NRK_APP_STACKSIZE);
  BUTTON_TASK.prio = 7;
  BUTTON_TASK.FirstActivation = TRUE;
  BUTTON_TASK.Type = BASIC_TASK;
  BUTTON_TASK.SchType = NONPREEMPTIVE;
  BUTTON_TASK.period.secs = 0;
  BUTTON_TASK.period.nano_secs = 100*NANOS_PER_MS;
  BUTTON_TASK.cpu_reserve.secs = 0;
  BUTTON_TASK.cpu_reserve.nano_secs = 50*NANOS_PER_MS;
  BUTTON_TASK.offset.secs = 0;
  BUTTON_TASK.offset.nano_secs = 0;

  RX_MSG_TASK.task = rx_msg_task;
  nrk_task_set_stk(&RX_MSG_TASK, rx_msg_task_stack, NRK_APP_STACKSIZE);
  RX_MSG_TASK.prio = 6; 
  RX_MSG_TASK.FirstActivation = TRUE;
  RX_MSG_TASK.Type = BASIC_TASK;
  RX_MSG_TASK.SchType = NONPREEMPTIVE;
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
  ACTUATE_TASK.SchType = NONPREEMPTIVE;
  ACTUATE_TASK.period.secs = 0;
  ACTUATE_TASK.period.nano_secs = 500*NANOS_PER_MS;
  ACTUATE_TASK.cpu_reserve.secs = 0;
  ACTUATE_TASK.cpu_reserve.nano_secs = 20*NANOS_PER_MS;
  ACTUATE_TASK.offset.secs = 0;
  ACTUATE_TASK.offset.nano_secs = 0;

  TX_NET_TASK.task = tx_net_task;
  nrk_task_set_stk(&TX_NET_TASK, tx_net_task_stack, NRK_APP_STACKSIZE*4);
  TX_NET_TASK.prio = 4;
  TX_NET_TASK.FirstActivation = TRUE;
  TX_NET_TASK.Type = BASIC_TASK;
  TX_NET_TASK.SchType = NONPREEMPTIVE;
  TX_NET_TASK.period.secs = 0;
  TX_NET_TASK.period.nano_secs = 500*NANOS_PER_MS;
  TX_NET_TASK.cpu_reserve.secs = 0;
  //TX_NET_TASK.cpu_reserve.nano_secs = 40*NANOS_PER_MS;
  TX_NET_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TX_NET_TASK.offset.secs = 0;
  TX_NET_TASK.offset.nano_secs = 0;

  SAMPLE_TASK.task = sample_task;
  nrk_task_set_stk( &SAMPLE_TASK, sample_task_stack, NRK_APP_STACKSIZE*2);
  SAMPLE_TASK.prio = 3;
  SAMPLE_TASK.FirstActivation = TRUE;
  SAMPLE_TASK.Type = BASIC_TASK;
  SAMPLE_TASK.SchType = NONPREEMPTIVE;
  SAMPLE_TASK.period.secs = 2;
  SAMPLE_TASK.period.nano_secs = 0;
  SAMPLE_TASK.cpu_reserve.secs = 0;
  SAMPLE_TASK.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  SAMPLE_TASK.offset.secs = 0;
  SAMPLE_TASK.offset.nano_secs = 0;

  HEARTBEAT_TASK.task = heartbeat_task;
  nrk_task_set_stk(&HEARTBEAT_TASK, heartbeat_task_stack, NRK_APP_STACKSIZE);
  HEARTBEAT_TASK.prio = 2;
  HEARTBEAT_TASK.FirstActivation = TRUE;
  HEARTBEAT_TASK.Type = BASIC_TASK;
  HEARTBEAT_TASK.SchType = NONPREEMPTIVE;
  HEARTBEAT_TASK.period.secs = 5;
  HEARTBEAT_TASK.period.nano_secs = 0;
  HEARTBEAT_TASK.cpu_reserve.secs = 0;
  HEARTBEAT_TASK.cpu_reserve.nano_secs = 5*NANOS_PER_MS;
  HEARTBEAT_TASK.offset.secs = 0;
  HEARTBEAT_TASK.offset.nano_secs = 0;

  nrk_activate_task(&BUTTON_TASK);
  nrk_activate_task(&RX_MSG_TASK);
  nrk_activate_task(&ACTUATE_TASK);
  nrk_activate_task(&TX_NET_TASK);
  nrk_activate_task(&SAMPLE_TASK);
  nrk_activate_task(&HEARTBEAT_TASK);

  nrk_kprintf( PSTR("Create done\r\n") );
}

