/*
 * Minimal smoke test: confirms the library links and the core
 * image/GC/color/draw lifecycle works. Replaced by a real suite in Phase 4.
 */

#include <stdio.h>
#include <stdlib.h>

#include <Ilib.h>

int main ( void )
{
  IImage image;
  IGC    gc;
  IColor red;

  image = ICreateImage ( 16, 16, IOPTION_NONE );
  if ( ! image ) {
    fprintf ( stderr, "ICreateImage failed\n" );
    return 1;
  }

  gc = ICreateGC ();
  if ( ! gc ) {
    fprintf ( stderr, "ICreateGC failed\n" );
    return 1;
  }

  red = IAllocColor ( 255, 0, 0 );
  ISetForeground ( gc, red );

  if ( IDrawLine ( image, gc, 0, 0, 15, 15 ) != INoError ) {
    fprintf ( stderr, "IDrawLine failed\n" );
    return 1;
  }

  IFreeColor ( red );
  IFreeGC ( gc );
  IFreeImage ( image );

  printf ( "ilib smoke test passed\n" );
  return 0;
}
