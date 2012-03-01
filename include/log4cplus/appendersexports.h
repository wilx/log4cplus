#ifndef LOG4CPLUS_APPENDERSEXPORTS_H
#define LOG4CPLUS_APPENDERSEXPORTS_H

#include "log4cplus/config.hxx"

#if defined(_WIN32)
	// log4cplus_EXPORTS is used by the CMake build system.  DLL_EXPORT is
	// used by the autotools build system.

	#if defined (LOG4CPLUS_BUILD_DLL)
		#if defined (INSIDE_LOG4CPLUS_APPENDERS)
			#define LOG4CPLUS_APPENDERS_EXPORT __declspec(dllexport)
		#else
			#define LOG4CPLUS_APPENDERS_EXPORT __declspec(dllimport)
		#endif
	#else
		#define LOG4CPLUS_APPENDERS_EXPORT
	#endif

#else
  #if defined (INSIDE_LOG4CPLUS_APPENDERS)
    #define LOG4CPLUS_APPENDERS_EXPORT LOG4CPLUS_DECLSPEC_EXPORT
  #else
    #define LOG4CPLUS_APPENDERS_EXPORT LOG4CPLUS_DECLSPEC_IMPORT
  #endif // defined (INSIDE_LOG4CPLUS_APPENDERS)

#endif // !_WIN32

#endif // LOG4CPLUS_APPENDERSEXPORTS_H
