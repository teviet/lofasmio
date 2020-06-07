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
Usage: %s [OPTION]... INFILE  OUTFILE\n\
Converts bbx to SIGPROC filterbank format\n\
\n\
  -h, --help              print this usage information\n\
  -H, --man               display the program's man page\n\
      --manpage           print the program's man page (groff)\n\
      --markdown          print the program's man page (markdown)\n\
  -V, --version           print program version\n\
  -v, --verbosity=LEVEL   set status message reporting level\n\
\n";

static const char *description = "\
# lfstats(1)\n\
\n\
## NAME\n\
\n\
`lf2fil(1)` - convert lofasm-filterbank(5) file to SIGPROC filterbank file\n\
\n\
## SYNOPSIS\n\
\n\
`lf2fil` [_OPTION_]... _INFILE_  _OUTFILE_\n\
\n\
## DESCRIPTION\n\
\n\
This program converts lofasm-filterbank file to SIGPROC-filterbank file.\n\
If _INFILE_ is absent or a single `-` character, then\n\
data is read from standard input. \n\
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
#include "sigproc.h"

static const char short_opts[] = "hHVv:p:m:";
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
		char opt;                 /* short option character */
		int lopt;                 /* long option index */
		char *infile;             /* input file name */
		char *outfile;            /* output file name */
		FILE *fpin = NULL;        /* input file pointer */
		FILE *fpout = NULL;       /* output file pointer */
		lfb_hdr ihead = {};       /* input header */
		sfb_hdr ohead = {};       /* output header */
		int64_t i, j;             /* indecies in dims 1 and 2 */
		int64_t nrow;             /* number of elements in one row*/
		int64_t n;                /* number of data read */
		int64_t iarg;             /* number of file arguments */
		double *data;             /* one row */
		float *rata;              /* reversed one row */

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

		/* Trivial checks. */
		iarg = argc - optind;
		if ( iarg <= 1 ) {
				lf_error( "no output file specified\n\t"
								"Try %s --help for more information", argv[0] );
				return 1;
		}
		if ( iarg > 2 ) {
				lf_error( "too many arguments specified\n\t"
								"Try %s --help for more information", argv[0] );
				return 1;
		}

		/* Read input header. */
		if ( !strcmp( argv[argc-2], "-" ) ) {
				if ( !( fpin = lfdopen( 0, "rb" ) ) ) {
						lf_error( "could not read from stdin" );
						fclose ( fpin );
						return 2;
				}
				infile = "stdin";
		} else {
				if ( !( fpin = lfopen( ( infile = argv[argc-2] ), "rb" ) ) ) {
						lf_error( "could not open input %s", infile );
						fclose ( fpin );
						return 2;
				}
		}
		if ( lfbxRead( fpin, &ihead, NULL ) ) {
				lf_error( "could not parse header from %s", infile );
				fclose( fpin );
				lfbxFree( &ihead );
				return 2;
		}

		/* Check data type. */
		if ( ihead.dims[3] != 64 ) {
				lf_error( "requires real64 data" );
				fclose( fpin );
				lfbxFree( &ihead );
				return 3;
		}
		if ( !ihead.data_type || strcmp( ihead.data_type, "real64" ) )
				lf_warning( "treating as real64 data" );

		/* Prepare output sigproc filterbank and write header */
		lf2sg_hdr (&ihead, &ohead);
		if ( !( fpout = fopen( ( outfile =  argv[argc-1] ) , "wb" ) ) ) {
				lf_error( "could not open output file %s", outfile );
				fclose( fpin );
				lfbxFree( &ihead );
				return 2;
		}
		write_header (&ohead, fpout);

		/* Allocate data row and reverse data row */
		nrow = 1;
		for ( i = 1; i < LFB_DMAX && ihead.dims[i]; i++ )
				nrow *= ihead.dims[i];
		nrow /= 8; /* number of bytes */
		nrow /= sizeof(double); /* number of doubles */
		if ( !( data = (double *)malloc( nrow*sizeof(double) ) ) ) {
				lf_error( "memory error" );
				fclose( fpin );
				fclose( fpout );
				lfbxFree( &ihead );
				return 4;
		}
		if ( !( rata = (float *)malloc( nrow*sizeof(float) ) ) ) {
				lf_error( "memory error" );
				fclose( fpin );
				fclose( fpout );
				lfbxFree( &ihead );
				return 4;
		}

		/* Read data, reverse and write. */
		for ( i = 0; i < ihead.dims[0]; i++ ) {
				// read step
				if ( feof( fpin ) )
						memset( data, 0, nrow*sizeof(double) );
				else if ( ( n = fread( data, sizeof(double), nrow, fpin ) ) < nrow ) {
						lf_warning( "read %lld doubles, expected %lld",
										(long long)( i*nrow + n ), (long long)( ihead.dims[0]*nrow ) );
						memset( data + n, 0, ( nrow - n )*sizeof(double) );
				}
				// reverse step
				for ( j = 0; j < nrow; j++ ) {
						rata[nrow-j-1] = data[j];
				}
				// write step
				fwrite (rata, sizeof (float), nrow, fpout);
		}

		/* Close files */
		fclose( fpin );
		fclose( fpout );
		/* Free data */
		free ( data );
		free ( rata );

		return 0;
}
