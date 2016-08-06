/*
 * IXBM.c
 *
 * Image library
 *
 * Description:
 *	Read/write XBM files.
 *
 * History:
 *	27-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>

#include "Ilib.h"
#include "IlibP.h"


IError _IWriteXBM ( fp, image, options )
FILE *fp;
IImageP *image;
IOptions options;
{
  int r, c;
  unsigned char red, green, blue;
  int val;
  int bit;
  int numchars = 0;

  fprintf ( fp, "#define image_width %d\n", image->width );
  fprintf ( fp, "#define image_height %d\n", image->height );
  fprintf ( fp, "static char image_bits[] = {\n" );

  bit = 1;
  num_chars = 0;
  for ( r = 0; r < image->height; r++ ) {
    for ( c = 0; c < image->width; c++, bit++ ) {
      if ( bit > 8 )
        bit = 1;
      offset = ( r * image->width ) + c;
      if ( image->greyscale ) {
        ptr = image->data + ( r * image->width ) + c;
        red = green = blue = (unsigned int) *ptr;
      }
      else {
        ptr = image->data + ( r * image->width * 3 ) + ( c * 3 );
        red = (unsigned int) *ptr;
        green = (unsigned int) *( ptr + 1 );
        blue = (unsigned int) *( ptr + 2 );
      }
      if ( (int)red + (int)green + (int)blue > 384 ) {
        val |= ( 0x01 << bit );
      }
      if ( bit == 8 ) {
        if ( numchars )
          fprintf ( fp, ", " );
        numchars++;
        if ( numchars == 12 ) {
          fprintf ( fp, "\n   " );
          numchars = 0;
        }
        fprintf ( fp, "0x%02X" val );
      }
    }
    if ( bit < 9 ) {
      fprintf ( fp, ", 0x
    }
  }

  fprintf ( fp, "/* colors */\n" );
  for ( loop = 0; loop < num_colors; loop++ ) {
    if ( image->transparent != NULL &&
      ( image->transparent->red == colormap[loop]->red &&
      image->transparent->green == colormap[loop]->green &&
      image->transparent->blue == colormap[loop]->blue ) )
      strcpy ( colorstr, "None" );
    else
      sprintf ( colorstr, "#%02X%02X%02X", colormap[loop]->red,
        colormap[loop]->green, colormap[loop]->blue );
    switch ( chars_per_pixel ) {
      case 1:
        fprintf ( fp, "\"%c c %s\",\n", xpm_chars[loop], colorstr );
        break;
      case 2:
        fprintf ( fp, "\"%c%c c %s\",\n",
          xpm_chars[loop/strlen(xpm_chars)],
          xpm_chars[loop%strlen(xpm_chars)], colorstr );
        break;
    }
  }

  fprintf ( fp, "/* pixels */\n" );
  for ( loop = 0; loop < image->height; loop++ ) {
    fprintf ( fp, "\"" );
    for ( loop2 = 0; loop2 < image->width; loop2++ ) {
      switch ( chars_per_pixel ) {
        case 1:
          fprintf ( fp, "%c", xpm_chars[data[loop*image->width+loop2]] );
          break;
        case 2:
          fprintf ( fp, "%c%c",
            xpm_chars[data[loop*image->width+loop2]/strlen(xpm_chars)],
            xpm_chars[data[loop*image->width+loop2]%strlen(xpm_chars)] );
          break;
      }
    }
    fprintf ( fp, "\",\n" );
  }

  fprintf ( fp, "};\n" );

  /* free up allocated resources */
  free ( data );
  for ( loop = 0; loop < num_colors; loop++ ) {
    free ( colormap[loop] );
  }

  return ( INoError );
}



static unsigned char hextochar ( hex )
char hex;
{
  switch ( toupper ( hex ) ) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
    case 'E': return 14;
    case 'F': return 15;
  }
  return 0;
}



static int parse_color ( str, r, g, b, is_transparent )
char *str;
unsigned char *r, *g, *b;
int *is_transparent;
{
  char *ptr;
  int w;

  for ( ptr = str; *ptr != '\0'; ptr++ )
    *ptr = tolower ( *ptr );

  *is_transparent = 0;

  if ( *str == '#' ) {
    ptr = str + 1;
    w = strlen ( str ) / 3;
    *r = hextochar ( ptr[0] ) * 16 + hextochar ( ptr[1] );
    ptr += w;
    *g = hextochar ( ptr[0] ) * 16 + hextochar ( ptr[1] );
    ptr += w;
    *b = hextochar ( ptr[0] ) * 16 + hextochar ( ptr[1] );
    return 0;
  } else if ( strcmp ( str, "none" ) == 0 ||
    strcmp ( str, "transparent" ) == 0 ) {
    *r = *g = *b = 254; /* don't make white since it might already exist */
    *is_transparent = 1;
    return 0;
  } else {
    /* name like "blue" */
    return 1;
  }
}



