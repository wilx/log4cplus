//   Copyright (C) 2012, Vaclav Zeman. All rights reserved.
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

#include <log4cplus/config.hxx>
#include <log4cplus/config/windowsh-inc.h>
#include <log4cplus/configurator.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/asyncappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/nteventlogappender.h>
#include <log4cplus/socketappender.h>
#include <log4cplus/syslogappender.h>
#include <log4cplus/win32debugappender.h>
#include <log4cplus/win32consoleappender.h>


namespace log4cplus
{

void
initializeLog4cplusAppenders()
{
    static bool initialized = false;
    if (initialized)
        return;

	initializeLog4cplus ();

	spi::AppenderFactoryRegistry & reg = spi::getAppenderFactoryRegistry ();

    LOG4CPLUS_REG_APPENDER (reg, ConsoleAppender);
    LOG4CPLUS_REG_APPENDER (reg, FileAppender);
    LOG4CPLUS_REG_APPENDER (reg, RollingFileAppender);
    LOG4CPLUS_REG_APPENDER (reg, DailyRollingFileAppender);
    LOG4CPLUS_REG_APPENDER (reg, SocketAppender);
#if defined(_WIN32)
#  if defined(LOG4CPLUS_HAVE_NT_EVENT_LOG)
    LOG4CPLUS_REG_APPENDER (reg, NTEventLogAppender);
#  endif
#  if defined(LOG4CPLUS_HAVE_WIN32_CONSOLE)
    LOG4CPLUS_REG_APPENDER (reg, Win32ConsoleAppender);
#  endif
    LOG4CPLUS_REG_APPENDER (reg, Win32DebugAppender);
#elif defined(LOG4CPLUS_HAVE_SYSLOG_H)
    LOG4CPLUS_REG_APPENDER (reg, SysLogAppender);
#endif
#ifndef LOG4CPLUS_SINGLE_THREADED
    LOG4CPLUS_REG_APPENDER (reg, AsyncAppender);
#endif
}

} // namespace log4cplus


#if defined (_WIN32) && defined (LOG4CPLUS_BUILD_DLL)

BOOL
WINAPI
DllMain (LOG4CPLUS_DLLMAIN_HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason) 
    { 
    case DLL_PROCESS_ATTACH:
    {
		log4cplus::initializeLog4cplusAppenders ();
        break;
    }

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
 
#else

namespace {

    struct _static_log4cplus_initializer
    {
        _static_log4cplus_initializer ()
        {
			log4cplus::initializeLog4cplusAppenders ();
        }
    } static initializer;
}


#endif
