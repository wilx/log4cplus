// Module:  Log4CPLUS
// File:    socket-unix.cxx
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


#include <log4cplus/config.hxx>
#if defined (LOG4CPLUS_USE_BSD_SOCKETS)

#include <cstring>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <log4cplus/internal/socket.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/spi/loggingevent.h>

#include <log4cplus/helpers/socket-wrapper.h>
#include <log4cplus/streams.h>
#include <sstream>

#include <arpa/inet.h>
 
#ifdef LOG4CPLUS_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef LOG4CPLUS_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if defined (LOG4CPLUS_HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined (LOG4CPLUS_HAVE_NETINET_TCP_H)
#include <netinet/tcp.h>
#endif

#include <errno.h>

#ifdef LOG4CPLUS_HAVE_NETDB_H
#include <netdb.h>
#endif

#include <unistd.h>


namespace
{


//! Helper for accept_wrap().
template <typename T, typename U>
struct socklen_var
{
    typedef T type;
};


template <typename U>
struct socklen_var<void, U>
{
    typedef U type;
};


template <typename T>
static inline
void
swap_auto_ptrs (std::auto_ptr<T> & a, std::auto_ptr<T> & b)
{ 
    T * tmp = a.release ();
    a = b;
    b.reset (tmp);   
}


template <typename T>
static inline
std::auto_ptr<T>
clone_auto_ptr (std::auto_ptr<T> const & ptr)
{
    if (ptr.get ())
        return std::auto_ptr<T> (new T (*ptr));
    else
        return std::auto_ptr<T> ();
}


} // namespace


