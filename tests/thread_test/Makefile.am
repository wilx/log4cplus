## Generated by Autogen from Makefile.am.tpl

if MULTI_THREADED
noinst_PROGRAMS += thread_test

thread_test_sources = \
	%D%/main.cxx

thread_test_SOURCES = $(thread_test_sources)

thread_test_CPPFLAGS = $(AM_CPPFLAGS)
thread_test_LDADD = $(liblog4cplus_la_file)
thread_test_LDFLAGS = -no-install

if BUILD_WITH_WCHAR_T_SUPPORT
noinst_PROGRAMS += thread_testU
thread_testU_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE=1 -D_UNICODE=1
thread_testU_SOURCES = $(thread_test_sources)
thread_testU_LDADD = $(liblog4cplusU_la_file)
thread_testU_LDFLAGS = -no-install
endif # BUILD_WITH_WCHAR_T_SUPPORT

endif # MULTI_THREADED
