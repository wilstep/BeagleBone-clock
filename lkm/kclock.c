/**
 * @file    kclock.c
 * @author  Stephen R Williams
 * @date    30 Aug 2018
 * @version 0.1
 * @brief  A kenral to run a 12hr clock, using the gpio to run a LCD display
*/

#include <linux/time.h>
#include <linux/unistd.h>
#include <linux/delay.h> 
#include <linux/init.h>             // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>           // Core header for loading LKMs into the kernel
#include <linux/kernel.h>           // Contains types, macros, functions for the kernel
#include <linux/gpio.h>  
#include <linux/kthread.h> 

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Stephen R Williams");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A 12hr clock, using the gpio to run a LCD display.");  ///< The description -- see modinfo
MODULE_VERSION("0.1");              ///< The version of the module

#define ngpio 23
static int gpio_seg[] = {2, 3, 4, 5, 14, 15, 23, 26, 44, 45, 46, 47, 48, 49, 50, 51, 60, 61, 65, 66, 67, 68, 69};
static int gpio_map[3][7] = {{61, 14, 49, 15, 3, 65, 46}, {44, 26, 2, 5, 4, 68, 67}, {69, 66, 51, 50, 60, 45, 23}};
//static unsigned int offset = -7200;
static bool num_map[11][7] = { {1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0}, {1, 1, 0, 1, 1, 0, 1},
                                   {1, 1, 1, 1, 0, 0, 1}, {0, 1, 1, 0, 0, 1, 1}, {1, 0, 1, 1, 0, 1, 1},
                                   {1, 0, 1, 1, 1, 1, 1}, {1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 1, 1},
                                   {1, 1, 1, 1, 0, 1, 1}, {0, 0, 0, 0, 0, 0, 0} };


static int offset = 0;    
module_param(offset, int, S_IRUGO); ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(offset, "the time offset");  ///< parameter description
static struct task_struct *task;            /// The pointer to the thread task



void wipe(void)
{
    int i;

    for(i=0; i<ngpio; ++i){
        gpio_set_value(gpio_seg[i], 0);              // Turn the LED off, makes it clear the device was unloaded
    }
}

void display(int n, int place)
{
    int i;
    
    if(place == 3){ // left most digit display numeral 1 or nothing, driven by 2 ports only      
        gpio_set_value(47, n);
        gpio_set_value(48, n);
        return;
    }
    for(i=0; i<7; ++i){
        gpio_set_value(gpio_map[place][i], num_map[n][i]);
    }
}

/** @brief The LED Flasher main kthread loop
 *
 *  @param arg A void pointer used in order to pass data to the thread
 *  @return returns 0 if successful
 */
static int clock(void *arg){
    int hour, min;
    struct timeval rawtime;
    struct tm timeinfo;

    printk(KERN_INFO "BBB kclock: Thread has started running \n");
    while(!kthread_should_stop()){           // Returns true when kthread_stop() is called
        wipe(); // give LCD a brief rest (seems to help legibility of screen), rough pwm
        msleep(10); // sleep for 20ms
        do_gettimeofday(&rawtime);
        time_to_tm(rawtime.tv_sec, offset, &timeinfo);
        min = timeinfo.tm_min;
        hour = timeinfo.tm_hour;
        display(min%10, 0);
        display(min/10, 1);
        if(hour > 12) hour -= 12;
        if(hour == 0) hour = 12;
        display(hour%10, 2);
        display(hour/10, 3);
        msleep(5); // sleep for 10ms
    }
    printk(KERN_INFO "BBB kclock: Thread has run to completion \n");
    printk(KERN_INFO "Offset was %d\n", offset);
    return 0;
}


/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init kclock_init(void){
    int i;

    printk(KERN_INFO "12hr Clock: Hello from the BBB LKM!\n");
    for(i=0; i<ngpio; ++i){
        gpio_request(gpio_seg[i], "sysfs");          // gpioLED is hardcoded to 49, request it
        gpio_direction_output(gpio_seg[i], false);   // Set the gpio to be in output mode and off
        //gpio_set_value(gpio_seg, 1);           // Not required as set by line above (here for reference)
        //gpio_export(gpioLED, false);             // Causes gpio49 to appear in /sys/class/gpio
    }
    task = kthread_run(clock, NULL, "clock_thread");   // Start the clock kthread
    if(IS_ERR(task)){                                     // Kthread name is LED_flash_thread
        printk(KERN_ALERT "BBB kclock: failed to create the task\n");
        return PTR_ERR(task);
    }
    return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit kclock_exit(void){
    kthread_stop(task);                      // Stop the LED flashing thread
    printk(KERN_INFO "12hr Clock: Goodbye from the BBB LKM!\n");
    wipe();
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(kclock_init);
module_exit(kclock_exit);
