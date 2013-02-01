/*
 * fakestress.c
 *
 *  Created on: Jan 13, 2013
 *  Author    : mbehr
 *  Copyright (c) 2013, Matthias Behr, mbehr@mcbehr.de
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#define DRV_VERSION "1.2"

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h> /* for kmalloc */
#include <linux/errno.h> /* for e.g. -ENOMEM, ... */
#include <linux/sched.h> /* for TASK_INTERRUPTIBLE */
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/average.h>

/*
 * to-do list: (global, for features, enhancements,...
 *
 */

/* module parameter: */

static unsigned long param_num_threads = 0; /* default to number of cpus/cores */
module_param(param_num_threads, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_num_threads, "number of kernel threads to use. Default (0) = same as number of cpus/cores.");

static unsigned long param_busy_time_us = MSEC_PER_SEC; // 1ms as default for the (bad) stress
module_param(param_busy_time_us, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_busy_time_us, "stress/busy time  in us.");

static unsigned long param_idle_time_us = MSEC_PER_SEC; // 1ms as default for the idle time between stress times
module_param(param_idle_time_us, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_idle_time_us, "idle time in us.");

static int param_int_lock_during_busy = 0; /* default to off i.e. no int lock while busy looping */
module_param(param_int_lock_during_busy, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_int_lock_during_busy, "do interrupt lock during busy period (1 = thread/0, 2 = all threads");

