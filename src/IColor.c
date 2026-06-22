/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IImage.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	26-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IAllocNamedColor()
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#include "Ilib.h"
#include "IlibP.h"
#include "IColorsP.h"

static int num_colors = 0;
static IColorP **colors = NULL;


static void init_colors ( void )
{
  IColorP *color;

  colors = (IColorP **) malloc ( 2 * sizeof ( IColorP * ) );
  if ( !colors )
    return;

  /* add black as color 0 */
  color = (IColorP *) malloc ( sizeof ( IColorP ) );
  if ( !color ) {
    free ( colors );
    colors = NULL;
    return;
  }
  color->red = color->green = color->blue = 0;
  color->alpha = 255;
  color->value = num_colors++;
  colors[0] = color;

  /* add white as color 1 */
  color = (IColorP *) malloc ( sizeof ( IColorP ) );
  if ( !color ) {
    free ( colors[0] );
    free ( colors );
    colors = NULL;
    num_colors = 0;
    return;
  }
  color->red = color->green = color->blue = 255;
  color->alpha = 255;
  color->value = num_colors++;
  colors[1] = color;
}


IColorP *_IGetColor ( int color )
{
  if ( colors == NULL )
    init_colors ();

  if ( colors == NULL || color < 0 || color >= num_colors )
    return ( NULL );

  return ( colors[color] );
}


IColor IAllocColorAlpha ( unsigned int red, unsigned int green,
  unsigned int blue, unsigned int alpha )
{
  IColorP *color;

  if ( colors == NULL )
    init_colors ();
  if ( colors == NULL )
    return ( 0 ); /* black; out of memory */

  if ( red > 255 || green > 255 || blue > 255 || alpha > 255 ) {
    fprintf ( stderr, "Bad color: %d/%d/%d/%d\n", red, green, blue, alpha );
    return ( 0 ); /* black */
  }
  else {
    int slot, i;
    color = (IColorP *) malloc ( sizeof ( IColorP ) );
    if ( !color )
      return ( 0 ); /* black */
    color->magic = IMAGIC_COLOR;
    color->red = red;
    color->green = green;
    color->blue = blue;
    color->alpha = alpha;

    /* Reuse a freed slot if one exists so repeated alloc/free cycles do not
       grow the table without bound. Otherwise append (growing the array). */
    slot = -1;
    for ( i = 0; i < num_colors; i++ ) {
      /* False positive: the analyzer models the malloc'd table slots as
         uninitialized, but every index < num_colors has been written. */
      /* NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult) */
      if ( colors[i] == NULL ) {
        slot = i;
        break;
      }
    }
    if ( slot < 0 ) {
      IColorP **new_colors = (IColorP **) realloc ( (void *) colors,
        ( (size_t) num_colors + 1 ) * sizeof ( IColorP * ) );
      if ( !new_colors ) {
        free ( color );
        return ( 0 ); /* black */
      }
      colors = new_colors;
      slot = num_colors++;
    }
    color->value = slot;
    colors[slot] = color;
    return ( color->value );
  }
}

IColor IAllocColor ( unsigned int red, unsigned int green, unsigned int blue )
{
  return ( IAllocColorAlpha ( red, green, blue, 255 ) );
}


IError _IFreeColor ( IColor color )
{
  IColorP *c = (IColorP *) (intptr_t) color;

  if ( color ) {
    c = _IGetColor ( color );
    if ( !c )
      return ( IInvalidColor );
    if ( c->magic != IMAGIC_COLOR )
      return ( IInvalidColor );
    if ( color < (unsigned int) num_colors && colors[color] ) {
      colors[color]->magic = 0;
      free ( colors[color] );
      colors[color] = NULL;
    }
    else
      return ( IInvalidColor );
  }

  return ( INoError );
}


IError IAllocNamedColor ( char *colorname, IColor *color_ret )
{
  int loop;
  char *cname, *ptr, *ptr2;
  int found = 0;

  cname = (char *) malloc ( strlen ( colorname ) + 1 );
  for ( ptr = colorname, ptr2 = cname; *ptr != '\0'; ptr++ ) {
    if ( *ptr != ' ' ) {
      *ptr2 = tolower ( *ptr );
      ptr2++;
    }
  }
  *ptr2 = '\0';

  for ( loop = 0; loop < I_NUM_NAMED_COLORS && !found; loop++ ) {
    if ( strcmp ( named_colors[loop].name, cname ) == 0 ) {
      *color_ret = IAllocColor ( named_colors[loop].r,
        named_colors[loop].g, named_colors[loop].b );
      found = 1;
    }
  }

  free ( cname );

  return ( found ? INoError : IInvalidColorName );
}
