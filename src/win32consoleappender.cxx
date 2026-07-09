//  Copyright (C) 2009-2017, Vaclav Haisman. All rights reserved.
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

#if defined(_WIN32) && defined (LOG4CPLUS_HAVE_WIN32_CONSOLE)
#include <log4cplus/config/windowsh-inc.h>
#include <log4cplus/win32consoleappender.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/streams.h>
#include <algorithm>
#include <limits>
#include <sstream>
#include <string>

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
#include <catch_amalgamated.hpp>
#endif

/* list of available colors which can be OR'ed together and provided as an INT in the config file, e.g.:
       log4cplus.appender.INFO_MSGS.TextColor=36
   for red text on green background

#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.
*/


namespace log4cplus
{

namespace
{

unsigned int const color_mode_classic = 0;
unsigned int const color_mode_vt = 1;
unsigned int const color_mode_force_vt = 2;

bool
parse_color_mode (tstring const & value, unsigned int & mode)
{
    if (value == LOG4CPLUS_TEXT ("classic"))
    {
        mode = color_mode_classic;
        return true;
    }
    else if (value == LOG4CPLUS_TEXT ("vt"))
    {
        mode = color_mode_vt;
        return true;
    }
    else if (value == LOG4CPLUS_TEXT ("force-vt"))
    {
        mode = color_mode_force_vt;
        return true;
    }

    mode = color_mode_classic;
    return false;
}


unsigned int
win32_rgb_to_sgr_index (unsigned int color, unsigned int red,
    unsigned int green, unsigned int blue)
{
    unsigned int index = 0;

    if (color & red)
        index |= 1;
    if (color & green)
        index |= 2;
    if (color & blue)
        index |= 4;

    return index;
}


std::string
win32_text_color_to_sgr (unsigned int color)
{
    unsigned int const foreground_color_bits
        = color & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    unsigned int const foreground_bits
        = foreground_color_bits | (color & FOREGROUND_INTENSITY);
    unsigned int const background_color_bits
        = color & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
    unsigned int const background_bits
        = background_color_bits | (color & BACKGROUND_INTENSITY);

    if (! foreground_bits && ! background_bits)
        return std::string ();

    std::string sgr ("\x1b[");
    bool have_code = false;

    if (foreground_bits)
    {
        unsigned int const foreground_index = win32_rgb_to_sgr_index (
            foreground_color_bits, FOREGROUND_RED, FOREGROUND_GREEN,
            FOREGROUND_BLUE);
        unsigned int const foreground_base
            = color & FOREGROUND_INTENSITY ? 90 : 30;

        sgr += std::to_string (foreground_base + foreground_index);
        have_code = true;
    }

    if (background_bits)
    {
        unsigned int const background_index = win32_rgb_to_sgr_index (
            background_color_bits, BACKGROUND_RED, BACKGROUND_GREEN,
            BACKGROUND_BLUE);
        unsigned int const background_base
            = color & BACKGROUND_INTENSITY ? 100 : 40;

        if (have_code)
            sgr += ';';
        sgr += std::to_string (background_base + background_index);
    }

    sgr += 'm';
    return sgr;
}


std::string
to_handle_bytes (tchar const * s, std::size_t str_len)
{
#if defined (UNICODE)
    std::wstring wstr (s, str_len);
    return helpers::tostring (wstr);
#else
    return std::string (s, str_len);
#endif
}


tstring
ascii_to_tstring (std::string const & str)
{
    return tstring (str.begin (), str.end ());
}


bool
write_handle_bytes (HANDLE out, char const * s, std::size_t str_len)
{
    if (str_len == 0)
        return true;

    std::size_t total_written = 0;

    do
    {
        DWORD const to_write = static_cast<DWORD> (
            (std::min<std::size_t>) (std::numeric_limits<DWORD>::max (),
                str_len - total_written));
        DWORD written = 0;

        BOOL ret = WriteFile (out, s + total_written, to_write, &written, 0);
        if (! ret)
        {
            helpers::getLogLog ().error (
                LOG4CPLUS_TEXT ("Win32ConsoleAppender::write_handle")
                LOG4CPLUS_TEXT ("- WriteFile has failed."));
            return false;
        }

        total_written += written;
    }
    while (total_written != str_len);

    return true;
}


bool
write_console_text (HANDLE console_out, tchar const * s, std::size_t str_len)
{
    if (str_len == 0)
        return true;

    std::size_t total_written = 0;

    do
    {
        DWORD const to_write = static_cast<DWORD> (
            (std::min<std::size_t>) (64*1024 - 1,
                str_len - total_written));
        DWORD written = 0;

        BOOL ret = WriteConsole (console_out, s + total_written, to_write,
            &written, 0);
        if (! ret)
        {
            helpers::getLogLog ().error (
                LOG4CPLUS_TEXT ("Win32ConsoleAppender::write_console")
                LOG4CPLUS_TEXT ("- WriteConsole has failed."));
            return false;
        }

        total_written += written;
    }
    while (total_written != str_len);

    return true;
}


} // namespace


Win32ConsoleAppender::Win32ConsoleAppender (bool allocConsole, bool logToStdErr, unsigned int textColor)
    : alloc_console (allocConsole)
    , log_to_std_err (logToStdErr)
    , text_color (textColor)
    , color_mode (color_mode_classic)
    , console_vt_failed (false)
{ }


Win32ConsoleAppender::Win32ConsoleAppender (
    helpers::Properties const & properties)
    : Appender (properties)
    , alloc_console (true)
    , log_to_std_err (false)
    , text_color (0)
    , color_mode (color_mode_classic)
    , console_vt_failed (false)
{
    properties.getBool (alloc_console, LOG4CPLUS_TEXT ("AllocConsole"));
    properties.getBool (log_to_std_err, LOG4CPLUS_TEXT ("logToStdErr"));
    properties.getUInt (text_color, LOG4CPLUS_TEXT ("TextColor"));

    tstring const color_mode_value = properties.getProperty (
        LOG4CPLUS_TEXT ("ColorMode"));
    if (! color_mode_value.empty ()
        && ! parse_color_mode (color_mode_value, color_mode))
    {
        helpers::getLogLog ().warn (
            LOG4CPLUS_TEXT ("Win32ConsoleAppender: unknown ColorMode value [")
            + color_mode_value
            + LOG4CPLUS_TEXT ("], using classic."));
    }
}


Win32ConsoleAppender::~Win32ConsoleAppender ()
{
    destructorImpl();
}


void
Win32ConsoleAppender::close ()
{
    closed = true;
}


void
Win32ConsoleAppender::append (spi::InternalLoggingEvent const & event)
{
    if (alloc_console)
        // We ignore the return value here. If we already have a console,
        // it will fail.
        AllocConsole ();

    HANDLE const console_out = GetStdHandle (
        log_to_std_err ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    if (console_out == INVALID_HANDLE_VALUE)
    {
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("Win32ConsoleAppender::append")
            LOG4CPLUS_TEXT ("- Unable to get STD_OUTPUT_HANDLE."));
        return;
    }

    DWORD const handle_type = GetFileType (console_out);
    if (handle_type == FILE_TYPE_UNKNOWN && GetLastError () != NO_ERROR)
    {
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("Win32ConsoleAppender::append")
            LOG4CPLUS_TEXT ("- Error retrieving handle type."));
        return;
    }

