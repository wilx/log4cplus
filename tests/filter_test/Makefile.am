## Generated by Autogen from Makefile.am.tpl

noinst_PROGRAMS += filter_test

filter_test_sources = \
	%D%/main.cxx

filter_test_SOURCES = $(filter_test_sources)

filter_test_CPPFLAGS = $(AM_CPPFLAGS)
filter_test_LDADD = $(liblog4cplus_la_file)
filter_test_LDFLAGS = -no-install

if BUILD_WITH_WCHAR_T_SUPPORT
noinst_PROGRAMS += filter_testU
filter_testU_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE=1 -D_UNICODE=1
filter_testU_SOURCES = $(filter_test_sources)
filter_testU_LDADD = $(liblog4cplusU_la_file)
filter_testU_LDFLAGS = -no-install
endif # BUILD_WITH_WCHAR_T_SUPPORT
