#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>

#define BUFFER_SIZE 2024

int main (int argc, char *argv[]){
    char *msg=malloc(BUFFER_SIZE);
    if (!msg) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    
    int sockfd=-1;
    struct sockaddr_in server;

    if (argc!=3){
        fprintf(stderr,"Usage: %s IP_ADDRESS PORT_NUMBER\n", argv[0]);
        free(msg);
        exit(EXIT_FAILURE);
    }
    
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if (sockfd<0){
        perror("Socket creation failed");
        free(msg);
        exit(EXIT_FAILURE);
    }
    
    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(atoi(argv[2]));
    
    if(inet_pton(AF_INET,argv[1],&server.sin_addr)<=0){
        perror("invalid address");
        close(sockfd);
        free(msg);
        exit(EXIT_FAILURE);
    }
    
    while(1){
        printf("Send: ");
        if (fgets(msg, BUFFER_SIZE, stdin)==NULL){
            break;
        }
        
        ssize_t sent = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&server, sizeof(server));
        if (sent < 0) {
            perror("sendto failed");
        }
    }
    
    close(sockfd);
    free(msg);
    return 0;
}