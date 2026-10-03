set (qt4debugappender_sources
  qt4debugappender.cxx)

find_package (Qt4 REQUIRED)
include (${QT_USE_FILE})

set (qt4debugappender log4cplusqt4debugappender${log4cplus_postfix})
add_library (${qt4debugappender} ${qt4debugappender_sources})
add_library (log4cplus::qt4debugappender ALIAS ${qt4debugappender})
if (UNICODE)
  target_compile_definitions (${qt4debugappender} PUBLIC UNICODE)
  target_compile_definitions (${qt4debugappender} PUBLIC _UNICODE)
  add_definitions (-UMBCS -U_MBCS)
endif (UNICODE)
if (${BUILD_SHARED_LIBS})
  target_compile_definitions (${qt4debugappender} PRIVATE ${log4cplus}_EXPORTS)
endif ()
target_link_libraries (${qt4debugappender}
  PUBLIC ${log4cplus}
  PRIVATE ${QT_LIBRARIES}
  PUBLIC ${CMAKE_THREAD_LIBS_INIT})

log4cplus_apply_libtool_version (${qt4debugappender} log4cplus_namelink_option)
if (NOT log4cplus_namelink_option)
  if (ANDROID)
    # Android does not seem to have SO version support.
  elseif (WIN32)
    set_target_properties (${qt4debugappender} PROPERTIES
      VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}")
  else ()
    set_target_properties (${qt4debugappender} PROPERTIES
      SOVERSION "${log4cplus_soversion}")
  endif ()

  if (APPLE)
    set_target_properties (${log4cplus} PROPERTIES
      MACHO_CURRENT_VERSION "${log4cplus_macho_current_version}"
      MACHO_COMPATIBILITY_VERSION "${log4cplus_macho_compatibility_version}")
  endif ()
endif ()
target_compile_definitions (${qt4debugappender} PRIVATE INSIDE_LOG4CPLUS_QT4DEBUGAPPENDER)

if (WIN32)
  set_target_properties (${qt4debugappender} PROPERTIES
    DEBUG_POSTFIX "D")
endif ()

log4cplus_prepare_libtool_install (${qt4debugappender} "${CMAKE_INSTALL_LIBDIR}")
install (TARGETS ${qt4debugappender}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ${log4cplus_namelink_option}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
log4cplus_install_libtool_alias (${qt4debugappender} "${CMAKE_INSTALL_LIBDIR}")

install (FILES ../include/log4cplus/qt4debugappender.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus)
