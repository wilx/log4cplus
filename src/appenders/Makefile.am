lib_LTLIBRARIES = liblog4cplus_appenders.la

AM_CPPFLAGS = -DINSIDE_LOG4CPLUS_APPENDERS \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	@LOG4CPLUS_NDEBUG@

AM_CXXFLAGS=-no-suppress @LOG4CPLUS_PROFILING_CXXFLAGS@

AM_LDFLAGS=@LOG4CPLUS_PROFILING_LDFLAGS@

include $(top_srcdir)/src/Makefile.am.includes

APPENDERS_INC = \
	$(INCLUDES_SRC) \
	$(INCLUDES_SRC_PATH)/appendersexports.h \
	$(INCLUDES_SRC_PATH)/asyncappender.h \
	$(INCLUDES_SRC_PATH)/consoleappender.h \
	$(INCLUDES_SRC_PATH)/fileappender.h \
	$(INCLUDES_SRC_PATH)/nteventlogappender.h \
	$(INCLUDES_SRC_PATH)/socketappender.h \
	$(INCLUDES_SRC_PATH)/win32consoleappender.h \
	$(INCLUDES_SRC_PATH)/win32debugappender.h \
	$(top_builddir)/include/log4cplus/config/defines.hxx

APPENDERS_SRC = \
	$(APPENDERS_INC) \
	asyncappender.cxx \
	consoleappender.cxx \
	fileappender.cxx \
	init.cxx \
	nteventlogappender.cxx \
	socketappender.cxx \
	syslogappender.cxx \
	win32consoleappender.cxx \
	win32debugappender.cxx

liblog4cplus_appenders_la_SOURCES = $(APPENDERS_SRC)
liblog4cplus_appenders_la_LIBADD = $(top_builddir)/src/core/liblog4cplus.la 
liblog4cplus_appenders_la_LDFLAGS = -no-undefined -version-info @LT_VERSION@ \
	-release 1.1
