%module log4cplus

%{

#include <log4cplus/logger.h>

%}

namespace log4cplus
{

%rename(assign) Logger::operator =;	
	
class Logger
{
public:
  Logger ();
  Logger (Logger const & other);
  Logger & operator = (Logger const & other);
  virtual ~Logger ();

  void swap (Logger & other);
  Logger getParent () const;

  void assertion (bool assertionVal, log4cplus::tstring const & message) const;
  bool isEnabledFor (LogLevel ll) const;
  void log (LogLevel ll, log4cplus::tstring const & message,
    char const * file = NULL, int line = -1) const;
  void log (spi::InternalLoggingEvent const & event) const;
  void forcedLog (LogLevel ll, log4cplus::tstring const & message,
    char const * file = NULL, int line = -1) const;
  void forcedLog (spi::InternalLoggingEvent const & event) const;
  LogLevel getChainedLogLevel () const;
  LogLevel getLogLevel () const;
  void setLogLevel (LogLevel ll);
  log4cplus::tstring const & getName () const;
  bool getAdditivity () const;
  void setAdditivity (bool additive);

  static bool exists (log4cplus::tstring const & loggerName);
  static Logger getInstance (log4cplus::tstring const & loggerName);
  static Logger getRoot ();  
};

} // namespace Logger

