static const char *version = "\
lftype version " VERSION "\n\
Copyright (c) 2016 Teviet Creighton.\n\
\n\
This program is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation, either version 3 of the License, or (at\n\
your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n\
General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\
\n";

static const char *usage = "\
Usage: %s [OPTION]... TYPE [INFILE [OUTFILE]]\n\
Converts storage type of a LoFASM data file.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
\n";

static const char *description = "\
# lftype(1)\n\
\n\
## NAME\n\
\n\
`lftype(1)` - change type of a lofasm-filterbank(5) or related file\n\
\n\
## SYNOPSIS\n\
\n\
`lftype` [_OPTION_]... _TYPE_ [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program changes the storage type of a file in\n\
lofasm-filterbank(5) format or a related file format.  The file is\n\
read from _INFILE_ and written to _OUTFILE_.  If either or both of\n\
these are not specified, or are given as a single `-` character, then\n\
standard input or output is used instead.\n\
\n\
The file need not be a strict lofasm-filterbank(5) file, but must be\n\
an abx(5) or bbx(5) file with a header comment of the form:\n\
\n\
> `%data_type:` _type_\n\
\n\
where _type_ is one of the recognized types (below).  Furthermore, the\n\
final dimension length must match the bit depth of the type.  The data\n\
is converted to the specified _TYPE_ from the command line.  The ouput\n\
file is written with the new data block, final dimension, and\n\
`%data_type:` header comment; all other header comments are unchanged.\n\
\n\
Conversion uses standard C type casting.  The program makes no attempt\n\
to rescale the data: values out of the dynamic range of the target\n\
_TYPE_ will typically be converted to the maximum or minimum value of\n\
that type.\n\
\n\
### Recognized Types\n\
\n\
The following types are recognized, both as an input type from the\n\
`%data_type:` comment line, and as a target type for the _TYPE_\n\
command-line argument.\n\
\n\
`char8`, `uchar8`:\n\
    Signed and unsigned single-byte integers, corresponding to the C\n\
    types `char` and `unsigned char`.\n\
\n\
`int16`, `uint16`:\n\
    Signed and unsigned two-byte integers, corresponding to the C\n\
    types `short` and `unsigned short`.\n\
\n\
`int32`, `uint32`:\n\
    Signed and unsigned four-byte integers, corresponding to the C\n\
    types `long` and `unsigned long`.\n\
\n\
`int64`, `uint64`:\n\
    Signed and unsigned eight-byte integers, corresponding to the C\n\
    types `long long` and `unsigned long long`.\n\
\n\
`real32`, `real64`:\n\
    IEEE 754 single-precision and double-precision floating point\n\
    numbers, corresponding to the C types `float` and `double`.\n\
\n\
### Endianness\n\
\n\
The lofasm-filterbank(5) standard calls for little-endian byte order\n\
for multi-byte data types.  For example, a `uint16` integer has its\n\
least-significant byte written first, followed by the most-significant\n\
byte.  This is the convention used by the common x86 family of\n\
computer processors (used for most LoFASM data analysis), but differs\n\
from the more intuitive big-endian \"network\" byte order.\n\
\n\
If the input file contains a comment of the form `%hdr_version:` then\n\
the subsequent set of hexadecimal characters is converted to a\n\
`real32` number and tested to see if it corresponds to an integer less\n\
than 256.  If so, then the endianness of the file and of the computer\n\
executing the program agree.  If not, a warning is printed; however,\n\
no other attempt is made to resolve endianness mismatches.  See the\n\
lofasm-filterbank(5) documentation for more details.\n\
\n\
## OPTIONS\n\
\n\
`-h, --help`:\n\
    Prints basic usage information to stdout and exits.\n\
\n\
`-H, --man`:\n\
    Displays this manual page using man(1).\n\
\n\
`--manpage`:\n\
    Prints this manual page to standard output, in groff format.\n\
\n\
`--markdown`:\n\
    Prints this manual page to standard output, in markdown format.\n\
\n\
`-V, --version`:\n\
    Prints version and copyright information.\n\
\n\
`-v, --verbosity=`_LEVEL_:\n\
    Sets the verbosity level for error reporting.  _LEVEL_ may be `0`\n\
    (quiet, no messages), `1` (default, error messages only), `2`\n\
    (verbose, errors and warnings), or `3` (very verbose, errors,\n\
    warnings, and extra information).\n\
\n\
## EXIT STATUS\n\
\n\
The proram exits with status 0 normally, 1 if there is an error\n\
parsing its arguments, 2 on read/write errors, 3 if the file has\n\
incorrect syntax, or 4 on memory errors\n\
\n\
## SEE ALSO\n\
\n\
bxRead(3),\n\
bxWrite(3),\n\
abx(5),\n\
bbx(5),\n\
lofasm-filterbank(5)\n\
\n";

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <getopt.h>
#include "markdown_parser.h"
#include "lofasmIO.h"

