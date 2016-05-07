#include <iostream>
#include <algorithm>
#include <cmath>
#include <string>
#include <map>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <boost/bind.hpp>
#include <sys/types.h>
#include <sstream>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/epoll.h>  

#define SHARED_MEMORY_NAME "my_shared_memory"
#define SERVER_PORT 3102
#define SERVER_IP "127.0.0.1"
#define EPOLL_RUN_TIMEOUT 20
#define MAX_EPOLL_EVENTS_PER_RUN 1000
#define SERVER_HOST_LEN 9
#define EPOLL_QUEUE_LEN 1000
#define BUF_SIZE 1024
#define accept_error "accept error"

using namespace std;

char accept_msg[] = "accepted connection\n";
char con_term_msg[] = "connection terminated\n";
char welcome[] = "Welcome\n";
int semid = 0;

enum{ notes_amount = 3591, key_len = 32, value_len = 256, tl_len = 4, mem_size = (1 << 20), is_empty_size = 3591, sem_count = 100};
hash<string> hash_fn;
int port = 3101;
char * mem = NULL;
char * is_empty_mem = NULL;
int sock_fd[4][2];

bool set(string key, string value, string tl, char * mem, char * is_empty_mem);
char * get(string key,char * mem, char * is_empty_mem);
void parent_write(char * s, int fd);
static int recv_file_descriptor(int socket);
static int send_file_descriptor(int socket, int fd_to_send); 

union semun {
    int val;                /* value for SETVAL */
    struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
    ushort *array;          /* array for GETALL & SETALL */
    struct seminfo *__buf;  /* buffer for IPC_INFO */
    void *__pad;
};

void sem_up(int num)
{
	struct sembuf op;
	op.sem_num = num % sem_count;
	op.sem_op = 1;
	op.sem_flg = 0;
	semop(semid, &op, 1);
}

void sem_down(int num)
{
	struct sembuf op;
	op.sem_num = num % sem_count;
	op.sem_op = -1;
	op.sem_flg = 0;
	semop(semid, &op, 1);
}

void sem_wait(int num)
{
	struct sembuf op;
	op.sem_num = num % sem_count;
	op.sem_op = 0;
	op.sem_flg = 0;
	semop(semid, &op, 1);
}


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
       perror ("bind error: ");
       exit (EXIT_FAILURE);
    }
	return MasterSocket;
}

void add_server(int &MasterSocket, int &epfd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLPRI | EPOLLET;
	ev.data.fd = MasterSocket;
	epoll_ctl(epfd, EPOLL_CTL_ADD, MasterSocket, &ev);
}

void add_client(int &client, int &epfd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLPRI;
	ev.data.fd = client;
	epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
}


string parse_str(string s)
{
	string op;
	s.erase(s.find_last_not_of(" \n\r\t")+1);
    auto pos = 0;
    while (pos < s.size() && s[pos] != ' ')
    {
    	op += s[pos];
    	pos++;
    }
    pos++;
    if (op == "get")
    {
    	string key;
    	while (pos < s.size())
    	{	
    		key += s[pos];
    		pos++;
    	}
    	if (!key.size())
    	{
    		string ans = "invalid operation\n";
    		return ans;
    	}
    	char * val = get(key, mem, is_empty_mem);
    	if (val != NULL)
    	{
    		string ans = "ok " + key + " " + val + "\n";
    		return ans;
    	}
    	else
    	{
    		string ans = "not key " + key + "\n";
    		return ans;
    	}

    }
    else if (op == "set")
    {
    	string key, tl, value;
    	while (pos < s.size() && s[pos] != ' ')
    	{
    		tl += s[pos];
    		pos++;
    	}
    	pos++;
    	while (pos < s.size() && s[pos] != ' ')
    	{
    		key += s[pos];
    		pos++;
    	}
    	pos++;
    	while (pos < s.size())
    	{
    		value += s[pos];
    		pos++;
    	}
    	if (!key.size() || !tl.size() || !value.size() || key.size() > 256 || value.size() > 32 || (!atoi(tl.c_str()) && tl != "0"))
    	{
    		string ans = "invalid operation, try again\n";
    		return ans;
    	}
    	else if (set(key, value, tl, mem, is_empty_mem))
    	{
    		string ans = "ok " + key + " " + value + "\n";
    		return ans;
    	}
    	else
    	{
    		string ans = "cannot write note in hash map\n";
    		return ans;
    	}
    }
    else
    {
    	string ans = "invalid operation, try again\n";
    	return ans;
    }
}

