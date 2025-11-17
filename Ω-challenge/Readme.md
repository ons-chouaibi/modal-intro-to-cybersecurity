# Switch-Kill-Switch: Kill-Kill-Kill Challenge

## Overview

TThe objective of the challenge was to implement an attack that forces a network switch to behave like a hub, by flooding it with Layer 2 (Ethernet) frames with spoofed source MAC addresses. 

The goal is to exhaust the switch's MAC address table and thereby:

- Make the switch broadcast all traffic (hub-like behavior), or
- Crash the switch (common in consumer-grade hardware), or
- Trigger a port shutdown if MAC flooding defense mechanisms are in place.

## Files

- `attack.c`: C program to generate and send Ethernet frames with random source MAC addresses.
- `Makefile`: Simplifies compilation of the attack program.

## Requirements

- Linux environment with root privileges.
- Raw socket access (run with `sudo`).
- Networking libraries:
  - `<net/if.h>`, `<netinet/ether.h>`, `<linux/if_packet.h>`, etc.

## Usage

```bash
make           # Compile the attack program
sudo ./attack <interface> <num_packets>
```
* <interface>: The name of the network interface (e.g., eth0, enp3s0).

* <num_packets>: Total number of packets to send in the flooding attack.

## Example
```bash
sudo ./attack eth0 5000
```
This command will send 5000 spoofed Ethernet frames through interface eth0.

## How It Works

The attack follows this logic:

1. A raw socket is created using the `AF_PACKET` family to allow crafting Ethernet frames.
2. The socket is bound to the specified network interface.
3. An Ethernet frame is prepared for each iteration with:
   - **Destination MAC**: A broadcast address.
   - **Source MAC**: Randomly generated (to flood the switch's MAC table).
   - **EtherType**: Set to IPv4 (0x0800).
   - **Payload**: A string like `"FLOOD_<packet_number>"` to fill the frame.
4. The frame is sent using `sendto()` system call.
5. Progress is printed every 100 packets.
6. On interruption (Ctrl+C), the program gracefully cleans up the socket and exits.

This approach aims to overflow the MAC address table of the switch, exploiting its limited memory and causing one of the following behaviors:
- It starts broadcasting all traffic (hub behavior).
- It crashes.
- It shuts down the port temporarily (if port security is enabled).

---

## Disclaimer

I did not have the opportunity to test this code on a physical switch.

- I do not currently own a physical switch.
- I started configuring a virtual switch for testing but ran out of time to complete it.

As such, while the logic and compilation of the code have been carefully implemented and verified, **the runtime behavior on actual hardware has not been validated**.