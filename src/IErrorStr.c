/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IErrorStr.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	22-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IInvalidPolygon
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "Ilib.h"
#include "IlibP.h"

#ifdef HAVE_GIFLIB
#include <gif_lib.h>
#endif


char *IErrorString ( IError err )
{
  switch ( err ) {
  case IInvalidImage:
    return ( "Invalid image" );
  case IInvalidGC:
    return ( "Invalid GC" );
  case IInvalidColor:
    return ( "Invalid color" );
  case INoTransparentColor:
    return ( "No transparent color set" );
  case IInvalidFont:
    return ( "Invalid font" );
  case IFunctionNotImplemented:
    return ( "Function not implemented" );
  case IInvalidFormat:
    return ( "Invalid file format" );
  case IFileInvalid:
    return ( "Invalid file" );
  case IErrorWriting:
    return ( "Error writing file" );
  case INoFontSet:
    return ( "No font set" );
  case INoSuchFont:
    return ( "Font not found" );
  case INoSuchFile:
    return ( "No such file" );
  case IFontError:
    return ( "Font error" );
  case IInvalidEscapeSequence:
    return ( "Invalid escape sequence in text" );
  case IInvalidArgument:
    return ( "Invalid argument" );
  case IInvalidColorName:
    return ( "Invalid color name" );
  case INoGIFSupport:
    return ( "GIF support not available" );
#ifdef HAVE_GIFLIB
  /* giflib 5.x removed the global GifLastError(); errors are reported per
     call/handle, which this generic routine does not have access to. */
  case IGIFError:
    return ( "GIF error" );
#endif
  case INoPNGSupport:
    return ( "PNG support not available" );
  case IPNGError:
    return ( "Unknown PNG error" );
  case IInvalidPolygon:
    return ( "Invalid polygon (only convex non-intersecting are supported)" );
  case INoWEBPSupport:
    return ( "WebP support not available" );
  case IWEBPError:
    return ( "Unknown WebP error" );
  case INoAVIFSupport:
    return ( "AVIF support not available" );
  case IAVIFError:
    return ( "Unknown AVIF error" );
  case INoError:
  default:
    return ( "error" );
  }
}
