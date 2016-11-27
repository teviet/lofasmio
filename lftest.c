static const char *version = "\
lftest version " VERSION "\n\
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
Usage: %s [OPTION}... [ARGS]...\n\
Test lofasm routines.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
\n\
Other options and non-option arguments may be allowed or required.\n\
\n";

static const char *description = "\
# lftest(1)\n\
\n\
## NAME\n\
\n\
`lftest` - test lofasm routines\n\
\n\
## SYNOPSIS\n\
\n\
`lftest` [_OPTION_]... [_ARGS_]...\n\
\n\
## DESCRIPTION\n\
\n\
This program is a sandbox for testing LoFASM routines.  Its usage and\n\
effects may change without notice.\n\
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
These are the standard options for `lofasmio` programs.  Since\n\
`lftest` is a sandbox for trying out functions, addtional options (or\n\
non-option arguments) may be added; conceivably the above options may\n\
also be modified or removed.\n\
\n\
## EXIT STATUS\n\
\n\
The program should return `EXIT_SUCCESS`=0 normally or\n\
`EXIT_FAILURE`=1 on an error.\n\
\n\
## SEE ALSO\n\
\n\
lofasmio(7)\n\
\n";


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include "lofasmIO.h"
#include "markdown_parser.h"

static const char short_opts[] = ":hHVv:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { 0, 0, 0, 0} };

int
main( int argc, char *argv[] )
{
  char opt;           /* short option character */
  int lopt;           /* long option index */
  FILE *fpin, *fpout; /* input and output file pointers */
  char *infile = NULL, *outfile = NULL; /* input and outpit filenames */
  const char *prefix = "magnitude of "; /* modification to data label */
  char *label;        /* modified data label */
  int i, j, k;        /* indecies */
  lfb_hdr head = {};  /* header structure */
  double *dat, *d;    /* row of data and pointer within it */

  /* Parse standard options. */
  while ( ( opt = getopt_long( argc, argv, short_opts, long_opts, &lopt ) )
          != -1 ) {
    switch ( opt ) {
    case 0:
      if ( !strcmp( long_opts[lopt].name, "manpage" ) )
	markdown_to_manpage( description, NULL );
      else if ( !strcmp( long_opts[lopt].name, "markdown" ) )
	fputs( description, stdout );
      exit( EXIT_SUCCESS );
    case 'h':
      fprintf( stdout, usage, argv[0] );
      exit( EXIT_SUCCESS );
    case 'H':
      markdown_to_man_out( description );
      exit( EXIT_SUCCESS );
    case 'V':
      fputs( version, stdout );
      exit( EXIT_SUCCESS );
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
      exit( EXIT_FAILURE );
    case ':':
      if ( optopt )
	lf_error( "option -%c requires an argument\n\t"
		  "Try %s --help for more information",
		  optopt, argv[0] );
      else
	lf_error( "option %s requires an argument\n\t"
		  "Try %s --help for more information",
		  argv[optind-1], argv[0] );
      exit( EXIT_FAILURE );
    default:
      lf_error( "internal error parsing option code %c\n"
		"Try %s --help for more information",
		opt, argv[0] );
      exit( EXIT_FAILURE );
    }
  }

  /* Get file names or NULL for stdin/stdout. */
  if ( optind >= argc || !strcmp( ( infile = argv[optind++] ), "-" ) )
    infile = NULL;
  if ( optind >= argc || !strcmp( ( outfile = argv[optind++] ), "-" ) )
    outfile = NULL;
  if ( optind >= argc ) {
    lf_error( "too many arguments\n\t\n"
	      "Try %s --help for more information", argv[0] );
    exit( EXIT_FAILURE );
  }

  /* Read input file header. */
  i = 0;
  if ( !infile )
    i = i || !( fpin = lfdopen( 0, "rb" ) );
  else
    i = i || !( fpin = lfopen( infile, "rb" ) );
  i = i || lfbxRead( fpin, &head, NULL );
  if ( i ) {
    lf_error( "could not read input header" );
    exit( EXIT_FAILURE );
  }

  /* Check that data exist and are complex double. */
  if ( head.dims[0] <= 0 || head.dims[1] <= 0 ||
       head.dims[2] != 2 || head.dims[3] != 64 ) {
    lf_error( "file must contain complex double data" );
    exit( EXIT_FAILURE );
  }
  k = head.dims[1];

  /* Allocate new data label and data row. */
  i = strlen( prefix ) + strlen( head.data_label ) + 1;
  if ( !( label = (char *)malloc( i ) ) ||
       !( dat = (double *)malloc( 2*k*8 ) ) ) {
    lf_error( "memory error" );
    exit( EXIT_FAILURE );
  }
  sprintf( label, "%s%s", prefix, head.data_label );
  free( head.data_label );
  head.data_label = label;
  head.dims[2] = 1;

  /* Write output header. */
  i = 0;
  if ( !outfile )
    i = i || isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) );
  else
    i = i || !( fpout = lfopen( outfile, "wb" ) );
  i = i || lfbxWrite( fpout, &head, NULL );
  if ( i ) {
    lf_error( "could not write output header" );
    exit( EXIT_FAILURE );
  }

  /* Read, transform, and write data row-by-row. */
  lf_info( "computing magnitude of data in %s",
	   ( infile ? infile : "stdin" ) );
  for ( i = 0; i < head.dims[0]; i++ ) {
    if ( fread( dat, 8, 2*k, fpin ) < 2*k )
      break;
    for ( j = 0, d = dat; j < k; j++, d += 2 )
      dat[j] = sqrt( d[0]*d[0] + d[1]*d[1] );
    if ( fwrite( dat, 8, k, fpout ) < k )
      break;
  }
  if ( i < head.dims[0] ) {
    lf_error( "premature end of data" );
    exit( EXIT_FAILURE );
  }
  fclose( fpin );
  fclose( fpout );
  lfbxFree( &head );
  free( dat );
  exit( EXIT_SUCCESS );
}
