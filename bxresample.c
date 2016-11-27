static const char *version = "\
bxresample version " VERSION "\n\
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
Usage: %s [OPTION]... DIM1[+DIM2[+...]] [INFILE [OUTFILE]]\n\
Rescale dimensions of an ABX or BBX file.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
\n";

static const char *description = "\
# bxresample(1)\n\
\n\
## NAME\n\
\n\
`bxresample` - resize abx(5) or bbx(5) files\n\
\n\
## SYNOPSIS\n\
\n\
`bxresample` [_OPTION_]... _DIMS_ [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program changes the dimensions of an abx(5) or bbx(5) file.  The\n\
final dimensions lengths are specified by the non-option argument\n\
_DIMS_ as described below.  The file is read from _INFILE_ and written\n\
to _OUTFILE_.  If either or both of these are not specified, or are\n\
given as a single `-` character, then standard input or output is used\n\
instead.\n\
\n\
The standard form of the _DIMS_ argument is a sequence of positive\n\
integers joined together, with the `+` sign serving as a delimiter:\n\
e.g.\n\
\n\
> _DIM1_[`+`_DIM2_[`+`...]]\n\
\n\
The number of dimension length specified must be less than the number\n\
of dimensions in _INFILE_; furthermore, the product of the remaining\n\
dimensions in _INFILE_ must be a multiple of 8 bits.  Conceptually,\n\
the argument _DIM1_[`+`...] specifies the number and arrangement of\n\
basic units to be sampled from _INFILE_, where each basic unit must be\n\
an integral number of bytes (to avoid complicated bit-shifting when\n\
writing _OUTFILE_).\n\
\n\
Optionally, any of _DIM#_ may be of the form `x`_FAC_ or `/`_DIV_\n\
instead of [`+`]_DIM_: the resulting dimension length will be the\n\
corresponding dimension from _INFILE_ multipled by _FAC_ or divided by\n\
_DIV_ (rounded down).\n\
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
bbx(5)\n\
\n";

#include <stdio.h>
#include <unistd.h>
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

