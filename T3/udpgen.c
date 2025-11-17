#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#define DEFAULT_TARGET "127.0.0.1"
#define DEFAULT_PORT 53
#define DEFAULT_INTERVAL 1
#define DEFAULT_SIZE 64

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -t TARGET   Target IP address (default: %s)\n", DEFAULT_TARGET);
    printf("  -p PORT     Target port (default: %d)\n", DEFAULT_PORT);
    printf("  -i INTERVAL Interval between packets in seconds (default: %d)\n", DEFAULT_INTERVAL);
    printf("  -s SIZE     Payload size in bytes (default: %d)\n", DEFAULT_SIZE);
    printf("  -h          Display this help message\n");
}

int main(int argc, char *argv[]) {
    // Default settings
    char target_ip[16] = DEFAULT_TARGET;
    int target_port = DEFAULT_PORT;
    int interval = DEFAULT_INTERVAL;
    int payload_size = DEFAULT_SIZE;
    
    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "t:p:i:s:h")) != -1) {
        switch (opt) {
            case 't':
                strncpy(target_ip, optarg, sizeof(target_ip) - 1);
                break;
            case 'p':
                target_port = atoi(optarg);
                break;
            case 'i':
                interval = atoi(optarg);
                break;
            case 's':
                payload_size = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        return 1;
    }
    
    // Configure target address
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    
    if (inet_pton(AF_INET, target_ip, &target_addr.sin_addr) <= 0) {
        perror("invalid address");
        close(sock);
        return 1;
    }
    
    // Allocate memory for payload
    char *payload = (char *)malloc(payload_size);
    if (payload == NULL) {
        perror("memory allocation failed");
        close(sock);
        return 1;
    }
    
    // Fill payload with random data
    srand(time(NULL));
    for (int i = 0; i < payload_size; i++) {
        payload[i] = rand() % 256;
    }
    
    // Set up signal handler for clean termination
    signal(SIGINT, handle_sigint);
    
    printf("Starting UDP traffic generator to %s:%d\n", target_ip, target_port);
    printf("Interval: %d seconds, Payload size: %d bytes\n", interval, payload_size);
    printf("Press Ctrl+C to stop\n");
    
    // Run as a daemon process
    if (daemon(0, 1) < 0) {
        perror("daemon creation failed");
        free(payload);
        close(sock);
        return 1;
    }
    
    // Counter for sent packets
    unsigned long counter = 0;
    time_t last_report = time(NULL);
    
    // Main loop
    while (keep_running) {
        // Send packet
        if (sendto(sock, payload, payload_size, 0, 
                  (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
            perror("sendto failed");
            break;
        }
        
        counter++;
        
        // Print status every 10 seconds
        time_t now = time(NULL);
        if (now - last_report >= 10) {
            printf("Sent %lu packets so far\n", counter);
            last_report = now;
        }
        
        // Wait for the specified interval
        sleep(interval);
    }
    
    printf("Stopped after sending %lu packets\n", counter);
    
    // Clean up
    free(payload);
    close(sock);
    
    return 0;
}
