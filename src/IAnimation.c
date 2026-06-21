/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IAnimation.c
 *
 * Image library
 *
 * Description:
 *	A multi-frame animation: an ordered list of frames (each an IImage with
 *	a display delay in milliseconds) plus a loop count. Reads and writes
 *	animated GIFs via the format-private helpers in IGIF.c.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "Ilib.h"
#include "IlibP.h"

typedef struct {
  unsigned int magic;
  IImageP **frames; /* owned frame images */
  int *delays;      /* per-frame display time, milliseconds */
  int nframes;
  int cap;
  int loop_count; /* 0 = loop forever */
} IAnimationP;

static IAnimationP *_anim_valid ( IAnimation anim )
{
  IAnimationP *a = (IAnimationP *) anim;
  if ( !a || a->magic != IMAGIC_ANIM )
    return ( NULL );
  return ( a );
}

/* Deep-copy an image so the animation owns its frames. */
static IImageP *_anim_dup_image ( IImageP *src )
{
  IOptions opt = IOPTION_NONE;
  IImageP *d;
  if ( src->channels == 4 )
    opt = IOPTION_ALPHA;
  else if ( src->greyscale )
    opt = IOPTION_GREYSCALE;
  d = (IImageP *) ICreateImage ( src->width, src->height, opt );
  if ( !d )
    return ( NULL );
  memcpy ( d->data, src->data,
    (size_t) src->width * src->height * src->channels );
  d->interlaced = src->interlaced;
  if ( src->transparent )
    ISetTransparent ( (IImage) d,
      IAllocColor ( src->transparent->red, src->transparent->green,
        src->transparent->blue ) );
  if ( src->comments ) {
    size_t n = strlen ( src->comments ) + 1;
    d->comments = (char *) malloc ( n );
    if ( d->comments )
      memcpy ( d->comments, src->comments, n );
  }
  return ( d );
}

IAnimation ICreateAnimation ()
{
  IAnimationP *a = (IAnimationP *) calloc ( 1, sizeof ( IAnimationP ) );
  if ( !a )
    return ( NULL );
  a->magic = IMAGIC_ANIM;
  a->loop_count = 0;
  return ( (IAnimation) a );
}

IError _IFreeAnimation ( IAnimation anim )
{
  IAnimationP *a = _anim_valid ( anim );
  int i;
  if ( !a )
    return ( IInvalidAnimation );
  for ( i = 0; i < a->nframes; i++ )
    _IFreeImage ( a->frames[i] );
  free ( a->frames );
  free ( a->delays );
  a->magic = 0;
  free ( a );
  return ( INoError );
}

IError IAddAnimationFrame ( IAnimation anim, IImage frame, int delay_ms )
{
  IAnimationP *a = _anim_valid ( anim );
  IImageP *src = (IImageP *) frame;
  IImageP *copy;

  if ( !a )
    return ( IInvalidAnimation );
  if ( !src || src->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  if ( a->nframes == a->cap ) {
    int ncap = a->cap ? a->cap * 2 : 8;
    IImageP **nf =
      (IImageP **) realloc ( a->frames, (size_t) ncap * sizeof ( IImageP * ) );
    int *nd = (int *) realloc ( a->delays, (size_t) ncap * sizeof ( int ) );
    if ( !nf || !nd ) {
      free ( nf );
      free ( nd );
      return ( IInvalidArgument );
    }
    a->frames = nf;
    a->delays = nd;
    a->cap = ncap;
  }

  copy = _anim_dup_image ( src );
  if ( !copy )
    return ( IInvalidImage );
  a->frames[a->nframes] = copy;
  a->delays[a->nframes] = ( delay_ms < 0 ) ? 0 : delay_ms;
  a->nframes++;
  return ( INoError );
}

int IAnimationFrameCount ( IAnimation anim )
{
  IAnimationP *a = _anim_valid ( anim );
  return ( a ? a->nframes : 0 );
}

IImage IAnimationFrame ( IAnimation anim, int index )
{
  IAnimationP *a = _anim_valid ( anim );
  if ( !a || index < 0 || index >= a->nframes )
    return ( NULL );
  return ( (IImage) a->frames[index] );
}

int IAnimationFrameDelay ( IAnimation anim, int index )
{
  IAnimationP *a = _anim_valid ( anim );
  if ( !a || index < 0 || index >= a->nframes )
    return ( 0 );
  return ( a->delays[index] );
}

IError ISetAnimationLoopCount ( IAnimation anim, int loops )
{
  IAnimationP *a = _anim_valid ( anim );
  if ( !a )
    return ( IInvalidAnimation );
  a->loop_count = ( loops < 0 ) ? 0 : loops;
  return ( INoError );
}

int IAnimationLoopCount ( IAnimation anim )
{
  IAnimationP *a = _anim_valid ( anim );
  return ( a ? a->loop_count : 0 );
}

IAnimation IReadAnimationFile ( FILE *fp, IFileFormat format )
{
  if ( !fp )
    return ( NULL );
  if ( format == IFORMAT_GIF ) {
#ifdef HAVE_GIFLIB
    IAnimation anim = NULL;
    if ( _IReadAnimGIF ( fp, &anim ) == INoError )
      return ( anim );
    return ( NULL );
#else
    return ( NULL );
#endif
  }
  return ( NULL );
}

IError IWriteAnimationFile ( FILE *fp, IAnimation anim, IFileFormat format,
  IOptions options )
{
  IAnimationP *a = _anim_valid ( anim );
  if ( !a )
    return ( IInvalidAnimation );
  if ( !fp )
    return ( IInvalidArgument );
  if ( a->nframes <= 0 )
    return ( IInvalidAnimation );
  if ( format == IFORMAT_GIF ) {
#ifdef HAVE_GIFLIB
    return ( _IWriteAnimGIF ( fp, anim, options ) );
#else
    (void) options;
    return ( INoGIFSupport );
#endif
  }
  return ( IInvalidFormat );
}
