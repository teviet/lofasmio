static const char *version = "\
lfmed version " VERSION "\n\
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
Usage: %s [OPTION]... [INFILE [OUTFILE]]\n\
Compute running medians of a LoFASM file.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
  -y, --cols=R           average with radius R along columns (dimension 1)\n\
  -x, --rows=R           average with radius R along rows (dimension 2)\n\
\n";

static const char *description = "\
# lfmed(1)\n\
\n\
## NAME\n\
\n\
`lfmed(1)` - perform running medians on a lofasm-filterbank(5) file\n\
\n\
## SYNOPSIS\n\
\n\
`lfmed` [_OPTION_]... [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program smoothes a lofasm-filterbank(5) file _INFILE_ by\n\
replacing each datum with the median of a set of points around and\n\
including it, writing the result to _OUTFILE_.  If _INFILE_ or\n\
_OUTFILE_ is not specified, or is a single `-` character, then\n\
standard input or standard output is used instead.\n\
\n\
If run without options, the program will perform the uninteresting\n\
task of copying _INFILE_ to _OUTFILE_.  At least one of the\n\
`-x, --rows` option or the `-y, --cols` option, or both, must be given\n\
to apply a filter.\n\
\n\
### Median Filters\n\
\n\
A running median filter differs from the traditional running mean\n\
filter (or, more generally, convolution) in several respects.  A\n\
median is an \"ordinal\", rather than arithmetic, property of the\n\
data: it depends only on whether one datum is larger or smaller than\n\
another, not by how much.  This makes it insensitive (robust) against\n\
large outliers, and thus ideal for removing these outliers without\n\
contaminating surrounding data.  Ordinal quantities are also\n\
insensitive to nonlinear transformations of the data, so long as those\n\
transformations are monotonic (preserving or reversing the \"greater\n\
than\" and \"less than\" operations): the median of the logarithm of\n\
raw data is simply the logarithm of the median of the raw data.\n\
\n\
The downside of median filters is that they require more computational\n\
resources than means or convolutions: each median requires re-sorting\n\
the surrounding data.  A running median of width _W_ applied to _N_\n\
data thus requires of order _N_*_W_*log(_W_) operations, whereas a\n\
running mean can be computed in order _N_ operations, and a\n\
convolution in order _N_*_W_, or _N_*log(_N_) if using Fourier\n\
methods.\n\
\n\
Technically, a median is not defined on complex data; however you can\n\
obtain results similar to a running mean by filtering the real and\n\
imaginary parts separately.  This is not yet implemented in the\n\
current code.\n\
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
`-y, --cols=`_R_:\n\
    Runs a median filter with half-width _R_ along each column of data\n\
    (time in a standard lofasm-filterbank(5) file), where _R_ is a\n\
    positive integer.  The size of the dataset used to compute each\n\
    median is actually 2*_R_+1: _R_ points on either side of the\n\
    datum, plus the datum itself.  Note that column filtering requires\n\
    that the entire data file be loaded into memory before applying\n\
    the filter.\n\
\n\
`-x, --rows=`_R_:\n\
    Runs a median filter with half-width _R_ along each row of data\n\
    (frequency in a standard lofasm-filterbank(5) file), where _R_ is\n\
    a positive integer.  The size of the dataset used to compute each\n\
    median is actually 2*_R_+1: _R_ points on either side of the\n\
    datum, plus the datum itself.  If only row filtering is specfied,\n\
    then the program will write each row as it is filtered, and need\n\
    not store more than one row in memory at a time.  If specified in\n\
    conjunction with `-y, --cols`, above, column filtering is\n\
    performed first.\n\
\n\
## EXIT STATUS\n\
\n\
The proram exits with status 0 normally, 1 if there is an error\n\
parsing its arguments, 2 on read/write errors, 3 if the file is badly\n\
formatted. and 4 on memory allocation errors.\n\
\n\
## SEE ALSO\n\
\n\
lfbxRead(3),\n\
lfbxWrite(3),\n\
lofasm-filterbank(5)\n\
\n";

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include "markdown_parser.h"
#include "lofasmIO.h"

static const char short_opts[] = "hHVv:y:x:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "cols", 1, 0, 'y' },
  { "rows", 1, 0, 'x' },
  { 0, 0, 0, 0} };

/* Array and comparison function for indexed sorting of
   non-consecutive data: ddata[0] points to the first element,
   ddata[stride] to the next, and so on.  The comparison is written so
   that NaNs will be sorted below anything else. */
double *ddata;
int stride;
int
ascend( const void *p1, const void *p2 )
{
  double diff = ddata[ *( (int *)(p1) )*stride ];
  diff -= ddata[ *( (int *)(p2) )*stride ];
  return ( diff > 0.0 ? 1 : ( diff == 0.0 ? 0 : -1 ) );
}

