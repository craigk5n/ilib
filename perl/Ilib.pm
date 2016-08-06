package Ilib;

use strict;
use Carp;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK $AUTOLOAD);

require Exporter;
require DynaLoader;
require AutoLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
	ILIB_URL
	ILIB_VERSION
	ILIB_VERSION_DATE
	ILIB_MAJOR_VERSION
	ILIB_MINOR_VERSION
	ILIB_MICRO_VERSION
	IOPTION_ASCII
	IOPTION_GRAYSCALE
	IOPTION_GREYSCALE
	IOPTION_INTERLACED
	IOPTION_NONE
	IBLACK_PIXEL
	IWHITE_PIXEL
	IFORMAT_GIF
	IFORMAT_PPM
	IFORMAT_PGM
	IFORMAT_PBM
	IFORMAT_XPM
	IFORMAT_XBM
	IFORMAT_PNG
	IFORMAT_JPEG
);
$VERSION = '0.01';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my $constname;
    my $val;
    my $is_str = 1;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "& not defined" if $constname eq 'constant';
    $val = str_constant($constname);
    if ( $val eq "" ) {
      $val = constant($constname, @_ ? $_[0] : 0);
      $is_str = 0;
    }
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
		croak "Your vendor has not defined Ilib macro $constname";
	}
    }
    no strict 'refs';
    #*$AUTOLOAD = sub () { $val };
    if ( $is_str ) {
      eval "sub $AUTOLOAD { \"$val\" }";
    } else {
      eval "sub $AUTOLOAD { $val }";
    }
    goto &$AUTOLOAD;
}

bootstrap Ilib $VERSION;

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is the stub of documentation for your module. You better edit it!

=head1 NAME

Ilib - A library that reads, creates, manipulates and saves images.

=head1 SYNOPSIS

  use Ilib;

  $image = new Ilib::Image ( 640, 480 );
  $gc = new Ilib:GC;
  $font = new Ilib::Font ( "/usr/local/lib/Ilib/fonts/helvR12.bdf" );
  $gc->setFont ( $font );
  $black = new Ilib::Color ( 0, 0, 0 );
  $gc->setForeground ( $black );
  $image->IDrawString ( $gc, 10, 10, "This is sample text",
    strlen ( "This is sample text" ) );
  $image->write ( "/tmp/output.png", I_FORMAT_PNG );

=head1 DESCRIPTION

Ilib is a library (and some tools and examples) written in C
that can read, create, manipulate and save images.  It is capable
of using X11 BDF fonts for drawing text.  That means you get
lots (208, to be exact) of fonts to use.  You can even create your
own if you know how to create an X11 BDF font.  It can read and
write PPM, GIF, PNG and JPG image format.

This Perl module provides a Perl API to access the functions within
Ilib.

The Ilib home page is at:
  http://www.radix.net/~cknudsen/Ilib/

=head1 Exported constants

  ILIB_MAJOR_VERSION
  ILIB_MICRO_VERSION
  ILIB_MINOR_VERSION
  ILIB_URL
  ILIB_VERSION
  ILIB_VERSION_DATE
  INUM_FORMATS
  IOPTION_ASCII
  IOPTION_GRAYSCALE
  IOPTION_GREYSCALE
  IOPTION_INTERLACED
  IOPTION_NONE
  IWHITE_PIXEL
  IBLACK_PIXEL
  IFORMAT_GIF
  IFORMAT_PPM
  IFORMAT_PGM
  IFORMAT_PBM
  IFORMAT_XPM
  IFORMAT_XBM
  IFORMAT_PNG
  IFORMAT_JPEG


=head1 AUTHOR

Craig Knudsen, cknudsen@radix.net, http://www.radix.net/~cknudsen/

=head1 SEE ALSO

perl(1).

=cut
