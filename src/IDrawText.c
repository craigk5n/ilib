/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IDrawText.c
 *
 * Image library
 *
 * Description:
 *	Multi-line text layout on top of IDrawString: word-wrap a string to a
 *	pixel width and draw it with per-line horizontal alignment. Wrapping
 *	happens at spaces; existing newlines start a new line; a single word
 *	wider than the wrap width is left to overflow rather than split.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "Ilib.h"
#include "IlibP.h"

/* Append a copy of [s, s+len) to a growing array of owned strings. Returns 0 on
   success, -1 on allocation failure. */
static int push_line ( char ***lines, int *n, int *cap, const char *s,
  int len )
{
  char *copy;
  if ( len < 0 )
    len = 0;
  if ( *n == *cap ) {
    int ncap = *cap ? *cap * 2 : 8;
    char **nl = (char **) realloc ( *lines, (size_t) ncap * sizeof ( char * ) );
    if ( !nl )
      return ( -1 );
    *lines = nl;
    *cap = ncap;
  }
  copy = (char *) malloc ( (size_t) len + 1 );
  if ( !copy )
    return ( -1 );
  if ( len > 0 )
    memcpy ( copy, s, (size_t) len );
  copy[len] = '\0';
  ( *lines )[( *n )++] = copy;
  return ( 0 );
}

static void free_lines ( char **lines, int n )
{
  int i;
  for ( i = 0; i < n; i++ )
    free ( lines[i] );
  free ( lines );
}

static int line_width ( IGC gc, IFont font, const char *s, int len )
{
  unsigned int w = 0;
  ITextWidth ( gc, font, (char *) s, (unsigned int) len, &w );
  return ( (int) w );
}

/* Word-wrap one logical line [s, e) (no embedded newline) to max_width pixels
   (0 = never wrap); appends each resulting line. Spaces are the break points;
   a word wider than max_width overflows on its own line. */
static int wrap_logical ( IGC gc, IFont font, const char *buf, int s, int e,
  unsigned int max_width, char ***lines, int *n, int *cap )
{
  int i = s, start = -1, last_end = -1;

  while ( i < e ) {
    int ws = i, we;
    while ( ws < e && buf[ws] == ' ' )
      ws++;
    if ( ws >= e )
      break;
    we = ws;
    while ( we < e && buf[we] != ' ' )
      we++;

    if ( start < 0 ) {
      start = ws;
      last_end = we;
    }
    else if ( max_width == 0 ||
              (unsigned int) line_width ( gc, font, buf + start, we - start ) <=
                max_width ) {
      last_end = we; /* the word still fits on the current line */
    }
    else {
      if ( push_line ( lines, n, cap, buf + start, last_end - start ) != 0 )
        return ( -1 );
      start = ws;
      last_end = we;
    }
    i = we;
  }

  if ( start >= 0 )
    return ( push_line ( lines, n, cap, buf + start, last_end - start ) );
  return ( push_line ( lines, n, cap, "", 0 ) ); /* preserve a blank line */
}

/* Split text into logical lines on '\n', word-wrapping each to max_width. */
static IError wrap_text ( IGC gc, IFont font, const char *text,
  unsigned int len, unsigned int max_width, char ***lines_out, int *n_out )
{
  char **lines = NULL;
  int n = 0, cap = 0, ls = 0, i;

  for ( i = 0; i <= (int) len; i++ ) {
    if ( i == (int) len || text[i] == '\012' ) {
      if ( wrap_logical ( gc, font, text, ls, i, max_width, &lines, &n,
             &cap ) != 0 ) {
        free_lines ( lines, n );
        return ( IInvalidArgument );
      }
      ls = i + 1;
    }
  }
  *lines_out = lines;
  *n_out = n;
  return ( INoError );
}

IError ITextBoxDimensions ( IGC gc, IFont font, char *text, unsigned int len,
  unsigned int max_width, unsigned int *width_return,
  unsigned int *height_return, int *lines_return )
{
  char **lines = NULL;
  int n = 0, i, maxw = 0;
  unsigned int fh = 0;
  IError ret;

  if ( width_return )
    *width_return = 0;
  if ( height_return )
    *height_return = 0;
  if ( lines_return )
    *lines_return = 0;
  if ( !font )
    return ( IInvalidFont );

  ret = wrap_text ( gc, font, text, len, max_width, &lines, &n );
  if ( ret != INoError )
    return ( ret );

  IFontSize ( font, &fh );
  for ( i = 0; i < n; i++ ) {
    int w = line_width ( gc, font, lines[i], (int) strlen ( lines[i] ) );
    if ( w > maxw )
      maxw = w;
  }
  free_lines ( lines, n );

  if ( width_return )
    *width_return = (unsigned int) maxw;
  if ( height_return )
    *height_return = fh * (unsigned int) n;
  if ( lines_return )
    *lines_return = n;
  return ( INoError );
}

IError IDrawText ( IImage image, IGC gc, int x, int y, char *text,
  unsigned int len, unsigned int max_width, IHAlign align )
{
  IGCP *gcp = (IGCP *) gc;
  IFont font;
  char **lines = NULL;
  int n = 0, i, box = (int) max_width;
  unsigned int fh = 0;
  IError ret;

  if ( !gcp || gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !gcp->font )
    return ( INoFontSet );
  font = (IFont) gcp->font;

  ret = wrap_text ( gc, font, text, len, max_width, &lines, &n );
  if ( ret != INoError )
    return ( ret );

  IFontSize ( font, &fh );

  /* For left alignment (or no wrap box) draw at x; otherwise center/right each
     line within the box (the wrap width, or the widest line when unwrapped). */
  if ( box <= 0 && align != IALIGN_LEFT ) {
    for ( i = 0; i < n; i++ ) {
      int w = line_width ( gc, font, lines[i], (int) strlen ( lines[i] ) );
      if ( w > box )
        box = w;
    }
  }

  for ( i = 0; i < n; i++ ) {
    int off = 0, ly = y + i * (int) fh;
    unsigned int llen = (unsigned int) strlen ( lines[i] );
    if ( align != IALIGN_LEFT ) {
      int w = line_width ( gc, font, lines[i], (int) llen );
      off = ( align == IALIGN_CENTER ) ? ( box - w ) / 2 : ( box - w );
    }
    ret = IDrawString ( image, gc, x + off, ly, lines[i], llen );
    if ( ret != INoError )
      break;
  }

  free_lines ( lines, n );
  return ( ret );
}
