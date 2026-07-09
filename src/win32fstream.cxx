// -*- C++ -*-
//
//  Copyright (C) 2026, Vaclav Haisman. All rights reserved.
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

#if defined (_WIN32)

#include <log4cplus/config/windowsh-inc-full.h>
#include <log4cplus/fstreams.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace log4cplus { namespace helpers { namespace detail {

unsigned int
win32_acp_code_page ()
{
    return GetACP ();
}

unsigned int
win32_utf8_code_page ()
{
    return CP_UTF8;
}

bool
win32_is_utf8_code_page (unsigned int code_page)
{
    return code_page == CP_UTF8;
}

std::size_t
win32_code_page_max_char_size (unsigned int code_page, std::error_code & ec)
{
    CPINFO cp_info = {};
    if (! GetCPInfo (code_page, &cp_info))
    {
        ec = windows_error ();
        return 0;
    }

    ec.clear ();
    return cp_info.MaxCharSize;
}

bool
win32_is_dbcs_lead_byte (unsigned int code_page, unsigned char ch)
{
    return IsDBCSLeadByteEx (code_page, ch) != 0;
}

bool
win32_acp_path (char const * path, std::wstring & result,
                 std::error_code & ec)
{
    if (! path)
    {
        ec = std::make_error_code (std::errc::invalid_argument);
        result.clear ();
        return false;
    }

    int const size = MultiByteToWideChar (CP_ACP, 0, path, -1, nullptr, 0);
    if (! size)
    {
        ec = windows_error ();
        result.clear ();
        return false;
    }

    std::vector<wchar_t> buffer (static_cast<std::size_t> (size));
    if (! MultiByteToWideChar (CP_ACP, 0, path, -1, &buffer[0], size))
    {
        ec = windows_error ();
        result.clear ();
        return false;
    }

    ec.clear ();
    result.assign (&buffer[0]);
    return true;
}

bool
win32_multibyte_to_wide (unsigned int code_page, char const * input,
                         int input_size, wchar_t * output, int output_size,
                         int & count, std::error_code & ec)
{
    count = MultiByteToWideChar (code_page, 0, input, input_size, output,
                                 output_size);
    if (! count)
    {
        ec = windows_error ();
        return false;
    }

    ec.clear ();
    return true;
}

bool
win32_wide_to_multibyte (unsigned int code_page, wchar_t const * input,
                         int input_size, char * output, int output_size,
                         bool fail_on_best_fit, bool & used_default,
                         int & count, std::error_code & ec)
{
    bool const utf8 = win32_is_utf8_code_page (code_page);
    DWORD const flags = utf8 ? WC_ERR_INVALID_CHARS
                             : fail_on_best_fit ? WC_NO_BEST_FIT_CHARS : 0;
    BOOL default_used = FALSE;
    BOOL * const default_used_ptr = utf8 ? nullptr : &default_used;

    count = WideCharToMultiByte (code_page, flags, input, input_size, output,
                                 output_size, nullptr, default_used_ptr);
    if (! count)
    {
        ec = windows_error ();
        used_default = false;
        return false;
    }

    ec.clear ();
    used_default = default_used != FALSE;
    return true;
}

unsigned char
win32_code_page_default_char (unsigned int code_page, std::error_code & ec)
{
    CPINFO cp_info = {};
    if (! GetCPInfo (code_page, &cp_info))
    {
        ec = windows_error ();
        return 0;
    }

    ec.clear ();
    return static_cast<unsigned char> (cp_info.DefaultChar[0]);
}

} } } // namespace log4cplus::helpers::detail


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)

#include <catch_amalgamated.hpp>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace {

/** @brief Removes a test file if it exists. */
void erase (wchar_t const * path) {
    DeleteFileW (path);
}

/** @brief Reads a test file as uninterpreted bytes through Win32. */
std::vector<char> raw_read (wchar_t const * path) {
    std::vector<char> result;
    HANDLE const h =
        CreateFileW (path, GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return result;
    }
    LARGE_INTEGER size = {};
    if (GetFileSizeEx (h, &size)) {
        result.resize (static_cast<std::size_t> (size.QuadPart));
        DWORD got = 0;
        if (!result.empty ()
            && !ReadFile (h, &result[0], static_cast<DWORD> (result.size ()),
                          &got, nullptr)) {
            result.clear ();
        } else {
            result.resize (got);
        }
    }
    CloseHandle (h);
    return result;
}

