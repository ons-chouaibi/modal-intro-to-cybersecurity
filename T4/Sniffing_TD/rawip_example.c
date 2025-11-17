/*
 * rawip_example.c
 *
 *  Created on: May 4, 2016
 *      Author: jiaziyi
 */


 #include<stdio.h>
 #include<string.h>
 #include<sys/socket.h>
 #include<stdlib.h>
 #include<netinet/in.h>
 #include<arpa/inet.h>
 #include<unistd.h>
 
 #include "header.h"
 
 #define SRC_IP  "192.168.1.111" //set your source ip here. It can be a fake one
 #define SRC_PORT 54321 //set the source port here. It can be a fake one
 
 #define DEST_IP "129.104.89.108" //set your destination ip here
 #define DEST_PORT 5555 //set the destination port here
 #define TEST_STRING "test data" //a test string as packet payload
 
 int main(int argc, char *argv[])
 {
	 char source_ip[] = SRC_IP;
	 char dest_ip[] = DEST_IP;
 
 
	 int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
 
	 int hincl = 1;                  /* 1 = on, 0 = off */
	 setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));
 
	 if(fd < 0)
	 {
		 perror("Error creating raw socket ");
		 exit(1);
	 }
 
	 char packet[65536], *data;
	 char data_string[] = TEST_STRING;
	 memset(packet, 0, 65536);
 
	 //IP header pointer
	 struct iphdr *iph = (struct iphdr *)packet;
 
	 //UDP header pointer
	 struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));
	 struct pseudo_udp_header psh; //pseudo header
 
	 //data section pointer
	 data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
 
	 //fill the data section
	 strncpy(data, data_string, strlen(data_string));
 
	 //fill the IP header here
	 iph->ihl=5;
	 iph->version=4;
	 iph->tos=0;
	 iph->tot_len=htons(sizeof(struct iphdr)+sizeof(struct udphdr)+strlen(data_string));
	 iph->ttl=64;
	 iph->protocol=IPPROTO_UDP;
	 iph->check=0;
	 iph->saddr=inet_addr(source_ip);
	 iph->daddr=inet_addr(dest_ip);
	 iph->check=checksum((unsigned short *)packet, iph->ihl*4);
 
 
	 //fill the UDP header
	 udph->source=htons(SRC_PORT);
	 udph->dest=htons(DEST_PORT);
	 udph->len=htons(sizeof(struct udphdr)+strlen(data_string));
	 udph->check=0;
 
	 psh.source_address=inet_addr(source_ip);
	 psh.dest_address=inet_addr(dest_ip);
	 psh.placeholder=0;
	 psh.protocol=IPPROTO_UDP;
	 psh.udp_length=htons(sizeof(struct udphdr)+strlen(data_string));
 
	 //this part was inspired from https://stackoverflow.com/questions/3845106/how-to-build-a-tcp-pseudo-header-and-related-data-for-checksum-verification-in-c
	 int pseudo_len = sizeof(psh) + sizeof(struct udphdr) + strlen(data_string);
	 char *pseudo_buf = malloc(pseudo_len);
 
	 memcpy(pseudo_buf, &psh, sizeof(psh));
	 memcpy(pseudo_buf + sizeof(psh), udph, sizeof(struct udphdr));
	 memcpy(pseudo_buf + sizeof(psh) + sizeof(struct udphdr), data, strlen(data_string));
 
	 udph->check = checksum((unsigned short*)pseudo_buf, pseudo_len);
	 free(pseudo_buf);
	 
	 //send the packet
	 struct sockaddr_in addr;
	 addr.sin_family=AF_INET;
	 addr.sin_port=htons(DEST_PORT);
	 addr.sin_addr.s_addr=inet_addr(dest_ip);
	 if (sendto(fd,packet,ntohs(iph->tot_len), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		 perror("sendto() failed");
	 } else {
		 printf("Packet sent successfully.\n");
	 }
 
	 close(fd);
 
 
	 return 0;
 
 }
 