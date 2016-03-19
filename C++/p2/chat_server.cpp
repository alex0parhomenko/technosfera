#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <iterator>
#include <string.h>
#include <list>
#include <sys/epoll.h>  
#include <time.h>

#define SERVER_PORT 3100
#define SERVER_IP "127.0.0.1"
#define EPOLL_RUN_TIMEOUT 20
#define MAX_EPOLL_EVENTS_PER_RUN 10
#define SERVER_HOST_LEN 9
#define EPOLL_QUEUE_LEN 1000
#define BUF_SIZE 1024
#define accept_error "accept error"
//SERVER
char accept_msg[] = "accepted connection\n";
char con_term_msg[] = "connection terminated\n";
char welcome[] = "Welcome\n";

int server_init()
{
	auto MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(SERVER_PORT);
	SockAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	auto optval = 1;
	setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (bind(MasterSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr)) < 0)
    {
       perror ("bind");
       exit (EXIT_FAILURE);
    }
	return MasterSocket;
}

int add_client(int &MasterSocket, int &epfd)
{
	struct epoll_event ev;
	int client = accept(MasterSocket, 0, 0);
	if (client < 0)
		perror(accept_error);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = client;
	epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
	return client;
}

void add_server(int &MasterSocket, int &epfd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLPRI | EPOLLET;
	ev.data.fd = MasterSocket;
	epoll_ctl(epfd, EPOLL_CTL_ADD, MasterSocket, &ev);
}

int main()
{
	setbuf(stdout, NULL);
	int MasterSocket = server_init();
	int epfd = epoll_create(EPOLL_QUEUE_LEN);
	add_server(MasterSocket, epfd);
	struct epoll_event events[EPOLL_QUEUE_LEN];
	char RecvBuffer[BUF_SIZE];
	listen(MasterSocket, SOMAXCONN);
	std::vector<int> all_fd;
	while(true)
	{
		int cou_events = epoll_wait(epfd, events, 1000, -1);
		for (int i = 0; i < cou_events; i++)
		{
			if (events[i].data.fd == MasterSocket)
			{
				int client = add_client(MasterSocket, epfd);
				std::cout << accept_msg;
				send(client, welcome, sizeof(welcome), MSG_DONTROUTE);
				bool flag = true;
				for (auto j = 0; j < all_fd.size(); j++)
				{
					if (all_fd[j] == 0)
					{
						all_fd[j] = client;
						flag = false;
						break;
					}
				}
				if (flag)
					all_fd.push_back(client);
			}
			else if (events[i].events & EPOLLIN)
			{
				int cou = 0;
				while (true)
				{
					cou = read(events[i].data.fd, RecvBuffer, BUF_SIZE - 1);
					RecvBuffer[cou] = '\0';
					printf("%s", RecvBuffer);
					if (cou == 0)
						break;
					for (auto j = 0; j < all_fd.size(); j++)
					{
						if (all_fd[j] == 0)
							continue;
						send(all_fd[j], RecvBuffer, strlen(RecvBuffer), 0);
					}
					if (RecvBuffer[strlen(RecvBuffer) - 1] == '\n')
						break;
					memset(RecvBuffer, 0, sizeof(RecvBuffer));
				}
				if (cou == 0)
				{
					printf("%s", con_term_msg);
					close(events[i].data.fd);
					for (auto fd_beg = all_fd.begin(); fd_beg != all_fd.end(); ++fd_beg)
					{
						if (*fd_beg == events[i].data.fd)
						{
							all_fd.erase(fd_beg);
							break;
						}
					}
				}
			}
			
		}
	}
	close(MasterSocket);
	return 0;
}