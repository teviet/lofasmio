static const char *version = "\
lfmean version " VERSION "\n\
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
Compute running means of a LoFASM file.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
  -y, --cols=LEN         average LEN points along columns (dimension 1)\n\
  -x, --rows=LEN         average LEN points along rows (dimension 2)\n\
\n";

static const char *description = "\
# lfmean(1)\n\
\n\
## NAME\n\
\n\
`lfmean(1)` - perform running means on a lofasm-filterbank(5) file\n\
\n\
## SYNOPSIS\n\
\n\
`lfmean` [_OPTION_]... [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program smoothes a lofasm-filterbank(5) file _INFILE_ by\n\
replacing each datum with the mean of a set of points around and\n\
including it, writing the result to _OUTFILE_.  If _INFILE_ or\n\
_OUTFILE_ is not specified, or is a single `-` character, then\n\
standard input or standard output is used instead.\n\
\n\
If run without options, the program will perform the uninteresting\n\
task of copying _INFILE_ to _OUTFILE_.  At least one of the\n\
`-x, --rows` option or the `-y, --cols` option, or both, must be given\n\
to apply a filter.\n\
\n\
Typically one computes a running mean before downsampling a data file.\n\
To facilitate this, the average is a forward-looking average rather\n\
than a symmetric average about each point.  That is, if the averaging\n\
length is _LEN_, then output point _N_ is the mean of input points _N_\n\
through _N_+ _LEN_-1.  When _N_ approaches the end of the array, the\n\
averaging lengtn is reduced; however, these points will be dropped if\n\
one downsamples by a factor _LEN_.\n\
\n\
A running median is more effective at removing outliers, but is\n\
slower: see lfmed(1).  The algorithm used by this program can compute\n\
a running mean in order _N_ operations, where _N_ is the number of\n\
data points.\n\
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
`-y, --cols=`_LEN_:\n\
    Applies a running-mean filter with length _LEN_ along each column\n\
    of data (time in a standard lofasm-filterbank(5) file), where\n\
    _LEN_ is a positive integer.  Note that column filtering requires\n\
    that the entire data file be loaded into memory before applying\n\
    the filter.\n\
\n\
`-x, --rows=`_LEN_:\n\
    Applies a running-mean filter with length _LEN_ along each row of\n\
    data (frequency in a standard lofasm-filterbank(5) file), where\n\
    _LEN_ is a positive integer.  If only row filtering is specfied,\n\
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
#include <math.h>
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