void start_server()
{
	auto MasterSocket = server_init();
	auto epfd = epoll_create(EPOLL_QUEUE_LEN);
	add_client(MasterSocket, epfd);
	struct epoll_event events[EPOLL_QUEUE_LEN];
	char RecvBuffer[BUF_SIZE + 1];
	listen(MasterSocket, SOMAXCONN);

	while(true)
	{
		int cou_events = epoll_wait(epfd, events, 1000, -1);
		if (cou_events == -1)
		{
			perror("epoll error:");
			exit(EXIT_FAILURE);
		}
		for (auto i = 0; i < cou_events; i++)
		{
			if (events[i].data.fd == MasterSocket)
			{
				int client = accept(MasterSocket, 0, 0);
				if (client < 0)
					continue;	
				auto proc_num = rand() % 4;
				send_file_descriptor(sock_fd[proc_num][0], client);
			}
		}
	}
	close(MasterSocket);
	return;
}



bool set(string key, string value, string tl, char * mem, char * is_empty_mem)
{
	auto h = hash_fn(key) % notes_amount;
	auto pos = h * (key_len + value_len + tl_len);
	auto num = h;
	char * k = (char *)malloc((key_len + 1) * sizeof(char));
	memset(k, 0, key_len + 1);
	char is_del = 0;
	int real_tl = atoi(tl.c_str());
	while (pos + key_len < mem_size)
	{
		memcpy(k, mem + pos, key_len);
		memcpy(&is_del, is_empty_mem + num, sizeof(char));
		if (!strlen(k) || is_del)
		{
			sem_wait(num);
			sem_up(num);
			memcpy((void *)(mem + pos), key.c_str(), key.size() + 1);
			memcpy((void *)(mem + pos + key_len), &real_tl, sizeof(int));
			memcpy((void *)(mem + pos + key_len + tl_len), value.c_str(), value.size() + 1);
			free(k);
			sem_down(num);
			return true;
		}
		pos += key_len + value_len + tl_len;
		num++;
	}
	free(k);
	return false;
}

char * get(string key, char * mem, char * is_empty_mem)
{
	auto h = hash_fn(key) % notes_amount;
	auto pos = h * (key_len + value_len + tl_len);
	auto num = h;
	char * k = (char *) malloc((key_len + 1) * sizeof(char));
	memset(k, 0, key_len + 1);
	char is_del = 0;
	while (pos + key_len < mem_size && num < is_empty_size)
	{
		sem_wait(num);
		sem_up(num);
		memcpy((void *)k, (mem + pos), key_len * sizeof(char));
		memcpy(&is_del, is_empty_mem + num, sizeof(char));
		char * val = (char *)(mem + pos + key_len + tl_len);
		sem_down(num);
		if (!is_del && !strlen(k))
			return NULL;
		if (!is_del && !strcmp(k, key.c_str()))
			return val;
		pos += key_len + value_len + tl_len;
		num++;
	}
	free(k);
	return NULL;
}


static int send_file_descriptor(int socket, int fd_to_send)
{
	struct msghdr message;
	struct iovec iov[1];
	struct cmsghdr *control_message = NULL;
	char ctrl_buf[CMSG_SPACE(sizeof(int))];
	char data[1];

	memset(&message, 0, sizeof(struct msghdr));
	memset(ctrl_buf, 0, CMSG_SPACE(sizeof(int)));

	data[0] = ' ';
	iov[0].iov_base = data;
	iov[0].iov_len = sizeof(data);

	message.msg_name = NULL;
	message.msg_namelen = 0;
	message.msg_iov = iov;
	message.msg_iovlen = 1;
	message.msg_controllen =  CMSG_SPACE(sizeof(int));
	message.msg_control = ctrl_buf;

	control_message = CMSG_FIRSTHDR(&message);
	control_message->cmsg_level = SOL_SOCKET;
	control_message->cmsg_type = SCM_RIGHTS;
	control_message->cmsg_len = CMSG_LEN(sizeof(int));

	*((int *) CMSG_DATA(control_message)) = fd_to_send;

	return sendmsg(socket, &message, 0);
}