namespace log4cplus { namespace helpers { namespace net {

//
//
//

int
af_to_int (AddressFamily af)
{
    switch (af)
    {
    default:
        throw Error ("af_to_int", EkNotSupported, af);

    case AfUnspec:
#ifdef AF_UNSPEC
        return AF_UNSPEC;
#else
        return 0;
#endif

#ifdef AF_UNIX
    case AfUnix:
        return AF_UNIX;
#endif

    case AfInet:
        return AF_INET;

#ifdef AF_INET6
    case AfInet6:
        return AF_INET6;
#endif
    }
}


AddressFamily
int_to_af (int af)
{
    switch (af)
    {
    default:
        // TODO: Better handling of unknown values.
#ifdef AF_UNSPEC
    case AF_UNSPEC:
#endif
        return AfUnspec;

#ifdef AF_UNIX
    case AF_UNIX:
        return AfUnix;
#endif

    case AF_INET:
        return AfInet;

#ifdef AF_INET6
    case AF_INET6:
        return AfInet6;
#endif
    }
}


int
st_to_int (SocketType st)
{
    switch (st)
    {
    default:
        throw Error ("st_to_int", EkNotSupported, st);

    case StUnspec:
        return 0;

    case StStream:
        return SOCK_STREAM;
        
    case StDgram:
        return SOCK_DGRAM;

#ifdef SOCK_RAW
    case StRaw:
        return SOCK_RAW;
#endif
     
#ifdef SOCK_RDM   
    case StRdm:
        return SOCK_RDM;
#endif
      
#ifdef SOCK_SEQPACKET  
    case StSeqPacket:
        return SOCK_SEQPACKET;
#endif
    }
}


SocketType
int_to_st (int st)
{
    switch (st)
    {
    default:
        throw Error ("int_to_st", EkNotSupported, st);

    case SOCK_STREAM:
        return StStream;

    case SOCK_DGRAM:
        return StDgram;

#ifdef SOCK_RAW
    case SOCK_RAW:
        return StRaw;
#endif

#ifdef SOCK_RDM
    case SOCK_RDM:
        return StRdm;
#endif

#ifdef SOCK_SEQPACKET
    case SOCK_SEQPACKET:
        return StSeqPacket;
#endif
    }
}


int
sol_to_int (SocketLevel sl)
{
    static int const table[] = {
        SOL_SOCKET
#if defined (SOL_IP)
        , SOL_IP
#else
        , IPPROTO_IP
#endif

#if defined (SOL_IPV6)
        , SOL_IPV6
#else
        , IPPROTO_IPV6
#endif

        , IPPROTO_ICMP

#if defined (SOL_RAW)
        , SOL_RAW
#else
        , IPPROTO_RAW
#endif

#if defined (SOL_TCP)
        , SOL_TCP
#else
        , IPPROTO_TCP
#endif

#if defined (SOL_UDP)
        , SOL_UDP
#else
        , IPPROTO_UDP
#endif
    };

    if (+sl >= sizeof (table) / sizeof (table[0]))
        return -1;
    
    return table[sl];
}


int
so_to_int (SocketOption so)
{
    switch (so)
    {
    default:
        throw Error ("so_to_int", EkNotSupported, so);

#ifdef SO_KEEPALIVE
    case SoKeepAlive:
        return SO_KEEPALIVE;
#endif

#ifdef SO_LINGER
    case SoLinger:
        return SO_LINGER;
#endif

#ifdef SO_REUSEADDR
    case SoReuseAddr:
        return SO_REUSEADDR;
#endif

#ifdef SO_NODELAY
    case SoNoDelay:
        return SO_NODELAY;
#endif       
    }
}


int
sd_to_int (ShutdownDirection sd)
{
    switch (sd)
    {
    default:
        throw Error ("sd_to_int", EkNotSupported, sd);

    case ShutRd:
        return SHUT_RD;

    case ShutWr:
        return SHUT_WR;

    case ShutRdWr:
        return SHUT_RDWR;
    }
}


template <typename FlagsDest, typename FlagDest, typename FlagsSrc
    , typename FlagSrc>
static inline
void
set_if_set (FlagsDest & dest, FlagDest dest_flag, FlagsSrc f, FlagSrc flag)
{
    if ((f & flag) != 0)
        dest |= dest_flag;
}


#define LOG4CPLUS_UNHANDLED_FLAG(var, f)  \
do {                                      \
    if (((var) & (f)) != 0)               \
        return -1;                        \
} while (0)


int
mf_to_int (MsgFlags mf)
{
    int ret = 0;

#if defined (MSG_EOR)
    set_if_set (ret, MSG_EOR, mf, MsgEor);
#else
    // TODO: Handle unimplemented flags in callers.
    LOG4CPLUS_UNHANDLED_FLAG (mf, MsgEor);
#endif
    set_if_set (ret, MSG_NOSIGNAL, mf, MsgNoSignal);
    set_if_set (ret, MSG_PEEK, mf, MsgPeek);
    set_if_set (ret, MSG_OOB, mf, MsgOob);
    set_if_set (ret, MSG_WAITALL, mf, MsgWaitAll);

    return ret;
}


int
aif_to_int (AiFlags aif)
{
    int ret = 0;

#if defined (AI_PASSIVE)
    set_if_set (ret, AI_PASSIVE, aif, AiPassive);
#else
    LOG4CPLUS_UNHANDLED_FLAG (aif, AiPassive);
#endif

#if defined (AI_CANONNAME)
    set_if_set (ret, AI_CANONNAME, aif, AiCanonName);
#else
    LOG4CPLUS_UNHANDLED_FLAG (aif, AiCanonName);
#endif

#if defined (AI_NUMERICHOST)
    set_if_set (ret, AI_NUMERICHOST, aif, AiNumericHost);
#else
    LOG4CPLUS_UNHANDLED_FLAG (aif, AiNumericHost);
#endif

#if defined (AI_V4MAPPED)
    set_if_set (ret, AI_V4MAPPED, aif, AiV4Mapped);
#else
    LOG4CPLUS_UNHANDLED_FLAG (aif, AiV4Mapped);
#endif

#if defined (AI_ALL)
    set_if_set (ret, AI_ALL, aif, AiAll);
#else
    LOG4CPLUS_UNHANDLED_FLAG (aif, AiAll);
#endif

#if defined (AI_ADDRCONFIG)
    set_if_set (ret, AI_ADDRCONFIG, aif, AiAddrConfig);
#else
    LOG4CPLUS_UNHANDLED_FLAG (aif, AiAddrConfig);
#endif

    return ret;
}


AiFlags
int_to_aif (int aif)
{
    int ret = 0;

#if defined (AI_PASSIVE)
    set_if_set (ret, AiPassive, aif, AI_PASSIVE);
#endif

#if defined (AI_CANONNAME)
    set_if_set (ret, AiCanonName, aif, AI_CANONNAME);
#endif

#if defined (AI_NUMERICHOST)
    set_if_set (ret, AiNumericHost, aif, AI_NUMERICHOST);
#endif

#if defined (AI_V4MAPPED)
    set_if_set (ret, AiV4Mapped, aif, AI_V4MAPPED);
#endif

#if defined (AI_ALL)
    set_if_set (ret, AiAll, aif, AI_ALL);
#endif

#if defined (AI_ADDRCONFIG)
    set_if_set (ret, AiAddrConfig, aif, AI_ADDRCONFIG);
#endif

    return AiFlags (ret);
}


int
proto_to_int (Protocol p)
{
    switch (p)
    {
    default:
        throw Error ("proto_to_int", EkNotSupported, +p);

#ifdef IPPROTO_IP
    case IpProtoIp:
        return IPPROTO_IP;
#endif

#ifdef IPPROTO_IPV6
    case IpProtoIpv6:
        return IPPROTO_IPV6;
#endif

#ifdef IPPROTO_ICMP
    case IpProtoIcmp:
        return IPPROTO_ICMP;
#endif

#ifdef IPPROTO_RAW
    case IpProtoRaw:
        return IPPROTO_RAW;
#endif

#ifdef IPPROTO_TCP
    case IpProtoTcp:
        return IPPROTO_TCP;
#endif

#ifdef IPPROTO_UDP
    case IpProtoUdp:
        return IPPROTO_UDP;
#endif
    }
}


Protocol
int_to_proto (int p)
{
    switch (p)
    {
    default:
        throw Error ("int_to_proto", EkNotSupported, p);

#ifdef IPPROTO_IP
    case IPPROTO_IP:
        return IpProtoIp;
#endif

#ifdef IPPROTO_IPV6
    case IPPROTO_IPV6:
        return IpProtoIpv6;
#endif

#ifdef IPPROTO_ICMP
    case IPPROTO_ICMP:
        return IpProtoIcmp;
#endif

#ifdef IPPROTO_RAW
    case IPPROTO_RAW:
        return IpProtoRaw;
#endif

#ifdef IPPROTO_TCP
    case IPPROTO_TCP:
        return IpProtoTcp;
#endif

#ifdef IPPROTO_UDP
    case IPPROTO_UDP:
        return IpProtoUdp;
#endif
    }
}


//
//
//

tstring
format_errno_str (int eno)
{
    tostringstream oss;

    oss << LOG4CPLUS_TEXT ("errno ") << eno << LOG4CPLUS_TEXT (": ")
        << LOG4CPLUS_C_STR_TO_TSTRING (std::strerror (eno));
    
    return oss.str ();
}


static
void
fill_error_message (std::auto_ptr<tstring> & str, ErrorKind ek, long eno)
{
    switch (ek)
    {
    case EkNone:
    default:
        str.reset (new tstring);
        break;

    case EkNotSupported:
        str.reset (new tstring (LOG4CPLUS_TEXT ("not supported")));
        break;

    case EkErrno:
        str.reset (new tstring (format_errno_str (static_cast<int>(eno))));
        break;
    }
}


Error::Error ()
    : error_kind (EkNone)
    , error_num (0)
{ }


Error::Error (tchar const * origin, ErrorKind ek, long eno)
    : error_kind (ek)
    , error_num (eno)
    , error_origin (new tstring (origin))
{
    fill_error_message (error_message, error_kind, error_num);
}


Error::Error (Error const & other)
    : error_kind (other.error_kind)
    , error_num (other.error_num)
    , error_message (clone_auto_ptr (other.error_message))
    , error_origin (clone_auto_ptr (other.error_origin))
{ }


Error::~Error ()
{ }


Error &
Error::operator = (Error const & other)
{
    Error (other).swap (*this);
    return *this;
}


void
Error::swap (Error & other)
{
    using std::swap;
    swap (error_kind, other.error_kind);
    swap (error_num, other.error_num);
    swap_auto_ptrs (error_message, other.error_message);
    swap_auto_ptrs (error_origin, other.error_origin);
}


bool
Error::error () const
{
    return error_kind != EkNone;
}


bool
Error::no_error () const
{
    return error_kind == EkNone;
}


ErrorKind
Error::get_source () const
{
    return error_kind;
}


long
Error::get_error () const
{
    return error_num;
}


tstring const &
Error::get_message () const
{
    if (! error_message.get ())
        fill_error_message (error_message, error_kind, error_num);

    return *error_message;
}


tstring const &
Error::get_origin () const
{
    if (! error_origin.get ())
        error_origin.reset (new tstring);

    return *error_origin;
}


//
//
//

struct Socket::Data
{
    Data ();

