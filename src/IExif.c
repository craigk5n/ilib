/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IExif.c
 *
 * Image library
 *
 * Description:
 *	Minimal EXIF parsing -- just enough to recover the Orientation tag
 *	(0x0112) from a JPEG APP1 payload. The payload starts with the
 *	"Exif\0\0" identifier followed by a TIFF header (byte-order mark, magic,
 *	IFD0 offset) and the IFD0 entries. Everything is bounds-checked so
 *	malformed data returns "normal" (1) rather than reading out of range.
 *
 ****************************************************************************/

#include <string.h>

#include "Ilib.h"
#include "IlibP.h"

#define EXIF_TAG_ORIENTATION 0x0112

static unsigned int exif_rd16 ( const unsigned char *d, unsigned int off,
  int little )
{
  return ( little ? ( d[off] | ( d[off + 1] << 8 ) )
                  : ( ( d[off] << 8 ) | d[off + 1] ) );
}

static unsigned int exif_rd32 ( const unsigned char *d, unsigned int off,
  int little )
{
  return ( little ? ( (unsigned int) d[off] | ( (unsigned int) d[off + 1] << 8 ) |
                      ( (unsigned int) d[off + 2] << 16 ) |
                      ( (unsigned int) d[off + 3] << 24 ) )
                  : ( ( (unsigned int) d[off] << 24 ) |
                      ( (unsigned int) d[off + 1] << 16 ) |
                      ( (unsigned int) d[off + 2] << 8 ) |
                      (unsigned int) d[off + 3] ) );
}

/* Return the EXIF orientation (1..8) from a JPEG APP1 payload, or 1 if it is
   absent or the data is malformed. */
int _IExifOrientation ( const unsigned char *data, unsigned int len )
{
  unsigned int tiff, ifd, count, i;
  int little;

  if ( !data || len < 14 )
    return ( 1 );
  if ( memcmp ( data, "Exif\0\0", 6 ) != 0 )
    return ( 1 );

  tiff = 6; /* TIFF header begins right after the identifier */
  if ( data[tiff] == 'I' && data[tiff + 1] == 'I' )
    little = 1;
  else if ( data[tiff] == 'M' && data[tiff + 1] == 'M' )
    little = 0;
  else
    return ( 1 );

  /* IFD0 offset is relative to the start of the TIFF header. */
  if ( tiff + 8 > len )
    return ( 1 );
  ifd = tiff + exif_rd32 ( data, tiff + 4, little );
  if ( ifd < tiff || ifd + 2 > len )
    return ( 1 );

  count = exif_rd16 ( data, ifd, little );
  for ( i = 0; i < count; i++ ) {
    unsigned int e = ifd + 2 + i * 12;
    if ( e + 12 > len )
      break;
    if ( exif_rd16 ( data, e, little ) == EXIF_TAG_ORIENTATION ) {
      /* SHORT value sits in the first 2 bytes of the value field. */
      unsigned int v = exif_rd16 ( data, e + 8, little );
      return ( ( v >= 1 && v <= 8 ) ? (int) v : 1 );
    }
  }
  return ( 1 );
}
