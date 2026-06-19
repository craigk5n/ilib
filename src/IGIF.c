/*
 * IGIF.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * Previous version of Ilib used code from pbmplus to save GIF images.
 * Due to copyright issues with encoding GIFs, the GIF encoding routines
 * have been removed.  Instead we use Giflib which is freely available at:
 *	http://prtr-13.ucsc.edu/~badger/software/giflib.shtml
 *
 * Libungif is available at:
 *	http://prtr-13.ucsc.edu/~badger/software/libungif.shtml
 *
 * Note that you may use giflib or libungif.  They are basically the same
 * library except that libungif does not include the questionable LZW
 * compression algorithm (that's subject to the Unisys copyright.)  Thus,
 * GIFs from libungif will be uncompressed (larger).
 *
 * Thanks to Eric S. Raymond and Gershon Elber for making Giflib available.
 *
 * History:
 *	20-Oct-04	Craig Knudsen   cknudsen@cknudsen.com
 *			Code contributed from Greg McLearn (in Jul 2000)
 *			Handle when sColor is not present in GIF
 *	20-Aug-99	Craig Knudsen   cknudsen@cknudsen.com
 *			Support writing transparent colors.
 *	19-Aug-99	Craig Knudsen   cknudsen@cknudsen.com
 *			Updated to support writing interlaced GIF files.
 *			Added support for reading/writing comments.
 *			Added support for reading transparent color.
 *	18-Aug-99	Craig Knudsen   cknudsen@cknudsen.com
 *			Updated to support GIF extensions (like interlace)
 *			when reading files.
 *	17-May-98	Craig Knudsen	cknudsen@cknudsen.com
 *			Updated to use Giflib instead of doing or own
 *			GIF encoding.  Saves me from getting sued ;-)
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#ifdef HAVE_GIFLIB

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define PROGRAM_NAME	"Ilib"

#include <gif_lib.h>

#include "Ilib.h"
#include "IlibP.h"

/*
** giflib API compatibility.
**   - giflib 5.0 renamed MakeMapObject/FreeMapObject to Gif* and added an
**     Error out-parameter to the *Open* functions.
**   - giflib 5.1 added an Error out-parameter to the *CloseFile functions.
** The code below is written against the modern (5.x) API; these shims keep it
** building against older giflib releases.
*/
#if !defined(GIFLIB_MAJOR) || GIFLIB_MAJOR < 5
#define GifMakeMapObject  MakeMapObject
#define GifFreeMapObject  FreeMapObject
#define ILIB_GIF_OPEN_HAS_ERR  0
#define ILIB_GIF_CLOSE_HAS_ERR 0
#else
#define ILIB_GIF_OPEN_HAS_ERR  1
#if GIFLIB_MAJOR > 5 || (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1)
#define ILIB_GIF_CLOSE_HAS_ERR 1
#else
#define ILIB_GIF_CLOSE_HAS_ERR 0
#endif
#endif

#define colors_match(color,r,g,b) \
 ((color)->red == (r) && (color)->green == (g) && (color)->blue == (b) )

#define MAX_COLORMAP_SIZE       (256)


#define ABS(a)	((a) < 0 ? (0-(a)) : (a) )

static int InterlacedOffset[] = { 0, 4, 2, 1 };
static int InterlacedJumps[] = { 8, 8, 4, 2 };


static int color_compare ( unsigned int r, unsigned int g, unsigned int b, IColorP *test1, IColorP *test2 )
{
  int diff1, diff2;

  diff1 = ABS ( (int)r - (int)test1->red ) +
    ABS ( (int)g - (int)test1->green ) +
    ABS ( (int)b - (int)test1->blue );
  diff2 = ABS ( (int)r - (int)test2->red ) +
    ABS ( (int)g - (int)test2->green ) +
    ABS ( (int)b - (int)test2->blue );
  return ( diff1 > diff2 );
}

