/* SPDX-License-Identifier: GPL-2.0-only */
/*
** Routines for reading in a BDF (font) file.
** BDF files can be obtained as part of the X Windows System
** at ftp://ftp.x.org/
**
** Copyright 1999 Craig Knudsen
**
** 12-Apr-1999	Craig Knudsen	cknudsen@cknudsen.com
**		Added support for high ascii characters.
**		(This has disabled usage of the escape sequences like
**		"\033agrave".)
** 17-May-1996	Craig Knudsen   cknudsen@cknudsen.com
**		Created
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "Ilib.h"
#include "IlibP.h"
#include "IFontBDF.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif


/*
** Define the BDF translations into standard ASCII.
** Note we will still handle non-ASCII characters.
** Use "\033agrave;" to get the agrave symbol.
*/
typedef struct {
  char *name;
  unsigned char value;
} char_translation;

char_translation translation[] = {
  { "space", ' ' },
  { "exclam", '!' },
  { "quotedbl", '\"' },
  { "numbersign", '#' },
  { "dollar", '$' },
  { "percent", '%' },
  { "ampersand", '&' },
  { "quoteright", '\'' },
  { "parenleft", '(' },
  { "parenright", ')' },
  { "asterisk", '*' },
  { "plus", '+' },
  { "comma", ',' },
  { "minus", '-' },
  { "period", '.' },
  { "slash", '/' },
  { "zero", '0' },
  { "one", '1' },
  { "two", '2' },
  { "three", '3' },
  { "four", '4' },
  { "five", '5' },
  { "six", '6' },
  { "seven", '7' },
  { "eight", '8' },
  { "nine", '9' },
  { "colon", ':' },
  { "semicolon", ';' },
  { "less", '<' },
  { "equal", '=' },
  { "greater", '>' },
  { "question", '?' },
  { "at", '@' },
  { "bracketleft", '[' },
  { "backslash", '\\' },
  { "bracketright", ']' },
  { "asciicircum", '^' },
  { "underscore", '_' },
  { "quoteleft", '`' },
  { "braceleft", '{' },
  { "bar", '|' },
  { "braceright", '}' },
  { "asciitilde", '~' },
  { NULL, '\0' },
};

static char *trans_table[256];

/* Upper bound on a single BDF glyph's width/height (pixels). Real glyphs are
   far smaller; this guards the data-buffer allocation against overflow from
   bogus BBX values in an untrusted font file. */
#define IFONT_MAX_GLYPH_DIM 4096

typedef struct {
  char *name;                /* translated name ("A", "B", "\033agrave;") */
  unsigned int *data;        /* character definition */
  unsigned int width;        /* character width */
  unsigned int height;       /* height (size of lines array) */
  unsigned int actual_width; /* width with padding on left and right */
  int xoffset;               /* pixels to move to the right before drawing */
  int yoffset;               /* pixels to move to up before drawing */
} Char;

typedef struct {
  char *name;       /* font name */
  char *family;     /* font family ("Times") */
  char *face_name;  /* face name ("Times Roman") */
  char *width_name; /* face name ("Normal") */
  char *weight;     /* font weight ("R") */
  int proportional; /* yes or no? (fixed/proportional font) */
  int pixel_size;
  int font_ascent;
  int font_descent;
  Char *chars[256];             /* list of chars */
  Char **other_chars;           /* non ASCII chars */
  unsigned int num_other_chars; /* size of above array */
} Font;


static void free_character ( Char *ch );
static void free_font ( Font *font );

static Font **fonts = NULL;
static int num_fonts = 0;

/* One-entry lookup cache for IFontBDFGetChar. File-scope (not function-local)
   so IFontBDFFree can invalidate it -- otherwise a freed font's name pointer
   could be reused by a later allocation, making the cache return the freed
   (dangling) font. */
static char *bdf_last_name = NULL;
static Font *bdf_last_font = NULL;

