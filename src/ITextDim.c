/*
 * ITextDim.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	19-May-98	Craig Knudsen	cknudsen@cknudsen.com
 *			Added support for anti-aliasing.
 *			Added IGC arg to ITextDimensions, ITextWidth and
 *			ITextHeight.
 *	17-May-98	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#include "Ilib.h"
#include "IlibP.h"
#include "IFontBDF.h"


#define SPACES_PER_TAB		8

IError ITextWidth ( gc, font, text, len, width_return )
IGC gc;
IFont font;
char *text;
unsigned int len;
unsigned int *width_return;
{
  unsigned int height_return;
  return ( ITextDimensions ( gc, font, text, len, width_return,
    &height_return ) );
}


IError ITextHeight ( gc, font, text, len, height_return )
IGC gc;
IFont font;
char *text;
unsigned int len;
unsigned int *height_return;
{
  unsigned int width_return;
  return ( ITextDimensions ( gc, font, text, len, &width_return,
    height_return ) );
}



IError ITextDimensions ( gc, font, text, len, width_return, height_return )
IGC gc;
IFont font;
char *text;
unsigned int len;
unsigned int *width_return;
unsigned int *height_return;
{
  IGCP *gcp = (IGCP *)gc;
  IFontP *fontp = (IFontP *)font;
  unsigned int *bitdata;
  unsigned int height, width, actual_width, size, font_height;
  int xoffset, yoffset, charx, chary;
  char ch[256], *ptr;
  int loop, loop2;
  IError ret;
  int char_num = 0;
  int ret_width = 0, ret_height = 0;

  if ( ! font )
    return ( INoFontSet );

  if ( fontp->magic != IMAGIC_FONT )
    return ( IInvalidFont );

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );

  charx = 0;
  chary = 0;

  IFontSize ( font, &font_height );

  ret_height = font_height;

  for ( ptr = text, loop = 0; loop < len; loop++, ptr++ ) {
    if ( *ptr == '\012' ) {
      charx = 0;
      chary += font_height;
      ret_height += font_height;
      char_num = 0;
      continue;
    }
    else if ( *ptr == '\t' ) {
      ret = IFontBDFGetChar ( fontp->name, ch, &bitdata, &width, &height,
        &actual_width, &size, &xoffset, &yoffset );
      charx += ( 8 - ( char_num % 8 ) ) * actual_width;
      if ( charx > ret_width )
        ret_width = charx;
      continue;
    }
    else if ( *ptr != '\033' ) {
      ch[0] = *ptr;
      ch[1] = '\0';
    }
    else {
      loop2 = 0;
      ptr++;
      while ( *ptr != ';' && loop < len && loop2 < 256 ) {
        ch[loop2] = *ptr;
        ptr++;
        loop++;
        loop2++;
      }
      ch[loop2] = '\0';
      if ( *ptr != ';' ) {
        return ( IInvalidEscapeSequence );
      }
    }
    ret = IFontBDFGetChar ( fontp->name, ch, &bitdata, &width, &height,
      &actual_width, &size, &xoffset, &yoffset );
    if ( ! ret ) {
      charx += actual_width;
      char_num++;
      if ( charx > ret_width )
        ret_width = charx;
    }
  }

  if ( gcp->antialiased ) {
    ret_width /= 2;
    ret_height /= 2;
  }
  *width_return = (unsigned int)ret_width;
  *height_return = (unsigned int)ret_height;

  return ( INoError );
}



