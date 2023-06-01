
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
        time_duration = atoi(reply);
        cout << time_duration << endl;

        tcp_socket.async_write_some(boost::asio::buffer(reply, max_length),
            boost::bind(&session::upload, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        cout << "UPLOAD START\n";

    }

    void upload(const boost::system::error_code& error,
        size_t bytes_transferred) // upload file to client
    {
        if (!error)
        {
            chrono::system_clock::time_point start = chrono::system_clock::now();
            auto endTime = chrono::system_clock::now();
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - start);

            while(time.count() < (time_duration+1)*1000){
                int i=0;
                while (i<max_length){
                    buf[i] = std::rand() % 10+48; // 0~9 rand number send
                    i++;
                }
                buf[max_length-2] = '\n';
                buf[max_length-1] = '\0';

                tcp_socket.write_some(boost::asio::buffer(buf, max_length));
                endTime = chrono::system_clock::now();
                time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - start);
                // cout << time.count() << " ";
            }
            sleep(1);
            string endmsg = "\r\n";
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
            cout << "DOWNLOAD START\n";
            // fstream fout("readfile_server.txt");
            char reply[max_length];
            int reply_length=0;
            const int LEN = 100; // 프로그레스바 길이  
            int i;  
            // float tick = (float)100/LEN; // 몇 %마다 프로그레스바 추가할지 계산 
            int bar_count; // 프로그레스바 갯수 저장 변수  
            float percent; // 퍼센트 저장 변수  
            long double throughput=0;
            int recv_size=0;

            chrono::system_clock::time_point start = chrono::system_clock::now();
            auto endTime = chrono::system_clock::now();
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - start);

            while(1) {
                // endTime = chrono::system_clock::now();
                // time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - start);

                reply_length = tcp_socket.read_some(boost::asio::buffer(reply, max_length)); //size
                if(reply_length <10) {
                    cout << "reply is small ";
                    break;
                }
                recv_size += reply_length;
            }
            cout << "\nbreak\n" ;
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
        start_udp_receive();
        
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

    void start_udp_receive()
    {
        udp_socket_.async_receive_from(
            boost::asio::buffer(recv_buffer_), udp_endpoint_,
            boost::bind(&server::handle_receive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void handle_receive(const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
    {
        if (!error)
        {
            // Echo the message back to the sender
            cout << recv_buffer_[0];
            cout << recv_buffer_[1] ;
            cout << recv_buffer_[2] ;
            cout << recv_buffer_[3] << "\n";

            udp_socket_.async_send_to(
                boost::asio::buffer(recv_buffer_), udp_endpoint_,
                boost::bind(&server::handle_send, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            start_udp_receive();
        }
    }
    void handle_send(const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
    {
        if (!error)
        {
            start_udp_receive();
        }
    }
    
    boost::asio::io_service& io_service_;
    tcp::acceptor tcp_acceptor_;
    udp::socket udp_socket_;
    udp::resolver udp_resolver_;
    udp::endpoint udp_endpoint_;
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