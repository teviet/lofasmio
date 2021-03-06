static const char *version = "\
lfstats version " VERSION "\n\
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
Usage: %s [OPTION]... [INFILE] [> OUTFILE]\n\
Compute various statistics from a LoFASM file.\n\
\n\
  -h, --help              print this usage information\n\
  -H, --man               display the program's man page\n\
      --manpage           print the program's man page (groff)\n\
      --markdown          print the program's man page (markdown)\n\
  -V, --version           print program version\n\
  -v, --verbosity=LEVEL   set status message reporting level\n\
  -p, --percent=P1[+...]  compute percentiles\n\
  -m, --moments=N         compute up to Nth moment\n\
\n";

static const char *description = "\
# lfstats(1)\n\
\n\
## NAME\n\
\n\
`lfstats(1)` - get statistics of lofasm-filterbank(5) file\n\
\n\
## SYNOPSIS\n\
\n\
`lfstats` [_OPTION_]... [_INFILE_] [`>` _OUTFILE_]\n\
\n\
## DESCRIPTION\n\
\n\
This program computes various statistical properties of a\n\
lofasm-filterbank(5) file.  By default it computes the maximum,\n\
minimum, mean, and standard deviation.  Options generate additional\n\
statistics.  If _INFILE_ is absent or a single `-` character, then\n\
data is read from standard input; statistics are written to standard\n\
output.\n\
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
`-p, --percent=`_P1_[+...]:\n\
    Computes specified percentiles.  For instance, `--percent=50` will\n\
    compute the median, `--percent=25+50+75` will compute the median\n\
    and quartiles.  `--percent=0+100` will compute the maximum and\n\
    minimum, but these are generated by default without this option.\n\
\n\
    Percentiles are ordinal statistics, and thus are robust against\n\
    fluctuations or nonlinear transformations of the data.  However,\n\
    they require the entire file to be loaded into memory and sorted,\n\
    which may tax system resources.  Other statistics can be computed\n\
    progressively.\n\
\n\
`-m, --moments=`_N_:\n\
    Computes moments of the data up to number _N_.  The number of\n\
    points (_N_=0), mean (_N_=1), and standard deviation (_N_=2) are\n\
    computed by default, without this option.  Higher values of _N_\n\
    compute standardized moments (i.e. moments about the mean and\n\
    normalized by the standard deviation).  That is, if <...>\n\
    represents the expectation value of a quantity and _xi_ are the\n\
    data in the file, then we define:\n\
\n\
>> mean: _x_ = < _xi_ >  \n\
>> standard deviation: _s_ = sqrt[ < ( _xi_ - _x_ )^2 > ]  \n\
>> standard moment _k_>2: _mk_ = < ( _xi_ - _x_ )^ _k_ >/( _s_ ^ _k_ )\n\
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
#include <getopt.h>
#include <math.h>
#include "markdown_parser.h"
#include "lofasmIO.h"

static const char short_opts[] = "hHVv:p:m:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "percent", 1, 0, 'p' },
  { "momemnts", 1, 0, 'm' },
  { 0, 0, 0, 0} };

/* Comparison function for sorting data.  The comparison is written so
   that NaNs will be sorted below anything else. */
int
ascend( const void *p1, const void *p2 )
{
  double diff = *( (double *)(p1) ) - *( (double *)(p2) );
  return ( diff > 0.0 ? 1 : ( diff == 0.0 ? 0 : -1 ) );
}

/* Algorithm from stackoverflow for computing binomial coefficient
   without overflow. */
long long
combi( int n, int k )
{
  long long ans = 1;
  int j = 1;
  k = ( k > n - k ? n - k : k );
  for( ; j <= k; j++, n-- )
    if ( n%j == 0 )
      ans *= n/j;
    else if ( ans%j == 0 )
      ans = ( ans/j )*n;
    else
      ans = ( ans*n )/j;
  return ans;
}

