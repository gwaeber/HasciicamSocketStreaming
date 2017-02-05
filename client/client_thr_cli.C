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
* Date:       19.01.2017
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "../data.h"

// Methods
static void cleaner  (void *p);


extern int id_queue_thr_ipc_client;



void *client_thr_cli (void *arg){

    struct MSG_SRV_IP_ADDRESS msg;

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push   (cleaner, NULL);

    char* server_ip_add;                 // server IP entered by user
    unsigned long server_ip_add_test;    // server IP to test user entry


    // Ask and validate server IP
    printf ("Enter server IP address  : ");
    scanf("%ms", &server_ip_add);        // m is for dynamic allocation
    server_ip_add_test = inet_addr(server_ip_add);

    while( INADDR_NONE == server_ip_add_test ) {
      printf ("Wrong IP format !\n");
      printf ("Please enter server IP address again  : ");
      scanf("%ms", &server_ip_add);
      server_ip_add_test = inet_addr(server_ip_add);
    }


    // Send message to thread client_thr_send
    memset (&msg, 0, sizeof(msg));
    msg.type          = MSG_TYPE;
    msg.header.sender = SENDER_CLIENT_THR_CLI;
    strcpy(msg.header.ip, server_ip_add);
    msgsnd (id_queue_thr_ipc_client, &msg, sizeof (msg.header), 0);
    free(server_ip_add);

    pthread_cleanup_pop(0);
    pthread_exit (NULL);

}



static void cleaner (void *p){
    printf ("client_thr_cli: Thread end\n");
}
