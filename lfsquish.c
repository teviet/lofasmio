static const char *version = "\
lfsquish version " VERSION "\n\
Copyright (c) 2019 Teviet Creighton.\n\
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
Reduce the time or frequency resolution of a LoFASM file.\n\
\n\
  -h, --help               print this usage information\n\
  -H, --man                display the program's man page\n\
      --manpage            print the program's man page (groff)\n\
      --markdown           print the program's man page (markdown)\n\
  -V, --version            print program version\n\
  -v, --verbosity=LEVEL    set status message reporting level\n\
  -t, --dim1=FAC1[+OFF1]  downsampling factor, offset in dimension 1\n\
  -f, --dim2=FAC2[+OFF2]  downsampling factor, offset in dimension 2\n\
\n";

static const char *description = "\
# lfsquish(1)\n\
\n\
## NAME\n\
\n\
`lfsquish(1)` - downsample a lofasm-filterbank(5) file\n\
\n\
## SYNOPSIS\n\
\n\
`lfsquish` [_OPTION_]... [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program averages and resamples a lofasm-filterbank(5) file\n\
_INFILE_, writing the result to _OUTFILE_.  If _INFILE_ or _OUTFILE_\n\
is not specified, or is a single `-` character, then standard input or\n\
standard output is used instead.  This program streams its input and\n\
output one row at a time, so it is suitable for use within a pipeline\n\
(e.g. being fed data by lfcat(1) or other programs).\n\
\n\
If run without options, the program will perform the uninteresting\n\
task of copying _INFILE_ to _OUTFILE_.  At least one of the `-t,\n\
--dim1` option or the `-f, --dim2` option, or both, must be given to\n\
perform downsampling.  Conventionally these refer to the time and\n\
frequency axes, respectively, but the program will also work on other\n\
2d spaces (e.g. delay graphs, two-frequency graphs, etc.).\n\
\n\
Each dimension is downsampled by an integer factor, _FAC1_ or _FAC2_\n\
respectively.  That is, the input array is sliced into rectangular\n\
boxes of size *FAC1*x*FAC2*, and the average of each box is recorded\n\
as a point in the output array.  Any incomplete boxes are discarded:\n\
thus the dimensions of the output array are divided by the\n\
corresponding factor, rounded down.  An error is returned if this\n\
results in a dimension of zero.\n\
\n\
If neither downsampling option is given, the program performs the\n\
rather uninteresting task of copying _INFILE_ to _OUTFILE_ unchanged.\n\
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
`-t, --dim1=`_FAC1_[`+`_OFF1_]:\n\
`-f, --dim2=`_FAC2_[`+`_OFF2_]:\n\
    Sets the downsampling factor for the dimension 1 and 2 axes (time\n\
    and frequency, respectively, for standard LoFASM spectrograms).\n\
    _FAC1_ and _FAC2_ must be positive integers, and may not exceed\n\
    the length of their respective axes, else an error occurs.  A\n\
    value of 1 leaves the sampling unchanged.\n\
\n\
    Alternatively, either or both of _FAC1_ and _FAC2_ may take the\n\
    form `/`_N_ (i.e. preceding the integer argument with a slash),\n\
    which then specifies the number of output samples in the\n\
    corresponding dimension.  The downsampling factor is set to the\n\
    corresponding dimension length divided by _N_, rounded down.  As\n\
    above, _N_ must be a positive integer and may not exceed the\n\
    length of the corresponding axis.\n\
\n\
    Optionally, a second integer _OFF1_ and/or _OFF2_ may be specified\n\
    in the option argument (conventionally delimited by a `+` sign).\n\
    If present, this gives a sampling offset: i.e. a number of samples\n\
    to skip from the beginning of dimension 1 or 2 before starting the\n\
    resampling.  If not given, `0` is assumed: it cannot be negative,\n\
    and the sum of the sampling factor and offset cannot exceed the\n\
    dimension length.\n\
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

static const char short_opts[] = "hHVv:t:f:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "dim1", 1, 0, 't' },
  { "dim2", 1, 0, 'f' },
  { 0, 0, 0, 0} };

