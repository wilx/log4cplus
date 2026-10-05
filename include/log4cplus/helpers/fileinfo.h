// -*- C++ -*-
//
//  Copyright (C) 2012-2017, Vaclav Zeman. All rights reserved.
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

#if ! defined (LOG4CPLUS_HELPERS_FILEINFO_H)
#define LOG4CPLUS_HELPERS_FILEINFO_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/helpers/timehelper.h>
#include <array>
#include <cstdint>
#ifdef LOG4CPLUS_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


namespace log4cplus { namespace helpers {

struct LOG4CPLUS_EXPORT FileInfo;

/** Allocation-free identity of a filesystem object.
 *
 * Identity excludes modification time and size. It is not a persistent
 * identifier: filesystems can reuse identifiers after an object is deleted.
 * Default-constructed identities are invalid and compare equal to each other.
 * POSIX compares device and inode; Windows compares the volume serial number
 * and the full 128-bit file identifier.
 */
class LOG4CPLUS_EXPORT FileIdentity
{
public:
    constexpr FileIdentity () noexcept = default;

    constexpr bool isValid () const noexcept
    {
        return kind != Kind::empty;
    }

    constexpr bool operator== (FileIdentity const &) const noexcept = default;

private:
    enum class Kind { empty, posix, windows };
    Kind kind = Kind::empty;
    std::uintmax_t filesystem = 0;
    std::array<std::uintmax_t, 2> object {};

    friend LOG4CPLUS_EXPORT bool getFileInfo (FileInfo *, tstring const &);
};

/** OS independent file metadata and identity.
 *
 * Modification times retain available fractions up to microsecond precision.
 * The identity member changes this structure's ABI; consumers must rebuild.
 */
struct LOG4CPLUS_EXPORT FileInfo
{
    helpers::Time mtime;
    bool is_link;
    off_t size;
    FileIdentity identity;
};


/** Query metadata and identity by path, following symbolic links.
 *
 * Returns true only when every field, including a valid identity, is available.
 * Returns false and sets errno on failure, leaving the output unchanged.
 * On Windows, errno holds the Win32 error code cast to int; the primary error
 * is also preserved in GetLastError(). Filesystems that cannot provide the full
 * identity fail the query.
 * Narrow Windows paths are converted to UTF-16 using helpers::towstring().
 */
LOG4CPLUS_EXPORT bool getFileInfo (FileInfo * fi, tstring const & name);


} } // namespace log4cplus { namespace helpers {

#endif // LOG4CPLUS_HELPERS_FILEINFO_H
