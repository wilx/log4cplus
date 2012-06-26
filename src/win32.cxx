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

#if defined (_WIN32)
#include <log4cplus/internal/win32.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <vector>


namespace log4cplus { namespace internal {


Win32Support win32_;
Win32Support const & win32 = win32_;


#define LOG4CPLUS_WIN32_FUNC_DEF(name) \
    functype_ ## name pwin32_ ## name

#define LOG4CPLUS_WIN32_FUNC_INIT(name, module) \
    do { \
        typedef functype_ ## name funtype; \
        funtype & funptr = pwin32_ ## name; \
        funptr = reinterpret_cast<funtype> (GetProcAddress ((module), #name));\
        if (! funptr) \
            win32_throw_exception ("GetProcAddress()"); \
    } while (0)


#if _WIN32_WINNT >= 0x0600
LOG4CPLUS_WIN32_FUNC_DEF (GetFileInformationByHandleEx);
#endif
LOG4CPLUS_WIN32_FUNC_DEF (GetFileInformationByHandle);
LOG4CPLUS_WIN32_FUNC_DEF (GetFileSizeEx);


void
initializeWin32 ()
{
    helpers::LogLog & loglog = helpers::getLogLog ();

    OSVERSIONINFOEX oviex = {};
    oviex.dwOSVersionInfoSize = sizeof (oviex);
    BOOL ret = GetVersionEx (reinterpret_cast<LPOSVERSIONINFO>(&oviex));
    if (! ret)
        win32_throw_exception ("GetVersionEx()");

    win32_.os_major = oviex.dwMajorVersion;
    win32_.os_minor = oviex.dwMinorVersion;
    win32_.sp_major = oviex.wServicePackMajor;

    tstring const sysdir = win32_GetSystemDirectory ();
    tstring const kernel32_path = sysdir + LOG4CPLUS_TEXT ("\\Kernel32.dll");

    HMODULE const kernel32_mod = GetModuleHandle (kernel32_path.c_str ());
    if (! kernel32_mod)
        win32_throw_exception ("GetModuleHandle()");

    
    // Windows Vista and higher.
    if (win32.os_major >= 6)
    {
#if _WIN32_WINNT >= 0x0600
        LOG4CPLUS_WIN32_FUNC_INIT (GetFileInformationByHandleEx, kernel32_mod);
#endif
    }
    
    // Windows XP and higher.
    if (win32.os_major > 5 || (win32.os_major == 5 && win32.os_minor >= 1))
    {
        LOG4CPLUS_WIN32_FUNC_INIT (GetFileInformationByHandle, kernel32_mod);
    }

    if (win32.os_major >= 5)
    {
        LOG4CPLUS_WIN32_FUNC_INIT (GetFileSizeEx, kernel32_mod);
    }
}


tstring
win32_GetSystemDirectory ()
{
    std::vector<tchar> buf (MAX_PATH + 1);
    UINT ret = GetSystemDirectory (&buf[0], static_cast<UINT>(buf.size ()));
    if (ret > buf.size ())
    {
        buf.resize (ret);
        ret = GetSystemDirectory (&buf[0], static_cast<UINT>(buf.size ()));
        if (! ret)
            win32_throw_exception ("GetSystemDirectory()");
    }
    else if (ret == 0)
        win32_throw_exception ("GetSystemDirectory()");

    return tstring (buf.begin (), buf.begin () + ret);
}


helpers::Time
FILETIME_to_Time (LARGE_INTEGER const & li)
{
    FILETIME ft;
    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = li.HighPart;
    return FILETIME_to_Time (ft);
}


helpers::Time
FILETIME_to_Time (FILETIME const & ft)
{
    typedef unsigned __int64 uint64_type;
    uint64_type t100ns
        = uint64_type (ft.dwHighDateTime) << 32
        | ft.dwLowDateTime;

    // Number of 100-ns intervals between UNIX epoch and Windows system time
    // is 116444736000000000.
    uint64_type const offset = uint64_type (116444736) * 1000 * 1000 * 1000;
    uint64_type fixed_time = t100ns - offset;

    return helpers::Time (fixed_time / (10 * 1000 * 1000),
        fixed_time % (10 * 1000 * 1000) / 10);
}


void
win32_throw_exception (char const * func)
{
    DWORD eno = GetLastError ();
    helpers::getLogLog ().error (LOG4CPLUS_STRING_TO_TSTRING (func)
        + LOG4CPLUS_TEXT (": ")
        + helpers::convertIntegerToString (eno), true);
}


} } // namespace log4cplus { namespace internal {

#endif // _WIN32
