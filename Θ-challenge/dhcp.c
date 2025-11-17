#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "dhcp.h"

void makeoffer(dhcp_pack * offer, dhcp_pack* Discover)
{
    memset(offer,0,sizeof(dhcp_pack));
    offer->op=2;
    offer->htype=1;
    offer->hlen=6;
    offer->hops=0;
    offer->xid=Discover->xid;
    offer->secs=0;
    offer->flags=Discover->flags;
    memcpy(offer->chaddr,Discover->chaddr,sizeof(Discover->chaddr));
    offer->yiaddr.s_addr=inet_addr(CLIENTIP);
    offer->siaddr.s_addr=inet_addr(MYIP);
    offer->magic=htonl(0x63825363);
    char * opt = offer->options;
    *opt++=53;
    *opt++=1;
    *opt++=2;
    *opt++=54;
    *opt++=4;
    memcpy(opt,&(offer->siaddr.s_addr),4);//change this once you have the real ip adress to change
    // and also the shit in dhcp.h cause
    // I really can't be bothere to look for it right now
    opt+=4; 
    *opt++=51;
    *opt++=4;
    uint32_t lease=htonl(3600);
    memcpy(opt,&lease,4);
    opt+=4;
    *opt++=1;
    *opt++=4;
    *opt++=255;
    *opt++=255;
    *opt++=255;
    *opt++=0;
    //check this one to because again
    // I really cant be fucking bothered 
    
    
    *opt++=3;
    *opt++=4;
//that too by the way make sure it is the right one or it won't work 
    //and it will start bitching 
   
    memcpy(opt,&(offer->siaddr),4);
    opt+=4;
//change it to the good DNS okay thank you veru much 
    *opt++=6;
    *opt++=4;
    memcpy(opt,&(offer->siaddr),4);
    opt+=4;
    *opt++=255;

}
void dhcp_ack(dhcp_pack* dhcp, dhcp_pack* dhcp_ack){
    dhcp_ack->op = 2;
    dhcp_ack->htype = 1;
    dhcp_ack->hlen = 6;
    dhcp_ack->hops = 0;
    dhcp_ack->xid = dhcp->xid;
    dhcp_ack->secs = 0;
    dhcp_ack->flags = 0;
    dhcp_ack->ciaddr.s_addr = 0;
    dhcp_ack->yiaddr.s_addr = inet_addr(CLIENTIP);
    dhcp_ack->siaddr.s_addr = inet_addr(MYIP);
    dhcp_ack->giaddr.s_addr = 0;
    memcpy(dhcp_ack->chaddr, dhcp->chaddr, 6);
    memset(dhcp_ack->chaddr + 6, 0, 10);
    memset(dhcp_ack->sname, 0, 64);
    memset(dhcp_ack->file, 0, 128);
    dhcp_ack->magic = htonl(0x63825363);
    dhcp_ack->options[0] = 53;
    dhcp_ack->options[1] = 1;
    dhcp_ack->options[2] = 5;
    dhcp_ack->options[3] = 54;
    dhcp_ack->options[4] = 4;
    memcpy(dhcp_ack->options + 5, &dhcp_ack->siaddr.s_addr, 4);
    dhcp_ack->options[9] = 51;
    dhcp_ack->options[10] = 4;
    dhcp_ack->options[11] = 0;
    dhcp_ack->options[12] = 0;
    dhcp_ack->options[13] = 3;
    dhcp_ack->options[14] = 255;
    dhcp_ack->options[15] = 1;
    dhcp_ack->options[16] = 4;
    dhcp_ack->options[17] = 255;
    dhcp_ack->options[18] = 255;
    dhcp_ack->options[19] = 255;
    dhcp_ack->options[20] = 0;
    dhcp_ack->options[21] = 3;
    dhcp_ack->options[22] = 4;
    dhcp_ack->options[23] =172;
    dhcp_ack->options[24] =20;
    dhcp_ack->options[25] =10;
    dhcp_ack->options[26] =1;
    dhcp_ack->options[27] = 6;
    dhcp_ack->options[28] = 4;
    memcpy(dhcp_ack->options + 29, &dhcp_ack->siaddr.s_addr, 4);
    dhcp_ack->options[33] = 255;
}

int main(void)
{
    int mysocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    int yes = 1 ; 
    struct sockaddr_in servsock;
    socklen_t some=sizeof(servsock);
    memset(&servsock,0,sizeof(servsock));
    servsock.sin_family=AF_INET;
    servsock.sin_port=htons(67);
    servsock.sin_addr.s_addr=htonl(INADDR_ANY);

    if ( setsockopt(mysocket,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(yes))<0)
    {
        printf("error with the broad cast thing\n ");
        return -1;
    }
    if ( setsockopt(mysocket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))<0)
    {
        printf(" please work \n");
        return -1;
    }
    if(bind(mysocket,(struct sockaddr *)&servsock,some)<0)
    {
        printf("the binding is bitching \n");
        return -1;
    }
    struct sockaddr_in serv;
    memset(&serv,0,sizeof(serv));
    serv.sin_addr.s_addr=htonl(INADDR_BROADCAST);
    serv.sin_port=htons(68);
    serv.sin_family=AF_INET;
    while(1)
    {
        struct sockaddr_in from;
        
        
        dhcp_pack * discover=malloc(sizeof(dhcp_pack));
        dhcp_pack * offer=malloc(sizeof(dhcp_pack));
        if(recvfrom(mysocket,discover,sizeof(dhcp_pack),0,(struct sockaddr *)&from,&some)<0)
        {
            printf("receiving is on some crazy shit ! \n");
            return  -1 ;
        }
        if(discover->options[2]==1)
        {
        printf("%d\n",discover->xid);
       /* if(discover->op==2)
        {
            continue;
        }*/
        printf("%d\n",discover->op);
        makeoffer(offer,discover);
        if(sendto(mysocket,offer,sizeof(dhcp_pack),0,(struct sockaddr *)&serv,sizeof(serv))<0)
        {
            printf("wassup with this shit the sending is just making me wanna end my life \n");
            return -1;
        }
        }
        else
        {
            dhcp_ack(discover,offer);
            if(sendto(mysocket,offer,sizeof(dhcp_pack),0,(struct sockaddr *)&serv,sizeof(serv))<0)
            {
                printf("wassup with this shit the sending is just making me wanna end my life \n");
                return -1;
            }
        }
        usleep(100000);
    }
}