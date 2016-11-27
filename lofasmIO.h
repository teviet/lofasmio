// 2>&-### SELF-EXTRACTING DOCUMENTATION ###############################
// 2>&-#                                                               #
// 2>&-# Run "bash <thisfile> > <docfile.md>" to extract documentation #
// 2>&-#                                                               #
// 2>&-#################################################################
// 2>&-; awk '/^<\/MARKDOWN>/{f=0};f;/^<MARKDOWN>/{f=1}' $0; exit 0

/***********************************************************************
lofasmIO.h
Copyright (c) 2016 Teviet Creighton.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#ifndef _LOFASMIO_H
#define _LOFASMIO_H
#ifdef  __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#include <stdio.h>

/*
<MARKDOWN>
<INCLUDE> README - gives an introduction to the lofasmio package

### Programs

The `lofasmio` package includes a number of utility programs for
preprocessing LoFASM data.  When properly compiled, the lofasmio(7)
manpage should have in its **SYNOPSIS** section a list of all such
programs.  Standard options to these programs are:

    -h, --help             print usage information
    -H, --man              display the program's man page
        --manpage          print the program's man page (groff)
        --markdown         print the program's man page (markdown)
    -V, --version          print program version
    -v, --verbosity=LEVEL  set status message reporting level

Many of the programs are conceptually data filters, with a usage of
the form:

> _PROGRAM_ [_OPTION_]... [_INFILE_ [_OUTFILE_]]

That is, if run without explicit input and output filenames, they will
read from standard input and write to standard output.  In most cases,
supplying a `-` character as a file argument also tells _PROGRAM_ to
use standard input or output for that file.  This facilitates their
use in pipelines, without cluttering the filesystem with temporary
files: e.g.

> _PROG1_ `<` _INFILE_ `|` _PROG2_ `|` ... `|` _PROGn_ `>` _OUTFILE_

### File Fomats

The standard LoFASM file format is described in lofasm_filterbank(5)
documentation.  If compiled with zlib(3), `lofasmio` programs will
natively read and write gzipped or uncompressed filterbank files.  (If
an output filename is not specified, e.g. when writing to stdandard
output in a pipeline, the programs will gzip their output by default).

Within the programs and functions, the routines lfopen(3) and
lfdopen(3) handle opening plain or compressed named files or numerical
file descriptors.  These functions insert appropriate (de)compression
algrithms into all reading and writing functions, so that programs can
use standard C functions such as fscanf(3) and fprintf(3) without
worrying about whether the underlying file stream is compressed or
not.

If compiled without zlib(3), the programs will read and write only
uncompressed filterbank files, which you can compress and uncompress
with separate calls to gzip(1) and gunzip(1).  In this case, lfopen(3)
and lfdopen(3) revert to the standard C functions fopen(3) and
fdopen(3).

Within the `lofasmio` programs and functions, filterbank metadata is
stored in a dedicated header structure `lfb_hdr`, defined as below:

    typedef struct {
        char *hdr_type;
        float hdr_version;
        char *station;
        char *channel;
        char *start_time;
        double time_offset_J2000;
        double frequency_offset_DC;
        char *dim1_label;
        double dim1_start;
        double dim1_span;
        char *dim2_label;
        double dim2_start;
        double dim2_span;
        char *data_label;
        double data_offset;
        double data_scale;
        char *data_type;
        int dims[LFB_DMAX];
    } lfb_hdr;

where `LFB_MAX`=4 is the number of dimensions in a LoFASM filterbank.
</MARKDOWN> */
#define LFB_DMAX 4
typedef struct {
  char *hdr_type;
  float hdr_version;
  char *station;
  char *channel;
  char *start_time;
  double time_offset_J2000;
  double frequency_offset_DC;
  char *dim1_label;
  double dim1_start;
  double dim1_span;
  char *dim2_label;
  double dim2_start;
  double dim2_span;
  char *data_label;
  double data_offset;
  double data_scale;
  char *data_type;
  int dims[LFB_DMAX];
} lfb_hdr;
/*
<MARKDOWN>

See lofasm-filterbank(5) for more information on these metadata
fields.  The function lfbxRead(3) will populate these fields when
reading a filterbank, and lfbxWrite(3) will use them when writing a
filterbank.

## EXAMPLE

The following program reads a complex lofasm-filterbank(5) file from
its first argument or `stdin`, and writes the modulus of the data to
its second argument or `stdout`.  Compile with linking flags
`-llofasmio`, `-lm`, and `-lz` (the last can be dropped if
`liblofasmio` was compiled without `zlib`).

    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <math.h>
    #include "lofasmIO.h"
    
    int
    main( int argc, char *argv[] )
    {
        int i = 1, j, k;
        char *infile = NULL, *outfile = NULL, *label;
        const char *prefix = "magnitude of ";
        lfb_hdr head = {};
        double *dat, *d;
        FILE *fpin, *fpout;
    
        // Get file names or NULL for stdin/stdout.
        if ( i >= argc || !strcmp( ( infile = argv[i++] ), "-" ) )
            infile = NULL;
        if ( i >= argc || !strcmp( ( outfile = argv[i++] ), "-" ) )
            outfile = NULL;
    
        // Read input file header.
        i = 0;
        if ( !infile )
            i = i || !( fpin = lfdopen( 0, "rb" ) );
        else
            i = i || !( fpin = lfopen( infile, "rb" ) );
        i = i || lfbxRead( fpin, &head, NULL );
        if ( i ) {
            lf_error( "could not read input header" );
            exit( EXIT_FAILURE );
        }
    
        // Check that data exist and are complex double.
        if ( head.dims[0] <= 0 || head.dims[1] <= 0 ||
             head.dims[2] != 2 || head.dims[3] != 64 ) {
            lf_error( "file must contain complex double data" );
            exit( EXIT_FAILURE );
        }
        k = head.dims[1];
    
        // Allocate new data label and data row.
        i = strlen( prefix ) + strlen( head.data_label ) + 1;
        if ( !( label = (char *)malloc( i ) ) ||
             !( dat = (double *)malloc( 2*k*8 ) ) ) {
            lf_error( "memory error" );
            exit( EXIT_FAILURE );
        }
        sprintf( label, "%s%s", prefix, head.data_label );
        free( head.data_label );
        head.data_label = label;
        head.dims[2] = 1;
    
        // Write output header.
        i = 0;
        if ( !outfile )
            i = i || isatty( 1 ) || !( fpout = lfdopen( 1, "wbZ" ) );
        else
            i = i || !( fpout = lfopen( outfile, "wb" ) );
        i = i || lfbxWrite( fpout, &head, NULL );
        if ( i ) {
            lf_error( "could not write output header" );
            exit( EXIT_FAILURE );
        }
    
        // Read, transform, and write data row-by-row.
        for ( i = 0; i < head.dims[0]; i++ ) {
            if ( fread( dat, 8, 2*k, fpin ) < 2*k )
                break;
            for ( j = 0, d = dat; j < k; j++, d += 2 )
                dat[j] = sqrt( d[0]*d[0] + d[1]*d[1] );
            if ( fwrite( dat, 8, k, fpout ) < k )
                break;
        }
        if ( i < head.dims[0] ) {
            lf_error( "premature end of data" );
            exit( EXIT_FAILURE );
        }
        fclose( fpin );
        fclose( fpout );
        lfbxFree( &head );
        free( dat );
        exit( EXIT_SUCCESS );
    }

## See Also

lfbxRead(3),
lfbxWrite(3),
lfdopen(3),
lfopen(3),
zlib(3),
lofasm-filterbank(5)
</MARKDOWN> */


