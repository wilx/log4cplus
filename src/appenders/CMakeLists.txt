set (log4cplus_appenders_sources
  asyncappender.cxx
  consoleappender.cxx
  fileappender.cxx
  socketappender.cxx
  syslogappender.cxx)

#message (STATUS "Type: ${UNIX}|${CYGWIN}|${WIN32}")

# Uncomment the following line to get locale aware string conversions.
#add_definitions (-DLOG4CPLUS_WORKING_LOCALE)

if ("${UNIX}" OR "${CYGWIN}")
  set (log4cplus_appenders_sources ${log4cplus_appenders_sources})
elseif (WIN32)
  set (log4cplus_appenders_sources ${log4cplus_appenders_sources}
    nteventlogappender.cxx
    win32consoleappender.cxx
    win32debugappender.cxx)

  # Comment out the following lines if you want non-Unicode version.
  add_definitions (-DUNICODE -D_UNICODE -UMBCS -U_MBCS)
  set (log4cplus_postfix "${log4cplus_postfix}U")

  #add_definitions (-DLOG4CPLUS_STATIC)
  #set (log4cplus_postfix "${log4cplus_postfix}S")
endif ()


add_library (log4cplus_appenders ${log4cplus_appenders_sources})
target_link_libraries (log4cplus_appenders log4cplus ${CMAKE_THREAD_LIBS_INIT})

set_target_properties (log4cplus_appenders PROPERTIES
  VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}"
  SOVERSION "${log4cplus_soversion}"
  COMPILE_FLAGS "-DINSIDE_LOG4CPLUS")

if (WIN32)
  set_target_properties (log4cplus_appenders PROPERTIES
    DEBUG_POSTFIX "${log4cplus_postfix}D"
    RELEASE_POSTFIX "${log4cplus_postfix}"
    MINSIZEREL_POSTFIX "${log4cplus_postfix}"
    RELWITHDEBINFO_POSTFIX "${log4cplus_postfix}")
  target_link_libraries (log4cplus_appenders ws2_32 advapi32)
endif ()

install(TARGETS log4cplus_appenders
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

install(
  FILES
    ../include/log4cplus/appendersexports.h
    ../include/log4cplus/asyncappender.h
    ../include/log4cplus/consoleappender.h
    ../include/log4cplus/fileappender.h
    ../include/log4cplus/nteventlogappender.h
    ../include/log4cplus/socketappender.h
    ../include/log4cplus/syslogappender.h
    ../include/log4cplus/win32debugappender.h
    ../include/log4cplus/win32consoleappender.h
  DESTINATION
    include/log4cplus)