    mutable int socket;
};


Socket::Data::Data ()
    : socket (-1)
{ }


//
//
//

Socket::Socket ()
    : data (new Data)
{ }


Socket::Socket (Socket const & other)
    : data (new Data (*other.data))
{ }


Socket::~Socket ()
{ }


Socket &
Socket::operator = (Socket const & other)
{
    Socket (other).swap (*this);
    return *this;
}


void
Socket::swap (Socket & other)
{
    swap_auto_ptrs (data, other.data);
}


Socket::Data &
Socket::get_data ()
{
    return *data;
}


Socket::Data const &
Socket::get_data () const
{
    return *data;
}


bool
Socket::initialized () const
{
    return data->socket != -1;
}


//
//
//

struct SockAddr::Data
{
    Data ();
    Data (sockaddr const &);

    sockaddr addr;
};


SockAddr::Data::Data ()
    : addr (sockaddr ())
{ }


SockAddr::Data::Data (sockaddr const & sa)
    : addr (sa)
{ }


//
//
//

SockAddr::SockAddr ()
    : data (new SockAddr::Data)
{ }


SockAddr::SockAddr (SockAddr::Data const & d)
    : data (new SockAddr::Data (d))
{ }


SockAddr::SockAddr (SockAddr const & other)
    : data (clone_auto_ptr (other.data))
{ }


SockAddr::~SockAddr ()
{ }


SockAddr &
SockAddr::operator = (SockAddr const & other)
{
    SockAddr (other).swap (*this);
    return *this;
}


void
SockAddr::swap (SockAddr & other)
{
    using std::swap;
    swap (*data, *other.data);
}


SockAddr::Data &
SockAddr::get_data ()
{
    return *data;
}


SockAddr::Data const &
SockAddr::get_data () const
{
    return *data;
}


//
//
//

struct SockAddrIn::Data
{
    Data ();

