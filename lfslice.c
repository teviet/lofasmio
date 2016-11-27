static const char *version = "\
lfslice version " VERSION "\n\
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
Extract a frequency range from LoFASM data files.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
  -f, --freq=FMIN+FMAX   specify frequency range in Hz\n\
  -n, --bin=NMIN+NMAX    specify frequency range in bins\n\
\n";

static const char *description = "\
# lfslice(1)\n\
\n\
## NAME\n\
\n\
`lfslice` - extract frequency band from LoFASM data\n\
\n\
## SYNOPSIS\n\
\n\
`lfslice` [_OPTION_]... [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program reads data from _INFILE_ in lofasm-filterbank(5) format,\n\
extracts a specified frequency range, and writes the passed band to\n\
_OUTFILE_ in the same format.  If _INFILE_ or _OUTFILE_ is not\n\
specified, or is a single `-` character, then standard input or\n\
standard output is used instead.\n\
\n\
The frequency range should be specified with either the `-f, --freq`\n\
or `-n, --bin` option, below.  Without options, the program will\n\
trivially copy input to output.\n\
\n\
Frequency or bin ranges specified of the form _FMIN_`+`_FMAX_ or\n\
_NMIN_`+`_NMAX_ are interpreted as ranges close on the lower end and\n\
open at the upper end; e.g. [*FMIN*,*FMAX*).  That is, a channel is\n\
selected if its bin number or lower frequency edge is greater than or\n\
equal to the lower bound, and strictly less than the upper bound.\n\
This convention is chosen so that if you make a sequence of calls to\n\
`lfslice`, with the upper bound of one call becoming the lower bound\n\
of the next, then the output files will cover every channel without\n\
duplication or gaps.\n\
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
`-f, --freq=`_FMIN_`+`_FMAX_:\n\
    Specifies the frequency range to extract in Hz.  The arguments are\n\
    read as two concatnated double-precision floats: the `+` sign of\n\
    _FMAX_ is used to delimit it from _FMIN_.\n\
\n\
`-n, --bin=`_NMIN_`+`_NMAX_:\n\
    Specifies the frequency range to extract in bins (channels).  The\n\
    arguments are read as two concatnated integers: the `+` sign of\n\
    _NMAX_ is used to delimit it from _NMIN_.\n\
\n\
## EXIT STATUS\n\
\n\
The proram exits with status 0 normally, 1 if there is an error\n\
parsing its arguments, 2 on read/write errors, 3 if the requested\n\
frequency range does not overlap with the input file, and 4 on memory\n\
allocation errors.\n\
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

static const char short_opts[] = ":hHVv:f:n:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "freq", 1, 0, 'f' },
  { "bin", 1, 0, 'n' },
  { 0, 0, 0, 0} };

#define LEN 1024 /* character buffer size */

int
main( int argc, char **argv )
{
  char opt;               /* short option character */
  int lopt;               /* long option index */
  char *infile, *outfile; /* input/output filenames */
  FILE *fpin, *fpout;     /* input/output file objects */
  lfb_hdr head = {};      /* input/output filterbank header */
  unsigned char *row;     /* single timestep of data */
  double fmin, fmax;      /* frequency range to extract (Hz) */
  int nmin, nmax;         /* bin range to extract */
  char mode = '\0';       /* whether -f or -n was specified */
  int j = 0;              /* index over time */
  int nbin, nrow, nslice; /* bytes in a single bin, row, or slice */

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
    case 'f':
      if ( sscanf( optarg, "%lf%lf", &fmin, &fmax ) < 2 ) {
	lf_error( "could not parse frequency range" );
	return 1;
      }
      mode = 'f';
      if ( fmin > fmax ) {
	double temp = fmin;
	fmin = fmax;
	fmax = temp;
      }
      break;
    case 'n':
      if ( sscanf( optarg, "%d%d", &nmin, &nmax ) < 2 ) {
	lf_error( "could not parse bin range" );
	return 1;
      }
      mode = 'n';
      if ( nmin > nmax ) {
	int temp = nmin;
	nmin = nmax;
	nmax = temp;
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
  if ( lfbxRead( fpin, &head, NULL ) ) {
    lf_error( "could not parse header from %s", infile );
    fclose( fpin );
    lfbxFree( &head );
    return 2;
  }

  /* Get bin range. */
  if ( mode == 'f' ) {
    nmin = (int)ceil( ( fmin - head.dim2_start - head.frequency_offset_DC )
		      *head.dims[1]/head.dim2_span );
    nmax = (int)ceil( ( fmax - head.dim2_start - head.frequency_offset_DC )
		      *head.dims[1]/head.dim2_span );
  } else if ( mode != 'n' ) {
    nmin = 0;
    nmax = head.dims[1];
  }
  nmin = ( nmin < 0 ? 0 : nmin );
  nmax = ( nmax > head.dims[1] ? head.dims[1] : nmax );
  if ( nmax <= nmin ) {
    lf_error( "frequencies span no channel boundaries" );
    fclose( fpin );
    lfbxFree( &head );
    return 3;
  }
  nbin = head.dims[2]*head.dims[3]/8;
  nrow = head.dims[1]*nbin;
  nslice = ( nmax - nmin )*nbin;

  /* Allocate data. */
  if ( !( row = (unsigned char *)malloc( nrow ) ) ) {
    lf_error( "memory error" );
    fclose( fpin );
    lfbxFree( &head );
    return 4;
  }

  /* Adjust header parameters and write output header. */
  head.dim2_start += nmin*head.dim2_span/head.dims[1];
  head.dim2_span *= (double)( nmax - nmin )/(double)( head.dims[1] );
  head.dims[1] = nmax - nmin;
  if ( !outfile ) {
    if ( isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) ) ) {
      lf_error( "could not write to stdout" );
      fclose( fpin );
      free( row );
      lfbxFree( &head );
      return 2;
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( outfile, "wb" ) ) ) {
    lf_error( "could not open output file %s", outfile );
    fclose( fpin );
    free( row );
    lfbxFree( &head );
    return 2;
  }
  if ( lfbxWrite( fpout, &head, NULL ) ) {
    lf_error( "error writing header to %s", outfile );
    fclose( fpout );
    fclose( fpin );
    free( row );
    lfbxFree( &head );
    return 2;
  }

  /* Extract data from data block. */
  for ( j = 0; j < head.dims[0] && fread( row, 1, nrow, fpin ) == nrow; j++ )
    if ( fwrite( row + nmin*nbin, 1, nslice, fpout ) < nslice ) {
      lf_error( "error writing data to %s", outfile );
      fclose( fpout );
      fclose( fpin );
      free( row );
      lfbxFree( &head );
      return 2;
    }
  fclose( fpout );
  fclose( fpin );
  free( row );
  lfbxFree( &head );
  if ( j < head.dims[0] )
    lf_warning( "read %d rows from %s, expected %d", j, infile, head.dims[0] );
  return 0;
}
