# Ilib tutorial

A short, practical introduction to using the Ilib C API. It mirrors the two
bundled programs in [`examples/`](../examples): `iconvert` (format conversion)
and `isample` (drawing). For the complete reference see the
[Doxygen API docs](https://craigk5n.github.io/ilib/).

## Contents

- [Building and linking](#building-and-linking)
- [Core concepts](#core-concepts)
- [Example 1: converting between formats](#example-1-converting-between-formats)
- [Example 2: drawing an image](#example-2-drawing-an-image)
- [Working with fonts](#working-with-fonts)
- [Error handling](#error-handling)

## Building and linking

Include the single public header and link the library. With pkg-config:

```bash
cc myprog.c $(pkg-config --cflags --libs ilib) -o myprog
```

With CMake:

```cmake
find_package(Ilib REQUIRED)
target_link_libraries(myprog PRIVATE Ilib::ilib)
```

```c
#include <Ilib.h>
```

## Core concepts

- **Opaque handles.** `IImage`, `IGC` (graphics context), `IFont` and `IColor`
  are opaque types you create, use, then free. Never dereference them.
- **Error returns.** Most functions return an `IError` (`INoError` is success);
  `IErrorString()` turns a code into a message. Constructors
  (`ICreateImage`, `ICreateGC`, `IAllocColor`) return a handle or `NULL`/`0`.
- **Free macros.** Release handles with `IFreeImage()`, `IFreeGC()`,
  `IFreeFont()`, `IFreeColor()` — these free the object and null the handle.

## Example 1: converting between formats

Reading an image, then writing it back in another format, is the whole job of
`iconvert`. The format is discovered from the filename with `IFileType()`:

```c
#include <stdio.h>
#include <Ilib.h>

int main(void)
{
    IFileFormat in_fmt, out_fmt;
    IImage image;
    FILE *fp;
    IError ret;

    IFileType("photo.png", &in_fmt);    /* -> IFORMAT_PNG */
    IFileType("photo.gif", &out_fmt);   /* -> IFORMAT_GIF */

    fp = fopen("photo.png", "rb");
    ret = IReadImageFile(fp, in_fmt, IOPTION_NONE, &image);
    fclose(fp);
    if (ret != INoError) {
        fprintf(stderr, "read: %s\n", IErrorString(ret));
        return 1;
    }

    fp = fopen("photo.gif", "wb");
    ret = IWriteImageFile(fp, image, out_fmt, IOPTION_NONE);
    fclose(fp);
    if (ret != INoError)
        fprintf(stderr, "write: %s\n", IErrorString(ret));

    IFreeImage(image);
    return ret == INoError ? 0 : 1;
}
```

Ilib reads PPM, PGM, XPM, GIF, PNG, JPEG and BMP, and writes all of those
except BMP. Always open files in binary mode (`"rb"`/`"wb"`).

## Example 2: drawing an image

`isample` builds an image from scratch: create the canvas, make a graphics
context, allocate colors, set the foreground, then draw. Here is the gist:

```c
#include <stdio.h>
#include <string.h>
#include <Ilib.h>

int main(void)
{
    IImage image = ICreateImage(200, 80, IOPTION_NONE);
    IGC    gc    = ICreateGC();
    IColor blue, white;
    FILE  *fp;

    /* Colors come from RGB triples or X11 color names. */
    blue = IAllocColor(0, 0, 200);
    IAllocNamedColor("white", &white);

    /* Fill the background by drawing with white, then switch to blue. */
    ISetForeground(gc, white);
    IFillRectangle(image, gc, 0, 0, 200, 80);

    ISetForeground(gc, blue);
    IDrawLine(image, gc, 10, 70, 190, 70);
    IDrawRectangle(image, gc, 5, 5, 190, 70);

    fp = fopen("out.ppm", "wb");
    IWriteImageFile(fp, image, IFORMAT_PPM, IOPTION_NONE);
    fclose(fp);

    IFreeColor(blue);
    IFreeColor(white);
    IFreeGC(gc);
    IFreeImage(image);
    return 0;
}
```

The drawing API follows a subset of the X11 graphics functions: points, lines,
rectangles, polygons, circles, arcs and ellipses, each with a `IDraw*` (outline)
and `IFill*` (filled) form, plus `IFloodFill()`.

## Working with fonts

To draw text you need a font in the graphics context. Load a `.bdf` file at
runtime:

```c
IFont font;
ILoadFontFromFile("helv24", "/usr/local/share/ilib/fonts/helvR24.bdf", &font);
ISetFont(gc, font);
IDrawString(image, gc, 10, 40, "Hello", strlen("Hello"));
IFreeFont(font);
```

Run `pkg-config --variable=fontdir ilib` to find where the bundled fonts were
installed. To ship a self-contained binary instead, convert a font to a C
header with [`ifont2h`](man/ifont2h.1) and embed it:

```bash
ifont2h helvR24.bdf > helvR24.h
```

```c
#include "helvR24.h"
IFont font;
ILoadFontFromData("helv24", helvR24_font, &font);
```

## Error handling

Check the `IError` from every call and surface it with `IErrorString()`:

```c
IError ret = IWriteImageFile(fp, image, fmt, IOPTION_NONE);
if (ret != INoError)
    fprintf(stderr, "%s\n", IErrorString(ret));
```

Constructors signal failure differently — they return `NULL` (`ICreateImage`,
`ICreateGC`) or `0` (`IAllocColor`), so check the handle before using it.

## See also

- [`examples/iconvert`](../examples/iconvert) and
  [`examples/isample`](../examples/isample) — the full programs.
- The client tools — [`iindex`](man/iindex.1), [`idisplayfont`](man/idisplayfont.1)
  and others — are additional worked examples.
- [API reference](https://craigk5n.github.io/ilib/).