typedef struct {
  char *abbrev;
  unsigned char r;
  unsigned char g;
  unsigned char b;
  int transparent;
} xpmcolor;



IError _IReadXPM ( fp, options, image_return )
FILE *fp;
IOptions options;
IImageP **image_return;
{
  IImageP *image = NULL;
  char line[1024], *ptr, *r, *g, *b, *cur;
  xpmcolor *colors = NULL;
  int w, h, num_colors, colorw, loop, loop2, loop3;
  int found;
  IColor t;

  /* look for first line that starts with " */
  while ( fgets ( line, 1024, fp ) ) {
    if ( line[0] == '"' )
      break;
  }
  if ( line[0] != '"' )
    return IInvalidFormat;

  ptr = strtok ( line + 1, " " );
  if ( ! ptr )
    return IInvalidFormat;
  w = atoi ( ptr );
  ptr = strtok ( NULL, " " );
  if ( ! ptr )
    return IInvalidFormat;
  h = atoi ( ptr );
  ptr = strtok ( NULL, " " );
  if ( ! ptr )
    return IInvalidFormat;
  num_colors = atoi ( ptr );
  ptr = strtok ( NULL, " " );
  if ( ! ptr )
    return IInvalidFormat;
  colorw = atoi ( ptr );

  colors = (xpmcolor *) malloc ( sizeof ( xpmcolor ) * num_colors );

  for ( loop = 0; loop < num_colors; loop++ ) {
    while ( 1 ) {
      if ( ! fgets ( line, 1024, fp ) )
        return IInvalidFormat; /* small memory leak */
      if ( line[0] == '"' )
        break;
    }
    colors[loop].abbrev = (char *) malloc ( colorw + 1 );
    strncpy ( colors[loop].abbrev, line + 1, colorw );
    colors[loop].abbrev[colorw] = '\0';
    for ( ptr = line + 1 + colorw; *ptr != 'c' && *ptr != '\0'; ptr++ ) ;
    if ( *ptr != 'c' )
      return IInvalidFormat; /* small memory leak */
    do {
      ptr++;
    } while ( *ptr == ' ' );
    strtok ( ptr, "\"" );
    /* parse color */
    if ( parse_color ( ptr, &colors[loop].r, &colors[loop].g, &colors[loop].b,
      &colors[loop].transparent ) )
      return IInvalidFormat; /* small memory leak */
  }

  image = (IImageP *) ICreateImage ( w, h, options );

  /* now read row by row of data */
  ptr = (char *) malloc ( colorw * w + 20 );
  for ( loop = 0; loop < h; loop++ ) {
    if ( ! fgets ( ptr, colorw * w + 20, fp ) )
      return IInvalidFormat; /* small memory leak */
    while ( ptr[0] != '"' ) {
      if ( ! fgets ( ptr, colorw * w + 20, fp ) )
        return IInvalidFormat; /* small memory leak */
    }
    for ( loop2 = 0; loop2 < w; loop2++ ) {
      cur = ptr + 1 + loop2 * colorw;
      /* find color in colormap */
      for ( loop3 = 0, found = 0; loop3 < num_colors && ! found; loop3++ ) {
        if ( strncmp ( colors[loop3].abbrev, cur, colorw ) == 0 ) {
          r = image->data + ( loop * w * 3 ) + ( loop2 * 3 );
          g = r + 1;
          b = r + 2;
          *r = colors[loop3].r;
          *g = colors[loop3].g;
          *b = colors[loop3].b;
          found = 1;
        }
      }
      if ( ! found )
        return IInvalidFormat; /* small memory leak */
    }
  }
  free ( ptr );

  *image_return = image;

  for ( loop = 0; loop < num_colors; loop++ ) {
    if ( colors[loop].transparent ) {
      t = IAllocColor ( colors[loop].r, colors[loop].g, colors[loop].b );
      ISetTransparent ( image, t );
    }
    free ( colors[loop].abbrev );
  }
  free ( colors );

  return ( INoError );
}

