[= AutoGen5 template -*- Mode: scheme -*-
am
=][=
(use-modules (ice-9 ftw))
=]## Generated by Autogen from [= (tpl-file) =]

TESTSUITE = %D%/testsuite
AUTOTEST = $(AUTOM4TE) --language=Autotest
TESTSUITE_AT = \
[=
(let ((files (list)))
  (define (emit-at-files-ftw-cb filename statinfo flag)
    (begin
      (if (and (eq? flag 'regular)
               (string-suffix-ci? ".at" filename))
          (set! files (append! files (list filename))))
      #t))
  (begin
    (ftw "." emit-at-files-ftw-cb)
    ;; Add the generated header as it will not be found by file search.
    (set! files (sort! files string-ci<?))
    (emit "\t"
          (string-join (map (lambda (x)
                              (string-append "%D%/"
                                             (if (string-prefix? "./" x)
                                                 (substring x 2)
                                                 x)))
                            files)
                       " \\\n\t")
          "\n")))
=]

all: $(TESTSUITE)

$(TESTSUITE): $(TESTSUITE_AT) %D%/atlocal.in
	cd "$(abs_top_srcdir)" && $(AUTOTEST) -I tests %D%/testsuite.at -o $@

%D%/atconfig: $(top_builddir)/config.status
	cd "$(top_builddir)" && ./config.status $@

check-local: %D%/atconfig %D%/atlocal $(TESTSUITE)
	cd "$(top_builddir)/tests" && $(SHELL) "$(abs_top_srcdir)/$(TESTSUITE)" $(TESTSUITEFLAGS)

clean-local:
	cd "$(top_builddir)/tests" && (test ! -f "$(abs_top_srcdir)/$(TESTSUITE)" || $(SHELL) "$(abs_top_srcdir)/$(TESTSUITE)" --clean)

EXTRA_DIST += %D%/testsuite.at $(TESTSUITE) %D%/atlocal.in
[= FOR tests =][=
 (out-push-new (string-append (get "name") "/" "Makefile.am"))
=]## Generated by Autogen from [= (tpl-file) =]

[= IF need_threads =]if MULTI_THREADED
[=
  (if (not (string-null? (get "conditional")))
      (emit "if " (get "conditional") "\n"))
=][= ENDIF =]noinst_PROGRAMS += [=name=]

[=name=]_sources = \
[=
(let ((name (get "name"))
      (files (list)))
  (define (emit-cxx-files-ftw-cb filename statinfo flag)
    (begin
      (if (and (eq? flag 'regular)
               (string-suffix-ci? ".cxx" filename))
          (set! files (append! files (list filename))))
      #t))
  (begin
    (ftw name emit-cxx-files-ftw-cb)
    ;; Add the generated header as it will not be found by file search.
    (set! files (sort! files string-ci<?))
    (emit "\t"
          (string-join (map (lambda (x)
                              (string-append "%D%/"
                                             (if (string-prefix? name x)
                                                 (substring x (1+ (string-length name)))
                                                 x)))
                            files)
                       " \\\n\t")
          "\n")))
=]
[=name=]_SOURCES = $([=name=]_sources)

[=name=]_CPPFLAGS = $(AM_CPPFLAGS)
[=name=]_LDADD = $(liblog4cplus_la_file)
[=name=]_LDFLAGS = -no-install

if BUILD_WITH_WCHAR_T_SUPPORT
noinst_PROGRAMS += [=name=]U
[=name=]U_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE=1 -D_UNICODE=1
[=name=]U_SOURCES = $([=name=]_sources)
[=name=]U_LDADD = $(liblog4cplusU_la_file)
[=name=]U_LDFLAGS = -no-install
endif # BUILD_WITH_WCHAR_T_SUPPORT
[=
(begin
  (if (access? (string-concatenate (list (get "name") "/Makefile.am.inc")) R_OK)
      (emit "include %D%/Makefile.am.inc")))
=][= IF need_threads =]
endif # MULTI_THREADED
[= ENDIF =][=
(begin
  (if (not (string-null? (get "conditional")))
    (emit "endif # " (get "conditional") "\n"))
  (out-pop))
=][= ENDFOR tests =]
