/**
* Copyright 2016 University of Applied Sciences Western Switzerland / Fribourg
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Project:    HEIA-FR / Embedded Systems 3 Laboratory
*
* Abstract:   Hasciicam client/server application
*
* Author:     C. Vallélian & G. Waeber
* Class:      T-3a
* Date:       11.01.2017
*/

#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "../data.h"

#define PRESSED  0
#define RELEASED 1

#define SLEEP_BTN_POLLING  30000
#define SLEEP_BTN_RELEASE  500000

// Methods
static void cleaner (void *p);


extern int id_queue_thr_ipc_server_button;

int fd;            // File descriptor for I/O (LEDs and buttons) device driver


/**
 * Handle pression on buttons SW1 and SW2
 */
void *server_thr_io (void *arg){

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push   (cleaner, NULL);

    bool stream_state   = false;           // true is stream has to be active
    bool stream_changed = false;           // true if stream state changed
    int  btn_1_prev_state = RELEASED;
    int  btn_2_prev_state = RELEASED;

    // IPC message
    struct MSG_SRV_BUTTON msg_button;
    memset (&msg_button, 0, sizeof(msg_button));
    msg_button.type                = MSG_TYPE;
    msg_button.header.sender       = SENDER_SERVER_THR_BUTTON;

    int  status_btn;       // return status when reading from device driver
    int  status_led;       // return status when writing to device driver
    char led[LED_NB];      // LEDs current state
    char btn[BTN_NB];      // Buttons current state


    // Open I/O device driver
    fd = open (IO_DD_PATH, O_RDWR);

    if(fd != -1){


      // Init LEDs
      led[0] = false;
      status_led = write(fd, led, LED_NB);

      // Polling on the buttons state
      while(1) {

         status_btn = read(fd, btn, BTN_NB);

         // Enable stream if button SW1 is pressed
         if((btn[0] == PRESSED) && (btn_1_prev_state == RELEASED) && (stream_state == false)){
            printf("Button SW 1 pressed\n");
            stream_state   = true;
            stream_changed = true;
         }

         // Disable stream if button SW2 is pressed
         if((btn[1] == PRESSED) && (btn_2_prev_state == RELEASED) && (stream_state == true)){
            printf("Button SW 2 pressed\n");
            stream_state   = false;
            stream_changed = true;
         }

         // Update buttons previous state for next loop iteration
         btn_1_prev_state = btn[0];
         btn_2_prev_state = btn[1];

         // Update LED and inform srv_thr_send that stream state changed
         if(stream_changed){

            // Change LED state
            led[0] = stream_state;
            status_led = write(fd, led, LED_NB);

            // Communicate stream state change to srv_thr_send
            msg_button.header.stream_state = stream_state;
            msgsnd (id_queue_thr_ipc_server_button, &msg_button, sizeof (msg_button.header), 0);

            // Stream change handled
            stream_changed = false;

            // Time for user to release the button
            usleep(SLEEP_BTN_RELEASE);

         }

         // Sleep between two polling on I/O device driver
         usleep(SLEEP_BTN_POLLING);

      }

   }else{
      printf("Unable to access I/O device driver !\n");
   }

   pthread_cleanup_pop(0);
   pthread_exit (NULL);

}



static void cleaner (void *p){
    close(fd);
    printf ("server_thr_button : Thread end\n");
}
