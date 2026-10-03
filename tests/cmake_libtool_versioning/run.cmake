cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED COMPILER_ARG1)
  set(COMPILER_ARG1 "")
endif()
# Keep the original command-line string for nested CMake configurations, and
# parse a separate copy for execute_process() calls that invoke the compiler.
separate_arguments(compiler_arguments NATIVE_COMMAND "${COMPILER_ARG1}")
# Initialize through CC: CMake 3.20 overwrites ARG1 when CMAKE_C_COMPILER is
# supplied explicitly. Its compiler detection expands CC as a CMake list,
# so escape semicolons in this environment value, but not in ARG1 itself.
string(REPLACE ";" "\\;" compiler_environment "\"${COMPILER}\" ${COMPILER_ARG1}")

if(NOT TEST_CONFIG)
  set(TEST_CONFIG Release)
endif()
# Concurrent CTest configurations must not remove each other's working files.
set(TEST_ROOT "${TEST_ROOT}/${TEST_CONFIG}")
set(generator_args "")
if(TEST_GENERATOR)
  list(APPEND generator_args -G "${TEST_GENERATOR}")
endif()
if(TEST_MAKE_PROGRAM)
  list(APPEND generator_args "-DCMAKE_MAKE_PROGRAM=${TEST_MAKE_PROGRAM}")
endif()
set(targets log4cplus log4cplusqt4debugappender
  log4cplusqt5debugappender log4cplusqt6debugappender)

