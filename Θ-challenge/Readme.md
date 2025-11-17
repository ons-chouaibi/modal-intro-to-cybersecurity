# Θ-Challenge: DHCP Starvation & Spoofing

## Objective

This challenge demonstrates two core attack vectors on DHCP-based networks:

- **DHCP Starvation**: Exhausting the IP lease pool of a legitimate DHCP server using fake requests.
- **DHCP Spoofing**: Deploying a rogue DHCP server to assign malicious configurations to clients.



## Core Components

### `starving.c`: DHCP Starvation Attack

- Continuously sends DHCP DISCOVER and REQUEST packets with **random MAC addresses**.
- Goal: consume all available IPs on the legitimate DHCP server, preventing clients from getting valid leases.

### `dhcp.c`: Rogue DHCP Server

- Listens for **DHCP DISCOVER** messages from clients.
- Sends forged **DHCPOFFER** and **DHCPACK** responses, offering attacker-controlled IP, DNS, and gateway.
- Redirects victims to the attacker's infrastructure (e.g., a malicious DNS or web server).

## Building the Code

```bash
make
```
## Execution Steps

### 1. DHCP Starvation

Start by flooding the real DHCP server with bogus MAC addresses:

```bash
sudo ./starving
```
You should see output like:
```
it has been sent
REceived Offer
Sent DHCP Discover with MAC: aa:bb:cc:dd:ee:ff
...
```

This prevents the real DHCP server from issuing leases to legitimate clients.

---

### 2. Start the Rogue DHCP Server

In another terminal:
```bash
sudo ./dhcp
```
* Example output:

-1648239094
1
-1648239094
1
...

Each line represents a received `DHCPDISCOVER` and the offered spoofed response.

---

## How to Test

1. Use a second device (e.g., phone or tablet).
2. Forget and reconnect to the Wi-Fi network.
3. Your rogue server should assign:
    - An IP address (e.g., `172.20.10.100`)
    - Malicious DNS / gateway (defined in `dhcp.h`)


