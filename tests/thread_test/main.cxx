
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/logger.h>
#include <log4cplus/ndc.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/helpers/sleep.h>
#include <log4cplus/streams.h>
#include <exception>
#include <iostream>
#include <string>


using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;
using namespace log4cplus::thread;


#define MILLIS_TO_NANOS 1000
#define NUM_THREADS 4
#define NUM_LOOPS 10

class SlowObject {
public:
    SlowObject() 
        : logger(Logger::getInstance(LOG4CPLUS_TEXT("SlowObject"))) 
    {
        logger.setLogLevel(TRACE_LOG_LEVEL);
    }

    void doSomething()
    {
        LOG4CPLUS_TRACE_METHOD(logger, LOG4CPLUS_TEXT("SlowObject::doSomething()"));
        {
            thread::MutexGuard guard (mutex);
            LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("Actually doing something..."));
            sleep(0, 75 * MILLIS_TO_NANOS);
            LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("Actually doing something...DONE"));
        }
        log4cplus::thread::yield();
    }

    ~SlowObject ()
    { }

private:
    thread::Mutex mutex;
    Logger logger;
};

SlowObject *global;


class TestThread : public AbstractThread {
public:
    TestThread(tstring n) 
        : name(n), logger(Logger::getInstance(LOG4CPLUS_TEXT("test.TestThread"))) 
     {
     }

    virtual void run();

private:
    tstring name;
    Logger logger;
};



struct visitor
{
    template <typename T>
    void operator () (T const * str, std::size_t) const
    {
        std::wcout << str << std::endl;
    }

    template <typename T>
    void operator () (std::basic_string<T> const & str) const
    {
        std::wcout << str.c_str () << std::endl;
    }
};



void
foo (string_param const & s)
{
    std::wcout << "s.get_size(): " << s.get_size () << std::endl;

    s.visit (visitor ());

    string_param copy_of_s (s, log4cplus::helpers::string_param::makecopy);
    copy_of_s.visit (visitor ());

    log4cplus::tstring tstr (s.to_tstring ());
    std::wcout << tstr << std::endl;
}


int
test_stringparam ()
{
    char array[] = "test";
    char const carray[] = "test";
    char * pchar = &array[0];
    char const * pcchar = &array[0];
    char const * const cpcchar = &array[0];
    std::string str ("test");

    wchar_t warray[] = L"test";
    wchar_t const cwarray[] = L"test";
    wchar_t * pwchar = &warray[0];
    wchar_t const * pcwchar = &warray[0];
    wchar_t const * const cpcwchar = &warray[0];
    std::wstring wstr (L"test");

#if defined (LOG4CPLUS_HAVE_CPP0X)
    char16_t u16array[] = { 't', 'e', 's', 't', '\0' };
    char16_t const cu16array[] = { 't', 'e', 's', 't', '\0' };
    char16_t * pu16char = &u16array[0];
    char16_t const * pcu16char = &u16array[0];
    char16_t const * const cpcu16char = &u16array[0];
    std::u16string u16str;

    char32_t u32array[] = { 't', 'e', 's', 't', '\0' };
    char32_t const cu32array[] = { 't', 'e', 's', 't', '\0' };
    char32_t * pu32char = &u32array[0];
    char32_t const * pcu32char = &u32array[0];
    char32_t const * const cpcu32char = &u32array[0];
    std::u32string u32str;
#endif

    foo ("test");
    foo (array);
    foo (carray);
    foo (str);
    foo (str.c_str ());
    foo (pchar);
    foo (pcchar);
    foo (cpcchar);

    foo (L"test");
    foo (warray);
    foo (cwarray);
    foo (wstr);
    foo (wstr.c_str ());
    foo (pwchar);
    foo (pcwchar);
    foo (cpcwchar);

#if defined (LOG4CPLUS_HAVE_CPP0X)
    //foo (u"test");
    foo (u16array);
    foo (cu16array);
    foo (u16str);
    foo (u16str.c_str ());
    foo (pu16char);
    foo (pcu16char);
    foo (cpcu16char);

    //foo (U"test");
    foo (u32array);
    foo (cu32array);
    foo (u32str);
    foo (u32str.c_str ());
    foo (pu32char);
    foo (pcu32char);
    foo (cpcu32char);
#endif

    int dummy = 0;

    return 0;
}


int
main() 
{
    auto_ptr<SlowObject> globalContainer(new SlowObject());
    global = globalContainer.get();

    try {
        log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("main"));
        Logger::getRoot().setLogLevel(INFO_LOG_LEVEL);
        LogLevel ll = logger.getLogLevel();
        tcout << "main Priority: " << getLogLevelManager().toString(ll) << endl;

        helpers::SharedObjectPtr<Appender> append_1(new ConsoleAppender());
        append_1->setLayout( std::auto_ptr<Layout>(new log4cplus::TTCCLayout()) );
        Logger::getRoot().addAppender(append_1);
        append_1->setName(LOG4CPLUS_TEXT("cout"));

        test_stringparam ();

	    append_1 = 0;

        log4cplus::helpers::SharedObjectPtr<TestThread> threads[NUM_THREADS];
        int i = 0;
        for(i=0; i<NUM_THREADS; ++i) {
            tostringstream s;
            s << "Thread-" << i;
            threads[i] = new TestThread(s.str());
        }

        for(i=0; i<NUM_THREADS; ++i) {
            threads[i]->start();
        }
        LOG4CPLUS_DEBUG(logger, "All Threads started...");

        for(i=0; i<NUM_THREADS; ++i) {
            while(threads[i]->isRunning()) {
                sleep(0, 200 * MILLIS_TO_NANOS);
            }
        }
        LOG4CPLUS_INFO(logger, "Exiting main()...");
    }
    catch(std::exception &e) {
        LOG4CPLUS_FATAL(Logger::getRoot(), "main()- Exception occured: " << e.what());
    }
    catch(...) {
        LOG4CPLUS_FATAL(Logger::getRoot(), "main()- Exception occured");
    }

    log4cplus::Logger::shutdown();
    return 0;
}


void
TestThread::run()
{
    try {
        LOG4CPLUS_WARN(logger, name + LOG4CPLUS_TEXT(" TestThread.run()- Starting..."));
        NDC& ndc = getNDC();
        NDCContextCreator _first_ndc(name);
        LOG4CPLUS_DEBUG(logger, "Entering Run()...");
        for(int i=0; i<NUM_LOOPS; ++i) {
            NDCContextCreator _ndc(LOG4CPLUS_TEXT("loop"));
            global->doSomething();
        }
        LOG4CPLUS_DEBUG(logger, "Exiting run()...");

        ndc.remove();
    }
    catch(exception& e) {
        LOG4CPLUS_FATAL(logger, "TestThread.run()- Exception occurred: " << e.what());
    }
    catch(...) {
        LOG4CPLUS_FATAL(logger, "TestThread.run()- Exception occurred!!");
    }
    LOG4CPLUS_WARN(logger, name << " TestThread.run()- Finished");
} // end "run"