static const char short_opts[] = ":hHVv:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { 0, 0, 0, 0} };

/* Macro to read a row from fpin, convert it, and write it to fpout.
   The type of from and to are unspecified. */
#define CONVERT( from, to ) \
do { \
  int ni = sizeof( *(from) ); \
  int no = sizeof( *(to) ); \
  if ( !( (from) = malloc( ne*ni ) ) || !( (to) = malloc( ne*no ) ) ) { \
    lf_error( "memory error" ); \
    fclose( fpout ); \
    fclose( fpin ); \
    if ( (from) ) \
      free( (from) ); \
    free( encoding ); \
    return 4; \
  } \
  for ( i = 0; i < nrows && !feof( fpin ); i++ ) { \
    if ( ( n = bxReadData( encoding, (unsigned char *)(from), ne*ni, fpin ) ) \
	 < ne*ni ) { \
      lf_warning( "read %lld bytes from %s, expected %lld", \
	          (long long)( i*ne*ni + n ), infile, \
                  (long long)( nrows*ne*ni ) ); \
      memset( (unsigned char *)(from) + n, 0, ne*ni - n ); \
    } \
    for ( j = 0; j < ne; j++ ) \
      (to)[j] = (from)[j]; \
    if ( bxWriteData( encoding, (const unsigned char *)(to), ne*no, nr, fpout ) \
	 < ne*no ) { \
      lf_error( "error writing data to %s", outfile ); \
      fclose( fpout ); \
      fclose( fpin ); \
      free( (from) ); \
      free( (to) ); \
      free( encoding ); \
      return 2; \
    } \
  } \
  free( (from) ); \
  memset( (unsigned char *)(to), 0, ne*no ); \
  for ( ; i < nrows; i++ ) \
    bxWriteData( encoding, (const unsigned char *)(to), ne*no, nr, fpout ); \
  free( (to) ); \
} while ( 0 )

static const char *versionkey = "hdr_version:";
static const char *typekey = "data_type:";

