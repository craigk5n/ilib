/* Handle validation: the magic-number guards must reject NULL and bogus
   handles with the right IError instead of crashing. */

#include <Ilib.h>
#include "greatest.h"

TEST create_image_succeeds ( void )
{
  IImage im = ICreateImage ( 8, 8, IOPTION_NONE );
  ASSERT ( im != NULL );
  IFreeImage ( im );
  PASS ();
}

TEST create_gc_succeeds ( void )
{
  IGC gc = ICreateGC ();
  ASSERT ( gc != NULL );
  IFreeGC ( gc );
  PASS ();
}

TEST null_image_rejected ( void )
{
  IGC gc = ICreateGC ();
  ASSERT_EQ ( IInvalidImage, IDrawPoint ( NULL, gc, 0, 0 ) );
  IFreeGC ( gc );
  PASS ();
}

TEST bogus_image_rejected ( void )
{
  /* A non-NULL pointer whose leading magic word is not IMAGIC_IMAGE. */
  unsigned int not_an_image = 0;
  IGC gc = ICreateGC ();
  ASSERT_EQ ( IInvalidImage, IDrawPoint ( (IImage) &not_an_image, gc, 0, 0 ) );
  IFreeGC ( gc );
  PASS ();
}

TEST null_gc_rejected ( void )
{
  IImage im = ICreateImage ( 8, 8, IOPTION_NONE );
  ASSERT_EQ ( IInvalidGC, IDrawPoint ( im, NULL, 0, 0 ) );
  IFreeImage ( im );
  PASS ();
}

TEST bogus_gc_rejected ( void )
{
  unsigned int not_a_gc = 0;
  IImage im = ICreateImage ( 8, 8, IOPTION_NONE );
  ASSERT_EQ ( IInvalidGC, IDrawPoint ( im, (IGC) &not_a_gc, 0, 0 ) );
  IFreeImage ( im );
  PASS ();
}

SUITE ( handles )
{
  RUN_TEST ( create_image_succeeds );
  RUN_TEST ( create_gc_succeeds );
  RUN_TEST ( null_image_rejected );
  RUN_TEST ( bogus_image_rejected );
  RUN_TEST ( null_gc_rejected );
  RUN_TEST ( bogus_gc_rejected );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( handles );
  GREATEST_MAIN_END ();
}
