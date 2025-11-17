#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 80
#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 10

// Use the same IP as your DHCP server
#define MYIP "172.20.10.2"  // Same as in dhcp.h

// Enhanced HTML content
const char *response_template = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Connection: close\r\n"
    "Server: Fake-Facebook-Server\r\n"
    "\r\n"
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>This is not ADN.fr</title>\n"
    "    <style>\n"
    "        body {\n"
    "            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\n"
    "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
    "            display: flex;\n"
    "            justify-content: center;\n"
    "            align-items: center;\n"
    "            min-height: 100vh;\n"
    "            margin: 0;\n"
    "            padding: 20px;\n"
    "        }\n"
    "        .container {\n"
    "            background-color: white;\n"
    "            border-radius: 12px;\n"
    "            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);\n"
    "            padding: 40px;\n"
    "            text-align: center;\n"
    "            max-width: 500px;\n"
    "            animation: slideIn 0.5s ease-out;\n"
    "        }\n"
    "        @keyframes slideIn {\n"
    "            from { opacity: 0; transform: translateY(-20px); }\n"
    "            to { opacity: 1; transform: translateY(0); }\n"
    "        }\n"
    "        h1 {\n"
    "            color: #1877f2;\n"
    "            font-size: 36px;\n"
    "            margin-bottom: 20px;\n"
    "            font-weight: 700;\n"
    "        }\n"
    "        .warning {\n"
    "            color: #721c24;\n"
    "            background: linear-gradient(135deg, #f8d7da 0%, #f5c6cb 100%);\n"
    "            border: 2px solid #f5c6cb;\n"
    "            padding: 20px;\n"
    "            border-radius: 8px;\n"
    "            margin: 20px 0;\n"
    "            font-weight: 600;\n"
    "        }\n"
    "        .attack-info {\n"
    "            background: #e3f2fd;\n"
    "            border: 2px solid #2196f3;\n"
    "            border-radius: 8px;\n"
    "            padding: 15px;\n"
    "            margin: 20px 0;\n"
    "            color: #0d47a1;\n"
    "        }\n"
    "        .details {\n"
    "            font-size: 14px;\n"
    "            color: #666;\n"
    "            margin-top: 20px;\n"
    "            text-align: left;\n"
    "        }\n"
    "        .emoji {\n"
    "            font-size: 24px;\n"
    "            margin: 0 5px;\n"
    "        }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <div class=\"container\">\n"
    "        <h1><span class=\"emoji\">🚫</span> This is not ADN.fr</h1>\n"
    "        \n"
    "        <div class=\"warning\">\n"
    "            <span class=\"emoji\">⚠️</span>\n"
    "            <strong>MITM Attack Successful!</strong><br>\n"
    "            Your connection to ADN.fr has been intercepted.\n"
    "        </div>\n"
    "        \n"
    "        <div class=\"attack-info\">\n"
    "            <strong>Attack Vector:</strong> DHCP Spoofing + DNS Hijacking<br>\n"
    "            <strong>Attacker IP:</strong> " MYIP "<br>\n"
    "            <strong>Target Domain:</strong> www.adn.fr\n"
    "        </div>\n"
    "        \n"
    "        <p>This page demonstrates a successful Man-in-the-Middle attack where:</p>\n"
    "        \n"
    "        <div class=\"details\">\n"
    "            <p><span class=\"emoji\">✅</span> <strong>DHCP Starvation:</strong> Exhausted legitimate DHCP server</p>\n"
    "            <p><span class=\"emoji\">✅</span> <strong>Rogue DHCP:</strong> Provided malicious network configuration</p>\n"
    "            <p><span class=\"emoji\">✅</span> <strong>DNS Poisoning:</strong> Redirected DNS queries to attacker</p>\n"
    "            <p><span class=\"emoji\">✅</span> <strong>Traffic Interception:</strong> Captured web requests</p>\n"
    "        </div>\n"
    "        \n"
    "        <div style=\"margin-top: 30px; font-size: 12px; color: #999;\">\n"
    "            <p>Educational demonstration of network security vulnerabilities</p>\n"
    "        </div>\n"
    "    </div>\n"
    "</body>\n"
    "</html>\n";

// Signal handler for child processes (prevents zombies)
void sigchld_handler(int s) {
    (void)s; // Suppress unused parameter warning
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Handle client connection
void handle_client(int client_socket, struct sockaddr_in *client_addr) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    printf("[+] Handling client: %s\n", inet_ntoa(client_addr->sin_addr));
    
    // Read client request
    ssize_t bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        // Extract request info
        char method[16] = "", path[256] = "", version[16] = "";
        if (sscanf(buffer, "%15s %255s %15s", method, path, version) >= 2) {
            printf("[+] %s %s from %s\n", method, path, inet_ntoa(client_addr->sin_addr));
        }
        
        // Log user agent if present
        char *user_agent = strstr(buffer, "User-Agent:");
        if (user_agent) {
            char agent[256];
            char *end = strstr(user_agent, "\r\n");
            if (end) {
                int len = end - user_agent;
                if (len > 255) len = 255;
                strncpy(agent, user_agent, len);
                agent[len] = '\0';
                printf("[+] %s\n", agent);
            }
        }
        
        // Send HTTP response
        ssize_t sent = send(client_socket, response_template, strlen(response_template), 0);
        if (sent > 0) {
            printf("[+] Sent %zd bytes to %s\n", sent, inet_ntoa(client_addr->sin_addr));
        }
    } else if (bytes_read < 0) {
        perror("recv failed");
    }
    
    // Close connection
    close(client_socket);
    printf("[+] Connection closed for %s\n", inet_ntoa(client_addr->sin_addr));
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int opt = 1;
    
    printf("[+] Fake Web Server starting\n");
    printf("    Serving on: %s:%d\n", MYIP, PORT);
    printf("    Attack page: 'This is not Facebook'\n");
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_socket);
        return 1;
    }
    
    // Prepare server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(MYIP);  // Bind to our specific IP
    server_addr.sin_port = htons(PORT);
    
    // Bind to port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed - trying INADDR_ANY");
        // Fallback to any address
        server_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Bind failed completely");
            close(server_socket);
            return 1;
        }
    }
    
    // Listen for connections
    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }
    
    // Set up signal handler for child processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction failed");
    }
    
    printf("[+] Web server listening on port %d\n", PORT);
    printf("[+] Press Ctrl+C to stop\n");
    printf("[+] Ready to serve fake Facebook pages!\n");
    
    // Main accept loop
    while (1) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Fork to handle client (simpler than pthread)
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(server_socket);  // Child doesn't need the listening socket
            handle_client(client_socket, &client_addr);
            exit(0);
        } else if (pid > 0) {
            close(client_socket);  
        } else {
            perror("Fork failed");
            close(client_socket);
        }
    }
    
    close(server_socket);
    return 0;
}