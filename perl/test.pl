# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; print "1..1\n"; }
END {print "not ok 1\n" unless $loaded;}
use Ilib;
$loaded = 1;
print "ok 1\n";

######################### End of black magic.

# Insert your test code below (better if it prints "ok 13"
# (correspondingly "not ok 13") depending on the success of chunk 13
# of the test code):

printf "Ilib Version %s (%s)\n", ILIB_VERSION, ILIB_VERSION_DATE;

$width = 500;
$height = 150;
$fontname1 = "helvR24";
$fontpath1 = "../../fonts/helvR24.bdf";

$image = Ilib::ICreateImage ( $width, $height, IOPTION_GREYSCALE );
open ( F,  ">out.gif" );

$gc = Ilib::ICreateGC ();
$background = Ilib::IAllocNamedColor ( "gray" );
$topshadow = Ilib::IAllocNamedColor ( "lightgrey" );
$bottomshadow = Ilib::IAllocNamedColor ( "darkgrey" );
$textcolor = Ilib::IAllocNamedColor ( "yellow" );
$white = Ilib::IAllocNamedColor ( "white" );
$black = Ilib::IAllocNamedColor ( "black" );

Ilib::ISetBackground ( $gc, $background );

# draw top shadow rectangle
Ilib::ISetForeground ( $gc, $topshadow );
Ilib::IFillRectangle ( $image, $gc, 2, 2, $width - 2, $height - 2 );

# draw bottom shadow rectangle
Ilib::ISetForeground ( $gc, $bottomshadow );
Ilib::IFillRectangle ( $image, $gc, 2, 2, $width - 2, $height - 2 );

# draw background rectangle
Ilib::ISetForeground ( $gc, $background );
Ilib::IFillRectangle ( $image, $gc, 2, 2, $width - 4, $height - 4 );

# load font
Ilib::ILoadFontFromFile ( $fontname1, $fontpath1, \$largefont );

# write output file
Ilib::IWriteImageFile ( F, $image, IFORMAT_GIF, IOPTION_NONE );

exit 0;
