# Generate C font headers from the bundled X11 BDF fonts using the ifont2h
# tool, and wire them onto a target.
#
#   ilib_use_font_headers(<target> <font> [<font> ...])
#
# Each <font> names a file fonts/<font>.bdf; ifont2h converts it to
# <build>/generated-fonts/<font>.h (defining <font>_font[]), which the bundled
# clients/examples #include. Each header is generated at most once and shared
# across targets. Requires the ifont2h target to exist.

function(ilib_use_font_headers target)
  if(NOT TARGET ifont2h)
    message(FATAL_ERROR
      "ilib_use_font_headers(${target}) requires the ifont2h target "
      "(ILIB_BUILD_CLIENTS=ON)")
  endif()

  set(_dir "${CMAKE_BINARY_DIR}/generated-fonts")
  file(MAKE_DIRECTORY "${_dir}")

  foreach(_font ${ARGN})
    set(_hdr "${_dir}/${_font}.h")
    set(_bdf "${CMAKE_SOURCE_DIR}/fonts/${_font}.bdf")
    if(NOT EXISTS "${_bdf}")
      message(FATAL_ERROR "ilib_use_font_headers: missing font ${_bdf}")
    endif()
    # Generate each header only once, even across subdirectories.
    if(NOT TARGET ilib_font_${_font})
      add_custom_command(
        OUTPUT "${_hdr}"
        COMMAND $<TARGET_FILE:ifont2h> "${_bdf}" > "${_hdr}"
        DEPENDS ifont2h "${_bdf}"
        COMMENT "Generating font header ${_font}.h"
        VERBATIM)
      add_custom_target(ilib_font_${_font} DEPENDS "${_hdr}")
    endif()
    add_dependencies(${target} ilib_font_${_font})
  endforeach()

  target_include_directories(${target} PRIVATE "${_dir}")
endfunction()
