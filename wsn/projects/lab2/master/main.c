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
#include <stdlib.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>

/*** DEFINE STATEMENTS ***/
#define NUM_SLAVES  3
#define MAC_ADDR  (NUM_SLAVES+1)
#define MAX_NUM_ROUNDS 10
#define INIT_ROUND_DURATION 11111 // 11,111ms = 11.11s
#define INIT_TIME_TO_WAIT 1000
#define NODE_WAIT_TIME 5000 // wait 3sec for ack when sending kill msg
#define LED_ON 1
#define LED_OFF 0
#define MSG_RECEIVED 1
#define MSG_NOT_RECEIVED -1
#define MSG_SENT 1
#define MSG_NOT_SENT 0
#define TIMEOUT_OCCURED 1 
#define NO_TIMEOUT_OCCURED 0
#define PENALTY 50
#define SCORES_UPDATED 1
#define SCORES_NOT_UPDATED 0


/*** ENMERATIONS ***/
typedef enum
{
  INIT,
  FIND_NEW,
  PICK_NEW,
  POLL,
  UPDATE_SCORE,
  KILL_NODE,
  GAME_OVER
} states;

/*** GLOBAL VARIABLE INSTANTIATION ***/
// GAMEPLAY
// round counter
uint8_t round_counter;

// max time to wait for the round (in seconds)
uint16_t time_to_wait = INIT_TIME_TO_WAIT;
uint32_t round_duration = INIT_ROUND_DURATION;

// score global
int16_t total_score = 0;
int16_t high_scores[3] = {0};

// current slave id global
uint8_t slave_id;

// TASKS
nrk_task_type WHACKY_TASK;
NRK_STK whacky_task_stack[NRK_APP_STACKSIZE];
void whacky_task (void);
void nrk_create_taskset ();


// BUFFERS
uint8_t whacky_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
char tx_buf[RF_MAX_PAYLOAD_SIZE];

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

  nrk_create_taskset ();
  nrk_start ();

  return 0;
}
/********************** send_message() *********************************
 * Function: Send value that is stored in tx_buf
 *
 * tx_buf format:
 * "POLL: [slave_id] [slave_state]"
 * slave_id - is target device
 * slave_state - command the slave to turn off/on LED
 *
 * Return:
 * 0 - Message was NOT sent
 * 1 - Message was sent
 **********************************************************************/
uint8_t send_message()
{
  int8_t val;

  nrk_led_set (BLUE_LED);
  val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);
  if(val != NRK_OK)
  {
    //nrk_kprintf(PSTR("Could not Transmit!\r\n"));
    return MSG_NOT_SENT; // unsuccessful
  }
  else
  {
    // Task gets control again after TX complete
    //printf("  -> Sent Msg: %s\r\n", tx_buf);
    nrk_led_clr (BLUE_LED);
    return MSG_SENT; // successful
  }
}

// global variable definition for time difference.
// defined it here for convenience.
uint16_t delta_time;

/********************** receive_message() *************************************
 * Function:
 * Waits to receive a packet from 'mole' within the time limit (time_to_wait).
 * If packet is received...
 * -->the global variable (delta_time) is set to the time difference
 *    between start and received.
 *
 * Return:
 * 0 - No message received within time limit
 * 1 - Message was received.
 *
 * ****************************************************************************/
