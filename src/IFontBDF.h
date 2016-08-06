/*
** Routines for reading in a BDF (font) file.
** BDF files can be obtained as part of the X Windows System.
** URL: ftp://ftp.x.org/pub/R6.1/xc/fonts/bdf/75dpi/
**
** Copyright 1996 Craig Knudsen
**
** 17-May-96	Craig Knudsen	cknudsen@cknudsen.com
**		Created
*/

#ifndef _ifontbdf_h
#define _ifontbdf_h

/*
** Read in a BDF font file.
*/
IError IFontBDFReadFile (
#ifndef _NO_PROTO
  char *name,			/* font name */
  char *path			/* full path to BDF file */
#endif
);

IError IFontBDFReadData (
#ifndef _NO_PROTO
  char *name,			/* font name */
  char **lines			/* font file data lines of text */
				/* terminated with a NULL */
#endif
);

/*
** Free a font
*/
IError IFontBDFFree (
#ifndef _NO_PROTO
  char *name
#endif
);

/*
** Get the pixel size (height) of the font
*/
IError IFontBDFGetSize (
#ifndef _NO_PROTO
  char *name,			/* BDF filename */
  unsigned int *height_return	/* pixel height */
#endif
);

/*
** Get the width of a character string
*/
IError IFontBDFTextWidth (
#ifndef _NO_PROTO
  char *name,			/* BDF font name */
  char *ptr,			/* input chars */
  int len,			/* length of ptr */
  unsigned int *width_return	/* length of text in pixels */
#endif
);

/*
** Get the definition of a character
*/
IError IFontBDFGetChar (
#ifndef _NO_PROTO
  char *name,			/* BDF font name */
  char *ch, 			/* char we are looking up ("A", "\033agrave;") */
  unsigned int **bitdata,	/* array of 0/1 values containing the char definition */
  unsigned int *width,		/* width of bitdata */
  unsigned int *height,		/* height of bitdata */
  unsigned int *actual_width,	/* actual character width */
  unsigned int *size,		/* size of returned array - same as bdfGetSize() */
  int *xoffset,			/* pixels to move to the right before drawing */
  int *yoffset			/* pixel to move up before drawing */
#endif
);

#endif /* _ifontbdf_h */

