if (UNICODE)
  add_compile_definitions (UNICODE _UNICODE)
  add_definitions (-UMBCS -U_MBCS)
endif (UNICODE)

if (${BUILD_SHARED_LIBS})
  add_compile_definitions (log4cplus_EXPORTS)
endif ()

# A function to set up a test, since it's the same for each one.  Note:
# unit_tests test is not set up using this function because it does not like
# the additional argument on commmand line and consequently does not run any
# test.
function(log4cplus_add_test _name)
  set(_srcs ${ARGN})
#  message (STATUS "${_name} sources: ${_srcs}")
  add_executable (${_name} ${_srcs})
  target_link_libraries (${_name} PUBLIC ${log4cplus})
  get_filename_component (_log4cplus_properties "log4cplus.properties.in"
    ABSOLUTE)
  add_test(NAME ${_name}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMAND ${_name} ${_log4cplus_properties})
endfunction()


add_subdirectory (appender_test)
add_subdirectory (configandwatch_test)
add_subdirectory (customloglevel_test)
add_subdirectory (fileappender_test)
add_subdirectory (filter_test)
add_subdirectory (hierarchy_test)
add_subdirectory (loglog_test)
add_subdirectory (ndc_test)
add_subdirectory (ostream_test)
add_subdirectory (patternlayout_test)
add_subdirectory (performance_test)
add_subdirectory (priority_test)
add_subdirectory (propertyconfig_test)
#add_subdirectory (socket_test) # I don't know how this test is supposed to be executed
add_subdirectory (thread_test)
add_subdirectory (timeformat_test)
if (WITH_UNIT_TESTS)
  add_subdirectory (unit_tests)
endif (WITH_UNIT_TESTS)
if (LOG4CPLUS_QT6)
  add_subdirectory (qt6messagehandler_test)
endif (LOG4CPLUS_QT6)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
  function(log4cplus_add_libtool_versioning_test name compiler compiler_arg1)
    add_test(NAME "${name}"
      COMMAND "${CMAKE_COMMAND}"
        "-DUTILS=${PROJECT_SOURCE_DIR}/Log4CPlusUtils.cmake"
        "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/${name}"
        "-DCOMPILER=${compiler}"
        "-DCOMPILER_ARG1=${compiler_arg1}"
        "-DTEST_GENERATOR=${CMAKE_GENERATOR}"
        "-DTEST_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
        "-DTEST_CONFIG=$<CONFIG>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake_libtool_versioning/run.cmake")
  endfunction()

  log4cplus_add_libtool_versioning_test(cmake_libtool_versioning
    "${CMAKE_C_COMPILER}" "${CMAKE_C_COMPILER_ARG1}")
  # Exercise required arguments and quoting without depending on ccache or
  # another external compiler wrapper. Retain any parent compiler arguments.
  set(log4cplus_test_compiler_arg1
    "-E env 'LOG4CPLUS_TEST_COMPILER_ARG=two words;one argument' '${CMAKE_C_COMPILER}' ${CMAKE_C_COMPILER_ARG1}")
  log4cplus_add_libtool_versioning_test(cmake_libtool_versioning_compiler_args
    "${CMAKE_COMMAND}" "${log4cplus_test_compiler_arg1}")
endif ()