int8_t receive_message() 
{
  int8_t timeout_flag, message_received_flag;
  char *local_buf;
  uint8_t len;
  int8_t rssi;
  uint8_t received_light_value = -1;
  uint8_t received_node_id = -1;
  //char *header_buf;

  //Timer management
  nrk_time_t start_time, end_time;

  // Wait until an RX packet is received
  timeout_flag = NO_TIMEOUT_OCCURED;
  message_received_flag = MSG_NOT_RECEIVED;

  nrk_time_get(&start_time); // retrieve start_time
  while((message_received_flag == MSG_NOT_RECEIVED) && (timeout_flag == NO_TIMEOUT_OCCURED))
  {
    if(bmac_rx_pkt_ready()) // if a packet is ready to be received
    {
      //response stored in local_buf
      local_buf = bmac_rx_pkt_get(&len, &rssi);
      //printf("  -> RECEIVED PACKET: %s\r\n", local_buf);
      
      // Parse led value from buffer
      //sscanf(local_buf, "%s %d %d", header_buf, &received_node_id, &received_light_value);
      if (local_buf[0] == 'A' && local_buf[1] == 'C' && local_buf[2] == 'K' && local_buf[3] == ':')
      {
        //pos = 5; // assume a space
        received_node_id = local_buf[5] - '0'; // assume a space;
        received_light_value = local_buf[7] - '0'; // assume a space
        //printf("  -> Value of Node %d is %d\r\n", received_node_id, received_light_value);
        message_received_flag = MSG_RECEIVED;
      }
       
      // Release the RX buffer so future packets can arrive 
      bmac_rx_pkt_release ();
      
      // calculate and update delta_time
      if(end_time.nano_secs > start_time.nano_secs)
      {
        delta_time = ((end_time.secs-start_time.secs)*1000+(end_time.nano_secs-start_time.nano_secs)/1000000);
      }
      else
      {
        delta_time = ((end_time.secs-start_time.secs)*1000-(start_time.nano_secs-end_time.nano_secs)/1000000);
      }

      if (received_node_id == slave_id)
      {
        return received_light_value;
      }
      else
      {
        return MSG_NOT_RECEIVED;
      }
    }
    
    // Get end time to check time difference
    nrk_time_get(&end_time);
    
    
    if(end_time.nano_secs > start_time.nano_secs)
    {
      // check to see if its been over 'time_to_wait' seconds. 
      // NOTE: Originally the value was set to 1000
      if(((end_time.secs-start_time.secs)*1000+(end_time.nano_secs-start_time.nano_secs)/1000000) > 1000)
      {
        timeout_flag = TIMEOUT_OCCURED;
      }
    }
    else
    {
      // check to see if its been over 'time_to_wait' seconds. 
      // NOTE: Originally the value was set to 1000
      if(((end_time.secs-start_time.secs)*1000-(start_time.nano_secs-end_time.nano_secs)/1000000) >  1000)
      {
        timeout_flag = TIMEOUT_OCCURED;
      }
    }
  }
  
  // its been over 'time_to_wait' seconds AND node has not received packet...
  if(timeout_flag == TIMEOUT_OCCURED)
  {
    //nrk_kprintf(PSTR("  -> Rx Timed Out!\r\n"));
    return MSG_NOT_RECEIVED; // return 0 indicating that we timed out / didn't receive a packet
  }
  nrk_kprintf(PSTR("  -> LOGICAL ERROR IN RECEIVE MESSAGE!\r\n"));
  return MSG_RECEIVED;
}

/**
 * pick_new_mole - pick new mole randomly based on the current mole
 *
 * @param current - current mole
 * @returns randomly selected mole
 */
uint8_t pick_new_mole(uint8_t current) {
  nrk_time_t random_time;
  nrk_time_get(&random_time);

  uint8_t step = (uint8_t) (random_time.secs % (NUM_SLAVES - 1));
  return (current + step + 1) % NUM_SLAVES;
}

/**
 * get_time_duration_ms - compare inputs and determine the number of 
 *  time between them.
 *
 * @param start - starting time
 * @param end - ending time
 * @returns uint16 containing the number of ms between start and end
 */
uint16_t get_time_duration_ms(nrk_time_t start, nrk_time_t end) {
  uint16_t nanosec_diff_ms = 0;
  if (end.nano_secs >= start.nano_secs) {
    nanosec_diff_ms = (end.nano_secs-start.nano_secs)/1000000;
  } else {
    nanosec_diff_ms = (start.nano_secs-end.nano_secs)/1000000;
  }
  return (uint16_t) ((end.secs-start.secs)*1000) + nanosec_diff_ms;
}

