# Bundled example programs.

# Installed binaries are prefixed "ilib-" (see clients/CMakeLists.txt); the
# CMake target names keep their historical spelling.

# iconvert: format-conversion tool, links the library directly.
add_executable(iconvert iconvert/iconvert.c)
set_target_properties(iconvert PROPERTIES OUTPUT_NAME ilib-convert)
target_link_libraries(iconvert PRIVATE Ilib::ilib)
ilib_set_warnings(iconvert)

# isample: drawing demo. It compiles in two fonts via headers generated from
# BDF files by the ifont2h tool, so it needs that tool to have been built.
if(TARGET ifont2h)
  add_executable(isample isample/isample.c)
  set_target_properties(isample PROPERTIES OUTPUT_NAME ilib-sample)
  target_link_libraries(isample PRIVATE Ilib::ilib)
  ilib_set_warnings(isample)
  ilib_use_font_headers(isample helvR24 helvR08)
else()
  message(STATUS "ifont2h not built (ILIB_BUILD_CLIENTS=OFF) -- skipping isample")
endif()
