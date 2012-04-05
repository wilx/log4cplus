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

#ifndef LOG4CPLUS_HELPERS_SOCKET_WRAPPER_H
#define LOG4CPLUS_HELPERS_SOCKET_WRAPPER_H

#include <log4cplus/config.hxx>
#include <log4cplus/tstring.h>
#include <memory>
#include <string>


namespace log4cplus { namespace helpers { namespace net {


enum ErrorKind
{
    EkNone
    , EkNotSupported
    , EkErrno
    , EkWin32GetLastError
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

LOG4CPLUS_EXPORT int af_to_int (AddressFamily);


enum SocketType
{
    StUnspec
    , StStream
    , StDgram
    , StRaw
    , StRdm
    , StSeqPacket
};

LOG4CPLUS_EXPORT int st_to_int (SocketType);
LOG4CPLUS_EXPORT SocketType int_to_st (int);


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

LOG4CPLUS_EXPORT int sol_to_int (SocketLevel);


//! This enum specifies protocol information. It is used by, e.g.,
//! create_socket(), AddrInfo::get_proto(), etc.
enum Protocol
{
    IpProtoIp
    , IpProtoIpv6
    , IpProtoIcmp
    , IpProtoRaw
    , IpProtoTcp
    , IpProtoUdp
};

LOG4CPLUS_EXPORT int proto_to_int (Protocol p);
LOG4CPLUS_EXPORT Protocol int_to_proto (int);


enum SocketOption
{
    SoKeepAlive
    , SoLinger
    , SoReuseAddr
    , SoNoDelay
};

LOG4CPLUS_EXPORT int so_to_int (SocketOption);


enum ShutdownDirection
{
    ShutRd
    , ShutWr
    , ShutRdWr
};

LOG4CPLUS_EXPORT int sd_to_int (ShutdownDirection);


enum MsgFlags
{
    MsgEor        = 1 << 0
    , MsgNoSignal = 1 << 1
    , MsgPeek     = 1 << 2
    , MsgOob      = 1 << 3
    , MsgWaitAll  = 1 << 4
};

LOG4CPLUS_EXPORT int mf_to_int (MsgFlags);


enum AiFlags
{
    AiPassive       = 1 << 0
    , AiCanonName   = 1 << 1
    , AiNumericHost = 1 << 2
    , AiNumericServ = 1 << 3
    , AiV4Mapped    = 1 << 4
    , AiAll         = 1 << 5
    , AiAddrConfig  = 1 << 6
};

LOG4CPLUS_EXPORT int aif_to_int (AiFlags);


class LOG4CPLUS_EXPORT Error
{
public:
    Error ();
    Error (tchar const *, ErrorKind, long);
    Error (Error const &);
    ~Error ();
    Error & operator = (Error const &);
    void swap (Error &);

    bool error () const;
    bool no_error () const;
    ErrorKind get_source () const;
    long get_error () const;
    tstring const & get_message () const;
    tstring const & get_origin () const;

private:
    ErrorKind error_kind;
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
    bool initialized () const;

private:
    std::auto_ptr<Data> data;
};


class SockAddr;
class SockAddrIn;


class LOG4CPLUS_EXPORT SockAddr
{
public:
    struct Data;

    SockAddr ();
    SockAddr (Data const &);
    SockAddr (SockAddr const &);
    ~SockAddr ();
    SockAddr & operator = (SockAddr const &);

    void swap (SockAddr &);

    Data & get_data ();
    Data const & get_data () const;

private:
    std::auto_ptr<Data> data;
};


class LOG4CPLUS_EXPORT SockAddrIn
{
public:
    struct Data;

    SockAddrIn ();
    SockAddrIn (SockAddrIn const &);
    explicit SockAddrIn (SockAddr const &);
    ~SockAddrIn ();
    SockAddrIn & operator = (SockAddrIn const &);

    void swap (SockAddrIn &);

private:
    std::auto_ptr<Data> data;
};


class LOG4CPLUS_EXPORT AddrInfo
{
public:
    struct Data;

    AddrInfo ();
    AddrInfo (Data const &);
    ~AddrInfo ();
    AddrInfo (AddrInfo const &);
    AddrInfo & operator = (AddrInfo const &);
    
    void swap (AddrInfo &);
    AiFlags get_flags () const;
    AddressFamily get_family () const;
    SocketType get_socktype () const;
    Protocol get_proto () const;
    std::size_t get_socklen () const;
    SockAddr get_addr () const;

private:
    std::auto_ptr<Data> data;
};


LOG4CPLUS_EXPORT Error create_socket (Socket &, AddressFamily, SocketType,
    Protocol);
LOG4CPLUS_EXPORT Error close_socket (Socket const &);
LOG4CPLUS_EXPORT Error shutdown_socket (Socket const &, ShutdownDirection);
LOG4CPLUS_EXPORT Error bind_socket (Socket const &, SockAddr const &,
    std::size_t);
LOG4CPLUS_EXPORT Error listen_on_socket (Socket const &, int);
LOG4CPLUS_EXPORT Error accept_on_socket (Socket &, SockAddr &, std::size_t &);
LOG4CPLUS_EXPORT Error send_on_socket (Socket const &, std::size_t &,
    void const *, std::size_t, MsgFlags);
LOG4CPLUS_EXPORT Error receive_from_socket (Socket const &, void *,
    std::size_t &, MsgFlags);

LOG4CPLUS_EXPORT Error set_option (Socket const &, SocketLevel, SocketOption,
    const void * option_value, std::size_t option_len);
LOG4CPLUS_EXPORT Error set_keep_alive (Socket const &, bool = true);
LOG4CPLUS_EXPORT Error set_linger (Socket const &, bool = true);
LOG4CPLUS_EXPORT Error set_reuse_addr (Socket const &, bool = true);
LOG4CPLUS_EXPORT Error set_no_delay (Socket const &, bool = true);


} } } // namespace log4cplus { namespace helpers { namespace net {

#endif // LOG4CPLUS_HELPERS_SOCKET_WRAPPER_H
