cmake_minimum_required(VERSION 3.16)

# --- Single-source the version from the public header -----------------------
# src/Ilib.h remains the authoritative source of the version number; CMake
# parses it so the two can never drift (see also AUTHORS / NEWS / ChangeLog).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/Ilib.h" _ilib_version_line
     REGEX "^#define[ \t]+ILIB_VERSION[ \t]+\"[0-9.]+\"")
string(REGEX REPLACE ".*\"([0-9.]+)\".*" "\\1" ILIB_VERSION "${_ilib_version_line}")
if(NOT ILIB_VERSION)
  message(FATAL_ERROR "Could not parse ILIB_VERSION from src/Ilib.h")
endif()

project(Ilib
  VERSION ${ILIB_VERSION}
  DESCRIPTION "C library to read, create, manipulate and save raster images"
  HOMEPAGE_URL "https://www.k5n.us/Ilib.php"
  LANGUAGES C)

# --- Options ----------------------------------------------------------------
option(BUILD_SHARED_LIBS  "Build Ilib as a shared library"        ON)
option(ILIB_WITH_PNG      "Enable PNG support (requires libpng)"   ON)
option(ILIB_WITH_JPEG     "Enable JPEG support (requires libjpeg)" ON)
option(ILIB_WITH_GIF      "Enable GIF support (requires giflib)"   ON)
option(ILIB_BUILD_CLIENTS  "Build the bundled client tools"        ON)
option(ILIB_BUILD_EXAMPLES "Build the bundled example programs"    ON)
option(ILIB_BUILD_TESTS    "Build the test suite"                  OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

add_subdirectory(src)

if(ILIB_BUILD_CLIENTS)
  add_subdirectory(clients)
endif()
if(ILIB_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
if(ILIB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# --- Install package config + pkg-config ------------------------------------
include(CMakePackageConfigHelpers)

install(EXPORT IlibTargets
  FILE IlibTargets.cmake
  NAMESPACE Ilib::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Ilib")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IlibConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/IlibConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Ilib")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/IlibConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/IlibConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/IlibConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Ilib")

# pkg-config (.pc) for non-CMake consumers. Compute the path from the
# installed pkgconfig directory back up to the prefix so the .pc is relocatable
# (the depth is prefix-independent, so this survives --install --prefix).
file(RELATIVE_PATH ILIB_PC_REL_PREFIX
  "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig"
  "${CMAKE_INSTALL_PREFIX}")
string(REGEX REPLACE "/$" "" ILIB_PC_REL_PREFIX "${ILIB_PC_REL_PREFIX}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ilib.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ilib.pc"
  @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ilib.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

# --- Configuration summary --------------------------------------------------
message(STATUS "")
message(STATUS "Ilib ${PROJECT_VERSION} configuration:")
message(STATUS "  Shared library : ${BUILD_SHARED_LIBS}")
message(STATUS "  PNG support    : ${ILIB_PNG_ENABLED}")
message(STATUS "  JPEG support   : ${ILIB_JPEG_ENABLED}")
message(STATUS "  GIF support    : ${ILIB_GIF_ENABLED}")
message(STATUS "  Clients        : ${ILIB_BUILD_CLIENTS}")
message(STATUS "  Examples       : ${ILIB_BUILD_EXAMPLES}")
message(STATUS "  Tests          : ${ILIB_BUILD_TESTS}")
message(STATUS "")
