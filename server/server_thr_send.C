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

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../data.h"
#include "../functions.h"



// Methodss
static void cleaner (void *p);


// Variables
static int* s;
struct SOCKET_TAB_STRUCT socket_tab_send[MAX_CLIENTS];     // store clients socket

extern int id_queue_thr_ipc_server_table;
extern int id_queue_thr_ipc_server_socket;
extern int id_queue_thr_ipc_server_button;

int   fifo_fd;           // FIFO to receive video data from hascicam
char *fifo_buf;          // Buffer to store video data received from FIFO
char *fifo_buf_o;        // Pointer on FIFO original address
int   nbBytes;           // Number of bytes read from FIFO

int   nbFullPackets;     // Number of full packets of size (MSG_SIZE) when fragmenting video data
int   nbTotalPackets;    // Total number of packets
bool  suppPacket;        // Supplementary packet of size < MSG_SIZE when fragmenting
int   suppPacketSize;    // Size of the supplementary packet when fragmenting

bool stream_state;       // true if stream active

/**
 * Send video data to users
 */
void *server_thr_send (void *arg){

    // IPC messages and type
    struct MSG_SRV_BUTTON         msg_button;      // get noritify by button that stream state changed
    struct MSG_SRV_SOCKET         msg_srv_socket;  // get server socket from server_thr_receive
    struct MSG_CLIENT_LIST_CHANGE msg;             // get notified by server_thr_receive that client table changed
    long int type = 0;

    // Thread
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push   (cleaner, NULL);

    struct SERVER_DATA data;              // data passed to clients throught the sockets

    stream_state = false;                 // initial stream state

    // Receive IPC from server_thr_receive with server socket
    msgrcv (id_queue_thr_ipc_server_socket, &msg_srv_socket, sizeof(msg_srv_socket.header), 0, 0);
    if (msg_srv_socket.header.sender == SENDER_SERVER_THR_RECEIVE) {
       s = msg_srv_socket.header.s;
    } else {
       printf("Server socket not recevied !\n");
       pthread_exit (NULL);
    }

    // Allocate space for FIFO buffer
    fifo_buf = (char*)malloc(FIFO_BUF_SIZE);
    printf("server_thr_send opening FIFO for reading, waiting on writer (hasciicam)\n");

    // Open FIFO in RO
    fifo_fd  = open(FIFO_PATH, O_RDONLY);
    if(fifo_fd == -1) printf("Unable to open FIFO !\n");
    printf("Hasciicam writer available\n");
    fifo_buf_o = fifo_buf;

    // Calculate packets number and size for video data fragmenting
    nbFullPackets   = FIFO_BUF_SIZE/MSG_SIZE;
    suppPacketSize  = FIFO_BUF_SIZE-(nbFullPackets*MSG_SIZE);
    suppPacket      = (suppPacketSize > 0);
    nbTotalPackets  = (suppPacket) ? (nbFullPackets+1) : nbFullPackets;


    // Main loop
    while(1){


      // If no stream, wait until stream is available (again)
      if(!stream_state){

         // Blocking IPC
         msgrcv (id_queue_thr_ipc_server_button, &msg_button, sizeof(msg_button.header), type, 0);
         if (msg_button.header.sender == SENDER_SERVER_THR_BUTTON) {

           // IPC message parameters received from button thread
           stream_state = msg_button.header.stream_state;
           printf("Stream state changed (%i)\n", stream_state);

           // Empty sender to mark message as "read"
           msg_button.header.sender = 0;

           // Inform user that stream is available
           for(int i = 0; i < MAX_CLIENTS; i++){
              if (socket_tab_send[i].used){
                 printf("Inform  client %i that video is available.\n", i);
                 data.options = 3;
                 sendto (*s, &data, sizeof(data), 0, (struct sockaddr*) &socket_tab_send[i].socket, sizeof(socket_tab_send[i].socket));
              }
           }

           // Empty the FIFO
           // TODO à voir si solution

         }

      }


      // Receive IPC (bool stream status) from button thread
      msgrcv (id_queue_thr_ipc_server_button, &msg_button, sizeof(msg_button.header), type, IPC_NOWAIT);
      if (msg_button.header.sender == SENDER_SERVER_THR_BUTTON) {

          // IPC message parameters received from button thread
          stream_state = msg_button.header.stream_state;
          printf("Stream state changed (%i)\n", stream_state);

          // empty sender to mark message as "read"
          msg_button.header.sender = 0;

          // Inform user that stream is not available
          if(!stream_state){
             for(int i = 0; i < MAX_CLIENTS; i++){
               if (socket_tab_send[i].used){
                  printf("Inform  client %i that video is not available.\n", i);
                  data.options = 1;
                  sendto (*s, &data, sizeof(data), 0, (struct sockaddr*) &socket_tab_send[i].socket, sizeof(socket_tab_send[i].socket));
               }
            }
          }


      }



      // Receive IPC (client socket table) from server_thr_receive
      msgrcv (id_queue_thr_ipc_server_table, &msg, sizeof(msg.header), type, IPC_NOWAIT);
      if (msg.header.sender == SENDER_SERVER_THR_RECEIVE) {

          // IPC message parameters received from receive thread
          printf ("Server send thread received following IPC from receive thread :\n");
          for(int i = 0; i < MAX_CLIENTS; i++){
             printf ("socket_%i    : %s:%i (%i)\n", i, inet_ntoa(msg.header.socket_tab[i].socket.sin_addr), msg.header.socket_tab[i].socket.sin_port, msg.header.socket_tab[i].used);
          }
          printf("\n");

          // get the new client socket table
          memcpy(socket_tab_send, msg.header.socket_tab, sizeof(msg.header.socket_tab));

          // empty sender to mark message as "read"
          msg.header.sender = 0;

      }


      // Get video data from FIFO
      fifo_buf = fifo_buf_o;
      memset (fifo_buf, 0, FIFO_BUF_SIZE);
      nbBytes = read (fifo_fd, fifo_buf, FIFO_BUF_SIZE);
      //printf ("server_thr_send received %d bytes from Hasciicam FIFO\n", nbBytes);

      // Get video data and send it to subscribed clients
      if(stream_state){

         // Split video data and send it to clients
         for(int i = 1; i <= nbTotalPackets; i++){

              // Test if fragment is first (START) or last (STOP)
              if(i == 0){
                 data.options = buildOptions(true, true, true, false);  // stream with START bit
              }else if (i == nbTotalPackets){
                 data.options = buildOptions(true, true, false, true);  // stream with STOP bit
              }else{
                 data.options = buildOptions(true, true, false, false); // stream
              }

              // Test video data size
              if((i == nbTotalPackets) && (suppPacket)){
                 memcpy(data.data, fifo_buf, suppPacketSize);
                 fifo_buf = fifo_buf+suppPacketSize;
                 data.length = suppPacketSize;
              }else{
                 memcpy(data.data, fifo_buf, MSG_SIZE);
                 fifo_buf = fifo_buf+MSG_SIZE;
                 data.length = MSG_SIZE;
              }

              // Client loop
              for(int j = 0; j < MAX_CLIENTS; j++){
                 if (socket_tab_send[j].used){
                    //printf("Send data fragment %i to client %i\n", i, j);
                    sendto (*s, &data, sizeof(data), 0, (struct sockaddr*) &socket_tab_send[j].socket, sizeof(socket_tab_send[j].socket));
                }

              } // end client loop

         } // end video loop



      }


    }



    pthread_cleanup_pop(0);
    pthread_exit (NULL);

}



static void cleaner (void *p){
    close (fifo_fd);
    free(fifo_buf_o);
    printf ("server_thr_send: Thread end\n");
}
