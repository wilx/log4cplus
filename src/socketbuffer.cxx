// Module:  Log4CPLUS
// File:    socketbuffer.cxx
// Created: 5/2003
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

#include <cstring>
#include <limits>
#include <log4cplus/helpers/socketbuffer.h>
#include <log4cplus/helpers/loglog.h>

#if !defined(_WIN32)
#  include <netdb.h>
#else
#  include <log4cplus/config/windowsh-inc.h>
#endif

#if defined (LOG4CPLUS_HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif


namespace log4cplus { namespace helpers {


//////////////////////////////////////////////////////////////////////////////
// SocketBuffer ctors and dtor
//////////////////////////////////////////////////////////////////////////////

SocketBuffer::SocketBuffer(std::size_t maxsize_)
: maxsize(maxsize_),
  size(0),
  pos(0),
  buffer(new char[maxsize])
{
}


SocketBuffer::~SocketBuffer()
{
    delete [] buffer;
}


//////////////////////////////////////////////////////////////////////////////
// SocketBuffer methods
//////////////////////////////////////////////////////////////////////////////

unsigned char
SocketBuffer::readByte()
{
    if(pos >= maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readByte()- end of buffer reached"));
        return 0;
    }
    else if((pos + sizeof(unsigned char)) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readByte()- Attempt to read beyond end of buffer"));
        return 0;
    }

    unsigned char ret = static_cast<unsigned char>(buffer[pos]);
    pos += sizeof(unsigned char);

    return ret;
}



unsigned short
SocketBuffer::readShort()
{
    if(pos >= maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readShort()- end of buffer reached"));
        return 0;
    }
    else if((pos + sizeof(unsigned short)) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readShort()- Attempt to read beyond end of buffer"));
        return 0;
    }

    unsigned short ret;
    std::memcpy(&ret, buffer + pos, sizeof(ret));
    ret = ntohs(ret);
    pos += sizeof(unsigned short);

    return ret;
}



unsigned int
SocketBuffer::readInt()
{
    if(pos >= maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readInt()- end of buffer reached"));
        return 0;
    }
    else if((pos + sizeof(unsigned int)) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readInt()- Attempt to read beyond end of buffer"));
        return 0;
    }

    unsigned int ret;
    std::memcpy (&ret, buffer + pos, sizeof(ret));
    ret = ntohl(ret);
    pos += sizeof(unsigned int);
    
    return ret;
}


tstring
SocketBuffer::readString(unsigned char sizeOfChar)
{
    std::size_t strlen = readInt();
    std::size_t bufferLen = strlen * sizeOfChar;

    if(strlen == 0) {
        return tstring();
    }
    if(pos > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readString()- end of buffer reached"));
        return tstring();
    }

    if((pos + bufferLen) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readString()- Attempt to read beyond end of buffer"));
        bufferLen = (maxsize - 1) - pos;
        strlen = bufferLen / sizeOfChar;
    }

#ifndef UNICODE
    if(sizeOfChar == 1) {
        tstring ret(&buffer[pos], strlen);
        pos += strlen;
        return ret;
    }
    else if(sizeOfChar == 2) {
        tstring ret;
        for(tstring::size_type i=0; i<strlen; ++i) {
            unsigned short tmp = readShort();
            ret += static_cast<char>(tmp < 256 ? tmp : ' ');
        }
        return ret;
    }
    else {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readString()- Invalid sizeOfChar!!!!"));
    }

#else /* UNICODE */
    if(sizeOfChar == 1) {
        std::string ret(&buffer[pos], strlen);
        pos += strlen;
        return towstring(ret);
    }
    else if(sizeOfChar == 2) {
        tstring ret;
        for(tstring::size_type i=0; i<strlen; ++i) {
            ret += static_cast<tchar>(readShort());
        }
        return ret;
    }
    else {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::readString()- Invalid sizeOfChar!!!!"));
    }
#endif

    return tstring();
}



void
SocketBuffer::appendByte(unsigned char val)
{
    if((pos + sizeof(unsigned char)) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::appendByte()- Attempt to write beyond end of buffer"));
        return;
    }

    buffer[pos] = static_cast<char>(val);
    pos += sizeof(unsigned char);
    size = pos;
}



void
SocketBuffer::appendShort(unsigned short val)
{
    if((pos + sizeof(unsigned short)) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::appendShort()- Attempt to write beyond end of buffer"));
        return;
    }

    unsigned short s = htons(val);
    std::memcpy(buffer + pos, &s, sizeof (s));
    pos += sizeof(s);
    size = pos;
}



void
SocketBuffer::appendInt(unsigned int val)
{
    if((pos + sizeof(unsigned int)) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::appendInt()- Attempt to write beyond end of buffer"));
        return;
    }

    int i = htonl(val);
    std::memcpy(buffer + pos, &i, sizeof (i));
    pos += sizeof(i);
    size = pos;
}



void
SocketBuffer::appendString(const tstring& str)
{
    std::size_t const strlen = str.length();
    static std::size_t const sizeOfChar = sizeof (tchar) == 1 ? 1 : 2;

    if((pos + sizeof(unsigned int) + strlen * sizeOfChar) > maxsize)
    {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::appendString()-")
            LOG4CPLUS_TEXT(" Attempt to write beyond end of buffer"));
        return;
    }

