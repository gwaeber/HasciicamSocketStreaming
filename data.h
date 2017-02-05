#pragma once
#ifndef DATA_H
#define DATA_H

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
#include <stdint.h>



// SERVER
// -----------------------------------------------------------------------------

#define FIFO_PATH "/tmp/hasciicamFifo"      // FIFO path
#define FIFO_BUF_SIZE 3204                  // FIFO buffer size (video 352x288  / ASCII 88x36)

#define IO_DD_PATH "/dev/io_dd"             // I/O device driver path
#define IO_DD_NAME "io_dd"                  // I/O device driver file name
#define IO_DD_NUMBER 42                     // Major number
#define IO_DD_KO_PATH "io_dd.ko" // I/O device driver .ko file

#define BTN_NB  4          // number of buttons
#define LED_NB  4          // number of LEDs

#define MAX_CLIENTS 4      // number of clients that can be connected at the same time
#define MSG_SIZE    1000   // size of the data send to the client throught the socket

/*
    Options (32 bit)   -   LSB to MSB
    +-----+--------+-------+--------------------------+
    | BIT |  NAME  | VALUE |         MEANING          |
    +-----+--------+-------+--------------------------+
    |   0 | SUB    |     1 | si client inscrit        |
    |     |        |     0 | si client pas inscrit    |
    |   1 | STREAM |     1 | si stream active         |
    |     |        |     0 | si stream pas active     |
    |   2 | START  |     1 | 1er fragment de data     |
    |     |        |     0 | -                        |
    |   3 | STOP   |     1 | dernier fragment de data |
    |     |        |     0 | -                        |
    +-----+--------+-------+--------------------------+
    Data (MSG_SIZE bytes)
*/

#define SUB_BIT     0      // SUBSCRIBE bit in the options uint32
#define STREAM_BIT  1      // STREAM bit in the options uint32
#define START_BIT   2      // START bit in the options uint32
#define STOP_BIT    3      // STOP bit in the options uint32

struct SERVER_DATA {
    uint32_t   options;          // options
    uint32_t   length;           // video data length
    char       data[MSG_SIZE];   // video data
};

// Used to store client sockets
struct SOCKET_TAB_STRUCT {
   struct sockaddr_in socket;
   bool               used;
};





// CLIENT
// -----------------------------------------------------------------------------

/*
    Options (32 bit)   -   LSB to MSB
    +-----+--------+-------+--------------------------+
    | BIT |  NAME  | VALUE |         MEANING          |
    +-----+--------+-------+--------------------------+
    |   0 | CMD    |     1 | subscribe                |
    |     |        |     0 | unsubscribe              |
    +-----+--------+-------+--------------------------+
*/

#define CMD_SUBSCRIBE     1
#define CMD_UNSUBSCRIBE   0

struct CLIENT_DATA {
    uint32_t   options;
};






// MESSAGE PASSING
// -----------------------------------------------------------------------------

#define QUEUE_THR_IPC_CLIENT          1
#define QUEUE_THR_IPC_SERVER_TABLE    3
#define QUEUE_THR_IPC_SERVER_SOCKET   4
#define QUEUE_THR_IPC_SERVER_BUTTON   5

#define SENDER_CLIENT_THR_CLI         1
#define SENDER_SERVER_THR_RECEIVE     2
#define SENDER_SERVER_THR_BUTTON      3

#define MAX_SIZE    16
#define MSG_TYPE     1


// Pass client socket list from server_thr_receive to server_thr_send
struct HEADER_CLIENT_LIST_CHANGE {
   unsigned short           sender;
   struct SOCKET_TAB_STRUCT socket_tab[MAX_CLIENTS];     // client socket table
};

struct MSG_CLIENT_LIST_CHANGE {
   long int      type;
   struct HEADER_CLIENT_LIST_CHANGE header;
};


// Pass server socket from server_thr_receive to server_thr_send
struct HEADER_SRV_SOCKET {
   unsigned short sender;
   int*           s;                                     // server socket
};

struct MSG_SRV_SOCKET {
   long int                 type;
   struct HEADER_SRV_SOCKET header;
};


// Pass server IP address from client_thr_cli to client_thr_socket_handler
struct HEADER_SRV_IP_ADDRESS {
   unsigned short sender;
   char           ip[15];                                // server IP address
};

struct MSG_SRV_IP_ADDRESS {
   long int                     type;
   struct HEADER_SRV_IP_ADDRESS header;
};


// Pass stream state from server_thr_button to server_thr_send
struct HEADER_SRV_BUTTON {
   unsigned short sender;
   bool           stream_state;     // true if stream enabled, false otherwise
};

struct MSG_SRV_BUTTON {
   long int                  type;
   struct HEADER_SRV_BUTTON  header;
};




#endif
