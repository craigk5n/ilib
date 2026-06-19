# Apply the project's standard warning flags to a target.
#
#   ilib_set_warnings(<target>)
#
# Enables -Wall -Wextra on GCC/Clang. -Werror is gated on ILIB_WERROR (enabled
# in CI) so that new warnings from future compilers do not break ordinary
# downstream builds.
function(ilib_set_warnings target)
  target_compile_options(${target} PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
    $<$<AND:$<BOOL:${ILIB_WERROR}>,$<C_COMPILER_ID:GNU,Clang,AppleClang>>:-Werror>)
endfunction()
