#pragma once

#include <netinet/in.h>
#include <sstream>

namespace network
{

class Socket
{
    private:
        int port;
        int rcvbuf_size;
        int sndbuf_size;

        int local_fd;
        int remote_fd;
        sockaddr_in address;
        char * rcvbuf;
        char * sndbuf;

    public:
        Socket() = default;
        Socket(int port, int rcvbuf_size, int sndbuf_size);
        ~Socket();

        void accept();
        void bind();
        void connect(const char * ip);
        void listen(int backlog = 3);
        void open(int backlog = 3);
        std::stringstream receive();
        void send(std::stringstream & ss);
};

} // network