static const char *version = "\
lfcat version " VERSION "\n\
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
Usage: %s [OPTION]... [INFILE]... OUTFILE\n\
Combine consecutive LoFASM files in time.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
  -p, --pad=VALUE        set floating-point value to pad gaps\n\
  -m, --medpad=STEPS     pad gaps using median of STEPS on eiter side\n\
  -l, --medlin=STEPS     as -m but interpolate between either side\n\
  -a, --align-warn=EPS   set threshold for alignment warnings\n\
  -A, --align-err=EPS    set threshold for alignment errors\n\
\n";

static const char *description = "\
# lfcat(1)\n\
\n\
## NAME\n\
\n\
`lfcat(1)` - concatenate lofasm-filterbank(5) files\n\
\n\
## SYNOPSIS\n\
\n\
`lfcat` [_OPTION_]... [_INFILE_]... _OUTFILE_\n\
\n\
## DESCRIPTION\n\
\n\
This program concatenates multiple lofasm-filterbank(5) files\n\
_INFILE_... that span the same frequency range but cover\n\
non-overlapping time spans.  The time spans need not be continuous:\n\
gaps will be filled with 0 or another value specified by the\n\
`-p, --pad` option.  The input files need not be given in order.  The\n\
earliest file in the sorted list is used to specify the initial start\n\
time and sampling interval (timestep).\n\
\n\
At least one non-option argument _OUTFILE_ must be given, indicating\n\
the name of the output file.  If _OUTFILE_ is a single `-` character,\n\
the result will be written to standard output.  If no other non-option\n\
argument _INFILE_ is given, a single input file is read from standard\n\
input, resulting in a \"trivial\" concatenation.  Also, any one of\n\
_INFILE_ may be a single `-` character, indicating a file to be read\n\
from standard input.\n\
\n\
The input files must have identical frequency ranges and sample\n\
spacings, identical time sample spacings, and cannot overlap in time,\n\
or the program will exit with an error.  Alignment errors (i.e. when a\n\
timestep in the output file does not exactly match the timestep in the\n\
input file) are resolved by picking the nearest input timestep; more\n\
sophisticated interpolation methods are not currently implemented.  An\n\
information messages (see `-v, --verbosity` below) will report any\n\
misalignment; options allow this to trigger warnings or errors at\n\
user-specified thresholds.\n\
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
`-p, --pad=`_VALUE_:\n\
    Sets a value to be used to pad gaps between input files.  The\n\
    value will be parsed as a floating-point number: in addition to\n\
    ordinary numbers, you may also specify `nan`, `inf`, etc.  The\n\
    default is `0`.  If _VALUE_ is a single `-` character, then\n\
    padding is not performed: the program will exit with an error if a\n\
    gap is encountered.\n\
\n\
`-m, --medpad=`_STEPS_:\n\
    Pad gaps between files using a median average of the points on\n\
    either side of the gap.  The median is computed using up to\n\
    _STEPS_ timesteps on either side, but never more than half the\n\
    length of the file on that side.  (Technically we use the \"upper\n\
    median\", i.e. the sample _NSTEP_ of the sorted data, rather than\n\
    the \"true\" median.)  Each component of each frequency bin is\n\
    averaged separately.\n\
\n\
`-l, --medlin=`_STEPS_:\n\
    As `-m, --medpad`, above, except that two separate medians are\n\
    computed, one each using data on either side of the gap; the gap\n\
    itself is padded with a linear interpolation between the two\n\
    medians.  This can reduce some edge effects if the average level\n\
    changes across the gap.\n\
\n\
`-a, --align-warn=`_EPS_:\n\
    Prints a warning message if the alignment to equal-spaced output\n\
    timesteps causes an input time to be shifted more than _EPS_\n\
    timesteps: this should be a positive fraction less than 1.  Only\n\
    the largest shift is reported.  Regardless of this option, the\n\
    largest nonzero alignment shift will be reported in very verbose\n\
    mode (see `--verbosity`, above).\n\
\n\
`-A, --align-err=`_EPS_:\n\
    As `--align-warn`, above, except that a shift exceeding _EPS_\n\
    timesteps triggers an error message and exits the program.  You\n\
    can specify both `--align-warn` and `--align-err` with different\n\
    thresholds.\n\
\n\
## EXIT STATUS\n\
\n\
The proram exits with status 0 normally, 1 if there is an error\n\
parsing its arguments, 2 on read/write errors, 3 if the files have\n\
misaligned frequencies, misaligned timesteps, gaps (if \"no padding\"\n\
was specified), or incompatible metadata, and 4 on memory allocation\n\
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

static const char short_opts[] = "hHVv:p:m:l:a:A:";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "pad", 1, 0, 'p' },
  { "medpad", 1, 0, 'm' },
  { "medlin", 1, 0, 'l' },
  { "align-warn", 1, 0, 'a' },
  { "align-err", 1, 0, 'A' },
  { 0, 0, 0, 0} };

