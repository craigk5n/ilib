/*
 * IImage.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Ilib.h"
#include "IlibP.h"

#include "IFontBDF.h"


IError ILoadFontFromFile ( char *name, char *path, IFont *font_return )
{
  IFontP *font = NULL;
  IError ret;

  ret = IFontBDFReadFile ( name, path );
  if ( ! ret ) {
    font = (IFontP *) malloc ( sizeof ( IFontP ) );
    memset ( font, '\0', sizeof ( IFontP ) );
    font->magic = IMAGIC_FONT;
    font->name = (char *) malloc ( strlen ( name ) + 1 );
    strcpy ( font->name, name );
    *font_return = (IFont) font;
    return ( INoError );
  } else {
    return ( IFileInvalid );
  }
}


IError ILoadFontFromData ( char *name, char **lines, IFont *font_return )
{
  IFontP *font = NULL;
  IError ret;

  ret = IFontBDFReadData ( name, lines );
  if ( ! ret ) {
    font = (IFontP *) malloc ( sizeof ( IFontP ) );
    memset ( font, '\0', sizeof ( IFontP ) );
    font->magic = IMAGIC_FONT;
    font->name = (char *) malloc ( strlen ( name ) + 1 );
    strcpy ( font->name, name );
    *font_return = (IFont) font;
    return ( INoError );
  } else {
    return ( IFileInvalid );
  }
}



IError _IFreeFont ( IFont font )
{
  IFontP *fontp = (IFontP *)font;

  if ( fontp ) {
    if ( fontp->magic != IMAGIC_FONT )
      return ( IInvalidFont );
    IFontBDFFree ( fontp->name );
    free ( fontp->name );
    free ( fontp );
  }

  return ( INoError );
}


IError IFontSize ( IFont font, unsigned int *height_return )
{
  IFontP *fontp = (IFontP *)font;

  if ( ! fontp )
    return ( IInvalidFont );
  if ( fontp->magic != IMAGIC_FONT )
    return ( IInvalidFont );

  return ( _IFontBDFGetSize ( fontp->name, height_return ) );
}






