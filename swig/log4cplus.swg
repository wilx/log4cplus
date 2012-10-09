%module log4cplus

%include "std_basic_string.i"

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

