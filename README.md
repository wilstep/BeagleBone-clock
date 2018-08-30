# BeagleBone 12hr Clock  
![alt text](https://raw.githubusercontent.com/wilstep/BeagleBone-clock/master/setup.jpg)

This project makes use of a 4 digit LCD display to form a 12hr clock. The are two separate programs that achieve this same end. The first is written in C++ and runs at the user level, while the second is written in C and is a loadable kernel module (LKM) running inside the kernel. The project makes extensive use of the BeagleBone's GPIO ports to run the display. Because 23 GPIO ports are used it is necessary to make use of a custom device tree overlay, to change the modes on some of the ports. The number of GPIO ports could be reduced to 14 by using three 7-segment decoders (note: the left most digit either displays numeral 1 or nothing, and thus is controlled with only 2 GPIO ports), and then the custom device tree wouldn't be needed. A picture of my setup is given in 'clock.jpg'. The display is kept in sync with the BeagleBone's internal clock by the C++ program `clock.cpp` or alternatively by the LKM which may be compiled from `kclock.c`.

I used the following LCD display, (https://www.jaycar.com.au/4-digit-field-effect-lcd/p/ZD1886), which is rated for 3V. This was powered directly with the 3.3v GPIO signals, which have slightly too much voltage. It might be better to reduce the voltage to 3V using resistor voltage dividers (the LCD takes very little current), but this would be a lot of messing around. Instead I used some crude pulse width modulation (pwm) from software (crude because the voltage isn't smoothed).


## Setting up the GPIO overlay  
Setting up the GPIO overlay is not a straight forward task. The following websites contain useful information for doing this  
(https://vadl.github.io/beagleboneblack/2016/07/29/setting-up-bbb-gpio)  
(http://derekmolloy.ie/gpios-on-the-beaglebone-black-using-device-tree-overlays/)  
(https://github.com/jadonk/validation-scripts/blob/master/test-capemgr/README.md)  

The custom overlay I have made for this project is `clock.dts`  
I successfully installed the device tree compiler as per the instructions in (https://vadl.github.io/beagleboneblack/2016/07/29/setting-up-bbb-gpio) which worked for my Debian kernel  
`Linux beaglebone 4.1.15-ti-rt-r43 #1 SMP PREEMPT RT Thu Jan 21 20:13:58 UTC 2016 armv7l GNU/Linux`  
  
to compile the overlay  
compile as su: `dtc -O dtb -o clock-00A0.dtbo -b 0 -@ clock.dts`  
then     
`cp clock-00A0.dtbo /lib/firmware/`  
`echo clock > /sys/devices/platform/bone_capemgr/slots`  
to make permanent (survive a reboot) add `clock` to the file `/etc/default/capemgr` formatted as `cape=[comma delimited cape names]`

Once this has been done, if using the C++ `clock.cpp` program, the ports have to be initiated and set for output mode. Because there are so many ports I have provided a script for this `gpio_scr`. You will probably need to give it run permission `chmod +x gpio_scr` then run it from the su account.

## The cpp code
The program which runs the clock is given in clock.cpp. This program is trivial to compile  
`g++ clock.cpp -o clock`.  
Run the program in the background from the su account, e.g. from the program's directory `./clock &`  
The program uses an infinite loop: to terminate it you can find the process id number `ps -A | grep clock` and then use this number `num` with the kill command `sudo kill num`

## The LKM C code
The program may be found in the `lkm` subdirectory. You must have the kernel headers for your kernel installed in order to compile this. On my BBB the following  
`apt-cache search linux-headers-$(uname -r)` output
`linux-headers-4.1.15-ti-rt-r43 - Linux kernel headers for 4.1.15-ti-rt-r43 on armhf`
To compile go into the subdirectory and issue the command `make`  
To load the LKM type  
`sudo insmod kclock.ko`
or if you wish to offset the time for you local time zone, add the seconds (10hrs = 36,000 secs in my case)  
`insmod kclock.ko offset=36000`  
To unload the LKM  
`sudo rmmod kclock`  
The LKM writes to the kernel log. You can monitor this from the su account
`cd /var/log`  
`tail -f kern.log`


## The wiring  
The program makes use of the 23 GPIO ports  
P8-7, P8-8, P8-9, P8-10, P8-11, P8-12, P8-13, P8-14, P8-15, P8-16, P8-18, P8-26  
P9-12, P9-14, P9-15, P9-16, P9-17, P9-18, P9-21, P9-22, P9-23, P9-24, P9-26

There are two ways you could go about wiring the LCD up. 1) you could pair the above nominated ports at random with the terminals on the LCD that need to be connected to a port. You could then turn each of the ports on and off at the command line to establish which port is connected to which LCD component (or manually trace the wires) and rewrite the values given in the array `gpio_map` at the top of `clock.cpp` and `kclock.c`. 2) you could wire up the same pairs as I used, where the digit segment (e.g. 2B) gives the digit number (1 on left of the LCD and 4 on the right) and segment per

<pre>
      A  
     ___  
  F |   | B  
     -G-  
  E |   | C  
     ---  
      D  
</pre>

The table you need to do this is  
Port    Digit segment
P8-7    2B  
P8-8    3G  
P8-9    2A  
P8-10   3F  
P8-11   2F  
P8-12   3A  
P8-13   2G  
P8-14   3B  
P8-15   1B  
P8-16   4G  
P8-18   4F  
P8-26   4A  
P9-12   2E  
P9-14   2D  
P9-15   1C  
P9-16   2C  
P9-17   3D  
P9-18   3E  
P9-21   4E  
P9-22   3C  
P9-23   4C  
P9-24   4D  
P9-26   4B  