IError _IWriteGIF ( FILE *fp, IImageP *image, IOptions options )
{
  int r, c, offset;
  unsigned char *ptr;
  unsigned int red, green, blue;
  IColorP *colormap[MAX_COLORMAP_SIZE];
  ColorMapObject *GIFcolormap;
  int num_colors = 0, color_ceil, loop, loop2, closest, bits_per_pixel;
  int color_found;
  int transparent = -1, interlaced = 0;
  int fd;
  int gif_err = 0;
  GifFileType *gft = NULL;
  GifByteType ext[4];
  unsigned char *data;
  IError ret = INoError;

  GIFcolormap = NULL;

  /* first make an 8-bit version of the image (calloc zero-fills) */
  data = (unsigned char *) calloc ( image->width * image->height,
    sizeof ( unsigned char ) );
  if ( ! data )
    return ( IGIFError );

  /* Reduce to 256 colors
  ** This is a god-awful hack of an algorithm.  Should really use a
  ** better one.
  ** The first 256 colors found will be used, the rest will be converted
  ** to closest.
  ** NOTE: we should change this to call QuantizeBuffer in GIFLIB.
  */
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
      if ( ! color_found ) {
        if ( num_colors < MAX_COLORMAP_SIZE ) {
          colormap[num_colors] = (IColorP *) malloc ( sizeof ( IColorP ) );
          memset ( colormap[num_colors], '\0', sizeof ( IColorP ) );
          colormap[num_colors]->magic = IMAGIC_COLOR;
          colormap[num_colors]->red = red;
          colormap[num_colors]->green = green;
          colormap[num_colors]->blue = blue;
          data[offset] = num_colors;
          num_colors++;
        } else {
          // find closest!
          closest = 0;
          for ( loop = 0; loop < num_colors; loop++ ) {
            if ( color_compare ( red, green, blue, colormap[closest],
              colormap[loop] ) < 0 )
              closest = loop;
          }
        }
      }
    }
  }

  /* how many cells in colormap (eg. 28->32, 55->64, etc.) */
  for ( bits_per_pixel = 1, color_ceil = 2; color_ceil < num_colors;
    color_ceil *= 2, bits_per_pixel++ ) ;

  fd = fileno ( fp );

  GIFcolormap = GifMakeMapObject ( color_ceil, NULL );
  if ( ! GIFcolormap ) {
    ret = IGIFError;
    goto cleanup;
  }
  for ( loop = 0; loop < color_ceil; loop++ ) {
    if ( loop < num_colors ) {
      GIFcolormap->Colors[loop].Red = colormap[loop]->red;
      GIFcolormap->Colors[loop].Green = colormap[loop]->green;
      GIFcolormap->Colors[loop].Blue = colormap[loop]->blue;
    } else {
      GIFcolormap->Colors[loop].Red = GIFcolormap->Colors[loop].Green =
        GIFcolormap->Colors[loop].Blue = 0;
    }
  }

#if ILIB_GIF_OPEN_HAS_ERR
  gft = EGifOpenFileHandle ( fd, &gif_err );
#else
  gft = EGifOpenFileHandle ( fd );
#endif
  if ( gft == NULL ) {
    ret = IGIFError;
    goto cleanup;
  }

  /* causes seg fault...
  EGifSetGifVersion ( "89a" );
  */
  if ( options & IOPTION_INTERLACED )
    interlaced = 1;

  if ( image->transparent ) {
    for ( loop = 0; loop < num_colors; loop++ ) {
      if ( colors_match ( colormap[loop], image->transparent->red,
        image->transparent->green, image->transparent->blue ) ) {
        transparent = loop;
        break;
      }
    }
  }
  else
    transparent = -1;

  if ( EGifPutScreenDesc ( gft, image->width, image->height,
    bits_per_pixel, 0, GIFcolormap ) == GIF_ERROR ) {
    ret = IGIFError;
    goto cleanup;
  }

  if ( image->comments )
    EGifPutComment ( gft, image->comments );

  if ( transparent >= 0 ) {
    ext[0] = 1;
    ext[1] = 0;
    ext[2] = 0;
    ext[3] = transparent;
    EGifPutExtension ( gft, GRAPHICS_EXT_FUNC_CODE, 4, ext );
  }

  if ( EGifPutImageDesc ( gft, 0, 0, image->width, image->height,
    interlaced, NULL ) == GIF_ERROR ) {
    ret = IGIFError;
    goto cleanup;
  }

  /* for interlaced images, we need to write the rows in a different order */
  if ( interlaced ) {
    for ( loop = 0; loop < 4; loop++ ) {
      for ( loop2 = InterlacedOffset[loop]; loop2 < image->height;
        loop2 += InterlacedJumps[loop] ) {
        if ( EGifPutLine ( gft, data + ( loop2 * image->width ), image->width )
          == GIF_ERROR ) {
         ret = IGIFError;
         goto cleanup;
        }
      }
    }
  } else {
    /* Write the data all at once */
    if ( EGifPutLine ( gft, data, image->width * image->height ) == GIF_ERROR ) {
      ret = IGIFError;
      goto cleanup;
    }
  }

cleanup:
  if ( gft ) {
#if ILIB_GIF_CLOSE_HAS_ERR
    EGifCloseFile ( gft, &gif_err );
#else
    EGifCloseFile ( gft );
#endif
  }

  /* free up allocated resources */
  free ( data );
  for ( loop = 0; loop < num_colors; loop++ ) {
    free ( colormap[loop] );
  }
  if ( GIFcolormap )
    GifFreeMapObject ( GIFcolormap );

  return ( ret );
}





