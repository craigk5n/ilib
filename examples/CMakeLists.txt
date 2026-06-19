# Bundled example programs.

# iconvert: format-conversion tool, links the library directly.
add_executable(iconvert iconvert/iconvert.c)
target_link_libraries(iconvert PRIVATE Ilib::ilib)

# isample: drawing demo. It compiles in two fonts via headers generated from
# BDF files by the ifont2h tool, so it needs that tool to have been built.
if(TARGET ifont2h)
  add_executable(isample isample/isample.c)
  target_link_libraries(isample PRIVATE Ilib::ilib)
  ilib_use_font_headers(isample helvR24 helvR08)
else()
  message(STATUS "ifont2h not built (ILIB_BUILD_CLIENTS=OFF) -- skipping isample")
endif()
