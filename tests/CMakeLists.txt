# Test suite.
#
# Placeholder for Phase 4 (testing & coverage). The real suite -- round-trip
# format tests, decoder fuzz fixtures, drawing golden-pixel checks, and
# handle-validation tests -- will be added here and registered with CTest via
# add_test(). For now a trivial smoke test confirms the library links.

add_executable(ilib_smoke smoke.c)
target_link_libraries(ilib_smoke PRIVATE Ilib::ilib)
add_test(NAME ilib_smoke COMMAND ilib_smoke)
