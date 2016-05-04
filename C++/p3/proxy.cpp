#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <fstream>

using boost::asio::ip::tcp;
using namespace std;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
{
public:
    char buf_in[1024];
    char buf_out[1024];
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new tcp_connection(io_service));
    }

    tcp::socket& socket()
    {
        return socket_in_;
    }

    tcp::socket& socket_out()
    {
        return socket_out_;
    }

    void connect_to_ip(string now_ip, string now_host)
    {
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(now_ip), atoi(now_host.c_str()));
        this->socket_out().async_connect(ep, boost::bind(&tcp_connection::connect_handler, this, boost::asio::placeholders::error));
        read_client();
        read_server();
    }

    void read_client()
    {
        boost::asio::async_read(socket_in_, boost::asio::buffer(buf_in, 1024), boost::asio::transfer_at_least(1),
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

    void read_server()
    {
        boost::asio::async_read(socket_out_, boost::asio::buffer(buf_out, 1024), boost::asio::transfer_at_least(1),
          boost::bind(&tcp_connection::handle_read_server, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void write_server()
    {
        boost::asio::async_write(socket_out_, boost::asio::buffer(buf_in),
        boost::bind(&tcp_connection::handle_write_server, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    }

private:
    tcp::socket socket_in_;
    tcp::socket socket_out_;
    std::string message_;
    bool in_flag = false;
    bool out_flag = false;
    tcp_connection(boost::asio::io_service& io_service) : socket_in_(io_service), socket_out_(io_service) {}

    void handle_write_client(const boost::system::error_code& err, size_t t)
    {
        if (!err)
        {
            read_server();
        }
    }

    void handle_read_client(const boost::system::error_code& err, size_t bytes_transferred)
    {
        if (!err)
        {
            buf_in[bytes_transferred] = '\0';
            write_server();
        }
        else if (err == boost::asio::error::eof)
        {
            socket_in_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
            in_flag = true;
            if (in_flag && out_flag)
            {
                socket_in_.close();
                socket_out_.close();
            }
        }
    }

    void handle_write_server(const boost::system::error_code& err, size_t t)
    {
        if (!err)
        {
            read_client();
        }
    }

    void handle_read_server(const boost::system::error_code& err, std::size_t bytes_transferred)
    {
        if (!err)
        {
            buf_out[bytes_transferred] = '\0';
            write_client();
        }
        else if (err == boost::asio::error::eof)
        {
            socket_out_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
            out_flag = true;
            if (in_flag && out_flag)
            {
                socket_in_.close();
                socket_out_.close();
            }
        }
    }
    void connect_handler(const boost::system::error_code& err){}
};

class tcp_server
{
public:
    tcp_server(boost::asio::io_service& io_service, int port, std::vector <std::string> ip_string) 
    : acceptor_(io_service, tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port)), ip_string(ip_string), io_service(io_service)
    {
        start_accept();
    }

private:
    tcp::acceptor acceptor_;
    std::vector <std::string> ip_string;
    vector <tcp::socket> sockets;
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
        if (!error)
        {
            srand(time(NULL));
            auto pos = rand() % ip_string.size();

            auto ip_s = ip_string[pos];

            pos = ip_s.find(':');
            auto now_ip = ip_s.substr(0, pos);
            auto now_host = ip_s.substr(pos + 1);

            new_connection->connect_to_ip(now_ip, now_host);
        }
        start_accept();
    }
};

int main(int argc, char * argv[])
{
    auto port = -1;
    std::vector <std::string> ip_string;
    std::ifstream fin;
    if (argc >= 2)
    {
        fin.open(argv[1]);
        std::string s;

        while (fin >> s)
        {
            if (port == -1)
                port = std::atoi(s.c_str());
            else
                ip_string.push_back(s);
        }
    }
    else
    {
        perror("couldnt read input file");
        exit(EXIT_FAILURE);
    }

    try
    {
        boost::asio::io_service io_service;
        tcp_server server(io_service, port, ip_string);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}