int
main( int argc, char **argv )
{
  char opt;               /* short option character */
  int lopt;               /* long option index */
  char *infile, *outfile; /* input/output filenames */
  char *dims;             /* resampled dimensions specifier */
  char *a, *b;            /* pointers within dims */
  FILE *fpin, *fpout;     /* input/output file pointers */
  char **headv = NULL;    /* header comments */
  int *dimin = NULL;      /* dimensions read */
  int *dimout = NULL;     /* dimensions written */
  int *out;               /* coordinates of current point in oufile */
  int headc, dimc;        /* number of comments, dimensions */
  char *encoding = NULL;  /* encoding type */
  unsigned char *samp;    /* sample unit */
  int nsamp;              /* bytes per sample unit */
  int n, nin;             /* number of bytes read and expected */
  int i, ndim;            /* index and number of dimensions resampled */
  int k, knext;           /* current sample read and next to be copied */

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
    lf_error( "missing dimensions argument\n\t"
	      "Try %s --help for more information", argv[0] );
    return 1;
  }
  dims = argv[optind++];
  if ( optind >= argc || !strcmp( ( infile = argv[optind++] ), "-" ) )
    infile = NULL;
  if ( optind >= argc || !strcmp( ( outfile = argv[optind++] ), "-" ) )
    outfile = NULL;
  if ( optind < argc ) {
    lf_error( "too many arguments\n\t"
	      "Try %s --help for more information", argv[0] );
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
  if ( bxRead( fpin, &headc, &headv, &dimc, &dimin, &encoding, NULL, 0 ) ) {
    lf_error( "could not parse header from %s", infile );
    fclose( fpin );
    return 2;
  }

  /* Compute output dimensions. */
  if ( !( dimout = (int *)calloc( dimc + 1, sizeof(int) ) ) ||
       !( out = (int *)calloc( dimc + 1, sizeof(int) ) ) ) {
    lf_error( "memory error" );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimin );
    if ( dimout )
      free( dimout );
    return 4;
  }
  for ( i = 0, a = dims; i < dimc; i++ ) {
    if ( *a == 'x' )
      dimout[i] = (int)( dimin[i]*strtod( a + 1, &b ) );
    else if ( *a == '/' ) {
      double d = strtod( a + 1, &b );
      dimout[i] = ( d > 0.0 ? (int)( dimin[i]/d ) : 0.0 );
    } else
      dimout[i] = (int)( strtod( a, &b ) );
    if ( b == a )
      break;
    if ( dimout[i] <= 0 ) {
      lf_error( "dimension %d in %s gives non-positive output dimension",
		i, dims );
      fclose( fpin );
      for ( i = 0; i < headc; i++ )
	free( headv[i] );
      free( headv );
      free( dimin );
      free( dimout );
      free( out );
      return 1;
    }
    a = b;
  }

  /* Compute bytes in each sample unit. */
  ndim = i;
  nsamp = 1;
  for ( ; i < dimc; i++ )
    nsamp *= ( dimout[i] = dimin[i] );
  if ( nsamp%8 ) {
    lf_error( "sample unit is %d bits; must be a multiple of 8", nsamp );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimin );
    free( dimout );
    free( out );
    return 3;
  }
  nsamp /= 8;
  if ( !( samp = (unsigned char *)malloc( nsamp*sizeof(unsigned char) ) ) ) {
    lf_error( "memory error" );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimin );
    free( dimout );
    free( out );
    return 3;
  }

  /* Make a note of the number of bytes expected from infile. */
  for ( i = 0, nin = 1; i < dimc; i++ )
    nin *= dimin[i];
  nin /= 8;

  /* Write output header. */
  if ( !outfile ) {
    if ( isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) ) ) {
      lf_error( "could not write to stdout" );
      fclose( fpin );
      for ( i = 0; i < headc; i++ )
	free( headv[i] );
      free( headv );
      free( dimin );
      free( dimout );
      free( out );
      free( samp );
      return 2;
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( outfile, "wb" ) ) ) {
    lf_error( "could not open output file %s", outfile );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimin );
    free( dimout );
    free( out );
    free( samp );
    return 2;
  }
  if ( bxWrite( fpout, headv, dimout, encoding, NULL, 0 ) ) {
    lf_error( "error writing header to %s", outfile );
    fclose( fpout );
    fclose( fpin );
    for ( i = 0; i < headc; i++ )
      free( headv[i] );
    free( headv );
    free( dimin );
    free( dimout );
    free( out );
    free( samp );
    return 2;
  }
  for ( i = 0; i < headc; i++ )
    free( headv[i] );
  free( headv );

  /* Step through files. */
  k = -1;
  for ( ; out[0] < dimout[0]; ) {

    /* Find point in input array. */
    for ( i = 0, knext = 0; i < ndim; i++ )
      knext = dimin[i]*knext + ( out[i]*dimin[i] )/dimout[i];

    /* Read up to specified point. */
    for ( ; k < knext; k++ )
      if ( ( n = bxReadData( encoding, samp, nsamp, fpin ) ) < nsamp ) {
	lf_warning( "read %d bytes from %s, expected %d",
		    k*nsamp + n, infile, nin );
	memset( samp + n, 0, ( nsamp - n )*sizeof(unsigned char) );
      }

    /* Write specified point. */
    if ( ( n = bxWriteData( encoding, samp, nsamp, 0, fpout ) ) < nsamp ) {
      lf_error( "error writing data to %s", outfile );
      fclose( fpout );
      fclose( fpin );
      free( dimin );
      free( dimout );
      free( out );
      free( samp );
    }

    /* Increment to next point in output array. */
    out[ndim-1]++;
    for ( i = ndim - 1; i > 0; i-- )
      if ( out[i] >= dimout[i] ) {
	out[i] = 0;
	out[i-1]++;
      }
  }
  fclose( fpout );
  fclose( fpin );
  free( dimin );
  free( dimout );
  free( out );
  free( samp );
  return 0;
}