static int recv_file_descriptor(int socket)
{
	int sent_fd;
	struct msghdr message;
	struct iovec iov[1];
	struct cmsghdr *control_message = NULL;
	char ctrl_buf[CMSG_SPACE(sizeof(int))];
	char data[1];
	int res;

	memset(&message, 0, sizeof(struct msghdr));
	memset(ctrl_buf, 0, CMSG_SPACE(sizeof(int)));

	iov[0].iov_base = data;
	iov[0].iov_len = sizeof(data);

	message.msg_name = NULL;
	message.msg_namelen = 0;
	message.msg_control = ctrl_buf;
	message.msg_controllen = CMSG_SPACE(sizeof(int));
	message.msg_iov = iov;
	message.msg_iovlen = 1;

	if((res = recvmsg(socket, &message, 0)) <= 0)
		return res;

	for(control_message = CMSG_FIRSTHDR(&message); control_message != NULL; control_message = CMSG_NXTHDR(&message, control_message))
	{
		if ((control_message->cmsg_level == SOL_SOCKET) && (control_message->cmsg_type == SCM_RIGHTS))
		{
			return *((int *) CMSG_DATA(control_message));
		}
	}

	return -1;
}


int main(int argc, char * argv[])
{
	auto semkey = 0;
	setbuf(stdout, NULL);
	srand(time(NULL));
	auto pids = new int[4];
	auto length = mem_size + is_empty_size + 1;
	int fd = open("mmap.txt", O_CREAT | O_RDWR | O_TRUNC, 0666);
	ftruncate(fd, length);
	auto ptr = (char *)mmap(0, length, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
	{
		perror("Mapping failed");
		exit(EXIT_FAILURE);
	}
	if ((semkey = ftok("/tmp", 'a')) == (key_t)-1) 
	{
    	perror("IPC error: ftok"); 
    	exit(1);
	}

	if ((semid = semget(semkey, sem_count, S_IRUSR | S_IWUSR | IPC_CREAT)) != -1)
    {
    	union semun semopts;    
        semopts.val = 0;
        for (auto i = 0; i < sem_count; i++)
        	auto k = semctl(semid, i, SETVAL, semopts);
    }
    else
    {
    	perror("IPC error: semget");
    	exit(1);
    }

	close(fd);

	mem = ptr;
	is_empty_mem = ptr + mem_size;

	memset(is_empty_mem, 0, is_empty_size);
	memset(mem, 0, mem_size);

	for (auto i = 0; i < 4; i++)
		socketpair(PF_LOCAL, SOCK_STREAM, 0, sock_fd[i]);

	for (auto i = 0; i < 4; i++)
	{
		if (!(pids[i] = fork()))
		{
			close(sock_fd[i][0]);
			auto proc_num = i;
			auto epfd = epoll_create(EPOLL_QUEUE_LEN);
			struct epoll_event events[EPOLL_QUEUE_LEN];
			add_client(sock_fd[proc_num][1], epfd);

			while (1)
			{
				int cou_events = epoll_wait(epfd, events, 1000, -1);
				if (cou_events == -1)
				{
					perror("epoll error:");
					exit(EXIT_FAILURE);
				}
				for (auto i = 0; i < cou_events; i++)
				{
					if (events[i].data.fd == sock_fd[proc_num][1])
					{
						
						auto sock_num = recv_file_descriptor(sock_fd[proc_num][1]);
						add_client(sock_num, epfd);
					}
					else if (events[i].events & EPOLLIN)
					{
						char buf[BUF_SIZE];
						auto cou_b = read(events[i].data.fd, buf, BUF_SIZE);
						if (cou_b <= 0)
						{
							close(events[i].data.fd);
							continue;
						}
						buf[cou_b - 1] = '\0';
						string ans = parse_str((string)buf);
						write(events[i].data.fd, ans.c_str(), ans.size());
					}
				}	
			}
			
			exit(0);
		}
		else
			close(sock_fd[i][1]);
	}
	start_server();
    munmap((void *)ptr, length);
	return 0;
}



