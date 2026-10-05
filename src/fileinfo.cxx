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

#include <log4cplus/config.hxx>
#include <log4cplus/helpers/fileinfo.h>
#include <log4cplus/tstring.h>
#include <cerrno>
#include <cstring>
#include <utility>

#ifdef LOG4CPLUS_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef LOG4CPLUS_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if defined (_WIN32)
#include <log4cplus/config/windowsh-inc-full.h>
#endif

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)

#include <catch_amalgamated.hpp>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#if ! defined (_WIN32)
#include <fcntl.h>
#endif

#endif


namespace log4cplus::helpers {

namespace {

/** Read modification time from the available stat fields, preferring fractional
 * timestamps and rounding down to microseconds; fall back to whole seconds.
 */
template <typename Stat>
Time
stat_mtime (Stat const & status)
{
    if constexpr (requires { status.st_mtim.tv_sec; status.st_mtim.tv_nsec; })
        return Time (chrono::seconds (status.st_mtim.tv_sec))
            + chrono::floor<Duration> (
                chrono::nanoseconds (status.st_mtim.tv_nsec));
    else if constexpr (requires { status.st_mtimespec.tv_sec;
                                 status.st_mtimespec.tv_nsec; })
        return Time (chrono::seconds (status.st_mtimespec.tv_sec))
            + chrono::floor<Duration> (
                chrono::nanoseconds (status.st_mtimespec.tv_nsec));
    else if constexpr (requires { status.st_mtime; status.st_mtimensec; })
        return Time (chrono::seconds (status.st_mtime))
            + chrono::floor<Duration> (
                chrono::nanoseconds (status.st_mtimensec));
    else
        return Time (chrono::seconds (status.st_mtime));
}

#if defined (_WIN32)

void
set_windows_error (DWORD error) noexcept
{
    errno = static_cast<int> (error);
    SetLastError (error);
}

Time
filetime_mtime (FILETIME const & time)
{
    std::uint64_t const ticks = (std::uint64_t (time.dwHighDateTime) << 32)
        | time.dwLowDateTime;
    // Divide before subtracting the epoch: the unsigned FILETIME range fits
    // signed microseconds, and floor rounding also works before the Unix epoch.
    constexpr long long epoch_microseconds = 11644473600000000LL;
    return Time (Duration (static_cast<long long> (ticks / 10)
                          - epoch_microseconds));
}

struct QueryHandle
{
    HANDLE value;
    ~QueryHandle ()
    {
        int const error_number = errno;
        DWORD const error = GetLastError ();
        CloseHandle (value);
        errno = error_number;
        SetLastError (error);
    }
};

struct WindowsInfo
{
    BY_HANDLE_FILE_INFORMATION metadata;
    FILE_ID_INFO identity;
};

template <typename MetadataQuery, typename IdentityQuery>
bool
query_windows_info (HANDLE handle, WindowsInfo & result,
                    MetadataQuery metadata_query, IdentityQuery identity_query)
{
    WindowsInfo candidate {};
    if (! metadata_query (handle, &candidate.metadata)
        || ! identity_query (handle, FileIdInfo, &candidate.identity,
                             sizeof candidate.identity))
        return false;
    result = candidate;
    return true;
}

#endif

} // namespace

bool
getFileInfo (FileInfo * fi, tstring const & name)
{
    FileInfo candidate {};
#if defined (_WIN32)
    wchar_t const * path;
#if defined (UNICODE)
    path = name.c_str ();
#else
    std::wstring const wide_path = towstring (name);
    path = wide_path.c_str ();
#endif
    HANDLE const handle = CreateFileW (path, 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        set_windows_error (GetLastError ());
        return false;
    }
    QueryHandle const guard { handle };
    WindowsInfo info;
    if (! query_windows_info (handle, info, GetFileInformationByHandle,
                              GetFileInformationByHandleEx))
    {
        set_windows_error (GetLastError ());
        return false;
    }

    // stat() reports zero size for Windows directories.
    std::uint64_t const size = (info.metadata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        ? 0 : (std::uint64_t (info.metadata.nFileSizeHigh) << 32)
                  | info.metadata.nFileSizeLow;
    if (! std::in_range<off_t> (size))
    {
        set_windows_error (ERROR_ARITHMETIC_OVERFLOW);
        return false;
    }
    candidate.mtime = filetime_mtime (info.metadata.ftLastWriteTime);
    candidate.is_link = false;
    candidate.size = static_cast<off_t> (size);
    candidate.identity.kind = FileIdentity::Kind::windows;
    candidate.identity.filesystem = info.identity.VolumeSerialNumber;
    std::uint64_t words[2];
    static_assert (sizeof words == sizeof info.identity.FileId.Identifier);
    std::memcpy (words, info.identity.FileId.Identifier, sizeof words);
    candidate.identity.object = { words[0], words[1] };

#else
    struct stat fileStatus;
    if (stat (LOG4CPLUS_TSTRING_TO_STRING (name).c_str (),
            &fileStatus) == -1)
        return false;

    candidate.mtime = stat_mtime (fileStatus);
    candidate.is_link = S_ISLNK (fileStatus.st_mode);
    candidate.size = fileStatus.st_size;
    candidate.identity.kind = FileIdentity::Kind::posix;
    candidate.identity.filesystem = static_cast<std::uintmax_t> (fileStatus.st_dev);
    candidate.identity.object = { static_cast<std::uintmax_t> (fileStatus.st_ino), 0 };

#endif

    *fi = candidate;
    return true;
}

} // namespace log4cplus::helpers

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)