int
main( int argc, char **argv )
{
  int opt, lopt;          /* option character and index */
  unsigned long long fac[2] = {}; /* downsample factor along dimensions */
  unsigned long long npt[2] = {}; /* specified points along dimensions */
  unsigned long long off[2] = {}; /* offset in each dimension */
  char *infile, *outfile; /* input/output file names */
  FILE *fpin, *fpout;     /* input/output file pointers */
  int64_t lin, lout, nin; /* input/output row lengths, input samples */
  int64_t i, j, k, n;     /* indecies */
  int d;                  /* dimension index */
  lfb_hdr head = {};      /* file header */
  double *in, *out;       /* input block and output row */

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
    case 't': case 'f':
      d = ( opt == 't' ? 0 : 1 );
      if ( optarg[0] == '/' )
	n = sscanf( optarg, "/%lld%lld", fac + d, off + d );
      else
	n = sscanf( optarg, "%lld%lld", fac + d, off + d );
      if ( fac[d] < 1 || fac[d] > INT64_MAX ) {
	lf_error( "bad -%c, --dim%1i argument %s", opt, d + 1, optarg );
	return 1;
      }
      if ( optarg[0] == '/' ) {
	npt[d] = fac[d];
	fac[d] = 0;
      } else
	npt[d] = 0;
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

  /* Compute downsampling factors and output dimensions. */
  for ( i = 0; i < 2; i++ ) {
    if ( npt[i] < 1 ) {
      if ( fac[i] < 1 )
	fac[i] = 1;
      if ( ( npt[i] = ( head.dims[i] - off[i] )/fac[i] ) < 1 ) {
	lf_error( "dim%1li offset %lli + factor %lli exceeds length %li",
		  i, off[i], fac[i], head.dims[i] );
	fclose( fpin );
	lfbxFree( &head );
	return 1;
      }
    } else if ( ( fac[i] = ( head.dims[i] - off[i] )/npt[i] ) < 1 ) {
      lf_error( "dim%1li points %lli + offset %lli exceeds length %li",
		i, npt[i], off[i], head.dims[i] );
      fclose( fpin );
      lfbxFree( &head );
      return 1;
    }
  }

  /* Allocate data storage.  Note that output array doubles as
     accumulator array, so it's the same length as an input row. */
  nin = head.dims[0];
  lin = head.dims[1]*head.dims[2];
  lout = npt[1]*head.dims[2];
  in = (double *)malloc( lin*sizeof(double) );
  out = (double *)malloc( lin*sizeof(double) );
  if ( !in || !out ) {
    lf_error( "memory error" );
    fclose( fpin );
    if ( in )
      free( in );
    if ( out )
      free( out );
    lfbxFree( &head );
    return 4;
  }

  /* Write output file header. */
  head.dim1_span *= (double)( fac[0] )*npt[0]/head.dims[0];
  head.dim2_span *= (double)( fac[1] )*npt[1]/head.dims[1];
  head.dims[0] = npt[0];
  head.dims[1] = npt[1];
  if ( !outfile ) {
    if ( isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) ) ) {
      lf_error( "could not write to stdout" );
      fclose( fpin );
      free( in );
      free( out );
      lfbxFree( &head );
      return 2;
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( outfile, "wb" ) ) ) {
    lf_error( "could not open output file %s", outfile );
    fclose( fpin );
    free( in );
    free( out );
    lfbxFree( &head );
    return 2;
  }
  if ( lfbxWrite( fpout, &head, NULL ) ) {
    lf_error( "error writing header to %s", outfile );
    fclose( fpout );
    fclose( fpin );
    free( in );
    free( out );
    lfbxFree( &head );
    return 2;
  }

  /* Skip off[0] input rows. */
  for ( n = 0; n < off[0] && !feof( fpin ); n++ )
    if ( ( j = fread( in, sizeof(double), lin, fpin ) ) < lin )
      lf_warning( "read %lld data from %s, expected %lld",
		  (long long)( n*lin + j ), infile,
		  (long long)( nin*lin ) );

  /* Generate output row-by-row. */
  for ( i = 0; i < npt[0] && !feof( fpin ); i++ ) {

    /* Read block of fac[0] input lines, accumulating in out array. */
    memset( out, 0, lin*sizeof(double) );
    for ( n = 0; n < fac[0] && !feof( fpin ); n++ ) {
      if ( ( j = fread( in, sizeof(double), lin, fpin ) ) < lin )
	lf_warning( "read %lld data from %s, expected %lld",
		    (long long)( i*fac[0]*lin + n*lin + j ), infile,
		    (long long)( nin*lin ) );
      for ( k = off[1]; k < j; k++ )
	out[k] += in[k];
    }
    if ( n < fac[0] )
      lf_warning( "read %lld data from %s, expected %lld",
		  (long long)( i*fac[0]*lin + n*lin ), infile,
		  (long long)( nin*lin ) );
    for ( k = off[1]; k < lin; k++ )
      out[k] /= fac[0];

    /* Resample in place along dim2. */
    for ( j = 0; j < npt[1]; j++ ) {
      for ( k = 0; k < head.dims[2]; k++ )
	out[k] = out[head.dims[2]*off[1] + k];
      for ( n = 1; n < fac[1]; n++ ) {
	for ( k = 0; k < head.dims[2]; k++ )
	  out[head.dims[2]*j+k] +=
	    out[head.dims[2]*( j*fac[1] + n + off[1] ) + k];
      }
    }
    for ( j = 0; j < lout; j++ )
      out[j] /= fac[1];

    /* Write row out. */
    if ( fwrite( out, sizeof(double), lout, fpout ) < lout ) {
      lf_error( "could not write data to %s", outfile );
      fclose( fpin );
      fclose( fpout );
      free( in );
      free( out );
      lfbxFree( &head );
      return 2;
    }
  }

  /* If input terminated prematurely, explicitly fill remainder of
     output array with zeroes. */
  free( in );
  fclose( fpin );
  memset( out, 0, lout*sizeof(double) );
  for ( ; i < npt[0]; i++ ) {
    if ( fwrite( out, sizeof(double), lout, fpout ) < lout ) {
      lf_error( "could not write data to %s", outfile );
      fclose( fpout );
      free( out );
      lfbxFree( &head );
      return 2;
    }
  }

  /* Finished. */
  fclose( fpout );
  free( out );
  lfbxFree( &head );
  return 0;
}
