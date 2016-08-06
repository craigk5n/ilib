/*
 * IDrawStr.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	21-Jan-00	Geovan Rodriguez <geovan@cigb.edu.cu>
 *			Added IDrawStringRotatedAngle()
 *	23-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added support for text styles:
 *			ITEXT_NORMAL, ITEXT_ETCHED_IN, 
 *			ITEXT_ETCHED_OUT, ITEXT_SHADOWED
 *	21-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IDrawStringRotated()
 *			Removed anti-aliasing stuff since it didn't really
 *			work.  Eventually, true type fonts will be used
 *			for this (FreeType lib).
 *	18-May-98	Craig Knudsen	cknudsen@cknudsen.com
 *			Added support for anti-aliasing fonts by using a
 *			font 2X bigger than we need.
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
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
#include "IFontBDF.h"


#define SPACES_PER_TAB		8



static IError draw_string_rotated_90 (
#ifndef _NO_PROTO
  IImage image,
  IGC gc,
  int x,
  int y,
  char *text,
  unsigned int len,
  ITextDirection direction
#endif
);



IError IDrawString ( image, gc, x, y, text, len )
IImage image;
IGC gc;
int x;
int y;
char *text;
unsigned int len;
{
  return IDrawStringRotated ( image, gc, x, y, text, len, ITEXT_LEFT_TO_RIGHT );
}



/* calculate values for topshadow and bottomshadow */
static void make_top_and_bottom_shadow ( incolorp, top, bottom )
IColorP *incolorp;
IColor *top, *bottom;
{
  unsigned int topr, topg, topb, bottomr, bottomg, bottomb;

  if ( incolorp == NULL ) {
    *top = 0;
    *bottom = 1;
  } else {
    topr = incolorp->red > 205 ? 255 : incolorp->red + 50;
    topg = incolorp->green > 205 ? 255 : incolorp->green + 50;
    topb = incolorp->blue > 205 ? 255 : incolorp->blue + 50;
    *top = IAllocColor ( topr, topg, topb );
    bottomr = incolorp->red < 50 ? 0 : incolorp->red - 50;
    bottomg = incolorp->green < 50 ? 0 : incolorp->green - 50;
    bottomb = incolorp->blue < 50 ? 0 : incolorp->blue - 50;
    *bottom = IAllocColor ( bottomr, bottomg, bottomb );
  }
}


/* calculate values for shadows  */
static void make_shadows ( incolorp, shadows, nshadows )
IColorP *incolorp;
IColor *shadows;
int nshadows;
{
  unsigned int rinc, ginc, binc, rstart, gstart, bstart;
  int loop;

  rstart = ( incolorp->red / 2 );
  gstart = ( incolorp->green / 2 );
  bstart = ( incolorp->blue / 2 );

  if ( incolorp != NULL ) {
    rinc = ( incolorp->red - rstart ) / nshadows;
    ginc = ( incolorp->green - gstart ) / nshadows;
    binc = ( incolorp->blue - bstart ) / nshadows;
    for ( loop = 0; loop < nshadows; loop++ )
      shadows[loop] = IAllocColor ( rstart + rinc * loop,
        gstart + ginc * loop, bstart + binc * loop );
  }
}


