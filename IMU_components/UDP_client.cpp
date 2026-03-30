#include "UDP_client.h"


int UDP_client::client_setup()
{
	// Creating socket file descriptor 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    { 
        std::cout << "socket creation failed" << std::endl; 
        return -1; 
    } 
    else
    {
		memset(&servaddr, 0, sizeof(servaddr)); 
	      
	    // Filling server information 
	    servaddr.sin_family = AF_INET; 
	    servaddr.sin_port = htons(PORT); 
	    servaddr.sin_addr.s_addr = inet_addr("172.18.160.2"); //replace with address	
		return 0;
	}
}

void UDP_client::send(char* buffer)
{
	sendto(sockfd, (const char *)buffer, strlen(buffer), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
}

void UDP_client::receive(char* buffer)
{
	
	//TODO	
	
}


// Driver code 
/*int main() { 
    char buffer[32]; 
    
    sprintf(buffer, "Example data: %f", 3.14);
    
    UDP_client client;
    client.client_setup();
    client.send(buffer);
    
   
   // close(sockfd); 
    return 0; 
}*/