static int param_int_measure_latency = 0; /* default to off, i.e. no time measuring of the latency */
module_param(param_int_measure_latency, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(param_int_measure_latency, "do latency measuring (1 = on, >=2 = reset stats for thread n-2, default(0) = off");

/*
 * we do access the params_ at runtime without any protection (semaphore, atomic,...).
 * For now we can life with it as the risk of partially written data is low and it doesn't matter
 * if we miss one change and apply it a loop later (even volatile can't be set as this leads to
 * macro warnings.
 * todo bug p2 mb: check how they can be protected vs. changes from /sys/modules/... changes.
 */

/* module global variables: */

struct task_struct **stress_tasks=0;
unsigned long use_threads=0; /* so many threads will actually be created/started */
atomic_t open_threads; /* so many threads are still running. We wait for 0 at module exit. */

/* exit function called on module unload: */

static void fakestress_exit(void)
{
	unsigned long i;
	printk( KERN_ALERT "fakestress_exit\n");

	/* stop our threads here and wait for them */
	for (i=0; i<use_threads; i++){
		if (stress_tasks && stress_tasks[i]){
			kthread_stop(stress_tasks[i]);
		}
	}
	if (stress_tasks){
		kfree(stress_tasks);
		stress_tasks = 0;
	}
	/* we do have to wait for the threads to stop as we are accessing at least the module parameters which will be invalid afterwards. */
	while(atomic_read(&open_threads)>0){
		msleep(2); /* we could use some semaphores, waitqueues,... as well but lets stick to this simple loop */
	}
	printk( KERN_INFO "fakestress all threads exited.\n");
}

int stress_fn(void *data); /* prototype, see implementation later. */

/* init function called on module load: */
static int __init fakestress_init(void)
{
	int retval = 0;
	unsigned long i;
	use_threads = param_num_threads;

	printk( KERN_ALERT "fakestress module_init (HZ=%d) (V" DRV_VERSION ") (c) M. Behr, 2013\n", HZ);

	printk( KERN_INFO "fakestress param_num_threads = %lu\n", param_num_threads);
	printk( KERN_INFO "fakestress param_busy_time_us = %lu\n", param_busy_time_us);
	printk( KERN_INFO "fakestress param_idle_time_us = %lu\n", param_idle_time_us);
	printk( KERN_INFO "fakestress param_int_lock_during_busy = %d\n", param_int_lock_during_busy);
	printk( KERN_INFO "fakestress param_int_measure_latency = %d\n", param_int_measure_latency);

	/* if param_num threads is zero autodetermine based on avail cpu/cores */
	if (0 == param_num_threads){
		use_threads = nr_cpu_ids;
		printk( KERN_INFO "fakestress will use %lu threads\n", use_threads);
	}
	atomic_set(&open_threads, 0);

	/* allocate memory for the tasks: */
	stress_tasks = kmalloc(use_threads * sizeof(struct task_struct *), GFP_KERNEL);
	if (!stress_tasks){
		printk( KERN_ALERT "fakestress can't alloc stress_tasks memory! Aborting\n");
		/* nothing else to cleanup till now */
		return -ENOMEM;
	}
	for (i=0; i<use_threads; i++){
		stress_tasks[i] = kthread_create(stress_fn, (void *)i, "fakestress/%lu", i);
		if (stress_tasks[i]){
			/* now bind to a cpu if autodetect was used: */
			if (0 == param_num_threads){
				kthread_bind(stress_tasks[i], i);
				printk( KERN_INFO "fakestress/%lu bound to cpu %lu\n", i, i);
			}
			/* and start it */
			atomic_inc(&open_threads);
			wake_up_process(stress_tasks[i]);
		}else{
			printk( KERN_ALERT "fakestress can't create fakestress/%lu", i);
			/* but we keep on running */
		}
	}

	return retval; // 0 = success,
}

module_init( fakestress_init );
module_exit( fakestress_exit );

int stress_fn(void *data){
	unsigned long thread_nr = (unsigned long) data;
	static DEFINE_SPINLOCK(lock); /* we define the spinlock
	static to the function but we don't use it for synchronization
	but just to lock the irqs */

	unsigned long latency_us_min = 0xffffffff;
	unsigned long latency_us_max = 0;
	struct ewma latency_us_avg;
#define EWMA_LAT_WEIGHT 1024
#define EWMA_LAT_FACTOR 8
	unsigned long latency_j_last_print = jiffies;

	struct timeval t_before, t_after;

	printk(KERN_INFO "fakestress/%lu stress_fn started\n", thread_nr);

	ewma_init(&latency_us_avg, EWMA_LAT_WEIGHT, EWMA_LAT_FACTOR); /* init the exponential weighted average */

	while(!kthread_should_stop()){
		int do_int_lock = (2 == param_int_lock_during_busy) || (1 == param_int_lock_during_busy && 0 == thread_nr);
		/* param = 1 --> int lock only on cpu 0, param = 2 --> int lock an all cpus */
		int measure_latency = param_int_measure_latency;
		/* 0 == off, 1 == on, 2 == reset counters from thread (-2 + param) */
		if (measure_latency == (thread_nr+2)){
			/* reset counters to zero. */
			printk(KERN_INFO "fakestress/%lu reset stats", thread_nr);
			printk(KERN_INFO "fakestress/%lu latency [us]: min=%lu, max=%lu, avg(ewma)=%lu\n", thread_nr,
					latency_us_min, latency_us_max, ewma_read(&latency_us_avg));
			latency_us_min = 0xffffffff;
			latency_us_max = 0;
			ewma_init(&latency_us_avg, EWMA_LAT_WEIGHT, EWMA_LAT_FACTOR);
			param_int_measure_latency=1; /* auto reset param to "on" */
		}
		/* loop idle */
		if (measure_latency){
			do_gettimeofday(&t_before);
		}
		if (param_idle_time_us>0)
			msleep(param_idle_time_us / 1000);
		if (measure_latency){
			unsigned long lat_us;
			do_gettimeofday(&t_after);
			lat_us = (t_after.tv_sec - t_before.tv_sec)*1000000ul +
					(t_after.tv_usec - t_before.tv_usec);
			if (lat_us < latency_us_min){
				latency_us_min = lat_us;
			}
			if (lat_us > latency_us_max){
				latency_us_max = lat_us;
			}
			ewma_add(&latency_us_avg, lat_us);
			/* print stat every 2s */
			if (time_after(jiffies, latency_j_last_print + (2*HZ))){
				latency_j_last_print = jiffies;
				printk(KERN_INFO "fakestress/%lu latency [us]: min=%lu, max=%lu, avg(ewma)=%lu\n", thread_nr,
						latency_us_min, latency_us_max, ewma_read(&latency_us_avg));
			}
		}

		/* loop badly -> create stress, i.e. cpu load */
		if (param_busy_time_us>0){
			unsigned long flags;
			if (do_int_lock){
				spin_lock_irqsave(&lock, flags);
			}
			if (param_busy_time_us>50000) /* use mdelay with max 2% deviation */
				mdelay(param_busy_time_us/1000);
			else
				udelay(param_busy_time_us);
			if (do_int_lock){
				spin_unlock_irqrestore(&lock, flags);
			}
		}
	}
	printk(KERN_INFO "fakestress/%lu stress_fn exited\n", thread_nr);
	atomic_dec(&open_threads);
	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Matthias Behr");
MODULE_DESCRIPTION("Driver fakeing/simulating system stress on other driver and apps. (patents pending)\n");
