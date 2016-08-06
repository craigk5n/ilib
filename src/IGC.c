/*
 * IImage.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	23-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added ISetTextStyle().
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "Ilib.h"
#include "IlibP.h"


IGC ICreateGC ()
{
  IGCP *gc;

  gc = (IGCP *) malloc ( sizeof ( IGCP ) );
  memset ( gc, '\0', sizeof ( IGCP ) );

  gc->magic = IMAGIC_GC;
  gc->foreground = _IGetColor ( IBLACK_PIXEL );
  gc->background = _IGetColor ( IWHITE_PIXEL );
  gc->line_width = 1;
  gc->line_style = ILINE_SOLID;
  gc->text_style = ITEXT_NORMAL;

  return ( (IGC) gc );
}


IError _IFreeGC ( gc )
IGC gc;
{
  IGCP *gcp = (IGCP *)gc;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
    gcp->magic = 0;
    free ( gcp );
  }

  return ( INoError );
}


IError ISetFont ( gc, font )
IGC gc;
IFont font;
{
  IFontP *fontp = (IFontP *)font;
  IGCP *gcp = (IGCP *)gc;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }
  else 
    return ( IInvalidGC );

  if ( fontp ) {
    if ( fontp->magic != IMAGIC_FONT )
      return ( IInvalidFont );
  }
  else 
    return ( IInvalidFont );

  gcp->font = fontp;
  gcp->antialiased = 0;

  return ( INoError );
}



/*
** NOTE: this is just experimental code.
** It produces some pretty ugly text.  Need to look at gimp and
** so how they do this.
*/
IError ISetAntiAliasedFont ( gc, font )
IGC gc;
IFont font;
{
  IFontP *fontp = (IFontP *)font;
  IGCP *gcp = (IGCP *)gc;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }
  else 
    return ( IInvalidGC );

  if ( fontp ) {
    if ( fontp->magic != IMAGIC_FONT )
      return ( IInvalidFont );
  }
  else 
    return ( IInvalidFont );

  gcp->font = fontp;
  gcp->antialiased = 1;

  return ( INoError );
}


IError ISetForeground ( gc, color )
IGC gc;
IColor color;
{
  IGCP *gcp = (IGCP *)gc;
  IColorP *colorp;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }
  else 
    return ( IInvalidGC );

  colorp = _IGetColor ( color );
  if ( ! colorp )
    return ( IInvalidColor );

  gcp->foreground = colorp;

  return ( INoError );
}



IError ISetBackground ( gc, color )
IGC gc;
IColor color;
{
  IGCP *gcp = (IGCP *)gc;
  IColorP *colorp;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }
  else 
    return ( IInvalidGC );

  colorp = _IGetColor ( color );
  if ( ! colorp )
    return ( IInvalidColor );

  gcp->background = colorp;

  return ( INoError );
}


IError ISetLineWidth ( gc, line_width )
IGC gc;
unsigned int line_width;
{
  IGCP *gcp = (IGCP *)gc;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }

  gcp->line_width = line_width;

  return ( INoError );
}



IError ISetLineStyle ( gc, line_style )
IGC gc;
ILineStyle line_style;
{
  IGCP *gcp = (IGCP *)gc;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }

  gcp->line_style = line_style;

  return ( INoError );
}




IError ISetTextStyle ( gc, text_style )
IGC gc;
ITextStyle text_style;
{
  IGCP *gcp = (IGCP *)gc;

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }

  gcp->text_style = text_style;

  return ( INoError );
}