    sockaddr_in addr;
};


SockAddrIn::Data::Data ()
{
    std::memset (&addr, 0, sizeof (addr));
}


//
//
//

SockAddrIn::SockAddrIn ()
    : data (new SockAddrIn::Data)
{ }


SockAddrIn::SockAddrIn (SockAddrIn const & other)
    : data (clone_auto_ptr (other.data))
{ }


SockAddrIn::SockAddrIn (SockAddr const & sa)
    : data (new SockAddrIn::Data)
{
    std::memcpy (&data->addr, &sa.get_data ().addr, sizeof (sockaddr_in));
}


SockAddrIn::~SockAddrIn ()
{ }


SockAddrIn &
SockAddrIn::operator = (SockAddrIn const & other)
{
    SockAddrIn (other).swap (*this);
    return *this;
}


void
SockAddrIn::swap (SockAddrIn & other)
{
    using std::swap;
    swap (*data, *other.data);
}


//
//
//

struct AddrInfo::Data
{
    Data ();
    Data (addrinfo const &);

    addrinfo info;
};


AddrInfo::Data::Data ()
{
    std::memset (&info, 0, sizeof (info));
}


AddrInfo::Data::Data (addrinfo const & ai)
    : info (ai)
{ }


//
//
//

AddrInfo::AddrInfo ()
    : data (new AddrInfo::Data)
{ }


AddrInfo::AddrInfo (AddrInfo::Data const & d)
    : data (new AddrInfo::Data (d))
{ }


AddrInfo::AddrInfo (AddrInfo const & other)
    : data (clone_auto_ptr (other.data))
{ }


AddrInfo::~AddrInfo ()
{ }


AddrInfo &
AddrInfo::operator = (AddrInfo const & other)
{
    AddrInfo (other).swap (*this);
    return *this;
}


void
AddrInfo::swap (AddrInfo & other)
{
    swap_auto_ptrs (data, other.data);
}


AiFlags
AddrInfo::get_flags () const
{
    return int_to_aif (data->info.ai_flags);
}


AddressFamily
AddrInfo::get_family () const
{
    return int_to_af (data->info.ai_family);
}


SocketType
AddrInfo::get_socktype () const
{
    return int_to_st (data->info.ai_socktype);
}


Protocol
AddrInfo::get_proto () const
{
    return int_to_proto (data->info.ai_protocol);
}


std::size_t
AddrInfo::get_socklen () const
{
    return static_cast<std::size_t>(data->info.ai_protocol);
}


SockAddr
AddrInfo::get_addr () const
{
    return SockAddr (SockAddr::Data (*data->info.ai_addr));
}


//
//
//

Error
create_socket (Socket & sock, AddressFamily af, SocketType st, int proto)
{
    Socket::Data & sd = sock.get_data ();
    
    sd.socket = socket (af_to_int (af), st_to_int (st), proto);
    if (sd.socket == -1)
        return Error (LOG4CPLUS_TEXT ("socket"), EkErrno, errno);
    
    return Error ();
}


Error
close_socket (Socket const & socket)
{
    Socket::Data const & sd = socket.get_data ();

    int ret = close (sd.socket);
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("close"), EkErrno, errno);