IError IFontBDFReadFile ( char *name, char *path )
{
  struct stat buf;
  FILE *fp;
  char text[256];
  IError ret;
  char **lines, **tmp;
  int num_lines = 0, loop;

  if ( stat ( path, &buf ) == 0 ) {
    fp = fopen ( path, "r" );
    if ( !fp )
      return ( INoSuchFile );
    lines = malloc ( 1 );
    if ( !lines ) {
      fclose ( fp );
      return ( IFontError );
    }
    while ( fgets ( text, 256, fp ) ) {
      tmp = realloc ( lines, ( num_lines + 1 ) * sizeof ( char * ) );
      if ( !tmp ) {
        for ( loop = 0; loop < num_lines; loop++ )
          free ( lines[loop] );
        free ( lines );
        fclose ( fp );
        return ( IFontError );
      }
      lines = tmp;
      if ( text[strlen ( text ) - 1] == '\012' )
        text[strlen ( text ) - 1] = '\0';
      if ( text[strlen ( text ) - 1] == '\015' )
        text[strlen ( text ) - 1] = '\0';
      lines[num_lines] = (char *) malloc ( strlen ( text ) + 1 );
      strcpy ( lines[num_lines++], text );
    }
    fclose ( fp );
    tmp = realloc ( lines, ( num_lines + 1 ) * sizeof ( char * ) );
    if ( !tmp ) {
      for ( loop = 0; loop < num_lines; loop++ )
        free ( lines[loop] );
      free ( lines );
      return ( IFontError );
    }
    lines = tmp;
    lines[num_lines++] = NULL;
    ret = IFontBDFReadData ( name, lines );
    for ( loop = 0; lines[loop]; loop++ )
      free ( lines[loop] );
    free ( lines );
    return ( ret );
  }
  else {
    fprintf ( stderr, "Cannot find font file: %s\n", path );
    return ( INoSuchFile );
  }
}


