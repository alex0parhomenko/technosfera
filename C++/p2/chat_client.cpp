#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <list>
#include <time.h>

#define BUF_SIZE 1024
#define SERVER_PORT 3100
#define SERVER_IP "127.0.0.1"
#define SERVER_HOST_LEN 9

int client_init()
{
	auto ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(SERVER_PORT);
	SockAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	connect(ClientSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
	return ClientSocket;
}

int main()
{
	auto ClientSocket = client_init();

	/*while(true)
	{
		listen(MasterSocket, SOMAXCONN);
		int SlaveSocket = accept(MasterSocket, 0, 0);
	// ...
	}*/
	return 0;
}