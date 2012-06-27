// -*- C++ -*-
// Module:  Log4CPLUS
// File:    win32.h
// Created: 6/2012
// Author:  Vaclav Zeman
//
//
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

#ifndef LOG4CPLUS_INTERNAL_WIN32_H
#define LOG4CPLUS_INTERNAL_WIN32_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#if defined (_WIN32)
#include <log4cplus/config/windowsh-inc.h>
#include <log4cplus/tstring.h>
#include <log4cplus/helpers/timehelper.h>


namespace log4cplus { namespace internal {


struct Win32Support
{
    int os_major;
    int os_minor;
    int sp_major;
};

extern Win32Support const & win32;


void initializeWin32 ();
tstring win32_GetSystemDirectory ();
helpers::Time FILETIME_to_Time (FILETIME const &);
helpers::Time FILETIME_to_Time (LARGE_INTEGER const &);
void win32_throw_exception(char const *) LOG4CPLUS_ATTRIBUTE_NORETURN;


struct Handle
{
    Handle ()
        : h (INVALID_HANDLE_VALUE)
    { }

    explicit
    Handle (HANDLE h_)
        : h (h_)
    { }

    ~Handle ()
    {
        close ();
    }

    void close ()
    {
        if (h && h != INVALID_HANDLE_VALUE)
        {
            CloseHandle (h);
            h = INVALID_HANDLE_VALUE;
        }
    }
    
    HANDLE h;
};


#define LOG4CPLUS_WIN32_FUNC_DECL(ret, callconv, name, params) \
    typedef ret (callconv * functype_ ## name) params; \
    extern functype_ ## name pwin32_ ## name

#if _WIN32_WINNT >= 0x0600
LOG4CPLUS_WIN32_FUNC_DECL (BOOL, WINAPI, GetFileInformationByHandleEx,
    (HANDLE, FILE_INFO_BY_HANDLE_CLASS, LPVOID, DWORD));
#endif

LOG4CPLUS_WIN32_FUNC_DECL (BOOL, WINAPI, GetFileInformationByHandle,
    (HANDLE, LPBY_HANDLE_FILE_INFORMATION));

LOG4CPLUS_WIN32_FUNC_DECL (BOOL, WINAPI, GetFileSizeEx,
    (HANDLE, PLARGE_INTEGER));


} } // namespace log4cplus { namespace internal {

#endif // _WIN32
#endif // LOG4CPLUS_INTERNAL_WIN32_H
