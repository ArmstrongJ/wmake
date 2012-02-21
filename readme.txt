Open Watcom Make 1.9 
====================
This project contains the source code for Open Watcom's powerful make tool (wmake) that has been severed from the Open Watcom build system.  The purpose of this "fork" is to allow advancements in wmake without the need to work within Open Watcom's complicated build system.

The goal is to allow building of i386 GNU/Linux and i386 Windows NT wmake targets.  Currently only building GNU/Linux target is working.

Requirements
============
To build this project, the following are necessary:

    * GNU make
    * GCC
    
or, alternatively,

    * Open Watcom
    
Quick Start
===========
From the top-level directory, execute:
    
    make -f gnumake
    
to build with GNU tools.  To build with the Open Watcom package on GNU/Linux, execute:

    wmake -f makefile.linux
    
The final product, the wmake executable, should reside in the linux386 directory.

Licensing
=========
The source code is licensed under the Sybase Open Watcom Public License version 1.0.  The full text of said license can be found in license.txt.
