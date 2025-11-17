/*
 * dns_server.c
 *
 *  Created on: Apr 26, 2016
 *      Author: jiaziyi
 */


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdbool.h>
#include<time.h>

#include "dns.h"

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr server;

	char *dns_server = "172.18.241.118";
	int port = 53; //the default port of DNS service
	FILE * pattern_file = NULL;

	//to keep the information received.
	res_record answers[ANS_SIZE], auth[ANS_SIZE], addit[ANS_SIZE];
	query queries[ANS_SIZE];


	if(argc == 2)
	{
		port = atoi(argv[1]); //if we need to define the DNS to a specific port
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	int enable = 1;

	if(sockfd <0 )
	{
		perror("socket creation error");
		exit_with_error("Socket creation failed");
	}

	//in some operating systems, you probably need to set the REUSEADDR
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	{
	    perror("setsockopt(SO_REUSEADDR) failed");
	}

	//for v4 address
	struct sockaddr_in *server_v4 = (struct sockaddr_in*)(&server);
	server_v4->sin_family = AF_INET;
	server_v4->sin_addr.s_addr = htonl(INADDR_ANY);
	server_v4->sin_port = htons(port);

	//bind the socket
	if(bind(sockfd, &server, sizeof(*server_v4))<0){
		perror("Binding error");
		exit_with_error("Socket binding failed");
	}

	printf("The dns_server is now listening on port %d ... \n", port);
	//print out
	uint8_t buf[BUF_SIZE], send_buf[BUF_SIZE]; //receiving buffer and sending buffer
	struct sockaddr remote;
	int n;
	socklen_t addr_len = sizeof(remote);
	struct sockaddr_in *remote_v4 = (struct sockaddr_in*)(&remote);

	while(1){
		//an infinite loop that keeps receiving DNS queries and send back a reply
		//complete your code here
		
        recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*)&remote, &addr_len);
        memcpy(send_buf, buf, BUF_SIZE);
        
        dns_header *dns = (dns_header*)send_buf;
        dns->qr = 1;           
        dns->an_count = htons(1);  
        
        uint8_t *p = send_buf + sizeof(dns_header);
        while(*p != 0) p += *p + 1;  
        p++;  
        p += sizeof(question);  
        
        *p++ = 0xc0; *p++ = 0x0c;           
        *((uint16_t*)p) = htons(TYPE_A); p += 2;     
        *((uint16_t*)p) = htons(CLASS_IN); p += 2;   
        *((uint32_t*)p) = htonl(300); p += 4;       
        *((uint16_t*)p) = htons(4); p += 2;        
        
        char *uip = "93.184.216.34"; 
        inet_pton(AF_INET, uip, p); p += 4;
        
        sendto(sockfd, send_buf, p - send_buf, 0, (struct sockaddr*)&remote, addr_len);

        printf("Query answered with %s\n", uip);
	}
	return 0;
}