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

#if defined (LOG4CPLUS_HAVE_WIN32_TRACELOGGING)

#include <log4cplus/config/windowsh-inc-full.h>
#include <traceloggingprovider.h>
#include <winmeta.h>

#include <log4cplus/win32traceloggingappender.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/loglevel.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/thread/syncprims-pub-impl.h>

#include <cstdint>
#include <mutex>
#include <string>

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
#include <catch_amalgamated.hpp>
#endif


TRACELOGGING_DEFINE_PROVIDER (
    g_log4cplus_trace_logging_provider,
    "log4cplus.Win32TraceLoggingAppender",
    (0x4662f114, 0x3cc3, 0x49f2, 0xb6, 0x9d, 0xe4, 0xaa, 0x24, 0x6e,
        0x3e, 0xcd));


#if defined (UNICODE)
#define LOG4CPLUS_TLG_TSTRING(value, name) \
    TraceLoggingWideString ((value), (name))
#else
#define LOG4CPLUS_TLG_TSTRING(value, name) \
    TraceLoggingString ((value), (name))
#endif

#define LOG4CPLUS_TLG_WRITE(event_name, event_level)                         \
    TraceLoggingWrite (                                                       \
        g_log4cplus_trace_logging_provider, event_name,                       \
        TraceLoggingLevel (event_level),                                      \
        LOG4CPLUS_TLG_TSTRING (message, "Message"),                          \
        LOG4CPLUS_TLG_TSTRING (logger, "Logger"),                            \
        LOG4CPLUS_TLG_TSTRING (level_name_ptr, "LevelName"),                 \
        TraceLoggingInt32 (level_value, "LevelValue"),                       \
        LOG4CPLUS_TLG_TSTRING (thread, "Thread"),                            \
        LOG4CPLUS_TLG_TSTRING (thread2, "Thread2"),                          \
        LOG4CPLUS_TLG_TSTRING (file, "File"),                                \
        LOG4CPLUS_TLG_TSTRING (function, "Function"),                        \
        TraceLoggingInt32 (line, "Line"),                                    \
        LOG4CPLUS_TLG_TSTRING (ndc, "NDC"),                                  \
        LOG4CPLUS_TLG_TSTRING (mdc_ptr, "MDC"),                              \
        TraceLoggingInt64 (timestamp.seconds, "TimestampUnixSeconds"),       \
        TraceLoggingInt32 (timestamp.microseconds, "TimestampMicroseconds"))