/** @brief Replaces a test file with the supplied uninterpreted bytes. */
bool raw_write (wchar_t const * path, char const * data, DWORD size) {
    HANDLE const h =
        CreateFileW (path, GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    bool const ok =
        WriteFile (h, data, size, &written, nullptr) && written == size;
    CloseHandle (h);
    return ok;
}

CATCH_TEST_CASE ("An open file can be renamed", "[open][sharing]") {
    // Start with clean source and destination paths and open the source.
    wchar_t const * const old_name = L"rename-open-old.txt";
    wchar_t const * const new_name = L"rename-open-new.txt";
    erase (old_name);
    erase (new_name);
    log4cplus::helpers::win32_fstream fs (old_name, std::ios_base::out
                                                    | std::ios_base::trunc
                                                    | std::ios_base::binary);
    CATCH_INFO ("open rename source");
    CATCH_CHECK (fs.is_open ());

    // Write, rename the still-open file, and continue through the same handle.
    fs << "before" << std::flush;
    CATCH_INFO ("rename an open stream");
    CATCH_CHECK (MoveFileExW (old_name, new_name, MOVEFILE_REPLACE_EXISTING)
                 != 0);
    fs << "-after";
    fs.close ();

    // Verify that both writes followed the handle to the destination path.
    std::vector<char> bytes = raw_read (new_name);
    CATCH_INFO ("continued output follows renamed handle");
    CATCH_CHECK (std::string (bytes.begin (), bytes.end ()) == "before-after");
    erase (new_name);
}

CATCH_TEST_CASE ("Text mode converts UTF-8 and line endings",
                 "[unicode][text]") {
    // Write wide text containing LF and a non-ASCII BMP character.
    wchar_t const * const name = L"unicode-text.txt";
    erase (name);
    {
        log4cplus::helpers::win32_wfstream fs (name, std::ios_base::out
                                                     | std::ios_base::trunc);
        wchar_t text[] = {L'A', L'\n', static_cast<wchar_t> (0x5fc3), 0};
        fs.write (text, 3);
        fs.close ();
    }

    // Verify UTF-8 encoding and Windows text-mode CRLF expansion as raw bytes.
    std::vector<char> const bytes = raw_read (name);
    static char const expected[] = {'A',         '\r',        '\n',
                                    char (0xe5), char (0xbf), char (0x83)};
    CATCH_INFO ("wide text becomes UTF-8 with CRLF");
    CATCH_CHECK (bytes
                 == std::vector<char> (expected, expected + sizeof (expected)));

    // Read through the wide stream and verify UTF-8 and CRLF decoding.
    {
        log4cplus::helpers::win32_wfstream fs (name, std::ios_base::in);
        wchar_t text[4] = {};
        fs.read (text, 3);
        CATCH_INFO ("UTF-8 and CRLF decode to wide text");
        CATCH_CHECK (
            (text[0] == L'A' && text[1] == L'\n' && text[2] == 0x5fc3));
        fs.close ();
    }
    erase (name);
}

CATCH_TEST_CASE ("Text mode consumes a leading UTF-8 BOM", "[unicode][bom]") {
    // Create a raw UTF-8 file beginning with a BOM.
    wchar_t const * const name = L"bom.txt";
    char const bytes[] = {char (0xef), char (0xbb), char (0xbf), 'X'};
    CATCH_INFO ("create BOM input");
    CATCH_CHECK (raw_write (name, bytes, sizeof (bytes)));

    // Text mode consumes the leading BOM.
    log4cplus::helpers::win32_wfstream text (name, std::ios_base::in);
    CATCH_INFO ("text mode consumes initial BOM");
    CATCH_CHECK (text.get () == L'X');
    text.close ();

    // Binary mode exposes U+FEFF as ordinary decoded content.
    log4cplus::helpers::win32_wfstream binary (name, std::ios_base::in
                                                     | std::ios_base::binary);
    CATCH_INFO ("binary mode exposes BOM as content");
    CATCH_CHECK (binary.get () == static_cast<wchar_t> (0xfeff));
    binary.close ();
    erase (name);
}

CATCH_TEST_CASE ("Narrow conversion uses the code page replacement",
                 "[unicode][conversion]") {
    // Store a UTF-8 character that the C/ASCII code page cannot represent.
    wchar_t const * const name = L"replacement.txt";
    char const euro[] = {char (0xe2), char (0x82), char (0xac)};
    CATCH_INFO ("create replacement input");
    CATCH_CHECK (raw_write (name, euro, sizeof (euro)));

    // Decode through a narrow stream using replacement mode.
    log4cplus::helpers::win32_open_options opts;
    opts.conversion_errors = log4cplus::helpers::conversion_error_policy::replace;
    log4cplus::helpers::win32_fstream fs;
    fs.open (name, std::ios_base::in | std::ios_base::binary, opts);
    char actual = 0;
    fs.get (actual);

    // Verify that Windows selected the code page's configured default byte.
    std::error_code default_char_error;
    unsigned char const default_char =
        log4cplus::helpers::detail::win32_code_page_default_char (
            log4cplus::helpers::detail::windows_us_ascii_code_page,
            default_char_error);
    CATCH_INFO ("query ASCII default character");
    CATCH_CHECK (! default_char_error);
    CATCH_INFO ("replacement uses code-page configured default");
    CATCH_CHECK (static_cast<unsigned char> (actual) == default_char);
    fs.close ();
    erase (name);
}

CATCH_TEST_CASE ("Malformed UTF-8 obeys the conversion policy",
                 "[unicode][conversion]") {
    // Define representative malformed UTF-8 byte sequences followed by text.
    wchar_t const * const name = L"malformed.txt";
    char const invalid_lead[] = {char (0xff), 'x'};
    char const invalid_continuation[] = {char (0xe2), '(', char (0xa1), 'x'};
    char const overlong[] = {char (0xc0), char (0xaf), 'x'};
    char const surrogate[] = {char (0xed), char (0xa0), char (0x80), 'x'};
    char const too_large[] = {char (0xf4), char (0x90), char (0x80),
                              char (0x80), 'x'};
    char const truncated[] = {char (0xe2), char (0x82), 'x'};

    struct malformed_vector {
        char const * bytes;
        DWORD size;
        char const * description;
    };

    malformed_vector const vectors[] = {
        {invalid_lead, sizeof (invalid_lead), "invalid UTF-8 lead byte"},
        {invalid_continuation, sizeof (invalid_continuation),
         "invalid UTF-8 continuation byte"},
        {overlong, sizeof (overlong), "overlong UTF-8 encoding"},
        {surrogate, sizeof (surrogate), "UTF-8 encoded surrogate"},
        {too_large, sizeof (too_large), "UTF-8 value above U+10FFFF"},
        {truncated, sizeof (truncated), "truncated UTF-8 sequence"},
    };

    // Exercise both conversion policies for every malformed sequence.
    for (std::size_t i = 0; i != sizeof (vectors) / sizeof (vectors[0]); ++i) {
        malformed_vector const & vector = vectors[i];

        // Write bytes directly so no encoder can sanitize the malformed input.
        CATCH_INFO (vector.description);
        CATCH_CHECK (raw_write (name, vector.bytes, vector.size));

        // Replacement mode emits U+FFFD and resumes at later valid input.
        log4cplus::helpers::win32_open_options replace;
        replace.conversion_errors =
            log4cplus::helpers::conversion_error_policy::replace;
        log4cplus::helpers::win32_wfstream forgiving;
        forgiving.open (name, std::ios_base::in | std::ios_base::binary,
                        replace);
        std::wstring decoded;
        wchar_t character = 0;
        while (forgiving.get (character)) {
            decoded.push_back (character);
        }
        CATCH_INFO ("replacement mode emits U+FFFD for malformed UTF-8");
        CATCH_CHECK (std::find (decoded.begin (), decoded.end (),
                                static_cast<wchar_t> (0xfffd))
                     != decoded.end ());
        CATCH_INFO ("replacement mode continues after malformed UTF-8");
        CATCH_CHECK (
            (!decoded.empty () && decoded[decoded.size () - 1] == L'x'));
        forgiving.close ();

        // Strict mode stops immediately and preserves the conversion error.
        log4cplus::helpers::win32_wfstream strict (
            name, std::ios_base::in | std::ios_base::binary);
        CATCH_INFO ("strict mode rejects malformed UTF-8");
        CATCH_CHECK (strict.get () == std::char_traits<wchar_t>::eof ());
        CATCH_INFO ("strict malformed input retains diagnostic");
        CATCH_CHECK (
            strict.last_error ()
            == std::make_error_code (std::errc::illegal_byte_sequence));
        strict.close ();
    }
    erase (name);
}

CATCH_TEST_CASE ("Malformed UTF-16 output obeys the conversion policy",
                 "[unicode][conversion]") {
    // Define malformed UTF-16 sequences and their replacement-mode UTF-8 bytes.
    wchar_t const * const name = L"malformed-wide.txt";
    wchar_t const lone_low[] = {static_cast<wchar_t> (0xdc00), L'x'};
    wchar_t const high_then_normal[] = {static_cast<wchar_t> (0xd800), L'x'};
    wchar_t const two_highs[] = {static_cast<wchar_t> (0xd800),
                                 static_cast<wchar_t> (0xd801), L'x'};
    wchar_t const trailing_high[] = {static_cast<wchar_t> (0xd800)};

    char const replacement_then_x[] = {char (0xef), char (0xbf), char (0xbd),
                                       'x'};
    char const two_replacements_then_x[] = {
        char (0xef), char (0xbf), char (0xbd), char (0xef),
        char (0xbf), char (0xbd), 'x'};
    char const replacement_only[] = {char (0xef), char (0xbf), char (0xbd)};

    struct malformed_vector {
        wchar_t const * input;
        std::streamsize input_size;
        char const * expected;
        std::size_t expected_size;
        char const * description;
    };

    malformed_vector const vectors[] = {
        {lone_low, sizeof (lone_low) / sizeof (lone_low[0]), replacement_then_x,
         sizeof (replacement_then_x), "lone low surrogate"},
        {high_then_normal,
         sizeof (high_then_normal) / sizeof (high_then_normal[0]),
         replacement_then_x, sizeof (replacement_then_x),
         "high surrogate followed by a normal character"},
        {two_highs, sizeof (two_highs) / sizeof (two_highs[0]),
         two_replacements_then_x, sizeof (two_replacements_then_x),
         "two consecutive high surrogates"},
        {trailing_high, sizeof (trailing_high) / sizeof (trailing_high[0]),
         replacement_only, sizeof (replacement_only),
         "trailing high surrogate"},
    };

    // Exercise strict and replacement output policies for every sequence.
    for (std::size_t i = 0; i != sizeof (vectors) / sizeof (vectors[0]); ++i) {
        malformed_vector const & vector = vectors[i];

        // Strict mode rejects malformed UTF-16 and records the diagnostic.
        log4cplus::helpers::win32_wfstream strict (
            name,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        strict.write (vector.input, vector.input_size);
        strict.close ();
        CATCH_INFO (vector.description);
        CATCH_CHECK (strict.fail ());
        CATCH_INFO ("strict malformed UTF-16 output retains diagnostic");
        CATCH_CHECK (
            strict.last_error ()
            == std::make_error_code (std::errc::illegal_byte_sequence));

        // Replacement mode writes U+FFFD and continues when input remains.
        log4cplus::helpers::win32_open_options replace;
        replace.conversion_errors =
            log4cplus::helpers::conversion_error_policy::replace;
        log4cplus::helpers::win32_wfstream forgiving;
        forgiving.open (name,
                        std::ios_base::out | std::ios_base::trunc
                            | std::ios_base::binary,
                        replace);
        forgiving.write (vector.input, vector.input_size);
        forgiving.close ();
        CATCH_INFO ("replacement mode accepts malformed UTF-16 output");
        CATCH_CHECK (!forgiving.fail ());

        // Verify the exact replacement bytes written to the file.
        std::vector<char> const actual = raw_read (name);
        std::vector<char> const expected (
            vector.expected, vector.expected + vector.expected_size);
        CATCH_INFO ("malformed UTF-16 output has expected replacement bytes");
        CATCH_CHECK (actual == expected);
    }

    erase (name);
}

CATCH_TEST_CASE ("Sharing and append modes preserve Win32 semantics",
                 "[open][sharing][append]") {
    // Remove delete sharing and verify that an open handle prevents rename.
    wchar_t const * const old_name = L"sharing-old.txt";
    wchar_t const * const new_name = L"sharing-new.txt";
    erase (old_name);
    erase (new_name);
    log4cplus::helpers::win32_open_options narrow;
    narrow.share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    log4cplus::helpers::win32_fstream locked;
    locked.open (old_name, std::ios_base::out | std::ios_base::trunc, narrow);
    CATCH_INFO ("share option can deny rename");
    CATCH_CHECK (MoveFileExW (old_name, new_name, MOVEFILE_REPLACE_EXISTING)
                 == 0);
    locked.close ();
    erase (old_name);
    erase (new_name);

    // Seed the append target to verify that app mode does not truncate it.
    wchar_t const * const append_name = L"append.txt";
    char const prefix[] = {'p', 'r', 'e', 'f', 'i', 'x', '-'};
    erase (append_name);
    CATCH_INFO ("create existing append target");
    CATCH_CHECK (raw_write (append_name, prefix, sizeof (prefix)));

    // Open two append streams and interleave writes to the shared file.
    log4cplus::helpers::win32_fstream a (append_name, std::ios_base::out
                                                      | std::ios_base::app
                                                      | std::ios_base::binary);
    log4cplus::helpers::win32_fstream b (append_name, std::ios_base::out
                                                      | std::ios_base::app
                                                      | std::ios_base::binary);
    CATCH_INFO ("app-mode streams report the existing EOF before writing");
    CATCH_CHECK ((static_cast<std::streamoff> (a.tellp ())
                  == static_cast<std::streamoff> (sizeof (prefix))
                  && static_cast<std::streamoff> (b.tellp ())
                         == static_cast<std::streamoff> (sizeof (prefix))));
    a << 'A' << std::flush;
    b << 'B' << std::flush;

    // A seek changes the logical cursor but app mode must still write at EOF.
    a.seekp (0, std::ios_base::beg);
    CATCH_INFO ("seek an app-mode stream");
    CATCH_CHECK (!a.fail ());
    a << 'C' << std::flush;
    a.close ();
    b.close ();

    // Verify preservation and that every physical write targeted current EOF.
    std::vector<char> bytes = raw_read (append_name);
    CATCH_INFO ("each app-mode write targets current EOF");
    CATCH_CHECK (std::string (bytes.begin (), bytes.end ()) == "prefix-ABC");
    erase (append_name);
}

CATCH_TEST_CASE ("tofstream preserves output file defaults",
                 "[open][output]") {
    wchar_t const * const name = L"tofstream-default.txt";
    erase (name);

    {
        log4cplus::tofstream out {std::filesystem::path (name)};
        CATCH_INFO ("default tofstream creates a new file");
        CATCH_REQUIRE (out.is_open ());
        out << LOG4CPLUS_TEXT ("created");
        out.close ();
    }
    std::vector<char> created = raw_read (name);
    CATCH_CHECK (std::string (created.begin (), created.end ()) == "created");

    char const old[] = {'o', 'l', 'd'};
    CATCH_REQUIRE (raw_write (name, old, sizeof (old)));
    {
        log4cplus::tofstream out {std::filesystem::path (name)};
        CATCH_INFO ("default tofstream truncates an existing file");
        CATCH_REQUIRE (out.is_open ());
        out << LOG4CPLUS_TEXT ("new");
        out.close ();
    }
    std::vector<char> truncated = raw_read (name);
    CATCH_CHECK (std::string (truncated.begin (), truncated.end ()) == "new");
    erase (name);
}

CATCH_TEST_CASE ("Byte positions can be saved and restored", "[seek]") {
    // Create a bidirectional binary stream with known contents.
    wchar_t const * const name = L"seek.txt";
    erase (name);
    log4cplus::helpers::win32_fstream fs (
        name, std::ios_base::in | std::ios_base::out | std::ios_base::trunc
                  | std::ios_base::binary);
    fs << "abc" << std::flush;

    // Verify beginning seeks and restoration of a position returned by tellg.
    fs.seekg (0, std::ios_base::beg);
    CATCH_INFO ("seek to beginning");
    CATCH_CHECK (fs.get () == 'a');
    std::streampos const saved = fs.tellg ();
    CATCH_INFO ("read after tell");
    CATCH_CHECK (fs.get () == 'b');
    fs.seekg (saved);
    CATCH_INFO ("seek to saved position");
    CATCH_CHECK (fs.get () == 'b');

    // Treat nonzero current-relative offsets as external byte counts.
    fs.clear ();
    fs.seekg (1, std::ios_base::cur);
    CATCH_INFO ("accept a current-relative byte seek");
    CATCH_CHECK ((!fs.fail () && fs.peek () == std::char_traits<char>::eof ()));
    fs.close ();
    erase (name);
}

CATCH_TEST_CASE ("Seeking resynchronizes at UTF-8 boundaries",
                 "[seek][unicode]") {
    // Create adjacent two-, three-, and four-byte UTF-8 sequences.
    wchar_t const * const name = L"multibyte-seek.txt";
    char const bytes[] = {'A',         char (0xc2), char (0xa2), char (0xe3),
                          char (0x81), char (0x8a), char (0xf0), char (0x93),
                          char (0x80), char (0x84), 'B'};
    erase (name);
    CATCH_INFO ("create multibyte seek input");
    CATCH_CHECK (raw_write (name, bytes, sizeof (bytes)));

    log4cplus::helpers::win32_wfstream fs (name, std::ios_base::in
                                                 | std::ios_base::binary);
    CATCH_INFO ("open multibyte seek input");
    CATCH_CHECK (fs.is_open ());

    // Positions returned by tellg remain exact and restorable.
    CATCH_INFO ("read ASCII before multibyte character");
    CATCH_CHECK (fs.get () == L'A');
    std::wstreampos const before = fs.tellg ();
    CATCH_INFO ("read two-byte character");
    CATCH_CHECK (fs.get () == L'\u00a2');
    std::wstreampos const after = fs.tellg ();
    fs.seekg (before);
    CATCH_INFO ("restore position before two-byte character");
    CATCH_CHECK (fs.get () == L'\u00a2');
    fs.seekg (after);
    CATCH_INFO ("restore position after two-byte character");
    CATCH_CHECK (fs.get () == L'\u304a');

    // Every continuation-byte target advances to the next leading byte.
    struct resynchronization_vector {
        std::streamoff requested;
        std::streamoff adjusted;
        wchar_t expected;
        bool expected_is_high_surrogate;
        char const * description;
    };

    resynchronization_vector const vectors[] = {
        {2, 3, L'\u304a', false, "two-byte continuation"},
        {4, 6, 0, true, "first three-byte continuation"},
        {5, 6, 0, true, "second three-byte continuation"},
        {7, 10, L'B', false, "first four-byte continuation"},
        {8, 10, L'B', false, "second four-byte continuation"},
        {9, 10, L'B', false, "third four-byte continuation"},
    };
    for (std::size_t i = 0; i != sizeof (vectors) / sizeof (vectors[0]); ++i) {
        resynchronization_vector const & vector = vectors[i];
        fs.clear ();
        fs.seekg (std::wstreampos (vector.requested));
        bool const position_matches =
            !fs.fail ()
            && static_cast<std::streamoff> (fs.tellg ()) == vector.adjusted;
        wchar_t const actual = fs.get ();
        bool const character_matches =
            vector.expected_is_high_surrogate
                ? log4cplus::helpers::detail::is_high_surrogate (actual)
                : actual == vector.expected;
        CATCH_INFO (vector.description);
        CATCH_CHECK ((position_matches && character_matches));
    }

    // Current-relative seeking is undefined between UTF-16 surrogate units.
    fs.clear ();
    fs.seekg (6, std::ios_base::beg);
    CATCH_INFO ("extract the first code unit of a supplementary scalar");
    CATCH_CHECK (log4cplus::helpers::detail::is_high_surrogate (fs.get ()));
    fs.seekg (1, std::ios_base::cur);
    CATCH_INFO ("reject current seek inside decoded scalar output");
    CATCH_CHECK (fs.fail ());

    // Beginning-, current-, and end-relative byte offsets all resynchronize.
    fs.clear ();
    fs.seekg (4, std::ios_base::beg);
    CATCH_INFO ("resynchronize a beginning-relative byte offset");
    CATCH_CHECK (
        (!fs.fail () && static_cast<std::streamoff> (fs.tellg ()) == 6));
    fs.seekg (0, std::ios_base::beg);
    CATCH_INFO ("restart before a current-relative seek");
    CATCH_CHECK (fs.get () == L'A');
    fs.seekg (1, std::ios_base::cur);
    CATCH_INFO ("resynchronize a current-relative byte offset");
    CATCH_CHECK (
        (!fs.fail () && static_cast<std::streamoff> (fs.tellg ()) == 3));
    fs.seekg (-2, std::ios_base::cur);
    CATCH_INFO ("apply a negative current-relative byte offset");
    CATCH_CHECK ((!fs.fail () && static_cast<std::streamoff> (fs.tellg ()) == 1
                  && fs.get () == L'\u00a2'));
    fs.seekg (-4, std::ios_base::end);
    CATCH_INFO ("resynchronize an end-relative byte offset");
    CATCH_CHECK ((!fs.fail () && static_cast<std::streamoff> (fs.tellg ()) == 10
                  && fs.get () == L'B'));

    // Positions beyond EOF remain unchanged and read as EOF.
    fs.clear ();
    fs.seekg (2, std::ios_base::end);
    CATCH_INFO ("preserve a byte position beyond EOF");
    CATCH_CHECK ((!fs.fail () && static_cast<std::streamoff> (fs.tellg ()) == 13
                  && fs.peek () == std::char_traits<wchar_t>::eof ()));

    // Reject targets before byte zero and targets outside streamoff range.
    fs.clear ();
    fs.seekg (-1, std::ios_base::beg);
    CATCH_INFO ("reject a negative absolute byte position");
    CATCH_CHECK (fs.fail ());
    fs.clear ();
    fs.seekg ((std::numeric_limits<std::streamoff>::max) (),
              std::ios_base::end);
    CATCH_INFO ("reject seek byte arithmetic overflow");
    CATCH_CHECK (fs.fail ());

    fs.close ();
    erase (name);
}

CATCH_TEST_CASE ("Seeking preserves malformed input diagnostics",
                 "[seek][unicode][conversion]") {
    // A malformed non-continuation lead remains a syntactic seek boundary.
    wchar_t const * const name = L"malformed-seek.txt";
    char const bytes[] = {'A', char (0xff), 'B'};
    erase (name);
    CATCH_INFO ("create malformed seek input");
    CATCH_CHECK (raw_write (name, bytes, sizeof (bytes)));

    log4cplus::helpers::win32_wfstream strict (name, std::ios_base::in
                                                     | std::ios_base::binary);
    strict.seekg (1, std::ios_base::beg);
    CATCH_INFO ("seek does not skip a malformed leading byte");
    CATCH_CHECK (static_cast<std::streamoff> (strict.tellg ()) == 1);
    CATCH_INFO ("strict conversion still reports malformed seek input");
    CATCH_CHECK (
        (strict.get () == std::char_traits<wchar_t>::eof ()
         && strict.last_error ()
                == std::make_error_code (std::errc::illegal_byte_sequence)));
    strict.close ();

    // Replacement mode observes the same byte and then continues normally.
    log4cplus::helpers::win32_open_options replace;
    replace.conversion_errors =
        log4cplus::helpers::conversion_error_policy::replace;
    log4cplus::helpers::win32_wfstream forgiving;
    forgiving.open (name, std::ios_base::in | std::ios_base::binary, replace);
    forgiving.seekg (1, std::ios_base::beg);
    CATCH_INFO ("replacement conversion remains active after a seek");
    CATCH_CHECK ((forgiving.get () == static_cast<wchar_t> (0xfffd)
                  && forgiving.get () == L'B'));
    forgiving.close ();

    // A malformed continuation run is traversed without being decoded.
    char const continuations[] = {'A',         char (0x80), char (0x81),
                                  char (0x82), char (0x83), char (0x84),
                                  'B'};
    CATCH_INFO ("create malformed continuation seek input");
    CATCH_CHECK (raw_write (name, continuations, sizeof (continuations)));
    log4cplus::helpers::win32_wfstream resynchronized (
        name, std::ios_base::in | std::ios_base::binary);
    resynchronized.seekg (1, std::ios_base::beg);
    CATCH_INFO ("seek traverses a long malformed continuation run");
    CATCH_CHECK ((!resynchronized.fail ()
                  && static_cast<std::streamoff> (resynchronized.tellg ()) == 6
                  && resynchronized.get () == L'B'));
    resynchronized.close ();
    erase (name);
}

CATCH_TEST_CASE ("Output seeking uses adjusted byte positions",
                 "[seek][output][append]") {
    // Build UTF-8 through an output-only handle that also has read access.
    wchar_t const * const name = L"output-seek.txt";
    erase (name);
    log4cplus::helpers::win32_wfstream output (name, std::ios_base::out
                                                     | std::ios_base::trunc
                                                     | std::ios_base::binary);
    wchar_t const initial[] = {L'A', L'\u304a', L'B'};
    output.write (initial, 3);
    output.flush ();
    std::wstreampos const original_end = output.tellp ();

    // Verify the output-only native handle can inspect seek targets.
    char first = 0;
    DWORD got = 0;
    OVERLAPPED read_at_zero = {};
    CATCH_INFO ("output-only handle includes read access");
    CATCH_CHECK (
        (ReadFile (output.native_handle (), &first, 1, &got, &read_at_zero)
         && got == 1 && first == 'A'));

    // An absolute output target inside U+304A advances to its following byte.
    output.seekp (std::wstreampos (2));
    CATCH_INFO ("resynchronize an output-only byte position");
    CATCH_CHECK ((!output.fail ()
                  && static_cast<std::streamoff> (output.tellp ()) == 4));
    output.put (L'X');

    // Positions returned by tellp remain exactly restorable without history.
    output.seekp (original_end);
    CATCH_INFO ("restore an output position returned by tellp");
    CATCH_CHECK ((!output.fail ()
                  && static_cast<std::streamoff> (output.tellp ()) == 5));
    output.put (L'Y');

    // Seeking beyond EOF permits a later write to create a zero-filled gap.
    output.seekp (7, std::ios_base::beg);
    CATCH_INFO ("preserve output position beyond EOF");
    CATCH_CHECK ((!output.fail ()
                  && static_cast<std::streamoff> (output.tellp ()) == 7));
    output.put (L'Z');
    output.close ();
    char const expected[] = {'A', char (0xe3), char (0x81), char (0x8a),
                             'X', 'Y',         0,           'Z'};
    CATCH_INFO ("output seeking overwrites at adjusted and sparse positions");
    CATCH_CHECK (raw_read (name)
                 == std::vector<char> (expected, expected + sizeof (expected)));

    // App mode still writes at EOF after its logical target is adjusted.
    log4cplus::helpers::win32_wfstream append (
        name, std::ios_base::out | std::ios_base::app | std::ios_base::binary);
    append.seekp (std::wstreampos (2));
    CATCH_INFO ("resynchronize an app-mode logical position");
    CATCH_CHECK ((!append.fail ()
                  && static_cast<std::streamoff> (append.tellp ()) == 4));
    append.put (L'C');
    append.close ();
    std::vector<char> const appended = raw_read (name);
    CATCH_INFO ("app mode ignores adjusted logical position for writes");
    CATCH_CHECK ((appended.size () == sizeof (expected) + 1
                  && appended[appended.size () - 1] == 'C'));
    erase (name);
}

CATCH_TEST_CASE ("External byte buffers can be configured", "[buffer]") {
    wchar_t const * const name = L"configured-buffer.txt";
    erase (name);

    // Reject storage that cannot hold one complete UTF-8 sequence.
    log4cplus::helpers::win32_fstream invalid;
    char too_small[3] = {};
    CATCH_INFO ("reject a three-byte external buffer");
    CATCH_CHECK (invalid.rdbuf ()->pubsetbuf (too_small, 3) == nullptr);
    CATCH_INFO ("small external buffer reports invalid argument");
    CATCH_CHECK (invalid.last_error ()
                 == std::make_error_code (std::errc::invalid_argument));

    // Reject inconsistent null/count pairs without changing configuration.
    char adequate[4] = {};
    CATCH_INFO ("reject null storage with a nonzero size");
    CATCH_CHECK (invalid.rdbuf ()->pubsetbuf (nullptr, 1) == nullptr);
    CATCH_INFO ("reject non-null storage with a zero size");
    CATCH_CHECK (invalid.rdbuf ()->pubsetbuf (adequate, 0) == nullptr);

    // Reject element counts whose byte capacity would overflow size_t.
    log4cplus::helpers::win32_u32fstream overflow;
    char32_t overflow_storage[1] = {};
    CATCH_INFO ("reject overflowing external buffer size");
    CATCH_CHECK (
        overflow.rdbuf ()->pubsetbuf (
            overflow_storage, (std::numeric_limits<std::streamsize>::max) ())
        == nullptr);

    // Use exactly four caller-owned bytes for supplementary UTF-8 output.
    log4cplus::helpers::win32_wfstream output;
    wchar_t external[2] = {};
    CATCH_INFO ("accept a four-byte external buffer");
    CATCH_CHECK (output.rdbuf ()->pubsetbuf (external, 2) == output.rdbuf ());
    output.open (name, std::ios_base::out | std::ios_base::trunc
                           | std::ios_base::binary);
    CATCH_INFO ("open output with an external buffer");
    CATCH_CHECK (output.is_open ());

    // Buffer replacement is forbidden after the file has been opened.
    wchar_t replacement[2] = {};
    CATCH_INFO ("reject buffer replacement after open");
    CATCH_CHECK (output.rdbuf ()->pubsetbuf (replacement, 2) == nullptr);
    wchar_t const hieroglyph[] = L"\U00013004";
    output.write (hieroglyph,
                  static_cast<std::streamsize> (
                      sizeof (hieroglyph) / sizeof (hieroglyph[0]) - 1));
    output.close ();
    char const expected[] = {char (0xf0), char (0x93), char (0x80),
                             char (0x84)};
    CATCH_INFO ("external output buffer preserves supplementary UTF-8");
    CATCH_CHECK (raw_read (name)
                 == std::vector<char> (expected, expected + sizeof (expected)));

    // A null zero-sized request selects the four-byte internal staging mode.
    log4cplus::helpers::win32_wfstream minimal;
    CATCH_INFO ("accept minimally buffered mode");
    CATCH_CHECK (minimal.rdbuf ()->pubsetbuf (nullptr, 0) == minimal.rdbuf ());
    minimal.open (name, std::ios_base::in | std::ios_base::binary);
    wchar_t decoded[2] = {};
    minimal.read (decoded, 2);
    CATCH_INFO ("minimal input buffer decodes supplementary UTF-8");
    CATCH_CHECK ((decoded[0] == hieroglyph[0] && decoded[1] == hieroglyph[1]));
    minimal.close ();

    // The selected minimal buffer persists across close and reopen.
    minimal.open (name, std::ios_base::out | std::ios_base::trunc
                            | std::ios_base::binary);
    minimal.put (L'Z');
    minimal.close ();
    CATCH_INFO ("minimal buffer persists for later output");
    CATCH_CHECK (raw_read (name) == std::vector<char> (1, 'Z'));
    erase (name);
}

CATCH_TEST_CASE ("Buffered input spans encoding boundaries",
                 "[buffer][unicode][text]") {
    // Arrange BOM, CRLF, BMP, and supplementary sequences across refills.
    wchar_t const * const name = L"buffer-boundaries.txt";
    char const bytes[] = {char (0xef), char (0xbb), char (0xbf), 'A',
                          '\r',        '\n',        char (0xe3), char (0x81),
                          char (0x8a), char (0xf0), char (0x93), char (0x80),
                          char (0x84), 'Z'};
    erase (name);
    CATCH_INFO ("create buffered boundary input");
    CATCH_CHECK (raw_write (name, bytes, sizeof (bytes)));

    // Decode through an external buffer whose raw capacity is four bytes.
    log4cplus::helpers::win32_wfstream fs;
    wchar_t external[2] = {};
    CATCH_INFO ("configure four-byte input buffer");
    CATCH_CHECK (fs.rdbuf ()->pubsetbuf (external, 2) == fs.rdbuf ());
    fs.open (name, std::ios_base::in);
    wchar_t actual[6] = {};
    fs.read (actual, 6);
    wchar_t const expected[] = {L'A',
                                L'\n',
                                L'\u304a',
                                static_cast<wchar_t> (0xd80c),
                                static_cast<wchar_t> (0xdc04),
                                L'Z'};
    CATCH_INFO ("bulk input handles UTF-8 and text boundaries");
    CATCH_CHECK (
        (fs.gcount () == 6 && std::equal (actual, actual + 6, expected)));
    fs.close ();
    erase (name);
}

CATCH_TEST_CASE ("Buffered streams preserve positions across directions",
                 "[buffer][direction]") {
    wchar_t const * const name = L"buffer-direction.txt";
    char const original[] = {'A', 'B', 'C', 'D', 'E'};
    erase (name);
    CATCH_INFO ("create direction-change input");
    CATCH_CHECK (raw_write (name, original, sizeof (original)));

    // Read ahead through a four-byte cache but consume only one character.
    log4cplus::helpers::win32_fstream fs;
    char external[4] = {};
    CATCH_INFO ("configure direction-change buffer");
    CATCH_CHECK (fs.rdbuf ()->pubsetbuf (external, 4) == fs.rdbuf ());
    fs.open (name,
             std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    CATCH_INFO ("consume one buffered input character");
    CATCH_CHECK (fs.get () == 'A');
    CATCH_INFO ("bulk read fills the caller-owned buffer");
    CATCH_CHECK (std::string (external, external + 4) == "ABCD");

    // Switching to output must use the consumed position, not read-ahead EOF.
    fs.put ('X');
    fs.flush ();
    fs.seekg (0, std::ios_base::beg);
    char actual[5] = {};
    fs.read (actual, 5);
    CATCH_INFO ("read-ahead does not move the logical write position");
    CATCH_CHECK (std::string (actual, actual + 5) == "AXCDE");

    // A current-position query reports consumed bytes, not prefetched bytes.
    fs.clear ();
    fs.seekg (0, std::ios_base::beg);
    CATCH_INFO ("restart buffered position query");
    CATCH_CHECK (fs.get () == 'A');
    CATCH_INFO ("tellg ignores prefetched input bytes");
    CATCH_CHECK (static_cast<std::streamoff> (fs.tellg ()) == 1);
    fs.close ();
    erase (name);
}

CATCH_TEST_CASE ("Putback preserves one extracted code unit",
                 "[buffer][putback]") {
    // Include single-unit, BMP, and supplementary wide characters.
    wchar_t const * const name = L"putback.txt";
    char const bytes[] = {'A',         char (0xe3), char (0x81), char (0x8a),
                          'B',         char (0xf0), char (0x93), char (0x80),
                          char (0x84), 'C'};
    erase (name);
    CATCH_INFO ("create putback input");
    CATCH_CHECK (raw_write (name, bytes, sizeof (bytes)));

    log4cplus::helpers::win32_wfstream fs;
    wchar_t external[2] = {};
    CATCH_INFO ("configure putback input buffer");
    CATCH_CHECK (fs.rdbuf ()->pubsetbuf (external, 2) == fs.rdbuf ());
    fs.open (name, std::ios_base::in | std::ios_base::binary);

    // Immediate unget restores the last extracted code unit exactly once.
    CATCH_INFO ("extract character for immediate putback");
    CATCH_CHECK (fs.get () == L'A');
    fs.unget ();
    CATCH_INFO ("put back the immediately preceding character");
    CATCH_CHECK (!fs.fail ());
    fs.unget ();
    CATCH_INFO ("reject putback before the available history");
    CATCH_CHECK (fs.fail ());
    fs.clear ();
    CATCH_INFO ("re-extract an immediately put-back character");
    CATCH_CHECK (fs.get () == L'A');
    fs.putback (L'A');
    CATCH_INFO ("same-character putback restores the previous character");
    CATCH_CHECK ((!fs.fail () && fs.get () == L'A'));

    // Lookahead must not discard the previously extracted character.
    CATCH_INFO ("peek after an extracted character");
    CATCH_CHECK (fs.peek () == L'\u304a');
    fs.unget ();
    CATCH_INFO ("putback survives a subsequent underflow caused by peek");
    CATCH_CHECK ((!fs.fail () && fs.get () == L'A' && fs.get () == L'\u304a'));

    // Replacing the previous character with a different value is unsupported.
    fs.putback (L'X');
    CATCH_INFO ("reject mismatched putback character");
    CATCH_CHECK (fs.fail ());
    fs.clear ();
    CATCH_INFO ("continue after rejected mismatched putback");
    CATCH_CHECK (fs.get () == L'B');

    // Preserve the last UTF-16 code unit of a supplementary scalar.
    wchar_t const high = fs.get ();
    wchar_t const low = fs.get ();
    CATCH_INFO ("extract a supplementary UTF-16 surrogate pair");
    CATCH_CHECK ((log4cplus::helpers::detail::is_high_surrogate (high)
                  && log4cplus::helpers::detail::is_low_surrogate (low)));
    CATCH_INFO ("peek beyond a supplementary character");
    CATCH_CHECK (fs.peek () == L'C');
    fs.unget ();
    CATCH_INFO ("reject tellg inside a decoded supplementary scalar");
    CATCH_CHECK (fs.tellg () == std::wstreampos (std::streamoff (-1)));
    fs.clear ();
    CATCH_INFO ("put back the last supplementary code unit after lookahead");
    CATCH_CHECK ((!fs.fail () && fs.get () == low && fs.get () == L'C'));

    // EOF probing retains the last extracted character for putback.
    CATCH_INFO ("probe EOF after the final character");
    CATCH_CHECK (fs.peek () == std::char_traits<wchar_t>::eof ());
    fs.clear ();
    fs.unget ();
    CATCH_INFO ("putback survives an EOF probe");
    CATCH_CHECK ((!fs.fail () && fs.get () == L'C'));
    fs.close ();
    erase (name);
}

/** @brief Verifies one UTF-8 file round trip for a stream character type. */
template <typename CharT>
void typed_round_trip (wchar_t const * name, CharT value) {
    erase (name);

    // Encode one character of the selected stream type as UTF-8.
    {
        log4cplus::helpers::basic_win32_fstream<CharT> fs (
            name,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        fs.write (&value, 1);
        fs.close ();
        CATCH_INFO ("typed UTF-8 output");
        CATCH_CHECK (!fs.fail ());
    }

    // Decode the file back into the same stream character type.
    {
        log4cplus::helpers::basic_win32_fstream<CharT> fs (
            name, std::ios_base::in | std::ios_base::binary);
        CharT actual = 0;
        fs.read (&actual, 1);
        CATCH_INFO ("typed UTF-8 round trip");
        CATCH_CHECK (actual == value);
        fs.close ();
    }
    erase (name);
}

/** @brief Verifies exact UTF-8 bytes and wide-character decoding. */
template <std::size_t InputSize, std::size_t ExpectedSize>
void wide_utf8_bytes_test (char const * file_name,
                           wchar_t const (&input)[InputSize],
                           char const (&expected)[ExpectedSize],
                           char const * description) {
    DeleteFileA (file_name);

    // Encode the escaped wide input through the custom stream.
    log4cplus::helpers::win32_wfstream output (
        file_name,
        std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    CATCH_INFO ("open wide UTF-8 test output");
    CATCH_CHECK (output.is_open ());
    output.write (input, InputSize - 1);
    output.close ();
    CATCH_INFO ("write wide UTF-8 test output");
    CATCH_CHECK (!output.fail ());

    // Read with std::ifstream and compare the exact external UTF-8 bytes.
    std::ifstream bytes (file_name, std::ios_base::in | std::ios_base::binary);
    CATCH_INFO ("open UTF-8 output with std::ifstream");
    CATCH_CHECK (bytes.is_open ());
    std::string const actual ((std::istreambuf_iterator<char> (bytes)),
                              std::istreambuf_iterator<char> ());
    std::string const wanted (expected, ExpectedSize - 1);
    CATCH_INFO (description);
    CATCH_CHECK (actual == wanted);

    bytes.close ();

    // Decode through the custom wide stream and compare with the original.
    log4cplus::helpers::win32_wfstream decoded (
        file_name, std::ios_base::in | std::ios_base::binary);
    CATCH_INFO ("reopen UTF-8 output as a wide stream");
    CATCH_CHECK (decoded.is_open ());
    std::wstring round_trip (InputSize - 1, L'\0');
    decoded.read (&round_trip[0], InputSize - 1);
    std::wstring const original (input, InputSize - 1);
    CATCH_INFO ("UTF-8 output decodes to the original wide string");
    CATCH_CHECK (
        (decoded.gcount () == static_cast<std::streamsize> (InputSize - 1)
         && round_trip == original));
    decoded.close ();

    DeleteFileA (file_name);
}

CATCH_TEST_CASE ("Wide strings have exact UTF-8 representations",
                 "[unicode][round-trip]") {
    // Japanese ohayō written entirely with universal character escapes.
    wchar_t const ohayo[] = L"\u304a\u306f\u3088\u3046";
    char const ohayo_utf8[] =
        "\xe3\x81\x8a\xe3\x81\xaf\xe3\x82\x88\xe3\x81\x86";
    wide_utf8_bytes_test ("ohayo-utf8.txt", ohayo, ohayo_utf8,
                          "Japanese ohayo has expected UTF-8 bytes");

    // Icelandic “ábyrgð” with escapes for the non-ASCII characters.
    wchar_t const abyrgd[] = L"\u00e1byrg\u00f0";
    char const abyrgd_utf8[] = "\xc3\xa1"
                               "byrg"
                               "\xc3\xb0";
    wide_utf8_bytes_test ("abyrgd-utf8.txt", abyrgd, abyrgd_utf8,
                          "Icelandic abyrgd has expected UTF-8 bytes");

    // Egyptian hieroglyph U+13004 exercises a character outside the BMP.
    wchar_t const hieroglyph[] = L"\U00013004";
    char const hieroglyph_utf8[] = "\xf0\x93\x80\x84";
    wide_utf8_bytes_test ("hieroglyph-utf8.txt", hieroglyph, hieroglyph_utf8,
                          "out-of-BMP hieroglyph has expected UTF-8 bytes");
}

CATCH_TEST_CASE ("UTF-16 stream characters round trip through UTF-8",
                 "[unicode][round-trip]") {
    typed_round_trip<char16_t> (L"u16.txt", static_cast<char16_t> (0x5fc3));
}

CATCH_TEST_CASE ("UTF-32 stream characters round trip through UTF-8",
                 "[unicode][round-trip]") {
    typed_round_trip<char32_t> (L"u32.txt", static_cast<char32_t> (0x1f642));
}

#if defined(__cpp_char8_t)
CATCH_TEST_CASE ("UTF-8 stream characters round trip through UTF-8",
                 "[unicode][round-trip]") {
    typed_round_trip<char8_t> (L"u8.txt", static_cast<char8_t> ('x'));
}
#endif

} // namespace

#endif // LOG4CPLUS_WITH_UNIT_TESTS

#endif // _WIN32
