/******************************************************************************
*  Nano-RK, a real-time operating system for sensor networks.
*  Copyright (C) 2007, Real-Time and Multimedia Lab, Carnegie Mellon University
*  All rights reserved.
*
*  This is the Open Source Version of Nano-RK included as part of a Dual
*  Licensing Model. If you are unsure which license to use please refer to:
*  http://www.nanork.org/nano-RK/wiki/Licensing
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, version 2.0 of the License.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*******************************************************************************/

/*** INCLUDE STATEMENTS ***/
#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ff_basic_sensor.h>

/*** DEFINE STATMENTS **/
#define MAC_ADDR	0x0002
#define LED_ON_VAL 1
#define LED_OFF_VAL 0
#define LIGHT_THRESHOLD 900

/*** ENUMERATIONS ***/
typedef enum 
{
  STATE_INIT,
  STATE_LED_OFF,
  STATE_POLL_SENSOR,
  STATE_LED_ON_ACK,
  STATE_LED_OFF_ACK,
} states;

typedef enum
{
  MSG_POLL,
  MSG_TURN_LED_ON,
  MSG_TURN_LED_OFF,
  MSG_IGNORE,
  MSG_NO_MESSAGE
} message_type;

/*** TASKS **/
nrk_task_type WHACKY_TASK;
NRK_STK whacky_task_stack[NRK_APP_STACKSIZE];
void whacky_task (void);

void nrk_create_taskset ();

/*** BUFFER INSTANTIATION ***/
uint8_t whacky_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t cmd[RF_MAX_PAYLOAD_SIZE];

/*** GLOBAL VARIABLES **/
uint16_t light_baseline;

void nrk_register_drivers();

int main ()
{
  uint16_t div;
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
 *
 * @returns message_type for node to act on
 */
message_type receive_message() 
{
  // local variable declarations
  uint16_t node_id, led_set;
  int8_t rssi;
  uint8_t len;
  uint8_t *local_buf;
  uint8_t pos;
  message_type return_message;

  node_id = 0;

  // Get the RX packet -> if not ready then return as such
  nrk_led_set (ORANGE_LED);
  if(!bmac_rx_pkt_ready())
  {
    return MSG_NO_MESSAGE;
  }
  local_buf = bmac_rx_pkt_get(&len, &rssi);

  // PARSE 'SET' COMMAND
  if((len>8) && (local_buf[0]=='S') && (local_buf[1]=='E') && (local_buf[2]=='T') && (local_buf[3]==':')) {
    
    // assume a space between "SET:" and the node number in the message
    pos = 5;
    while((pos<len) && (local_buf[pos] != '\0') && (local_buf[pos] >='0') && (local_buf[pos]<='9'))
    {
      node_id *= 10;
      node_id = (local_buf[pos]-'0');
      pos++;
    }

    // assume a space between the node number and the LED setting in the message
    pos++;
    led_set = (local_buf[pos]-'0');
    
    if((node_id == MAC_ADDR) && (led_set == LED_ON_VAL)) 
    {
      return_message = MSG_TURN_LED_ON;
    } 
    else if ((node_id == MAC_ADDR) && (led_set == LED_OFF_VAL)) 
    {
      return_message = MSG_TURN_LED_OFF;
    }
    else if(led_set == 66)
    {
      return_message = MSG_TURN_LED_OFF;
    }
    else
    {
      return_message = MSG_IGNORE;
    }
  }
  // PARSE 'POLL' COMMAND
  else if((len>5) && (local_buf[0]=='P') && (local_buf[1]=='O') && (local_buf[2]=='L') && (local_buf[3]=='L') && (local_buf[4]==':'))
  {
    // Assume that there is a space after POLL
    pos = 6;
    while(pos < len && local_buf[pos] != '\0' && local_buf[pos] >='0' && local_buf[pos]<='9') 
    {
	    node_id *= 10;
	    node_id += (local_buf[pos]-'0');
	    pos++;
    }
    if(node_id == MAC_ADDR)
    {
      return_message = MSG_POLL;
    }
    else 
    {
      return_message = MSG_IGNORE;
    }
  }
  // OTHERWISE -> ignore
  else
  {
    return_message = MSG_IGNORE;
    printf("msg data = ignore\r\n");
  }
  nrk_led_clr(ORANGE_LED);
  // Release the RX buffer so future packets can arrive 
  bmac_rx_pkt_release ();  
  
  return return_message;
}

/**
 * send_message - send message based on current state
 *
 * @param current - current state (STATE_LED_ON_ACK orSTATE_LED_OFF_ACK)
 */
void send_message(states current) {
  int8_t val;
  
  // determine the correct message to send
  switch(current) {
    case STATE_LED_ON_ACK: 
    {
	    sprintf (tx_buf, "ACK: %d %d", MAC_ADDR, (uint8_t)LED_ON_VAL);
	    break;
    }
    case STATE_LED_OFF_ACK:
    {
      sprintf (tx_buf, "ACK: %d %d", MAC_ADDR, (uint8_t)LED_OFF_VAL);
      break;
    }
  }
  
  // send the packet
	val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);
	if(val != NRK_OK)	
	{
	  nrk_kprintf(PSTR("Could not Transmit!\r\n"));
	}
}

