/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IBMP.c
 *
 * Image library
 *
 * Description:
 *	Read BMP files.
 *
 * History:
 *	01-Apr-00	Jim Winstead	jimw@trainedmonkey.com
 *	Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include "Ilib.h"
#include "IlibP.h"

#define BMP_HEADER ('B' + ('M' << 8))

/* Compression formats */
#define BI_RGB                0
#define BI_RLE8                1
#define BI_RLE4                2
#define BI_BITFIELDS        3

#define SET_PIXEL(buffer, byte) \
	do { \
		*((buffer)+0) = (colortable[(byte)&0xff]&(0xff<<16))>>16; \
		*((buffer)+1) = (colortable[(byte)&0xff]&(0xff<<8))>>8; \
		*((buffer)+2) = (colortable[(byte)&0xff]&(0xff<<0))>>0; \
	} while (0);

static int ReadShort (FILE *fp, int *ret)
{
  int c = fgetc(fp);
  if (c == EOF) return 0;
  *ret = (c&0xff);
  c = fgetc(fp);
  if (c == EOF) return 0;
  *ret |= (c&0xff) << 8;
  return 1;
}

static int ReadLong (FILE *fp, int *ret)
{
  int c = fgetc(fp);
  if (c == EOF) return 0;
  *ret = (c&0xff);
  c = fgetc(fp);
  if (c == EOF) return 0;
  *ret |= (int)((unsigned int)(c&0xff) << 8);
  c = fgetc(fp);
  if (c == EOF) return 0;
  *ret |= (int)((unsigned int)(c&0xff) << 16);
  c = fgetc(fp);
  if (c == EOF) return 0;
  /* shift as unsigned: (c&0xff)<<24 would overflow signed int when bit 7 set */
  *ret |= (int)((unsigned int)(c&0xff) << 24);
  return 1;
}

