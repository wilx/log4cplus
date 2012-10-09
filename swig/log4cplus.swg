%module log4cplus

%include "std/std_string.i"
//%include "std/std_wstring.i"
#ifdef SWIGPYTHON
%include "python/std_basic_string.i"
#endif

%{
#include "log4cplus/tchar.h"
#include "log4cplus/tstring.h"
%}

namespace log4cplus
{

#ifdef UNICODE
typedef wchar_t tchar;
#else
typedef char tchar;
#endif

typedef std::basic_string<tchar> tstring;

typedef int LogLevel;

} // namespace log4cplus

%include "loggingevent.swg"
%include "logger.swg"

