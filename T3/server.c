#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "message.h"
/* I worked on this code with Bahaedddine Abdessalem*/


#define BUFFER_SIZE 1024

int server_port;
int max_concurrent;
unsigned int delay;
pthread_mutex_t mutex;
pthread_cond_t cond;

int active_connections = 0;

void* handle_connection(void* arg) {
    int sockfd = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    ssize_t received_bytes = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client, &len);
    if (received_bytes < 0) {
        perror("recieve failed");
        close(sockfd);
        pthread_exit(NULL);
    }
    usleep(delay);
    buffer[received_bytes] = '\0';
    
    printf("Received : %s",  buffer);

    if (sendto(sockfd, buffer, received_bytes, 0, (struct sockaddr *)&client, len) <0){
        perror("send failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Update connection count
    pthread_mutex_lock(&mutex);
    active_connections--;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server;

    max_concurrent = 5;
    delay = 1000000;  

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <PORT> [MAX_CONNECTIONS] [DELAY_MS]\n", argv[0]);
        return 1;
    }
    server_port = atoi(argv[1]);

    if (argc >= 3) max_concurrent = atoi(argv[2]);
    if (argc >= 4) delay = atoi(argv[3]) * 1000;

    printf("Starting server on port %d (Max connections: %d, Delay: %dμs)\n",
          server_port, max_concurrent, delay);

    if (pthread_mutex_init(&mutex, NULL) ||
        pthread_cond_init(&cond, NULL)) {
        perror("Sync initialization failed");
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(server_port);

    if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Binding failed");
        close(sockfd);
        return 1;
    }

    while (1) {
        int* sock_ptr = malloc(sizeof(int));
        if (sock_ptr == NULL) {
            perror("Memory allocation failed");
            close(sockfd);
            return 1;
        }
        *sock_ptr = sockfd;

        pthread_mutex_lock(&mutex);
        while (active_connections >= max_concurrent) {
            pthread_cond_wait(&cond, &mutex);
        }
        active_connections++;
        pthread_mutex_unlock(&mutex);

        pthread_t connection_thread;
        if (pthread_create(&connection_thread, NULL, handle_connection, sock_ptr)) {
            perror("Thread creation error");
            close(sockfd);
            free(sock_ptr);
            
            pthread_mutex_lock(&mutex);
            active_connections--;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
            
            continue;
        }
        pthread_detach(connection_thread);
    }

    close(sockfd);
    return 0;
}