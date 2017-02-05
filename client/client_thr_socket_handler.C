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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../data.h"
#include "../functions.h"

// Methods
static void cleaner  (void *p);
static void unsub();


extern int id_queue_thr_ipc_client;

static int s;
char*      ip = (char*) "";



void *client_thr_socket_handler (void *arg) {

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push   (cleaner, NULL);


    struct MSG_SRV_IP_ADDRESS msg;
    long int                  type = 0;

    struct CLIENT_DATA request;
    struct SERVER_DATA data;
    struct sockaddr_in sin;
    int                receive_result;


    // Receive server IP address from client_thr_cli
    while(1){
         msgrcv (id_queue_thr_ipc_client, &msg, sizeof(msg.header), type, 0);
         //printf("sender =%s", msg.header.sender);
         if (msg.header.sender == SENDER_CLIENT_THR_CLI){
             //printf ("DEBUG: client_th_send received IP = %s\n", msg.header.ip);
             ip = &msg.header.ip[0];
             //printf("DEBUG: IP=%s", ip);
             break;
         }
    }

    // Send request to server
    s = socket (AF_INET, SOCK_DGRAM, 0);
    memset (&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &sin.sin_addr);
    sin.sin_port = htons(1234);
    connect (s, (struct sockaddr *) &sin, sizeof(sin));

    // Send to the server
    request.options = CMD_SUBSCRIBE;
    printf ("\nSubscribing to server %s...\n", ip);
    write (s, &request, sizeof(request));

    // Receive the confirmation from the server
    printf("Waiting for an answer...\n");

    // Stream parameters
    bool sub, stream, stop;


    // Handle the received packets
    while(1){

        receive_result = read (s, &data, sizeof(data));
        if(receive_result != -1){

            sub    = getSubFromOptions(data.options);
            stream = getStreamFromOptions(data.options);

            if(sub && stream){

               // If the client is subscribed and there is a stream
               printf("Enjoy !\n");

               // Start reading and printing the data
               while(1){

                   // Read the socket
                   read (s, &data, sizeof(data));

                   // Get the new options bits
                   //start  = getStartFromOptions(data.options);
                   stop   = getStopFromOptions(data.options);
                   sub    = getSubFromOptions(data.options);
                   stream = getStreamFromOptions(data.options);

                   if(stream && sub){

                     // Get data from packet and display them
                     printf("%.*s", data.length, data.data);

                     // Handle stop bit, clear screen
                     if(stop) printf("\e[1;1H\e[2J");

                   }else{
                     printf("Stream paused, no more data to display.\n");
                     break;
                   }

               }

           }else if(sub && !stream){

             // Subscribed but no stream available
             printf("You are subscribed, but there is no stream available for the moment.\n");
             printf("Please wait a moment for the stream...\n");

             // Waiting for the stream to become available (stream bit)
             while(1){

               read (s, &data, sizeof(data));
               // Get new sub and stream bits
               sub    = getSubFromOptions(data.options);
               stream = getStreamFromOptions(data.options);

               // Done waiting if the stream becomes available or the client is no more subscribed (server down for ex.)
               if(stream || !sub) break;

             }

           }else{   // Implicit test !sub

             printf("You couldn't be subscribed (too much subscribers already). Please come back later.\n");
             break;
           }

       }else{
          printf("Server unavailable !\n");
          break;
      }

    }

    // Proper finish (unsubscribe from the stream and socket close)
    unsub();
    close(s);
    pthread_cleanup_pop(0);
    pthread_exit (NULL);

}



/**
 * Unsubscribe from video stream
 */
static void unsub(){
    struct CLIENT_DATA request;
    request.options = CMD_UNSUBSCRIBE;
    write (s, &request, sizeof(request));
}



static void cleaner (void *p){
    // Proper finish (unsubscribe from the stream and socket close)
    unsub();
    close(s);
    printf("Bye bye !");
}
