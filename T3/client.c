#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "message.h"
/* I worked on this code with Bahaedddine Abdessalem*/

#define BUFFER_SIZE 1024

int sockfd;
struct sockaddr_in server;
pthread_t send_thread, recv_thread;
char rbuffer[BUFFER_SIZE],sbuffer[BUFFER_SIZE];
socklen_t len = sizeof(server);

void* receive_messages() {
    
    while (1) {
        ssize_t bytes_received = recvfrom(sockfd, rbuffer, BUFFER_SIZE, 0, (struct sockaddr*)&server, &len);
        if (bytes_received < 0) {
            perror("Receive failed");
            break;
        }
        rbuffer[bytes_received] = '\0';
        printf("Received: %s\n", rbuffer);
    }
    return NULL;
}

void* send_messages() {
    while (1) {
        printf("Send: ");
        if (!fgets(sbuffer, BUFFER_SIZE, stdin)) break;
        sbuffer[strcspn(sbuffer, "\n")] = '\0';
        if (sendto(sockfd, sbuffer, strlen(sbuffer), 0, (struct sockaddr*)&server, sizeof(server)) < 0) {
            perror("Send failed");
            break;
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <SERVER_IP> <PORT>\n", argv[0]);
        return 1;
    }

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server.sin_addr);

    printf("Connecting to %s:%d\n", server_ip, port);

    if (pthread_create(&send_thread, NULL, send_messages, NULL) ||
        pthread_create(&recv_thread, NULL, receive_messages, NULL)) {
        perror("Thread creation failed");
        return 1;
    }

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    close(sockfd);
    return 0;
}