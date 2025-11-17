** μ-challenge: Vulnerable Applications
This repository contains the code for the μ-challenge.

** Requirements

-VirtualBox
-The VM image provided with the challenge


*Start the VM (no login is necessary)

**The Challenge

*Files:

attack.c - Code that configures SSH access on the target system
hack.c - Code that connects to the vulnerable application and sends custom commands for name addition
Makefile - Builds the attack and hack programs

*Usage:
-Use the hack.c program to craft and send malicious input (you can personalize the command you send in the program)
-Use the attack.c programm to install ssh configuration on the machine and add an ssh key

Running the Exploit

Compile the code:
make

Run the attack program:
./attack (or ./hack)

Ps: for the ./attack program you do not need to modify the code, just run the pogram and it will ask you for your public ssh key, and will add it on its own.