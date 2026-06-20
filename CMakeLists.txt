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
option(ILIB_WITH_FREETYPE "Enable TrueType fonts (requires FreeType)" ON)
option(ILIB_BUILD_CLIENTS  "Build the bundled client tools"        ON)
option(ILIB_BUILD_EXAMPLES "Build the bundled example programs"    ON)
option(ILIB_BUILD_TESTS    "Build the test suite"                  OFF)
option(ILIB_WERROR         "Treat compiler warnings as errors (CI)" OFF)
option(ILIB_SANITIZE       "Build with AddressSanitizer + UBSan"   OFF)
option(ILIB_COVERAGE       "Build with coverage instrumentation"   OFF)
option(ILIB_FUZZ           "Build libFuzzer decoder harness (Clang)" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# Emit compile_commands.json for clang-tidy / clangd / IDE tooling (Phase 3+).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Instrumentation (applied to every target). Used by the CI sanitizer and
# coverage jobs; off by default for normal builds.
if(ILIB_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()
if(ILIB_COVERAGE)
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()
if(ILIB_FUZZ)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "ILIB_FUZZ requires Clang (libFuzzer)")
  endif()
  # Instrument the whole tree for coverage-guided fuzzing + ASan/UBSan; the
  # harness target adds -fsanitize=fuzzer (the libFuzzer main) at link time.
  add_compile_options(-fsanitize=fuzzer-no-link,address,undefined
                      -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()

include(GNUInstallDirs)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/IlibFonts.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/IlibWarnings.cmake")

add_subdirectory(src)

if(ILIB_BUILD_CLIENTS)
  add_subdirectory(clients)
endif()
if(ILIB_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
if(ILIB_BUILD_TESTS OR ILIB_FUZZ)
  if(ILIB_BUILD_TESTS)
    enable_testing()
  endif()
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

# --- Bundled fonts ----------------------------------------------------------
# Install the sample X11 BDF fonts to a data dir so consumers can load them at
# runtime via ILoadFontFromFile(). The location is advertised through the
# pkg-config `fontdir` variable (and documented in the README).
set(ILIB_FONT_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/ilib/fonts")
file(GLOB ILIB_BUNDLED_FONTS "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.bdf")
install(FILES ${ILIB_BUNDLED_FONTS}
  DESTINATION "${ILIB_FONT_INSTALL_DIR}")

# Assemble the pkg-config Libs.private list from the math lib plus whichever
# optional codecs were actually enabled, so static consumers link correctly.
set(ILIB_PC_PRIVATE_LIBS "-lm")
if(ILIB_PNG_ENABLED)
  string(APPEND ILIB_PC_PRIVATE_LIBS " -lpng")
endif()
if(ILIB_JPEG_ENABLED)
  string(APPEND ILIB_PC_PRIVATE_LIBS " -ljpeg")
endif()
if(ILIB_GIF_ENABLED)
  string(APPEND ILIB_PC_PRIVATE_LIBS " -lgif")
endif()
if(ILIB_FREETYPE_ENABLED)
  string(APPEND ILIB_PC_PRIVATE_LIBS " -lfreetype")
endif()

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
message(STATUS "  FreeType fonts : ${ILIB_FREETYPE_ENABLED}")
message(STATUS "  Clients        : ${ILIB_BUILD_CLIENTS}")
message(STATUS "  Examples       : ${ILIB_BUILD_EXAMPLES}")
message(STATUS "  Tests          : ${ILIB_BUILD_TESTS}")
message(STATUS "  Install prefix : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Font dir       : ${CMAKE_INSTALL_PREFIX}/${ILIB_FONT_INSTALL_DIR}")
message(STATUS "")

# --- API documentation (Doxygen) --------------------------------------------
# `cmake --build build --target docs` regenerates the HTML API reference from
# the annotated public header. Gated on Doxygen being available so a normal
# build never requires it. The CI docs workflow publishes the output to Pages.
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  set(DOXYGEN_INPUT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/Ilib.h")
  set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
  add_custom_target(docs
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
else()
  message(STATUS "Doxygen not found -- 'docs' target unavailable")
endif()

# --- Packaging (CPack) ------------------------------------------------------
# `cpack -G TGZ` (binary) and `cpack --config CPackSourceConfig.cmake` (source)
# produce release tarballs; the release CI workflow wires this to tag pushes.
# Only configure CPack for a top-level build, not when Ilib is a subproject.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CPACK_PACKAGE_NAME "ilib")
  set(CPACK_PACKAGE_VENDOR "k5n.us")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
  set(CPACK_PACKAGE_CONTACT "Craig Knudsen <craigk5n@gmail.com>")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

  set(CPACK_GENERATOR "TGZ")
  set(CPACK_SOURCE_GENERATOR "TGZ")
  set(CPACK_VERBATIM_VARIABLES ON)

  # Binary package file name carries the platform/processor.
  set(CPACK_PACKAGE_FILE_NAME
    "ilib-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "ilib-${PROJECT_VERSION}")

  # Keep build artifacts, VCS, and editor cruft out of the source tarball.
  set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/" "/\\.github/" "/build.*/" "/\\.vscode/"
    "\\.o$" "\\.a$" "\\.so" "~$"
    "/CLAUDE\\.md$" "/IDEAS\\.md$")

  # Debian package metadata (used when `cpack -G DEB` runs on a Debian host).
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_SECTION "libs")

  include(CPack)
endif()
