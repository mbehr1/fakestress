[![Build Status](https://travis-ci.org/mbehr1/fakestress.svg?branch=master)](https://travis-ci.org/mbehr1/fakestress)

README for the fakestress driver.
=================================

Usage:
------
To build the module simply call

`make`

;-) (well will be more difficult for cross compilation but in any case quite simple)

To load the module simply call the provided script:

`sudo ./fakestress_load`

or load the module manually.

To unload the module simply call the provided script:

`sudo ./fakestress_unload`

or unload the module manually.

Module parameters:
------------------
The behaviour of the kernel module can be adjusted by setting the following parameters
either at module startup (e.g. `insmod fakestress param_x=y param_y=w`)
or at runtime using the `/sys/module/fakestress/parameters/` interface.

E.g. to enable interrupt locks on all cpus (using spinlock_irqsave) do as root:

`echo -n 2 > /sys/module/fakestress/parameters/param_int_lock_during_busy`

Currently there are 5 parameters:
- param_num_threads: number of kernel threads to use. Defaults to 0 = autodetect, i.e. same as number of cpus.
 With the 0 the threads will be bound to a cpu (/0 to cpu 0, /1 to cpu 1,...). If a value is given threads
 will not be bound.

- param_busy_time_us: stress/busy time in usecs. Defaults to 1ms. Is using mdelay if >50000 otherwise udelay.

- param_idle_time_us: idle time in usecs. Defaults to 1ms. Is using msleep(value/1000). If set to 0 no mdelay is
 used so this will most likely completely hang your system at least if used on all cpus.

- param_int_lock_during_busy: do interrupt lock during busy period. 

  value | behaviour
  ----- | ---------
  0 (default) | off (no interrupt locks)
  1 | enable interrupt locks on thread/0
  2 | enable interrupt locks on all threads

  This uses a spinlock_irqsave/restore around the busy looping on none, thread/0 or all threads.

- param_measure_latency: measure the latency in microseconds (us) of the idle loop. The time is measured without
 the time of the msleep. Prints the min, max and avg (based on ewma 1024 8) and distribution per 1/2HZ values every 2 seconds.
 Default to 0 (off), 1 = do measurement on all threads. >=2 reset the statistics of thread <id-2>.
 

Matthias Behr, January 13th 2013 (last update June 29th 2014). 
 