int
main( int argc, char **argv )
{
  char opt;                /* option character */
  int lopt;                /* long option index */
  int r1 = 0, r2 = 0;      /* width along each dimension */
  char *infile, *outfile;  /* input/output file names */
  FILE *fpin, *fpout;      /* input/output file pointers */
  int i, imin, imax;       /* index and range in columns */
  int j, jmin, jmax;       /* index and range in rows */
  int n, k;                /* more indecies */
  lfb_hdr head = {};       /* file header */
  double *dat, *row, *out; /* data block, row, and fitered output */
  int *idx;                /* sorting index */

  /* Parse options. */
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
    case 'y':
      if ( sscanf( optarg, "%d", &r1 ) < 1 || r1 < 0 ) {
	lf_error( "bad -y, --cols argument %s", optarg );
	return 1;
      }
      break;
    case 'x':
      if ( sscanf( optarg, "%d", &r2 ) < 1 || r2 < 0 ) {
	lf_error( "bad -x, --rows argument %s", optarg );
	return 1;
      }
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
  if ( optind >= argc || !strcmp( ( infile = argv[optind++] ), "-" ) )
    infile = NULL;
  if ( optind >= argc || !strcmp( ( outfile = argv[optind++] ), "-" ) )
    outfile = NULL;
  if ( optind < argc ) {
    lf_error( "too many arguments" );
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
    fclose( fpin );
    return 2;
  }
  if ( lfbxRead( fpin, &head, NULL ) ) {
    lf_error( "could not parse header from %s", infile );
    fclose( fpin );
    lfbxFree( &head );
    return 2;
  }

  /* Check data type. */
  if ( head.dims[2] != 1 ) {
    lf_error( "requires real scalar data" );
    fclose( fpin );
    lfbxFree( &head );
    return 3;
  }
  if ( head.dims[3] != 64 ) {
    lf_error( "requires real64 data" );
    fclose( fpin );
    lfbxFree( &head );
    return 3;
  }
  if ( !head.data_type || strcmp( head.data_type, "real64" ) )
    lf_warning( "treating as real64 data" );

  /* Allocate data storage. */
  n = ( r1 > 0 ? head.dims[0] : 1 );
  dat = (double *)malloc( n*head.dims[1]*sizeof(double) );
  n = ( r2 > 0 && head.dims[1] > n ? head.dims[1] : n );
  out = (double *)malloc( n*sizeof(double) );
  idx = (int *)malloc( n*sizeof(int) );
  if ( !dat || !out || !idx ) {
    lf_error( "memory error" );
    fclose( fpin );
    if ( dat )
      free( dat );
    if ( out )
      free( out );
    if ( idx )
      free( idx );
    lfbxFree( &head );
    return 4;
  }

  /* Write output file header. */
  if ( !outfile ) {
    if ( isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) ) ) {
      lf_error( "could not write to stdout" );
      fclose( fpin );
      free( dat );
      free( out );
      free( idx );
      lfbxFree( &head );
      return 2;
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( outfile, "wb" ) ) ) {
    lf_error( "could not open output file %s", outfile );
    fclose( fpin );
    free( dat );
    free( out );
    free( idx );
    lfbxFree( &head );
    return 2;
  }
  if ( lfbxWrite( fpout, &head, NULL ) ) {
    lf_error( "error writing header to %s", outfile );
    fclose( fpout );
    fclose( fpin );
    free( dat );
    free( out );
    free( idx );
    lfbxFree( &head );
    return 2;
  }

  /* Do column filtering, if requested. */
  if ( r1 > 0 ) {
    n = head.dims[0]*head.dims[1];
    if ( ( i = fread( dat, sizeof(double), n, fpin ) ) < n ) {
      lf_warning( "read %d data from %s, expected %d", i, infile, n );
      memset( dat + i, 0, ( n - i )*sizeof(double) );
    }
    stride = head.dims[1];
    for ( j = 0; j < head.dims[1]; j++ ) {
      for ( i = 0; i < head.dims[0]; i++ ) {
	imin = ( i - r1 < 0 ? 0 : i - r1 );
	imax = ( i + r1 > head.dims[0] - 1 ? head.dims[0] - 1 : i + r1 );
	ddata = dat + imin*head.dims[1] + j;
	for ( k = 0; k < imax - imin; k++ )
	  idx[k] = k;
	qsort( idx, imax - imin, sizeof(int), ascend );
	out[i] = ddata[ ( idx[ ( imax - imin )/2 ] )*stride ];
      }
      for ( i = 0; i < head.dims[0]; i++ )
	dat[ i*stride + j ] = out[i];
    }
  }

  /* Do row filtering, if requested; write output in any case. */
  n = head.dims[1];
  stride = 1;
  for ( i = 0; i < head.dims[0]; i++ ) {

    /* Get next row of data, reading it if we haven't already. */
    if ( r1 > 0 )
      row = dat + i*n;
    else {
      if ( !feof( fpin ) ) {
	if ( ( j = fread( dat, sizeof(double), n, fpin ) ) < n ) {
	  lf_warning( "read %d data from %s, expected %d", i*n + j,
		     infile, head.dims[0]*n );
	  memset( dat + j, 0, ( n - j )*sizeof(double) );
	}
	row = dat;
      } else {
	memset( out, 0, n*sizeof(double) );
	row = NULL;
      }
    }

    /* Sort it if needed. */
    if ( r2 > 0 && row ) {
      for ( j = 0; j < n; j++ ) {
	jmin = ( j - r2 < 0 ? 0 : j - r2 );
	jmax = ( j + r2 > n - 1 ? n - 1 : j + r2 );
	ddata = row + jmin;
	for ( k = 0; k < jmax - jmin; k++ )
	  idx[k] = k;
	qsort( idx, jmax - jmin, sizeof(int), ascend );
	out[j] = ddata[ ( idx[ ( jmax - jmin )/2 ] ) ];
      }
    }
    if ( r2 > 0 )
      row = out;
    if ( fwrite( row, sizeof(double), n, fpout ) < n ) {
      lf_error( "could not write data to %s", outfile );
      fclose( fpin );
      fclose( fpout );
      free( dat );
      free( out );
      free( idx );
      lfbxFree( &head );
      return 2;
    }
  }

  /* Finished. */
  fclose( fpin );
  fclose( fpout );
  free( dat );
  free( out );
  free( idx );
  lfbxFree( &head );
  return 0;
}
