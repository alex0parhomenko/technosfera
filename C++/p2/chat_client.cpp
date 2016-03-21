#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <list>
#include <time.h>

#define BUF_SIZE 1024
#define SERVER_PORT 3100
#define SERVER_IP "127.0.0.1"

int client_init()
{
	auto ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket < 0)
	{
		perror("connect fail:");
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(SERVER_PORT);
	SockAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	if (connect(ClientSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr)) < 0)
	{
		perror("connect error:");
		exit(EXIT_FAILURE);
	}
	return ClientSocket;
}

int main(int argc, char * argv[])
{
	char buf[BUF_SIZE + 1];
	auto ClientSocket = client_init();
	auto cou = 0;
	struct timeval waitd;
	fd_set read_flags, write_flags;
	while (1)
	{
		waitd.tv_sec = 3;
        FD_ZERO(&read_flags);
        FD_ZERO(&write_flags);
        FD_SET(ClientSocket, &read_flags);
        FD_SET(0, &read_flags);
        auto sel = select(ClientSocket + 1, &read_flags, &write_flags, (fd_set*)0, &waitd);
        if (sel > 0)
        {
        	if (FD_ISSET(ClientSocket, &read_flags))
        	{
        		cou = read(ClientSocket, buf, BUF_SIZE);
        		printf("%s", buf);
        		memset(buf, 0, sizeof(buf));
        	}
        	if (FD_ISSET(0, &read_flags))
        	{
        		fgets(buf, BUF_SIZE, stdin);
        		write(ClientSocket, buf, strlen(buf));
        		memset(buf, 0, sizeof(buf));
        	}
        }
        else if(sel == -1)
        {
        	perror("select error:");
        	exit(EXIT_FAILURE);
        }

	}
	close(ClientSocket);
	return 0;
}