namespace log4cplus
{

namespace
{

enum trace_event_kind
{
    trace_event_fatal,
    trace_event_error,
    trace_event_warn,
    trace_event_info_kind,
    trace_event_debug,
    trace_event_trace
};


enum trace_event_level : UCHAR
{
    trace_level_critical = 1,
    trace_level_error = 2,
    trace_level_warning = 3,
    trace_level_info = 4,
    trace_level_verbose = 5
};


struct trace_event_info
{
    trace_event_kind kind;
    char const * name;
    trace_event_level level;
};


struct trace_timestamp
{
    std::int64_t seconds;
    std::int32_t microseconds;
};


std::mutex provider_mutex;
unsigned provider_ref_count = 0;


trace_event_info
get_trace_event_info (LogLevel ll)
{
    if (ll >= FATAL_LOG_LEVEL)
        return { trace_event_fatal, "Fatal", trace_level_critical };
    else if (ll >= ERROR_LOG_LEVEL)
        return { trace_event_error, "Error", trace_level_error };
    else if (ll >= WARN_LOG_LEVEL)
        return { trace_event_warn, "Warn", trace_level_warning };
    else if (ll >= INFO_LOG_LEVEL)
        return { trace_event_info_kind, "Info", trace_level_info };
    else if (ll >= DEBUG_LOG_LEVEL)
        return { trace_event_debug, "Debug", trace_level_verbose };
    else
        return { trace_event_trace, "Trace", trace_level_verbose };
}


void
append_escaped_mdc_part (tstring & result, tstring const & value)
{
    for (tchar ch : value)
    {
        if (ch == LOG4CPLUS_TEXT ('\\') || ch == LOG4CPLUS_TEXT ('=')
            || ch == LOG4CPLUS_TEXT (';'))
            result += LOG4CPLUS_TEXT ('\\');

        result += ch;
    }
}


tstring
serialize_mdc (MappedDiagnosticContextMap const & mdc)
{
    tstring result;
    bool first = true;

    for (auto const & item : mdc)
    {
        if (! first)
            result += LOG4CPLUS_TEXT (';');

        append_escaped_mdc_part (result, item.first);
        result += LOG4CPLUS_TEXT ('=');
        append_escaped_mdc_part (result, item.second);
        first = false;
    }

    return result;
}


trace_timestamp
make_trace_timestamp (helpers::Time const & timestamp)
{
    return {
        static_cast<std::int64_t> (helpers::to_time_t (timestamp)),
        static_cast<std::int32_t> (helpers::microseconds_part (timestamp))
    };
}


bool
register_trace_logging_provider ()
{
    std::lock_guard<std::mutex> guard (provider_mutex);

    if (provider_ref_count == 0)
    {
        ULONG const status = TraceLoggingRegister (
            g_log4cplus_trace_logging_provider);
        if (status != ERROR_SUCCESS)
        {
            helpers::getLogLog ().error (
                LOG4CPLUS_TEXT ("Win32TraceLoggingAppender: ")
                LOG4CPLUS_TEXT ("TraceLoggingRegister failed with status ")
                + helpers::convertIntegerToString (status));
            return false;
        }
    }

    ++provider_ref_count;
    return true;
}


void
unregister_trace_logging_provider ()
{
    std::lock_guard<std::mutex> guard (provider_mutex);

    if (provider_ref_count == 0)
        return;

    --provider_ref_count;
    if (provider_ref_count == 0)
        TraceLoggingUnregister (g_log4cplus_trace_logging_provider);
}


void
write_trace_logging_event (spi::InternalLoggingEvent const & event)
{
    trace_event_info const event_info = get_trace_event_info (
        event.getLogLevel ());
    trace_timestamp const timestamp = make_trace_timestamp (
        event.getTimestamp ());
    tstring const level_name = getLogLevelManager ().toString (
        event.getLogLevel ());
    tstring const mdc = serialize_mdc (event.getMDCCopy ());

    tchar const * const message = event.getMessage ().c_str ();
    tchar const * const logger = event.getLoggerName ().c_str ();
    tchar const * const level_name_ptr = level_name.c_str ();
    std::int32_t const level_value = static_cast<std::int32_t> (
        event.getLogLevel ());
    tchar const * const thread = event.getThread ().c_str ();
    tchar const * const thread2 = event.getThread2 ().c_str ();
    tchar const * const file = event.getFile ().c_str ();
    tchar const * const function = event.getFunction ().c_str ();
    std::int32_t const line = static_cast<std::int32_t> (event.getLine ());
    tchar const * const ndc = event.getNDC ().c_str ();
    tchar const * const mdc_ptr = mdc.c_str ();

    switch (event_info.kind)
    {
    case trace_event_fatal:
        LOG4CPLUS_TLG_WRITE ("Fatal", trace_level_critical);
        break;

    case trace_event_error:
        LOG4CPLUS_TLG_WRITE ("Error", trace_level_error);
        break;

    case trace_event_warn:
        LOG4CPLUS_TLG_WRITE ("Warn", trace_level_warning);
        break;

    case trace_event_info_kind:
        LOG4CPLUS_TLG_WRITE ("Info", trace_level_info);
        break;

    case trace_event_debug:
        LOG4CPLUS_TLG_WRITE ("Debug", trace_level_verbose);
        break;

    case trace_event_trace:
        LOG4CPLUS_TLG_WRITE ("Trace", trace_level_verbose);
        break;
    }
}


} // namespace


Win32TraceLoggingAppender::Win32TraceLoggingAppender ()
    : provider_registered (false)
{
    init ();
}


Win32TraceLoggingAppender::Win32TraceLoggingAppender (
    helpers::Properties const & properties)
    : Appender (properties)
    , provider_registered (false)
{
    init ();
}


Win32TraceLoggingAppender::~Win32TraceLoggingAppender ()
{
    destructorImpl ();
}


void
Win32TraceLoggingAppender::init ()
{
    provider_registered = register_trace_logging_provider ();
}


void
Win32TraceLoggingAppender::close ()
{
    if (provider_registered)
    {
        unregister_trace_logging_provider ();
        provider_registered = false;
    }

    closed = true;
}


void
Win32TraceLoggingAppender::append (spi::InternalLoggingEvent const & event)
{
    if (! provider_registered)
        return;

    write_trace_logging_event (event);
}


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)