IError _IReadGIF ( FILE *fp, IOptions options, IImageP **image_return )
{
  IImageP *image = NULL;
  GifFileType *gft = NULL;
  (void) options;
  GifRecordType rt;
  GifPixelType *gifdata = NULL;
  GifByteType *extension;
  int extcode;
  int fd;
  int gif_err = 0;
  int loop, loop2, col;
  unsigned char *ptr, *r, *g, *b;
  unsigned int temp;
  char *comments = NULL;
  unsigned int transparent_ind = 0;
  int trans_set = 0;
  IColor transcolor;

  fd = fileno ( fp );

#if ILIB_GIF_OPEN_HAS_ERR
  gft = DGifOpenFileHandle ( fd, &gif_err );
#else
  gft = DGifOpenFileHandle ( fd );
#endif
  if ( gft == NULL )
    goto fail;

  while ( image == NULL ) {
    rt = UNDEFINED_RECORD_TYPE;
    if ( DGifGetRecordType ( gft, &rt ) == GIF_ERROR )
      goto fail;
    if ( rt == IMAGE_DESC_RECORD_TYPE ) {
      if ( DGifGetImageDesc ( gft ) == GIF_ERROR )
        goto fail;
      image = (IImageP *) ICreateImage ( gft->Image.Width,
        gft->Image.Height, IOPTION_NONE );
      if ( ! image )
        goto fail;
      gifdata = (GifPixelType *) malloc ( image->width  * image->height );
      if ( ! gifdata )
        goto fail;
      /* we read the lines out of order for interlaced images (yuck) */
      if ( gft->Image.Interlace ) {
        for ( loop = 0; loop < 4; loop++ ) {
          for ( loop2 = InterlacedOffset[loop]; loop2 < image->height;
            loop2 += InterlacedJumps[loop] ) {
            if ( DGifGetLine ( gft, gifdata + ( loop2 * image->width ),
              image->width ) == GIF_ERROR )
              goto fail;
          }
        }
      } else {
        if ( DGifGetLine ( gft, gifdata, image->width * image->height )
          == GIF_ERROR )
          goto fail;
      }
      /* convert to a 24-bit image ONLY if its got a valid colormap -- GLM 2000*/
      if (gft -> SColorMap)  {
        for ( loop = 0; loop < image->height; loop++ ) {
          for ( col = 0; col < image->width; col++ ) {
            ptr = gifdata + ( loop * image->width ) + col;
            temp = *ptr;
            r = image->data + ( loop * image->width * 3 ) + ( col * 3 );
            g = r + 1;
            b = r + 2;
            if ( temp > (unsigned int) gft->SColorMap->ColorCount ) {
              temp = gft->SColorMap->ColorCount - 1;
              fprintf ( stderr, "ILib Warning: Invalid color found in GIF.\n" );
            }
            *r = gft->SColorMap->Colors[temp].Red;
            *g = gft->SColorMap->Colors[temp].Green;
            *b = gft->SColorMap->Colors[temp].Blue;
          }
        }
      }
    } else if ( rt == EXTENSION_RECORD_TYPE ) {
      /* ignore all extensions except comments */
      DGifGetExtension ( gft, &extcode, &extension );
      while ( extension != NULL ) {
        if ( extcode == COMMENT_EXT_FUNC_CODE ) {
          if ( comments != NULL )
            free ( comments );
          comments = (char *) malloc ( strlen ( (char *)( extension + 1 ) ) + 1 );
          strcpy ( comments, (char *)( extension + 1 ) );
        } else if ( extcode == GRAPHICS_EXT_FUNC_CODE ) {
          /* this is used to set transparent color index */
          if ( extension[1] & 0x01 ) {
            trans_set = 1;
            transparent_ind = extension[4];
          }
        } else {
          /*
          fprintf ( stderr, "Ignoring unknown extension: %d\n",
            extension[0] );
          */
        }
        DGifGetExtensionNext ( gft, &extension );
      }
    } else {
      /* TERMINATE_RECORD_TYPE (end of GIF, no image) or an undefined record:
         no image will ever appear, so stop instead of looping forever. */
      goto fail;
    }
  }

  /* lookup transparent color from colormap IF it exists -- GLM 2000*/
  if ( trans_set && gft->SColorMap ) {
    transcolor = IAllocColor (
      gft->SColorMap->Colors[transparent_ind].Red,
      gft->SColorMap->Colors[transparent_ind].Green,
      gft->SColorMap->Colors[transparent_ind].Blue );
    ISetTransparent ( image, transcolor );
  }

  image->comments = comments;
  comments = NULL;   /* ownership transferred to image */

  if ( gifdata )
    free ( gifdata );
#if ILIB_GIF_CLOSE_HAS_ERR
  DGifCloseFile ( gft, &gif_err );
#else
  DGifCloseFile ( gft );
#endif

  *image_return = image;

  return ( INoError );

fail:
  if ( gifdata )
    free ( gifdata );
  if ( comments )
    free ( comments );
  if ( image )
    _IFreeImage ( image );
  if ( gft )
#if ILIB_GIF_CLOSE_HAS_ERR
    DGifCloseFile ( gft, &gif_err );
#else
    DGifCloseFile ( gft );
#endif
  return ( IGIFError );
}


#endif /* HAVE_GIFLIB */

