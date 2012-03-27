// -*- C++ -*-
//  Copyright (C) 2012, Vaclav Zeman. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modifica-
//  tion, are permitted provided that the following conditions are met:
//
//  1. Redistributions of  source code must  retain the above copyright  notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//  APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//  DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//  OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//  ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//  (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef LOG4CPLUS_NET_SOCKETS_SOCKET_H
#define LOG4CPLUS_NET_SOCKETS_SOCKET_H

#include <log4cplus/config.hxx>
#include <log4cplus/tstring.h>
#include <memory>
#include <string>


namespace log4cplus { namespace net {


enum ErrorSource
{
    EsNone
    , EsNotSupported
    , EsErrno
    , EsWin32GetLastError
};


class LOG4CPLUS_EXPORT Error
{
public:
    Error ();
    Error (tchar const *, ErrorSource, long);
    Error (Error const &);
    ~Error ();
    Error & operator = (Error const &);
    void swap (Error &);

    bool noerror () const;
    ErrorSource get_source () const;
    long get_error () const;
    tstring const & get_message () const;
    tstring const & get_origin () const;

private:
    ErrorSource error_source;
    long error_num;
    mutable std::auto_ptr<tstring> error_message;
    mutable std::auto_ptr<tstring> error_origin;
};


class LOG4CPLUS_EXPORT Socket
{
public:
    Socket ();
    Socket (const Socket &);
    virtual ~Socket ();
    Socket & operator = (Socket const &);

    void swap (Socket &);

    struct Data;

    Data & get_data ();
    Data const & get_data () const;

private:
    std::auto_ptr<Data> data;
};


enum AddressFamily
{
    AfUnspec
    , AfUnix
    , AfInet
    , AfInet6

    , PfUnspec = AfUnspec
    , PfUnix = AfUnix
    , PfInet = AfInet
    , PfInte6 = AfInet6
};


typedef AddressFamily ProtocolFamily;


enum SocketType
{
    StStream
    , StDgram
    , StRaw
    , StRdm
    , StSeqPacket
};


enum SocketLevel
{
    SolSocket
    , SolIpProtoIp
    , SolIpProtoIpv6
    , SolIpProtoIcmp
    , SolIpProtoRaw
    , SolIpProtoTcp
    , SolIpProtoUdp
};


enum SocketOption
{
    SoKeepAlive
    , SoLinger
    , SoReuseAddr
    , SoNoDelay
};


LOG4CPLUS_EXPORT Error create_socket (Socket &, AddressFamily, SocketType, int);
LOG4CPLUS_EXPORT Error set_option (Socket const &, SocketLevel, SocketOption,
    const void * option_value, std::size_t option_len);
LOG4CPLUS_EXPORT Error set_keep_alive (Socket const &, bool);
LOG4CPLUS_EXPORT Error set_linger (Socket const &, bool);
LOG4CPLUS_EXPORT Error set_reuse_addr (Socket const &, bool);
LOG4CPLUS_EXPORT Error set_no_delay (Socket const &, bool);


} } // namespace log4cplus { namespace net {

#endif // LOG4CPLUS_NET_SOCKETS_SOCKET_H
