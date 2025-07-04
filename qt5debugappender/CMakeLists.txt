set (qt5debugappender_sources
  qt5debugappender.cxx)

find_package(Qt5 COMPONENTS Core REQUIRED)

set (qt5debugappender log4cplusqt5debugappender${log4cplus_postfix})
add_library (${qt5debugappender} ${qt5debugappender_sources})
add_library (log4cplus::qt5debugappender ALIAS ${qt5debugappender})
if (UNICODE)
  target_compile_definitions (${qt5debugappender} PUBLIC UNICODE)
  target_compile_definitions (${qt5debugappender} PUBLIC _UNICODE)
  add_definitions (-UMBCS -U_MBCS)
endif (UNICODE)
if (${BUILD_SHARED_LIBS})
  target_compile_definitions (${qt5debugappender} PRIVATE ${log4cplus}_EXPORTS)
endif ()
target_link_libraries (${qt5debugappender}
  PUBLIC ${log4cplus}
  PRIVATE Qt5::Core
  PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_include_directories (${qt5debugappender} PUBLIC ${Qt5Widgets_INCLUDES})
target_compile_definitions (${qt5debugappender} PUBLIC ${Qt5Widgets_DEFINITIONS})

if (ANDROID)
  # Android does not seem to have SO version support.
elseif (WIN32)
  set_target_properties (${qt5debugappender} PROPERTIES
    VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}")
else ()
  set_target_properties (${qt5debugappender} PROPERTIES
    SOVERSION "${log4cplus_soversion}")
endif ()
target_compile_definitions (${qt5debugappender} PRIVATE INSIDE_LOG4CPLUS_QT5DEBUGAPPENDER)

if (APPLE)
  set_target_properties (${log4cplus} PROPERTIES
    MACHO_CURRENT_VERSION "${log4cplus_macho_current_version}"
    MACHO_COMPATIBILITY_VERSION "${log4cplus_macho_compatibility_version}")
endif ()

if (WIN32)
  set_target_properties (${qt5debugappender} PROPERTIES
    DEBUG_POSTFIX "D")
endif ()

install (TARGETS ${qt5debugappender}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install (FILES ../include/log4cplus/qt5debugappender.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus)
