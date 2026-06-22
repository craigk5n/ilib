/* SPDX-License-Identifier: GPL-2.0-only */
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

#define PROGRAM_NAME "Ilib"

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
#if !defined( GIFLIB_MAJOR ) || GIFLIB_MAJOR < 5
#define GifMakeMapObject MakeMapObject
#define GifFreeMapObject FreeMapObject
#define ILIB_GIF_OPEN_HAS_ERR 0
#define ILIB_GIF_CLOSE_HAS_ERR 0
#else
#define ILIB_GIF_OPEN_HAS_ERR 1
#if GIFLIB_MAJOR > 5 || ( GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1 )
#define ILIB_GIF_CLOSE_HAS_ERR 1
#else
#define ILIB_GIF_CLOSE_HAS_ERR 0
#endif
#endif

#define colors_match( color, r, g, b ) \
  ( ( color )->red == ( r ) && ( color )->green == ( g ) && ( color )->blue == ( b ) )

#define MAX_COLORMAP_SIZE ( 256 )


#define ABS( a ) ( ( a ) < 0 ? ( 0 - ( a ) ) : ( a ) )

static int InterlacedOffset[] = { 0, 4, 2, 1 };
static int InterlacedJumps[] = { 8, 8, 4, 2 };


static int color_compare ( unsigned int r, unsigned int g, unsigned int b, IColorP *test1, IColorP *test2 )
{
  int diff1, diff2;

  diff1 = ABS ( (int) r - (int) test1->red ) +
          ABS ( (int) g - (int) test1->green ) +
          ABS ( (int) b - (int) test1->blue );
  diff2 = ABS ( (int) r - (int) test2->red ) +
          ABS ( (int) g - (int) test2->green ) +
          ABS ( (int) b - (int) test2->blue );
  return ( diff1 > diff2 );
}

/* Build an 8-bit indexed (palettized) version of `image`. On success returns
   INoError and sets *data_out (malloc'd width*height indices, free with
   free()), *map_out (GifMakeMapObject, free with GifFreeMapObject), *bpp_out,
   and *transparent_out (palette index of the image's transparent color, or
   -1). Shared by the single-frame and animated GIF writers. */
static IError _gif_index_image ( IImageP *image, int dither,
  unsigned char **data_out, ColorMapObject **map_out, int *bpp_out,
  int *transparent_out )
{
  int r, c, offset, loop;
  unsigned char *ptr;
  unsigned int red, green, blue;
  IColorP *colormap[MAX_COLORMAP_SIZE];
  ColorMapObject *GIFcolormap;
  int num_colors = 0, color_ceil, closest, bits_per_pixel, color_found;
  int transparent = -1;
  unsigned char *data, *reduced = NULL, *src = image->data;

  /* first make an 8-bit version of the image (calloc zero-fills) */
  data = (unsigned char *) calloc ( image->width * image->height,
    sizeof ( unsigned char ) );
  if ( !data )
    return ( IGIFError );

  /* GIF allows at most 256 colors. Reduce a color image to a 256-color palette
     up front (a no-op when it already fits) so the palette-building loop below
     captures every color exactly instead of dropping the overflow. With
     dither, error is diffused so gradients do not band. */
  if ( !image->greyscale ) {
    reduced =
      (unsigned char *) malloc ( (size_t) image->width * image->height * 3 );
    if ( !reduced ) {
      free ( data );
      return ( IGIFError );
    }
    _IReduceColorsRGBDither ( image->data, image->width, image->height,
      MAX_COLORMAP_SIZE, dither, reduced );
    src = reduced;
  }

  /* The first 256 colors found are used; any overflow maps to the closest. */
  for ( r = 0; r < image->height; r++ ) {
    for ( c = 0; c < image->width; c++ ) {
      offset = ( r * image->width ) + c;
      if ( image->greyscale ) {
        ptr = image->data + ( r * image->width ) + c;
        red = green = blue = (unsigned int) *ptr;
      }
      else {
        ptr = src + ( r * image->width * 3 ) + ( c * 3 );
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
          memset ( colormap[num_colors], '\0', sizeof ( IColorP ) );
          colormap[num_colors]->magic = IMAGIC_COLOR;
          colormap[num_colors]->red = red;
          colormap[num_colors]->green = green;
          colormap[num_colors]->blue = blue;
          data[offset] = num_colors;
          num_colors++;
        }
        else {
          closest = 0;
          for ( loop = 0; loop < num_colors; loop++ ) {
            if ( color_compare ( red, green, blue, colormap[closest],
                   colormap[loop] ) < 0 )
              closest = loop;
          }
          data[offset] = closest;
        }
      }
    }
  }

  /* how many cells in colormap (eg. 28->32, 55->64, etc.) */
  for ( bits_per_pixel = 1, color_ceil = 2; color_ceil < num_colors;
        color_ceil *= 2, bits_per_pixel++ )
    ;

  GIFcolormap = GifMakeMapObject ( color_ceil, NULL );
  if ( !GIFcolormap ) {
    free ( data );
    free ( reduced );
    for ( loop = 0; loop < num_colors; loop++ )
      free ( colormap[loop] );
    return ( IGIFError );
  }
  for ( loop = 0; loop < color_ceil; loop++ ) {
    if ( loop < num_colors ) {
      GIFcolormap->Colors[loop].Red = colormap[loop]->red;
      GIFcolormap->Colors[loop].Green = colormap[loop]->green;
      GIFcolormap->Colors[loop].Blue = colormap[loop]->blue;
    }
    else {
      GIFcolormap->Colors[loop].Red = GIFcolormap->Colors[loop].Green =
        GIFcolormap->Colors[loop].Blue = 0;
    }
  }

  if ( image->transparent ) {
    for ( loop = 0; loop < num_colors; loop++ ) {
      if ( colors_match ( colormap[loop], image->transparent->red,
             image->transparent->green, image->transparent->blue ) ) {
        transparent = loop;
        break;
      }
    }
  }

  for ( loop = 0; loop < num_colors; loop++ )
    free ( colormap[loop] );
  free ( reduced );

  *data_out = data;
  *map_out = GIFcolormap;
  *bpp_out = bits_per_pixel;
  *transparent_out = transparent;
  return ( INoError );
}

