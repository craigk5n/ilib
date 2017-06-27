# Ilib
Copyright (C) 2001-2016 Craig Knudsen, craig@k5n.us
http://www.k5n.us/Ilib.php

Ilib is a library (and some tools and examples) written in C
that can read, create, manipulate and save images.  It is capable
of using X11 BDF fonts for drawing text.  That means you get
lots (208, to be exact) of fonts to use.  You can even create your
own if you know how to create an X11 BDF font.  It can read and
write PPM, XPM, GIF, PNG and JPG image format.  It can read (but not
yet write) BMP.

Note: This code was originally developed in the late 1990s so some the C source
code was written for "classic" C before ANSI C was standard everywhere.
So functions are defined differently (not ANSI-style).

## API Documentation
The API documentation is generated from the source code and can be
found in Ilib.html.  You can view it online
[here](http://www.k5n.us/Ilib.php?topic=API).
Note that the API is modeled after a subset of the
[X11 API drawing functions](https://www.x.org/releases/X11R7.6/doc/libX11/specs/libX11/libX11.html#graphics_functions).


## Perl Module
The Ilib perl module is now included with the distribution.  It is not
compiled or installed by default.  This perl module builds like all other
perl modules.  AFTER installing the rest of Ilib (see instructions below),
do the following:
```
  cd perl
  perl Makefile.PL
  make
  make install
```
(Normally, you will need to be root to install.)


## Extras
- Giflib: [http://giflib.sourceforge.net/]
- LibPNG: [http://www.libpng.org/pub/png/libpng.html]
- libjpeg: [http://libjpeg.sourceforge.net/]
- Fonts:
  - X11 BDF fonts: You can find these in a number of places:
    - [Google search](https://www.google.com/search?q=timR24.bdf)
    - [Apple](https://opensource.apple.com/source/X11fonts/X11fonts-10.2/font-adobe-100dpi/font-adobe-100dpi-X11R7.0-1.0.0/)
  - Edit/Import/Create BDF fonts with 3rd party tools.  Use (google)[https://www.google.com/search?q=bdf+font+editor] to find them.

## Compiling (Mac):

Install giflib, libjpeg and libpng if you want to be able to
read or write images in those formats.
The easiest way to do this is with brew:

```
brew install giflib
brew install libjpeg
brew install libpng
```
Edit Makefile and change DYNAMIC to NO. Then...
```
make makefiles
make -i all
```
You'll get errors on not being able to find the shared libraries since
the makefiles are still somewhat broken for Mac.  But this will build
the static libIlib.a file.  You can compile some of the sample client
code using ```make static``` in each example client's directory.

## Compiling (Linux, Unix):

In order to make use of GIF, PNG or JPEG, you need to obtain the
add-on libraries mentioned above.  (First, check your system.  If
you have a Linux distribution, it's likely to have some of these
installed.)

To do so on ubuntu:

    yum install libjpeg-devel libpng-devel libgif-devel giflib-devel

Edit the definitions of LIBS, DEFINES, INCLUDES to indicate which
libraries are installed.  Change PREFIX if you don't want to install
in /usr/local.

Also, edit the values of CC and RANLIB if needed.

Then, just type "make makefiles; make all" to build everthing.
Both a shared and static library will be buily in the "lib" directory.
Type "make install" to install everything (defaults to /usr/local).

Look at the programs in "examples" and "clients" as examples how to use
Ilib.  The example program "iconvert" shows a handy tool for converting
images between different formats and "isample" shows how to draw text,
lines, etc.

## Compiling (Win32):

You should be able to use the Cygnus Win32 package to build Ilib using the
provided makefiles.  I have not tested this since Ilib v1.0.

Ilib-1.1.0 was built on Win95 with MS Visual Developer (Visual
C++).  GIFLIB was also built with MS Visual Developer.  Sorry, I'm not
going to try and provide makefiles or project files for this.

## TODO
- Automake support, obviously...
