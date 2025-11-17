This project is a practical implementation of a DNS spoofing attack, developed as part of the challenge "DNS Hi-Jacking: DNS Spoofing". 

## What This Tool Does

- Monitors DNS queries on the network
- Parses those queries in real time
- Sends back **forged DNS responses** with a hardcoded IP (`1.2.4.3`)
- All before the legitimate DNS server can answer (if you're lucky!)
- Logs everything in `log.txt` for analysis and debugging

## Compilation

Make sure you have `libpcap` installed. Then compile with:

```bash
make
```
## How to Run

After compiling the tool, launch it with:

```bash
sudo ./dns_spoof [target_port]
```
PS: If you don’t specify a port, it defaults to 53 (the standard DNS port)

## How It Works

* First, the program shows a list of available network interfaces, and you choose which one to monitor. Once selected, it sets up a packet sniffer on that interface. If the interface supports it, monitor mode is used. Otherwise, it defaults to promiscuous mode to capture all packets.

* Next, it applies a filter so that it only listens for DNS queries (UDP packets on port 53, or another port if you specify it).

* When a DNS request is spotted, the program grabs it, extracts the domain name being queried, and logs the raw data.

* Then it builds a **fake DNS response** that pretends to come from the real DNS server. This forged response includes:
- The same query ID and domain name
- A hardcoded IP address (we use `1.2.4.3`)
- Manually crafted IP and UDP headers
- Proper checksums to make it look legit

* Finally, it fires off this spoofed packet using a raw socket. The idea is to **beat the real DNS server** to the punch — if the client receives our response first, it’ll trust it and ignore the real one.

