static const char *version = "\
lfcoadd version " VERSION "\n\
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
Coadd distinct baseline LoFASM files starting at same time.\n\
\n\
  -h, --help             print this usage information\n\
  -H, --man              display the program's man page\n\
      --manpage          print the program's man page (groff)\n\
      --markdown         print the program's man page (markdown)\n\
  -V, --version          print program version\n\
  -v, --verbosity=LEVEL  set status message reporting level\n\
  -i, --ignore-align     ignore time/freq/data misalignments\n\
\n";

static const char *description = "\
# lfcoadd(1)\n\
\n\
## NAME\n\
\n\
`lfcoadd(1)` - coadd lofasm-filterbank(5) files\n\
\n\
## SYNOPSIS\n\
\n\
`lfcoadd` [_OPTION_]... [_INFILE_]... _OUTFILE_\n\
\n\
## DESCRIPTION\n\
\n\
This program coadds multiple (upto 4) distinct baseline lofasm-filterbank(5) files\n\
_INFILE_... that span the same frequency and time ranges but belong\n\
to different baselines.  The start times should be the same for all files,\n\
however the span times can be different, in which case the minimum span is chosen.\n\
The input files need not be given in order. The input files should have 64 bit depth.\n\
\n\
At least one non-option argument _OUTFILE_ must be given, indicating\n\
the name of the output file.  If _OUTFILE_ is a single `-` character,\n\
the result will be written to standard output.  If no other non-option\n\
argument _INFILE_ is given, a single input file is read from standard\n\
input, resulting in a \"trivial\" coaddition.  Also, any one of\n\
_INFILE_ may be a single `-` character, indicating a file to be read\n\
from standard input.  Output is written one row at a time, so this\n\
program can be used to feed a data analysis pipleline.\n\
\n\
The frequency/data dimensions should match, but the time dimension may or \n\
may not match. In case of a time dimension mismatch, the least dimension\n\
is used. The hardlimit of four is set since lofasm offers four auto-correlations.\n\
The motive behind coaddition is to improve S/N and reduce noise artefacts by adding\n\
overlapping power levels.\n\
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
    Sets the verbosity level for error reporting.  _LEVEL_ may be `1`\n\
    (quiet, no messages), `1` (default, error messages only), `2`\n\
    (verbose, errors and warnings), or `3` (very verbose, errors,\n\
    warnings, and extra information).\n\
`-i, --ignore-timing`:\n\
				Ignores time/frequency/data misalignments\n\
				and only checks for dimensions match for coaddition\n\
\n\
## EXIT STATUS\n\
\n\
The proram exits with status 0 normally, 1 if there is an error\n\
parsing its arguments, 2 on read/write errors, 3 if the files have\n\
misaligned frequencies, misaligned timesteps, \n\
or incompatible metadata, and 4 on memory allocation\n\
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

static const char short_opts[] = "hHVv:i";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "verbosity", 1, 0, 'v' },
  { "ignore-timing", 0, 0, 'i' },
  { 0, 0, 0, 0} };

#define LEN 1024 /* character buffer size */
#define MAXC 4   /* maximum number of coadditions */

/* Macro to free all memory and close all files, prior to exiting.
   This should only within main(), which should initialize all
   unallocated pointers to NULL to avoid potentially trying to free
   random pointer values. */
#define CLEANEXIT( code ) \
do { \
  lfbxFree( &header ); \
  if ( row ) free( row ); \
  if ( rrow ) free( rrow ); \
  if ( fpins[0] ) fclose( fpins[0] ); \
  if ( fpins[1] ) fclose( fpins[1] ); \
  if ( fpins[2] ) fclose( fpins[2] ); \
  if ( fpins[3] ) fclose( fpins[3] ); \
  if ( fpout ) fclose( fpout ); \
  exit( code ); \
} while ( 0 )


