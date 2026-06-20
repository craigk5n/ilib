/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IXPM.c
 *
 * Image library
 *
 * Description:
 *	Read and write XPM files.
 *	The reading of XPM files is not all that robust.  It handles
 *	XPMs generated from xv and The GIMP, but it does not handle
 *	the use of X11 rgb names (like "grey2") in the XPM file.
 *
 *	These routines are terribly slow, but hey, if you're using XPM
 *	you're obviously not that concerned about speed ;-)
 *
 * History:
 *	27-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Updated to read XPM and also handle transparency
 *			when writing.
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include "Ilib.h"
#include "IlibP.h"

#define colors_match( color, r, g, b ) \
  ( ( color )->red == red && ( color )->green == green && ( color )->blue == blue )

#define MAX_COLORMAP_SIZE ( 256 * 256 )


IError _IWriteXPM ( FILE *fp, IImageP *image, IOptions options )
{
  int r, c;
  unsigned char *ptr;
  unsigned int red, green, blue;
  IColorP *colormap[MAX_COLORMAP_SIZE];
  int num_colors = 0;
  int loop, loop2;
  int offset;
  (void) options;
  unsigned short *data;
  int color_found, chars_per_pixel;
  static char *xpm_chars =
    ".#abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  char colorstr[20];

  data = (unsigned short *) calloc ( image->width * image->height,
    sizeof ( unsigned short ) );
  memset ( data, '\0', image->width * image->height * sizeof ( short ) );

  for ( r = 0; r < image->height; r++ ) {
    for ( c = 0; c < image->width; c++ ) {
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
      color_found = 0;
      for ( loop = 0; loop < num_colors; loop++ ) {
        if ( colors_match ( colormap[loop], red, green, blue ) ) {
          data[offset] = loop;
          color_found = 1;
          break;
        }
      }
      if ( !color_found ) {
        if ( num_colors < MAX_COLORMAP_SIZE ) {
          colormap[num_colors] = (IColorP *) malloc ( sizeof ( IColorP ) );
          colormap[num_colors]->red = red;
          colormap[num_colors]->green = green;
          colormap[num_colors]->blue = blue;
          data[offset] = num_colors;
          num_colors++;
        }
      }
    }
  }

  fprintf ( fp, "/* XPM */\n" );
  if ( image->comments )
    fprintf ( fp, "/* %s */\n", image->comments );
  fprintf ( fp, "static char *image[] = {\n" );
  fprintf ( fp, "/* width height num_colors chars_per_pixel */\n" );
  if ( (size_t) num_colors > strlen ( xpm_chars ) )
    chars_per_pixel = 2;
  else
    chars_per_pixel = 1;
  fprintf ( fp, "\"%d %d %d %d\",\n", image->width, image->height,
    num_colors, chars_per_pixel );

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
        xpm_chars[loop / strlen ( xpm_chars )],
        xpm_chars[loop % strlen ( xpm_chars )], colorstr );
      break;
    }
  }

  fprintf ( fp, "/* pixels */\n" );
  for ( loop = 0; loop < image->height; loop++ ) {
    fprintf ( fp, "\"" );
    for ( loop2 = 0; loop2 < image->width; loop2++ ) {
      switch ( chars_per_pixel ) {
      case 1:
        fprintf ( fp, "%c", xpm_chars[data[loop * image->width + loop2]] );
        break;
      case 2:
        fprintf ( fp, "%c%c",
          xpm_chars[data[loop * image->width + loop2] / strlen ( xpm_chars )],
          xpm_chars[data[loop * image->width + loop2] % strlen ( xpm_chars )] );
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


static unsigned char hextochar ( char hex )
{
  switch ( toupper ( hex ) ) {
  case '0':
    return 0;
  case '1':
    return 1;
  case '2':
    return 2;
  case '3':
    return 3;
  case '4':
    return 4;
  case '5':
    return 5;
  case '6':
    return 6;
  case '7':
    return 7;
  case '8':
    return 8;
  case '9':
    return 9;
  case 'A':
    return 10;
  case 'B':
    return 11;
  case 'C':
    return 12;
  case 'D':
    return 13;
  case 'E':
    return 14;
  case 'F':
    return 15;
  }
  return 0;
}


static int parse_color ( char *str, unsigned char *r, unsigned char *g, unsigned char *b, int *is_transparent )
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
  }
  else if ( strcmp ( str, "none" ) == 0 ||
            strcmp ( str, "transparent" ) == 0 ) {
    *r = *g = *b = 254; /* don't make white since it might already exist */
    *is_transparent = 1;
    return 0;
  }
  else {
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


IError _IReadXPM ( FILE *fp, IOptions options, IImageP **image_return )
{
  IImageP *image = NULL;
  char line[1024], *ptr, *r, *g, *b, *cur;
  char *rowbuf = NULL;
  xpmcolor *colors = NULL;
  int w, h, num_colors = 0, colorw, loop, loop2, loop3;
  int found;
  IColor t;
  IError ret;

  /* look for first line that starts with " */
  while ( fgets ( line, 1024, fp ) ) {
    if ( line[0] == '"' )
      break;
  }
  if ( line[0] != '"' )
    goto fail;

  ptr = strtok ( line + 1, " " );
  if ( !ptr )
    goto fail;
  w = atoi ( ptr );
  ptr = strtok ( NULL, " " );
  if ( !ptr )
    goto fail;
  h = atoi ( ptr );
  ptr = strtok ( NULL, " " );
  if ( !ptr )
    goto fail;
  num_colors = atoi ( ptr );
  ptr = strtok ( NULL, " " );
  if ( !ptr )
    goto fail;
  colorw = atoi ( ptr );

  /* Validate the untrusted header before any file-driven allocation:
     positive values, sane caps, and no integer overflow in the image size
     (w*h*3) or the per-row buffer (colorw*w + 20). */
  if ( w <= 0 || h <= 0 )
    goto fail;
  if ( num_colors <= 0 || num_colors > ( 1 << 20 ) )
    goto fail;
  if ( colorw <= 0 || colorw > 32 )
    goto fail;
  if ( w > INT_MAX / 3 / h )
    goto fail;
  if ( w > ( INT_MAX - 20 ) / colorw )
    goto fail;

  /* calloc so every abbrev pointer starts NULL -> the cleanup can free them
     all unconditionally even on a mid-loop failure */
  colors = (xpmcolor *) calloc ( num_colors, sizeof ( xpmcolor ) );
  if ( !colors )
    goto fail;

  for ( loop = 0; loop < num_colors; loop++ ) {
    while ( 1 ) {
      if ( !fgets ( line, 1024, fp ) )
        goto fail;
      if ( line[0] == '"' )
        break;
    }
    colors[loop].abbrev = (char *) malloc ( colorw + 1 );
    /* copy exactly colorw bytes (line is a 1024-byte buffer, colorw <= 32),
       then terminate; memcpy avoids -Wstringop-truncation from strncpy */
    memcpy ( colors[loop].abbrev, line + 1, colorw );
    colors[loop].abbrev[colorw] = '\0';
    for ( ptr = line + 1 + colorw; *ptr != 'c' && *ptr != '\0'; ptr++ )
      ;
    if ( *ptr != 'c' )
      goto fail;
    do {
      ptr++;
    } while ( *ptr == ' ' );
    strtok ( ptr, "\"" );
    /* parse color */
    if ( parse_color ( ptr, &colors[loop].r, &colors[loop].g, &colors[loop].b,
           &colors[loop].transparent ) )
      goto fail;
  }

  image = (IImageP *) ICreateImage ( w, h, options );
  if ( !image )
    goto fail;

  /* now read row by row of data */
  rowbuf = (char *) malloc ( colorw * w + 20 );
  if ( !rowbuf )
    goto fail;
  for ( loop = 0; loop < h; loop++ ) {
    if ( !fgets ( rowbuf, colorw * w + 20, fp ) )
      goto fail;
    while ( rowbuf[0] != '"' ) {
      if ( !fgets ( rowbuf, colorw * w + 20, fp ) )
        goto fail;
    }
    for ( loop2 = 0; loop2 < w; loop2++ ) {
      cur = rowbuf + 1 + loop2 * colorw;
      /* find color in colormap */
      for ( loop3 = 0, found = 0; loop3 < num_colors && !found; loop3++ ) {
        if ( strncmp ( colors[loop3].abbrev, cur, colorw ) == 0 ) {
          r = (char *) image->data + ( loop * w * 3 ) + ( loop2 * 3 );
          g = r + 1;
          b = r + 2;
          *r = colors[loop3].r;
          *g = colors[loop3].g;
          *b = colors[loop3].b;
          found = 1;
        }
      }
      if ( !found )
        goto fail;
    }
  }
  for ( loop = 0; loop < num_colors; loop++ ) {
    if ( colors[loop].transparent ) {
      t = IAllocColor ( colors[loop].r, colors[loop].g, colors[loop].b );
      ISetTransparent ( image, t );
    }
  }

  *image_return = image;
  ret = INoError;
  goto cleanup;

fail:
  ret = IInvalidFormat;
  if ( image )
    _IFreeImage ( image );

cleanup:
  /* free the temporary parsing structures (the image, on success, keeps its
     own data and is returned via *image_return) */
  if ( colors ) {
    for ( loop = 0; loop < num_colors; loop++ )
      free ( colors[loop].abbrev ); /* NULL-safe: colors was calloc'd */
    free ( colors );
  }
  free ( rowbuf );

  return ( ret );
}
