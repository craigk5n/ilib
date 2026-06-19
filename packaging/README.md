# Downstream packaging notes

Ilib builds with CMake and installs a standard layout:

| Component            | Installed location                         |
|----------------------|--------------------------------------------|
| Shared/static lib    | `<prefix>/lib`                             |
| Public header        | `<prefix>/include/Ilib.h`                  |
| CMake package config | `<prefix>/lib/cmake/Ilib/`                 |
| pkg-config file      | `<prefix>/lib/pkgconfig/ilib.pc`           |
| Bundled BDF fonts    | `<prefix>/share/ilib/fonts/`               |

The library carries a `SOVERSION` equal to its major version, so ABI-breaking
releases bump the SONAME. Optional codecs (libpng, libjpeg, giflib) are
auto-detected; a missing codec is skipped, and the enabled set is reflected in
both `IlibConfig.cmake` (`find_dependency`) and `ilib.pc` (`Libs.private`).

These notes are a starting point — the actual recipes are maintained in the
respective ecosystem repositories, not here.

## Homebrew

A formula can build straight from a release tarball:

```ruby
class Ilib < Formula
  desc "C library to read, create, manipulate and save raster images"
  homepage "https://www.k5n.us/Ilib.php"
  url "https://github.com/craigk5n/ilib/releases/download/v1.1.10/ilib-1.1.10.tar.gz"
  # sha256 from the release SHA256SUMS
  license "GPL-2.0-only"

  depends_on "cmake" => :build
  depends_on "giflib"
  depends_on "jpeg"
  depends_on "libpng"

  def install
    system "cmake", "-B", "build", *std_cmake_args,
           "-DILIB_BUILD_CLIENTS=OFF", "-DILIB_BUILD_EXAMPLES=OFF"
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    system "pkg-config", "--exists", "ilib"
  end
end
```

## Debian / Ubuntu

CPack can emit a `.deb` directly on a Debian host:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
( cd build && cpack -G DEB )   # CPACK_DEBIAN_PACKAGE_SHLIBDEPS resolves deps
```

For an official package, a `debian/` directory using `debhelper` + `dh_cmake`
is preferred:

- `Build-Depends: cmake, debhelper-compat (= 13), libpng-dev, libjpeg-dev, libgif-dev`
- Split runtime (`libilib1`) and development (`libilib-dev`) packages along the
  install layout above; the `.so.MAJOR` symlink belongs in the runtime package
  and the bare `.so`, headers, `.pc`, and CMake config in `-dev`.

## vcpkg

A port (`ports/ilib/portfile.cmake`) follows the standard pattern:

```cmake
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO craigk5n/ilib
  REF "v${VERSION}"
  SHA512 <sha512-of-source-tarball>
)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS -DILIB_BUILD_CLIENTS=OFF -DILIB_BUILD_EXAMPLES=OFF)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME Ilib CONFIG_PATH lib/cmake/Ilib)
vcpkg_fixup_pkgconfig()
```

Declare the optional codecs (`libpng`, `libjpeg-turbo`, `giflib`) as
dependencies in `vcpkg.json`.
