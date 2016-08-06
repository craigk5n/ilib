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

typedef struct {
  char *name;			/* translated name ("A", "B", "\033agrave;") */
  unsigned int *data;		/* character definition */
  unsigned int width;		/* character width */
  unsigned int height;		/* height (size of lines array) */
  unsigned int actual_width;	/* width with padding on left and right */
  int xoffset;			/* pixels to move to the right before drawing */
  int yoffset;			/* pixels to move to up before drawing */
} Char;

typedef struct {
  char *name;			/* font name */
  char *family;			/* font family ("Times") */
  char *face_name;		/* face name ("Times Roman") */
  char *width_name;		/* face name ("Normal") */
  char *weight;			/* font weight ("R") */
  int proportional;		/* yes or no? (fixed/proportional font) */
  int pixel_size;
  int font_ascent;
  int font_descent;
  Char *chars[256];		/* list of chars */
  Char **other_chars;		/* non ASCII chars */
  unsigned int num_other_chars;	/* size of above array */
} Font;


static Font **fonts = NULL;
static int num_fonts = 0;

IError IFontBDFReadFile ( name, path )
char *name;
char *path;
{
  struct stat buf;
  FILE *fp;
  char text[256];
  IError ret;
  char **lines;
  int num_lines = 0, loop;

  if ( stat ( path, &buf ) == 0 ) {
    fp = fopen ( path, "r" );
    if ( !fp )
      return ( INoSuchFile );
    lines = malloc ( 1 );
    while ( fgets ( text, 256, fp ) ) {
      lines = realloc ( lines, ( num_lines + 1 ) * sizeof ( char * ) );
      if ( text[strlen(text)-1] == '\012' )
        text[strlen(text)-1] = '\0';
      if ( text[strlen(text)-1] == '\015' )
        text[strlen(text)-1] = '\0';
      lines[num_lines] = (char *) malloc ( strlen ( text ) + 1 );
      strcpy ( lines[num_lines++], text );
    }
    fclose ( fp );
    lines = realloc ( lines, ( num_lines + 1 ) * sizeof ( char * ) );
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


IError IFontBDFReadData ( name, lines )
char *name;
char **lines;
{
  char ch[256], *text;
  int temp;
  int in_bitmap = 0, hexval;
  int loop, which_char, which_bit;
  int num_ret;
  char c;
  Font *font;
  Char *character;
  int xpos, ypos;
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
      character = (Char *) malloc ( sizeof ( Char ) );
      memset ( character, '\0', sizeof ( Char ) );
      num_ret = 1;
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
    else if ( strncmp ( text, "BBX", 3 ) == 0 ) {
      sscanf ( text, "BBX %d %d %d %d", &character->width, &character->height,
        &character->xoffset, &character->yoffset );
      /* use the width and height to allocate space for the data */
      character->data = (unsigned int *) calloc ( character->height * character->width,
        sizeof ( int ) );
    }
    else if ( strncmp ( text, "DWIDTH", 6 ) == 0 ) {
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
      xpos = ypos = 0;
    } else if ( strcmp ( text, "ENDCHAR" ) == 0 ) {
      if ( in_bitmap ) {
        in_bitmap = 0;
        if ( ! character ) {
          return ( IFontError );
        }
        if ( strlen ( character->name ) == 1 )
          font->chars[(unsigned char)character->name[0]] = character;
        else {
          font->other_chars = (Char **) realloc ( font->other_chars,
            ( font->num_other_chars + 1 ) * sizeof ( Char * ) );
          font->other_chars[font->num_other_chars++] = character;
        }
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
    } else if ( in_bitmap ) {
      xpos = 0;
      for ( loop = 0; loop < character->width; loop++ ) {
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

  /* Now add this font to the tree of loaded fonts */
  if ( ! fonts )
     fonts = (Font **) malloc ( sizeof ( Font * ) );
  else
    fonts = (Font **) realloc ( fonts, 
    ( num_fonts + 1 ) * sizeof ( Font * ) );
  fonts[num_fonts++] = font;

  /* return success */
  return ( INoError );
}

Font *_IGetFont ( name )
char *name;
{
  int loop;

  if ( num_fonts ) {
    for ( loop = 0; loop < num_fonts; loop++ ) {
      if ( fonts[loop] ) {
        if ( strcmp ( fonts[loop]->name, name ) == 0 )
          return ( (Font *)fonts[loop] );
      }
    }
  }

  return ( NULL );
}


static void free_character ( ch )
Char *ch;
{
  free ( ch->name );
  free ( ch->data );
  free ( ch );
}


IError IFontBDFFree ( name )
char *name;
{
  Font *font;
  unsigned int loop;

  font = _IGetFont ( name );
  if ( ! font )
    return ( INoSuchFont );

  for ( loop = 0; loop < 256; loop++ ) {
    if ( font->chars[loop] )
      free_character ( font->chars[loop] );
  }
  for ( loop = 0; loop < font->num_other_chars; loop++ ) {
    free_character ( font->other_chars[loop] );
  }
  free ( font->other_chars );

  if ( font->name )
    free ( font->name );
  if ( font->family )
    free ( font->family );
  if ( font->face_name )
    free ( font->face_name );
  if ( font->width_name )
    free ( font->width_name );
  if ( font->weight )
    free ( font->weight );
  free ( font );

  return ( INoError );
}

IError _IFontBDFGetSize ( name, height_return )
char *name;
unsigned int *height_return;
{
  Font *font;

  font = _IGetFont ( name );
  if ( ! font )
    return ( INoSuchFont );

  *height_return = font->pixel_size;

  return ( INoError );
}



IError IFontBDFGetChar ( name, ch, bitdata, width, height,
  actual_width, size, xoffset, yoffset )
char *name;
char *ch;
unsigned int **bitdata;
unsigned int *height;
unsigned int *width;
unsigned int *actual_width;
unsigned int *size;
int *xoffset;
int *yoffset;
{
  Font *font;
  Char *character = NULL;
  unsigned int loop;
  static char *last_name = NULL;
  static Font *last_font = NULL;

  if ( ! ch || ! strlen ( ch ) )
    return ( IInvalidArgument );

  if ( last_name == name ) {
    font = last_font;
  }
  else {
    font = _IGetFont ( name );
    if ( ! font )
      return ( INoSuchFont );
    last_name = name;
    last_font = font;
  }

  /* we got the font.  now get the character */
  if ( strlen ( ch ) == 1 ) {
    character = font->chars[(unsigned char)ch[0]];
  }
  /* if more than a single char, must not be in the ascii list */
  else {
    for ( loop = 0, character = NULL; loop < font->num_other_chars; loop++ ) {
      if ( strcmp ( ch, font->other_chars[loop]->name ) == 0 ) {
        character = font->other_chars[loop];
        break;
      }
    }
    if ( ! character )
      character = font->chars[' '];
  }
  /* if still not there, use space */
  if ( ! character ) {
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


