/*
 * fakestress.c
 *
 *  Created on: Jan 13, 2013
 *      Author: mbehr
 *  (c) M. Behr, 2013
 *
 *  Based on source code and documentation from the book
 *  "Linux Device Drivers" by Alessandro Rubini and Jonathan Corbet,
 *  published by O'Reilly & Associates.
 *  Thanks a lot to Rubini, Corbet and O'Reilly!
 *
 * todo add GPLv2 lic here
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h> // kmalloc
#include <linux/fs.h>
#include <linux/errno.h> // for e.g. -ENOMEM, ...
#include <linux/types.h>
#include <linux/semaphore.h>
// #include <asm/uaccess.h> // for access_ok
#include <linux/sched.h> // for TASK_INTERRUPTIBLE
// #include <linux/wait.h> // wait queues
#include <linux/time.h>
#include <linux/random.h>
#include <linux/delay.h>

/*
 * to-do list: (global, for features, enhancements,...
 * todo p3: check maximum values for .delay_us (16bit) vs. our default value and int overflow handling
 * todo p2: add statistics on latency after a sleep
 */

/* module parameter: */

static unsigned long param_num_threads = 0; /* default to number of cpus/cores */
module_param(param_num_threads, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_num_threads, "number of kernel threads to use. Default (0) = same as number of cpus/cores.");

static unsigned long param_busy_time_us = MSEC_PER_SEC; // 1ms as default for the (bad) stress
module_param(param_busy_time_us, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_busy_time_us, "default stress/busy time  in us.");

static unsigned long param_idle_time_us = MSEC_PER_SEC; // 1ms as default for the idle time between stress times
module_param(param_idle_time_us, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_idle_time_us, "default idle time in us.");


//DEFINE_SEMAPHORE(sem_interchange);
//DECLARE_WAITQUEUE_HEAD(event_master);
// wait_queue_head_t event_master;


// exit function on module unload:
// used from _init as well in case of errors!

static void fakestress_exit(void)
{
	printk( KERN_ALERT "fakestress_exit\n");

	/* stop our threads here and wait for them */

}

// init function on module load:
static int __init fakestress_init(void)
{
	int retval = 0;
	unsigned long use_threads = param_num_threads;

	printk( KERN_ALERT "fakestress module_init (HZ=%d) (c) M. Behr, 2013\n", HZ);

	printk( KERN_INFO "fakestress param_num_threads = %lu\n", param_num_threads);
	printk( KERN_INFO "fakestress param_busy_time_us = %lu\n", param_busy_time_us);
	printk( KERN_INFO "fakestress param_idle_time_us = %lu\n", param_idle_time_us);

	/* if param_num threads is zero autodetermine based on avail cpu/cores */
	if (0 == param_num_threads){
		use_threads = nr_cpu_ids;
		printk( KERN_INFO "fakestress will use %lu threads\n", use_threads);
	}


//	init_waitqueue_head(&event_master);

/*
	vspi_devices = kmalloc( VSPI_NR_DEVS * sizeof( struct vspi_dev), GFP_KERNEL);
	if (!vspi_devices){
		retval = -ENOMEM;
		goto fail;
	}
	memset(vspi_devices, 0, VSPI_NR_DEVS * sizeof(struct vspi_dev));
*/

	return retval; // 0 = success,
}

module_init( fakestress_init );
module_exit( fakestress_exit );


// todo p3 define license MODULE_LICENSE("proprietary");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Matthias Behr");
MODULE_DESCRIPTION("Driver fakeing system stress on other driver and apps. (patents pending)\n");