namespace log4cplus::helpers {
namespace {

struct FileInfoDirectory
{
    std::filesystem::path path;

    FileInfoDirectory ()
    {
        auto const stamp = now ().time_since_epoch ().count ();
        for (unsigned i = 0; i < 100; ++i)
        {
            path = std::filesystem::temp_directory_path ()
                / ("log4cplus-fileinfo-" + std::to_string (stamp)
                   + "-" + std::to_string (i));
            if (std::filesystem::create_directory (path))
                return;
        }
        throw std::runtime_error ("Cannot create isolated FileInfo test directory");
    }

    ~FileInfoDirectory ()
    {
        std::error_code ignored;
        std::filesystem::remove_all (path, ignored);
    }
};

tstring
info_path (std::filesystem::path const & path)
{
#if defined (UNICODE)
    return path.wstring ();
#elif defined (_WIN32)
    return tostring (path.wstring ());
#else
    return path.string ();
#endif
}

void
write_info_file (std::filesystem::path const & path, char const * contents,
                 std::ios_base::openmode mode = std::ios_base::trunc)
{
    std::ofstream stream (path, mode | std::ios_base::binary);
    stream << contents;
    stream.close ();
    CATCH_REQUIRE (stream.good ());
}

FileInfo
read_info (std::filesystem::path const & path)
{
    FileInfo info {};
    CATCH_REQUIRE (getFileInfo (&info, info_path (path)));
    CATCH_REQUIRE (info.identity.isValid ());
    return info;
}

} // namespace

CATCH_TEST_CASE ("File identity excludes file contents", "[fileinfo]")
{
    FileIdentity const empty;
    CATCH_CHECK_FALSE (empty.isValid ());
    CATCH_CHECK (empty == FileIdentity ());

    FileInfoDirectory directory;
    auto const path = directory.path / "active.log";
    write_info_file (path, "first\n");
    FileInfo const original = read_info (path);
    CATCH_CHECK (original.identity != empty);
    CATCH_CHECK (original.size == 6);
    CATCH_CHECK_FALSE (original.is_link);
    CATCH_CHECK (read_info (path).identity == original.identity);

    write_info_file (path, "second\n", std::ios_base::app);
    FileInfo const appended = read_info (path);
    CATCH_CHECK (appended.size == 13);
    CATCH_CHECK (appended.identity == original.identity);

    write_info_file (path, "x");
    FileInfo const truncated = read_info (path);
    CATCH_CHECK (truncated.size == 1);
    CATCH_CHECK (truncated.identity == original.identity);
}

CATCH_TEST_CASE ("File identity follows rename and detects replacement", "[fileinfo]")
{
    FileInfoDirectory directory;
    auto const path = directory.path / "active.log";
    auto const archive = directory.path / "archive.log";
    write_info_file (path, "original");
    FileIdentity const identity = read_info (path).identity;
    std::filesystem::rename (path, archive);
    CATCH_CHECK (read_info (archive).identity == identity);
    // Keep the archived object alive so its identifier cannot be recycled.
    write_info_file (path, "replacement");
    CATCH_CHECK (read_info (path).identity != identity);
}

CATCH_TEST_CASE ("File identity is shared by hard links", "[fileinfo]")
{
    FileInfoDirectory directory;
    auto const path = directory.path / "file";
    auto const link = directory.path / "hard-link";
    write_info_file (path, "contents");
    std::error_code error;
    std::filesystem::create_hard_link (path, link, error);
    if (error)
        CATCH_SKIP ("Hard links are unavailable: " << error.message ());
    CATCH_CHECK (read_info (path).identity == read_info (link).identity);
}

CATCH_TEST_CASE ("File info follows supported symbolic links", "[fileinfo]")
{
    FileInfoDirectory directory;
    auto const path = directory.path / "file";
    auto const link = directory.path / "symbolic-link";
    write_info_file (path, "contents");
    std::error_code error;
    std::filesystem::create_symlink (path, link, error);
    if (error)
        CATCH_SKIP ("Symbolic links are unavailable: " << error.message ());
    FileInfo const target = read_info (path);
    FileInfo const linked = read_info (link);
    CATCH_CHECK (linked.identity == target.identity);
    CATCH_CHECK (linked.size == target.size);
    CATCH_CHECK (linked.mtime == target.mtime);
    CATCH_CHECK_FALSE (linked.is_link);
}

CATCH_TEST_CASE ("File info supports directories and leaves failures unchanged", "[fileinfo]")
{
    FileInfoDirectory directory;
    FileInfo info = read_info (directory.path);
#if defined (_WIN32)
    CATCH_CHECK (info.size == 0);
#endif
    FileInfo const saved = info;
    errno = 0;
    bool const result = getFileInfo (&info, info_path (directory.path / "missing"));
    int const error = errno;
#if defined (_WIN32)
    DWORD const native_error = GetLastError ();
#endif
    CATCH_CHECK_FALSE (result);
#if defined (_WIN32)
    CATCH_CHECK (error == static_cast<int> (ERROR_FILE_NOT_FOUND));
    CATCH_CHECK (native_error == ERROR_FILE_NOT_FOUND);
#else
    CATCH_CHECK (error == ENOENT);
#endif
    CATCH_CHECK (info.identity == saved.identity);
    CATCH_CHECK (info.mtime == saved.mtime);
    CATCH_CHECK (info.size == saved.size);
    CATCH_CHECK (info.is_link == saved.is_link);
}

CATCH_TEST_CASE ("Stat timestamp adapters retain fractions and support old layouts", "[fileinfo]")
{
    struct Timestamp { time_t tv_sec; long tv_nsec; };
    struct Posix { Timestamp st_mtim; };
    struct Darwin { Timestamp st_mtimespec; };
    // Account for st_mtime being a system-header macro naming a seconds field.
    struct Seconds
    {
#if defined (st_mtime)
        struct { time_t tv_sec = -1; } st_mtim, st_mtimespec;
#else
        time_t st_mtime = -1;
#endif
    };
    struct Legacy : Seconds { long st_mtimensec = 999999999; };
    struct Both : Posix, Darwin {};

    Time const expected = from_time_t (123) + chrono::microseconds (456789);
    CATCH_CHECK (stat_mtime (Posix {{123, 456789999}}) == expected);
    CATCH_CHECK (stat_mtime (Darwin {{123, 456789999}}) == expected);
    CATCH_CHECK (stat_mtime (Both {{{123, 456789999}}, {{456, 0}}}) == expected);
    CATCH_CHECK (stat_mtime (Posix {{-1, 999999999}}) == Time (Duration (-1)));
    CATCH_CHECK (stat_mtime (Darwin {{-1, 999999999}}) == Time (Duration (-1)));
    CATCH_CHECK (stat_mtime (Legacy {}) == Time (Duration (-1)));
    CATCH_CHECK (stat_mtime (Seconds {}) == from_time_t (-1));
    // Convert directly into microseconds, avoiding a nanosecond Clock's more
    // limited date range when its from_time_t() conversion is used.
    if constexpr (sizeof (time_t) >= sizeof (long long))
        CATCH_CHECK (stat_mtime (Posix {{static_cast<time_t> (-11644473600LL), 9999}})
            == Time (Duration (-11644473599999991LL)));
}

CATCH_TEST_CASE ("File info retains native fractional modification time", "[fileinfo]")
{
    FileInfoDirectory directory;
    auto const path = directory.path / "timestamp";
    write_info_file (path, "contents");
    constexpr time_t seconds = 1700000000;
#if defined (_WIN32)
    constexpr std::uint64_t ticks = 116444736000000000ULL
        + std::uint64_t (seconds) * 10000000ULL + 1234567ULL;
    FILETIME const stamp { static_cast<DWORD> (ticks), static_cast<DWORD> (ticks >> 32) };
    HANDLE const handle = CreateFileW (path.c_str (), FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    CATCH_REQUIRE (handle != INVALID_HANDLE_VALUE);
    bool const changed = SetFileTime (handle, nullptr, nullptr, &stamp) != 0;
    CloseHandle (handle);
    CATCH_REQUIRE (changed);
#else
    timespec const times[2] {{seconds, 123456789}, {seconds, 123456789}};
    CATCH_REQUIRE (utimensat (AT_FDCWD, path.c_str (), times, 0) == 0);
#endif
    FileInfo const info = read_info (path);
    CATCH_CHECK (info.mtime == from_time_t (seconds) + chrono::microseconds (123456));
}

#if defined (_WIN32)

CATCH_TEST_CASE ("Windows file sizes outside off_t fail without changing the output", "[fileinfo]")
{
    if constexpr (sizeof (off_t) >= sizeof (std::uint64_t))
        CATCH_SKIP ("off_t can hold every supported Windows file size");
    FileInfoDirectory directory;
    auto const path = directory.path / "sparse";
    write_info_file (path, "contents");
    FileInfo info = read_info (path);
    FileInfo const saved = info;
    HANDLE const handle = CreateFileW (path.c_str (), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    CATCH_REQUIRE (handle != INVALID_HANDLE_VALUE);
    QueryHandle const guard { handle };
    DWORD returned;
    if (! DeviceIoControl (handle, FSCTL_SET_SPARSE, nullptr, 0,
                          nullptr, 0, &returned, nullptr))
        CATCH_SKIP ("Sparse files are unavailable");
    LARGE_INTEGER size;
    size.QuadPart = static_cast<LONGLONG> (
        std::uint64_t (std::numeric_limits<off_t>::max ()) + 1);
    CATCH_REQUIRE (SetFilePointerEx (handle, size, nullptr, FILE_BEGIN) != 0);
    CATCH_REQUIRE (SetEndOfFile (handle) != 0);
    bool const result = getFileInfo (&info, info_path (path));
    int const error = errno;
    DWORD const native_error = GetLastError ();
    CATCH_CHECK_FALSE (result);
    CATCH_CHECK (error == static_cast<int> (ERROR_ARITHMETIC_OVERFLOW));
    CATCH_CHECK (native_error == ERROR_ARITHMETIC_OVERFLOW);
    CATCH_CHECK (info.identity == saved.identity);
    CATCH_CHECK (info.size == saved.size);
    CATCH_CHECK (info.mtime == saved.mtime);
    CATCH_CHECK (info.is_link == saved.is_link);
}

CATCH_TEST_CASE ("Windows timestamp conversion floors fractions before the Unix epoch", "[fileinfo]")
{
    auto convert = [] (std::uint64_t ticks) {
        return filetime_mtime (FILETIME { static_cast<DWORD> (ticks),
                                        static_cast<DWORD> (ticks >> 32) });
    };
    constexpr std::uint64_t epoch = 116444736000000000ULL;
    CATCH_CHECK (convert (epoch) == Time ());
    CATCH_CHECK (convert (epoch + 9) == Time ());
    CATCH_CHECK (convert (epoch + 19) == Time (Duration (1)));
    CATCH_CHECK (convert (epoch - 1) == Time (Duration (-1)));
    CATCH_CHECK (convert (epoch - 11) == Time (Duration (-2)));
    CATCH_CHECK (convert (0) == Time (Duration (-11644473600000000LL)));
}

CATCH_TEST_CASE ("An unsupported Windows identity query fails without committing metadata", "[fileinfo]")
{
    WindowsInfo result {};
    result.metadata.nFileSizeLow = 42;
    result.identity.VolumeSerialNumber = 17;
    auto metadata = [] (HANDLE, BY_HANDLE_FILE_INFORMATION * value) {
        value->nFileSizeLow = 99;
        return TRUE;
    };
    auto identity = [] (HANDLE, FILE_INFO_BY_HANDLE_CLASS, void *, DWORD) {
        SetLastError (ERROR_NOT_SUPPORTED);
        return FALSE;
    };
    bool const queried = query_windows_info (INVALID_HANDLE_VALUE, result, metadata, identity);
    DWORD const native_error = GetLastError ();
    CATCH_CHECK_FALSE (queried);
    CATCH_CHECK (native_error == ERROR_NOT_SUPPORTED);
    CATCH_CHECK (result.metadata.nFileSizeLow == 42);
    CATCH_CHECK (result.identity.VolumeSerialNumber == 17);
}

CATCH_TEST_CASE ("Windows file queries use the configured path conversion", "[fileinfo]")
{
    FileInfoDirectory directory;
    auto const path = directory.path / L"accent-\u00e9.log";
    write_info_file (path, "contents");
#if defined (UNICODE)
    CATCH_CHECK (read_info (path).size == 8);
#else
    // Use only a path the configured locale can represent without substitution.
    auto const narrow = info_path (path);
    if (std::filesystem::path (towstring (narrow)) != path)
        CATCH_SKIP ("The configured locale cannot represent the test filename");
    BOOL const was_ansi = AreFileApisANSI ();
    SetFileApisToOEM ();
    FileInfo info {};
    bool const result = getFileInfo (&info, narrow);
    if (was_ansi)
        SetFileApisToANSI ();
    CATCH_REQUIRE (result);
    CATCH_CHECK (info.size == 8);
#endif
}

#endif

} // namespace log4cplus::helpers

#endif
