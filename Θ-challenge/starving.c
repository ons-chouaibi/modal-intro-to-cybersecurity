#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <net/if.h>
#include "dhcp.h"

void generate(char  mac[6])
 {
    srand(time(NULL));
    for (int i = 0; i < 6; ++i) {
        mac[i] = (unsigned char)(rand() % 256);
    }

 }
 void makediscovery (dhcp_pack * dh , char * mac )
 {
    memset(dh,0,sizeof(dhcp_pack));
    dh->op=1;
    dh->htype=1 ; 
    dh->hlen=6;
    dh->hops=0;
    dh->xid=htonl(rand());
    memcpy(dh->chaddr,mac,6);
    dh->magic=htonl(0x63825363);
    uint8_t *opt = dh->options;


*opt++ = 53; *opt++ = 1; *opt++ = 1;
*opt++ = 255;

 }
uint8_t * option (uint8_t* opt)
{
    char * c =opt;
    while (*opt!=54)
    {
        c++;
        opt++;
    }
    opt+=2;
    return opt ;
}
void makerequest(dhcp_pack *offer , dhcp_pack *request )
{
    memset(request,0,sizeof(dhcp_pack));
    request->op=1;
    request->htype=1;
    request->hlen=6;
    request->hops=0;
    request->xid = offer->xid;
    request->flags=0x8000;
    memcpy(request->chaddr,offer->chaddr,6);
    request->magic=offer->magic;
    uint8_t * opt = request->options;
    *opt++=53;
    *opt++=1;
    *opt++=3;
    *opt++=50;
    *opt++=4;
    memcpy(opt,&(offer->yiaddr),4);
    opt+=4;
    *opt++=54;
    *opt++=4;
    memcpy(opt,option(offer->options),4);
    *opt+=4;
    *opt++=61;
    *opt++=7;
    *opt++=1;
    memcpy(opt,offer->chaddr,6);
    opt+=6;
    *opt=255;
}
 int main(void)
 {
    int mysocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    int yes = 1 ; 
    struct sockaddr_in servsock;

    memset(&servsock,0,sizeof(servsock));
    servsock.sin_family=AF_INET;
    servsock.sin_port=htons(68);
    servsock.sin_addr.s_addr=inet_addr("0.0.0.0");

    if ( setsockopt(mysocket,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(yes))<0)
    {
        printf("broadcast error\n ");
        return -1;
    }
    if ( setsockopt(mysocket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))<0)
    {
        printf(" socket error \n");
        return -1;
    }
    if(bind(mysocket,(struct sockaddr *) &servsock,sizeof(servsock)))
    {
        fprintf(stderr,"Bind error\n %s error no : %d",strerror(errno),errno);
        return -1;
    }
    printf("Bind done \n");
    struct sockaddr_in serv;
    memset(&serv,0,sizeof(serv));
    serv.sin_addr.s_addr=htonl(INADDR_BROADCAST);
    serv.sin_port=htons(67);
    serv.sin_family=AF_INET;
    while(1)
    {
        char mac[6];
        dhcp_pack *discover=malloc(sizeof(dhcp_pack));
        dhcp_pack *request=malloc(sizeof(dhcp_pack));
        generate(mac);
        makediscovery(discover,mac);
        if(sendto(mysocket,discover,sizeof(dhcp_pack),0,(struct sockaddr *) &serv,sizeof(serv))<0)
        {
            fprintf(stderr,"Bind problem\n %s error no : %d",strerror(errno),errno);
            return -1;
        }
        printf("Bind done \n");
        usleep(100000);
        if(recvfrom(mysocket,discover,sizeof(dhcp_pack),0,NULL,NULL)<0)
        {
            printf("receive error \n");
            return -1;
        }
        printf("received Offer \n");
        printf("Sent DHCP Discover with MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        makerequest(discover,request);
        
        if(sendto(mysocket,request,sizeof(dhcp_pack),0,(struct sockaddr *) &serv,sizeof(serv))<0)
        {
            printf("send error \n");
            return -1;
        }
        printf("the request is sent \n");
        usleep(100000);
    }
}