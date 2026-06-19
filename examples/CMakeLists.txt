# Bundled example programs.

# iconvert: format-conversion tool, links the library directly.
add_executable(iconvert iconvert/iconvert.c)
target_link_libraries(iconvert PRIVATE Ilib::ilib)

# isample: drawing demo. It compiles in two fonts via headers generated from
# BDF files by the ifont2h tool, so it needs that tool to have been built.
if(TARGET ifont2h)
  set(_isample_dir "${CMAKE_CURRENT_BINARY_DIR}/isample_generated")
  file(MAKE_DIRECTORY "${_isample_dir}")
  set(_generated_font_headers)
  foreach(_font helvR24 helvR08)
    set(_bdf "${CMAKE_SOURCE_DIR}/fonts/${_font}.bdf")
    set(_hdr "${_isample_dir}/${_font}.h")
    add_custom_command(
      OUTPUT "${_hdr}"
      COMMAND ifont2h "${_bdf}" > "${_hdr}"
      DEPENDS ifont2h "${_bdf}"
      COMMENT "Generating ${_font}.h from ${_font}.bdf"
      VERBATIM)
    list(APPEND _generated_font_headers "${_hdr}")
  endforeach()

  add_executable(isample isample/isample.c ${_generated_font_headers})
  target_link_libraries(isample PRIVATE Ilib::ilib)
  target_include_directories(isample PRIVATE "${_isample_dir}")
else()
  message(STATUS "ifont2h not built (ILIB_BUILD_CLIENTS=OFF) -- skipping isample")
endif()
