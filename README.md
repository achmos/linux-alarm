linux-alarm
===========

A Linux kernel module that sets up some psuedo-files in /proc and /sys that contains the current time, as well as a kernel alarm. Comes with a user space interface program. 

Disclaimer:
===========
This code was made for a class assignment for a Unix Programming course. If you are taking this same course or one similar, please do not use this code, as it would be considered plagiarism. Once the course is finished, feel free to look at my code to compare and contrast as you wish.



This code was written and tested on a Gentoo Linux Virtual Machine

Files
-----

There are two files and two makefiles. 
myclock.c: This is the kernel module code
hw7.c: This is the user space interface program. 

The makefiles in each respective directory are the files that they share the directory with. 

In order to compile and run the user space program, you must have X11 and GNOME installed on your system. 