    appendInt(static_cast<unsigned>(strlen));
#ifndef UNICODE
    std::memcpy(&buffer[pos], str.data(), strlen);
    pos += strlen;
    size = pos;
#else
    for(tstring::size_type i=0; i<str.length(); ++i) {
        appendShort(static_cast<unsigned short>(str[i]));
    }
#endif
}



void
SocketBuffer::appendBuffer(const SocketBuffer& buf)
{
    if((pos + buf.getSize()) > maxsize) {
        getLogLog().error(LOG4CPLUS_TEXT("SocketBuffer::appendBuffer()- Attempt to write beyond end of buffer"));
        return;
    }

    std::memcpy(&buffer[pos], buf.buffer, buf.getSize());
    pos += buf.getSize();
    size = pos;
}


//
//
//

BinaryBuffer::BinaryBuffer (std::size_t reserve)
    : pos (0)
{
    data.reserve (reserve);
}


BinaryBuffer::BinaryBuffer (void const * d, std::size_t sz)
    : data (static_cast<char const *>(d), static_cast<char const *>(d) + sz)
    , pos (0)
{ }


unsigned char
BinaryBuffer::readByte ()
{
    return readWorker<unsigned char> ();
}


unsigned short
BinaryBuffer::readShort()
{
    return readWorker<unsigned short> ();
}


unsigned int
BinaryBuffer::readInt()
{
    return readWorker<unsigned int> ();
}


void
BinaryBuffer::appendByte (unsigned char val)
{
    data.push_back (val);
}


void
BinaryBuffer::appendShort (unsigned short val)
{
    appendWorker (val);
}


void
BinaryBuffer::appendInt (unsigned int val)
{
    appendWorker (val);
}


void
BinaryBuffer::appendBuffer (BinaryBuffer const & buffer)
{
    data.insert (data.end (), buffer.data.begin (), buffer.data.end ());
}


void
BinaryBuffer::appendData (void const * d, std::size_t sz)
{
    std::size_t end_pos = data.size ();
    data.resize (end_pos + sz);
    std::memcpy (&data[end_pos], &d, sz);    
}


template <typename T>
void
BinaryBuffer::appendWorker (T val)
{
    std::size_t end_pos = data.size ();
    data.resize (end_pos + sizeof (T));
    std::memcpy (&data[end_pos], &val, sizeof (T));
}


template <typename T>
T
BinaryBuffer::readWorker ()
{
    std::size_t const data_size = data.size ();
    if (LOG4CPLUS_UNLIKELY (pos >= data_size))
        getLogLog ().error (
            LOG4CPLUS_TEXT ("BinaryBuffer::readWorker()")
            LOG4CPLUS_TEXT ("- end of buffer reached"),
            true);
    else if (LOG4CPLUS_UNLIKELY (pos + sizeof (T) > data_size))
        getLogLog ().error (
            LOG4CPLUS_TEXT ("BinaryBuffer::readWorker()")
            LOG4CPLUS_TEXT ("- Attempt to read beyond end of buffer"),
            true);

    T val;
    std::memcpy (&val, &data[pos], sizeof (T));
    pos += sizeof (T);
    return val;
}


} } // namespace log4cplus { namespace helpers {
