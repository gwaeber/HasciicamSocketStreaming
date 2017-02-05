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
* Date:       23.12.2016
*/

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "../data.h"

// Methods
static void catchSignal      (int signal);
static void createCatchSignal(void);


// Define all client threads
static pthread_t client_thr_socket_handler_ID;
static pthread_t client_thr_cli_ID;

void *client_thr_socket_handler (void *arg);
void *client_thr_cli            (void *arg);

int   id_queue_thr_ipc_client = -1;



int main (void) {

    printf ("** Start client **\n\n");

    void *returnMessage;

    createCatchSignal();

    // Create IPC queues
    id_queue_thr_ipc_client = msgget ((key_t)QUEUE_THR_IPC_CLIENT, 0666 | IPC_CREAT);

    pthread_create (&client_thr_cli_ID, NULL, client_thr_cli, NULL);
    pthread_create (&client_thr_socket_handler_ID, NULL, client_thr_socket_handler, NULL);

    pthread_join (client_thr_cli_ID, &returnMessage);
    pthread_join (client_thr_socket_handler_ID, &returnMessage);

    if (id_queue_thr_ipc_client != -1){
        msgctl(id_queue_thr_ipc_client, IPC_RMID, 0);
    }


    printf ("\n** Stop client **\n");
    exit (EXIT_SUCCESS);

}



static void createCatchSignal(void) {
    struct sigaction act;

    act.sa_handler = catchSignal;
    sigemptyset (&act.sa_mask);
    act.sa_flags   = 0;
    sigaction (SIGINT,  &act, 0);
    sigaction (SIGTSTP, &act, 0);
    sigaction (SIGTERM, &act, 0);
    sigaction (SIGABRT, &act, 0);
}



static void catchSignal (int signal) {
    //printf ("signal = %d\n", signal);
    pthread_cancel(client_thr_socket_handler_ID);
    pthread_cancel(client_thr_cli_ID);
}