IError _IReadBMP ( FILE *fp, IOptions options, IImageP **image_return )
{
  IImageP *image = NULL;
  int scratch, fileSize, offset, w, h, depth, compression,
      imagesize, xpelspermeter, ypelspermeter, colorsused, colorsimportant;
  int redmask = 0, greenmask = 0, bluemask = 0,
      redshift = 0, greenshift = 0, blueshift = 0,
      redbits = 0, greenbits = 0, bluebits = 0;
  int *colortable = NULL;

  /* read the header and make sure this is a BMP file */
  if (!ReadShort(fp, &scratch) || scratch != BMP_HEADER) goto fail;

  /* read the file size (not sure this is useful) */
  if (!ReadLong(fp, &fileSize)) goto fail;

  /* read the two reserved fields (must be zero, according to spec) */
  if (!ReadShort(fp, &scratch) || scratch != 0) goto fail;
  if (!ReadShort(fp, &scratch) || scratch != 0) goto fail;

  /* read the offset of the actual graphic bits */
  if (!ReadLong(fp, &offset)) goto fail;

  /*fprintf(stderr, "offset = %d\n", offset);*/

  /* verify that the header is 40 bytes */
  /* XXX: really should save this and just skip bytes we don't understand. */
  /* and we could be using this to detect things in OS/2 format */
  if (!ReadLong(fp, &scratch) || scratch != 40) goto fail;

  /* get image size and depth */
  if (!ReadLong(fp, &w)) goto fail;
  if (!ReadLong(fp, &h)) goto fail;
  if (!ReadShort(fp, &scratch) || scratch != 1) goto fail;
  if (!ReadShort(fp, &depth)) goto fail;

  /*fprintf(stderr, "image size: %dx%dx%d\n", w, h, depth);*/

  if (!ReadLong(fp, &compression)) goto fail;
  if (!ReadLong(fp, &imagesize)) goto fail;
  if (!ReadLong(fp, &xpelspermeter)) goto fail;
  if (!ReadLong(fp, &ypelspermeter)) goto fail;
  if (!ReadLong(fp, &colorsused)) goto fail;
  if (!ReadLong(fp, &colorsimportant)) goto fail;

  /* we have read 40 bytes of header */
  /* we have read 54 bytes of data from the file */

/*
  fprintf(stderr, "compression=%d, imagesize=%d, colorsused=%d\n",
      compression,
      imagesize,
      colorsused);
*/

  /* get the masks, shifts, and bits for bitfields (16 and 32 bits) */
  if (compression == BI_BITFIELDS) {
    /* Shift as unsigned and bound the shift count *before* shifting: a value
       comes from the file, and 1<<31 (signed) or 1<<>=32 is undefined. */
    if (!ReadLong(fp,&redmask)) goto fail;
    for (redshift = 0; redshift<32 && !(redmask&(1U<<redshift)); redshift++);
    for (redbits = 0; (redshift+redbits)<32 && (redmask&(1U<<(redshift+redbits))); redbits++);
    if (!ReadLong(fp,&greenmask)) goto fail;
    for (greenshift = 0; greenshift<32 && !(greenmask&(1U<<greenshift)); greenshift++);
    for (greenbits = 0; (greenshift+greenbits)<32 && (greenmask&(1U<<(greenshift+greenbits))); greenbits++);
    if (!ReadLong(fp,&bluemask)) goto fail;
    for (blueshift = 0; blueshift<32 && !(bluemask&(1U<<blueshift)); blueshift++);
    for (bluebits = 0; (blueshift+bluebits)<32 && (bluemask&(1U<<(blueshift+bluebits))); bluebits++);
    /*fprintf(stderr, "masks: %x %x %x\n", redmask, greenmask, bluemask);*/
    /*fprintf(stderr, "shifts: %d %d %d\n", redshift, greenshift, blueshift);*/
    /*fprintf(stderr, "bits: %d %d %d\n", redbits, greenbits, bluebits);*/
  }

  if (depth <= 8) {
    /* depth/colorsused are untrusted: avoid a negative-shift UB and an
       unbounded (or negative) colormap allocation. A <=8-bit palette has at
       most 2<<depth entries by this reader's own convention. */
    if (depth < 1) goto fail;
    if (!colorsused) colorsused = 2<<depth;
    if (colorsused < 0 || colorsused > (2<<depth)) goto fail;
    /* Always allocate >=256 entries (zero-filled): the pixel decoders index
       colortable[byte & 0xff], i.e. up to 255, regardless of colorsused. */
    colortable = calloc((colorsused < 256 ? 256 : colorsused), sizeof(int));
    if (!colortable) goto fail;
    for (scratch = 0; scratch < colorsused; scratch++) {
      if (!ReadLong(fp,(colortable+scratch))) goto fail;
    }
  }

  /* seek to the beginning of the image data */
  if (fseek(fp, offset, SEEK_SET) < 0) goto fail;

  /* Validate untrusted dimensions: the decode loops below assume positive
     w/h, and w*h*3 must not overflow. */
  if ( w <= 0 || h <= 0 || w > INT_MAX / 3 / h )
    goto fail;

  /* create a new image object */
  image = (IImageP *) ICreateImage ( w, h, options );
  if ( ! image )
    goto fail;

  if (depth == 24 && compression == BI_RGB) {
    int x, y;
    /* slurp in the data */
    for (y = h - 1; y >= 0; y--) {
      for (x = 0; x < w; x++) {
        int r,g,b;
        r = fgetc(fp); g = fgetc(fp); b = fgetc(fp);
        /* this leaks the image object if we hit a premature EOF */
        if (r == EOF || g == EOF || b == EOF) goto fail;
        *(image->data + (y*3*w) + (x*3)) = b;
        *(image->data + (y*3*w) + (x*3) + 1) = g;
        *(image->data + (y*3*w) + (x*3) + 2) = r;
      }
    }
  }
  else if (depth == 16 && compression == BI_BITFIELDS) {
    int x, y;
    /* slurp in the data */
    for (y = h - 1; y >= 0; y--) {
      for (x = 0; x < w; x++) {
        char *pixel = (char *)image->data + (y*3*w) + (x*3);
        int color;
        /* this leaks the image object if we hit a premature EOF */
        if (!ReadShort(fp, &color)) goto fail;
        /* scale 5/6 bit values to 8 bits */
        *(pixel + 0) = (((color&redmask)>>redshift)<<8)>>redbits;
        *(pixel + 1) = (((color&greenmask)>>greenshift)<<8)>>greenbits;
        *(pixel + 2) = (((color&bluemask)>>blueshift)<<8)>>bluebits;
      }
    }
  }
  else if (depth == 8 && compression == BI_RGB) {
    int x, y;
    /*fprintf(stderr, "8 bit, no compression\n");*/
    for (y = h - 1; y >= 0; y--) {
      for (x = 0; x < w; x++) {
        char *pixel = (char *)image->data+(y*w*3)+(x*3);
        int byte = fgetc(fp); if (byte == EOF) goto fail;
        SET_PIXEL(pixel, byte);
      }
    }

  } else if (depth <= 8 && compression) {
    int byte, count, x, y;
    char *data_start = (char *)image->data;
    char *data_end = (char *)image->data + (size_t)w * h * 3;
    char *buffer = (char *)image->data+w*(h-1)*3;
    /* bail on any RLE write that would land outside the pixel buffer */
#define BMP_RLE_INBOUNDS(b) ((b) >= data_start && (b) + 3 <= data_end)
    for (y = 0; y < h; ) {
      count = fgetc(fp);
      if (count == EOF) goto fail;
      if (count != 0) {
        /*fprintf(stderr, "stretch of %d bytes\n", count);*/
        byte = fgetc(fp); if (byte == EOF) goto fail;
        for (scratch = 0; scratch < count; scratch++) {
          if (!BMP_RLE_INBOUNDS(buffer)) goto fail;
          if (compression == 1) {
            SET_PIXEL(buffer, byte);
          }
          else {
            int thisbyte = (scratch & 0x01) ? (byte & 0x0f) : ((byte >> 4) & 0x0f);
            SET_PIXEL(buffer, thisbyte);
          }
          buffer += 3;
        }
      }
      else {
        count = fgetc(fp);
        if (count == EOF) goto fail;
        if (count == 0x01) break; /* end of bitmap */
        switch (count) {
        case 0x00: /* end of the line */
          /*fprintf(stderr, "hit end of line %d\n", y);*/
          y++;
          if (y < h)
            buffer = (char *)image->data+w*(h-y-1)*3;
          break;
        case 0x02: /* goto specific position */
          x = fgetc(fp);
          y = fgetc(fp);
          /*fprintf(stderr, "going to %d,%d\n", x, y);*/
          if (x == EOF || y == EOF) goto fail;
          if (x < 0 || x >= w || y < 0 || y >= h) goto fail;
          buffer = (char *)image->data+w*(h-y-1)*3+(x*3);
          break;
        default: /* a bunch of literal bytes */
          /*fprintf(stderr, "handling %d literal bytes\n", count);*/
          for (scratch = 0; scratch < count; scratch++) {
            byte = fgetc(fp); if (byte == EOF) goto fail;
            if (!BMP_RLE_INBOUNDS(buffer)) goto fail;
            if (compression == 1) {
              SET_PIXEL(buffer, byte);
            }
            else {
              int thisbyte = (scratch & 0x01) ? (byte & 0x0f) : ((byte >> 4) & 0x0f);
              SET_PIXEL(buffer, thisbyte);
            }
            buffer += 3;
          }
          /* handle padding */
          if (compression == 1) {
            if (count & 0x01) {
              /*fprintf(stderr, "eating padding\n");*/
              (void)fgetc(fp);
            }
          }
          else if ((count & 0x03) == 1 || ((count & 0x03) == 2)) {
            (void)fgetc(fp);
          }
          break;
        }
      }
    }
#undef BMP_RLE_INBOUNDS
  }
  else {
    goto fail;
  }

  if (colortable) free(colortable);
  *image_return = image;
  return ( INoError );

fail:
  if (colortable) free(colortable);
  if (image) _IFreeImage(image);
  return (IInvalidFormat);
}