void whacky_task ()
{
  // local variable instantiation 
  uint8_t fd;
  int8_t val;
  uint16_t light;
  uint16_t light_threshold = 0;
  message_type received;
  states cur_state = STATE_INIT;
   
  // Open ADC device as read 
  fd=nrk_open(FIREFLY_3_SENSOR_BASIC,READ);
  if(fd==NRK_ERROR) nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));
  
  // init bmac on channel 25 
  bmac_init (13);

  // This sets the next RX buffer.
  // This can be called at anytime before releasing the packet
  // if you wish to do a zero-copy buffer switch
  bmac_rx_pkt_set_buffer(rx_buf, RF_MAX_PAYLOAD_SIZE);
  while (1) 
  {
    switch(cur_state) 
    {
      case STATE_INIT:
      {
        // get light baseline
        light_baseline = 0;
        for (uint8_t x = 0; x < 5; x++)
        {
          val = nrk_set_status(fd,SENSOR_SELECT,LIGHT);
          val = nrk_read(fd,&light,2);
          light_baseline += light;
        }
        light_baseline = light_baseline / 5;
        light_threshold = light_baseline - 200;
        cur_state = STATE_LED_OFF;
        break;
      }
      case STATE_LED_OFF: 
      {
        //nrk_kprintf(PSTR("STATE LED OFF\r\n"));

        // state output
	      nrk_led_clr(BLUE_LED);
          
        // state actions
        received = receive_message();
        
        // next state logic
        if(received == MSG_POLL) 
        {
          cur_state = STATE_LED_OFF_ACK;
        }
        else if(received == MSG_TURN_LED_ON) 
        {
          cur_state = STATE_LED_ON_ACK;
        }
        else if(received == MSG_TURN_LED_OFF)
        {
          cur_state = STATE_LED_OFF_ACK;
        }
        else 
        {
          cur_state = STATE_LED_OFF;
        }
        break;
      }
      case STATE_LED_ON_ACK:
      {
        //nrk_kprintf(PSTR("STATE LED ON ACK\r\n"));
        // state outputs
        send_message(STATE_LED_ON_ACK);
        
	      // next state logic
	      cur_state = STATE_POLL_SENSOR;
        break;
      }
      case STATE_POLL_SENSOR: 
      {
        nrk_kprintf(PSTR("I'M THE MOLE!\r\n"));
        // state outputs
	      nrk_led_set(BLUE_LED);
	      
	      // state actions
	      val = nrk_set_status(fd,SENSOR_SELECT,LIGHT);
        val = nrk_read(fd,&light,2);
        printf("...light/threshold=%d/%d\r\n",light, light_threshold);
        received = receive_message();
        
        // next state logic
        if(received == MSG_TURN_LED_ON) 
        {
          cur_state = STATE_LED_ON_ACK;
        }
        else if(received == MSG_TURN_LED_OFF)
        {
          cur_state = STATE_LED_OFF_ACK;
        }
        else if((received == MSG_POLL) && (light > light_threshold))
        {
          cur_state = STATE_LED_ON_ACK;
        }
        else if((received == MSG_POLL) && (light < light_threshold)) 
        {
          cur_state = STATE_LED_OFF_ACK;
        }
        else if(light < light_threshold) 
        {
          cur_state = STATE_LED_OFF;
        }
        else {
          cur_state = STATE_POLL_SENSOR;
        }
        break;
      }

      case STATE_LED_OFF_ACK:
      {
        //nrk_kprintf(PSTR("STATE LED OFF ACK\r\n"));
        // state outputs
	      nrk_led_clr(BLUE_LED);
        send_message(STATE_LED_OFF_ACK);

	      // next state logic
	      cur_state = STATE_LED_OFF;
        break;
      }
      default: {
        break;
      }
    }
    nrk_wait_until_next_period();
  }
}

void nrk_create_taskset ()
{
  WHACKY_TASK.task = whacky_task;
  nrk_task_set_stk( &WHACKY_TASK, whacky_task_stack, NRK_APP_STACKSIZE);
  WHACKY_TASK.prio = 2;
  WHACKY_TASK.FirstActivation = TRUE;
  WHACKY_TASK.Type = BASIC_TASK;
  WHACKY_TASK.SchType = PREEMPTIVE;
  WHACKY_TASK.period.secs = 1;
  WHACKY_TASK.period.nano_secs = 0;
  WHACKY_TASK.cpu_reserve.secs = 0;
  WHACKY_TASK.cpu_reserve.nano_secs = 0;
  WHACKY_TASK.offset.secs = 0;
  WHACKY_TASK.offset.nano_secs = 0;
  nrk_activate_task (&WHACKY_TASK);

  printf ("Create done\r\n");
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
