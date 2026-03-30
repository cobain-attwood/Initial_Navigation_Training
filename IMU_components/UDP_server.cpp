#include "UDP_server.h"
  
 //TODO add comment section 
int UDP_server::server_setup()
{
    // Creating socket file descriptor 
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    { 
        perror("socket creation failed"); 
        return -1; 
    } 
    
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr));
    
    //Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT);
    
    // Bind the socket with the server address 
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    { 
        perror("bind failed"); 
        return -1; 
    } 
    std::cout << "server set up" << std::endl;
    return 0;
}

void UDP_server::send(const char* buffer)
{
    socklen_t len;
    len = sizeof(cliaddr);  //len is value/result 
    
    if(sendto(sockfd, (const char *)buffer, strlen(buffer), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len) == -1)
    {
        //ERROR HANDLING: i2c transaction failed
        std::cout << "Failed to send message over UDP." << std::endl;
        std::cout << "errno is: " << errno << std::endl;
        std::cout << strerror(errno) << std::endl;
    } 
    else
    {
        std::cout << "Message sent." << std::endl;
    }
}

void UDP_server::receive(char* buffer)
{
    socklen_t len;
    int n; 
    len = sizeof(cliaddr);  //len is value/result 
    
    n = recvfrom(sockfd, (char* )buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len); 
    buffer[n] = '\0'; 
    printf("Client : %s\n", buffer);
}
  
  
/*int main() { 

    //const char *hello = "Hello from server";
    char buffer[MAXLINE] = {0};
    UDP_server server;
    
    if(server.server_setup() == 0)
    {
        std::cout << "Server set up successful" << std::endl;
        while(1)
        {
            server.receive(buffer);
        }
    }
    else
    {
        std::cout << "Server set up failed" << std::endl;
    }
    
    return 0; 
}*/
