



#define CLIENTIP "192.168.192.55"
#define MYIP  "192.168.192.189"



typedef struct __attribute__((packed)){
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    struct in_addr ciaddr;
    struct in_addr yiaddr;
    struct in_addr siaddr;
    struct in_addr giaddr;
    uint8_t chaddr[16];
    char sname[64];
    char file[128];
    uint32_t magic;
    uint8_t options[312];
}dhcp_pack;  