int
main( int argc, char **argv )
{
  int opt, lopt;            /* option character and index */
  char *percent = NULL;     /* list of percentiles */
  char *a, *b;              /* pointers witin percent */
  int moments = 2;          /* highest-order moment */
  double *mk;               /* array of moments */
  char *infile;             /* input file name */
  FILE *fp = NULL;          /* file pointer */
  lfb_hdr head = {};        /* input header */
  int64_t i, j, imax, jmax; /* indecies and ranges in dims 1 and 2 */
  int64_t n;                /* number of data read */
  int k;                    /* index over moments */
  double *data;             /* array of data, or just one row */
  double d, min, max;       /* datum, minimum, and maximum */

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
    case 'p':
      percent = optarg;
      for ( b = percent, a = NULL; a != b; )
	if ( ( d = strtod( a = b, &b ) ) < 0.0 || d > 100.0 ) {
	  lf_error( "bad percentile %f in %s", d, percent );
	  return 1;
	}
      if ( b[0] ) {
	lf_error( "bad percentile argument %s", percent );
	return 1;
      }
      break;
    case 'm':
      if ( sscanf( optarg, "%d", &moments ) < 1 || moments < 0 ) {
	lf_error( "bad moments argument %s", optarg );
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
  if ( optind < argc ) {
    lf_error( "too many arguments" );
    return 1;
  }

  /* Allocate moments. */
  if ( !( mk = (double *)calloc( moments + 1, sizeof(double) ) ) ) {
    lf_error( "memory error" );
    return 4;
  }

  /* Set initial maximum and minimum. */
  min = strtod( "+inf", 0 );
  max = strtod( "-inf", 0 );

  /* Read input header. */
  if ( !infile ) {
    if ( !( fp = lfdopen( 0, "rb" ) ) ) {
      lf_error( "could not read stdin" );
      free( mk );
      return 2;
    }
    infile = "stdin";
  } else if ( !( fp = lfopen( infile, "rb" ) ) ) {
    lf_error( "could not open input file %s", infile );
    fclose( fp );
    free( mk );
    return 2;
  }
  if ( lfbxRead( fp, &head, NULL ) ) {
    lf_error( "could not parse header from %s", infile );
    fclose( fp );
    free( mk );
    lfbxFree( &head );
    return 2;
  }

  /* Check data type. */
  if ( head.dims[3] != 64 ) {
    lf_error( "requires real64 data" );
    fclose( fp );
    free( mk );
    lfbxFree( &head );
    return 3;
  }
  if ( !head.data_type || strcmp( head.data_type, "real64" ) )
    lf_warning( "treating as real64 data" );

  /* Allocate data storage. */
  if ( percent ) {
    imax = 1;
    jmax = head.dims[0]*head.dims[1]*head.dims[2];
  } else {
    imax = head.dims[0];
    jmax = head.dims[1]*head.dims[2];
  }
  if ( !( data = (double *)malloc( jmax*sizeof(double) ) ) ) {
    lf_error( "memory error" );
    fclose( fp );
    free( mk );
    lfbxFree( &head );
    return 4;
  }

  /* Read data, updating moments and extrema as we go. */
  for ( i = 0; i < imax; i++ ) {
    if ( feof( fp ) )
      memset( data, 0, jmax*sizeof(double) );
    else if ( ( n = fread( data, sizeof(double), jmax, fp ) ) < jmax ) {
      lf_warning( "read %lld doubles, expected %lld",
		  (long long)( i*jmax + n ), (long long)( imax*jmax ) );
      memset( data + n, 0, ( jmax - n )*sizeof(double) );
    }
    for ( j = 0; j < jmax; j++ ) {
      d = data[j];
      for ( k = 1; k <= moments; k++ )
	mk[k] += pow( d, k );
      if ( d < min )
	min = d;
      if ( d > max )
	max = d;
    }
  }
  fclose( fp );

  /* Compute all standardized moments. */
  mk[0] = imax*jmax;
  /* Convert sums to expectation values. */
  for ( k = moments; k >= 0; k-- )
    mk[k] /= mk[0];
  /* Convert moments about 0 to moment about the mean. */
  for ( k = moments; k >= 2; k-- )
    for ( j = k - 1; j >= 0; j-- )
      mk[k] += mk[j]*combi( k, j )*pow( -mk[1], k - j );
  /* Normalize by standard deviation. */
  if ( moments >= 2 )
    mk[2] = sqrt( mk[2] );
  for ( k = 3; k <= moments; k++ )
    mk[k] /= pow( mk[2], k );

  /* Write arithmetic stats. */
  printf( "npts:   %lld\n", (long long)( imax*jmax ) );
  if ( moments > 0 )
    printf( "mean:   %g\n", mk[1] );
  if ( moments > 1 )
    printf( "stddev: %g\n", mk[2] );
  for ( k = 3; k <= moments; k++ )
    printf( "m[%d]:   %g\n", k, mk[k] );
  printf( "range: [ %g, %g ]\n", min, max );
  free( mk );
  lfbxFree( &head );

  /* Print percentiles. */
  if ( percent ) {
    qsort( data, jmax, sizeof(double), ascend );
    for ( b = percent, a = NULL; a != b; ) {
      d = strtod( a = b, &b );
      if ( a != b )
	printf( "%5.1f %%ile: %g\n", d,
		data[(int)floor( 0.01*d*( jmax - 1 ) + 0.5 )] );
    }
  }
  return 0;
}