int
main( int argc, char **argv )
{
  int opt, lopt;           /* option character and index */
  unsigned long long l1 = 1, l2 = 1; /* filter length along each dimension */
  char *infile, *outfile;  /* input/output file names */
  FILE *fpin, *fpout;      /* input/output file pointers */
  int64_t i, j, n, k, z;   /* indecies */
  int64_t stride;          /* step between successive samples */
  lfb_hdr head = {};       /* file header */
  double *dat, *row, *out; /* data block, row, and fitered output */
  double *buf;             /* circular buffer of filtered data */
  double sum;              /* cumulative sum of data */

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
      if ( ( l1 = strtoull( optarg, NULL, 10 ) ) < 1 || l1 > INT64_MAX ) {
	lf_error( "bad -y, --cols argument %s", optarg );
	return 1;
      }
      break;
    case 'x':
      if ( ( l2 = strtoull( optarg, NULL, 10 ) ) < 1 || l2 > INT64_MAX ) {
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
  if ( head.dims[3] != 64 ) {
    lf_error( "requires real64 data" );
    fclose( fpin );
    lfbxFree( &head );
    return 3;
  }
  if ( !head.data_type || strcmp( head.data_type, "real64" ) )
    lf_warning( "treating as real64 data" );

  /* Check lengths. */
  if ( l1 > head.dims[0] )
    l1 = head.dims[0];
  if ( l2 > head.dims[1] )
    l2 = head.dims[1];

  /* Allocate data storage. */
  n = ( l1 > 1 ? head.dims[0] : 1 );
  dat = (double *)malloc( n*head.dims[1]*head.dims[2]*sizeof(double) );
  n = ( l2 > 1 && head.dims[1] > n ? head.dims[1] : n );
  out = (double *)malloc( n*sizeof(double) );
  buf = (double *)malloc( ( l1 > l2 ? l1 : l2 )*sizeof(double) );
  if ( !dat || !out || !buf ) {
    lf_error( "memory error" );
    fclose( fpin );
    if ( dat )
      free( dat );
    if ( out )
      free( out );
    if ( buf )
      free( buf );
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
      free( buf );
      lfbxFree( &head );
      return 2;
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( outfile, "wb" ) ) ) {
    lf_error( "could not open output file %s", outfile );
    fclose( fpin );
    free( dat );
    free( out );
    free( buf );
    lfbxFree( &head );
    return 2;
  }
  if ( lfbxWrite( fpout, &head, NULL ) ) {
    lf_error( "error writing header to %s", outfile );
    fclose( fpout );
    fclose( fpin );
    free( dat );
    free( out );
    free( buf );
    lfbxFree( &head );
    return 2;
  }

  /* Do column filtering, if requested. */
  if ( l1 > 1 ) {
    n = head.dims[0]*head.dims[1]*head.dims[2];
    if ( ( i = fread( dat, sizeof(double), n, fpin ) ) < n ) {
      lf_warning( "read %lld data from %s, expected %lld",
		  (long long)( i ), infile, (long long)( n ) );
      memset( dat + i, 0, ( n - i )*sizeof(double) );
    }
    stride = head.dims[1]*head.dims[2];
    for ( j = 0; j < stride; j++ ) {
      for ( i = 0, sum = 0.0; i < l1; i++ )
	sum += buf[i] = dat[ i*stride + j ];
      for ( i = k = 0; i < head.dims[0] - l1; i++ ) {
	sum -= buf[k];
	sum += buf[k] = dat[ ( i + l1 )*stride + j ];
	out[i] = sum/l1;
	if ( ++k > l1 )
	  k -= l1;
      }
      for ( ; i < head.dims[0]; i++ ) {
	sum -= buf[k];
	out[i] = sum/( head.dims[0] - i );
	if ( ++k > l1 )
	  k -= l1;
      }
      for ( i = 0; i < head.dims[0]; i++ )
	dat[ i*stride + j ] = out[i];
    }
  }

  /* Do row filtering, if requested; write output in any case. */
  n = head.dims[1]*head.dims[2];
  stride = head.dims[2];
  for ( i = 0; i < head.dims[0]; i++ ) {

    /* Get next row of data, reading it if we haven't already. */
    if ( l1 > 1 )
      row = dat + i*n;
    else {
      if ( !feof( fpin ) ) {
	if ( ( j = fread( dat, sizeof(double), n, fpin ) ) < n ) {
	  lf_warning( "read %lld data from %s, expected %lld",
		      (long long)( i*n + j ), infile,
		      (long long)( head.dims[0]*n ) );
	  memset( dat + j, 0, ( n - j )*sizeof(double) );
	}
	row = dat;
      } else {
	memset( out, 0, n*sizeof(double) );
	row = NULL;
      }
    }

    /* Average it if needed. */
    if ( l2 > 1 && row ) {
      stride = head.dims[2];
      for ( z = 0; z < stride; z++ ) {
	for ( j = 0, sum = 0.0; j < l2; j++ )
	  sum += buf[j] = dat[ j*stride + z ];
	for ( j = k = 0; j < head.dims[1] - l2; j++ ) {
	  sum -= buf[k];
	  sum += buf[k] = dat[ ( j + l2 )*stride + z ];
	  out[ j*stride + z ] = sum/l2;
	  if ( ++k > l2 )
	    k -= l2;
	}
	for ( ; j < head.dims[1]; j++ ) {
	  sum -= buf[k];
	  out[ j*stride + z ] = sum/( head.dims[1] - j );
	  if ( ++k > l2 )
	    k -= l2;
	}
      }
    }
    if ( l2 > 1 )
      row = out;
    if ( fwrite( row, sizeof(double), n, fpout ) < n ) {
      lf_error( "could not write data to %s", outfile );
      fclose( fpin );
      fclose( fpout );
      free( dat );
      free( out );
      free( buf );
      lfbxFree( &head );
      return 2;
    }
  }

  /* Finished. */
  fclose( fpin );
  fclose( fpout );
  free( dat );
  free( out );
  free( buf );
  lfbxFree( &head );
  return 0;
}
