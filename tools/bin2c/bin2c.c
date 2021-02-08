/*
 * This is bin2c program, which allows you to convert binary file to
 * C language array, for use as embedded resource, for instance you can
 * embed graphics or audio file directly into your program.
 * This is public domain software, use it on your own risk.
 * Contact Serge Fukanchik at fuxx@mail.ru  if you have any questions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* Replace . with _ */
char*
make_ident ( char* name )
{
  char* ret;
  char* p;

  ret = strdup ( name );

  for ( p = ret; p[0]; p++ )
  {
    if ( !isalnum ( p[0] ) ) p[0] = '_';
  }
  return ret;
}

int
main ( int argc, char* argv[] )
{
  unsigned char buf[BUFSIZ];
  char* ident;
  FILE *fd;
  size_t size, i, total, blksize = BUFSIZ;
  int need_comma = 0;

  if ( argc != 2 )
  {
    fprintf ( stderr, "Usage: %s binary_file > output_file\n", argv[0] );
    return -1;
  }

  fd = fopen ( argv[1], "rb" );
  if ( fd == NULL )
  {
    fprintf ( stderr, "%s: can't open %s for reading\n", argv[0], argv[1] );
    return -1;
  }

  fseek(fd, 0, SEEK_END);
  size = ftell(fd);
  rewind(fd);

  ident = make_ident ( argv[1] );

  printf ( "static const unsigned char __attribute__((section (\"._%s\"))) %s[] = {", ident, ident );
  for ( total = 0; total < size; )
  {
    if ( size - total < blksize ) blksize = size - total;
    if ( fread ( buf, 1, blksize, fd ) != blksize )
    {
      fprintf ( stderr, "%s: file read error\n", argv[0] );
      return -1;
    }
    for ( i = 0; i < blksize; i++ )
    {
      if ( need_comma ) printf ( ", " );
      else need_comma = 1;
      if ( ( total % 11 ) == 0 ) printf ( "\n\t" );
      printf ( "0x%.2x", buf[i] );
      total++;
    }
  }
  printf ( "\n};\n" );

  fclose ( fd );
  free ( ident );

  return 0;
}
