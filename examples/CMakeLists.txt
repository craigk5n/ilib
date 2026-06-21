# Bundled example programs.

# Installed binaries are prefixed "ilib-" (see clients/CMakeLists.txt); the
# CMake target names keep their historical spelling.

# iconvert: format-conversion + image-operation tool, links the library
# directly. It calls fmod() for the --rotate option, so it needs libm too.
add_executable(iconvert iconvert/iconvert.c)
set_target_properties(iconvert PROPERTIES OUTPUT_NAME ilib-convert)
target_link_libraries(iconvert PRIVATE Ilib::ilib)
find_library(ILIB_EXAMPLE_MATH_LIBRARY m)
if(ILIB_EXAMPLE_MATH_LIBRARY)
  target_link_libraries(iconvert PRIVATE "${ILIB_EXAMPLE_MATH_LIBRARY}")
endif()
ilib_set_warnings(iconvert)

# isample: drawing demo. It compiles in two fonts via headers generated from
# BDF files by the ifont2h tool, so it needs that tool to have been built.
if(TARGET ifont2h)
  add_executable(isample isample/isample.c)
  set_target_properties(isample PROPERTIES OUTPUT_NAME ilib-sample)
  target_link_libraries(isample PRIVATE Ilib::ilib)
  ilib_set_warnings(isample)
  ilib_use_font_headers(isample helvR24 helvR08)

  # ichart: charting demo (line/bar/pie + montage). Also compiles in a font.
  add_executable(ichart ichart/ichart.c)
  set_target_properties(ichart PROPERTIES OUTPUT_NAME ilib-chart)
  target_link_libraries(ichart PRIVATE Ilib::ilib)
  ilib_set_warnings(ichart)
  ilib_use_font_headers(ichart helvR08)
else()
  message(STATUS "ifont2h not built (ILIB_BUILD_CLIENTS=OFF) -- skipping isample/ichart")
endif()
