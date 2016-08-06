/*
 * IDrawCir.c
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



IError IDrawCircle ( image, gc, x, y, r )
IImage image;
IGC gc;
int x;
int y;
int r;
{
  return ( IDrawArc ( image, gc, x, y, r, r, 0.0, 360.0 ) );
}



