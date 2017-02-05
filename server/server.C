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
#include <linux/kdev_t.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "../data.h"

#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

// Methods
static void catchSignal       (int signal);
static void createCatchSignal (void);
static void server_exit       (void);
static void create_fifo       (void);
static void remove_fifo       (void);
static void create_io_dd      (void);
static void insert_io_dd      (void);
static void remove_io_dd      (void);


static pthread_t server_thr_send_ID;
static pthread_t server_thr_receive_ID;
static pthread_t server_thr_io_ID;

void *server_thr_send    (void *arg);
void *server_thr_receive (void *arg);
void *server_thr_io      (void *arg);

int   id_queue_thr_ipc_server_table  = -1;
int   id_queue_thr_ipc_server_socket = -1;
int   id_queue_thr_ipc_server_button = -1;


int main (void) {

    printf ("** Start server **\n\n");

    void *returnMessage;

    createCatchSignal();

    // Remove FIFO if already exists and then create it
    remove_fifo();
    create_fifo();

    // Create and init I/O device driver
    remove_io_dd();
    create_io_dd();
    insert_io_dd();

    // Launch Hasciicam
    int status = system("hasciicam -m text -s 352x288 &");
    if(status == -1){
      printf("Error while launching hasciicam (status=%i)", status);
      server_exit();
    }

    // Create IPC queues
    id_queue_thr_ipc_server_table  = msgget ((key_t)QUEUE_THR_IPC_SERVER_TABLE, 0666 | IPC_CREAT);
    id_queue_thr_ipc_server_socket = msgget ((key_t)QUEUE_THR_IPC_SERVER_SOCKET, 0666 | IPC_CREAT);
    id_queue_thr_ipc_server_button = msgget ((key_t)QUEUE_THR_IPC_SERVER_BUTTON, 0666 | IPC_CREAT);

    pthread_create (&server_thr_send_ID, NULL, server_thr_send, NULL);
    pthread_create (&server_thr_receive_ID, NULL, server_thr_receive, NULL);
    pthread_create (&server_thr_io_ID, NULL, server_thr_io, NULL);

    pthread_join (server_thr_send_ID, &returnMessage);
    pthread_join (server_thr_receive_ID, &returnMessage);
    pthread_join (server_thr_io_ID, &returnMessage);

    if (id_queue_thr_ipc_server_table != -1){
        msgctl(id_queue_thr_ipc_server_table, IPC_RMID, 0);
    }
    if (id_queue_thr_ipc_server_socket != -1){
        msgctl(id_queue_thr_ipc_server_socket, IPC_RMID, 0);
    }
    if (id_queue_thr_ipc_server_button != -1){
        msgctl(id_queue_thr_ipc_server_button, IPC_RMID, 0);
    }


    server_exit();

}


static void server_exit(void){
   printf ("\n** Stop server **\n");
   exit (EXIT_SUCCESS);
}


// Create FIFO
static void create_fifo(void){
   umask(0);
  if(mknod(FIFO_PATH, S_IFIFO|0666, 0) == -1){
     printf("Unable to create FIFO (%s) !\n", FIFO_PATH);
     server_exit();
  }
  if (access(FIFO_PATH, F_OK) == -1){
     printf("FIFO (%s) not found!\n", FIFO_PATH);
     server_exit();
  }
}


// Remove FIFO file if exists
static void remove_fifo(void){
   if (access(FIFO_PATH, F_OK) == 0){
     if(remove(FIFO_PATH) == -1){
        printf("Unable to remove FIFO (%s) !\n", FIFO_PATH);
        server_exit();
     }
   }
}


// Create I/O device driver (character) file
static void create_io_dd(void){
   umask(0);
   // mknod /dev/io_dd c 42 0
   dev_t io_dd_numbers = MKDEV(IO_DD_NUMBER, 0);
   if(mknod(IO_DD_PATH, S_IFCHR|0644, io_dd_numbers) == -1){
      printf("Unable to create I/O device driver file (%s) !\n", IO_DD_PATH);
   }
   if (access(IO_DD_PATH, F_OK) == -1){
      printf("I/O device driver (%s) not found!\n", IO_DD_PATH);
      server_exit();
   }

}


// Insert I/O device driver in kernel
static void insert_io_dd(void){
   // insmod io_dd.ko
   int fd = open(IO_DD_KO_PATH, O_RDONLY);
   struct stat st;
   fstat(fd, &st);
   size_t image_size = st.st_size;
   void *image = malloc(image_size);
   read(fd, image, image_size);
   close(fd);
   if (init_module(image, image_size, "") != 0) printf("Unable to load I/O device driver (%s) from %s !\n", IO_DD_NAME, IO_DD_KO_PATH);
}


// Remove I/O device driver file if exists
static void remove_io_dd(void){
   if (delete_module(IO_DD_NAME, O_NONBLOCK) != 0) printf("I/O device driver not unloaded\n");
   if (access(IO_DD_PATH, F_OK) == 0){
     // rmmod io_dd.ko
     if(remove(IO_DD_PATH) == -1) printf("Unable to remove I/O device driver (%s) !\n", IO_DD_PATH);
   }
}


static void createCatchSignal(void){
    struct sigaction act;

    act.sa_handler = catchSignal;
    sigemptyset (&act.sa_mask);
    act.sa_flags   = 0;
    sigaction (SIGINT,  &act, 0);
    sigaction (SIGTSTP, &act, 0);
    sigaction (SIGTERM, &act, 0);
    sigaction (SIGABRT, &act, 0);
}


static void catchSignal (int signal){
    //printf ("signal = %d\n", signal);
    pthread_cancel(server_thr_send_ID);
    pthread_cancel(server_thr_receive_ID);
    pthread_cancel(server_thr_io_ID);

    remove_fifo();
    remove_io_dd();

}
