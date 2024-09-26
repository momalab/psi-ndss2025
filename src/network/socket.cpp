#include "socket.h"

#include <arpa/inet.h>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

namespace network
{

Socket::Socket(int port, int rcvbuf_size, int sndbuf_size)
{
    this->port = port;
    this->rcvbuf_size = rcvbuf_size;
    this->sndbuf_size = sndbuf_size;

    // Create a socket
    if ((this->local_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw "Socket failed to create a socket";

    // Set SO_REUSEADDR option
    int opt = 1;
    if (setsockopt(this->local_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw "Socket failed to set SO_REUSEADDR option";
    
    // Set the socket options
    if (setsockopt(this->local_fd, SOL_SOCKET, SO_RCVBUF, &this->rcvbuf_size, sizeof(this->rcvbuf_size)))
        throw "Socket failed to set the receive buffer size";
    if (setsockopt(this->local_fd, SOL_SOCKET, SO_SNDBUF, &this->sndbuf_size, sizeof(this->sndbuf_size)))
        throw "Socket failed to set the send buffer size";

    // Verify buffer sizes
    socklen_t oplen = sizeof(this->rcvbuf_size);
    if (getsockopt(this->local_fd, SOL_SOCKET, SO_RCVBUF, &this->rcvbuf_size, &oplen))
        throw "Socket failed to verify the receive buffer size";
    oplen = sizeof(this->sndbuf_size);
    if (getsockopt(this->local_fd, SOL_SOCKET, SO_SNDBUF, &this->sndbuf_size, &oplen))
        throw "Socket failed to verify the send buffer size";
    
    this->rcvbuf = new char[this->rcvbuf_size];
    this->sndbuf = new char[this->sndbuf_size];

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(this->port);
}

Socket::~Socket()
{
    if (this->remote_fd >= 0) try
    {
        shutdown(this->remote_fd, SHUT_RDWR);
        close(this->remote_fd);
    }
    catch(...) {}

    if (this->local_fd >= 0) try
    {
        shutdown(this->local_fd, SHUT_RDWR);
        close(this->local_fd);
    }
    catch(...) {}

    try
    {
        delete[] this->rcvbuf;
        delete[] this->sndbuf;
    }
    catch(...) {}
}

void Socket::accept()
{
    int addrlen = sizeof(address);

    // Accept an incoming connection
    if ((this->remote_fd = ::accept(this->local_fd, (sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        throw "Socket failed to accept an incoming connection";
}

void Socket::bind()
{
    // Bind the socket to the address
    if (::bind(this->local_fd, (sockaddr *)&address, sizeof(address)) < 0)
        throw "Socket failed to bind the socket to the address";
}

void Socket::connect(const char * ip)
{
    // Convert the IP address to binary form
    if (!inet_pton(AF_INET, ip, &address.sin_addr))
        throw "Socket failed to convert the IP address to binary form";

    // Connect to the address
    if (::connect(this->local_fd, (sockaddr *)&address, sizeof(address)) < 0)
        throw "Socket failed to connect to the address";

    this->remote_fd = this->local_fd; // set remote_fd to local_fd for client
}

void Socket::listen(int backlog)
{
    // Listen for incoming connections
    if (::listen(this->local_fd, backlog) < 0)
        throw "Socket failed to listen for incoming connections";
}

void Socket::open(int backlog)
{
    this->bind();
    this->listen(backlog);
    this->accept();
}

stringstream Socket::receive()
{
    // Receive data stream size
    int valread, data_size;
    if ((valread = read(this->remote_fd, &data_size, sizeof(data_size))) < 0)
        throw "Socket failed to receive the data stream size";
    
    // Receive data stream
    stringstream ss;
    while (data_size > 0)
    {
        int size = min(data_size, this->rcvbuf_size);
        if ((valread = read(this->remote_fd, this->rcvbuf, size)) < 0)
            throw "Socket failed to receive the data stream";
        ss.write(this->rcvbuf, valread);
        data_size -= valread;
    }

    return ss;
}

void Socket::send(stringstream & ss)
{
    // Get data stream size
    ss.seekg(0, ios::end);
    int data_size = ss.tellg();

    // Send data stream size
    if (write(this->remote_fd, &data_size, sizeof(data_size)) < 0)
        throw "Socket failed to send the data stream size";
    
    // Send data stream
    ss.seekg(0, ios::beg); // Reset the stream position
    while (data_size > 0)
    {
        int size = min(data_size, this->sndbuf_size);
        ss.read(this->sndbuf, size);
        if (write(this->remote_fd, this->sndbuf, size) < 0)
            throw "Socket failed to send the data stream";
        data_size -= size;
    }
}

} // network