    sd.socket = -1;

    return Error ();
}


Error
shutdown_socket (Socket const & socket, ShutdownDirection dir)
{
    Socket::Data const & sd = socket.get_data ();

    int ret = shutdown (sd.socket, sd_to_int (dir));
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("shutdown"), EkErrno, errno);

    return Error ();
}

template <typename addr_ptr_type, typename socklen_type>
static
int
bind_wrap (
    int (* bind_func) (int, addr_ptr_type, socklen_type),
    int socket, sockaddr const * addr, std::size_t addr_len)
{
    return bind_func (socket, reinterpret_cast<addr_ptr_type>(addr),
        static_cast<socklen_type>(addr_len));
}


Error
bind_socket (Socket const & socket, SockAddr const & addr, std::size_t addr_len)
{
    Socket::Data const & sd = socket.get_data ();
    int ret = bind_wrap (&bind, sd.socket, &addr.get_data ().addr, addr_len);
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("bind"), EkErrno, errno);

    return Error ();
}


Error
listen_on_socket (Socket const & socket, int backlog)
{
    Socket::Data const & sd = socket.get_data ();
    int ret = listen (sd.socket, backlog);
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("listen"), EkErrno, errno);

    return Error ();
}


template <typename accept_sockaddr_ptr_type, typename accept_socklen_type>
static
int
accept_wrap (
    int (* accept_func) (int, accept_sockaddr_ptr_type, accept_socklen_type *),
    int socket, sockaddr * sa, std::size_t * len)
{
    typedef typename socklen_var<accept_socklen_type, socklen_t>::type
        socklen_var_type;
    socklen_var_type l = static_cast<socklen_var_type>(*len);
    int result = accept_func (socket,
        reinterpret_cast<accept_sockaddr_ptr_type>(sa),
        reinterpret_cast<accept_socklen_type *>(&l));
    *len = static_cast<socklen_t>(l);
    return result;
}


Error
accept_on_socket (Socket & newsocket, Socket & socket, SockAddr & sa,
    std::size_t & socklen)
{
    Socket::Data & sd = socket.get_data ();
    int ret = accept_wrap (&accept, sd.socket, &sa.get_data ().addr, &socklen);
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("accept"), EkErrno, errno);

    Socket::Data & newsd = newsocket.get_data ();
    newsd.socket = ret;

    return Error ();    
}


template <typename send_ret_type, typename buffer_len_type>
static
long
send_wrap (
    send_ret_type (* send_func) (int, void const *, buffer_len_type, int),
    int socket, void const * buf, std::size_t len, int flags)
{
    return static_cast<long>(
        send_func (socket, buf, static_cast<buffer_len_type>(len),
            flags));
}


Error
send_on_socket (Socket const & socket, std::size_t & sent, void const * buf,
    std::size_t buf_size, MsgFlags flags)
{
    Socket::Data const & sd = socket.get_data ();
    long ret = send_wrap (&send, sd.socket, buf, buf_size, mf_to_int (flags));
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("send"), EkErrno, errno);

    sent = static_cast<std::size_t>(ret);

    return Error ();
}


template <typename recv_ret_type, typename buffer_len_type>
static
long
recv_wrap (
    recv_ret_type (* recv_func) (int, void *, buffer_len_type, int),
    int socket, void * buf, std::size_t len, int flags)
{
    return static_cast<long>(
        recv_func (socket, buf, static_cast<buffer_len_type>(len),
            flags));
}


