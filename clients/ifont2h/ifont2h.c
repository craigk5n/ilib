/*
 * font2h.c
 *
 * Image library
 *
 * Description:
 *	Create an include file from a BDF font file.
 *
 * History:
 *	29-May-96	Craig Knudsen	cknudsen@radix.net
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main ( argc, argv )
int argc;
char *argv[];
{
  FILE *fp;
  char *ptr, *ptr2;
  char name[200];
  char text[1024], text2[1024];

  if ( argc <= 1 ) {
    fprintf ( stderr, "Usage: font2h <bdf font file>\n" );
    exit ( 1 );
  }

  fp = fopen ( argv[1], "r" );
  if ( ! fp ) {
    fprintf ( stderr, "Error opening file: %s\n", argv[1] );
    exit ( 1 );
  }

  /* get name */
  ptr = argv[1] + strlen ( argv[1] ) - 1;
  while ( ptr != argv[1] && *ptr != '/' )
    ptr--;
  if ( *ptr == '/' )
    ptr++;
  strcpy ( name, ptr );
  strtok ( name, "." );

  printf ( "/* file generated by font2h - do not edit */\n\n" );
  printf ( "#ifndef _%s_h\n", name );
  printf ( "#define _%s_h\n", name );
  printf ( "static char *%s_font[] = {\n", name );

  while ( fgets ( text, 1024, fp ) ) {
    if ( strncmp ( text, "COMMENT", 7 ) == 0 )
      continue;
    for ( ptr = text, ptr2 = text2; *ptr != '\012' && *ptr != '\015' &&
      *ptr != '\0'; ptr++, ptr2++ ) {
      if ( *ptr == '"' ) {
        *ptr2 = '\\';
        ptr2++;
      }
      *ptr2 = *ptr;
    }
    *ptr2 = '\0';
    printf ( "  \"%s\",\n", text2 );
  }
  printf ( "  NULL,\n};\n" );
  printf ( "#endif /* _%s_h */\n", name );

  fclose ( fp );

  return ( 0 );
}
