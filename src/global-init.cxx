// Module:  Log4CPLUS
// File:    global-init.cxx
// Created: 5/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2013 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/config.hxx>
#include <log4cplus/config/windowsh-inc.h>
#include <log4cplus/logger.h>
#include <log4cplus/ndc.h>
#include <log4cplus/mdc.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/internal/internal.h>
#include <log4cplus/thread/impl/tls.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/helpers/forkhandler.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/hierarchy.h>
#include <log4cplus/hierarchylocker.h>
#include <log4cplus/consoleappender.h>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
// Forward Declarations
namespace log4cplus
{

#ifdef UNICODE
LOG4CPLUS_EXPORT tostream & tcout = std::wcout;
LOG4CPLUS_EXPORT tostream & tcerr = std::wcerr;

#else
LOG4CPLUS_EXPORT tostream & tcout = std::cout;
LOG4CPLUS_EXPORT tostream & tcerr = std::cerr;

#endif // UNICODE


namespace
{


//! Default context.
struct DefaultContext
{
    log4cplus::thread::Mutex console_mutex;
    helpers::LogLog loglog;
    LogLevelManager log_level_manager;
    helpers::Time TTCCLayout_time_base;
    NDC ndc;
    MDC mdc;
    Hierarchy hierarchy;
    spi::AppenderFactoryRegistry appender_factory_registry;
    spi::LayoutFactoryRegistry layout_factory_registry;
    spi::FilterFactoryRegistry filter_factory_registry;
    spi::LocaleFactoryRegistry locale_factory_registry;
};


enum DCState
{
    DC_UNINITIALIZED,
    DC_INITIALIZED,
    DC_DESTROYED
};


static DCState default_context_state;
static DefaultContext * default_context;


struct destroy_default_context
{
    ~destroy_default_context ()
    {
        delete default_context;
        default_context = 0;
        default_context_state = DC_DESTROYED;
    }
} static destroy_default_context_;


static
void
alloc_dc ()
{
    assert (! default_context);
    assert (default_context_state == DC_UNINITIALIZED);

    if (default_context)
        throw std::logic_error (
            "alloc_dc() called with non-NULL default_context.");

    if (default_context_state == DC_INITIALIZED)
        throw std::logic_error ("alloc_dc() called in DC_INITIALIZED state.");

    default_context = new DefaultContext;

    if (default_context_state == DC_DESTROYED)
        default_context->loglog.error (
            LOG4CPLUS_TEXT ("Re-initializing default context after it has")
            LOG4CPLUS_TEXT (" already been destroyed.\n")
            LOG4CPLUS_TEXT ("The memory will be leaked."));

    default_context_state = DC_INITIALIZED;
}


static
DefaultContext *
get_dc (bool alloc = true)
{
    if (LOG4CPLUS_UNLIKELY (! default_context && alloc))
        alloc_dc ();
    return default_context;
}


} // namespace


namespace helpers
{


log4cplus::thread::Mutex const &
getConsoleOutputMutex ()
{
    return get_dc ()->console_mutex;
}


LogLog &
getLogLog ()
{
    return get_dc ()->loglog;
}


} // namespace helpers


helpers::Time const &
getTTCCLayoutTimeBase ()
{
    return get_dc ()->TTCCLayout_time_base;
}


LogLevelManager &
getLogLevelManager ()
{
    return get_dc ()->log_level_manager;
}


Hierarchy &
getDefaultHierarchy ()
{
    return get_dc ()->hierarchy;
}


NDC &
getNDC ()
{
    return get_dc ()->ndc;
}


MDC &
getMDC ()
{
    return get_dc ()->mdc;
}


namespace spi
{


AppenderFactoryRegistry &
getAppenderFactoryRegistry ()
{
    return get_dc ()->appender_factory_registry;
}


LayoutFactoryRegistry &
getLayoutFactoryRegistry ()
{
    return get_dc ()->layout_factory_registry;
}


FilterFactoryRegistry &
getFilterFactoryRegistry ()
{
    return get_dc ()->filter_factory_registry;
}


LocaleFactoryRegistry &
getLocaleFactoryRegistry()
{
    return get_dc ()->locale_factory_registry;
}


} // namespace spi


namespace internal
{


gft_scratch_pad::gft_scratch_pad ()
    : uc_q_str_valid (false)
    , q_str_valid (false)
    , s_str_valid (false)
{ }


gft_scratch_pad::~gft_scratch_pad ()
{ }


appender_sratch_pad::appender_sratch_pad ()
{ }


appender_sratch_pad::~appender_sratch_pad ()
{ }


per_thread_data::per_thread_data ()
    : fnull (0)
{ }


per_thread_data::~per_thread_data ()
{
    if (fnull)
        std::fclose (fnull);
}


log4cplus::thread::impl::tls_key_type tls_storage_key;


#if ! defined (LOG4CPLUS_SINGLE_THREADED) \
    && defined (LOG4CPLUS_THREAD_LOCAL_VAR)

LOG4CPLUS_THREAD_LOCAL_VAR per_thread_data * ptd = 0;


per_thread_data *
alloc_ptd ()
{
    per_thread_data * tmp = new per_thread_data;
    set_ptd (tmp);
    // This is a special hack. We set the keys' value to non-NULL to
    // get the ptd_cleanup_func to execute when this thread ends. The
    // cast is safe; the associated value will never be used if read
    // again using the key.
    thread::impl::tls_set_value (tls_storage_key,
        reinterpret_cast<void *>(1));

    return tmp;
}

#  else

per_thread_data *
alloc_ptd ()
{
    per_thread_data * tmp = new per_thread_data;
    set_ptd (tmp);
    return tmp;
}

#  endif


} // namespace internal


void initializeFactoryRegistry();


//! Thread local storage clean up function for POSIX threads.
static
void
ptd_cleanup_func (void * arg)
{
    internal::per_thread_data * const arg_ptd
        = static_cast<internal::per_thread_data *>(arg);
    internal::per_thread_data * const ptd = internal::get_ptd (false);
    (void) ptd;

    // Either it is a dummy value or it should be the per thread data
    // pointer we get from internal::get_ptd().
    assert (arg == reinterpret_cast<void *>(1)
        || arg_ptd == ptd
        || (! ptd && arg_ptd));

    if (arg == reinterpret_cast<void *>(1))
        // Setting the value through the key here is necessary in case
        // we are using TLS using __thread or __declspec(thread) or
        // similar constructs with POSIX threads.  Otherwise POSIX
        // calls this cleanup routine more than once if the value
        // stays non-NULL after it returns.
        thread::impl::tls_set_value (internal::tls_storage_key, 0);
    else if (arg)
    {
        // Instead of using internal::get_ptd(false) here we are using
        // the value passed to this function directly.  This is
        // necessary because of the following (from SUSv4):
        //
        // A call to pthread_getspecific() for the thread-specific
        // data key being destroyed shall return the value NULL,
        // unless the value is changed (after the destructor starts)
        // by a call to pthread_setspecific().
        delete arg_ptd;
        thread::impl::tls_set_value (internal::tls_storage_key, 0);
    }
    else
    {
        // In this case we fall through to threadCleanup() and it does
        // all the necessary work itself.
        ;
    }

    threadCleanup ();
}


static
void
threadSetup ()
{
    internal::get_ptd (true);
}


template <typename Class>
static
void
for_all_appenders_call (LoggerList const & logger_list,
    void (Class:: * fun_ptr) ())
{
    for (LoggerList::const_iterator it = logger_list.begin ();
         it != logger_list.end(); ++it)
    {
        spi::AppenderAttachable const & aa = *it;
        SharedAppenderPtrList appender_list (aa.getAllAppenders ());
        for (SharedAppenderPtrList::iterator appender_it
                 = appender_list.begin();
             appender_it != appender_list.end (); ++appender_it)
        {
            Appender * appender = &**appender_it;
            helpers::getLogLog ().debug (
                LOG4CPLUS_TEXT("testing ") + appender->getName ());

            Class * class_ptr;
            helpers::getLogLog ().debug (
                LOG4CPLUS_C_STR_TO_TSTRING (typeid (appender).name ())
                );
            if ((class_ptr = dynamic_cast<Class *>(appender)))
            {
                helpers::getLogLog ().debug (
                    LOG4CPLUS_TEXT("calling (class_ptr->*fun_ptr) ()"));
                log4cplus::thread::MutexGuard appender_guard(
                    appender->access_mutex);
                (class_ptr->*fun_ptr) ();
            }
        }
    }
}



static
void
at_exit ()
{
    DefaultContext * const ctx = get_dc ();

    ctx->loglog.debug (LOG4CPLUS_TEXT ("preparing exit"));

    // Lock loggers hierarchy.

    Hierarchy & hierarchy = ctx->hierarchy;
    std::auto_ptr<HierarchyLocker> hl (new HierarchyLocker (hierarchy));

    // Tell all appenders to flush.

    LoggerList logger_list = hierarchy.getCurrentLoggers ();
    logger_list.insert (logger_list.end (), hierarchy.getRoot ());
    for_all_appenders_call<Appender> (logger_list,
        &Appender::flush);
}


#if defined (LOG4CPLUS_WITH_ATFORK_HANDLERS)
static
void
prepare_fork ()
{
    DefaultContext * const ctx = get_dc ();

    ctx->loglog.debug (LOG4CPLUS_TEXT ("preparing fork"));

    // Lock loggers hierarchy.

    Hierarchy & hierarchy = ctx->hierarchy;
    std::auto_ptr<HierarchyLocker> hl (new HierarchyLocker (hierarchy));

    // Notify all appenders about impending fork.

    LoggerList logger_list = hierarchy.getCurrentLoggers ();
    logger_list.insert (logger_list.end (), hierarchy.getRoot ());
    for_all_appenders_call<helpers::ForkHandler> (logger_list,
        &helpers::ForkHandler::prepare_fork);

    // Flush standard streams.

    tcerr.flush ();
    tcout.flush ();

    std::fflush (0);

    ctx->loglog.debug (LOG4CPLUS_TEXT ("done preparing fork"));
}


static
void
after_fork_parent ()
{
    helpers::getLogLog ().debug (LOG4CPLUS_TEXT ("after fork in parent"));
}


static
void
after_fork_child ()
{
    get_dc ()->TTCCLayout_time_base = helpers::Time::gettimeofday();
    helpers::getLogLog ().debug (LOG4CPLUS_TEXT ("after fork in child"));
}

#endif


void
initializeLog4cplus()
{
    static bool initialized = false;
    if (initialized)
        return;

    internal::tls_storage_key = thread::impl::tls_init (ptd_cleanup_func);
    threadSetup ();

    DefaultContext * dc = get_dc (true);
    dc->TTCCLayout_time_base = helpers::Time::gettimeofday ();
    Logger::getRoot();
    initializeFactoryRegistry();

#if defined (LOG4CPLUS_WITH_ATFORK_HANDLERS)
    int ret
        = pthread_atfork (prepare_fork, after_fork_parent, after_fork_child);
    (void)ret;
#endif

    std::atexit (at_exit);

    initialized = true;
}


void
initialize ()
{
    initializeLog4cplus ();
}


void
threadCleanup ()
{
    // Do thread-specific cleanup.
    internal::per_thread_data * ptd = internal::get_ptd (false);
    delete ptd;
    internal::set_ptd (0);
}


#if defined (_WIN32)
static
VOID CALLBACK
initializeLog4cplusApcProc (ULONG_PTR /*dwParam*/)
{
    initializeLog4cplus ();
    threadSetup ();
}


static
void
queueLog4cplusInitializationThroughAPC ()
{
    if (! QueueUserAPC (initializeLog4cplusApcProc, GetCurrentThread (),
        0))
        throw std::runtime_error ("QueueUserAPC() has failed");
}


static
void NTAPI
thread_callback (LPVOID /*hinstDLL*/, DWORD fdwReason, LPVOID /*lpReserved*/)
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        // We cannot initialize log4cplus directly here. This is because
        // DllMain() is called under loader lock. When we are using C++11
        // threads and synchronization primitives then there is a deadlock
        // somewhere in internals of std::mutex::lock().
        queueLog4cplusInitializationThroughAPC ();
        break;
    }

    case DLL_THREAD_ATTACH:
    {
        // We could call threadSetup() here but that imposes overhead
        // on threads that do not use log4cplus. Thread local data will
        // be initialized lazily instead.
        break;
    }

    case DLL_THREAD_DETACH:
    {
        // Do thread-specific cleanup.
        log4cplus::threadCleanup ();

        break;
    }

    case DLL_PROCESS_DETACH:
    {
        // Perform any necessary cleanup.

        // Do thread-specific cleanup.
        log4cplus::threadCleanup ();
#if ! defined (LOG4CPLUS_THREAD_LOCAL_VAR)
        log4cplus::thread::impl::tls_cleanup (
            log4cplus::internal::tls_storage_key);
#endif
        break;
    }

    } // switch
}

