# Contributing to Ilib

Thanks for your interest in improving Ilib! This document covers how to build,
test, and submit changes.

## Getting started

Ilib builds with CMake (3.16+). Install the optional codec libraries you want
format support for, then:

```bash
cmake -B build -DILIB_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

See the [README](README.md) for dependency package names and build options.

## Development workflow

1. **Branch** off `master` (`git switch -c my-change`).
2. **Write tests** for new behavior or bug fixes. The suite lives in `tests/`
   and uses the vendored [greatest](https://github.com/silentbicycle/greatest)
   framework; decoder/malformed-input tests are the highest-value additions.
3. **Build warning-clean.** CI enforces `-Wall -Wextra -Werror` and a
   `clang-tidy` static-analysis gate. Build locally with `-DILIB_WERROR=ON` to
   catch warnings early.
4. **Run the sanitizers** for memory-touching changes:
   `cmake -B build -DILIB_BUILD_TESTS=ON -DILIB_SANITIZE=ON` then `ctest`.
5. **Open a pull request** against `master`. Keep PRs focused; describe the
   change and how you tested it (the PR template prompts for this).

All pull requests run the full CI matrix (gcc/clang, Linux/macOS), sanitizers,
coverage, a libFuzzer smoke test, and clang-tidy. They must be green to merge.

## Coding conventions

- **Language:** ISO C (C11). Write new function definitions as prototypes, not
  K&R style.
- **Public vs private:** public symbols are prefixed `I`; internal helpers are
  prefixed `_I`. The public API lives in `src/Ilib.h` (the single source of
  truth); internals in `src/IlibP.h`.
- **Opaque handles & magic numbers:** new API functions that take a handle must
  validate its magic field before use and return the matching `IError`
  (`IInvalidImage`, `IInvalidGC`, …) on mismatch.
- **One concern per file:** add a new format/operation as its own `src/I*.c`
  and register it in `src/CMakeLists.txt`.
- **Untrusted input:** decoders parse untrusted files. Validate dimensions and
  allocation sizes, check every allocation, and free everything on error paths.
- **Formatting:** the tree is kept `clang-format`-clean (see `.clang-format`).
  Run `clang-format-14 -i <files>` before committing; CI enforces it with
  clang-format 14 (newer versions may format differently).
- **License header:** start new source files with
  `/* SPDX-License-Identifier: GPL-2.0-only */`.

See [CLAUDE.md guidance in the README](README.md) and the existing sources for
examples.

## Reporting bugs and requesting features

Use the GitHub issue templates. For **security vulnerabilities** (especially in
the image decoders), please follow [SECURITY.md](SECURITY.md) instead of
opening a public issue.

## License

By contributing, you agree that your contributions will be licensed under the
project's [GPL-2.0-only](COPYING) license.