IError IDrawStringRotated ( image, gc, x, y, text, len, direction )
IImage image;
IGC gc;
int x;
int y;
char *text;
unsigned int len;
ITextDirection direction;
{
  IGCP *gcp = (IGCP *)gc;
  IError ret = INoError;
  IColor shadows[20], top, bottom;
  IColorP *fgsave;
  int loop, nshadows;
  unsigned int font_height;

  if ( ! gcp )
    return ( IInvalidGC );

  fgsave = gcp->foreground;

  switch ( gcp->text_style ) {
    case ITEXT_NORMAL:
      ret = draw_string_rotated_90 ( image, gc, x, y, text, len, direction );
      break;
    case ITEXT_ETCHED_IN:
    case ITEXT_ETCHED_OUT:
      if ( gcp->background != NULL ) {
        if ( gcp->text_style == ITEXT_ETCHED_OUT )
          make_top_and_bottom_shadow ( gcp->background, &top, &bottom );
        else
          make_top_and_bottom_shadow ( gcp->background, &bottom, &top );
        ISetForeground ( gc, top );
        ret = draw_string_rotated_90 ( image, gc, x - 1, y - 1,
          text, len, direction );
        if ( ! ret ) {
          ISetForeground ( gc, bottom );
          ret = draw_string_rotated_90 ( image, gc, x + 1, y + 1,
            text, len, direction );
        }
      }
      if ( ! ret ) {
        gcp->foreground = gcp->background;
        ret = draw_string_rotated_90 ( image, gc, x, y,
          text, len, direction );
      }
      break;
    case ITEXT_SHADOWED:
      if ( gcp->background != NULL ) {
        IFontSize ( (IFont)gcp->font, &font_height );
        nshadows = font_height / 5;
        make_shadows ( gcp->background, shadows, nshadows );
        gcp->foreground = fgsave;
        for ( loop = nshadows; loop > 0; loop-- ) {
          ISetForeground ( gc, shadows[loop-1] );
          ret = draw_string_rotated_90 ( image, gc, x + loop, y + loop,
            text, len, direction );
        }
        gcp->foreground = fgsave;
      }
      if ( ! ret )
        ret = draw_string_rotated_90 ( image, gc, x, y, text, len, direction );
      break;
  }

  gcp->foreground = fgsave;

  return ( ret );
}


static IError draw_string_rotated_90 ( image, gc, x, y, text, len, direction )
IImage image;
IGC gc;
int x;
int y;
char *text;
unsigned int len;
ITextDirection direction;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  unsigned int *bitdata;
  unsigned int height, width, actual_width, size, font_height;
  int xoffset, yoffset, charx, chary;
  char ch[256], *ptr;
  int loop, loop2, loop3;
  IError ret;
  int myx, myy;
  int char_num = 0;

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );

  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  if ( ! gcp->font )
    return ( INoFontSet );

  charx = x;
  chary = y;

  IFontSize ( (IFont)gcp->font, &font_height );

  for ( ptr = text, loop = 0; loop < len; loop++, ptr++ ) {
    if ( *ptr == '\012' ) {
      switch ( direction ) {
        case ITEXT_LEFT_TO_RIGHT:
          charx = x;
          chary += font_height;
          break;
        case ITEXT_TOP_TO_BOTTOM:
          chary = y;
          charx -= font_height;
          break;
        case ITEXT_BOTTOM_TO_TOP:
          chary = y;
          charx += font_height;
          break;
      }
      char_num = 0;
      continue;
    }
    else if ( *ptr == '\t' ) {
      ret = IFontBDFGetChar ( gcp->font->name, ch, &bitdata, &width, &height,
        &actual_width, &size, &xoffset, &yoffset );
      switch ( direction ) {
        case ITEXT_LEFT_TO_RIGHT:
          charx += ( 8 - ( char_num % 8 ) ) * actual_width;
          break;
        case ITEXT_TOP_TO_BOTTOM:
          chary -= ( 8 - ( char_num % 8 ) ) * actual_width;
          break;
        case ITEXT_BOTTOM_TO_TOP:
          chary += ( 8 - ( char_num % 8 ) ) * actual_width;
          break;
      }
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
    ret = IFontBDFGetChar ( gcp->font->name, ch, &bitdata, &width, &height,
      &actual_width, &size, &xoffset, &yoffset );
    if ( ! ret ) {
      for ( loop3 = 0; loop3 < height; loop3++ ) {
        switch ( direction ) {
          case ITEXT_LEFT_TO_RIGHT:
            myy = chary - ( height + yoffset ) + loop3;
            for ( loop2 = 0; loop2 < width; loop2++ ) {
              if ( bitdata[loop3 * width + loop2] ) {
                myx = charx + xoffset + loop2;
                _ISetPoint ( imagep, gcp, myx, myy );
              }
            }
            break;
          case ITEXT_TOP_TO_BOTTOM:
            myx = charx + ( height + yoffset ) - loop3;
            for ( loop2 = 0; loop2 < width; loop2++ ) {
              if ( bitdata[loop3 * width + loop2] ) {
                myy = chary + xoffset + loop2;
                _ISetPoint ( imagep, gcp, myx, myy );
              }
            }
            break;
          case ITEXT_BOTTOM_TO_TOP:
            myx = charx - ( height + yoffset ) + loop3;
            for ( loop2 = 0; loop2 < width; loop2++ ) {
              if ( bitdata[loop3 * width + loop2] ) {
                myy = chary - xoffset - loop2;
                _ISetPoint ( imagep, gcp, myx, myy );
              }
            }
            break;
        }
      }
      switch ( direction ) {
        case ITEXT_LEFT_TO_RIGHT:
          charx += actual_width;
          break;
        case ITEXT_TOP_TO_BOTTOM:
          chary += actual_width;
          break;
        case ITEXT_BOTTOM_TO_TOP:
          chary -= actual_width;
          break;
      }
      char_num++;
    }
  }

  return ( INoError );
}





