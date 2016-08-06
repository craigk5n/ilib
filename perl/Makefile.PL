use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'	=> 'Ilib',
    'VERSION_FROM' => 'Ilib.pm', # finds $VERSION
    'LIBS'	=> ['-L../ -L/usr/lib/X11 -L/usr/X11R6/lib -lIlib -lgif -lX11 -lpng -ljpeg'],   # e.g., '-lm' 
    'DEFINE'	=> '-DHAVE_GIFLIB -DHAVE_PNGLIB -DHAVE_JPEGLIB',     # e.g., '-DHAVE_SOMETHING' 
    'INC'	=> '-I..',     # e.g., '-I/usr/include/other' 
);
