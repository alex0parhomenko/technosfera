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
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

enum{ notes_amount = 3591, key_len = 32, value_len = 256, tl_len = 4, mem_size = (1 << 20), is_empty_size = 3591, SIZE = 1024};
hash<string> hash_fn;
char * mem = NULL;
char * is_empty_mem = NULL;

bool set(string key, string value, string tl, char * mem, char * is_empty_mem);
char * get(string key,char * mem, char * is_empty_mem);

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
{
public:
    char buf_in[SIZE];
    char buf_out[SIZE];
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new tcp_connection(io_service));
    }

    tcp::socket& socket()
    {
        return socket_in_;
    }

    void read_client()
    {
        boost::asio::async_read(socket_in_, boost::asio::buffer(buf_in, SIZE), boost::asio::transfer_at_least(1),
          boost::bind(&tcp_connection::handle_read_client, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    void write_client()
    {
        boost::asio::async_write(socket_in_, boost::asio::buffer(buf_out),
        boost::bind(&tcp_connection::handle_write_client, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    }

private:
    tcp::socket socket_in_;
    std::string message_;
    tcp_connection(boost::asio::io_service& io_service) : socket_in_(io_service) {}

    void handle_write_client(const boost::system::error_code& err, size_t t)
    {
        if (!err)
        {
        	memset(buf_out, 0, SIZE);
        }
    }

    void handle_read_client(const boost::system::error_code& err, size_t bytes_transferred)
    {
        if (!err)
        {
            buf_in[bytes_transferred] = '\0';
            string s = buf_in;
            s.erase(s.find_last_not_of(" \n\r\t")+1);
            //cout << s << endl;
            string op;
            auto pos = 0;
            memset(buf_out, 0, SIZE);
            memset(buf_in, 0, SIZE);
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
            		memcpy(buf_out, ans.c_str(), ans.size() + 1);
            		write_client();
            	}
            	char * val = get(key, mem, is_empty_mem);
            	if (val != NULL)
            	{
            		string ans = "ok " + key + " " + val + "\n";
            		memcpy(buf_out, ans.c_str(), ans.size() + 1); 
            		write_client();
            	}
            	else
            	{
            		string ans = "not key " + key + "\n";
            		memcpy(buf_out, ans.c_str(), ans.size() + 1);
            		write_client();	
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
            	if (!key.size() || !tl.size() || !value.size() || key.size() > 256 || value.size() > 32 || !atoi(tl.c_str()))
            	{
            		string ans = "invalid operation, try again\n";
            		memcpy(buf_out, ans.c_str(), ans.size() + 1);
            		write_client();
            	}
            	else if (set(key, value, tl, mem, is_empty_mem))
            	{
            		string ans = "ok " + key + " " + value + "\n";
            		memcpy(buf_out, ans.c_str(), ans.size() + 1);
            		write_client();
            	}
            	else
            	{
            		string ans = "cannot write note in hash map\n";
            		memcpy(buf_out, ans.c_str(), ans.size() + 1);
            		write_client();
            	}
            }
            else
            {
            	string ans = "invalid operation, try again\n";
            	memcpy(buf_out, ans.c_str(), ans.size() + 1);
            	write_client();
            }
            read_client();
        }
        else if (err == boost::asio::error::eof)
        {
            socket_in_.close();
        }
    }
};

class tcp_server
{
public:
    tcp_server(boost::asio::io_service& io_service, int port) 
    : acceptor_(io_service, tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port)), io_service(io_service)
    {
        start_accept();
    }

private:
    tcp::acceptor acceptor_;
    boost::asio::io_service& io_service;
    vector <tcp_connection::pointer> all_connections;

    void start_accept()
    {
        tcp_connection::pointer new_connection =
          tcp_connection::create(acceptor_.get_io_service());

        all_connections.push_back(new_connection);

        acceptor_.async_accept(new_connection->socket(),
            boost::bind(&tcp_server::handle_accept, this, new_connection,
              boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error)
    {
        new_connection->read_client();
        start_accept();
    }
};



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
			memcpy((void *)(mem + pos), key.c_str(), key.size() + 1);
			memcpy((void *)(mem + pos + key_len), &real_tl, sizeof(int));
			memcpy((void *)(mem + pos + key_len + tl_len), value.c_str(), value.size() + 1);
			free(k);
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
		memcpy((void *)k, (mem + pos), key_len * sizeof(char));
		memcpy(&is_del, is_empty_mem + num, sizeof(char));

		if (!is_del && !strlen(k))
			return NULL;
		if (!is_del && !strcmp(k, key.c_str()))
		{
			char * val = (char *)(mem + pos + key_len + tl_len);
			return val;
		}
		pos += key_len + value_len + tl_len;
		num++;
	}
	free(k);
	return NULL;
}


int main(int argc, char * argv[])
{
	setbuf(stdout, NULL);
	mem = (char *)malloc(mem_size * sizeof(char));
	is_empty_mem = (char *)malloc(is_empty_size * sizeof(char));
	memset(is_empty_mem, 0, is_empty_size);
	memset(mem, 0, mem_size);
	
	auto port = 3101;
	try
    {
        boost::asio::io_service io_service;
        tcp_server server(io_service, port);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
	return 0;
}