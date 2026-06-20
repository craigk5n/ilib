/* SPDX-License-Identifier: GPL-2.0-only */
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
#include <string.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"


IError IDrawEllipse ( IImage image, IGC gc, int x, int y, int r1, int r2 )
{
  /* A single full sweep so anti-aliased drawing uses the dedicated ellipse
     rasterizer (the result is the same full ellipse either way). */
  return ( IDrawArc ( image, gc, x, y, r1, r2, 0.0, 360.0 ) );
}