IError IFontBDFReadData ( char *name, char **lines )
{
  char ch[256], *text;
  int temp;
  int in_bitmap = 0, hexval;
  int loop, which_char, which_bit;
  char c;
  Font *font;
  Char *character = NULL;
  int xpos, ypos = 0;
  static int first = 1;
  int line_no = 0;

  /* Initialize a few things the first time this is called */
  if ( first ) {
    first = 0;
    memset ( trans_table, '\0', sizeof ( trans_table ) );
    for ( loop = 0; translation[loop].name; loop++ ) {
      trans_table[translation[loop].value] = translation[loop].name;
    }
  }

  font = (Font *) malloc ( sizeof ( Font ) );
  memset ( font, '\0', sizeof ( Font ) );
  font->name = (char *) malloc ( strlen ( name ) + 1 );
  strcpy ( font->name, name );
  memset ( font->chars, '\0', sizeof ( font->chars ) );
  font->other_chars = malloc ( 4 );

  while ( lines[line_no] ) {
    text = lines[line_no];
    if ( strncmp ( text, "STARTCHAR", 9 ) == 0 ) {
      strcpy ( ch, text + 10 );
      for ( loop = 0; translation[loop].name; loop++ ) {
        if ( strcmp ( translation[loop].name, ch ) == 0 ) {
          ch[0] = translation[loop].value;
          ch[1] = '\0';
          break;
        }
      }
      in_bitmap = 0;
      /* A previous STARTCHAR with no ENDCHAR leaves a dangling character. */
      if ( character )
        free_character ( character );
      character = (Char *) malloc ( sizeof ( Char ) );
      memset ( character, '\0', sizeof ( Char ) );
      character->name = (char *) malloc ( strlen ( ch ) + 1 );
      strcpy ( character->name, ch );
    }
    else if ( strncmp ( text, "ENCODING", 8 ) == 0 ) {
      temp = atoi ( text + 9 );
      if ( temp > 0 && temp < 256 && character != NULL ) {
        sprintf ( ch, "%c", temp );
        strcpy ( character->name, ch );
      }
    }
    else if ( strncmp ( text, "PIXEL_SIZE", 10 ) == 0 ) {
      font->pixel_size = atoi ( text + 11 );
    }
    else if ( strncmp ( text, "FONT_ASCENT", 11 ) == 0 ) {
      font->font_ascent = atoi ( text + 12 );
    }
    else if ( strncmp ( text, "FONT_DESCENT", 12 ) == 0 ) {
      font->font_descent = atoi ( text + 13 );
    }
    else if ( strncmp ( text, "SPACING", 7 ) == 0 ) {
      if ( strncmp ( text + 8, "\"P\"", 3 ) == 0 )
        font->proportional = TRUE;
      else
        font->proportional = FALSE;
    }
    else if ( strncmp ( text, "FACE_NAME", 9 ) == 0 ) {
      font->face_name = (char *) malloc ( strlen ( text + 10 ) + 1 );
      strcpy ( font->face_name, text + 10 );
    }
    else if ( strncmp ( text, "SETWIDTH_NAME", 13 ) == 0 ) {
      font->width_name = (char *) malloc ( strlen ( text + 14 ) + 1 );
      strcpy ( font->width_name, text + 14 );
    }
    else if ( strncmp ( text, "BBX", 3 ) == 0 && character != NULL ) {
      sscanf ( text, "BBX %d %d %d %d", &character->width, &character->height,
        &character->xoffset, &character->yoffset );
      /* Reject absurd glyph dimensions: untrusted BBX values would otherwise
         overflow the int multiplication below and under-allocate the buffer. */
      if ( character->width > IFONT_MAX_GLYPH_DIM ||
           character->height > IFONT_MAX_GLYPH_DIM ) {
        character->width = 0;
        character->height = 0;
      }
      /* use the width and height to allocate space for the data */
      if ( character->width > 0 && character->height > 0 )
        character->data = (unsigned int *) calloc (
          (size_t) character->height * character->width,
          sizeof ( unsigned int ) );
    }
    else if ( strncmp ( text, "DWIDTH", 6 ) == 0 && character != NULL ) {
      sscanf ( text, "DWIDTH %d %d", &character->actual_width, &temp );
    }
    else if ( strcmp ( text, "BITMAP" ) == 0 ) {
      /*
            if ( character->width <= 0 || character->height <= 0 ) {
              fprintf ( stderr, "BBX line not found. (line %d)\n", line_no + 1 );
              return ( IFontError );
            }
      */
      in_bitmap = 1;
      ypos = 0;
    }
    else if ( strcmp ( text, "ENDCHAR" ) == 0 ) {
      if ( in_bitmap ) {
        in_bitmap = 0;
        if ( !character ) {
          free_font ( font );
          return ( IFontError );
        }
        if ( strlen ( character->name ) == 1 )
          font->chars[(unsigned char) character->name[0]] = character;
        else {
          Char **ctmp = (Char **) realloc ( font->other_chars,
            ( font->num_other_chars + 1 ) * sizeof ( Char * ) );
          if ( !ctmp ) {
            free_character ( character );
            free_font ( font );
            return ( IFontError );
          }
          font->other_chars = ctmp;
          font->other_chars[font->num_other_chars++] = character;
        }
        character = NULL; /* ownership transferred to font */
      }
      /*
            for ( loop = 0; loop < character->width * character->height; loop++ ) {
              if ( character->data[loop] )
                printf ( "+" );
              else
                printf ( "-" );
            }
            printf ( "\n-------------------\n" );
      */
    }
    else if ( in_bitmap && character != NULL && character->data != NULL ) {
      xpos = 0;
      for ( loop = 0; (unsigned int) loop < character->width; loop++ ) {
        which_char = loop / 4;
        c = text[which_char];
        if ( isdigit ( c ) )
          hexval = (int) ( c - '0' );
        else
          hexval = (int) ( c - 'A' ) + 10;
        which_bit = 3 - ( loop % 4 );
        if ( hexval & ( 1 << which_bit ) ) {
          character->data[ypos * character->width + xpos] = 1;
          /*printf ( "X" );*/
        }
        else {
          character->data[ypos * character->width + xpos] = 0;
          /*printf ( " " );*/
        }
        xpos++;
      }
      ypos++;
      /*printf ( "\n" );*/
    }
    line_no++;
  }

  /* A trailing STARTCHAR with no ENDCHAR leaves an unattached character. */
  if ( character )
    free_character ( character );

  /* Now add this font to the tree of loaded fonts */
  {
    Font **ftmp;
    if ( !fonts )
      ftmp = (Font **) malloc ( sizeof ( Font * ) );
    else
      ftmp = (Font **) realloc ( fonts,
        ( num_fonts + 1 ) * sizeof ( Font * ) );
    if ( !ftmp ) {
      free_font ( font );
      return ( IFontError );
    }
    fonts = ftmp;
  }
  fonts[num_fonts++] = font;

  /* return success */
  return ( INoError );
}

