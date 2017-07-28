static const char *version = "\
lfchop version " VERSION "\n\
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
Extract a time range from LoFASM data files.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
  -t, --time=TMIN+TMAX   specify time range in s (J2000)\n\
  -n, --step=NMIN+NMAX   specify time range in steps\n\
\n";

static const char *description = "\
# lfchop(1)\n\
\n\
## NAME\n\
\n\
`lfchop(1)` - extract frequency band from LoFASM data\n\
\n\
## SYNOPSIS\n\
\n\
`lfchop` [_OPTION_]... [_INFILE_ [_OUTFILE_]]\n\
\n\
## DESCRIPTION\n\
\n\
This program reads data from _INFILE_ in lofasm-filterbank(5) format,\n\
extracts a specified time range, and writes the resuly to _OUTFILE_ in\n\
the same format.  If _INFILE_ or _OUTFILE_ is not specified, or is a\n\
single `-` character, then standard input or standard output is used\n\
instead.\n\
\n\
The time range should be specified with either the `-t, --time` or\n\
`-n, --step` option, below.  Without options, the program will\n\
trivially copy input to output.\n\
\n\
Time or step ranges specified of the form _TMIN_`+`_TMAX_ or\n\
_NMIN_`+`_NMAX_ are interpreted as ranges close on the lower end and\n\
open at the upper end; e.g. [*TMIN*,*TMAX*).  That is, a timestep is\n\
selected if its step number or initial time is greater than or equal\n\
to the lower bound, and strictly less than the upper bound.  This\n\
convention is chosen so that if you make a sequence of calls to\n\
`lfchop`, with the upper bound of one call becoming the lower bound of\n\
the next, then the output files will cover every step without\n\
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
`-t, --time=`_TMIN_`+`_TMAX_:\n\
    Specifies the time range to extract in seconds from J2000.  The\n\
    arguments are read as two concatnated double-precision floats: the\n\
    `+` sign of _TMAX_ is used to delimit it from _TMIN_.\n\
\n\
`-n, --step=`_NMIN_`+`_NMAX_:\n\
    Specifies the time range to extract in steps from the start of the\n\
    file.  The arguments are read as two concatnated integers: the `+`\n\
    sign of _NMAX_ is used to delimit it from _NMIN_.\n\
\n\
## EXIT STATUS\n\
\n\
The proram exits with status 0 normally, 1 if there is an error\n\
parsing its arguments, 2 on read/write errors, 3 if the requested time\n\
range does not overlap with the input file, and 4 on memory allocation\n\
errors.\n\
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

static const char short_opts[] = ":hHVv:t:n:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "time", 1, 0, 't' },
  { "step", 1, 0, 'n' },
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
  double tmin, tmax;      /* time range to extract (s) */
  int nmin, nmax;         /* timestep range to extract */
  char mode = '\0';       /* whether -t or -n was specified */
  int j = 0;              /* index over time */
  int nrow;               /* bytes in a single row */

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
    case 't':
      if ( sscanf( optarg, "%lf%lf", &tmin, &tmax ) < 2 ) {
	lf_error( "could not parse time range" );
	return 1;
      }
      mode = 't';
      if ( tmin > tmax ) {
	double temp = tmin;
	tmin = tmax;
	tmax = temp;
      }
      break;
    case 'n':
      if ( sscanf( optarg, "%d%d", &nmin, &nmax ) < 2 ) {
	lf_error( "could not parse timestep range" );
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

  /* Get timestep range. */
  if ( mode == 't' ) {
    nmin = (int)ceil( ( tmin - head.dim1_start - head.time_offset_J2000 )
		      *head.dims[0]/head.dim1_span );
    nmax = (int)ceil( ( tmax - head.dim1_start - head.time_offset_J2000 )
		      *head.dims[0]/head.dim1_span );
  } else if ( mode != 'n' ) {
    nmin = 0;
    nmax = head.dims[1];
  }
  nmin = ( nmin < 0 ? 0 : nmin );
  nmax = ( nmax > head.dims[0] ? head.dims[0] : nmax );
  if ( nmax <= nmin ) {
    lf_error( "requested times span no timesteps" );
    fclose( fpin );
    lfbxFree( &head );
    return 3;
  }
  nrow = head.dims[1]*head.dims[2]*head.dims[3]/8;

  /* Allocate data. */
  if ( !( row = (unsigned char *)malloc( nrow ) ) ) {
    lf_error( "memory error" );
    fclose( fpin );
    lfbxFree( &head );
    return 4;
  }

  /* Adjust header parameters and write output header. */
  head.dim1_start += nmin*head.dim1_span/head.dims[0];
  head.dim1_span *= (double)( nmax - nmin )/(double)( head.dims[0] );
  head.dims[0] = nmax - nmin;
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
  for ( j = 0; j < nmin && fread( row, 1, nrow, fpin ) == nrow; j++ )
    ;
  if ( j == nmin )
    for ( ; j < nmax && fread( row, 1, nrow, fpin ) == nrow; j++ )
      if ( fwrite( row, 1, nrow, fpout ) < nrow ) {
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
