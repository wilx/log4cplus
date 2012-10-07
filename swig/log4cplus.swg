%module log4cplus

%include "std_string.i"

namespace log4cplus
{

typedef int LogLevel;

} // namespace log4cplus

%include "loggingevent.swg"
%include "logger.swg"