CATCH_TEST_CASE ("Win32TraceLoggingAppender event level mapping",
    "[win32][tracelogging]")
{
    CATCH_CHECK (get_trace_event_info (FATAL_LOG_LEVEL).name
        == std::string ("Fatal"));
    CATCH_CHECK (get_trace_event_info (FATAL_LOG_LEVEL).level
        == trace_level_critical);
    CATCH_CHECK (get_trace_event_info (ERROR_LOG_LEVEL).name
        == std::string ("Error"));
    CATCH_CHECK (get_trace_event_info (ERROR_LOG_LEVEL).level
        == trace_level_error);
    CATCH_CHECK (get_trace_event_info (WARN_LOG_LEVEL).name
        == std::string ("Warn"));
    CATCH_CHECK (get_trace_event_info (WARN_LOG_LEVEL).level
        == trace_level_warning);
    CATCH_CHECK (get_trace_event_info (INFO_LOG_LEVEL).name
        == std::string ("Info"));
    CATCH_CHECK (get_trace_event_info (INFO_LOG_LEVEL).level
        == trace_level_info);
    CATCH_CHECK (get_trace_event_info (DEBUG_LOG_LEVEL).name
        == std::string ("Debug"));
    CATCH_CHECK (get_trace_event_info (DEBUG_LOG_LEVEL).level
        == trace_level_verbose);
    CATCH_CHECK (get_trace_event_info (TRACE_LOG_LEVEL).name
        == std::string ("Trace"));
    CATCH_CHECK (get_trace_event_info (TRACE_LOG_LEVEL).level
        == trace_level_verbose);
}


CATCH_TEST_CASE ("Win32TraceLoggingAppender MDC serialization",
    "[win32][tracelogging]")
{
    MappedDiagnosticContextMap mdc;

    CATCH_CHECK (serialize_mdc (mdc).empty ());

    mdc[LOG4CPLUS_TEXT ("plain")] = LOG4CPLUS_TEXT ("value");
    mdc[LOG4CPLUS_TEXT ("semi;equals=slash\\")]
        = LOG4CPLUS_TEXT ("a;b=c\\d");

    CATCH_CHECK (serialize_mdc (mdc)
        == LOG4CPLUS_TEXT ("plain=value;semi\\;equals\\=slash\\\\=a\\;b\\=c\\\\d"));
}


CATCH_TEST_CASE ("Win32TraceLoggingAppender timestamp conversion",
    "[win32][tracelogging]")
{
    helpers::Time const ts = helpers::time_from_parts (123, 456);
    trace_timestamp const converted = make_trace_timestamp (ts);

    CATCH_CHECK (converted.seconds == 123);
    CATCH_CHECK (converted.microseconds == 456);
}

#endif // defined (LOG4CPLUS_WITH_UNIT_TESTS)


} // namespace log4cplus

#endif // LOG4CPLUS_HAVE_WIN32_TRACELOGGING
