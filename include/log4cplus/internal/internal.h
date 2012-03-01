// Module:  Log4CPLUS
// File:    internal.h
// Created: 1/2009
// Author:  Vaclav Haisman
//
//
//   Copyright (C) 2009-2010, Vaclav Haisman. All rights reserved.
//   
//   Redistribution and use in source and binary forms, with or without modifica-
//   tion, are permitted provided that the following conditions are met:
//   
//   1. Redistributions of  source code must  retain the above copyright  notice,
//      this list of conditions and the following disclaimer.
//   
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//   
//   THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//   FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//   APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//   INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//   DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//   OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//   ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//   (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/** @file This header contains declaration internal to log4cplus
 * and/or its appenders.
 */


#ifndef LOG4CPLUS_INTERNAL_INTERNAL_HEADER_
#define LOG4CPLUS_INTERNAL_INTERNAL_HEADER_

#include <memory>
#include <vector>
#include <sstream>
#include <cstdio>
#include <log4cplus/config.hxx>
#include <log4cplus/tstring.h>
#include <log4cplus/streams.h>
#include <log4cplus/ndc.h>
#include <log4cplus/mdc.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/thread/impl/tls.h>
#include <log4cplus/helpers/snprintf.h>


namespace log4cplus {

namespace internal {


//! Canonical empty string. It is used when the need to return empty string
//! by reference arises.
extern LOG4CPLUS_EXPORT log4cplus::tstring const empty_str;


struct LOG4CPLUS_EXPORT gft_scratch_pad
{
    gft_scratch_pad ();
    ~gft_scratch_pad ();
    
    void
    reset ()
    {
        uc_q_str_valid = false;
        q_str_valid = false;
        s_str_valid = false;
        ret.clear ();
    }
    
    log4cplus::tstring q_str;
    log4cplus::tstring uc_q_str;
    log4cplus::tstring s_str;
    log4cplus::tstring ret;
    log4cplus::tstring fmt;
    log4cplus::tstring tmp;
    std::vector<tchar> buffer;
    bool uc_q_str_valid;
    bool q_str_valid;
    bool s_str_valid;
};


struct LOG4CPLUS_EXPORT appender_sratch_pad
{
    appender_sratch_pad ();
    ~appender_sratch_pad ();

    tostringstream oss;
    tstring str;
};


//! Per thread data.
struct LOG4CPLUS_EXPORT per_thread_data
{
    per_thread_data ();
    ~per_thread_data ();

    tostringstream macros_oss;
    DiagnosticContextStack ndc_dcs;
    MappedDiagnosticContextMap mdc_map;
    log4cplus::tstring thread_name;
    log4cplus::tstring thread_name2;
    gft_scratch_pad gft_sp;
    appender_sratch_pad appender_sp;
    log4cplus::tstring faa_str;
    spi::InternalLoggingEvent forced_log_ev;
    std::FILE * fnull;
    log4cplus::helpers::snprintf_buf snprintf_buf;
};


#if ! defined (LOG4CPLUS_SINGLE_THREADED) \
    && defined (LOG4CPLUS_THREAD_LOCAL_VAR)

//! The default value of the \param alloc is false for Win32 DLL builds
//! since per thread data are already initialized by DllMain().
LOG4CPLUS_EXPORT
per_thread_data *
get_ptd (bool alloc
#if defined (_WIN32) && defined (LOG4CPLUS_BUILD_DLL)
         = false
#else
         = true
#endif
    );

#else // defined (LOG4CPLUS_THREAD_LOCAL_VAR)

LOG4CPLUS_EXPORT per_thread_data * get_ptd (bool alloc = true);

#endif // defined (LOG4CPLUS_THREAD_LOCAL_VAR)


inline
tstring &
get_thread_name_str ()
{
    return get_ptd ()->thread_name;
}


inline
tstring &
get_thread_name2_str ()
{
    return get_ptd ()->thread_name2;
}


inline
gft_scratch_pad &
get_gft_scratch_pad ()
{
    return get_ptd ()->gft_sp;
}


inline
appender_sratch_pad &
get_appender_sp ()
{
    return get_ptd ()->appender_sp;
}


} // namespace internal {


namespace detail
{

LOG4CPLUS_EXPORT void clear_tostringstream (tostringstream &);

} // namespace detail


} // namespace log4cplus { 


#endif // LOG4CPLUS_INTERNAL_INTERNAL_HEADER_
