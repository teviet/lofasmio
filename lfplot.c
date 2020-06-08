static const char *version = "\
lfplot version " VERSION "\n\
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
Plot one or more rows or columns of a LoFASM data.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
  -g, --geometry=WxH     specify size of plot\n\
  -d, --dimension=DIM    plot columns (DIM=1) or rows (DIM=2)\n\
  -n, --number=N         plot N columns or rows\n\
  -z, --component=COMP   convert complex to real\n\
  -s, --lin=SCALE[+OFF]  convert on a linear scale\n\
  -l, --log=BASE         convert to log scale\n\
  -r, --range=MIN[+MAX]  restrict range of data\n\
\n";

static const char *description = "\
# lfplot(1)\n\
\n\
## NAME\n\
\n\
`lfplot(1)` - plot lofasm-filterbank(5) data\n\
\n\
## SYNOPSIS\n\
\n\
`lfplot` [_OPTION_]... [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program plots one or more rows or columns from _INFILE_ in\n\
lofasm-filterbank(5) format.  The plot is printed to _OUTFILE_ in\n\
PostScript format.  If _INFILE_ or _OUTFILE_ is not specified, or is a\n\
single `-` character, then standard input or standard output is used\n\
instead.\n\
\n\
With no options, the program generates a 256x256 plot of the first row\n\
of _INFILE_.  If the data are complex, then the real and imaginary\n\
components are plotted in red and blue.  If multiple rows or colums\n\
are specified, then they are plotted in colours ranging continuously\n\
from red to brown to dark green to dark cyan to blue, with real and\n\
imaginary components having the same colour.  The `-z, --component`\n\
option can convert complex data to real for plotting.\n\
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
`-g, --geometry=`_W_`x`_H_:\n\
    Specifies the dimensions of the plot, as two positive integers _W_\n\
    and _H_ separated by a `x` character.  The dimensions are in\n\
    PostScript \"points\", which will convert to 1/72 inches on a\n\
    hardcopy device, or pixels on a pixel-mapped display or image.\n\
\n\
`-d, --dimension=`_DIM_:\n\
    Specifies whether to plot data colums (_DIM_=`1`) or rows\n\
    (_DIM_=`2`).\n\
\n\
`-n, --number=`_N_:\n\
    Specifies the number of rows or colums to plot.  By default, only\n\
    the first row or column in _INFILE_ is plotted.  For _N_ greater\n\
    than 1, plots a number of rows or columns equal to _N_,\n\
    approximately equally spaced along the dimension.  If _N_ is\n\
    greater than the dimension length, then every row or column is\n\
    plotted.  If the data are complex (and not converted to real with\n\
    the `-z, --component` option, below), then the number of lines on\n\
    the plot is twice _N_.\n\
\n\
`-z, --component=`_COMP_:\n\
    Specifies how complex data is converted to real numbers before\n\
    mapping to the colour scale.  This option is ignored for real\n\
    data.  The recognized values of _COMP_ are `re`, `im`, `abs`, or\n\
    `arg`, giving the real part, imagimary part, complex magnitude, or\n\
    complex argument (arctangent) of the input.  If not specified,\n\
    `abs` is assumed.  The argument specified by `arg` will range from\n\
    -pi/2 to +pi/2, though it can be remapped by the `-s, --lin`\n\
    option, below.\n\
\n\
`-s, --lin=`_SCALE_[+*OFF*]:\n\
    Performs a linear remapping of the data.  The argument consists of\n\
    a number _SCALE_, with an optional second number _OFF_ directly\n\
    appended with its sign character as a delimiter.  The data are\n\
    first multiplied by _SCALE_, and then optionally added to _OFF_.\n\
\n\
`-l, --log=`_BASE_:\n\
    Converts the output from linear to log scale.  If _BASE_ is a\n\
    number, it is taken as the base of the logarithm; e.g. `2` or\n\
    `10`.  A _BASE_ of `e` specifies the natural logarithm: equivalent\n\
    to base `2.718281828459045`.  A _BASE_ of `dB` specifies output in\n\
    decibels: equivalent to base `1.258925411794167`.  Any rescaling\n\
    specified by the `-s, --lin` option is done before taking the\n\
    logarithm.\n\
\n\
`-r, --range=`_MIN_[+*MAX*]:\n\
    Truncate the range of the data, The argument consists of a number\n\
    _MIN_, with an optional second number _MAX_ directly appended with\n\
    its sign character as a delimiter.  The data are restricted to the\n\
    range [*MIN*,*MAX*].  This is done before any linear or\n\
    logarithmic remapping, above.\n\
\n\
## EXIT STATUS\n\
\n\
The proram exits with status 0 normally, 1 if there is an error\n\
parsing its arguments, 2 on read/write errors, or 3 if the file has\n\
incorrect syntax.\n\
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