    tstring & str = formatEvent (event);
    std::size_t const str_len = str.size ();
    const tchar * s = str.c_str ();
    DWORD mode;

    if (handle_type == FILE_TYPE_CHAR && GetConsoleMode (console_out, &mode))
    {
        // It seems that we have real console handle here. We can use
        // WriteConsole() directly.
        if (text_color && color_mode != color_mode_classic
            && ! console_vt_failed)
        {
            if (enable_console_vt (console_out))
                write_console_vt (console_out, s, str_len);
            else
            {
                console_vt_failed = true;
                write_console (console_out, s, str_len);
            }
        }
        else
            write_console (console_out, s, str_len);
    }
    else
    {
        // It seems that console is redirected.
        if (text_color && color_mode == color_mode_force_vt)
            write_handle_vt (console_out, s, str_len);
        else
            write_handle (console_out, s, str_len);
    }
}


void
Win32ConsoleAppender::write_handle (void * outvoid, tchar const * s,
    std::size_t str_len)
{
    HANDLE out = static_cast<HANDLE>(outvoid);
    std::string const str = to_handle_bytes (s, str_len);
    write_handle_bytes (out, str.data (), str.size ());
}


void
Win32ConsoleAppender::write_handle_vt (void * outvoid, tchar const * s,
    std::size_t str_len)
{
    HANDLE out = static_cast<HANDLE>(outvoid);
    if (str_len == 0)
        return;

    std::string str = win32_text_color_to_sgr (text_color);

    str += to_handle_bytes (s, str_len);
    str += "\x1b[0m";

    write_handle_bytes (out, str.data (), str.size ());
}