/**
 * update_high_scores - Update high scores with new score
 *
 * @param new_score - new score to be considered
 * @return SCORES_UPDATED if high scores were updated, 
 *    SCORES_NOT_UPDATED otherwise.
 */
uint8_t update_high_scores(uint16_t new_score)
{
  if (new_score > high_scores[0])
  {
    high_scores[2] = high_scores[1];
    high_scores[1] = high_scores[0];
    high_scores[0] = new_score;  
    return SCORES_UPDATED;
  }
  else if (new_score > high_scores[1])
  {
    high_scores[2] = high_scores[1];
    high_scores[1] = new_score;
    return SCORES_UPDATED;
  }
  else if (new_score > high_scores[2])
  {
    high_scores[2] = new_score;
    return SCORES_UPDATED;
  }
  return SCORES_NOT_UPDATED;
}

void whacky_task ()
{
  uint16_t round_timer;
  uint16_t wait_node_timer;
  nrk_sig_t uart_rx_signal;
  nrk_time_t wait_node_start_time, round_start_time, current_time;

  // Get the signal for UART RX
  uart_rx_signal=nrk_uart_rx_signal_get();

  // Register task to wait on signal
  nrk_signal_register(uart_rx_signal);

  // init bmac on channel 13
  // Team Wireless Wizards!
  bmac_init (13);

  // This sets the next RX buffer.
  // This can be called at anytime before releasing the packet
  // if you wish to do a zero-copy buffer switch
  bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);

  // initialize the state
  slave_id = 0;
  states cur_state = INIT;

  while (1)
  {
    switch(cur_state)
    {
      case INIT:
      {
        char option;
        //nrk_kprintf(PSTR("STATE INIT\r\n"));
        // update tx_buf with led value of 'r'
        // all nodes which receive this message should reset.
        sprintf (tx_buf, "SET: %d %c", 0, 'r');

        for(uint8_t i = 0; i < 5; i++) {
          send_message();
          // pause for 10ms (10k us) between reset messages
          nrk_spin_wait_us(10000);
        }

        nrk_kprintf(PSTR("Press 's' to start\r\n" ));
        // press 's' to start
        do{
          if(nrk_uart_data_ready(NRK_DEFAULT_UART))
          {
            option=getchar();
          }
          else
          {
            nrk_event_wait(SIG(uart_rx_signal));
          }
        } while(option!='s');

        // state outputs
        // Start new game, reset score and round duration
        round_counter = 0;
        total_score = 0;
        round_duration = INIT_ROUND_DURATION;
        // next state logic
        cur_state = PICK_NEW;
        break;
      }

      case PICK_NEW:
      {        
        // clear the queue so no extraneous data is discovered late.
        while(bmac_rx_pkt_ready()) {
          receive_message();
        }

        // pick new slave randomly
        slave_id = pick_new_mole(slave_id);
        //printf("  -> Picked new node id %d\r\n", slave_id);
        
        // start wait node timer
        nrk_time_get(&wait_node_start_time);
        cur_state = FIND_NEW;
      }

      case FIND_NEW:
      {
        // update tx_buf with new slave_id and state
        sprintf (tx_buf, "SET: %d %d", slave_id, (uint8_t)LED_ON);

        // send the message to slave
        if(!send_message()) {
          nrk_kprintf(PSTR("Could not send message...\r\n" ));
          cur_state = PICK_NEW;
        }
        else
        {
          // get acknowledgement from slave
          int8_t led_val = receive_message();
          if(led_val == LED_ON)
          {
            /**
             * START THE ROUND:
             *  - start the round timer
             *  - increment the round counter
             *  - set the round time
             */
            nrk_time_get(&round_start_time);
            round_counter += 1;
            printf("\r\nSTARTING ROUND #%d (%d)\r\n", round_counter, slave_id);

            round_duration = (round_duration*9)/10;
            //round_duration = round_duration - 1000;
            printf("  -> Round duration: %dms\r\n", round_duration);
            cur_state = POLL; // update current_state to POLL
          }
          else
          {
            nrk_time_get(&current_time);
            wait_node_timer = get_time_duration_ms(wait_node_start_time, current_time);

            if (wait_node_timer > NODE_WAIT_TIME)
            {
              cur_state = KILL_NODE;
            } 
            else
            {
              cur_state = FIND_NEW;
            }
          }
        }
        break;
      }

      case POLL:
      {
        // state output(s)
        sprintf (tx_buf, "POLL: %d", slave_id);
        send_message();

        // state action(s)
        uint8_t led_val = receive_message();

        nrk_time_get(&current_time);
        round_timer = get_time_duration_ms(round_start_time, current_time);

        // next state logic
        if(led_val == LED_OFF)
        {
          //nrk_kprintf(PSTR("received LED OFF msg\r\n"));
          cur_state = UPDATE_SCORE;
        }
        else if((round_timer > round_duration))
        {
          if((round_timer > round_duration)) {
            nrk_kprintf(PSTR("!!ROUND TIMER EXPIRED!!\r\n"));
            // printf("Round timer expired: round timer=%u, round time=%u\r\n", round_timer, round_duration);
            total_score -= PENALTY;
            printf("  -> Penalty: %d points, new score=%d\r\n", PENALTY, total_score);
          }
          cur_state = KILL_NODE;
          // start kill node timer
          nrk_time_get(&wait_node_start_time);
        }
        else
        {
          cur_state = POLL;
        }
        break;
      }

      case UPDATE_SCORE:
      {
        // state actions
        uint16_t points = ((round_duration - round_timer) / 100)*((round_counter / 3) + 1);
        total_score += points;
        
        nrk_kprintf(PSTR("!!ROUND COMPLETED!!\r\n"));
        printf("  -> You got %d points\r\n", points);
        printf("  -> Current score: %d\r\n", total_score);

        // next state logic
        if(round_counter >= MAX_NUM_ROUNDS)
        {
          cur_state = GAME_OVER;
        }
        else
        {
          cur_state = KILL_NODE;
          // start kill node timer
          nrk_time_get(&wait_node_start_time);
        }
        break;
      }

      case KILL_NODE:
      {
        // update tx_buf with new slave_id and state
        sprintf (tx_buf, "SET: %d %d", slave_id, (uint8_t)LED_OFF);

        if(!send_message()) // if send_message was unsuccessful
        {
          // print state and stay in current state until message is sent
          nrk_kprintf( PSTR("Send Fail\r\n" ));
        }
        else // message was sent!
        {
          uint8_t led_val = receive_message();
          if(led_val == LED_OFF)
          {
            if(round_counter >= MAX_NUM_ROUNDS)
            {
              cur_state = GAME_OVER; // end the game
            }
            else // may have timed out this round, but try again!
            {
            cur_state = PICK_NEW; // update current_state to POLL
            }
          }
          else
          {
            nrk_time_get(&current_time);
            wait_node_timer = get_time_duration_ms(wait_node_start_time, current_time);
            if (wait_node_timer > NODE_WAIT_TIME) {
              cur_state = PICK_NEW;
            }
          }
        }
        break;
      }

      case GAME_OVER:
      {
        nrk_kprintf(PSTR("!!GAME OVER!!\r\n"));
        printf("  -> Final Score:%d\r\n", total_score);
        uint8_t updated_high_scores = update_high_scores(total_score);
        if (updated_high_scores == SCORES_UPDATED)
        {
          nrk_kprintf(PSTR("NEW HIGH SCORE!\r\n"));
          for (uint8_t i=0; i < 3; i++)
          {
            printf("  %d => %d\r\n", i+1, high_scores[i]);
          }
        }
        cur_state = INIT;
        break;
      }

      default:
      {
        cur_state = INIT;
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
  WHACKY_TASK.cpu_reserve.secs = 2;
  WHACKY_TASK.cpu_reserve.nano_secs = 0;
  WHACKY_TASK.offset.secs = 0;
  WHACKY_TASK.offset.nano_secs = 0;
  nrk_activate_task (&WHACKY_TASK);

  printf ("Create done\r\n");
}

