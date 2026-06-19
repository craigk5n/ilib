# Test suite (greatest, vendored as greatest.h).
#
# Each concern is a separate test executable registered individually with
# CTest. Splitting them this way means a crash in one group (e.g. a malformed
# image that segfaults a decoder) is reported as that test failing without
# taking down the rest of the suite.

# Tests that only use the public API.
set(ILIB_PUBLIC_TESTS test_color test_text)

# Tests that also read back pixels via the internal layout (pixutil.h ->
# IlibP.h), so they need the private src/ include directory.
set(ILIB_WHITEBOX_TESTS test_handles test_draw test_formats)

foreach(_t ${ILIB_PUBLIC_TESTS} ${ILIB_WHITEBOX_TESTS})
  add_executable(${_t} ${_t}.c)
  target_link_libraries(${_t} PRIVATE Ilib::ilib)
  target_include_directories(${_t} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  # Tests are not subject to -Werror: the vendored greatest.h is third-party
  # and need not satisfy the library's strict warning policy.
  add_test(NAME ${_t} COMMAND ${_t})
endforeach()

# White-box tests reach into src/IlibP.h.
foreach(_t ${ILIB_WHITEBOX_TESTS})
  target_include_directories(${_t} PRIVATE "${CMAKE_SOURCE_DIR}/src")
endforeach()

# The text tests load BDF fonts from the source tree.
target_compile_definitions(test_text PRIVATE
  ILIB_TEST_FONT_DIR="${CMAKE_SOURCE_DIR}/fonts")
