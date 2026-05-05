cmake_minimum_required(VERSION 3.14)

# Read version from the VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" KISSAT_VERSION)
string(STRIP "${KISSAT_VERSION}" KISSAT_VERSION)

project(kissat VERSION ${KISSAT_VERSION} LANGUAGES C)

# --------------------------------------------------------------------------- #
# Options
# --------------------------------------------------------------------------- #

option(KISSAT_BUILD_EXECUTABLE "Build the kissat command-line executable" ON)

# --------------------------------------------------------------------------- #
# Generate build.h (replaces scripts/generate-build-header.sh)
# --------------------------------------------------------------------------- #

# Attempt to get the git commit id
set(KISSAT_ID "unknown")
find_package(Git QUIET)
if(Git_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE KISSAT_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_RESULT)
  if(NOT GIT_RESULT EQUAL 0)
    set(KISSAT_ID "unknown")
  endif()
endif()

set(KISSAT_COMPILER "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
string(TIMESTAMP KISSAT_BUILD "%a %b %d %H:%M:%S %Z %Y")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/build.h"
  @ONLY)

# --------------------------------------------------------------------------- #
# Collect source files
# --------------------------------------------------------------------------- #

# Application-only sources (part of the executable, not the library)
set(KISSAT_APP_SOURCES
  src/application.c
  src/handle.c
  src/parse.c
  src/witness.c)

# All .c files in src/
file(GLOB KISSAT_ALL_SOURCES CONFIGURE_DEPENDS "src/*.c")

# Library sources = all sources minus main.c and app sources
set(KISSAT_LIB_SOURCES ${KISSAT_ALL_SOURCES})
list(REMOVE_ITEM KISSAT_LIB_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/application.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/handle.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/parse.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/witness.c")

# --------------------------------------------------------------------------- #
# Library target
# --------------------------------------------------------------------------- #

add_library(kissat STATIC ${KISSAT_LIB_SOURCES})

target_include_directories(kissat
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(kissat PRIVATE NDEBUG)

# The library needs libm on Unix-like systems
include(CheckLibraryExists)
check_library_exists(m sqrt "" HAVE_LIBM)
if(HAVE_LIBM)
  target_link_libraries(kissat PUBLIC m)
endif()

set_target_properties(kissat PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------------- #
# Executable target (optional)
# --------------------------------------------------------------------------- #

if(KISSAT_BUILD_EXECUTABLE)
  add_executable(kissat-bin
    src/main.c
    ${KISSAT_APP_SOURCES})

  target_link_libraries(kissat-bin PRIVATE kissat)

  target_include_directories(kissat-bin PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

  set_target_properties(kissat-bin PROPERTIES
    OUTPUT_NAME kissat
    C_STANDARD 99
    C_STANDARD_REQUIRED ON)
endif()

# --------------------------------------------------------------------------- #
# Install & export (supports find_package and FetchContent/CPM)
# --------------------------------------------------------------------------- #

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS kissat
  EXPORT kissatTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES src/kissat.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(KISSAT_BUILD_EXECUTABLE)
  install(TARGETS kissat-bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(EXPORT kissatTargets
  FILE kissatTargets.cmake
  NAMESPACE kissat::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kissat)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/kissatConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/kissatConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kissat)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/kissatConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/kissatConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/kissatConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kissat)