int
main( int argc, char **argv )
{
		char opt;                   /* short option character */
		int lopt;                   /* long option index */
  int ignore = 0;             /* whether to ignore mismatches */
		FILE *fpin    = NULL;       /* temp to hold */
		FILE *fpins[MAXC] = {NULL}; /* input files */
		FILE *fpout   = NULL;       /* output file */
		int istd = 0;               /* whether input is stdin */
		char *infile, *outfile;     /* input/output filenames */
		lfb_hdr header = {};        /* single-input or output header */
		lfb_hdr headers[MAXC] = {}; /* array to hold headers */
		double *row = NULL;         /* single timestep of data  float*/
		double *rrow = NULL;        /* single timestep of coadded data */
		int64_t nrow;               /* number of bytes per row */
		int nin;                    /* number of input files */
		float iin;                  /* reciprocal of number of input files */
		int64_t dim1s[MAXC];        /* sizes of inputs */
		int64_t odim1;              /* output size */ 
		int64_t iodim1;             /* index of headers w/ output size */ 
		int64_t i, j, k, n;         /* indecies and size/return code */

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
						case 'i':
								ignore = 1;
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

		/* Trivial checks before moving ahead. */
		nin = argc - optind - 1;
		if ( nin < 0 ) {
				lf_error( "no output file specified\n\t"
								"Try %s --help for more information", argv[0] );
				CLEANEXIT( 1 );
		}
		if ( nin <= 1 ) {
				lf_error( "only one input file specified\n\t"
								"Try %s --help for more information", argv[0] );
				CLEANEXIT( 1 );
		}
		if ( nin > MAXC ) {
				lf_error( "at max only 4 auto col lofasm files can be co-added\n\t"
								"Try %s --help for more information", argv[0] );
				CLEANEXIT( 1 );
		}
		iin = 1.0 / nin ; 

		/* Set up array of file headers. */
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
				// read dim1s
				dim1s[i] = (headers + i)->dims[0];
		}

		/* Find minimum dim1 */
		odim1  = dim1s[0];
		iodim1 = 0;
		for ( i = 1; i < nin; i++ ) {
				if ( odim1 > dim1s[i] ) {
						odim1 = dim1s[i];
						iodim1 = i;
				}
		}

		/* Check for identical frequency and time samplings and data types. */
		for ( i = 1; i < nin; i++ ) {
				lfb_hdr *h1 = headers, *h2 = headers + i;
				if ( h1->frequency_offset_DC + h1->dim2_start !=
								h2->frequency_offset_DC + h2->dim2_start ||
				    h1->time_offset_J2000 + h1->dim1_start !=
								h2->time_offset_J2000 + h2->dim1_start ||
								h1->dim1_span != h2->dim1_span ||
								h1->dim2_span != h2->dim2_span ||
								h1->data_offset != h2->data_offset ||
								h1->data_scale != h2->data_scale ||
								strcmp( h1->data_type, h2->data_type ) ) {
						if ( ignore )
								lf_warning ( "ignoring incompatible time/frequencies/data types" );
						else {
								lf_error( "incompatible time/frequencies/data types" );
								CLEANEXIT( 3 );
						}
				}
				for ( n = 1; n < LFB_DMAX; n++ )
						if ( h1->dims[n] != h2->dims[n] ) {
								lf_error( "incompatible frequencies/data dimensions" );
								CLEANEXIT( 3 );
						}
				if ( h1->dims[LFB_DMAX-1] != 64 &&
								h2->dims[LFB_DMAX-1] != 64 ) {
								lf_error( "only real64 supported as of now." );
								CLEANEXIT( 3 );
				} 
		}

		/* Write output header. */
		memcpy( &header, headers + iodim1, sizeof(lfb_hdr) );
		strcpy (header.channel, "XX");
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

		/* Allocate data row and result row */
		nrow = 1;
		for ( i = 1; i < LFB_DMAX && header.dims[i]; i++ )
				nrow *= header.dims[i];
		nrow /= 8; /* number of bytes */
		nrow /= sizeof(double); /* number of doubles */
		if ( !( row = (double*)malloc( nrow*sizeof(double) ) ) ) {
				lf_error( "memory error" );
				CLEANEXIT( 4 );
		}
		if ( !( rrow = (double*)malloc( nrow*sizeof(double) ) ) ) {
				lf_error( "memory error" );
				CLEANEXIT( 4 );
		}

		/* Open input argument files */
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
						bxSkipHeader( fpin );
				}
				fpins [i] = istd ? NULL : fpin;
		}
		fpin = NULL;

		/* Coadd data (main working loop). */
		for ( k = 0; k < header.dims[0]; k++ ) {
				// initialize step
				for ( i = 0; i < nrow; i++ )
						rrow[i] = 0.0f;
				// work step
				for ( i = 0; i < nin; i++ ) {
						// read step
						if ( ( n = fread( row, sizeof(double), nrow, fpins[i] ) ) < nrow ) {
								lf_info( "read %lld bytes from %s, expected %lld",
												(long long)( n ), infile,
												(long long)( headers[i].dims[0]*nrow ) );
								for ( j = n; j < nrow; j++ )
										row[j] = 0.0f;
						}
						// sum step
						for ( j = 0; j < nrow; j++ ) {
								rrow[j] += (iin * row[j]);
						}
				}
				// write step
				if ( fwrite( rrow, sizeof(double), nrow, fpout ) < nrow ) {
						lf_error( "error writing to %s", outfile );
						CLEANEXIT( 2 );
				}
		}

		/* Finished. */
		CLEANEXIT( 0 );
}
