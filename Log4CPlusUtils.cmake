#
# Utility macros for Log4Cplus project
#

# Get Log4cplus version macro
# first param - path to include folder, we will rip version from version.h
macro(log4cplus_get_version _include_PATH vmajor vminor vpatch)
  file(STRINGS "${_include_PATH}/log4cplus/version.h" _log4cplus_VER_STRING_AUX REGEX ".*#define[ ]+LOG4CPLUS_VERSION[ ]+")
  string(REGEX MATCHALL "[0-9]+" _log4clpus_VER_LIST "${_log4cplus_VER_STRING_AUX}")
  list(LENGTH _log4clpus_VER_LIST _log4cplus_VER_LIST_LEN)
# we also count '4' from the name...
  if(_log4cplus_VER_LIST_LEN EQUAL 5)
    list(GET _log4clpus_VER_LIST 2 ${vmajor})
    list(GET _log4clpus_VER_LIST 3 ${vminor})
    list(GET _log4clpus_VER_LIST 4 ${vpatch})
  endif()
endmacro()

# configure.ac is the single source of libtool's interface and release versions.
# Only literal assignments are accepted; CMake must not interpret shell code.
function(log4cplus_read_libtool_version configure_ac)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${configure_ac}")
  foreach(name IN ITEMS LT_VERSION LT_RELEASE)
    file(STRINGS "${configure_ac}" assignments REGEX "^[ \t]*${name}[ \t]*=")
    list(LENGTH assignments count)
    if(NOT count EQUAL 1)
      message(FATAL_ERROR "Expected one ${name} assignment in ${configure_ac}")
    endif()
    if(name STREQUAL "LT_VERSION")
      if(NOT assignments MATCHES "^[ \t]*LT_VERSION[ \t]*=[ \t]*([0-9]+):([0-9]+):([0-9]+)[ \t]*(#.*)?$")
        message(FATAL_ERROR "Malformed LT_VERSION in ${configure_ac}: expected current:revision:age")
      endif()
      set(current "${CMAKE_MATCH_1}")
      set(revision "${CMAKE_MATCH_2}")
      set(age "${CMAKE_MATCH_3}")
      foreach(part IN ITEMS current revision age)
        # Avoid interpreting leading zeroes as octal in math(EXPR).
        string(REGEX REPLACE "^0+([0-9])" "\\1" ${part} "${${part}}")
      endforeach()
    else()
      if(NOT assignments MATCHES "^[ \t]*LT_RELEASE[ \t]*=[ \t]*([A-Za-z0-9][A-Za-z0-9_.+-]*)[ \t]*(#.*)?$")
        message(FATAL_ERROR "Malformed LT_RELEASE in ${configure_ac}: expected a filename-safe release label")
      endif()
      set(release "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  if(age GREATER current)
    message(FATAL_ERROR "Invalid LT_VERSION in ${configure_ac}: age must not exceed current")
  endif()
  foreach(part IN ITEMS current revision age release)
    set(log4cplus_lt_${part} "${${part}}" PARENT_SCOPE)
  endforeach()
endfunction()

# Leave logical target names (including S/U decoration) and exported aliases
# alone. CMake supplies the platform's complete filenames and runtime metadata.
function(log4cplus_apply_libtool_version target namelink_option)
  set(${namelink_option} "" PARENT_SCOPE)
  get_target_property(type "${target}" TYPE)
  if(NOT log4cplus_use_libtool_versioning OR NOT type STREQUAL "SHARED_LIBRARY")
    return()
  endif()
  get_target_property(basename "${target}" OUTPUT_NAME)
  if(NOT basename)
    set(basename "${target}")
  endif()
  math(EXPR major "${log4cplus_lt_current} - ${log4cplus_lt_age}")
  if(APPLE)
    set(version "${major}")
    set(soversion "${major}")
    math(EXPR compatibility "${log4cplus_lt_current} + 1")
    set_target_properties("${target}" PROPERTIES
      MACHO_COMPATIBILITY_VERSION "${compatibility}"
      MACHO_CURRENT_VERSION "${compatibility}.${log4cplus_lt_revision}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(version "${log4cplus_lt_current}.${log4cplus_lt_revision}")
    set(soversion "${version}")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(version "${log4cplus_lt_current}.${log4cplus_lt_revision}")
    set(soversion "${log4cplus_lt_current}")
  else()
    set(version "${major}.${log4cplus_lt_age}.${log4cplus_lt_revision}")
    set(soversion "${major}")
  endif()
  set(output_name "${basename}")
  if(LOG4CPLUS_ENABLE_LIBTOOL_RELEASE)
    string(APPEND output_name "-${log4cplus_lt_release}")
  endif()
  set_target_properties("${target}" PROPERTIES
    OUTPUT_NAME "${output_name}"
    VERSION "${version}" SOVERSION "${soversion}"
    LOG4CPLUS_LIBTOOL_BASENAME "${basename}")
  set(${namelink_option} NAMELINK_SKIP PARENT_SCOPE)
endfunction()

# Generate the same install-time names and paths for preparation and alias
# creation. Native linker filenames include configuration postfixes.
function(_log4cplus_libtool_install_code target destination result)
  set(${result} "" PARENT_SCOPE)
  get_target_property(basename "${target}" LOG4CPLUS_LIBTOOL_BASENAME)
  if(NOT basename)
    return()
  endif()
  set(plain_prefix "$<TARGET_LINKER_FILE_PREFIX:${target}>${basename}")
  set(release_prefix "${plain_prefix}-${log4cplus_lt_release}")
  set(alias_version "")
  if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(alias_version ".${log4cplus_lt_current}.${log4cplus_lt_revision}")
  endif()
  if(IS_ABSOLUTE "${destination}")
    set(install_dir "${destination}")
  else()
    set(install_dir "\${CMAKE_INSTALL_PREFIX}/${destination}")
  endif()
  # @ONLY preserves install-time variables, including --prefix and DESTDIR.
  # BOOL emits 0/1 for compatibility with older install-script policies.
  string(CONFIGURE [=[
set(_log4cplus_realname "$<TARGET_FILE_NAME:@target@>")
set(_log4cplus_linkname "$<TARGET_LINKER_FILE_NAME:@target@>")
if($<BOOL:@LOG4CPLUS_ENABLE_LIBTOOL_RELEASE@>)
  # Strip only our inserted release, not matching text in a configuration
  # postfix or the original basename.
  string(FIND "${_log4cplus_linkname}" "@release_prefix@" _log4cplus_position)
  if(NOT _log4cplus_position EQUAL 0)
    message(FATAL_ERROR "Unexpected libtool linker filename: ${_log4cplus_linkname}")
  endif()
  string(LENGTH "@release_prefix@" _log4cplus_length)
  string(SUBSTRING "${_log4cplus_linkname}" ${_log4cplus_length} -1 _log4cplus_tail)
  set(_log4cplus_linkname "@plain_prefix@${_log4cplus_tail}")
endif()
string(APPEND _log4cplus_linkname "@alias_version@")
set(_log4cplus_alias "@install_dir@/${_log4cplus_linkname}")
set(_log4cplus_staged_alias "$ENV{DESTDIR}${_log4cplus_alias}")
]=] code @ONLY)
  set(${result} "${code}" PARENT_SCOPE)
endfunction()

# Call before install(TARGETS). On OpenBSD, disabling the release suffix makes
# the runtime filename coincide with the previous installation's linker alias.
# Remove that symlink so matching timestamps cannot make CMake skip the copy.
function(log4cplus_prepare_libtool_install target destination)
  _log4cplus_libtool_install_code("${target}" "${destination}" code)
  if(NOT code)
    return()
  endif()
  string(APPEND code [=[
if(_log4cplus_linkname STREQUAL _log4cplus_realname
    AND IS_SYMLINK "${_log4cplus_staged_alias}")
  file(REMOVE "${_log4cplus_staged_alias}")
endif()
]=])
  install(CODE "${code}")
endfunction()

# Call after install(TARGETS ... LIBRARY ... NAMELINK_SKIP). Libtool's linker
# alias omits the release; OpenBSD additionally retains current.revision.
function(log4cplus_install_libtool_alias target destination)
  _log4cplus_libtool_install_code("${target}" "${destination}" code)
  if(NOT code)
    return()
  endif()
  # Never replace an installed runtime file with a self-referential symlink.
  string(APPEND code [=[
if(NOT _log4cplus_linkname STREQUAL _log4cplus_realname)
  file(REMOVE "${_log4cplus_staged_alias}")
  file(CREATE_LINK "${_log4cplus_realname}" "${_log4cplus_staged_alias}" SYMBOLIC)
  list(APPEND CMAKE_INSTALL_MANIFEST_FILES "${_log4cplus_alias}")
endif()
]=])
  install(CODE "${code}")
endfunction()
