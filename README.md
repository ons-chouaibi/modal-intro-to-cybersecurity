# The Hacking Xperience - Cybersecurity Challenges

> *"To know how to secure something, you first need to know how to break it."*

A collection of practical cybersecurity challenges completed as part of the **CSC_43M05_EP: Modal d'informatique - Cybersecurity** course at École Polytechnique. This repository demonstrates hands-on experience with network attacks, exploitation techniques, and offensive security concepts.

## 🎯 Course Overview

This course focuses on understanding cybersecurity from an offensive perspective. Through a series of tutorials and challenges, I explored how various attacks work and implemented them from scratch using low-level programming techniques. Each challenge required independent thinking, code development, and a deep understanding of networking protocols and system vulnerabilities.

## 🛠️ Technologies & Skills

- **Languages**: C, Assembly (x86-64)
- **Libraries**: libpcap, raw sockets, packet crafting
- **Concepts**: Network protocols (TCP/IP, UDP, DNS, DHCP), buffer overflow exploitation, memory corruption, Layer 2 attacks
- **Tools**: GDB debugger, Wireshark, VirtualBox, Makefile

## 📂 Completed Challenges

### 1. DNS Spoofing Attack
**Challenge**: δ-challenge 

A DNS hijacking tool that intercepts DNS queries and responds with forged answers before the legitimate DNS server can reply.

**Key Features**:
- Real-time packet sniffing with libpcap
- Custom DNS response crafting
- Race condition exploitation (speed-based attack)
- Network interface monitoring (promiscuous/monitor mode)

**Technical Implementation**:
- Captures UDP packets on port 53
- Extracts queried domain names
- Forges DNS responses with spoofed IP addresses
- Calculates proper checksums for packet validity


---

### 2. TCP SYN Flood (DoS Attack)
**Challenge**: ν-challenge

An adaptive SYN flooding tool that automatically determines the minimum packet rate required to cause denial of service on a target server.

**Key Features**:
- Intelligent rate discovery algorithm
- Multi-threaded architecture (attack + connection testing)
- Source IP randomization to bypass filtering
- Real-time DoS threshold detection

**Technical Implementation**:
- Raw socket programming for custom packet crafting
- Manual IP and TCP header construction
- Incremental PPS (packets per second) increase
- Non-blocking connection verification


---

### 3. TCP Connection Hijacking (2-Part Challenge)
**Challenge**: 𝞃-challenge

A two-stage TCP hijacking attack: first disrupting an existing connection, then taking it over to inject malicious commands.

**Part 1 - TCP Reset Attack**:
- Monitors TCP traffic on specified ports
- Injects RST packets to forcefully terminate connections
- Real-time packet interception

**Part 2 - Connection Takeover**:
- Captures TCP sequence numbers from legitimate traffic
- Sends RST to disconnect the victim
- Impersonates victim to inject commands into the server

**Technical Implementation**:
- Packet sniffing and parsing
- Sequence number prediction
- Session state tracking
- Command injection payload delivery


---

### 4. DHCP Starvation & Rogue Server
**Challenge**: Θ-challenge

A dual-phase attack targeting DHCP infrastructure: exhausting the legitimate server's IP pool, then deploying a rogue DHCP server.

**Phase 1 - DHCP Starvation**:
- Floods DHCP server with DISCOVER/REQUEST packets
- Uses randomized MAC addresses
- Exhausts available IP lease pool

**Phase 2 - DHCP Spoofing**:
- Deploys rogue DHCP server
- Intercepts client DISCOVER messages
- Provides malicious network configuration (DNS, gateway, IP)

**Attack Impact**:
- Denies network access to legitimate clients
- Redirects traffic through attacker-controlled infrastructure
- Enables Man-in-the-Middle attacks


---

### 5. Buffer Overflow Exploitation
**Challenge**: μ-challenge (Vulnerable Applications)

Exploits memory corruption vulnerabilities to achieve arbitrary code execution on a vulnerable application running in a VM.

**Key Techniques**:
- Stack buffer overflow exploitation
- Return address overwriting (RIP hijacking)
- Shellcode injection (execve shell spawn)
- Offset calculation using GDB

**Technical Implementation**:
- Determined buffer overflow offset (140 bytes)
- Crafted NOP sled + shellcode payload
- Overwrote return address with buffer location
- Achieved shell access on target VM

**Additional Attack Vector**:
- SSH configuration manipulation
- Public key injection for persistent access


---

### 6. MAC Flooding (Switch-to-Hub Attack)
**Challenge**: Ω-challenge

Forces a network switch to behave like a hub by overwhelming its MAC address table, causing it to broadcast all traffic.

**Key Features**:
- Layer 2 (Ethernet frame) manipulation
- Random MAC address generation
- CAM table overflow exploitation

**Technical Implementation**:
- Raw socket (AF_PACKET) for Ethernet frame crafting
- Spoofed source MAC addresses
- High-volume frame transmission
- Graceful interrupt handling

**Potential Outcomes**:
- Switch enters failopen mode (broadcasts traffic)
- Port security triggers and shuts down the port
- Device crash (consumer-grade switches)


---

## 🧪 Testing Environment

All challenges were developed and tested in controlled environments:
- **VirtualBox VMs** for isolated vulnerability testing
- **Local network segments** for protocol attacks
- **Packet capture analysis** with Wireshark for verification

## ⚠️ Ethical Disclaimer

This repository is for **educational purposes only**. All code was developed as part of an academic cybersecurity course under controlled conditions. 

**Do not use these tools for:**
- Unauthorized access to systems or networks
- Disrupting services without explicit permission
- Any illegal activities

Always obtain proper authorization before performing security testing.

## 🏗️ Building & Running

Most challenges include a Makefile for easy compilation:

```bash
# General pattern
cd <challenge-directory>
make
sudo ./<binary> [arguments]
```

**Note**: Most tools require root privileges due to raw socket operations and low-level network access.

## 📚 Learning Outcomes

Through these challenges, I gained practical experience in:

1. **Network Protocol Analysis**: Deep understanding of TCP/IP, DNS, DHCP operation
2. **Offensive Security Mindset**: Thinking like an attacker to understand vulnerabilities
3. **Low-Level Programming**: Raw sockets, packet crafting, memory manipulation
4. **Exploitation Techniques**: Buffer overflows, race conditions, protocol weaknesses
5. **System Debugging**: Using GDB, analyzing core dumps, reverse engineering

## 🎓 Course Information

- **Institution**: École Polytechnique (X)
- **Course**: CSC_43M05_EP - Modal d'informatique - Cybersecurity
- **Format**: Tutorials + Independent Challenges
- **Approach**: Hands-on, research-oriented problem solving

## 📝 License

This code is shared for educational reference. Please respect academic integrity policies if you're taking a similar course.

---

**Contact**: For questions about specific implementations or approaches, feel free to open an issue.

*"The best defense is a good offense... understanding."*
