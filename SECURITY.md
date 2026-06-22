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

## Verifying release artifacts

Each tagged release ships a `SHA256SUMS` file and an SPDX SBOM
(`ilib-<version>.spdx.json`), both signed with [cosign](https://docs.sigstore.dev/)
using keyless (Sigstore/OIDC) signing. The signature bundles are attached as
`*.cosign.bundle`.

Verify the checksums file (then check the tarball against it):

```bash
cosign verify-blob SHA256SUMS \
  --bundle SHA256SUMS.cosign.bundle \
  --certificate-identity-regexp 'https://github.com/craigk5n/ilib/.github/workflows/.+' \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com

sha256sum -c SHA256SUMS
```

The SBOM bundle (`ilib-<version>.spdx.json.cosign.bundle`) verifies the same way.

## Automated scanning

CI runs AddressSanitizer/UBSan, a libFuzzer smoke suite (image decoders + the
BDF font parser), clang-tidy, CodeQL, and a Trivy supply-chain scan; GitHub
Actions are pinned to commit SHAs and tracked by Dependabot.

## What to expect

- Acknowledgement of your report as soon as practical.
- An assessment and, for confirmed issues, a fix on `master` followed by a
  tagged release.
- Credit in the release notes if you would like it.

Thank you for helping keep Ilib and its users safe.
