lib_LTLIBRARIES = liblog4cplus.la

AM_CPPFLAGS = -DINSIDE_LOG4CPLUS \
		-I$(top_srcdir)/include -I$(top_builddir)/include \
		@LOG4CPLUS_NDEBUG@

AM_CXXFLAGS=-no-suppress @LOG4CPLUS_PROFILING_CXXFLAGS@

AM_LDFLAGS=@LOG4CPLUS_PROFILING_LDFLAGS@

include $(top_srcdir)/src/Makefile.am.includes

SINGLE_THREADED_SRC = \
	$(INCLUDES_SRC) \
	appenderattachableimpl.cxx \
	appender.cxx \
	clogger.cxx \
	configurator.cxx \
	cygwin-win32.cxx \
	env.cxx \
	factory.cxx \
	filter.cxx \
	global-init.cxx \
	hierarchy.cxx \
	hierarchylocker.cxx \
	layout.cxx \
	logger.cxx \
	loggerimpl.cxx \
	loggingevent.cxx \
	loggingmacros.cxx \
	loglevel.cxx \
	loglog.cxx \
	logloguser.cxx \
	mdc.cxx \
	ndc.cxx \
	nullappender.cxx \
	objectregistry.cxx \
	patternlayout.cxx \
	pointer.cxx \
	property.cxx \
	queue.cxx \
	rootlogger.cxx \
	sleep.cxx \
	snprintf.cxx \
	socketbuffer.cxx \
	socket.cxx \
	socket-unix.cxx \
	socket-win32.cxx \
	stringhelper.cxx \
	stringhelper-clocale.cxx \
	stringhelper-cxxlocale.cxx \
	stringhelper-iconv.cxx \
	syncprims.cxx \
	threads.cxx \
	timehelper.cxx \
	tls.cxx \
	version.cxx

LIB_SRC = $(SINGLE_THREADED_SRC)

liblog4cplus_la_SOURCES = $(LIB_SRC)
liblog4cplus_la_LDFLAGS = -no-undefined -version-info @LT_VERSION@ -release 1.1
