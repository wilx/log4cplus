// -*- C++ -*-
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

#include <log4cplus/config.hxx>
#include <limits>
#include <log4cplus/helpers/fileinfo.h>

#ifdef LOG4CPLUS_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef LOG4CPLUS_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if defined (_WIN32)
#include <tchar.h>
#include <log4cplus/config/windowsh-inc.h>
#include <log4cplus/internal/win32.h>
#endif


namespace log4cplus { namespace helpers {


//! \note Why is the code to get file size and modification time this
//! complicated? It is because of Windows. There are two aspects for
//! this problem. First is the The <code>_stat()</code> call, the
//! second is desktop style versus Metro style applications.
//!
//! On Windows the _stat() call is plemented using FindFirstFileEx()
//! function. FindFirstFileEx() reads the file entries in directory
//! node. For NTFS, the times and sizes on the directory node are not
//! kept in sync, rather they are updated infrequently and/or on file
//! close. This means that _stat() is returning stale information for
//! files that are still open and being written to. To work around
//! that, it is possible to use functions like
//! GetFileInformationByHandleEx(), which is for Vista and later, or
//! GetFileInformationByHandle(), which is XP and later, or use
//! GetFileSizeEx() together with GetFileTime() on earlier
//! Windows. Instead of going for least common denominator, the code
//! chooses the news available function. Why? Here comes the second
//! aspect, Metro style applications cannot invoke all of the
//! functions. E.g., only GetFileInformationByHandleEx() is supported
//! for Metro style applications.
//!
//! (This dance here does not mean that log4cplus as whole already
//! supports Metro style applications. This is rather to avoid using
//! incompatible functions in advance.)
int
getFileInfo (FileInfo * fi, tstring const & name)
{
#if defined (_WIN32)
    typedef unsigned __int64 uint64_type;

    internal::Handle fh (CreateFile (name.c_str (), GENERIC_READ,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
    if (! fh.h)
    {
        DWORD eno = GetLastError ();
        if (eno == ERROR_FILE_NOT_FOUND)
        {
            return -1;
        }
        else
            internal::win32_throw_exception("CreateFile()");
    }

#if _WIN32_WINNT >= 0x0600
    if (internal::pwin32_GetFileInformationByHandleEx)
    {
        FILE_BASIC_INFO bi;
        FILE_STANDARD_INFO si;
        
        BOOL ret = internal::pwin32_GetFileInformationByHandleEx (fh.h,
            FileBasicInfo, &bi, sizeof (bi));
        if (! ret)
            internal::win32_throw_exception("GetFileInformationByHandleEx()");

        ret = internal::pwin32_GetFileInformationByHandleEx (fh.h,
            FileStandardInfo, &si, sizeof (si));
        if (! ret)
            internal::win32_throw_exception("GetFileInformationByHandleEx()");

        fi->mtime = internal::FILETIME_to_Time (bi.LastWriteTime);
        fi->is_link = false;           

        uint64_type file_size = uint64_type (si.EndOfFile.HighPart) << 32
            | si.EndOfFile.LowPart;
        if (file_size <= (std::numeric_limits<off_t>::max) ())
            fi->size = static_cast<off_t>(file_size);
        else
            return -1;
    }
    // This else connects with the following if.
    else
#endif

    if (internal::pwin32_GetFileInformationByHandle)
    {
        BY_HANDLE_FILE_INFORMATION bhfi = {};
        BOOL ret = internal::pwin32_GetFileInformationByHandle (fh.h, &bhfi);
        if (! ret)
            internal::win32_throw_exception ("GetFileInformationByHandle()");

        fi->mtime = internal::FILETIME_to_Time (bhfi.ftLastWriteTime);
        fi->is_link = false;
        uint64_type file_size = uint64_type (bhfi.nFileSizeHigh) << 32
            | bhfi.nFileSizeLow;
        if (file_size <= (std::numeric_limits<off_t>::max) ())
            fi->size = static_cast<off_t>(file_size);
        else
            return -1;
    }
    else if (internal::pwin32_GetFileSizeEx)
    {
        FILETIME ft = {};
        LARGE_INTEGER li = {};

        BOOL ret = GetFileTime (fh.h, 0, 0, &ft);
        if (! ret)
            internal::win32_throw_exception ("GetFileTime()");

        ret = internal::pwin32_GetFileSizeEx (fh.h, &li);
        if (! ret)
            internal::win32_throw_exception ("GetFileSizeEx()");

        fi->mtime = internal::FILETIME_to_Time (ft);
        fi->is_link = false;

        uint64_type file_size = uint64_type (li.HighPart) << 32
            | li.LowPart;
        if (file_size <= (std::numeric_limits<off_t>::max) ())
            fi->size = static_cast<off_t>(file_size);
        else
            return -1;
    }
    else
    {
        fh.close ();

        struct _stat fileStatus;
        if (_tstat (name.c_str (), &fileStatus) == -1)
            return -1;
    
        fi->mtime = helpers::Time (fileStatus.st_mtime);
        fi->is_link = false;
        fi->size = fileStatus.st_size;
    }

    return 0;

#else
    struct stat fileStatus;
    if (stat (LOG4CPLUS_TSTRING_TO_STRING (name).c_str (),
            &fileStatus) == -1)
        return -1;

    fi->mtime = helpers::Time (fileStatus.st_mtime);
    fi->is_link = S_ISLNK (fileStatus.st_mode);
    fi->size = fileStatus.st_size;
    
#endif
    
    return 0;
}

} } // namespace log4cplus { namespace helpers {
