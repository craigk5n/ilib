# Ilib core library

set(ILIB_SOURCES
  IAnimation.c IArcProp.c IAVIF.c IBMP.c IChart.c IColor.c ICompose.c
  IConvolve.c ICopyImg.c
  IDrawArc.c IDrawCir.c IDrawCurve.c IDrawEll.c IDrawLin.c IDrawPol.c IDrawPt.c
  IDrawRec.c IDrawStr.c IErrorStr.c IFileType.c
  IFillArc.c IFillCir.c IFillEll.c IFillPol.c IFillRec.c IFilter.c IFlood.c
  IFont.c IFontBDF.c IFontTTF.c IGC.c IGIF.c IImage.c IJPEG.c
  IPGM.c IPixel.c IPNG.c IPPM.c IQuantize.c IResample.c ITextDim.c ITransform.c
  IWEBP.c IXPM.c)
# NOTE: IXBM.c is intentionally excluded. It has never compiled (an
# unterminated string literal since the initial checkin), its only unique
# symbol _IWriteXBM is dead (XBM is NULL/NULL in the format dispatch table),
# and its other functions (_IReadXPM, hextochar, parse_color) are duplicates
# of IXPM.c. XBM read/write support does not actually exist; see IDEAS.md
# Phase 9 if it is ever wanted.

add_library(ilib ${ILIB_SOURCES})
add_library(Ilib::ilib ALIAS ilib)

set_target_properties(ilib PROPERTIES
  OUTPUT_NAME ilib
  VERSION   "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
  PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Ilib.h"
  C_STANDARD 11)   # K&R definitions are still legal in C11; Phase 3 tightens this

target_include_directories(ilib
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# The library is kept warning-clean under -Wall -Wextra (-Werror in CI).
ilib_set_warnings(ilib)

# --- Optional image-format backends -----------------------------------------
# The sources gate support on HAVE_GIFLIB / HAVE_PNGLIB / HAVE_JPEGLIB and
# include <gif_lib.h> / <png.h> / <jpeglib.h>.
set(_png_enabled OFF)
if(ILIB_WITH_PNG)
  find_package(PNG)
  if(PNG_FOUND)
    target_link_libraries(ilib PRIVATE PNG::PNG)
    target_compile_definitions(ilib PRIVATE HAVE_PNGLIB)
    set(_png_enabled ON)
  else()
    message(STATUS "libpng not found -- PNG support disabled")
  endif()
endif()

set(_jpeg_enabled OFF)
if(ILIB_WITH_JPEG)
  find_package(JPEG)
  if(JPEG_FOUND)
    target_link_libraries(ilib PRIVATE JPEG::JPEG)
    target_compile_definitions(ilib PRIVATE HAVE_JPEGLIB)
    set(_jpeg_enabled ON)
  else()
    message(STATUS "libjpeg not found -- JPEG support disabled")
  endif()
endif()

set(_gif_enabled OFF)
if(ILIB_WITH_GIF)
  find_package(GIF)
  if(GIF_FOUND)
    target_link_libraries(ilib PRIVATE GIF::GIF)
    target_compile_definitions(ilib PRIVATE HAVE_GIFLIB)
    set(_gif_enabled ON)
  else()
    message(STATUS "giflib not found -- GIF support disabled")
  endif()
endif()

# libwebp has no CMake config / Find module on most systems, so locate it via
# pkg-config (libwebp ships a libwebp.pc).
set(_webp_enabled OFF)
if(ILIB_WITH_WEBP)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(WEBP IMPORTED_TARGET libwebp)
    if(WEBP_FOUND)
      target_link_libraries(ilib PRIVATE PkgConfig::WEBP)
      target_compile_definitions(ilib PRIVATE HAVE_WEBPLIB)
      set(_webp_enabled ON)
    else()
      message(STATUS "libwebp not found -- WebP support disabled")
    endif()
  else()
    message(STATUS "pkg-config not found -- WebP support disabled")
  endif()
endif()

# libavif is located via pkg-config (libavif.pc); no CMake Find module.
set(_avif_enabled OFF)
if(ILIB_WITH_AVIF)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(AVIF IMPORTED_TARGET libavif)
    if(AVIF_FOUND)
      target_link_libraries(ilib PRIVATE PkgConfig::AVIF)
      target_compile_definitions(ilib PRIVATE HAVE_AVIFLIB)
      set(_avif_enabled ON)
    else()
      message(STATUS "libavif not found -- AVIF support disabled")
    endif()
  else()
    message(STATUS "pkg-config not found -- AVIF support disabled")
  endif()
endif()

set(_freetype_enabled OFF)
if(ILIB_WITH_FREETYPE)
  find_package(Freetype)
  if(Freetype_FOUND)
    target_link_libraries(ilib PRIVATE Freetype::Freetype)
    target_compile_definitions(ilib PRIVATE HAVE_FREETYPE)
    set(_freetype_enabled ON)
  else()
    message(STATUS "FreeType not found -- TrueType font support disabled")
  endif()
endif()

# Math library (sin/cos/sqrt in the drawing routines) -- separate lib on Linux.
find_library(ILIB_MATH_LIBRARY m)
if(ILIB_MATH_LIBRARY)
  target_link_libraries(ilib PRIVATE "${ILIB_MATH_LIBRARY}")
endif()

# Export enabled-feature state for the top-level summary + pkg-config.
set(ILIB_PNG_ENABLED  ${_png_enabled}  PARENT_SCOPE)
set(ILIB_JPEG_ENABLED ${_jpeg_enabled} PARENT_SCOPE)
set(ILIB_GIF_ENABLED  ${_gif_enabled}  PARENT_SCOPE)
set(ILIB_WEBP_ENABLED ${_webp_enabled} PARENT_SCOPE)
set(ILIB_AVIF_ENABLED ${_avif_enabled} PARENT_SCOPE)
set(ILIB_FREETYPE_ENABLED ${_freetype_enabled} PARENT_SCOPE)

# --- Install ----------------------------------------------------------------
install(TARGETS ilib
  EXPORT IlibTargets
  LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# The charting API ships as a second public header (the PUBLIC_HEADER property
# carries only Ilib.h).
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Ichart.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
