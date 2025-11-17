#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>

#define BUFFER_SIZE 2024

int main(int argc, char *argv[]){
    if (argc!=2){
        fprintf(stderr,"Usage: %s PORT_NUMBER\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int sockfd=-1;
    char *buffer=malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server, client;
    socklen_t len=sizeof(client);

    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0){
        perror("Socket creation failed");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    memset(&server,0,sizeof(server));
    memset(&client,0,sizeof(client));

    server.sin_family=AF_INET;
    server.sin_port=htons(atoi(argv[1]));
    server.sin_addr.s_addr=INADDR_ANY;

    if (bind(sockfd,(const struct sockaddr *)&server,sizeof(server))<0){
        perror("bind failed");
        close(sockfd);
        free(buffer);
        exit(EXIT_FAILURE);
    }
    
    while(1){
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0, (struct sockaddr *)&client, &len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }
        buffer[n]='\0';
        printf("Received: %s\n", buffer);
        
        ssize_t sent = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&client, len);
        if (sent < 0) {
            perror("sendto failed");
        }
    }
    
    close(sockfd);
    free(buffer);
    return 0;
}