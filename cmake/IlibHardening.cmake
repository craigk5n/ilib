# Optional exploit-mitigation flags, applied to every target in the build.
#
# On by default; disable with -DILIB_HARDENING=OFF. Skipped automatically under
# the sanitizer / coverage / fuzz instrumentation builds, because
# _FORTIFY_SOURCE conflicts with AddressSanitizer's interceptors and requires an
# optimizing build (its glibc guard would emit a #warning under -O0 that
# -Werror then rejects).
#
# Flags (GCC/Clang only):
#   -fstack-protector-strong          stack canaries
#   -Wformat -Wformat-security        flag unsafe format-string usage
#   -D_FORTIFY_SOURCE=2               fortified libc calls (optimized builds)
#   -fPIE / -pie (PIE executables)    via CMAKE_POSITION_INDEPENDENT_CODE
#   -Wl,-z,relro,-z,now               full RELRO (ELF/GNU linkers)

include(CheckCCompilerFlag)

if(ILIB_HARDENING AND NOT ILIB_SANITIZE AND NOT ILIB_COVERAGE
   AND NOT ILIB_FUZZ AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")

  check_c_compiler_flag(-fstack-protector-strong ILIB_HAS_STACK_PROTECTOR)
  if(ILIB_HAS_STACK_PROTECTOR)
    add_compile_options(-fstack-protector-strong)
  endif()

  add_compile_options(-Wformat -Wformat-security)

  # _FORTIFY_SOURCE only helps (and only builds cleanly) with optimization.
  string(TOUPPER "${CMAKE_BUILD_TYPE}" _ilib_build_type)
  if(_ilib_build_type MATCHES "RELEASE|RELWITHDEBINFO|MINSIZEREL")
    add_compile_options(-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2)
  endif()

  # Position-independent executables (the library is already PIC when shared).
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  # Full RELRO + immediate binding. macOS's linker rejects -z, so gate on ELF.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_link_options(-Wl,-z,relro,-z,now)
  endif()

  message(STATUS "Hardening flags enabled (ILIB_HARDENING=ON)")
endif()