static const char short_opts[] = ":hHVv:g:d:n:z:l:s:r:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "geometry", 1, 0, 'g' },
  { "dimension", 1, 0, 'd' },
  { "number", 1, 0, 'n' },
  { "components", 1, 0, 'z' },
  { "log", 1, 0, 'l' },
  { "lin", 1, 0, 's' },
  { "range", 1, 0, 'r' },
  { 0, 0, 0, 0} };

#define LEN 1024 /* character buffer size */

int
main( int argc, char **argv )
{
  int opt, lopt;                 /* option character and index */
  char *infile, *outfile;        /* input/output filenames */
  FILE *fpin, *fpout;            /* input/output file objects */
  double *dat = NULL;            /* block of data read */
  double *plt = NULL;            /* set of data to be plotted */
  int num = 1, dim = 2;          /* -n and -d options */
  int width = 256, height = 256; /* -g option */
  char *zarg = "";               /* -z option */
  double scale = 1.0, off = 0.0; /* -s option */
  double base = 0.0;             /* -l option */
  double min, max;               /* -r option */
  int range = 0;                 /* -r flag */
  lfb_hdr head = {};             /* lofasm header */
  int64_t i, j, jnext, k, n;     /* indecies and number of data read */
  int64_t stride;                /* spacing between data */
  int multi;                     /* plot multiple dim3 components */
  int first, eod = 0;            /* flags for NaN or end-of-data */
  double d, dmin, dmax;          /* datum and extrema */

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
    case 'g':
      if ( sscanf( optarg, "%dx%d", &width, &height ) != 2 ||
	   width < 1 || height < 1 ) {
	lf_error( "bad argyment %s to -g, --geometry", optarg );
	return 1;
      }
    case 'd':
      if ( ( dim = atoi( optarg ) ) < 1 ) {
	lf_error( "bad argyment %s to -d, --dimension", optarg );
	return 1;
      }
      break;
    case 'n':
      if ( ( num = atoi( optarg ) ) < 1 ) {
	lf_error( "bad argyment %s to -n, --number", optarg );
	return 1;
      }
      break;
    case 'z':
      zarg = optarg;
      if ( strcmp( zarg, "re" ) && strcmp( zarg, "im" ) &&
	   strcmp( zarg, "abs" ) && strcmp( zarg, "arg" ) ) {
	lf_error( "bad argument %s to -z, --components", optarg );
	return 1;
      }
      break;
    case 's':
      if ( sscanf( optarg, "%lf%lf", &scale, &off ) < 1 ) {
	lf_error( "bad argument %s to -s, --lin", optarg );
	return 1;
      }
      break;
    case 'l':
      if ( !strcmp( optarg, "dB" ) )
	base = pow( 10.0, 0.1 );
      else if ( !strcmp( optarg, "e" ) )
	base = M_E;
      else if ( ( base = atof( optarg ) ) <= 0.0 ) {
	lf_error( "bad argument %s to -l, --log", optarg );
	return 1;
      }
      base = 1.0/log( base );
      break;
    case 'r':
      if ( ( range = sscanf( optarg, "%lf%lf", &min, &max ) ) < 1 ) {
	lf_error( "bad argument %s to -r, --range", optarg );
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
      lf_error( "internal error parsing option code %c\n"
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
  if ( lfbxRead( fpin, &head, NULL ) > 1 ) {
    lf_error( "could not parse header from %s", infile );
    fclose( fpin );
    lfbxFree( &head );
    return 2;
  }

  /* Check output type. */
  if ( head.dims[3] != 64 ) {
    lf_error( "require real64 data" );
    fclose( fpin );
    lfbxFree( &head );
    return 1;
  }
  if ( head.data_type ) {
    if ( !strstr( head.data_type, "real64" ) )
      lf_warning( "reading as real64 not %s",
		  head.data_type + strlen( "%data_type: " ) );
  }

  /* Allocate data. */
  n = ( dim == 1 ? head.dims[0] : 1 );
  n *= head.dims[1]*head.dims[2];
  if ( !( dat = (double *)malloc( n*sizeof(double) ) ) ) {
    lf_error( "memory error" );
    fclose( fpin );
    lfbxFree( &head );
    return 4;
  }

  /* Open output file. */
  if ( !outfile ) {
    if ( isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) ) ) {
      lf_error( "could not write to stdout" );
      fclose( fpin );
      free( dat );
      lfbxFree( &head );
      return 2;
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( outfile, "wb" ) ) ) {
    lf_error( "could not open output file %s", outfile );
    fclose( fpin );
    free( dat );
    lfbxFree( &head );
    return 2;
  }

  /* Write PostScript header. */
  fprintf( fpout, "%%!PS-Adobe-3.0 EPSF-3.0\n" );
  if ( outfile )
    fprintf( fpout, "%%%%Title: %s\n", outfile );
  fprintf( fpout, "%%%%Creator: lfplot2d\n" );
  fprintf( fpout, "%%%%Orientation: Portrait\n"
	   "%%%%Pages: 0\n"
	   "%%%%LanguageLevel: 2\n"
	   "%%%%BoundingBox: 0 0 %i %i\n"
	   "%%%%EndComments\n\n"
	   "%%%%BeginProlog\n"
	   "/m { moveto } def\n"
	   "/l { lineto } def\n"
	   "%%%%EndProlog\n", width, height );

  /* Set up extrema. */
  dmin = strtod( "+inf", 0 );
  dmax = strtod( "-inf", 0 );

  /* Are we plotting multiple components? */
  multi = ( head.dims[2] != 1 && ( head.dims[2] != 2 || !zarg[0] ) );
  if ( multi )
    num *= head.dims[2];
  if ( head.dims[2] != 2 && zarg[0] )
    lf_warning( "ignoring --components=%s on non-complex data", zarg );

  /* Read entire array if plotting columns. */
  stride = head.dims[2];
  if ( dim == 1 ) {
    if ( ( k = fread( dat, sizeof(double), n, fpin ) ) < n ) {
      lf_warning( "read %lld numbers from %s, expected %lld",
		  (long long)( k ), infile, (long long)( n ) );
      memset( dat + k, 0, ( n - k )*sizeof(double) );
    }
    stride *= head.dims[1];
  }

  /* Make plots. */
  j = -1;
  for ( i = 0; i < num; i++ ) {
    if ( dim == 1 )
      plt = dat + ( ( i*head.dims[1] )/num )*head.dims[2]
	+ ( multi ? i%head.dims[2] : 0 );
    else {
      if ( multi )
	jnext = ( (i/head.dims[2])*head.dims[0] )/(num/head.dims[2]);
      else
	jnext = ( i*head.dims[0]/num );
      for ( ; !eod && j < jnext; j++ )
	if ( ( k = fread( dat, sizeof(double), n, fpin ) ) < n ) {
	  lf_warning( "read %lld numbers from %s, expected %lld",
		      (long long)( j*n + k ), infile,
		      (long long)( head.dims[0]*n ) );
	  memset( dat + k, 0, ( n - k )*sizeof(double) );
	  eod = 1;
	}
      if ( eod == 1 ) {
	memset( dat, 0, n*sizeof(double) );
	eod = 2;
      }
      plt = dat;
    }

    /* Apply transformations. */
    if ( zarg[0] ) {
      if ( !strcmp( zarg, "im" ) )
	for ( k = 0; k < head.dims[dim-1]; k++ )
	  plt[k*stride] = plt[k*stride+1];
      else if ( !strcmp( zarg, "abs" ) )
	for ( k = 0; k < head.dims[dim-1]; k++ )
	  plt[k*stride] = hypot( plt[k*stride], plt[k*stride+1] );
      else if ( !strcmp( zarg, "arg" ) )
	for ( k = 0; k < head.dims[dim-1]; k++ )
	  plt[k*stride] = atan2( plt[k*stride+1], plt[k*stride] );
    }
    /*if ( scale != 1.0 || off != 0.0 )
      for ( k = 0; k < head.dims[dim-1]; k++ )
	plt[k*stride] = scale*plt[k*stride] + off;
    if ( base != 0.0 )
      for ( k = 0; k < head.dims[dim-1]; k++ )
      plt[k*stride] = log( plt[k*stride] )*base;*/

    /* Make plot. */
    if ( i <= 0.5*num )
      fprintf( fpout, "%f %f 0 setrgbcolor\n", 1.0 - 2.0*i/( num - 0.999 ),
	       1.5*i/( num - 0.999 ) );
    else
      fprintf( fpout, "0 %f %f setrgbcolor\n", 1.5 - 1.5*i/( num - 0.999 ),
	       2.0*i/( num - 0.999 ) - 1.0 );
    for ( k = 0, first = 1; k < head.dims[dim-1]; k++ ) {
      d = ( multi ? plt[k*stride] : plt[ k*stride + i%head.dims[2] ] );
      if ( isnan( d ) || isinf( d ) )
	first = 1;
      else {
	if ( range > 0 && !( d > min ) )
	  d = min;
	if ( range > 0 && !( d < max ) )
	  d = max;
	if ( scale != 1.0 || off != 0.0 )
	  d = scale*d + off;
	if ( base != 0.0 )
	  d = log( d )*base;
	if ( d < dmin )
	  dmin = d;
	if ( d > dmax )
	  dmax = d;
	fprintf( fpout, "%f %f ", width*k/(double)( head.dims[dim-1] ),
		 height*d );
	if ( first ) {
	  fprintf( fpout, "m\n" );
	  first = 0;
	} else
	  fprintf( fpout, "l\n" );
      }
    }
    fprintf( fpout, "stroke\n" );
  }

  /* Finished. */
  fprintf( fpout, "%%%%EOF\n" );
  lf_info( "scaled data: [%g,%g]", dmin, dmax );
  fclose( fpout );
  fclose( fpin );
  free( dat );
  lfbxFree( &head );
  return 0;
}
