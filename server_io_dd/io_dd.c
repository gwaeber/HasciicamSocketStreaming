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

#include <linux/fs.h>
#include <linux/gpio.h>           // needed for i/o handling
#include <linux/init.h>           // needed for macros
#include <linux/interrupt.h>      // needed for interrupt handling
#include <linux/io.h>             // needed for mmio handling
#include <linux/kernel.h>         // needed for debugging
#include <linux/module.h>         // needed by all modules
#include <linux/poll.h>

/*
 * led1 - XE.INT23     - gpx2.7 -  31
 * led2 - XE.INT20     - gpx2.4 -  28
 * led3 - XE.INT11     - gpx1.3 -  19
 * led4 - XE.I2C1.SDA  - gpb3.2 - 209
 *
 * sw1  - XE.INT21     - gpx2.5 -  29
 * sw2  - XE.INT22     - gpx2-6 -  30
 * sw3  - XE.INT14     - gpx1-6 -  22
 * sw4  - XE.INT10     - gpx1.2 -  18
 */

#define SW1      29
#define SW2      30
#define SW3      22
#define SW4      18
#define SW_cnt    4

#define LED1     31
#define LED2     28
#define LED3     19
#define LED4    209
#define LED_cnt   4

#define IO_DD_MAJOR 42


// functions
static int     io_dd_open    (struct inode *inode, struct file *file);
static int     io_dd_release (struct inode *inode, struct file *file);
static ssize_t io_dd_read    (struct file *file, char *buf, size_t count, loff_t *offset);
static ssize_t io_dd_write   (struct file *file, const char *buf, size_t count, loff_t *offset);

static struct file_operations io_fops = {
    read:      io_dd_read,
    write:     io_dd_write,
    open:      io_dd_open,
    release:   io_dd_release
};



static int io_dd_open (struct inode *node, struct file *filp){
    if (filp->f_mode & FMODE_READ){
        printk (KERN_DEBUG "io_dd opened for reading\n");
    }
    if (filp->f_mode & FMODE_WRITE){
        printk (KERN_DEBUG "io_dd opened for writing\n");
    }
    printk (KERN_DEBUG "io_dd open, major: %d minor: %d\n", MAJOR(node->i_rdev), MINOR(node->i_rdev));
    return (0);
}


static int io_dd_release (struct inode *node, struct file *filp){
    printk (KERN_DEBUG "io_dd: release\n");
    return 0;
}


// Return buttons state to user
static ssize_t io_dd_read(struct file *filp, char __user *buf, size_t count, loff_t *offp){

   char sw_buf[SW_cnt];

   sw_buf[0] = gpio_get_value(SW1);
   sw_buf[1] = gpio_get_value(SW2);
   sw_buf[2] = gpio_get_value(SW3);
   sw_buf[3] = gpio_get_value(SW4);

   if (copy_to_user (buf, sw_buf, SW_cnt)){
      return -EFAULT;
   }

    return SW_cnt;

}


// Configure LEDs state from user
static ssize_t io_dd_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp){

   char led_buf[LED_cnt];

   if (copy_from_user (led_buf, buf, 4)){
      return -EFAULT;
   }

    gpio_set_value(LED1, led_buf[0]);
    gpio_set_value(LED2, led_buf[1]);
    gpio_set_value(LED3, led_buf[2]);
    gpio_set_value(LED4, led_buf[3]);

    return LED_cnt;

}


// Configure GPIO and initialize LEDs
static int __init io_dd_init(void){

   int status = 0;

   status = register_chrdev(IO_DD_MAJOR, "io_dd", &io_fops);

   // configure gpio for switch input 1 to 4
   if (status == 0) status = gpio_request (SW1, "sw1");
   if (status == 0) status = gpio_direction_input(SW1);
   if (status == 0) status = gpio_request (SW2, "sw2");
   if (status == 0) status = gpio_direction_input(SW2);
   if (status == 0) status = gpio_request (SW3, "sw3");
   if (status == 0) status = gpio_direction_input(SW3);
   if (status == 0) status = gpio_request (SW4, "sw4");
   if (status == 0) status = gpio_direction_input(SW4);

   // configure gpio for led output 1 to 4
   if (status == 0) status = gpio_request (LED1, "led1");
   if (status == 0) status = gpio_direction_output(LED1, 1);
   if (status == 0) status = gpio_request (LED2, "led2");
   if (status == 0) status = gpio_direction_output(LED2, 1);
   if (status == 0) status = gpio_request (LED3, "led3");
   if (status == 0) status = gpio_direction_output(LED3, 1);
   if (status == 0) status = gpio_request (LED4, "led4");
   if (status == 0) status = gpio_direction_output(LED4, 1);

   // initialize LEDs
   gpio_set_value(LED1, 0);
   gpio_set_value(LED2, 0);
   gpio_set_value(LED3, 0);
   gpio_set_value(LED4, 0);

   pr_info ("Linux module io_dd loaded (%d)\n", status);

   return status;
}



static void __exit io_dd_exit(void){

   gpio_free(SW1);
   gpio_free(SW2);
   gpio_free(SW3);
   gpio_free(SW4);

   gpio_free(LED1);
   gpio_free(LED2);
   gpio_free(LED3);
   gpio_free(LED4);

   unregister_chrdev(IO_DD_MAJOR, "io_dd");

   pr_info("Linux module io_dd unloaded\n");

}


module_init (io_dd_init);
module_exit (io_dd_exit);


MODULE_AUTHOR ("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION ("Module io_dd");
MODULE_LICENSE ("GPL");