/*************************************************************
STATUS LOGGING MACROS
**************************************************************/

/*
<MARKDOWN>
# lf_error(3)

## NAME

`lf_error, lf_warning, lf_info` - print status messages

## SYNOPSIS

`#include "lofasmIO.h"`

`extern int lofasm_verbosity;`

`void lf_error( const char *`_format_`, ... );`  
`void lf_warning( const char *`_format_`, ... );`  
`void lf_info( const char *`_format_`, ... );`

## DESCRIPTION

Programs compiled with `lofasmIO.h` may optionally use these functions
to standardize their error/warning/information reporting.  At present,
the functions simply wrap a call to fprintf(3), testing the value of
`lofasm_verbosity` before writing the message to `stderr`.  The values
of `lofasm_verbosity` are:

`0`:
    Quiet: No messages are written.

`1`:
    Default: Writes error messages (abnormal termination).

`2`:
    Verbose: As above, plus warnings (execution may proceed).

`3`:
    Very verbose: As above, plus info (normal status logging).

The functions may be implemented as macros: the arguments are used at
most once (in the call to fprintf(3)), but may not be used at all
depending on `lofasm_verbosity`, so arguments should not be
expressions with side effects.  Even for small values of
`lofasm_verbosity`, each invocation requires testing this value, so it
should not be used in an inner program loop: if you need a message
solely for debugging, it's better to wrap it in
`#ifdef DEBUG` ... `#endif`.

## RETURN VALUES

These functions do not return a value.

## SEE ALSO

fprintf(3)

</MARKDOWN> */
extern int lofasm_verbosity;
#define lf_error( ... ) \
do \
  if ( lofasm_verbosity > 0 ) { \
    const char *c = strrchr( __FILE__, '/' ); \
    c = ( c ? c + 1 : __FILE__ ); \
    fprintf( stderr, "\x1b[31;1m%s:\x1b[0m %s line %d: ", \
	     __func__, c, __LINE__ ); \
    fprintf( stderr, __VA_ARGS__ ); \
    putc( '\n', stderr ); \
  } \
