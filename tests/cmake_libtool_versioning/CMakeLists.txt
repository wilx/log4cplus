cmake_minimum_required(VERSION 3.20)
project(libtool_versioning_contract C)
# Include custom configurations when invoked from a multi-configuration parent.
if(CMAKE_CONFIGURATION_TYPES AND TEST_CONFIG
    AND NOT TEST_CONFIG IN_LIST CMAKE_CONFIGURATION_TYPES)
  list(APPEND CMAKE_CONFIGURATION_TYPES "${TEST_CONFIG}")
endif()
include("${UTILS}")
option(LOG4CPLUS_ENABLE_LIBTOOL_RELEASE "Include the libtool release suffix." ON)
set(log4cplus_use_libtool_versioning ON)
log4cplus_read_libtool_version("${METADATA}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/probe.c" "int probe(void) { return 42; }\n")

# Exercise the common contract without needing three generations of Qt SDKs.
foreach(target IN ITEMS log4cplus log4cplusqt4debugappender
    log4cplusqt5debugappender log4cplusqt6debugappender)
  add_library(${target} SHARED "${CMAKE_CURRENT_BINARY_DIR}/probe.c")
  set_target_properties(${target} PROPERTIES OUTPUT_NAME "${target}${DECORATION}")
  log4cplus_apply_libtool_version(${target} namelink_option)
  log4cplus_prepare_libtool_install(${target} "${LIBDIR}")
  install(TARGETS ${target} LIBRARY DESTINATION "${LIBDIR}" ${namelink_option})
  log4cplus_install_libtool_alias(${target} "${LIBDIR}")
  foreach(property IN ITEMS VERSION SOVERSION MACHO_CURRENT_VERSION
      MACHO_COMPATIBILITY_VERSION)
    get_target_property(value ${target} ${property})
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/properties.txt"
      "${target}.${property}=${value}\n")
  endforeach()
  file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${target}.name"
    CONTENT "$<TARGET_FILE_NAME:${target}>")
  file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${target}.path"
    CONTENT "$<TARGET_FILE:${target}>")
endforeach()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/consumer.c"
  "extern int probe(void); int main(void) { return probe() != 42; }\n")
add_executable(probe_consumer "${CMAKE_CURRENT_BINARY_DIR}/consumer.c")
target_link_libraries(probe_consumer PRIVATE log4cplus)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/probe_consumer.path"
  CONTENT "$<TARGET_FILE:probe_consumer>")

add_library(static_probe STATIC "${CMAKE_CURRENT_BINARY_DIR}/probe.c")
log4cplus_apply_libtool_version(static_probe namelink_option)
get_target_property(output_name static_probe OUTPUT_NAME)
if(namelink_option OR output_name)
  message(FATAL_ERROR "Static target acquired libtool naming")
endif()
