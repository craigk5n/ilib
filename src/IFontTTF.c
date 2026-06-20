/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IFontTTF.c
 *
 * Image library
 *
 * Description:
 *	Scalable (TrueType/OpenType) font support via FreeType. Glyphs are
 *	rendered anti-aliased and composited with the GC foreground using the
 *	source-over blend primitive (_IBlendPoint), so text gets the glyph's
 *	grayscale coverage as its alpha.
 *
 *	Only compiled when FreeType is available (HAVE_FREETYPE).
 *
 ****************************************************************************/

#ifdef HAVE_FREETYPE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Ilib.h"
#include "IlibP.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* One loaded face, keyed by the caller's name. */
typedef struct {
  char *name;
  FT_Face face;
  int pixel_size;
} TTFEntry;

static FT_Library ttf_library = NULL;
static int ttf_library_ready = 0;
static TTFEntry *ttf_fonts = NULL;
static int ttf_num_fonts = 0;

static TTFEntry *ttf_find ( const char *name )
{
  int i;
  for ( i = 0; i < ttf_num_fonts; i++ ) {
    if ( ttf_fonts[i].name && strcmp ( ttf_fonts[i].name, name ) == 0 )
      return ( &ttf_fonts[i] );
  }
  return ( NULL );
}

IError _IFontTTFLoad ( char *name, char *path, int pixel_size )
{
  FT_Face face;
  TTFEntry *entry, *grown;

  if ( !ttf_library_ready ) {
    if ( FT_Init_FreeType ( &ttf_library ) )
      return ( IFontError );
    ttf_library_ready = 1;
  }

  if ( pixel_size < 1 )
    pixel_size = 1;

  if ( FT_New_Face ( ttf_library, path, 0, &face ) )
    return ( INoSuchFile );
  if ( FT_Set_Pixel_Sizes ( face, 0, (FT_UInt) pixel_size ) ) {
    FT_Done_Face ( face );
    return ( IFontError );
  }

  /* Replace an existing entry of the same name, or append a new one. */
  entry = ttf_find ( name );
  if ( entry ) {
    FT_Done_Face ( entry->face );
  }
  else {
    grown = (TTFEntry *) realloc ( ttf_fonts,
      ( ttf_num_fonts + 1 ) * sizeof ( TTFEntry ) );
    if ( !grown ) {
      FT_Done_Face ( face );
      return ( IFontError );
    }
    ttf_fonts = grown;
    entry = &ttf_fonts[ttf_num_fonts++];
    entry->name = (char *) malloc ( strlen ( name ) + 1 );
    if ( !entry->name ) {
      FT_Done_Face ( face );
      ttf_num_fonts--;
      return ( IFontError );
    }
    strcpy ( entry->name, name );
  }
  entry->face = face;
  entry->pixel_size = pixel_size;
  return ( INoError );
}

IError _IFontTTFGetSize ( char *name, unsigned int *height_return )
{
  TTFEntry *entry = ttf_find ( name );
  if ( !entry )
    return ( INoSuchFont );
  *height_return = (unsigned int) entry->pixel_size;
  return ( INoError );
}

/* Draw text with (x,y) as the left end of the baseline, rotated by `angle`
   degrees (counterclockwise on screen). Each glyph is rendered to an 8-bit
   coverage bitmap (rotated by FreeType) and composited with the GC foreground.
   The pen tracks the baseline in floating point; FreeType's transformed advance
   is in 26.6 fixed point and is in y-up space, so the y component is negated to
   move on the y-down image. */
IError _IFontTTFDrawString ( IImageP *image, IGCP *gc, int x, int y, char *text,
  unsigned int len, double angle )
{
  TTFEntry *entry;
  FT_Face face;
  FT_Matrix matrix;
  double rad, pen_x, pen_y;
  unsigned int i;

  if ( !gc->font )
    return ( INoFontSet );
  entry = ttf_find ( ( (IFontP *) gc->font )->name );
  if ( !entry )
    return ( INoSuchFont );
  if ( !gc->foreground )
    return ( IInvalidGC );
  face = entry->face;

  /* Rotation in FreeType's y-up space. We flip y at draw/advance time, so a
     positive angle reads counterclockwise on the y-down image. */
  rad = angle * PI / 180.0;
  matrix.xx = (FT_Fixed) ( cos ( rad ) * 0x10000L );
  matrix.xy = (FT_Fixed) ( -sin ( rad ) * 0x10000L );
  matrix.yx = (FT_Fixed) ( sin ( rad ) * 0x10000L );
  matrix.yy = (FT_Fixed) ( cos ( rad ) * 0x10000L );
  FT_Set_Transform ( face, &matrix, NULL );

  pen_x = x;
  pen_y = y;

  for ( i = 0; i < len; i++ ) {
    FT_GlyphSlot slot;
    int gx, gy, row, col;

    if ( FT_Load_Char ( face, (FT_ULong) (unsigned char) text[i],
           FT_LOAD_RENDER ) )
      continue; /* skip glyphs that fail to load */
    slot = face->glyph;

    /* bitmap_left/top are relative to the (rotated) pen origin, in y-up space. */
    gx = (int) ( pen_x + 0.5 ) + slot->bitmap_left;
    gy = (int) ( pen_y + 0.5 ) - slot->bitmap_top;

    for ( row = 0; row < (int) slot->bitmap.rows; row++ ) {
      unsigned char *src = slot->bitmap.buffer + row * slot->bitmap.pitch;
      for ( col = 0; col < (int) slot->bitmap.width; col++ ) {
        unsigned int cover = src[col]; /* 0..255 grayscale coverage */
        if ( cover )
          _IBlendPoint ( image, gc, gx + col, gy + row, cover );
      }
    }

    /* Advance along the rotated baseline (26.6 -> pixels; y-up -> y-down). */
    pen_x += (double) slot->advance.x / 64.0;
    pen_y -= (double) slot->advance.y / 64.0;
  }

  FT_Set_Transform ( face, NULL, NULL ); /* reset for the next caller */

  return ( INoError );
}

void _IFontTTFFree ( char *name )
{
  TTFEntry *entry = ttf_find ( name );
  if ( !entry )
    return;
  FT_Done_Face ( entry->face );
  free ( entry->name );
  entry->name = NULL;
  entry->face = NULL;
}

#endif /* HAVE_FREETYPE */