void
Win32ConsoleAppender::write_console (void * console_void, tchar const * s,
    std::size_t str_len)
{
    HANDLE console_out = static_cast<HANDLE>(console_void);
    BOOL ret = FALSE;
    unsigned int oldColor = 0;

    if (text_color)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
        ret = GetConsoleScreenBufferInfo (console_out, &csbiInfo);
        if (! ret)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Win32ConsoleAppender::write_console:")
                LOG4CPLUS_TEXT(" GetConsoleScreenBufferInfo failed"));
            // fallback to standard gray on black
            oldColor = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
            goto output;
        }

        // store old color first
        oldColor = csbiInfo.wAttributes;

        // set new color
        ret = SetConsoleTextAttribute (console_out, text_color);
        if (! ret)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Win32ConsoleAppender::write_console:")
                LOG4CPLUS_TEXT(" SetConsoleTextAttribute failed"));
        }
    }

output:;
    write_console_text (console_out, s, str_len);

    if (text_color)
    {
        // restore old color again
        ret = SetConsoleTextAttribute (console_out, oldColor);
        if (! ret)
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Win32ConsoleAppender::write_console:")
                LOG4CPLUS_TEXT(" SetConsoleTextAttribute failed"));
    }
}


bool
Win32ConsoleAppender::enable_console_vt (void * console_void)
{
    HANDLE console_out = static_cast<HANDLE>(console_void);
    DWORD mode = 0;

    if (! GetConsoleMode (console_out, &mode))
    {
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("Win32ConsoleAppender::enable_console_vt")
            LOG4CPLUS_TEXT ("- GetConsoleMode has failed."));
        return false;
    }

    DWORD const requested_mode
        = mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (requested_mode != mode
        && ! SetConsoleMode (console_out, requested_mode))
    {
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("Win32ConsoleAppender::enable_console_vt")
            LOG4CPLUS_TEXT ("- SetConsoleMode has failed; falling back to ")
            LOG4CPLUS_TEXT ("classic console colors."));
        return false;
    }

    return true;
}


void
Win32ConsoleAppender::write_console_vt (void * console_void, tchar const * s,
    std::size_t str_len)
{
    HANDLE console_out = static_cast<HANDLE>(console_void);
    if (str_len == 0)
        return;

    std::string const sgr = win32_text_color_to_sgr (text_color);
    tstring str = ascii_to_tstring (sgr);

    str.append (s, str_len);
    str += LOG4CPLUS_TEXT ("\x1b[0m");

    write_console_text (console_out, str.c_str (), str.size ());
}


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)

CATCH_TEST_CASE ("Win32ConsoleAppender color mode parsing",
    "[win32][console]")
{
    unsigned int mode = color_mode_classic;

    CATCH_CHECK (parse_color_mode (LOG4CPLUS_TEXT ("classic"), mode));
    CATCH_CHECK (mode == color_mode_classic);

    CATCH_CHECK (parse_color_mode (LOG4CPLUS_TEXT ("vt"), mode));
    CATCH_CHECK (mode == color_mode_vt);

    CATCH_CHECK (parse_color_mode (LOG4CPLUS_TEXT ("force-vt"), mode));
    CATCH_CHECK (mode == color_mode_force_vt);

    CATCH_CHECK (! parse_color_mode (LOG4CPLUS_TEXT ("bogus"), mode));
    CATCH_CHECK (mode == color_mode_classic);

    CATCH_CHECK (! parse_color_mode (LOG4CPLUS_TEXT ("VT"), mode));
    CATCH_CHECK (mode == color_mode_classic);
}


CATCH_TEST_CASE ("Win32ConsoleAppender color attributes convert to SGR",
    "[win32][console]")
{
    CATCH_CHECK (win32_text_color_to_sgr (0).empty ());
    CATCH_CHECK (win32_text_color_to_sgr (FOREGROUND_RED) == "\x1b[31m");
    CATCH_CHECK (win32_text_color_to_sgr (
        FOREGROUND_GREEN | FOREGROUND_INTENSITY) == "\x1b[92m");
    CATCH_CHECK (win32_text_color_to_sgr (BACKGROUND_BLUE) == "\x1b[44m");
    CATCH_CHECK (win32_text_color_to_sgr (
        FOREGROUND_RED | FOREGROUND_GREEN | BACKGROUND_BLUE
        | BACKGROUND_INTENSITY) == "\x1b[33;104m");
}

#endif // defined (LOG4CPLUS_WITH_UNIT_TESTS)


} // namespace log4cplus

#endif
