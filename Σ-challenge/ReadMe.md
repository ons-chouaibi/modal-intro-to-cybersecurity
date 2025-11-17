# TCP SYN Flooding Tool

This repository contains a TCP SYN flooding tool. The tool sends a flood of TCP SYN packets to a target server, gradually increasing the packet rate until the server becomes unresponsive.

## Features

- Automatically discovers the minimum packets-per-second (PPS) needed to cause denial of service
- Real-time connection testing to verify attack effectiveness
- Gradually increases attack intensity to find the exact DoS threshold
- Randomizes source IP addresses to bypass simple filtering mechanisms
- Multi-threaded design for efficient packet generation and connection testing

## Code Structure

### Functions

- `checksum()`: Calculates the checksum for TCP/IP headers
- `generate_random_ip()`: Creates randomized source IP addresses for SYN packets
- `signal_handler()`: Handles interruption signals (Ctrl+C)
- `syn_flood()`: Main attack function that crafts and sends SYN packets
- `test_connection()`: Tests if the target is still accepting connections
- `main()`: Coordinates threads and manages the attack process

### Key Components

#### Packet Crafting
The tool manually creates IP and TCP headers using raw sockets. It populates all necessary fields, including:
- IP version, header length, TTL
- TCP flags (SYN flag set)
- Sequence numbers
- Window size
- Checksums for both IP and TCP headers

#### Attack Strategy
1. Starts at a low packet rate (100 PPS)
2. Increases rate by 10 PPS every 5 seconds
3. Simultaneously tests connection to the target
4. When connection fails repeatedly, identifies the DoS threshold
5. Continues the attack at the threshold rate

#### Connection Testing
- Uses non-blocking sockets with timeouts to test connections
- Requires multiple consecutive failures to confirm DoS
- Ensures stability by continuing tests for 5 seconds after initial failure

## Usage

### Compilation

```bash
make
```

### Running the Attack

```bash
sudo ./syn_flood <target_ip> <target_port>
```


Note: Root privileges are required for raw socket operations.

### Output

The tool provides real-time status updates:
- Number of packets sent
- Current packet rate
- Time elapsed
- Connection status

When the DoS threshold is discovered, the tool will display the minimum PPS required to maintain the denial of service.