while (0)
#define lf_warning( ... ) \
do \
  if ( lofasm_verbosity > 1 ) { \
    const char *c = strrchr( __FILE__, '/' ); \
    c = ( c ? c + 1 : __FILE__ ); \
    fprintf( stderr, "\x1b[33;1m%s:\x1b[0m %s line %d: ", \
	     __func__, c, __LINE__ ); \
    fprintf( stderr, __VA_ARGS__ ); \
    putc( '\n', stderr ); \
  } \
while (0)
#define lf_info( ... ) \
do \
  if ( lofasm_verbosity > 2 ) { \
    const char *c = strrchr( __FILE__, '/' ); \
    c = ( c ? c + 1 : __FILE__ ); \
    fprintf( stderr, "\x1b[32;1m%s:\x1b[0m %s line %d: ", \
	     __func__, c, __LINE__ ); \
    fprintf( stderr, __VA_ARGS__ ); \
    putc( '\n', stderr ); \
  } \
while (0)


/*************************************************************
FUNCTION PROTOTYPES
**************************************************************/

/* Routines to read/write compressed data. */
#ifndef NO_ZLIB
#include <zlib.h>
#endif
FILE *lfopen( const char *filename, const char *mode );
FILE *lfdopen( int fd, const char *mode );


/* Generic ABX/BBX I/O function prototypes. */
int
bxRead( FILE *fp, int *headc, char ***headv, int *dimc, int **dimv,
	char **encoding, void **data, int checks );
int
bxReadData( const char *encoding, void *buf, size_t n, FILE *fp );
int
bxWrite( FILE *fp, char **headv, const int *dimv,
	 const char *encoding, const void *data, int checks );
int
bxWriteData( const char *encoding, const void *buf, size_t n,
	     size_t nrow, FILE *fp );
void
bxSkipHeader( FILE *fp );


/* Generic lofasm-filterbank I/O function prototypes. */
int
lfbxRead( FILE *fp, lfb_hdr *header, void **data );
int
lfbxWrite( FILE *fp, lfb_hdr *header, void *data );
void
lfbxFree( lfb_hdr *header );

#ifdef  __cplusplus
#if 0
{
#endif
}
#endif
#endif /* _LOFASMIO_H */
