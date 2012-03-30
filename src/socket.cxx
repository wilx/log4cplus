// Module:  Log4CPLUS
// File:    socket-win32.cxx
// Created: 4/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2010 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/helpers/loglog.h>
#include <log4cplus/internal/socket.h>


namespace log4cplus { namespace helpers {


//////////////////////////////////////////////////////////////////////////////
// AbstractSocket ctors and dtor
//////////////////////////////////////////////////////////////////////////////

AbstractSocket::AbstractSocket()
: sock(),
  state(not_opened),
  err(0)
{
}



AbstractSocket::AbstractSocket(net::Socket sock_,
    SocketState state_, int err_)
: sock(sock_),
  state(state_),
  err(err_)
{
}



AbstractSocket::AbstractSocket(const AbstractSocket& rhs)
{
    copy(rhs);
}


AbstractSocket::~AbstractSocket()
{
    close();
}



//////////////////////////////////////////////////////////////////////////////
// AbstractSocket methods
//////////////////////////////////////////////////////////////////////////////

void
AbstractSocket::close()
{
    if (sock.initialized ())
        close_socket (sock);
}



bool
AbstractSocket::isOpen() const
{
    return sock.initialized ();
}




AbstractSocket&
AbstractSocket::operator=(const AbstractSocket& rhs)
{
    if(&rhs != this) {
        close();
        copy(rhs);
    }

    return *this;
}



void
AbstractSocket::copy(const AbstractSocket& r)
{
    AbstractSocket& rhs = const_cast<AbstractSocket&>(r);
    sock = rhs.sock;
    state = rhs.state;
    err = rhs.err;
    rhs.sock = net::Socket ();
    rhs.state = not_opened;
    rhs.err = 0;
}



//////////////////////////////////////////////////////////////////////////////
// Socket ctors and dtor
//////////////////////////////////////////////////////////////////////////////

Socket::Socket()
    : AbstractSocket()
{ }


Socket::Socket(const tstring& address, unsigned short port)
    : AbstractSocket()
{
    // 

    net::SocketAddrIn server;
    net::Socket sock;
    net::Error retval;

    //

    //    struct sockaddr_in server;
    //int sock;
    //int retval;

    //std::memset (&server, 0, sizeof (server));
    retval = get_host_by_name (LOG4CPLUS_TSTRING_TO_STRING(hostn).c_str(),
        0, &server);
    if (retval != 0)
        return INVALID_SOCKET_VALUE;

    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        return INVALID_SOCKET_VALUE;
    }

    socklen_t namelen = sizeof (server);
    while (
        (retval = ::connect(sock, reinterpret_cast<struct sockaddr*>(&server),
            namelen))
        == -1
        && (errno == EINTR))
        ;
    if (retval == INVALID_OS_SOCKET_VALUE) 
    {
        ::close(sock);
        return INVALID_SOCKET_VALUE;
    }

    state = ok;
    return to_log4cplus_socket (sock);
    

    // orig code

    sock = connectSocket(address, port, state);
    if (! sock.initialized ())
        goto error;

    net::set_no_delay (sock, true);
    if (setTCPNoDelay (sock, true) != 0)
        goto error;

    return;

error:
    err = get_last_socket_error ();
}


Socket::Socket(net::Socket sock_, SocketState state_, int err_)
    : AbstractSocket(sock_, state_, err_)
{ }


Socket::~Socket()
{ }



//////////////////////////////////////////////////////////////////////////////
// Socket methods
//////////////////////////////////////////////////////////////////////////////

bool
Socket::read(SocketBuffer& buffer)
{
    long retval = helpers::read(sock, buffer);
    if(retval <= 0) {
        close();
    }
    else {
        buffer.setSize(retval);
    }

    return (retval > 0);
}



bool
Socket::write(const SocketBuffer& buffer)
{
    long retval = helpers::write(sock, buffer);
    if(retval <= 0) {
        close();
    }

    return (retval > 0);
}




//////////////////////////////////////////////////////////////////////////////
// ServerSocket ctor and dtor
//////////////////////////////////////////////////////////////////////////////

ServerSocket::ServerSocket(unsigned short port)
{
    sock = openSocket(port, state);
    if(sock == INVALID_SOCKET_VALUE) {
        err = get_last_socket_error ();
    }
}



ServerSocket::~ServerSocket()
{
}



//////////////////////////////////////////////////////////////////////////////
// ServerSocket methods
//////////////////////////////////////////////////////////////////////////////

Socket
ServerSocket::accept()
{
    SocketState st = not_opened;
    net::Socket clientSock = acceptSocket(sock, st);
    return Socket(clientSock, st, 0);
}


} } // namespace log4cplus { namespace helpers {
