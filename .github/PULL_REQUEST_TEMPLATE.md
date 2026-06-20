<!-- Thanks for contributing to Ilib! Please keep PRs focused. -->

## Summary

<!-- What does this change do, and why? -->

## Changes

<!-- Bullet the notable changes. -->
-

## Testing

<!-- How did you verify this? Check what applies. -->
- [ ] `cmake -B build -DILIB_BUILD_TESTS=ON -DILIB_WERROR=ON` builds clean
- [ ] `ctest --test-dir build` passes
- [ ] Added/updated tests for the change
- [ ] Ran sanitizers (`-DILIB_SANITIZE=ON`) for memory-touching changes

## Notes

- [ ] Public API in `Ilib.h` is unchanged, or the change is source-compatible
- [ ] New source files start with an `SPDX-License-Identifier` header
