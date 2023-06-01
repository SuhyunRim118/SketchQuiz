
#include <cstdlib>
#include <iostream>
#define BOOST_BIND_NO_PLACEHOLDERS
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <fstream>
 
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

using namespace std;
 
class session
{
public:
    session(boost::asio::io_service& io_service)
    : tcp_socket(io_service), udp_socket(io_service) {}
    
    tcp::socket& socket()
    {
        return tcp_socket;
    }
    
    void start()
    {
        cout << "Client Connected\n" ;
        tcp_socket.async_read_some(boost::asio::buffer(reply, max_length),
            boost::bind(&session::handle_upload, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
  }
 
private:

    void handle_upload(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        tcp_socket.async_write_some(boost::asio::buffer(reply, max_length),
            boost::bind(&session::upload, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }

    void upload(const boost::system::error_code& error,
        size_t bytes_transferred) // upload file to client
    {
        if (!error)
        {

            tcp_socket.async_write_some(boost::asio::buffer(endmsg),
            boost::bind(&session::download, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            cout << "else\n";
            count=0;
            tcp_socket.async_read_some(boost::asio::buffer(buf, max_length),
                boost::bind(&session::download, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
            delete this;
        }  
    }

     void download(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        try 
        {
            reply_length = tcp_socket.read_some(boost::asio::buffer(reply, max_length)); //size
        }
        catch (exception const & e)
        {
            cerr << "exception: " << e.what() << endl;
        }
    }

    tcp::socket tcp_socket;
    udp::socket udp_socket;
    enum { max_length = 1024 };
    char reply[max_length];
    char buf[max_length];
    int count=0, time_duration;

};
 
class server
{
public:
    server(boost::asio::io_service& io_service, short port)
        : io_service_(io_service),
        tcp_acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        udp_socket_(io_service, udp::endpoint(udp::v4(), port)),
        udp_resolver_(io_service)
    {
        start_tcp_accept();
    }
 
private:
    void start_tcp_accept()
    {
        session* new_session = new session(io_service_);
        tcp_acceptor_.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
    }
    
    void handle_accept(session* new_session,
        const boost::system::error_code& error)
    {
        if (!error)
        {
            new_session->start();
        }
        else
        {
            delete new_session;
        }
        start_tcp_accept();
    }

    boost::asio::io_service& io_service_;
    tcp::acceptor tcp_acceptor_;
    std::array<char, 5> recv_buffer_;
};
 
int main(int argc, char* argv[])
{
    try
    {
        /*
        CANNOT SERVICE MULTIPLE CLIENT AT THE SAME TIME
        */
        if (argc != 2)
        {
            std::cerr << "Usage: ./fb_server <port>\n";
            return 1;
        }
        boost::asio::io_service io_service;
    
        server s(io_service, atoi(argv[1]));

        io_service.run();

        cout << "run\n";
    }
    catch (exception& e)
    {
        cerr << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}