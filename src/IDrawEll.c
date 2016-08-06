/*
 * IDrawEll.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	19-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"



IError IDrawEllipse ( image, gc, x, y, r1, r2 )
IImage image;
IGC gc;
int x;
int y;
int r1;
int r2;
{
  IError ret;

  ret = IDrawArc ( image, gc, x, y, r1, r2, 0.0, 90.0 );
  if ( ret )
    return ret;
  return ( IDrawArc ( image, gc, x, y, r1, r2, 90.0, 360.0 ) );
}