#endif


} // namespace log4cplus


#if defined (_WIN32) && defined (LOG4CPLUS_BUILD_DLL) && defined (_DLL)
extern "C"
BOOL
WINAPI
DllMain (LOG4CPLUS_DLLMAIN_HINSTANCE hinstDLL, DWORD fdwReason,
    LPVOID lpReserved)
{
    log4cplus::thread_callback (hinstDLL, fdwReason, lpReserved);

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#elif defined (_WIN32) \
    && defined (_MSC_VER) && _MSC_VER >= 1400 && defined (_DLL)
extern "C"
{

// This magic has been pieced together from several sources:
// - <http://www.nynaeve.net/?p=183>
// - <http://lists.cs.uiuc.edu/pipermail/cfe-dev/2011-November/018818.html>

#pragma data_seg (push, old_seg)
#ifdef _WIN64
#pragma const_seg (".CRT$XLX")
extern const
#else
#pragma data_seg (".CRT$XLX")
#endif
PIMAGE_TLS_CALLBACK log4cplus_p_thread_callback = log4cplus::thread_callback;
#pragma data_seg (pop, old_seg)
#ifdef _WIN64
#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:log4cplus_p_thread_callback")
#else
#pragma comment (linker, "/INCLUDE:__tls_used")
#pragma comment (linker, "/INCLUDE:_log4cplus_p_thread_callback")
#endif

} // extern "C"

#elif defined (_WIN32)
namespace {

struct _static_log4cplus_initializer
{
    _static_log4cplus_initializer ()
    {
        // It is not possible to reliably call initializeLog4cplus() here
        // when we are using Visual Studio and C++11 threads
        // and synchronization primitives. It would result into a deadlock
        // on loader lock.
#if ! (defined (LOG4CPLUS_WITH_CXX11_THREADS) && defined (_MSC_VER))
        log4cplus::initializeLog4cplus ();
#endif
    }

    ~_static_log4cplus_initializer ()
    {
        // Last thread cleanup.
        log4cplus::threadCleanup ();
#if ! defined (LOG4CPLUS_THREAD_LOCAL_VAR)
    log4cplus::thread::impl::tls_cleanup (
        log4cplus::internal::tls_storage_key);
#endif
    }
} static initializer;

} // namespace

#else
namespace {

struct _static_log4cplus_initializer
{
    _static_log4cplus_initializer ()
    {
        log4cplus::initializeLog4cplus();
    }

    ~_static_log4cplus_initializer ()
    {
        // Last thread cleanup.
        log4cplus::threadCleanup ();

        log4cplus::thread::impl::tls_cleanup (
            log4cplus::internal::tls_storage_key);
    }
} static initializer;

} // namespace

#endif
