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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
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
static void cleaner (void *p);


static int s;                                         // server socket
struct SOCKET_TAB_STRUCT socket_tab[MAX_CLIENTS];     // store clients socket

extern int id_queue_thr_ipc_server_table;
extern int id_queue_thr_ipc_server_socket;

extern int id_queue_thr_ipc_server_button;




/**
 * Used to store the client socket in the table when a client subscribes
 *
 * @param from (sockaddr_in) the client socket to store
 *
 * @return tab_id (int) the id in the table of the new client, -1 if store failed
 */
int storeSocket(sockaddr_in from){

   for (int i = 0; i < MAX_CLIENTS; i++){
      if(!socket_tab[i].used){
         socket_tab[i].socket = from;
         socket_tab[i].used = true;
         return i;
      }
   }

   return -1;

}




/**
 * Used to remove the client socket in the table when a client unsubscribes
 *
 * @param from (sockaddr_in) the client socket to remove
 *
 * @return tab_id (int) the id in the table of the new client, -1 if store failed
 */
int removeSocket(sockaddr_in from){

   char* client_ip   = inet_ntoa(from.sin_addr);
   int   client_port = from.sin_port;

   for (int i = 0; i < MAX_CLIENTS; i++){
      if((strcmp(inet_ntoa(socket_tab[i].socket.sin_addr), client_ip) == 0) && (socket_tab[i].socket.sin_port == client_port)){
         socket_tab[i].used = false;
         return i;
      }
   }

   return -1;

}





/**
 * Handle incomming data (sub, unsub) on a socket from clients
 */
void *server_thr_receive (void *arg){

    // IPC messages and type
    struct MSG_CLIENT_LIST_CHANGE msg;    // notify server_thr_send that client table changed

    // thread
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push   (cleaner, NULL);

    struct CLIENT_DATA  data;             // data passed from client throught the socket

    // server socket
    struct sockaddr_in from;
    struct sockaddr_in sin;
    socklen_t alen;

    s = socket (AF_INET, SOCK_DGRAM, 0);
    memset (&sin, 0, sizeof(sin));
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port        = htons(1234);

    alen = bind (s, (struct sockaddr*) &sin, sizeof (sin));


    // Pass socket to send thread
    struct MSG_SRV_SOCKET msg_srv_socket;
    memset (&msg_srv_socket, 0, sizeof(msg_srv_socket));
    msg_srv_socket.type          = MSG_TYPE;
    msg_srv_socket.header.sender = SENDER_SERVER_THR_RECEIVE;
    msg_srv_socket.header.s      = &s;
    msgsnd (id_queue_thr_ipc_server_socket, &msg_srv_socket, sizeof (msg_srv_socket.header), 0);



    while(1){

       alen = sizeof(from);

       // Receive data from client
       int rec_res = recvfrom (s, &data, sizeof(data), 0, (struct sockaddr*) &from, &alen);

       if (rec_res > -1) {

          // Print user data
          printf ("Server received following message through socket \nCMD : %hu\nIP  : %s:%i\n\n", data.options, inet_ntoa(from.sin_addr), from.sin_port);

          int  res = 0;                // result of the store/remove method

          // Handle subscription or unsubscription
          if (data.options == 1){

             // SUBSCRIBE
             res = storeSocket(from);

             // Send IPC message to thread client_th_send
             memset (&msg, 0, sizeof(msg));
             msg.type          = MSG_TYPE;
             msg.header.sender = SENDER_SERVER_THR_RECEIVE;

             // Send feedback to client
             struct SERVER_DATA data;

             if(res == -1){

                // Subscription failed (too much clients connected), send error message to client
                data.options = buildOptions(false, false, false, false);
                sendto (s, &data, sizeof(data), 0, (struct sockaddr*) &from, alen);

             } else if (res > -1){

                // Subscription success, send confirmation to client
                data.options = buildOptions(true, false, false, false);
                sendto (s, &data, sizeof(data), 0, (struct sockaddr*) &from, alen);

                // IPC send new sockets group to server_thr_send
                memcpy(msg.header.socket_tab, socket_tab, sizeof(msg.header.socket_tab));
                msgsnd (id_queue_thr_ipc_server_table, &msg, sizeof (msg.header), 0);

             } else {
                printf ("\nSomething went wrong ...\n\n");
             }


          } else if (data.options == 0){

             // UNSUBSCRIBE
             res = removeSocket(from);
             if(res == -1){

                printf("\nServer could not unsubscribe client, client was not subscribed\n\n");

             } else if (res > -1){

                // IPC send new sockets group to server_thr_send
                memcpy(msg.header.socket_tab, socket_tab, sizeof(msg.header.socket_tab));
                msgsnd (id_queue_thr_ipc_server_table, &msg, sizeof (msg.header), 0);

             } else {
                printf ("\nSomething went wrong ...\n\n");
             }



          } else {
             printf("Bad options cmd !\n");
          }


      }







    }


    pthread_cleanup_pop(0);
    pthread_exit (NULL);

}



static void cleaner (void *p){
    printf ("server_thr_receive : Thread end\n");
    close(s);
}