IError IDrawStringRotatedAngle ( image, gc, x, y, text, len, angle )
IImage image;
IGC gc;
int x;
int y;
char *text;
unsigned int len;
double angle;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  unsigned int *bitdata;
  unsigned int height, width, actual_width, size, font_height;
  int xoffset, yoffset, charx, chary;
  char ch[256], *ptr;
  int loop, loop2, loop3;
  IError ret;
  int myx, myy;
  int char_num = 0;

  double x1, y1, x2, y2;
  int alpha;
    
  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );

  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  if ( ! gcp->font )
    return ( INoFontSet );

  charx = x;
  chary = y;

  IFontSize ( (IFont)gcp->font, &font_height );

  for ( ptr = text, loop = 0; loop < len; loop++, ptr++ ) {
    
    if ( *ptr == '\012' ) {
      chary += font_height;
      charx = x;
      char_num = 0;
      continue;
    }
    else if ( *ptr == '\t' ) {
      ret = IFontBDFGetChar ( gcp->font->name, ch, &bitdata, &width, &height,
        &actual_width, &size, &xoffset, &yoffset );
      chary += ( 8 - ( char_num % 8 ) ) * actual_width;
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
    
    ret = IFontBDFGetChar ( gcp->font->name, ch, &bitdata, &width, &height,
      &actual_width, &size, &xoffset, &yoffset );

    alpha = angle;
          
    if ( ! ret ) {
      for ( loop3 = 0; loop3 < height; loop3++ ) {
        for ( loop2 = 0; loop2 < width; loop2++ ) {
          if ( bitdata[loop3 * width + loop2] ) {
            x1 = loop2 + xoffset;
            y1 = loop3 + size - height;
            x2 = x1 * cos ( alpha * M_PI / 180 ) +
              y1 * sin ( alpha * M_PI / 180 );
            y2 = -1 * x1 * sin ( alpha * M_PI / 180 ) +
              y1 * cos ( alpha * M_PI / 180 );
            myy = chary - (size + yoffset ) + y2;
            myx = charx + x2;
            _ISetPoint ( imagep, gcp, myx, myy );
          }
        }  
      }
      x1 = actual_width + xoffset;      
      y1 = 0;      
      x2 = x1 * cos ( alpha * M_PI / 180 ) +
        y1 * sin ( alpha * M_PI / 180 );      
      y2 = -1 * x1 * sin ( alpha * M_PI / 180 ) +
        y1 * cos ( alpha * M_PI / 180 );      
      charx += x2;      
      chary += y2;      
      char_num++;
    }
  }

  return ( INoError );
}