Font *_IGetFont ( char *name )
{
  int loop;

  if ( num_fonts ) {
    for ( loop = 0; loop < num_fonts; loop++ ) {
      if ( fonts[loop] ) {
        if ( strcmp ( fonts[loop]->name, name ) == 0 )
          return ( (Font *) fonts[loop] );
      }
    }
  }

  return ( NULL );
}


static void free_character ( Char *ch )
{
  free ( ch->name );
  free ( ch->data );
  free ( ch );
}


/* Free a font and everything it owns. Does NOT touch the global cache. */
static void free_font ( Font *font )
{
  unsigned int loop;

  if ( !font )
    return;
  for ( loop = 0; loop < 256; loop++ ) {
    if ( font->chars[loop] )
      free_character ( font->chars[loop] );
  }
  for ( loop = 0; loop < font->num_other_chars; loop++ ) {
    free_character ( font->other_chars[loop] );
  }
  free ( font->other_chars );
  free ( font->name );
  free ( font->family );
  free ( font->face_name );
  free ( font->width_name );
  free ( font->weight );
  free ( font );
}


IError IFontBDFFree ( char *name )
{
  Font *font;
  unsigned int loop;

  font = _IGetFont ( name );
  if ( !font )
    return ( INoSuchFont );

  /* Remove this font from the registry *before* freeing it, so a later
     _IGetFont() does not walk a dangling pointer (use-after-free). */
  for ( loop = 0; loop < (unsigned int) num_fonts; loop++ ) {
    if ( fonts[loop] == font )
      fonts[loop] = NULL;
  }

  /* Invalidate the IFontBDFGetChar lookup cache if it points at this font;
     otherwise a reused name pointer could resolve to this freed font. */
  if ( bdf_last_font == font ) {
    bdf_last_name = NULL;
    bdf_last_font = NULL;
  }

  free_font ( font );

  return ( INoError );
}

IError _IFontBDFGetSize ( char *name, unsigned int *height_return )
{
  Font *font;

  font = _IGetFont ( name );
  if ( !font )
    return ( INoSuchFont );

  *height_return = font->pixel_size;

  return ( INoError );
}


IError IFontBDFGetChar ( char *name, char *ch, unsigned int **bitdata,
  unsigned int *width, unsigned int *height, unsigned int *actual_width,
  unsigned int *size, int *xoffset, int *yoffset )
{
  Font *font;
  Char *character = NULL;
  unsigned int loop;

  if ( !ch || !strlen ( ch ) )
    return ( IInvalidArgument );

  if ( bdf_last_name == name && bdf_last_font != NULL ) {
    font = bdf_last_font;
  }
  else {
    font = _IGetFont ( name );
    if ( !font )
      return ( INoSuchFont );
    bdf_last_name = name;
    bdf_last_font = font;
  }

  if ( !font )
    return ( INoSuchFont );

  /* we got the font.  now get the character */
  if ( strlen ( ch ) == 1 ) {
    character = font->chars[(unsigned char) ch[0]];
  }
  /* if more than a single char, must not be in the ascii list */
  else {
    for ( loop = 0, character = NULL; loop < font->num_other_chars; loop++ ) {
      if ( strcmp ( ch, font->other_chars[loop]->name ) == 0 ) {
        character = font->other_chars[loop];
        break;
      }
    }
    if ( !character )
      character = font->chars[' '];
  }
  /* if still not there, use space */
  if ( !character ) {
    /* This should not happen if font is defined correctly */
    return ( IFontError );
  }

  *height = character->height;
  *width = character->width;
  *actual_width = character->actual_width;
  *size = font->pixel_size;
  *xoffset = character->xoffset;
  *yoffset = character->yoffset;
  *bitdata = character->data;

  return ( INoError );
}