int
main( int argc, char **argv )
{
  int opt, lopt;              /* option character and index */
  char *infile, *outfile;     /* input/output filenames */
  char intype[7], outtype[7]; /* input/output data types */
  FILE *fpin, *fpout;         /* input/output file pointers */
  char **headv;               /* header comments */
  int64_t *dimv;              /* dimensions */
  int headc, dimc;            /* number of comments, dimensions */
  char *encoding;             /* encoding type */
  int64_t i, j, nrows;        /* indecies and number of rows */
  int64_t ne, nr = 0, n;      /* effective, true, and read elements per row */
  char *c;                    /* pointer within header comment */
  /* Row data cast to various types. */
  char *c8row;
  unsigned char *uc8row;
  short *i16row;
  unsigned char *ui16row;
  long *i32row;
  unsigned long *ui32row;
  long long *i64row;
  unsigned long long *ui64row;
  float *r32row;
  double *r64row;

  /* Parse options. */
  opterr = 0;
  while ( ( opt = getopt_long( argc, argv, short_opts, long_opts, &lopt ) )
          != -1 ) {
    switch ( opt ) {
    case 0:
      if ( !strcmp( long_opts[lopt].name, "manpage" ) )
	markdown_to_manpage( description, NULL );
      else if ( !strcmp( long_opts[lopt].name, "markdown" ) )
	fputs( description, stdout );
      return 0;
    case 'h':
      fprintf( stdout, usage, argv[0] );
      return 0;
    case 'H':
      markdown_to_man_out( description );
      return 0;
    case 'V':
      fputs( version, stdout );
      return 0;
    case 'v':
      lofasm_verbosity = atoi( optarg );
      break;
    case '?':
      if ( optopt )
	lf_error( "unknown option -%c\n\t"
		  "Try %s --help for more information",
		  optopt, argv[0] );
      else
	lf_error( "unknown option %s\n\t"
		  "Try %s --help for more information",
		  argv[optind-1], argv[0] );
      return 1;
    case ':':
      if ( optopt )
	lf_error( "option -%c requires an argument\n\t"
		  "Try %s --help for more information",
		  optopt, argv[0] );
      else
	lf_error( "option %s requires an argument\n\t"
		  "Try %s --help for more information",
		  argv[optind-1], argv[0] );
      return 1;
    default:
      lf_error( "internal error parsing option code %c\n\t"
		"Try %s --help for more information",
		opt, argv[0] );
      return 1;
    }
  }

  /* Parse other arguments. */
  if ( optind >= argc ) {
    lf_error( "missing type argument\n\t"
	      "Try %s --help for more information", argv[0] );
    return 1;
  }
  if ( sscanf( argv[optind++], "%6s", intype ) != 1 ) {
    lf_error( "bad type argument" );
    return 1;
  }
  if ( optind >= argc || !strcmp( ( infile = argv[optind++] ), "-" ) )
    infile = NULL;
  if ( optind >= argc || !strcmp( ( outfile = argv[optind++] ), "-" ) )
    outfile = NULL;
  if ( optind < argc ) {
    lf_error( "too many arguments\n\t"
	      "Try %s --help for more information", argv[0] );
    return 1;
  }

  /* Check output type. */
  if ( strcmp( outtype, "char8" ) && strcmp( outtype, "uchar8" ) &&
       strcmp( outtype, "int16" ) && strcmp( outtype, "uint16" ) &&
       strcmp( outtype, "int32" ) && strcmp( outtype, "uint32" ) &&
       strcmp( outtype, "int64" ) && strcmp( outtype, "uint64" ) &&
       strcmp( outtype, "real32" ) && strcmp( outtype, "real32" ) &&
       strcmp( outtype, "real64" ) && strcmp( outtype, "real64" ) ) {
    lf_error( "unrecognized target type %s", outtype );
    return 1;
  }

  /* Read input header. */
  if ( !infile ) {
    if ( !( fpin = lfdopen( 0, "rb" ) ) ) {
      lf_error( "could not read stdin" );
      return 2;
    }
    infile = "stdin";
  } else if ( !( fpin = lfopen( infile, "rb" ) ) ) {
    lf_error( "could not open input file %s", infile );
    return 2;
  }
  if ( bxRead( fpin, &headc, &headv, &dimc, &dimv, &encoding, NULL, 0 ) ) {
    lf_error( "could not parse header from %s", infile );
    fclose( fpin );
    return 2;
  }

  /* Check endianness. */
  for ( i = 0; i < headc; i++ )
    if ( !strncmp( headv[i], versionkey, strlen( versionkey ) ) ) {
      int x;                          /* hex character */
      unsigned char cversion[4] = {}; /* version as byte sequence */
      float fversion[1];              /* version converted to real32 */
      for ( j = 0, c = headv[i] + strlen( versionkey ); j < 8 && *c; j++ ) {
	while ( isspace( *c ) )
	  c++;
	x = ( *c >= '0' && *c <= '9' ? *c - '0' :
	      ( *c >= 'a' && *c <= 'f' ? *c - 'a' + 10 :
		( *c >= 'A' && *c <= 'F' ? *c - 'A' + 10 : -1 ) ) );
	if ( x < 0 )
	  break;
	cversion[j/2] += ( j%2 ? x : 16*x );
      }
      if ( j < 8 )
	lf_warning( "bad version metadata %%%s\n", headv[i] );
      else {
	memcpy( fversion, cversion, 4 );
	if ( *fversion < 1.0 || *fversion > 255.0 ||
	     *fversion != floor( *fversion ) )
	  lf_warning( "file %s has wrong endianness", infile );
      }
      break;
    }

  /* Get input type. */
  for ( i = 0; i < headc; i++ )
    if ( !strncmp( headv[i], typekey, strlen( typekey ) ) ) {
      if ( sscanf( headv[i] + strlen( typekey ), "%6s", outtype ) < 1 ) {
	lf_error( "bad type metadata %%%s\n", headv[i] );
	fclose( fpin );
	for ( i = 0; i < headc; i++ )
	  free( headv[i] );
	free( headv );
	free( dimv );
	free( encoding );
	return 3;
      }
      break;
    }

  /* Check input type. */
  if ( strcmp( intype, "char8" ) && strcmp( intype, "uchar8" ) &&
       strcmp( intype, "int16" ) && strcmp( intype, "uint16" ) &&
       strcmp( intype, "int32" ) && strcmp( intype, "uint32" ) &&
       strcmp( intype, "int64" ) && strcmp( intype, "uint64" ) &&
       strcmp( intype, "real32" ) && strcmp( intype, "real32" ) &&
       strcmp( intype, "real64" ) && strcmp( intype, "real64" ) ) {
    lf_error( "unrecognized input type %s", intype );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimv );
    free( encoding );
    return 1;
  }
  for ( c = intype; !isdigit( (int)( *c ) ); c++ )
    ;
  if ( atoi( c ) != dimv[dimc-1] ) {
    lf_error( "input type %s does not match bit depth %lld", intype,
	      (long long)( dimv[dimc-1] ) );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimv );
    free( encoding );
    return 1;
  }

  /* Store new type in header. */
  free( headv[i] );
  if ( !( headv[i] = (char *)
	  malloc( ( strlen( typekey ) + strlen( outtype ) + 2 )
		  *sizeof(char) ) ) ) {
    lf_error( "memory error" );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      if ( headv[i] )
	free( headv[i] );
    free( headv );
    free( dimv );
    free( encoding );
    return 4;
  }
  sprintf( headv[i], "%s %s", typekey, outtype );
  for ( c = outtype; !isdigit( (int)( *c ) ); c++ )
    ;
  dimv[dimc-1] = atoi( c );

  /* Check that type conversion has not pushed us above bit limit. */
  for ( i = 0, n = 1; i < dimc; i++, n *= dimv[i] )
    if ( n > INT64_MAX/dimv[i] ) {
      lf_error( "number of bits exceeds INT64_MAX" );
      fclose( fpin );
      for ( i = 0; i < headc; i++ )
	if ( headv[i] )
	  free( headv[i] );
      free( headv );
      free( dimv );
      free( encoding );
      return 4;
    }

  /* Write output header. */
  if ( !outfile ) {
    if ( isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) ) ) {
      lf_error( "could not write to stdout" );
      fclose( fpin );
      for ( i = 0; i < headc; i++ )
	free( headv[i] );
      free( headv );
      free( dimv );
      free( encoding );
      return 2;
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( outfile, "wb" ) ) ) {
    lf_error( "could not open output file %s", outfile );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimv );
    free( encoding );
    return 2;
  }
  if ( bxWrite( fpout, headv, dimv, encoding, NULL, 0 ) ) {
    lf_error( "error writing header to %s", outfile );
    fclose( fpout );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimv );
    free( encoding );
    return 2;
  }
  for ( i = 0; i < headc; i++ )
    free( headv[i] );
  free( headv );

  /* Get number of elements per row. */
  if ( dimc == 1 )
    nrows = ne = 1;
  else if ( dimc == 2 ) {
    nrows = nr = 1;
    ne = dimv[0];
  } else {
    nrows = dimv[0];
    ne = 1;
    for ( i = 1; i < dimc - 1; i++ )
      ne *= dimv[i];
  }
  free( dimv );

  /* Perform conversion. */
  if ( !strcmp( intype, "char8" ) ) {
    if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( c8row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( c8row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( c8row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( c8row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( c8row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( c8row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( c8row, ui64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( c8row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( c8row, r64row );
  } else if ( !strcmp( intype, "uchar8" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( uc8row, c8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( uc8row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( uc8row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( uc8row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( uc8row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( uc8row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( uc8row, ui64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( uc8row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( uc8row, r64row );
  } else if ( !strcmp( intype, "int16" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( i16row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( i16row, uc8row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( i16row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( i16row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( i16row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( i16row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( i16row, ui64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( i16row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( i16row, r64row );
  } else if ( !strcmp( intype, "uint16" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( ui16row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( ui16row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( ui16row, i16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( ui16row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( ui16row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( ui16row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( ui16row, ui64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( ui16row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( ui16row, r64row );
  } else if ( !strcmp( intype, "int32" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( i32row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( i32row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( i32row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( i32row, ui16row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( i32row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( i32row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( i32row, ui64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( i32row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( i32row, r64row );
  } else if ( !strcmp( intype, "uint32" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( ui32row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( ui32row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( ui32row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( ui32row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( ui32row, i32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( ui32row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( ui32row, ui64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( ui32row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( ui32row, r64row );
  } else if ( !strcmp( intype, "int64" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( i64row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( i64row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( i64row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( i64row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( i64row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( i64row, ui32row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( i64row, ui64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( i64row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( i64row, r64row );
  } else if ( !strcmp( intype, "uint64" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( ui64row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( ui64row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( ui64row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( ui64row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( ui64row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( ui64row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( ui64row, i64row );
    else if ( !strcmp( outtype, "real32" ) )
      CONVERT( ui64row, r32row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( ui64row, r64row );
  } else if ( !strcmp( intype, "real32" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( r32row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( r32row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( r32row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( r32row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( r32row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( r32row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( r32row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( r32row, ui64row );
    else /* if ( !strcmp( outtype, "real64" ) ) */
      CONVERT( r32row, r64row );
  } else if ( !strcmp( intype, "real64" ) ) {
    if ( !strcmp( outtype, "char8" ) )
      CONVERT( r64row, c8row );
    else if ( !strcmp( outtype, "uchar8" ) )
      CONVERT( r64row, uc8row );
    else if ( !strcmp( outtype, "int16" ) )
      CONVERT( r64row, i16row );
    else if ( !strcmp( outtype, "uint16" ) )
      CONVERT( r64row, ui16row );
    else if ( !strcmp( outtype, "int32" ) )
      CONVERT( r64row, i32row );
    else if ( !strcmp( outtype, "uint32" ) )
      CONVERT( r64row, ui32row );
    else if ( !strcmp( outtype, "int64" ) )
      CONVERT( r64row, i64row );
    else if ( !strcmp( outtype, "uint64" ) )
      CONVERT( r64row, ui64row );
    else /* if ( !strcmp( outtype, "real32" ) ) */
      CONVERT( r64row, r32row );
  }

  /* Finished. */
  fclose( fpout );
  fclose( fpin );
  free( encoding );
  return 0;
}