Error
receive_from_socket (Socket const & socket, void * buf, std::size_t buffer_len,
    std::size_t & received, MsgFlags flags)
{
    Socket::Data const & sd = socket.get_data ();
    long ret = recv_wrap (&recv, sd.socket, buf, buffer_len,
        mf_to_int (flags));
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("recv"), EkErrno, errno);

    received = static_cast<std::size_t>(ret);

    return Error ();
}


template <typename option_ptr_type, typename socklen_type>
static
int
setsockopt_wrap (
    int (* setsockopt_func) (int, int, int, option_ptr_type, socklen_type),
    int socket, int level, int option, const void * option_value,
    std::size_t option_len)
{
    return setsockopt_func (socket, level, option,
        (option_ptr_type)(option_value),
        static_cast<socklen_type>(option_len));
}


Error
set_option (Socket const & socket, SocketLevel level,
    SocketOption option, const void * option_value, std::size_t option_len)
{
    Socket::Data const & sd = socket.get_data ();
    int ret = setsockopt_wrap (&setsockopt, sd.socket, sol_to_int (level),
        so_to_int (option), option_value, option_len);
    if (ret == -1)
        return Error (LOG4CPLUS_TEXT ("setsockopt"), EkErrno, errno);

    return Error ();
}


static
Error
set_bool_option (Socket const & socket, SocketLevel level, SocketOption option,
    bool val)
{
    int intval = static_cast<int>(val);
    return set_option (socket, level, option, &intval,
        sizeof (intval));

}


Error
set_keep_alive (Socket const & socket, bool val)
{
    return set_bool_option (socket, SolSocket, SoKeepAlive, val);
}


Error
set_linger (Socket const & socket, bool val)
{
    return set_bool_option (socket, SolSocket, SoLinger, val);
}


Error
set_reuse_addr (Socket const & socket, bool val)
{
    return set_bool_option (socket, SolSocket, SoReuseAddr, val);
}


Error
set_no_delay (Socket const & socket, bool val)
{
    return set_bool_option (socket, SolIpProtoTcp, SoNoDelay, val);
}


} // namespace net


namespace
{


#if ! defined (LOG4CPLUS_SINGLE_THREADED)
// We need to use log4cplus::thread here to work around compilation
// problem on AIX.
static log4cplus::thread::Mutex ghbn_mutex;

#endif


static
int
get_host_by_name (char const * hostname, std::string * name,
    struct sockaddr_in * addr)
{
#if defined (LOG4CPLUS_HAVE_GETADDRINFO)
    struct addrinfo hints;
    std::memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_CANONNAME;

    if (inet_addr (hostname) != static_cast<in_addr_t>(-1))
        hints.ai_flags |= AI_NUMERICHOST;

    struct addrinfo * res = 0;
    int ret = getaddrinfo (hostname, 0, &hints, &res);
    if (ret != 0)
        return ret;

    struct addrinfo const & ai = *res;
    assert (ai.ai_family == AF_INET);
    
    if (name)
        *name = ai.ai_canonname;

    if (addr)
        std::memcpy (addr, ai.ai_addr, ai.ai_addrlen);

    freeaddrinfo (res);

#else
    #if ! defined (LOG4CPLUS_SINGLE_THREADED)
    // We need to use log4cplus::thread here to work around
    // compilation problem on AIX.
    log4cplus::thread::MutexGuard guard (ghbn_mutex);

    #endif

    struct ::hostent * hp = gethostbyname (hostname);
    if (! hp)
        return 1;
    assert (hp->h_addrtype == AF_INET);

    if (name)
        *name = hp->h_name;

    if (addr)
    {
        assert (hp->h_length <= sizeof (addr->sin_addr));
        std::memcpy (&addr->sin_addr, hp->h_addr_list[0], hp->h_length);
    }

#endif

    return 0;
}


} // namespace


/////////////////////////////////////////////////////////////////////////////
// Global Methods
/////////////////////////////////////////////////////////////////////////////

SOCKET_TYPE
openSocket(unsigned short port, SocketState& state)
{
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        return INVALID_SOCKET_VALUE;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    int optval = 1;
    socklen_t optlen = sizeof (optval);
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &optval, optlen );

    int retval = bind(sock, reinterpret_cast<struct sockaddr*>(&server),
        sizeof(server));
    if (retval < 0)
        return INVALID_SOCKET_VALUE;

    if (::listen(sock, 10))
        return INVALID_SOCKET_VALUE;

    state = ok;
    return to_log4cplus_socket (sock);
}