IError _IWriteGIF ( FILE *fp, IImageP *image, IOptions options )
{
  int loop, loop2, bits_per_pixel, transparent, interlaced = 0;
  int fd, gif_err = 0;
  GifFileType *gft = NULL;
  GifByteType ext[4];
  unsigned char *data = NULL;
  ColorMapObject *GIFcolormap = NULL;
  IError ret;

  ret = _gif_index_image ( image, ( options & IOPTION_DITHER ) ? 1 : 0, &data,
    &GIFcolormap, &bits_per_pixel, &transparent );
  if ( ret != INoError )
    return ( ret );

  fd = fileno ( fp );
#if ILIB_GIF_OPEN_HAS_ERR
  gft = EGifOpenFileHandle ( fd, &gif_err );
#else
  gft = EGifOpenFileHandle ( fd );
#endif
  if ( gft == NULL ) {
    ret = IGIFError;
    goto cleanup;
  }

  if ( options & IOPTION_INTERLACED )
    interlaced = 1;

  if ( EGifPutScreenDesc ( gft, image->width, image->height, bits_per_pixel, 0,
         GIFcolormap ) == GIF_ERROR ) {
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

  if ( EGifPutImageDesc ( gft, 0, 0, image->width, image->height, interlaced,
         NULL ) == GIF_ERROR ) {
    ret = IGIFError;
    goto cleanup;
  }

  /* for interlaced images, we need to write the rows in a different order */
  if ( interlaced ) {
    for ( loop = 0; loop < 4; loop++ ) {
      for ( loop2 = InterlacedOffset[loop]; loop2 < image->height;
            loop2 += InterlacedJumps[loop] ) {
        if ( EGifPutLine ( gft, data + ( loop2 * image->width ),
               image->width ) == GIF_ERROR ) {
          ret = IGIFError;
          goto cleanup;
        }
      }
    }
  }
  else {
    if ( EGifPutLine ( gft, data, image->width * image->height ) ==
         GIF_ERROR ) {
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
  free ( data );
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
      if ( !image )
        goto fail;
      gifdata = (GifPixelType *) malloc ( image->width * image->height );
      if ( !gifdata )
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
      }
      else {
        if ( DGifGetLine ( gft, gifdata, image->width * image->height ) == GIF_ERROR )
          goto fail;
      }
      /* convert to a 24-bit image ONLY if its got a valid colormap -- GLM 2000*/
      if ( gft->SColorMap ) {
        for ( loop = 0; loop < image->height; loop++ ) {
          for ( col = 0; col < image->width; col++ ) {
            ptr = gifdata + ( loop * image->width ) + col;
            temp = *ptr;
            r = image->data + ( loop * image->width * 3 ) + ( col * 3 );
            g = r + 1;
            b = r + 2;
            if ( temp >= (unsigned int) gft->SColorMap->ColorCount ) {
              temp = ( gft->SColorMap->ColorCount > 0 )
                       ? (unsigned int) gft->SColorMap->ColorCount - 1
                       : 0;
              fprintf ( stderr, "ILib Warning: Invalid color found in GIF.\n" );
            }
            *r = gft->SColorMap->Colors[temp].Red;
            *g = gft->SColorMap->Colors[temp].Green;
            *b = gft->SColorMap->Colors[temp].Blue;
          }
        }
      }
    }
    else if ( rt == EXTENSION_RECORD_TYPE ) {
      /* ignore all extensions except comments */
      DGifGetExtension ( gft, &extcode, &extension );
      while ( extension != NULL ) {
        if ( extcode == COMMENT_EXT_FUNC_CODE ) {
          /* extension[0] is the sub-block length; the bytes are NOT
             NUL-terminated, so copy exactly that many. */
          unsigned int clen = extension[0];
          if ( comments != NULL )
            free ( comments );
          comments = (char *) malloc ( clen + 1 );
          if ( comments ) {
            memcpy ( comments, extension + 1, clen );
            comments[clen] = '\0';
          }
        }
        else if ( extcode == GRAPHICS_EXT_FUNC_CODE ) {
          /* this is used to set transparent color index */
          if ( extension[1] & 0x01 ) {
            trans_set = 1;
            transparent_ind = extension[4];
          }
        }
        else {
          /*
          fprintf ( stderr, "Ignoring unknown extension: %d\n",
            extension[0] );
          */
        }
        DGifGetExtensionNext ( gft, &extension );
      }
    }
    else {
      /* TERMINATE_RECORD_TYPE (end of GIF, no image) or an undefined record:
         no image will ever appear, so stop instead of looping forever. */
      goto fail;
    }
  }

  /* lookup transparent color from colormap IF it exists -- GLM 2000*/
  if ( trans_set && gft->SColorMap &&
       transparent_ind < (unsigned int) gft->SColorMap->ColorCount ) {
    transcolor = IAllocColor (
      gft->SColorMap->Colors[transparent_ind].Red,
      gft->SColorMap->Colors[transparent_ind].Green,
      gft->SColorMap->Colors[transparent_ind].Blue );
    ISetTransparent ( image, transcolor );
  }

  image->comments = comments;
  comments = NULL; /* ownership transferred to image */

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


/* ---------------------------------------------------------- animated GIF */

/* The animated read/write paths need giflib 5.x (DGifSlurp, the
   GraphicsControlBlock helpers and the streaming extension API). On older
   giflib these report no support rather than failing to build. */
#if defined( GIFLIB_MAJOR ) && GIFLIB_MAJOR >= 5

/* Read the NETSCAPE2.0 loop count from a slurped GIF (0 = forever / absent). */
static int _gif_loop_count ( GifFileType *gft )
{
  int i, j;
  for ( i = 0; i < gft->ImageCount; i++ ) {
    SavedImage *si = &gft->SavedImages[i];
    for ( j = 0; j < si->ExtensionBlockCount; j++ ) {
      ExtensionBlock *eb = &si->ExtensionBlocks[j];
      if ( eb->Function == APPLICATION_EXT_FUNC_CODE && eb->ByteCount >= 11 &&
           memcmp ( eb->Bytes, "NETSCAPE2.0", 11 ) == 0 ) {
        /* The loop count is in the following sub-block: 0x01, lo, hi. */
        if ( j + 1 < si->ExtensionBlockCount ) {
          ExtensionBlock *sub = &si->ExtensionBlocks[j + 1];
          if ( sub->ByteCount >= 3 && sub->Bytes[0] == 1 )
            return ( sub->Bytes[1] | ( sub->Bytes[2] << 8 ) );
        }
      }
    }
  }
  return ( 0 );
}

IError _IReadAnimGIF ( FILE *fp, IAnimation *anim_return )
{
  GifFileType *gft = NULL;
  int fd, gif_err = 0, i, x, y;
  int sw, sh;
  unsigned char *canvas = NULL, *saved = NULL;
  IAnimation anim = NULL;
  IError ret = IGIFError;

  fd = fileno ( fp );
#if ILIB_GIF_OPEN_HAS_ERR
  gft = DGifOpenFileHandle ( fd, &gif_err );
#else
  gft = DGifOpenFileHandle ( fd );
#endif
  if ( !gft )
    return ( IGIFError );
  if ( DGifSlurp ( gft ) == GIF_ERROR )
    goto done;

  sw = gft->SWidth;
  sh = gft->SHeight;
  if ( sw <= 0 || sh <= 0 || gft->ImageCount <= 0 )
    goto done;

  anim = ICreateAnimation ();
  if ( !anim )
    goto done;

  /* Full-screen RGB canvas, composited frame by frame honoring disposal. */
  canvas = (unsigned char *) malloc ( (size_t) sw * sh * 3 );
  saved = (unsigned char *) malloc ( (size_t) sw * sh * 3 );
  if ( !canvas || !saved )
    goto done;
  memset ( canvas, 0xff, (size_t) sw * sh * 3 ); /* start on white */

  for ( i = 0; i < gft->ImageCount; i++ ) {
    SavedImage *si = &gft->SavedImages[i];
    GifImageDesc *d = &si->ImageDesc;
    ColorMapObject *cmap = d->ColorMap ? d->ColorMap : gft->SColorMap;
    GraphicsControlBlock gcb;
    int has_gcb = ( DGifSavedExtensionToGCB ( gft, i, &gcb ) == GIF_OK );
    int transp = has_gcb ? gcb.TransparentColor : NO_TRANSPARENT_COLOR;
    int disposal = has_gcb ? gcb.DisposalMode : DISPOSAL_UNSPECIFIED;
    int delay_ms = has_gcb ? gcb.DelayTime * 10 : 0;
    IImageP *frame;

    if ( !cmap )
      goto done;
    if ( disposal == DISPOSE_PREVIOUS )
      memcpy ( saved, canvas, (size_t) sw * sh * 3 );

    /* Paint this frame's sub-rectangle onto the canvas. */
    for ( y = 0; y < d->Height; y++ ) {
      int cy = d->Top + y;
      if ( cy < 0 || cy >= sh )
        continue;
      for ( x = 0; x < d->Width; x++ ) {
        int cx = d->Left + x;
        int idx = si->RasterBits[y * d->Width + x];
        unsigned char *p;
        if ( cx < 0 || cx >= sw )
          continue;
        if ( transp != NO_TRANSPARENT_COLOR && idx == transp )
          continue; /* transparent: leave the canvas pixel */
        if ( idx >= cmap->ColorCount )
          idx = 0;
        p = canvas + ( (size_t) cy * sw + cx ) * 3;
        p[0] = cmap->Colors[idx].Red;
        p[1] = cmap->Colors[idx].Green;
        p[2] = cmap->Colors[idx].Blue;
      }
    }

    /* Snapshot the canvas as this frame. */
    frame = (IImageP *) ICreateImage ( sw, sh, IOPTION_NONE );
    if ( !frame )
      goto done;
    memcpy ( frame->data, canvas, (size_t) sw * sh * 3 );
    if ( IAddAnimationFrame ( anim, (IImage) frame, delay_ms ) != INoError ) {
      _IFreeImage ( frame );
      goto done;
    }
    _IFreeImage ( frame );

    /* Apply disposal for the next frame. */
    if ( disposal == DISPOSE_BACKGROUND ) {
      for ( y = 0; y < d->Height; y++ ) {
        int cy = d->Top + y;
        if ( cy < 0 || cy >= sh )
          continue;
        for ( x = 0; x < d->Width; x++ ) {
          int cx = d->Left + x;
          unsigned char *p;
          if ( cx < 0 || cx >= sw )
            continue;
          p = canvas + ( (size_t) cy * sw + cx ) * 3;
          p[0] = p[1] = p[2] = 0xff;
        }
      }
    }
    else if ( disposal == DISPOSE_PREVIOUS ) {
      memcpy ( canvas, saved, (size_t) sw * sh * 3 );
    }
  }

  ISetAnimationLoopCount ( anim, _gif_loop_count ( gft ) );
  *anim_return = anim;
  anim = NULL;
  ret = INoError;

done:
  free ( canvas );
  free ( saved );
  if ( anim )
    _IFreeAnimation ( anim );
  if ( gft )
#if ILIB_GIF_CLOSE_HAS_ERR
    DGifCloseFile ( gft, &gif_err );
#else
    DGifCloseFile ( gft );
#endif
  return ( ret );
}

IError _IWriteAnimGIF ( FILE *fp, IAnimation anim, IOptions options )
{
  GifFileType *gft = NULL;
  int fd, gif_err = 0, i, interlaced = 0;
  int nframes = IAnimationFrameCount ( anim );
  int sw = 0, sh = 0, loops;
  IError ret = IGIFError;

  if ( nframes <= 0 )
    return ( IInvalidAnimation );
  if ( options & IOPTION_INTERLACED )
    interlaced = 1;
  {
    IImageP *f0 = (IImageP *) IAnimationFrame ( anim, 0 );
    sw = f0->width;
    sh = f0->height;
  }
  loops = IAnimationLoopCount ( anim );

  fd = fileno ( fp );
#if ILIB_GIF_OPEN_HAS_ERR
  gft = EGifOpenFileHandle ( fd, &gif_err );
#else
  gft = EGifOpenFileHandle ( fd );
#endif
  if ( !gft )
    return ( IGIFError );
  EGifSetGifVersion ( gft, true ); /* animation needs the GIF89a extensions */

  for ( i = 0; i < nframes; i++ ) {
    IImageP *frame = (IImageP *) IAnimationFrame ( anim, i );
    unsigned char *data = NULL;
    ColorMapObject *cmap = NULL;
    int bpp, transp;
    GraphicsControlBlock gcb;
    GifByteType ext[4];
    int rows;

    if ( _gif_index_image ( frame, ( options & IOPTION_DITHER ) ? 1 : 0, &data,
           &cmap, &bpp, &transp ) != INoError )
      goto done;

    if ( i == 0 ) {
      /* Logical screen descriptor + the NETSCAPE2.0 looping extension. */
      if ( EGifPutScreenDesc ( gft, sw, sh, bpp, 0, cmap ) == GIF_ERROR ) {
        free ( data );
        GifFreeMapObject ( cmap );
        goto done;
      }
      EGifPutExtensionLeader ( gft, APPLICATION_EXT_FUNC_CODE );
      EGifPutExtensionBlock ( gft, 11, "NETSCAPE2.0" );
      {
        unsigned char sub[3];
        sub[0] = 1;
        sub[1] = (unsigned char) ( loops & 0xff );
        sub[2] = (unsigned char) ( ( loops >> 8 ) & 0xff );
        EGifPutExtensionBlock ( gft, 3, sub );
      }
      EGifPutExtensionTrailer ( gft );
    }

    /* Per-frame graphics control: delay (centiseconds), disposal, transparency. */
    memset ( &gcb, 0, sizeof ( gcb ) );
    gcb.DisposalMode = DISPOSE_BACKGROUND;
    gcb.UserInputFlag = false;
    gcb.DelayTime = IAnimationFrameDelay ( anim, i ) / 10;
    gcb.TransparentColor = transp;
    EGifGCBToExtension ( &gcb, ext );
    EGifPutExtension ( gft, GRAPHICS_EXT_FUNC_CODE, 4, ext );

    /* Each frame is a full-size image with its own local colormap. */
    if ( EGifPutImageDesc ( gft, 0, 0, frame->width, frame->height, interlaced,
           cmap ) == GIF_ERROR ) {
      free ( data );
      GifFreeMapObject ( cmap );
      goto done;
    }
    if ( interlaced ) {
      int loop, loop2;
      for ( loop = 0; loop < 4; loop++ ) {
        for ( loop2 = InterlacedOffset[loop]; loop2 < frame->height;
              loop2 += InterlacedJumps[loop] ) {
          if ( EGifPutLine ( gft, data + ( loop2 * frame->width ),
                 frame->width ) == GIF_ERROR ) {
            free ( data );
            GifFreeMapObject ( cmap );
            goto done;
          }
        }
      }
    }
    else {
      rows = frame->width * frame->height;
      if ( EGifPutLine ( gft, data, rows ) == GIF_ERROR ) {
        free ( data );
        GifFreeMapObject ( cmap );
        goto done;
      }
    }
    free ( data );
    GifFreeMapObject ( cmap );
  }
  ret = INoError;

done:
  if ( gft )
#if ILIB_GIF_CLOSE_HAS_ERR
    EGifCloseFile ( gft, &gif_err );
#else
    EGifCloseFile ( gft );
#endif
  return ( ret );
}

#else /* giflib < 5: animation API unavailable */

IError _IReadAnimGIF ( FILE *fp, IAnimation *anim_return )
{
  (void) fp;
  (void) anim_return;
  return ( INoGIFSupport );
}

IError _IWriteAnimGIF ( FILE *fp, IAnimation anim, IOptions options )
{
  (void) fp;
  (void) anim;
  (void) options;
  return ( INoGIFSupport );
}

#endif /* GIFLIB_MAJOR >= 5 */

#endif /* HAVE_GIFLIB */
