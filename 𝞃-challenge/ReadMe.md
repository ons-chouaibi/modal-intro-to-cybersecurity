## Tcp Hijacking challenge (Part 1)
This code implements the **first part** of the TCP hijacking challenge: performing a TCP Reset (RST) attack.

The code listens for TCP traffic on a specified **target port**, and as soon as it detects a connection attempt, it sends multiple **RST packet** back to the client until the connection is forcefully closed.




## Usage:
1. **Compile the program**  compile with make
2. **Run the program as root** sudo ./rst_attack
3. **Input port target**
4. **Stop with Ctrl+C** (otherwise the program will continue running)


## Tcp Hijacking challenge (Part 2)
This code implements the complete TCP hijacking attack: connection takeover with command injection.

The program monitors TCP traffic between a victim and server, captures sequence numbers, sends RST to the victim, then impersonates the victim to inject pre-configured commands into the hijacked connection.

## Usage:
1. **Compile the program**  compile with make
2. **Run the program as root** sudo ./rst_attack
3. **Input victim's ip,and the server's ip and port**
4. **Input the commands** : hit enter if you are done with the commands you want to send after the hijack