#define LEN 1024 /* character buffer size */

/* Array and comparison function for sorting headers in time. */
lfb_hdr *headers = NULL; /* list of headers to sort */
int
ascendingTimes( const void *p1, const void *p2 )
{
  lfb_hdr *h1 = headers + *( (int *)(p1) );
  lfb_hdr *h2 = headers + *( (int *)(p2) );
  double diff = h1->time_offset_J2000 - h2->time_offset_J2000;
  diff += h1->dim1_start - h2->dim1_start;
  return ( diff > 0.0 ? 1 : ( diff < 0.0 ? -1 : 0 ) );
}

/* Array and comparison function for sorting data. */
double *mddata = NULL; /* 2d array of doubles */
int stride;            /* number of doubles in timestep */
int
ascendingDoubles( const void *p1, const void *p2 )
{
  double diff = mddata[ *( (int *)(p1) )*stride ];
  diff -= mddata[ *( (int *)(p2) )*stride ];
  return ( diff > 0.0 ? 1 : ( diff < 0.0 ? -1 : 0 ) );
}


/* Macro to free all memory and close all files, prior to exiting.
   This should only within main(), which should initialize all
   unallocated pointers to NULL to avoid potentially trying to free
   random pointer values. */
#define CLEANEXIT( code ) \
do { \
  lfbxFree( &header ); \
  if ( row ) free( row ); \
  if ( gap ) free( gap ); \
  if ( idx ) free( idx ); \
  if ( mddx ) free( mddx ); \
  if ( headers ) free( headers ); \
  if ( data ) free( data ); \
  if ( data1 ) free( data1 ); \
  if ( data2 ) free( data2 ); \
  if ( fpin ) fclose( fpin ); \
  if ( fpout ) fclose( fpout ); \
  exit( code ); \
} while ( 0 )


