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

#include "Ilib.h"
#include "IlibP.h"
#include "IColorsP.h"

static int num_colors = 0;
static IColorP **colors = NULL;


static void init_colors ()
{
  IColorP *color;

  colors = (IColorP **) malloc ( 2 * sizeof ( IColorP * ) );

  /* add black as color 0 */
  color = (IColorP *) malloc ( sizeof ( IColorP ) );
  color->red = color->green = color->blue = 0;
  color->value = num_colors++;
  colors[0] = color;

  /* add white as color 1 */
  color = (IColorP *) malloc ( sizeof ( IColorP ) );
  color->red = color->green = color->blue = 255;
  color->value = num_colors++;
  colors[1] = color;
}



IColorP *_IGetColor ( color )
int color;
{
  if ( colors == NULL )
    init_colors ();

  if ( color < 0 || color >= num_colors )
    return ( NULL );

  return ( colors[color] );
}


IColor IAllocColor ( red, green, blue )
unsigned int red, green, blue;
{
  IColorP *color;

  if ( colors == NULL )
    init_colors ();

  if ( red > 255 || green > 255 || blue > 255 ) {
    fprintf ( stderr, "Bad color: %d/%d/%d\n", red, green, blue );
    return ( 0 ); /* black */
  }
  else {
    color = (IColorP *) malloc ( sizeof ( IColorP ) );
    color->magic = IMAGIC_COLOR;
    color->red = red;
    color->green = green;
    color->blue = blue;
    color->value = num_colors++;
    colors  = (IColorP **) realloc ( (void *)colors,
      ( num_colors * sizeof ( IColorP * ) ) );
    colors[color->value] = color;
    return ( color->value );
  }
}


IError _IFreeColor ( color )
IColor color;
{
  IColorP *c = (IColorP *)color;

  if ( color ) {
    c = _IGetColor ( color );
    if ( ! c )
      return ( IInvalidColor );
    if ( c->magic != IMAGIC_COLOR )
      return ( IInvalidColor );
    if ( color >= 0 && color < num_colors && colors[color] ) {
      colors[color]->magic = 0;
      free ( colors[color] );
      colors[color] = NULL;
    }
    else
      return ( IInvalidColor );
  }

  return ( INoError );
}



IError IAllocNamedColor ( colorname, color_ret )
char *colorname;
IColor *color_ret;
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

  for ( loop = 0; loop < I_NUM_NAMED_COLORS && ! found; loop++ ) {
    if ( strcmp ( named_colors[loop].name, cname ) == 0 ) {
      *color_ret = IAllocColor ( named_colors[loop].r,
        named_colors[loop].g, named_colors[loop].b );
      found = 1;
    }
  }

  free ( cname );

  return ( found ? INoError : IInvalidColorName );
}