SOCKET_TYPE
connectSocket(const tstring& hostn, unsigned short port, SocketState& state)
{
    struct sockaddr_in server;
    int sock;
    int retval;

    std::memset (&server, 0, sizeof (server));
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
}


namespace
{




// Some systems like HP-UX have socklen_t but accept() does not use it
// as type of its 3rd parameter. This wrapper works around this
// incompatibility.
template <typename accept_sockaddr_ptr_type, typename accept_socklen_type>
static
SOCKET_TYPE
accept_wrap (
    int (* accept_func) (int, accept_sockaddr_ptr_type, accept_socklen_type *),
    SOCKET_TYPE sock, struct sockaddr * sa, socklen_t * len)
{
    typedef typename socklen_var<accept_socklen_type, socklen_t>::type
        socklen_var_type;
    socklen_var_type l = static_cast<socklen_var_type>(*len);
    SOCKET_TYPE result
        = static_cast<SOCKET_TYPE>(
            accept_func (sock, sa,
                reinterpret_cast<accept_socklen_type *>(&l)));
    *len = static_cast<socklen_t>(l);
    return result;
}


} // namespace


SOCKET_TYPE
acceptSocket(SOCKET_TYPE sock, SocketState& state)
{
    struct sockaddr_in net_client;
    socklen_t len = sizeof(struct sockaddr);
    int clientSock;

    while(
        (clientSock = accept_wrap (accept, to_os_socket (sock),
            reinterpret_cast<struct sockaddr*>(&net_client), &len)) 
        == -1
        && (errno == EINTR))
        ;

    if(clientSock != INVALID_OS_SOCKET_VALUE) {
        state = ok;
    }

    return to_log4cplus_socket (clientSock);
}



int
closeSocket(SOCKET_TYPE sock)
{
    return ::close(to_os_socket (sock));
}



long
read(SOCKET_TYPE sock, SocketBuffer& buffer)
{
    long res, readbytes = 0;
 
    do
    { 
        res = ::read(to_os_socket (sock), buffer.getBuffer() + readbytes,
            buffer.getMaxSize() - readbytes);
        if( res <= 0 ) {
            return res;
        }
        readbytes += res;
    } while( readbytes < static_cast<long>(buffer.getMaxSize()) );
 
    return readbytes;
}



long
write(SOCKET_TYPE sock, const SocketBuffer& buffer)
{
#if defined(MSG_NOSIGNAL)
    int flags = MSG_NOSIGNAL;
#else
    int flags = 0;
#endif
    return ::send( to_os_socket (sock), buffer.getBuffer(), buffer.getSize(),
        flags );
}


tstring
getHostname (bool fqdn)
{
    char const * hostname = "unknown";
    int ret;
    std::vector<char> hn (1024, 0);

    while (true)
    {
        ret = ::gethostname (&hn[0], static_cast<int>(hn.size ()) - 1);
        if (ret == 0)
        {
            hostname = &hn[0];
            break;
        }
#if defined (LOG4CPLUS_HAVE_ENAMETOOLONG)
        else if (ret != 0 && errno == ENAMETOOLONG)
            // Out buffer was too short. Retry with buffer twice the size.
            hn.resize (hn.size () * 2, 0);
#endif
        else
            break;
    }

    if (ret != 0 || (ret == 0 && ! fqdn))
        return LOG4CPLUS_STRING_TO_TSTRING (hostname);

    std::string full_hostname;
    ret = get_host_by_name (hostname, &full_hostname, 0);
    if (ret == 0)
        hostname = full_hostname.c_str ();

    return LOG4CPLUS_STRING_TO_TSTRING (hostname);
}


int
setTCPNoDelay (SOCKET_TYPE sock, bool val)
{
#if (defined (SOL_TCP) || defined (IPPROTO_TCP)) && defined (TCP_NODELAY)
#if defined (SOL_TCP)
    int level = SOL_TCP;

#elif defined (IPPROTO_TCP)
    int level = IPPROTO_TCP;

#endif

    int result;
    int enabled = static_cast<int>(val);
    if ((result = setsockopt(sock, level, TCP_NODELAY, &enabled,
                sizeof(enabled))) != 0)
        set_last_socket_error (errno);
    
    return result;

#else
    // No recognizable TCP_NODELAY option.
    return 0;

#endif
}


} } // namespace log4cplus

#endif // LOG4CPLUS_USE_BSD_SOCKETS