int
main( int argc, char **argv )
{
  char opt;                  /* short option character */
  int lopt;                  /* long option index */
  float awarn = 2, aerr = 2; /* alignment warning/error thresholds */
  int gaps = 0, dopad = 1;   /* number of gaps and whether to pad them */
  double padd = 0.0;         /* pad value for gaps */
  float padf = 0.0;          /* pad value if data are real32 */
  int med = 0, lin = 0;      /* number of median steps; interpolate */
  int med1 = 0, med2 = 0;    /* median steps on either side of gap */
  FILE *fpin = NULL;         /* input file */
  FILE *fpout = NULL;        /* output file */
  int istd = 0;              /* whether input is stdin */
  char *infile, *outfile;    /* input/output filenames */
  lfb_hdr header = {};       /* single-input or output header */
  unsigned char *row = NULL; /* single timestep of data */
  unsigned char *gap = NULL; /* single missing timestep */
  int nrow;                  /* number of bytes per row */
  int nin;                   /* number of input files */
  int *idx = NULL;           /* index array of sorted input files */
  int *mddx = NULL;          /* index array of sorted data */
  double *data = NULL;       /* row stored as doubles */
  double *data1 = NULL;      /* median of preceding chunk */
  double *data2 = NULL;      /* median of following chunk */
  int i, j, k, klast, n;     /* indecies and size/return code */
  double dt, t0;             /* timestep and offset */
  double eps, epsmax = 0.0;  /* alignment errors */

  /* Parse options. */
  while ( ( opt = getopt_long( argc, argv, short_opts, long_opts, &lopt ) )
          != -1 ) {
    switch ( opt ) {
    case 0:
      if ( !strcmp( long_opts[lopt].name, "manpage" ) )
	markdown_to_manpage( description, NULL );
      else if ( !strcmp( long_opts[lopt].name, "markdown" ) )
	fputs( description, stdout );
      CLEANEXIT( 0 );
    case 'h':
      fprintf( stdout, usage, argv[0] );
      CLEANEXIT( 0 );
    case 'H':
      markdown_to_man_out( description );
      CLEANEXIT( 0 );
    case 'V':
      fputs( version, stdout );
      CLEANEXIT( 0 );
    case 'v':
      lofasm_verbosity = atoi( optarg );
      break;
    case 'p':
      if ( !strcmp( optarg, "-" ) )
	dopad = 0;
      else {
	char *tail; /* any unparseable part of optarg */
	dopad = 1;
	padd = strtod( optarg, &tail );
	if ( !tail[0] )
	  padf = strtof( optarg, &tail );
	if ( tail[0] ) {
	  lf_error( "could not parse pad value %s", optarg );
	  CLEANEXIT( 1 );
	}
      }
      break;
    case 'm': case 'l':
      if ( ( med = atof( optarg ) ) <= 0 ) {
	lf_error( "median steps must be positive" );
	CLEANEXIT( 1 );
      }
      lin = ( opt == 'l' );
      break;
    case 'a':
      if ( ( awarn = atof( optarg ) ) < 0.0 || awarn >= 1.0 ) {
	lf_error( "alignment warning threshold should be between 0 and 1" );
	CLEANEXIT( 1 );
      }
      break;
    case 'A':
      if ( ( aerr = atof( optarg ) ) < 0.0 || aerr >= 1.0 ) {
	lf_error( "alignment error threshold should be between 0 and 1\n" );
	CLEANEXIT( 1 );
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
      CLEANEXIT( 1 );
    case ':':
      if ( optopt )
	lf_error( "option -%c requires an argument\n\t"
		  "Try %s --help for more information",
		  optopt, argv[0] );
      else
	lf_error( "option %s requires an argument\n\t"
		  "Try %s --help for more information",
		  argv[optind-1], argv[0] );
      CLEANEXIT( 1 );
    default:
      lf_error( "internal error parsing option code %c\n\t"
		"Try %s --help for more information",
		opt, argv[0] );
      CLEANEXIT( 1 );
    }
  }

  /* Deal with trivial cases (no or one input) first. */
  nin = argc - optind - 1;
  if ( nin < 0 ) {
    lf_error( "no output file specified\n\t"
	      "Try %s --help for more information", argv[0] );
    CLEANEXIT( 1 );
  }
  if ( nin <= 1 ) {
    /* Open file. */
    if ( nin == 0 || !strcmp( argv[optind], "-" ) ) {
      if ( !( fpin = lfdopen( 0, "rb" ) ) ) {
	lf_error( "could not read from stdin" );
	CLEANEXIT( 2 );
      }
      infile = "stdin";
      istd = 1;
    } else if ( !( fpin = lfopen( ( infile = argv[optind] ), "rb" ) ) ) {
      lf_error( "could not open input %s", infile );
      CLEANEXIT( 2 );
    }
    /* Read header and allocate row buffer. */
    if ( lfbxRead( fpin, &header, NULL ) ) {
      lf_error( "could not parse header from %s", infile );
      CLEANEXIT( 2 );
    }
    nrow = 1;
    for ( i = 1; i < LFB_DMAX && header.dims[i]; i++ )
      nrow *= header.dims[i];
    nrow /= 8;
    if ( !( row = (unsigned char *)malloc( nrow*sizeof(unsigned char) ) ) ) {
      lf_error( "memory error" );
      CLEANEXIT( 4 );
    }
    /* Open output and copy file. */
    if ( !strcmp( argv[argc-1], "-" ) ) {
      if ( isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) ) ) {
	lf_error( "could not write to stdout" );
	CLEANEXIT( 2 );
      }
      outfile = "stdout";
    } if ( !( fpout = lfopen( ( outfile = argv[argc-1] ), "wb" ) ) ) {
      lf_error( "could not open output %s", outfile );
      CLEANEXIT( 2 );
    }
    if ( lfbxWrite( fpout, &header, NULL ) ) {
      lf_error( "error writing to %s", outfile );
      CLEANEXIT( 2 );
    }
    for ( i = 0; i < header.dims[0] && !feof( fpin ); i++ ) {
      if ( ( n = fread( row, 1, nrow, fpin ) ) < nrow )
	lf_warning( "read %d rows from %s, expected %d", i, infile,
		    header.dims[0] );
      if ( n > 0 && fwrite( row, 1, n, fpout ) < n ) {
	lf_error( "error writing to %s", outfile );
	CLEANEXIT( 2 );
      }
    }
    CLEANEXIT( 0 );
  }

  /* Set up array of file headers. */
  if ( !( headers = (lfb_hdr *)malloc( nin*sizeof(lfb_hdr) ) ) ) {
    lf_error( "memory error" );
    CLEANEXIT( 4 );
  }
  for ( i = 0; i < nin; i++ ) {
    if ( !strcmp( argv[optind+i], "-" ) ) {
      if ( !( fpin = lfdopen( 0, "rb" ) ) ) {
	lf_error( "could not read from stdin" );
	CLEANEXIT( 2 );
      }
      istd = 1;
      infile = "stdin";
    } else {
      if ( !( fpin = lfopen( ( infile = argv[optind+i] ), "rb" ) ) ) {
	lf_error( "could not open input %s", infile );
	CLEANEXIT( 2 );
      }
      istd = 0;
    }
    if ( lfbxRead( fpin, headers + i, NULL ) ) {
      lf_error( "could not parse header from %s", infile );
      CLEANEXIT( 2 );
    }
    if ( !istd )
      fclose( fpin );
    /* If using median padding, discard files less than 2 timesteps;
       it's just too much work trying to deal with them. */
    if ( med && headers[i].dims[0] < 2 ) {
      lf_warning( "discarding single-timestep file %s", infile );
      i--;
      nin--;
    }
  }

  /* Check for identical frequency sampling and data types. */
  for ( i = 1; i < nin; i++ ) {
    lfb_hdr *h1 = headers, *h2 = headers + i;
    if ( h1->frequency_offset_DC + h1->dim2_start !=
	 h2->frequency_offset_DC + h2->dim2_start ||
	 h1->dim2_span != h2->dim2_span ||
	 h1->data_offset != h2->data_offset ||
	 h1->data_scale != h2->data_scale ||
	 strcmp( h1->data_type, h2->data_type ) ) {
      lf_error( "incompatible frequencies/data types" );
      CLEANEXIT( 3 );
    }
    for ( n = 1; n < LFB_DMAX; n++ )
      if ( h1->dims[n] != h2->dims[n] ) {
	lf_error( "incompatible frequencies/data types" );
	CLEANEXIT( 3 );
      }
  }

  /* Sort array of file headers. */
  if ( !( idx = (int *)malloc( nin*sizeof(int) ) ) ) {
    lf_error( "memory error" );
    CLEANEXIT( 4 );
  }
  for ( i = 0; i < nin; i++ )
    idx[i] = i;
  qsort( idx, nin, sizeof(int), ascendingTimes );

  /* Refer all times to common epoch, and get timestep. */
  for ( i = 1; i < nin; i++ ) {
    lfb_hdr *h1 = headers + idx[0], *h2 = headers + idx[i];
    double tdiff = h2->time_offset_J2000 - h1->time_offset_J2000;
    h2->time_offset_J2000 = h1->time_offset_J2000;
    h2->dim1_start += tdiff;
  }
  t0 = headers[idx[0]].dim1_start;
  dt = headers[idx[0]].dim1_span/headers[idx[0]].dims[0];
  klast = headers[idx[0]].dims[0];

  /* Check for overlaps, gaps, and alignment errors. */
  for ( i = 1; i < nin; i++ ) {
    lfb_hdr *h = headers + idx[i];
    double kf = ( h->dim1_start - t0 )/dt;
    k = (int)round( kf );
    if ( k < klast ) {
      lf_error( "overlap between files %d and %d from argument list\n\t"
		"(ordered %d and %d after sorting)",
		idx[i-1], idx[i], i-1, i );
      CLEANEXIT( 3 );
    }
    if ( k > klast ) {
      gaps += k - klast;
      if ( !dopad ) {
	lf_error( "gap between files %d and %d from argument list\n\t"
		  "(ordered %d and %d after sorting)",
		  idx[i-1], idx[i], i-1, i );
	CLEANEXIT( 3 );
      }
    }
    if ( ( eps = fabs( k - kf ) ) > epsmax )
      epsmax = eps;
    klast = k + h->dims[0];
    kf += h->dim1_span/dt;
    k = (int)round( kf );
    if ( k != klast ) {
      lf_error( "misalignment in file %d from argument list\n\t"
		"(ordered %d after sorting)", idx[i], i );
      CLEANEXIT( 3 );
    }
    if ( ( eps = fabs( k - kf ) ) > epsmax )
      epsmax = eps;
  }
  if ( epsmax > aerr ) {
    lf_error( "max misalignment %e > %e", epsmax, aerr );
    CLEANEXIT( 3 );
  } else if ( epsmax > awarn )
    lf_warning( "max misalignment %e", epsmax );
  else if ( epsmax > 0.0 )
    lf_info( "max misalignment %e", epsmax );
  if ( gaps > 0 )
    lf_info( "total gaps %d/%d (%f%%)", gaps, klast,
	     (float)( 100.0*gaps )/klast );

  /* Write output header. */
  memcpy( &header, headers + idx[0], sizeof(lfb_hdr) );
  header.dims[0] = klast;
  header.dim1_span = klast*dt;
  if ( !strcmp( argv[argc-1], "-" ) ) {
    if ( !( fpout = lfdopen( 1, "wb" ) ) ) {
      lf_error( "could not write to stdout" );
      CLEANEXIT( 2 );
    }
    outfile = "stdout";
  } else if ( !( fpout = lfopen( ( outfile = argv[argc-1] ), "wb" ) ) ) {
    lf_error( "could not open output %s", outfile );
    CLEANEXIT( 2 );
  }
  if ( lfbxWrite( fpout, &header, NULL ) ) {
    lf_error( "error writing to %s", outfile );
    CLEANEXIT( 2 );
  }

  /* Allocate data row and gap-filling row. */
  nrow = 1;
  for ( i = 1; i < LFB_DMAX && header.dims[i]; i++ )
    nrow *= header.dims[i];
  nrow /= 8;
  if ( !( row = (unsigned char *)malloc( nrow*sizeof(unsigned char) ) ) ) {
    lf_error( "memory error" );
    CLEANEXIT( 4 );
  }
  if ( gaps ) {
    if ( med )
      gap = (unsigned char *)malloc( 2*med*nrow*sizeof(unsigned char) );
    else
      gap = (unsigned char *)malloc( nrow*sizeof(unsigned char) );
    if ( !gap ) {
      lf_error( "memory error" );
      CLEANEXIT( 4 );
    }
    if ( med ) {
      if ( strcmp( headers[0].data_type, "real64" ) ) {
	lf_warning( "median padding only implemented for real64 data" );
	memset( gap, 0, nrow*sizeof(unsigned char) );
	med = 0;
      }
      if ( !( mddx = (int *)malloc( 2*med*sizeof(int) ) ) ||
	   !( data = (double *)malloc( nrow ) ) ||
	   ( lin && ( !( data1 = (double *)malloc( nrow ) ) ||
		      !( data2 = (double *)malloc( nrow ) ) ) ) ) {
	lf_error( "memory error" );
	CLEANEXIT( 4 );
      }
      stride = nrow/sizeof(double);
    } else if ( padd == 0.0 )
      memset( gap, 0, nrow*sizeof(unsigned char) );
    else if ( !strcmp( headers[0].data_type, "real32" ) )
      for ( i = 0; i < nrow/4; i++ )
	memcpy( gap + 4*i, &padf, 4*sizeof(unsigned char) );
    else if ( !strcmp( headers[0].data_type, "real64" ) )
      for ( i = 0; i < nrow/8; i++ )
	memcpy( gap + 8*i, &padd, 8*sizeof(unsigned char) );
    else {
      lf_warning( "padding for %s data must be 0", headers[0].data_type );
      memset( gap, 0, nrow*sizeof(unsigned char) );
    }
  }

  /* Write data. */
  for ( i = k = 0; i < nin; i++ ) {
    lfb_hdr *h = headers + idx[i];         /* this file's header */
    double kf = ( h->dim1_start - t0 )/dt; /* initial index as float */
    int kstart = (int)round( kf );         /* initial index */
    int kend = kstart + h->dims[0];        /* final index */
    int kstop;                             /* last index before median chunk */

    /* Reopen input file and skip to data (if not stdin). */
    if ( !strcmp( argv[optind+idx[i]], "-" ) ) {
      if ( !( fpin = lfdopen( 0, "rb" ) ) ) {
	lf_error( "could not read from stdin" );
	CLEANEXIT( 2 );
      }
      istd = 1;
      infile = "stdin";
    } else {
      if ( !( fpin = lfopen( ( infile = argv[optind+idx[i]] ), "rb" ) ) ) {
	lf_error( "could not open input %s", infile );
	CLEANEXIT( 2 );
      }
      istd = 0;
      bxSkipHeader( fpin );
    }

    /* Find size of median chunk for this file, and index preceding
       final median chunk. */
    med2 = ( med < h->dims[0]/2 ? med : h->dims[0]/2 );
    kstop = kend - med2;

    /* Fill any preceding gap. */
    if ( k < kstart ) {

      /* Pad with a specified value. */
      if ( !med ) {
	for ( ; k < kstart; k++ )
	  if ( fwrite( gap, 1, nrow, fpout ) < nrow ) {
	    lf_error( "error writing to %s", outfile );
	    CLEANEXIT( 2 );
	  }
      }

      /* For median filters, start by placing the first chunk of the
	 new file in the gap block. */
      else {
	if ( ( n = fread( gap + med1*nrow, 1, med2*nrow, fpin ) )
	     < med2*nrow ) {
	  lf_info( "read %d bytes from %s, expected %d",
		   n, infile, ( kend - kstart )*nrow );
	  memset( gap + med1*nrow + n, 0, med2*nrow - n );
	}

	/* Linear interpolation between medians.  Last file's median
	   should have been stored in data1. */
	if ( lin ) {
	  double k0 = k, k1 = kstart - k; /* range of interpolation */
	  for ( j = 0; j < stride; j++ ) {
	    for ( n = 0; n < med2; n++ )
	      mddx[n] = n;
	    mddata = (double *)( gap + med1*nrow + 8*j );
	    qsort( mddx, med2, sizeof(int), ascendingDoubles );
	    data2[j] = mddata[ mddx[med1/2]*stride ];
	  }
	  for ( ; k < kstart; k++ ) {
	    kf = ( k - k0 + 0.5 )/k1;
	    for ( j = 0; j < stride; j++ )
	      data[j] = kf*data2[j] + ( 1.0 - kf )*data1[j];
	    if ( fwrite( data, 8, stride, fpout ) < stride ) {
	      lf_error( "error writing to %s", outfile );
	      CLEANEXIT( 2 );
	    }
	  }
	}

	/* Straight median of data on either side of gap. */
	else {
	  for ( j = 0; j < stride; j++ ) {
	    for ( n = 0; n < med1 + med2; n++ )
	      mddx[n] = n;
	    mddata = (double *)( gap + 8*j );
	    qsort( mddx, med1 + med2, sizeof(int), ascendingDoubles );
	    data[j] = mddata[ mddx[ ( med1 + med2 )/2 ]*stride ];
	  }
	  for ( ; k < kstart; k++ )
	    if ( fwrite( data, 8, stride, fpout ) < stride ) {
	      lf_error( "error writing to %s", outfile );
	      CLEANEXIT( 2 );
	    }
	}

	/* For median filters, finish by taking the leading chunk we
	   read into the gap array, and writing it to output. */
	if ( fwrite( gap + med1*nrow, 1, med2*nrow, fpout )
	     < med2*nrow ) {
	  lf_error( "error writing to %s", outfile );
	  CLEANEXIT( 2 );
	}
	k += med2;
      }
    }

    /* Copy (rest of) input file. */
    for ( ; k < kstop && !feof( fpin ); k++ ) {
      if ( ( n = fread( row, 1, nrow, fpin ) ) < nrow ) {
	lf_info( "read %d bytes from %s, expected %d",
		 ( k - kstart )*nrow + n, infile, ( kend - kstart )*nrow );
	memset( row + n, 0, nrow - n );
      }
      if ( fwrite( row, 1, nrow, fpout ) < nrow ) {
	lf_error( "error writing to %s", outfile );
	CLEANEXIT( 2 );
      }
    }
    if ( k < kstop ) {
      memset( row, 0, nrow - n );
      for ( ; k < kstop; k++ )
	if ( fwrite( row, 1, nrow, fpout ) < nrow ) {
	  lf_error( "error writing to %s", outfile );
	  CLEANEXIT( 2 );
	}
    }

    /* If using median filter, load last chunk of file into gap array.
       Write it out as well. */
    if ( med ) {
      med1 = med2;
      if ( ( n = fread( gap, 1, med1*nrow, fpin ) ) < med1*nrow ) {
	lf_info( "read %d bytes from %s, expected %d",
		 ( k - kstart )*nrow + n, infile, ( kend - kstart )*nrow );
	memset( gap + n, 0, med1*nrow - n );
      }
      if ( fwrite( gap, 1, med1*nrow, fpout ) < med1*nrow ) {
	lf_error( "error writing to %s", outfile );
	CLEANEXIT( 2 );
      }
      k += med1;

      /* For linearly-interpolated medians, store median of this chunk
	 in data1. */
      if ( lin )
	for ( j = 0; j < stride; j++ ) {
	  for ( n = 0; n < med1; n++ )
	    mddx[n] = n;
	  mddata = (double *)( gap + 8*j );
	  qsort( mddx, med1, sizeof(int), ascendingDoubles );
	  data1[j] = mddata[ mddx[med1/2]*stride ];
	}
    }
    if ( !istd ) {
      fclose( fpin );
      fpin = NULL;
    }
  }

  /* Finished. */
  CLEANEXIT( 0 );
}