function(run)
  # PARSE_ARGV preserves literal semicolons inside individual arguments.
  cmake_parse_arguments(PARSE_ARGV 0 command "" "" "")
  execute_process(COMMAND ${command_UNPARSED_ARGUMENTS} RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Command failed (${result}): ${ARGV}\n${output}\n${error}")
  endif()
endfunction()

function(configure_case name metadata)
  set(dir "${TEST_ROOT}/${name}")
  file(MAKE_DIRECTORY "${dir}")
  file(WRITE "${dir}/configure.ac" "${metadata}")
  # Reconfiguration must not append to the previous property snapshot.
  file(REMOVE "${dir}/properties.txt")
  run("${CMAKE_COMMAND}" -E env "CC=${compiler_environment}"
    "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${dir}"
    ${generator_args} "-DCMAKE_BUILD_TYPE=${TEST_CONFIG}" "-DTEST_CONFIG=${TEST_CONFIG}"
    "-DUTILS=${UTILS}" "-DMETADATA=${dir}/configure.ac"
    "-DLIBDIR=lib" "-DCMAKE_INSTALL_PREFIX=${TEST_ROOT}/unused prefix"
    "-DCMAKE_C_COMPILER_ARG1:STRING=${COMPILER_ARG1}"
    ${ARGN})
  # Check the detected invocation, not just the cache: older CMake versions
  # can silently rewrite ARG1 while leaving its cache entry unchanged.
  include("${dir}/CMakeFiles/${CMAKE_VERSION}/CMakeCCompiler.cmake")
  if(NOT CMAKE_C_COMPILER_ARG1 STREQUAL COMPILER_ARG1)
    message(FATAL_ERROR "Required compiler arguments changed during detection: '${CMAKE_C_COMPILER_ARG1}'")
  endif()
endfunction()

function(build_case name)
  run("${CMAKE_COMMAND}" --build "${TEST_ROOT}/${name}" --config "${TEST_CONFIG}")
endfunction()

function(install_case name prefix)
  set(stage "")
  if(ARGC GREATER 2)
    set(stage "${ARGV2}")
  endif()
  run("${CMAKE_COMMAND}" -E env "DESTDIR=${stage}"
    "${CMAKE_COMMAND}" --install "${TEST_ROOT}/${name}"
    --config "${TEST_CONFIG}" --prefix "${prefix}")
endfunction()

function(target_path name target result)
  file(READ "${TEST_ROOT}/${name}/${TEST_CONFIG}/${target}.path" path)
  set(${result} "${path}" PARENT_SCOPE)
endfunction()

function(expect_contains path expected)
  file(READ "${path}" actual)
  string(FIND "${actual}" "${expected}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "${path}: expected '${expected}', got '${actual}'")
  endif()
endfunction()

function(expect_manifest path entry)
  file(STRINGS "${path}" entries)
  if(NOT entry IN_LIST entries)
    message(FATAL_ERROR "Missing install manifest entry in ${path}: ${entry}")
  endif()
endfunction()

function(expect_alias prefix basename realname)
  set(alias "${prefix}/lib/${basename}")
  if(NOT IS_SYMLINK "${alias}")
    message(FATAL_ERROR "Missing symbolic linker alias: ${alias}")
  endif()
  file(READ_SYMLINK "${alias}" link)
  if(NOT link STREQUAL realname OR NOT EXISTS "${prefix}/lib/${realname}")
    message(FATAL_ERROR "Incorrect relative linker alias: ${alias} -> ${link}")
  endif()
endfunction()

file(REMOVE_RECURSE "${TEST_ROOT}")
set(metadata "LT_VERSION=7:2:4\nLT_RELEASE=3.0\n")
configure_case(native "${metadata}")
build_case(native)
set(prefix "${TEST_ROOT}/installed prefix")
install_case(native "${prefix}")
foreach(target IN LISTS targets)
  expect_alias("${prefix}" "lib${target}.so" "lib${target}-3.0.so.3.4.2")
  expect_manifest("${TEST_ROOT}/native/install_manifest.txt"
    "${prefix}/lib/lib${target}.so")
  if(EXISTS "${prefix}/lib/lib${target}-3.0.so")
    message(FATAL_ERROR "The release-decorated namelink was installed")
  endif()
endforeach()
target_path(native probe_consumer consumer)
run("${consumer}")
find_program(readelf readelf)
if(readelf)
  foreach(binary IN ITEMS "${prefix}/lib/liblog4cplus-3.0.so.3.4.2"
      "${consumer}")
    execute_process(COMMAND "${readelf}" -d "${binary}"
      RESULT_VARIABLE result OUTPUT_VARIABLE dynamic)
    if(NOT result EQUAL 0 OR NOT dynamic MATCHES "(SONAME|NEEDED).*\\[liblog4cplus-3.0.so.3\\]")
      message(FATAL_ERROR "Incorrect ELF runtime name in ${binary}: ${dynamic}")
    endif()
  endforeach()
endif()
# Reinstall, DESTDIR staging and an install-time prefix override.
install_case(native "${prefix}")
install_case(native /override "${TEST_ROOT}/stage root")
expect_alias("${TEST_ROOT}/stage root/override" liblog4cplus.so liblog4cplus-3.0.so.3.4.2)
expect_manifest("${TEST_ROOT}/native/install_manifest.txt"
  "/override/lib/liblog4cplus.so")

# Adding compatible interfaces retains the runtime name within a release.
file(WRITE "${TEST_ROOT}/native/configure.ac" "LT_VERSION=8:0:5\nLT_RELEASE=3.0\n")
# Building alone must notice the metadata dependency and reconfigure.
build_case(native)
install_case(native "${prefix}")
expect_alias("${prefix}" liblog4cplus.so liblog4cplus-3.0.so.3.5.0)
if(readelf)
  execute_process(COMMAND "${readelf}" -d "${prefix}/lib/liblog4cplus-3.0.so.3.5.0"
    RESULT_VARIABLE result OUTPUT_VARIABLE dynamic)
  if(NOT result EQUAL 0 OR NOT dynamic MATCHES "SONAME.*\\[liblog4cplus-3.0.so.3\\]")
    message(FATAL_ERROR "Compatible interface addition changed the runtime name: ${dynamic}")
  endif()
endif()
# Another release coexists at that ABI major; the common alias selects it.
file(WRITE "${TEST_ROOT}/native/configure.ac" "LT_VERSION=8:0:5\nLT_RELEASE=3.1\n")
build_case(native)
install_case(native "${prefix}")
expect_alias("${prefix}" liblog4cplus.so liblog4cplus-3.1.so.3.5.0)
if(NOT EXISTS "${prefix}/lib/liblog4cplus-3.0.so.3.4.2"
    OR NOT EXISTS "${prefix}/lib/liblog4cplus-3.0.so.3.5.0")
  message(FATAL_ERROR "Installing a new release removed the old runtime")
endif()

configure_case(absolute "${metadata}" "-DLIBDIR=${TEST_ROOT}/absolute lib")
build_case(absolute)
install_case(absolute /ignored "${TEST_ROOT}/absolute stage")
set(absolute_alias "${TEST_ROOT}/absolute stage${TEST_ROOT}/absolute lib/liblog4cplus.so")
file(READ_SYMLINK "${absolute_alias}" link)
if(NOT link STREQUAL "liblog4cplus-3.0.so.3.4.2")
  message(FATAL_ERROR "Absolute install destination alias failed")
endif()
expect_manifest("${TEST_ROOT}/absolute/install_manifest.txt"
  "${TEST_ROOT}/absolute lib/liblog4cplus.so")

# Omitting LT_RELEASE preserves ABI versioning for all four shared targets.
configure_case(no_release "${metadata}" -DLOG4CPLUS_ENABLE_LIBTOOL_RELEASE=OFF)
build_case(no_release)
set(no_release_prefix "${TEST_ROOT}/no release prefix")
install_case(no_release "${no_release_prefix}")
foreach(target IN LISTS targets)
  expect_alias("${no_release_prefix}" "lib${target}.so" "lib${target}.so.3.4.2")
  expect_manifest("${TEST_ROOT}/no_release/install_manifest.txt"
    "${no_release_prefix}/lib/lib${target}.so")
endforeach()
target_path(no_release probe_consumer consumer)
run("${consumer}")
if(readelf)
  foreach(binary IN ITEMS "${no_release_prefix}/lib/liblog4cplus.so.3.4.2"
      "${consumer}")
    execute_process(COMMAND "${readelf}" -d "${binary}"
      RESULT_VARIABLE result OUTPUT_VARIABLE dynamic)
    if(NOT result EQUAL 0 OR NOT dynamic MATCHES "(SONAME|NEEDED).*\\[liblog4cplus.so.3\\]")
      message(FATAL_ERROR "Incorrect runtime name without LT_RELEASE: ${dynamic}")
    endif()
  endforeach()
endif()
# Toggle in one build and prefix; the linker alias must follow the active mode.
configure_case(no_release "${metadata}" -DLOG4CPLUS_ENABLE_LIBTOOL_RELEASE=ON)
build_case(no_release)
install_case(no_release "${no_release_prefix}")
expect_alias("${no_release_prefix}" liblog4cplus.so liblog4cplus-3.0.so.3.4.2)
configure_case(no_release "${metadata}" -DLOG4CPLUS_ENABLE_LIBTOOL_RELEASE=OFF)
build_case(no_release)
install_case(no_release "${no_release_prefix}")
expect_alias("${no_release_prefix}" liblog4cplus.so liblog4cplus.so.3.4.2)

# Debug and Release must keep distinct linker aliases in a shared prefix.
function(check_postfix name release decoration debug_postfix)
  set(prefix "${TEST_ROOT}/${name} prefix")
  set(release_suffix "")
  if(release)
    set(release_suffix -3.0)
  endif()
  foreach(TEST_CONFIG IN ITEMS Release Debug)
    configure_case("${name}" "${metadata}"
      "-DLOG4CPLUS_ENABLE_LIBTOOL_RELEASE=${release}"
      "-DDECORATION=${decoration}" "-DCMAKE_DEBUG_POSTFIX=${debug_postfix}")
    build_case("${name}")
    install_case("${name}" "${prefix}")
    set(postfix "")
    if(TEST_CONFIG STREQUAL "Debug")
      set(postfix "${debug_postfix}")
    endif()
    foreach(target IN LISTS targets)
      set(base "${target}${decoration}")
      expect_alias("${prefix}" "lib${base}${postfix}.so"
        "lib${base}${release_suffix}${postfix}.so.3.4.2")
      expect_manifest("${TEST_ROOT}/${name}/install_manifest.txt"
        "${prefix}/lib/lib${base}${postfix}.so")
    endforeach()
    # Link by the public alias, independently of CMake's build-tree target.
    set(consumer "${TEST_ROOT}/${name}/installed-consumer-${TEST_CONFIG}")
    run("${COMPILER}" ${compiler_arguments} "${TEST_ROOT}/${name}/consumer.c"
      "-L${prefix}/lib" "-llog4cplus${decoration}${postfix}"
      "-Wl,-rpath,${prefix}/lib" -o "${consumer}")
    run("${consumer}")
    if(readelf)
      execute_process(COMMAND "${readelf}" -d "${consumer}"
        RESULT_VARIABLE result OUTPUT_VARIABLE dynamic)
      set(needed "liblog4cplus${decoration}${release_suffix}${postfix}.so.3")
      string(FIND "${dynamic}" "[${needed}]" found)
      if(NOT result EQUAL 0 OR found EQUAL -1)
        message(FATAL_ERROR "Incorrect installed consumer dependency: ${dynamic}")
      endif()
    endif()
  endforeach()
  foreach(target IN LISTS targets)
    set(base "${target}${decoration}")
    expect_alias("${prefix}" "lib${base}.so" "lib${base}${release_suffix}.so.3.4.2")
    expect_alias("${prefix}" "lib${base}${debug_postfix}.so"
      "lib${base}${release_suffix}${debug_postfix}.so.3.4.2")
  endforeach()
endfunction()

foreach(release IN ITEMS ON OFF)
  check_postfix("postfix_${release}" "${release}" "" d)
  # Matching release text in the postfix must survive suffix removal.
  check_postfix("unicode_postfix_${release}" "${release}" U -3.0d)
endforeach()

# These are generation checks, not native execution on the named platforms.
foreach(system IN ITEMS FreeBSD DragonFly Haiku NetBSD OpenBSD Darwin)
  configure_case("${system}" "${metadata}" "-DCMAKE_SYSTEM_NAME=${system}"
    -DCMAKE_C_COMPILER_WORKS=TRUE)
  if(system STREQUAL "Darwin")
    set(version 3)
    set(soversion 3)
    set(filename liblog4cplus-3.0.3.dylib)
    expect_contains("${TEST_ROOT}/${system}/properties.txt"
      "log4cplus.MACHO_CURRENT_VERSION=8.2")
    expect_contains("${TEST_ROOT}/${system}/properties.txt"
      "log4cplus.MACHO_COMPATIBILITY_VERSION=8")
  elseif(system STREQUAL "NetBSD")
    set(version 7.2)
    set(soversion 7)
    set(filename liblog4cplus-3.0.so.7.2)
  elseif(system STREQUAL "OpenBSD")
    set(version 7.2)
    set(soversion 7.2)
    set(filename liblog4cplus-3.0.so.7.2)
  else()
    set(version 3.4.2)
    set(soversion 3)
    set(filename liblog4cplus-3.0.so.3.4.2)
  endif()
  foreach(target IN LISTS targets)
    expect_contains("${TEST_ROOT}/${system}/properties.txt" "${target}.VERSION=${version}\n")
    expect_contains("${TEST_ROOT}/${system}/properties.txt" "${target}.SOVERSION=${soversion}\n")
  endforeach()
  expect_contains("${TEST_ROOT}/${system}/${TEST_CONFIG}/log4cplus.name" "${filename}")
  configure_case("${system}_no_release" "${metadata}" "-DCMAKE_SYSTEM_NAME=${system}"
    -DCMAKE_C_COMPILER_WORKS=TRUE -DLOG4CPLUS_ENABLE_LIBTOOL_RELEASE=OFF)
  string(REPLACE "-3.0" "" no_release_filename "${filename}")
  expect_contains("${TEST_ROOT}/${system}_no_release/${TEST_CONFIG}/log4cplus.name" "${no_release_filename}")
  foreach(target IN LISTS targets)
    expect_contains("${TEST_ROOT}/${system}_no_release/properties.txt" "${target}.VERSION=${version}\n")
    expect_contains("${TEST_ROOT}/${system}_no_release/properties.txt" "${target}.SOVERSION=${soversion}\n")
  endforeach()
endforeach()

# Execute the synthetic OpenBSD install on this ELF host to catch the alias
# colliding with the runtime filename. This is not native BSD validation.
build_case(OpenBSD_no_release)
set(openbsd_prefix "${TEST_ROOT}/OpenBSD install prefix")
install_case(OpenBSD_no_release "${openbsd_prefix}")
install_case(OpenBSD_no_release "${openbsd_prefix}")
foreach(target IN LISTS targets)
  set(runtime "${openbsd_prefix}/lib/lib${target}.so.7.2")
  if(NOT EXISTS "${runtime}" OR IS_SYMLINK "${runtime}")
    message(FATAL_ERROR "Runtime file was replaced by an alias: ${runtime}")
  endif()
  expect_manifest("${TEST_ROOT}/OpenBSD_no_release/install_manifest.txt"
    "${runtime}")
endforeach()

# A prior release-decorated install leaves a symlink at the next runtime's
# destination. Matching timestamps must not keep that old library installed.
find_program(touch touch REQUIRED)
function(check_openbsd_toggle name destination prefix stage)
  if(IS_ABSOLUTE "${destination}")
    set(manifest_dir "${destination}")
  else()
    set(manifest_dir "${prefix}/${destination}")
  endif()
  set(installed_dir "${stage}${manifest_dir}")
  foreach(release IN ITEMS ON OFF)
    configure_case("${name}_${release}" "${metadata}"
      -DCMAKE_SYSTEM_NAME=OpenBSD -DCMAKE_C_COMPILER_WORKS=TRUE
      "-DLOG4CPLUS_ENABLE_LIBTOOL_RELEASE=${release}" "-DLIBDIR=${destination}")
    build_case("${name}_${release}")
  endforeach()
  install_case("${name}_ON" "${prefix}" "${stage}")
  foreach(target IN LISTS targets)
    set(released "${installed_dir}/lib${target}-3.0.so.7.2")
    file(SHA256 "${released}" released_hash_${target})
    target_path("${name}_OFF" "${target}" source)
    run("${touch}" -r "${released}" "${source}")
  endforeach()
  # Reinstallation must also leave ordinary runtime files intact.
  foreach(pass RANGE 1 2)
    install_case("${name}_OFF" "${prefix}" "${stage}")
    foreach(target IN LISTS targets)
      set(runtime "${installed_dir}/lib${target}.so.7.2")
      if(NOT EXISTS "${runtime}" OR IS_SYMLINK "${runtime}")
        message(FATAL_ERROR "Stale OpenBSD alias at runtime destination: ${runtime}")
      endif()
      target_path("${name}_OFF" "${target}" source)
      file(SHA256 "${source}" source_hash)
      file(SHA256 "${runtime}" runtime_hash)
      file(SHA256 "${installed_dir}/lib${target}-3.0.so.7.2" released_hash)
      if(NOT runtime_hash STREQUAL source_hash
          OR NOT released_hash STREQUAL released_hash_${target})
        message(FATAL_ERROR "OpenBSD installation selected or modified the wrong runtime")
      endif()
      if(readelf)
        execute_process(COMMAND "${readelf}" -d "${runtime}"
          RESULT_VARIABLE result OUTPUT_VARIABLE dynamic)
        if(NOT result EQUAL 0 OR NOT dynamic MATCHES "SONAME.*\\[lib${target}\\.so\\.7\\.2\\]")
          message(FATAL_ERROR "Incorrect OpenBSD runtime name: ${dynamic}")
        endif()
      endif()
      expect_manifest("${TEST_ROOT}/${name}_OFF/install_manifest.txt"
        "${manifest_dir}/lib${target}.so.7.2")
    endforeach()
  endforeach()
  install_case("${name}_ON" "${prefix}" "${stage}")
  foreach(target IN LISTS targets)
    file(READ_SYMLINK "${installed_dir}/lib${target}.so.7.2" link)
    file(SHA256 "${installed_dir}/${link}" released_hash)
    if(NOT link STREQUAL "lib${target}-3.0.so.7.2"
        OR NOT released_hash STREQUAL released_hash_${target})
      message(FATAL_ERROR "OpenBSD released runtime or alias was not preserved")
    endif()
    expect_manifest("${TEST_ROOT}/${name}_ON/install_manifest.txt"
      "${manifest_dir}/lib${target}.so.7.2")
  endforeach()
endfunction()

check_openbsd_toggle(openbsd_toggle "lib space" "${TEST_ROOT}/OpenBSD toggle prefix" "")
check_openbsd_toggle(openbsd_staged "lib space" /override "${TEST_ROOT}/OpenBSD stage")
check_openbsd_toggle(openbsd_absolute "${TEST_ROOT}/OpenBSD absolute lib" /ignored
  "${TEST_ROOT}/OpenBSD absolute stage")

foreach(bad IN ITEMS "LT_RELEASE=3.0\n" "LT_VERSION=0:0:0\n"
    "LT_VERSION=1:0:2\nLT_RELEASE=3.0\n"
    "LT_VERSION=x:0:0\nLT_RELEASE=3.0\n"
    "LT_VERSION=0:0:0\nLT_RELEASE=../3.0\n"
    "LT_VERSION=0:0:0\nLT_VERSION=1:0:0\nLT_RELEASE=3.0\n")
  file(WRITE "${TEST_ROOT}/bad.ac" "${bad}")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "CC=${compiler_environment}"
    "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}"
    -B "${TEST_ROOT}/bad" ${generator_args}
    "-DCMAKE_C_COMPILER_ARG1:STRING=${COMPILER_ARG1}"
    "-DUTILS=${UTILS}" "-DMETADATA=${TEST_ROOT}/bad.ac"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(result EQUAL 0 OR NOT "${output}${error}" MATCHES "(LT_VERSION|LT_RELEASE)")
    message(FATAL_ERROR "Invalid metadata was not rejected: ${bad}\n${output}${error}")
  endif()
endforeach()
