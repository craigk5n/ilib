# Security Policy

Ilib parses untrusted image and font files, so decoder bugs can have security
impact (crashes, out-of-bounds access, memory corruption). Reports are taken
seriously.

## Supported versions

Ilib is maintained on a rolling basis; fixes land on `master` and ship in the
next tagged release. Please test against the latest `master` before reporting.

| Version | Supported          |
|---------|--------------------|
| latest `master` / newest release | :white_check_mark: |
| older releases | :x: |

## Reporting a vulnerability

Please **do not open a public issue** for security problems.

- Preferred: open a private
  [GitHub security advisory](https://github.com/craigk5n/ilib/security/advisories/new).
- Or email **craigk5n@gmail.com** with details.

Include, where possible:

- the affected component (e.g. the GIF/PNG/PPM decoder),
- a minimal input file or steps that reproduce the issue,
- the commit or release you tested,
- any crash output (an AddressSanitizer/UBSan trace is ideal).

You can reproduce decoder issues under sanitizers with:

```bash
cmake -B build -DILIB_BUILD_TESTS=ON -DILIB_SANITIZE=ON
cmake --build build && ctest --test-dir build
```

## What to expect

- Acknowledgement of your report as soon as practical.
- An assessment and, for confirmed issues, a fix on `master` followed by a
  tagged release.
- Credit in the release notes if you would like it.

Thank you for helping keep Ilib and its users safe.
