#include <bits/stdc++.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#define PORT 8080 
#define MAXLINE 1024 

class UDP_server
{
	private:
	int sockfd = 0; 
    struct sockaddr_in servaddr, cliaddr; 

	public:
	int server_setup();
	void send(const char* buffer);
	void receive(char* buffer);
};
