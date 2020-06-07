// 2>&-### SELF-EXTRACTING DOCUMENTATION ###############################
// 2>&-#                                                               #
// 2>&-# Run "bash <thisfile> > <docfile.md>" to extract documentation #
// 2>&-#                                                               #
// 2>&-#################################################################
// 2>&-; awk '/^<\/MARKDOWN>/{f=0};f;/^<MARKDOWN>/{f=1}' $0; exit 0

/***********************************************************************
lofasmIO.c
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

#include <bsd/stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "lofasmIO.h"

#define BX_CHECK_ID 1
#define BX_REQUIRE_ID 2
#define BX_CHECK_CRC32 4
#define BX_REQUIRE_CRC32 8

/* Default verbosity level. */
int lofasm_verbosity = 1;

#define LEN 1024 /* generic input buffer size */

/***********************************************************************
ZLIB INTERFACE ROUTINES
***********************************************************************/

/*
<MARKDOWN>
# lfopen(3)

## NAME

`lfopen(3)` - open a compressed or uncompressed file

## SYNOPSIS

`#include "lofasmIO.h"`

`FILE *lfopen( const char *`_filename_`, const char *`_mode_ `);`

## DESCRIPTION

Opens a file named _filename_ and returns a file pointer to it, with
the _mode_ argument specifying the type of access requested.  This
function works exactly like fopen(3), except that it will hook in
zlib(3) routines to read and write compressed data automatically.

When opening a file for reading, lfopen(), it will check the file for
a gzip(1) signature; when opening a file for writing, it will check
_filename_ for a `.gz` extension.  The _mode_ argument may also
contain extra flags to control this behaviour: a `Z` character
specifies compressed reading/writing, and a `T` character specifies
"transparent" (uncompressed) reading/writing, regardless of the file
name or signature.

Additional flags in _mode_ can set specific compression levels or
algorithms: see the documentation of zlib(3) if you require something
other than default behaviour.  Note that a compressed file cannot be
accessed with `+` mode (simultaneous reading and writing), and
lfopen() will return an error if this is attempted.

This function works only on named files.  To read/write compressed
data through a pipe or a standard stream such as `stdin` or `stdout`,
use lfdopen(3).

If compiled with `NO_ZLIB`, this function simply wraps fopen(3).
Thus, reading and writing will always be "transparent", and the
function will print a warning message if the _mode_ or _filename_
extension would have requested compressed input/output.

## RETURN VALUE

The function returns the file pointer associated with the file, or
NULL if the file could not be opened.

## SEE ALSO

fopen(3),
lfdopen(3),
zlib(3)

</MARKDOWN> */
FILE *lfopen( const char *filename, const char *mode )
{ 
#ifndef NO_ZLIB
  gzFile zfp;
  int i, j;
  char lmode[64] = {};
  int compress = strchr( mode, 'Z' ) != NULL;
  compress |= ( j = strlen( filename ) ) > 3 &&
    !strcmp( filename + j - 3, ".gz" );
  compress &= strchr( mode, 'T' ) == NULL;
  for ( i = j = 0; i < 62 && mode[i]; i++ )
    if ( mode[i] != 'Z' )
      lmode[j++] = mode[i];
  if ( !compress && strchr( mode, 'w' ) && !strchr( mode, 'T' ) )
    lmode[j++] = 'T';
  if ( !( zfp = gzopen( filename, lmode ) ) )
    return fopen( filename, mode );
  return funopen( zfp,
                  ( int (*)( void *, char *, size_t ) )gzread,
		  ( int (*)( void *, const char *, size_t ) )gzwrite,
		  ( fpos_t (*)( void *, fpos_t ,int ) )gzseek,
		  ( int (*)( void * ) )gzclose );
#else
  int i, j, k;
  char lmode[64] = {}, imode[64] = {};
  for ( i = j = k = 0; i < 63 && mode[i]; i++ )
    if ( strchr( "rwab+", mode[i] ) )
      lmode[j++] = mode[i];
    else
      imode[k++] = mode[i];
  if ( ( imode[0] || ( ( k = strlen( filename ) ) > 3 &&
		       !strcmp( filename + k - 3, ".gz" ) ) )
       && !strchr( imode, 'T' ) )
    lf_warning( "compiled with NO_ZLIB; ignoring compression" );
  return fopen( filename, lmode );
#endif
}

/*
<MARKDOWN>
# lfdopen(3)

## NAME

`lfdopen(3)` - allow compressed access to an open file descriptor

## SYNOPSIS

`#include "lofasmIO.h"`

`FILE *lfdopen( int` _fd_`, const char *`_mode_ `);`

## DESCRIPTION

This function returns a file pointer to an open file descriptor _fd_
according to access modes _mode_, while hooking in routines from
zlib(3) to allow automatic compression or decompression of data.

See lfopen(3) for more details about when compression/decompression is
enabled.  Since this function cannot use a filename extension to
determine whether compressed output is needed, lfdopen() will only
produce compressed output when specified by a `Z` flag in _mode_.

This function is particularly useful for enabling
compression/decompression on standard file streams such as `stdin` or
`stdout`.

## RETURN VALUE

A file pointer to the open file, or NULL, as for lfopen(3).

## EXAMPLE

To read compressed data from `stdin` and write compressed data to
`stdout`:

> `FILE *zstdin = lfdopen( 0, "rbZ" )`  
> `FILE *zstdout = lfdopen( 1, "wbZ" )`

To write compressed data to a system _command_ via a pipe:

> `FILE *fp = popen( "`_command_`", "wb" );`  
> `FILE *zfp = lfdopen( fileno( fp ), "wbZ" );`  
> `:`  
> `pclose( zfp );`

## SEE ALSO

fdopen(3),
lfopen(3),
zlib(3)

</MARKDOWN> */
FILE *lfdopen( int fd, const char *mode )
{
#ifndef NO_ZLIB
  gzFile zfp;
  int i, j;
  char lmode[64] = {};
  int compress = strchr( mode, 'Z' ) != NULL;
  compress &= strchr( mode, 'T' ) == NULL;
  for ( i = j = 0; i < 62 && mode[i]; i++ )
    if ( mode[i] != 'Z' )
      lmode[j++] = mode[i];
  if ( !compress && strchr( mode, 'w' ) && !strchr( mode, 'T' ) )
    lmode[j++] = 'T';
  if ( !( zfp = gzdopen( fd, lmode ) ) )
    return fdopen( fd, mode );
  return funopen( zfp,
                  ( int (*)( void *, char *, size_t ) )gzread,
		  ( int (*)( void *, const char *, size_t ) )gzwrite,
		  ( fpos_t (*)( void *, fpos_t ,int ) )gzseek,
		  ( int (*)( void * ) )gzclose );
#else
  int i, j, k;
  char lmode[64] = {}, imode[64] = {};
  for ( i = j = k = 0; i < 63 && mode[i]; i++ )
    if ( strchr( "rwab+", mode[i] ) )
      lmode[j++] = mode[i];
    else
      imode[k++] = mode[i];
  if ( imode[0] && !strchr( imode, 'T' ) )
    lf_warning( "compiled with NO_ZLIB; ignoring compression" );
  return fdopen( fd, lmode );
#endif
}

/***********************************************************************
GENERIC ABX/BBX READING AND WRITING
***********************************************************************/

/* Node in a variable-length list of strings. */
typedef struct tagstrlist_t {
  char *str;
  struct tagstrlist_t *next;
} strlist_t;

/* This macro evaluates to the numerical value of the hexadecimal
   digit.  The digit must be a character in the range '0' to '9', 'a'
   to 'f', or 'A' to 'F'; the macro evaluates to -1 if this is not the
   case. 
SB: Length of header isn't fixed. :/  header struct imposes a size of a float (4 bytes) meaning one hex character but there is a loop in the version parser which assumes the version is 8 hex characters long. 
   */
#define HEXVAL( digit ) \
( ( (digit) >= '0' && (digit) <= '9' ) ? (digit) - '0' : \
  ( ( (digit) >= 'a' && (digit) <= 'f' ) ? 10 + (digit) - 'a' : \
    ( ( (digit) >= 'A' && (digit) <= 'F' ) ? 10 + (digit) - 'A' : -1 ) ) )

/*
<MARKDOWN>
# bxReadData(3)

## NAME

`bxReadData(3)` - read data from an abx(5) or bbx(5) file

## SYNOPSIS

`#include "lofasmIO.h"`

`int64_t bxReadData( const char *`_encoding_`, void *`_buf_`, size_t` _n_`,
                    FILE *`_fp_ `);`

## DESCRIPTION

This function reads up to _n_ bytes from an abx(5) or bbx(5) file _fp_
according to the specified _encoding_, storing the bytes in _buf_.
Only complete bytes are read: if a parsing error is encountered in
_fp_, only the last complete byte is stored in _buf_.  The calling
function must be sure to pass a buffer with at least _n_ bytes of
space allocated to it.

## RETURN VALUE

Returns the number of complete bytes parsed from _fp_ and stored in
_buf_ (which will never exceed _n_), or -1 if passed bad arguments.
If _fp_ is unreadable (but non-NULL), the return code is 0.

## SEE ALSO

bxRead(3),
abx(5),
bbx(5)

</MARKDOWN> */
int64_t
bxReadData( const char *encoding, void *buf, size_t n, FILE *fp )
{
  unsigned char *ubuf = buf;
  if ( !encoding || !buf || !fp || n > INT64_MAX/8 ) {
    if ( !encoding )
      lf_error( "null encoding pointer" );
    if ( !buf )
      lf_error( "null storage buffer pointer" );
    if ( !fp )
      lf_error( "null file pointer" );
    if ( n > INT64_MAX/8 )
      lf_error( "requested number of bits 8*%lu exceeds INT64_MAX", n );
    return -1;
  }
  if ( !strcmp( encoding, "raw256" ) )
    return fread( ubuf, sizeof(unsigned char), n, fp );
  else if ( !strcmp( encoding, "raw16" ) ) {
    size_t i;       /* index */
    int ch, x1, x2; /* next character and hex values */
    for ( i = 0; i < n; i++ ) {
      do
	ch = fgetc( fp );
      while ( isspace( ch ) );
      if ( ( x1 = HEXVAL( ch ) ) < 0 )
	return i;
      do
	ch = fgetc( fp );
      while ( isspace( ch ) );
      if ( ( x2 = HEXVAL( ch ) ) < 0 )
	return i;
      ubuf[i] = 16*x1 + x2;
    }
  } else if ( !strcmp( encoding, "float" ) ) {
    size_t i; /* index */
    float f;  /* next float value */
    for ( i = 0; i < n/4; i++ ) {
      if ( fscanf( fp, "%f", &f ) != 1 )
	return i;
      memcpy( ubuf + 4*i, &f, 4*sizeof(unsigned char) );
    }
  } else if ( !strcmp( encoding, "double" ) ) {
    size_t i; /* index */
    double d; /* next double value */
    for ( i = 0; i < n/8; i++ ) {
      if ( fscanf( fp, "%lf", &d ) != 1 )
	return i;
      memcpy( ubuf + 8*i, &d, 8*sizeof(unsigned char) );
    }
  } else {
    lf_error( "unrecognized encoding: %s", encoding );
    return -1;
  }
  return n;
}


/*
<MARKDOWN>
# bxRead(3)

## NAME

`bxRead(3)` - read an abx(5) or bbx(5) file

## SYNOPSIS

`#include "lofasmIO.h"`

`int64_t bxRead( lfile` _fp_`, int *`_headc_`, char ***`_headv_`, int *`_dimc_`,
                int64_t **`_dimv_`, char **`_encoding_`, void **`_data_`,
                int` _checks_ ` );`

## DESCRIPTION

This function parses ABX (ASCII Bit Array) or BBX (Binary Bit Array)
data from a file stream.  These formats are described in detail in
their own abx(5) and bbx(5) documentation, but in simple terms, they
consist of zero or more header comment lines (each comment line
indicated by an initial `%` character), a single line of numerical
data storing the length of each dimension of the array and the name of
the encoding format, followed by the encoded data block.

When called, _fp_ should point to an open readable file stream.  BBX
files should be read in binary mode, while ABX files may be read in
ASCII or binary mode.

Header comments are reported by the _headv_ and _headc_ arguments.  If
_headc_ is not NULL, then `*`_headc_ will return the number of header
lines read.  If _headv_ is not NULL, then `*`_headv_ will be allocated
to point to an array of strings storing the comments.  The length of
the array is one greater than the number of comment lines (i.e. one
greater than `*`_headc_): the last element in the array will be
`*`_headv_`[*`_headc_`] = NULL`.  Thus, `*`_headv_ and `*`_headc_ are
like argv and argc variables, and it is possible to parse `*`_headv_
without knowing `*`_headc_ (by looking for the NULL terminator).  Each
entry in the `*`_headv_ array will be a single comment line, without
the trailing newline or initial `%` character.  If both _headc_ and
_headv_ are NULL, then the header comments are silently skipped.

The _dimc_ and _dimv_ arguments report the size and dimensions of the
data: _dimv_ must be non-NULL.  On return, `*`_dimc_ will store the
number N of dimensions of the array, and `*`_dimv_ will be an array of
length `*`_dimc_+1 storing the size of the array in each dimension;
`*`_dimv_`[N]` will be 0 (an invalid dimension size) to mark then end
of the vector in the absence of `*`_dimc_.

If _encoding_ is not NULL, then `*`_encoding_ should be passed in as
NULL.  On return, it will be allocated to point to a string storing
the name of the encoding.

If _data_ is NULL, the parser will stop after reading the metadata,
leaving _fp_ positioned at the start of the data block.  Otherwise,
_data_ should be a non-NULL handle to a NULL pointer.  On return,
`*`_data_ will be allocated to length:

> len = ceil( L[0]\*...\*L[N-1]/8.0 )

(in bytes), where N is the number of dimensions (returned in
`*`_dimc_), and L[0..N-1] are the lenghts of each dimension (returned
in `*`_dimv_).  This array is loaded with data read from the remainder
of the file stream, according to the format described below.  If the
stream encounters an end-of-data condition or error condition before
filling `*`_data_, the remaining space will be filled with zeroes.

The _checks_ argument is a bitwise-or of flags indicating additional
integrity checks to be performed on the data.  Normally, these involve
testing the presence and value of certain header comment fields.  If
_checks_ is 0, then no additional checks will be performed.  See
**Integrity Checks**, below, for more details.

### Integrity Checks

If the _checks_ and the _headv_ arguments are nonzero, then bxRead(3)
will perform additional tests on the ABX or BBX file, returning an
error (and discarding the read data) if any checks fail.  The tests
are performed after reading all metadata, so a parsing error before
this point will trigger a different error.  The _checks_ argument
should be a bitwise-or of one or more of the following flags
(`#define`d as global constants):

`BX_CHECK_ID`:
  Looks for `%ABX` or `%\002BX` as the first comment line, and checks
  whether it is consistent with the requested encoding.  If the ID is
  not present, the test is ignored.  This test is done after reading
  all metadata but before starting to read the data.

`BX_REQUIRE_ID`:
  As `BX_CHECK_ID`, above, but fails automatically if no ID comment
  line is found.

## RETURN VALUE

On success, the function returns the actual number of bytes read and
stored in `*`_data_, which may be less than L[0]\*...\*L[N-1]/8 on a
premature end of data.  This does not necessarily reflect an error,
and the output is deterministic (remaining data are set to zero): a
file may deliberately end early if the remaining data are zero.  A
return code of 0 means the file ended immediately after the metadata
line.  A return code of -1 indicates that the metadata line was
unreadable (unparseable or non-positive numbers, unrecognized
encoding); -2 indicates that a non-optional handle argument was passed
in as NULL, or is non-NULL and points to a non-NULL pointer; -3
indicates that internal memory allocation failed.

If the file failed an integrity check specified by the _checks_
variable, then the return code is -4 times the bitwise-or of the flags
for the failed checks.  Thus one can determine which check failed by
taking `( -( return code ) ) >> 2` and doing a bitwise-and against the
various flags.

On a negative return code, `*`_headc_, `*`_headv_, `*`_dimc_,
`*`_dimv_, and `*`_data_ will be unchanged, though `*`_fp_ may be
advanced.

## SEE ALSO

bxReadData(3),
bxWrite(3),
abx(5),
bbx(5)

</MARKDOWN> */
int64_t
bxRead( FILE *fp, int *headc, char ***headv, int *dimc, int64_t **dimv,
	char **encoding, void **data, int checks )
{
  unsigned char *dat = NULL; /* internal copy of *data */
  char *enc = NULL;          /* internal copy of *encoding */
  int hc = 0, dc = 0;        /* internal copies of *headc, *dimc */
  int64_t i, n, nb;          /* index, counter, and total number of bits */
  int64_t k = 0;             /* number of bytes read */
  char *line;                /* current line */
  char *a, *b;               /* pointers within current line */
  int64_t *dims;             /* dimensions */
  int len, max = 1024;       /* current and allocated length of line or dims */
  strlist_t head = {};       /* head of list of comment strings */
  strlist_t *here, *prev;    /* pointers to list of comment strings */
  char type = 0;             /* designated format 'a' or 'b' */
  char *temp;

  /* Check arguments. */
  if ( !fp || !dimv || *dimv || ( encoding && *encoding ) ||
       ( data && *data ) || ( headv && *headv ) ) {
    if ( !fp )
      lf_error( "null file pointer" );
    if ( !dimv )
      lf_error( "null dimv handle" );
    if ( *dimv )
      lf_error( "dimv handle points to non-NULL pointer" );
    if ( encoding && *encoding )
      lf_error( "encoding handle points to non-NULL pointer" );
    if ( data && *data )
      lf_error( "data handle points to non-NULL pointer" );
    if ( headv && *headv )
      lf_error( "headv handle points to non-NULL pointer" );
    return -2;
  }

  /* Read header. */
  if ( !( line = (char *)malloc( max*sizeof(char) ) ) ) {
    lf_error( "memory error" );
    return -3;
  }
  here = &head;
  while ( !feof( fp ) ) {
    /* Read an entire line, lengthening as necessary. */
    len = 0;
    while ( !feof( fp ) ) {
      if ( fgets( line + len, max - len, fp ) ) {
	len = strlen( line );
	if ( line[len-1] == '\n' )
	  break;
	temp = (char *)realloc( line, 2*max );
	if ( !temp ) {
	  free( line );
	  for ( prev = here = head.next; here; prev = here ) {
	    here = here->next;
	    free( prev->str );
	    free( prev );
	  }
	  lf_error( "memory error" );
	  return -3;
	}
	line = temp;
	max *= 2;
      }
    }

    /* Add comment lines to head linked list. */
    if ( line[0] != '%' )
      break;
    hc++;
    if ( !( here->next = (strlist_t *)calloc( 1, sizeof(strlist_t) ) ) ||
	 !( here->next->str = (char *)malloc( (len-1)*sizeof(char) ) ) ) {
      free( line );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      lf_error( "memory error" );
      return -3;
    }
    here = here->next;
    strncpy( here->str, line + 1, len - 2 );
  }

  /* Read dimensions from first non-comment line. */
  max = 1024;
  if ( !( dims = (int64_t *)malloc( max*sizeof(int64_t) ) ) ) {
    free( line );
    for ( prev = here = head.next; here; prev = here ) {
      here = here->next;
      free( prev->str );
      free( prev );
    }
    lf_error( "memory error" );
    return -3;
  }
  for ( dc = 0, a = line; 1; dc++, a = b ) {
    unsigned long long d = strtoull( a, &b, 10 );
    if ( b == a )
      break;
    if ( d < 1 || d > INT64_MAX ) {
      free( line );
      free( dims );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      lf_error( "dimension[%d] = %llu out of range", dc, d );
      return -1;
    }
    dims[dc] = d;
    if ( dc >= max - 1 ) {
      int64_t *temp = (int64_t *)realloc( dims, 2*max*sizeof(int64_t) );
      if ( !temp ) {
	free( line );
	free( dims );
	for ( prev = here = head.next; here; prev = here ) {
	  here = here->next;
	  free( prev->str );
	  free( prev );
	}
	lf_error( "memory error" );
	return -3;
      }
      dims = temp;
      max *= 2;
    }
  }
  dims[dc] = 0;
  for ( i = 0, nb = 1; i < dc; i++ ) {
    if ( nb > INT64_MAX/dims[i] ) {
      free( line );
      free( dims );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      lf_error( "total size of array exceeds INT64_MAX bits" );
      return -3;
    }
    nb *= dims[i];
  }

  /* Get encoding name. */
  while ( isspace( (int)( *a ) ) )
    a++;
  b = a;
  while ( *b && !isspace( (int)( *b ) ) )
    b++;
  if ( *b != '\n' ) {
    free( line );
    free( dims );
    for ( prev = here = head.next; here; prev = here ) {
      here = here->next;
      free( prev->str );
      free( prev );
    }
    lf_error( "encoding token '%s' not followed by newline", a );
    return -1;
  }
  *b = '\0';
  if ( !( enc = (char *)malloc( ( b - a + 1 )*sizeof(char) ) ) ) {
    free( line );
    free( dims );
    for ( prev = here = head.next; here; prev = here ) {
      here = here->next;
      free( prev->str );
      free( prev );
    }
    lf_error( "memory error" );
    return -3;
  }
  strcpy( enc, a );
  free( line );

  /* Check format. */
  if ( head.next ) {
    if ( !strncmp( head.next->str, "%ABX", 4 ) )
      type = 'a';
    else if ( !strncmp( head.next->str, "%BBX", 4 ) )
      type = 'b';
  }

  /* Run initial tests. */
  if ( checks ) {
    if ( ( checks&BX_REQUIRE_ID ) &&
	 ( !type ||
	   ( type == 'a' && strcmp( enc, "raw16" ) &&
	     strcmp( enc, "float" ) ) ||
	   ( type == 'b' && strcmp( enc, "raw256" ) ) ) ) {
      free( dims );
      free( enc );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      if ( !type )
	lf_error( "REQUIRE_ID: file type (ascii/binary) not specified" );
      else
	lf_error( "REQUIRE_ID: encoding '%s' not recognized for %s file type",
		  enc, ( type == 'a' ? "ascii" : "binary" ) );
      return -4*BX_REQUIRE_ID;
    }
    if ( ( checks&BX_CHECK_ID ) &&
	 ( type &&
	   ( ( type == 'a' && strcmp( enc, "raw16" ) &&
	       strcmp( a, "float" ) ) ||
	     ( type == 'b' && strcmp( enc, "raw256" ) ) ) ) ) {
      free( dims );
      free( enc );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      lf_error( "CHECK_ID: encoding '%s' not recognized of %s file type",
		enc, ( type == 'a' ? "ascii" : "binary" ) );
      return -4*BX_CHECK_ID;
    }
  }
  if ( ( !strcmp( enc, "float" ) && dims[dc-1]%32 ) ||
       ( !strcmp( enc, "double" ) && dims[dc-1]%64 ) ) {
    free( dims );
    free( enc );
    for ( prev = here = head.next; here; prev = here ) {
      here = here->next;
      free( prev->str );
      free( prev );
    }
    lf_error( "incorrect number of bits %lld for encoding '%s'",
	      (long long)( dims[dc-1] ), enc );
    return -1;
  }

  /* Allocate data array (assuming data is not a NULL handle). */
  if ( data ) {
    n = ( nb%8 ? nb/8 + 1 : nb/8 );
    if ( !( dat = (unsigned char *)calloc( n, sizeof(unsigned char) ) ) ) {
      free( dims );
      free( enc );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      lf_error( "memory error" );
      return -3;
    }

    /* Read data. */
    if ( ( k = bxReadData( enc, dat, n, fp ) ) < 0 ) {
      free( dat );
      free( dims );
      free( enc );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      lf_error( "read error" );
      return -1;
    }
  }

  /* Set output arguments. */
  if ( headv ) {
    if ( !( *headv = (char **)malloc( (hc+1)*sizeof(char *) ) ) ) {
      free( dims );
      free( enc );
      if ( dat )
	free( dat );
      for ( prev = here = head.next; here; prev = here ) {
	here = here->next;
	free( prev->str );
	free( prev );
      }
      lf_error( "memory error" );
      return -3;
    }
    for ( prev = here = head.next, i = 0; here; prev = here, i++ ) {
      here = prev->next;
      (*headv)[i] = prev->str;
      free( prev );
    }
    (*headv)[i] = NULL;
  }
  if ( headc )
    *headc = hc;
  if ( dimc )
    *dimc = dc;
  *dimv = dims;
  if ( encoding )
    *encoding = enc;
  if ( data )
    *data = dat;
  return k;
}

/*
<MARKDOWN>
# bxSkipHeader(3)

## NAME

`bxSkipHeader(3)` - skip to start of ABX/BBX data block

## SYNOPSIS

`#include "lofasmIO.h"`

`void bxSkipHeader( FILE *`_fp_ ` );`

## DESCRIPTION

This function advances _fp_ to the start of the line following the
first line that does not start with a `%` character.  That is, it
skips over BX comment lines and the BX dimension/encoding metadata
line, leaving _fp_ positioned at the start of the data block.

The effect on _fp_ is similar to a call to lfbxRead(3) with a NULL
_data_ pointer, without the overhead of actually parsing the header.

## RETURN VALUE

This function does not return a value.

## SEE ALSO

bxRead(3),
abx(5),
bbx(5)

</MARKDOWN> */
void
bxSkipHeader( FILE *fp )
{
  char buf[LEN] = {}; /* input buffer */
  while ( fgets( buf, LEN, fp ) && buf[0] == '%' )
    while ( buf[ strlen( buf ) - 1 ] != '\n' && fgets( buf, LEN, fp ) )
      ;
  while ( buf[ strlen( buf ) - 1 ] != '\n' && fgets( buf, LEN, fp ) )
    ;
  return;
}


/*
<MARKDOWN>
# bxWriteData(3)

## NAME

`bxWriteData(3)` - write data to an abx(5) or bbx(5) file

## SYNOPSIS

`#include "lofasmIO.h"`

`int64_t bxWriteData( const char *`_encoding_`, const void *`_buf_`,
                     size_t` _n_`, size_t` _nrow_`, FILE *`_fp_ `);`

## DESCRIPTION

This function writes up to _n_ bytes from _buf_ to a file _fp_
according to the specified _encoding_ for an abx(5) or bbx(5) file.
For an ASCII _encoding_, newlines are inserted at least every 80
characters (or fewer); additionally, if _nrow_ is positive, an extra
newline is inserted after _nrow_ bytes (this is used to break up rows
in human-readable output, assuming that each row contains an integer
number of bytes or multi-byte sequences as specified by the
_encoding_).  The calling function must be sure to pass a buffer with
at least _n_ bytes of data in it.

## RETURN VALUE

Returns the number of complete bytes successfully written to _fp_.  If
this is less than _n_, then a write error occurred; the data in _fp_
should be correct up to the last confirmed successfully-encoded byte,
but may have garbled or partially-encoded data after this point.  The
value of _errno_ will be set as for fputc(3).

## SEE ALSO

bxWrite(3),
abx(5),
bbx(5)

</MARKDOWN> */
int64_t
bxWriteData( const char *encoding, const void *buf, size_t n,
	     size_t nrow, FILE *fp )
{
  int64_t i, nl;  /* number of bytes written and bytes per line */
  int64_t kr, kl; /* indecies over bytes written per row and line */
  const unsigned char *ubuf = buf;
  if ( !encoding || !buf || !fp || n > INT64_MAX/8 ) {
    if ( !encoding )
      lf_error( "null encoding pointer" );
    if ( !buf )
      lf_error( "null storage buffer pointer" );
    if ( !fp )
      lf_error( "null file pointer" );
    if ( n > INT64_MAX/8 )
      lf_error( "requested number of bits 8*%lu exceeds INT64_MAX", n );
    return -1;
  }
  if ( nrow == 0 )
    nrow = n;
  if ( !strcmp( encoding, "raw256" ) )
    return fwrite( ubuf, sizeof(unsigned char), n, fp );
  else if ( !strcmp( encoding, "raw16" ) ) {
    int a, b; /* hexadecimal integers for each nibble */
    nl = 40;
    for ( i = kr = kl = 0; i < n; i++ ) {
      a = ubuf[i]/16;
      b = ubuf[i]%16;
      if ( fprintf( fp, "%c%c", ( a < 10 ? '0' + a : 'A' + a - 10 ),
		    ( b < 10 ? '0' + b : 'A' + b - 10 ) ) < 0 )
	return i;
      if ( ( kl += 2 ) >= nl && ( kl = 0, fprintf( fp, "\n" ) < 0 ) )
	return i + 1;
      if ( ( kr += 2 ) >= nrow && ( kr = kl = 0, fprintf( fp, "\n" ) < 0 ) )
	return i + 1;
    }
    if ( kl > 0 && fprintf( fp, "\n" ) < 1 )
      return i + 1;
  }
  else if ( !strcmp( encoding, "float" ) ) {
    float f; /* four bytes encoded as a float */
    nl = 5*sizeof(float);
    for ( i = kr = kl = 0; i < n; i += sizeof(float) ) {
      memcpy( &f, ubuf + i, sizeof(float) );
      if ( fprintf( fp, "% 16.8e", f ) < 0 )
	return i;
      if ( ( kl += 4 ) >= nl && ( kl = 0, fprintf( fp, "\n" ) < 0 ) )
	return i + 1;
      if ( ( kr += 4 ) >= nrow && ( kr = kl = 0, fprintf( fp, "\n" ) < 0 ) )
	return i + 1;
    }
    if ( kl > 0 && fprintf( fp, "\n" ) < 0 )
      return i + 1;
  }
  else if ( !strcmp( encoding, "double" ) ) {
    float d; /* eight bytes encoded as a float */
    nl = 3*sizeof(double);
    for ( i = kr = kl = 0; i < n; i += sizeof(float) ) {
      memcpy( &d, ubuf + i, sizeof(float) );
      if ( fprintf( fp, "% 25.16e", d ) < 0 )
	return i;
      if ( ( kl += 8 ) >= nl && ( kl = 0, fprintf( fp, "\n" ) < 0 ) )
	return i + 1;
      if ( ( kr += 8 ) >= nrow && ( kr = kl = 0, fprintf( fp, "\n" ) < 0 ) )
	return i + 1;
    }
    if ( kl > 0 && fprintf( fp, "\n" ) < 0 )
      return i + 1;
  }
  return n;
}


/*
<MARKDOWN>
# bxWrite(3)

## NAME

`bxWrite(3)` - write an abx(5) or bbx(5) file

## SYNOPSIS

`#include "lofasmIO.h"`

`int bxWrite( FILE *`_fp_`, const char **`_headv_`, const int64_t *`_dimv_`,
             const char *`_encoding_`, const void *`_data_`, int ` _checks_ ` );`

## DESCRIPTION

This function writes ABX (ASCII Bit Array) or BBX (Binary Bit Array)
data to a file stream.  These formats are described in detail in their
own abx(5) and bbx(5) documentation, but in simple terms, they consist
of zero or more header comment lines (each comment line indicated by
an initial `%` character), a single line of numerical data storing the
length of each dimension of the array and the name of the encoding
format, followed by the encoded data block.

When called, _fp_ should point to an open writable file stream: BBX
files should be written in binary mode, while ABX files may be written
in ASCII or binary mode.

The _headv_ argument should point to an array of strings, with a NULL
pointer indicating the end of the array, following the converntion for
argv-style string arrays.  (Thus, no explicit array length need be
specicfied.)  These strings will be written, in order, as header
comments: they will be written preceded by a `%` character and
terminated by a `\n` character.  Properly speaking, the strings
themselves should not contain newlines: multi-line comments should be
recorded as separate strings.  However, this routine is lenient, and
will simply insert `%` characters after any newlines it encounters in
_headv_.

The _dimv_ argument should point to an array of integers storing the
lengths of each dimension of the bit array.  Entries should be
positive numbers, with a value of 0 indicating the end of the array.
(Thus, once again, no explicit array length need be specified.)

The _encoding_ argument should be a string specifying one of the
recognixed ABX or BBX encodings; see the documentation in bxRead(3).
Note that some encodings may place restrictions on other metadata: for
instance, the `float` encoding requires the last nonzero entry in
_dimv_ to be a multiple of 32.

The _data_ argument is the data to be written, it should contain at
least as many bits as the product of the leading positive entries in
_dimv_; any additional bits allocated in _data_ are ignored (including
trailing bits required to give an integer number of bytes).  If it is
NULL, then only the file header will be written.

The _checks_ argument is a bitwise-or of flags indicating additional
header comments that will be generated and inserted automatically by
bxWrite(3), that can subsequently be used to test the integrity of the
file.  If _checks_ is 0, then only those header comments specified in
_headv_ will be written, and it is the caller's responsibility to
ensure that any integrity check fields are correct.  See **Integrity
Checks**, below, for more details.

### Integrity Checks

If the _checks_ is nonzero, then bxWrite(3) may write additional
comments beyond those specified in _headv_.  The _checks_ argument
should be a bitwise-or of one or more of the following flags
(`#define`d as global constants):

`BX_CHECK_ID`:
  If _headv_[0] is either `%ABX` or `%\002BX`, checks that it matches
  the requested _encoding_, changing _headv_[0] if necessary.  If
  _headv_ is not present or doesn't match one of these values, then
  this flag is ignored.

`BX_REQUIRE_ID`:
  Ensures that the first line of the file begins with either `%ABX` or
  `%\002BX` according to the specified _encoding_.  If _headv_ is
  present and the first string begins with the correct sequence, then
  no additional steps are taken; if _headv_ is not present or
  _headv_[0] does not match the required ID, then the `%ABX` or
  `%\002BX` comment line is prepended to the file before the entries
  in _headv_.

## RETURN VALUE

The function returns 0 on success, -1 if passed bad arguments, or -2
on a writing error.  A return code of -1 leaves _fp_ untouched, while
a return code of -2 may correspond to a partially-written file.

## SEE ALSO

bxRead(3),
bxWriteData(3),
abx(5),
bbx(5)

</MARKDOWN> */
int
bxWrite( FILE *fp, char **headv, const int64_t *dimv,
	 const char *encoding, const void *data, int checks )
{
  const int64_t *dim;    /* pointer to dimv */
  char **head;           /* pointer to headv */
  const char *c;         /* pointer to character in headv */
  int64_t nb = 1, n, nr; /* number of bits, bytes, and bytes per row */

  /* Check arguments. */
  if ( !fp || !headv || !dimv || dimv[0] <= 0 || !encoding ) {
    if ( !fp )
      lf_error( "null file pointer" );
    if ( !headv )
      lf_error( "null headv array" );
    if ( !dimv )
      lf_error( "null dimv array" );
    if ( dimv[0] <= 0 )
      lf_error( "non-positive dimv[0]" );
    if ( !encoding )
      lf_error( "null encoding string" );
    return -1;
  }
  for ( dim = dimv; *dim; dim++ ) {
    if ( *dim < 0 ) {
      lf_error( "negative dimension" );
      return -1;
    } else if ( nb > INT64_MAX/( *dim ) ) {
      lf_error( "number of bits exceeds INT64_MAX" );
      return -1;
    }
    nb *= *dim;
  }
  n = ( nb%8 ? nb/8 + 1 : nb/8 );
  nr = ( ( nb/dimv[0] )%8 ? n : nb/dimv[0]/8 );
  dim--;
  if ( !strcmp( encoding, "float" ) && ( *dim )%( 8*sizeof(float) ) ) {
    lf_error( "float encoding: last dimension %lld not a multiple of %lu bits",
	      (long long)( *dim ), 8*sizeof(float) );
    return -1;
  }
  if ( !strcmp( encoding, "double" ) && ( *dim )%( 8*sizeof(double) ) ) {
    lf_error( "double encoding: last dimension %lld not a multiple of %lu bits",
	      (long long)( *dim ), 8*sizeof(double) );
    return -1;
  }
  if ( strcmp( encoding, "raw16" ) && strcmp( encoding, "float" ) &&
       strcmp( encoding, "double" ) && strcmp( encoding, "raw256" ) ) {
    lf_error( "unrecognized encoding '%s'", encoding );
    return -1;
  }

  /* Perform additional requested checks. */
  head = headv;
  if ( checks&BX_REQUIRE_ID ) {
    if ( ( !strcmp( encoding, "raw16" ) || !strcmp( encoding, "float" ) ||
	   !strcmp( encoding, "double" ) ) &&
	 ( !(*head) || strncmp( *head, "ABX", 3 ) ) ) {
      if ( fputs( "%ABX\n", fp ) == EOF ) {
	lf_error( "write error" );
	return -2;
      }
    } else if ( !strcmp( encoding, "raw256" ) &&
		( !(*head) || strncmp( *head, "BBX", 3 ) ) ) {
      if ( fputs( "%BBX\n", fp ) == EOF ) {
	lf_error( "write error" );
	return -2;
      }
    }
  } else if ( checks&BX_CHECK_ID && *head &&
	      !( strncmp( *head, "ABX", 3 ) && strncmp( *head, "BBX", 3 ) ) ) {
    if ( !strcmp( encoding, "raw16" ) || !strcmp( encoding, "float" ) ||
	 !strcmp( encoding, "double" ) ) {
      if ( fputs( "%ABX\n", fp ) == EOF ) {
	lf_error( "write error" );
	return -2;
      }
    } else if ( !strcmp( encoding, "raw256" ) ) {
      if ( fputs( "%BBX\n", fp ) == EOF ) {
	lf_error( "write error" );
	return -2;
      }
    }
    head++;
  }

  /* Write comments. */
  for ( ; *head; head++ ) {
    if ( putc( '%', fp ) == EOF ) {
      lf_error( "write error" );
      return -2;
    }
    for ( c = *head; *c; c++ ) {
      if ( putc( *c, fp ) == EOF ) {
	lf_error( "write error" );
	return -2;
      }
      if ( *c == '\n' && putc( '%', fp ) == EOF ) {
	lf_error( "write error" );
	return -2;
      }
    }
    if ( putc( '\n', fp ) == EOF ) {
      lf_error( "write error" );
      return -2;
    }
  }

  /* Write metadata line. */
  for ( dim = dimv; *dim; dim++ )
    if ( fprintf( fp, "%lld ", (long long)( *dim ) ) < 0 ) {
      lf_error( "write error" );
      return -2;
    }
  if ( fprintf( fp, "%s\n", encoding ) < 0 ) {
    lf_error( "write error" );
    return -2;
  }

  /* Write data. */
  if ( data && bxWriteData( encoding, data, n, nr, fp ) < n ) {
    lf_error( "write error" );
    return -2;
  }

  /* Finished. */
  return 0;
}

/***********************************************************************
LOFASM FILTERBANK READING AND WRITING
***********************************************************************/

/* This macro returns the value of a key:value pair.  Assuming str is
   of the form "<key>:<value>" for the requested key, the macro
   evaluates to a pointer to the character after the colon; otherwise
   it evaluates to NULL. */
#define KEYVAL( str, key ) \
( strncmp( str, key, strlen( key ) ) || str[strlen( key )] != ':' ? NULL : \
  str + strlen( key ) + 1 )

/* A function wrapper of the above macro, to protect its arguments. */
char *keyval( char *str, char *key )
{
  return KEYVAL( str, key );
}

/* This function trims whitespace from the front and back of src,
   allocates *dest to be long enough to hold the trimmed string, and
   copies the trimmed contents.  Initially *dest should be NULL, but
   this is not checked.  If the trimming leaves an empty string, then
   *dest is left unchanged.  Returns 0, or 1 if allocation failed. */
int
dupstr( char **dest, char *src ) {
  char *a, *b = src + strlen( src ) - 1;
  char c;
  while ( b >= src && isspace( (int)( *b ) ) )
    b--;
  c = b[1];
  b[1] = '\0';
  a = src;
  while ( isspace( (int)( *a ) ) )
    a++;
  if ( *a ) {
    if ( !( *dest = (char *)malloc( strlen( a ) + 1 ) ) ) {
      b[1] = c;
      return 1;
    }
    strcpy( *dest, a );
  }
  b[1] = c;
  return 0;
}

/* Computes the base SI from a string of the form "<value> (<unit>)",
   where <unit> is the specified unit with optional SI prefix
   "afpnumkMGTPE" (note that only power-of-1000 prefixes are
   recognized, and "micro" is abbreviated 'u').  All arguments should
   be non-NULL, but this is not checked.  Returns 0, or 1 if the unit
   is not recognized (in which case *value is unchanged). */
int
si_value( double *value, char *string, char *unit )
{
  char prefix[] = "afpnumkMGTPE";
  char *u = strchr( string, '(' ), *p;
  double val = atof( string );
  if ( !u )
    return 1;
  do
    u++;
  while ( isspace( (int)( *u ) ) );
  if ( ( p = strchr( prefix, *u ) ) && !strncmp( u + 1, unit, strlen( unit ) ) )
    val *= pow( 1000.0, -6 + (int)( p - prefix ) );
  else if ( strncmp( u, unit, strlen( unit ) ) )
    return 1;
  *value = val;
  return 0;
}

/*
<MARKDOWN>
# lfbxRead(3)

## NAME

`lfbxRead(3)` - read a BBX file with LoFASM filterbank header

## SYNOPSIS

`#include "lofasmIO.h"`

`int lfbxRead( FILE *`_fp_`, lfb_hdr *`_header_`, void **`_data_ ` );`

## DESCRIPTION

This function reads a standard LoFASM data file: a BBX file with
specific header fields needed to populate a `lfb_hdr` structure.  The
_fp_ argument should point to the start of an open binary-readable
file, and the _header_ should point to the structure to be populated;
both must be non-NULL.  See the lofasm-filterbank(5) documentation for
more information about the LoFASM filterbank format.

If _data_ is NULL, then reading stops at the end of the header,
leaving _fp_ pointing to the first byte of data.  This is the normal
mode of operation, since one will rarely want to read the entire file
into memory at once.  However, should you so desire, then _data_ may
be a non-NULL handle to a NULL pointer `*`_data_.  The routine will
then allocate `*`_data_ to point to an array large enough to hold the
entire data block, and will fill it with the data.  Regardless of the
specified bit depth, the memory allocated by this routine will always
be aligned to a multiple of 8 bytes, allowing it to safely store any
standard data type up to double-precision floats.

## RETURN VALUE

The function returns 0 normally, but may issue a *warning* if a
required or expected header field is missing.  It returns nonzero, and
issues an *error*, if the routine could not parse the array dimensions
or encoding, or allocate `*`_data_ (if requested), or if _fp_ or
_header_ is NULL, or if `*`_data_ is passed as non-NULL.  On error, a
partially-filled header may be returned, to allow for higher-level
diagnostics.

## SEE ALSO

lfbxWrite(3),
bbx(5),
lofasm-filterbank(5)

</MARKDOWN> */
int
lfbxRead( FILE *fp, lfb_hdr *header, void **data )
{
  char line[LEN];        /* current line */
  char *str, *tail;      /* pointers to header strings */
  int64_t i, n;          /* index and length */
  unsigned long long d;  /* dimension */
  int read = 0;          /* whether a line was already read */
  int err = 0, warn = 0; /* error and warning flags */

  /* Check arguments. */
  if ( !fp || !header || ( data && *data ) ) {
    if ( !fp )
      lf_error( "null file pointer" );
    if ( !header )
      lf_error( "null header" );
    if ( data && *data )
      lf_error( "data handle points to non-NULL pointer" );
    return 3;
  }

  /* Set default values for header fields. */
  memset( header, 0, sizeof(lfb_hdr) );
  header->hdr_version = strtof( "NaN", NULL );
  header->dim1_start = header->dim1_span = header->dim2_start
    = header->dim2_span = strtod( "NaN", NULL );
  header->data_scale = 1.0;

  /* Check first header line. */
  if ( !fgets( line, LEN, fp ) || strcmp( line, "%\002BX\n" ) ) {
    lf_warning( "no BBX signature" );
    warn = 1;
    read = 1;
  }

  /* Parse header comments. */
  while ( 1 ) {

    /* Get line, breaking on first non-comment line. */
    if ( read )
      read = 0;
    else if ( !fgets( line, LEN, fp ) ) {
      line[0] = '\0';
      break;
    }
    if ( line[0] != '%' )
      break;

    /* Parse string fields. */
    if ( ( tail = keyval( line + 1, "hdr_type" ) ) )
      err = dupstr( &( header->hdr_type ), tail );
    else if ( ( tail = keyval( line + 1, "station" ) ) )
      err = dupstr( &( header->station ), tail );
    else if ( ( tail = keyval( line + 1, "channel" ) ) )
      err = dupstr( &( header->channel ), tail );
    else if ( ( tail = keyval( line + 1, "start_time" ) ) )
      err = dupstr( &( header->start_time ), tail );
    else if ( ( tail = keyval( line + 1, "dim1_label" ) ) )
      err = dupstr( &( header->dim1_label ), tail );
    else if ( ( tail = keyval( line + 1, "dim2_label" ) ) )
      err = dupstr( &( header->dim2_label ), tail );
    else if ( ( tail = keyval( line + 1, "data_label" ) ) )
      err = dupstr( &( header->data_label ), tail );
    else if ( ( tail = keyval( line + 1, "data_type" ) ) )
      err = dupstr( &( header->data_type ), tail );

    /* Numerical fields. */
    else if ( ( tail = keyval( line + 1, "start_mjd" ) ) )
      header->start_mjd = atof( tail );
    else if ( ( tail = keyval( line + 1, "dim1_start" ) ) )
      header->dim1_start = atof( tail );
    else if ( ( tail = keyval( line + 1, "dim1_span" ) ) )
      header->dim1_span = atof( tail );
    else if ( ( tail = keyval( line + 1, "dim2_start" ) ) )
      header->dim2_start = atof( tail );
    else if ( ( tail = keyval( line + 1, "dim2_span" ) ) )
      header->dim2_span = atof( tail );
    else if ( ( tail = keyval( line + 1, "data_offset" ) ) )
      header->data_offset = atof( tail );
    else if ( ( tail = keyval( line + 1, "data_scale" ) ) )
      header->data_scale = atof( tail );

    /* Version field. */
    else if ( ( tail = keyval( line + 1, "hdr_version" ) ) ) {
      unsigned char b[4]; /* version as byte array */
      float f;         /* version as float */
      char *c = tail;     /* position in value string */
      while ( isspace( *c ) )
	c++;
						f = atof (c);
      if ( f < 1.0 || f > 255.0 || f != floor( f ) ) {
	lf_warning( "file has wrong endianness" );
	warn = 1;
      } else {
	header->hdr_version = f;
	if ( f != LFB_VERSION ) {
	  lf_warning( "reading version %.0f file with version %d"
		      " library", f, LFB_VERSION );
	  warn = 1;
	}
      }
    }

    /* Offset fields. */
    else if ( ( tail = keyval( line + 1, "time_offset_J2000" ) ) ) {
      if ( si_value( &( header->time_offset_J2000 ), tail, "s" ) )
	lf_warning( "ignoring invalid time_offset_J2000" );
    } else if ( ( tail = keyval( line + 1, "frequency_offset_DC" ) ) ) {
      if ( si_value( &( header->frequency_offset_DC ), tail, "Hz" ) )
	lf_warning( "ignoring invalid frequency_offset_DC" );
    }

    /* Check for allocation error within dupstr() calls. */
    if ( err ) {
      lf_error( "could not allocate header comment string %.64s", tail );
      return 2;
    }

    /* If line was longer than buffer, skip to next line. */
    while ( line[ strlen( line ) - 1 ] != '\n' )
      if ( !fgets( line, LEN, fp ) ) {
	line[0] = '\0';
	break;
      }
  }

  /* Check for premature EOF. */
  if ( !line[0] || line[ strlen( line ) - 1 ] != '\n' ) {
    if ( feof( fp ) )
      lf_error( "end-of-file while reading header" );
    else
      lf_error( "error while reading header" );
    return 2;
  }

  /* Check for required fields. */
  if ( !header->hdr_type || !header->station || !header->channel ||
       !header->data_type || isnan( header->hdr_version ) ||
       isnan( header->dim1_start ) || isnan( header->dim1_span ) ||
       isnan( header->dim2_start ) || isnan( header->dim2_span ) ) {
    lf_warning( "required metadata missing" );
    warn = 1;
  }
  if ( header->hdr_type && strcmp( header->hdr_type, "LoFASM-filterbank" ) ) {
    lf_warning( "trying to read non-filterbank data" );
    warn = 1;
  }

  /* Line should now store LFB_DMAX dimensions, and encoding.  Legacy
     files may have fewer dimensions, but must have at least one (the
     bit depth).  In this case, the last dimension read is assumed to
     be the bit depth: it is moved to the end of the list, and any
     unfilled dimensions are set explicitly to 1. */
  memset( header->dims, 0, LFB_DMAX*sizeof(int) );
  for ( i = 0, str = line; i < LFB_DMAX; i++, str = tail ) {
    d = strtoull( str, &tail, 10 );
    if ( tail == str )
      break;
    else if ( d < 1 || d > INT64_MAX ) {
      lf_error( "dimension[%d] = %llu out of range", (int)( i ), d );
      return 2;
    } else
      header->dims[i] = d;
  }
  if ( i < 1 ) {
    lf_error( "no dimensions" );
    return 2;
  } else if ( i < LFB_DMAX ) {
    lf_warning( "%d missing dimensions; setting to 1", LFB_DMAX - (int)( i ) );
    warn = 1;
    header->dims[LFB_DMAX-1] = header->dims[i-1];
    for ( n = i - 1; n < LFB_DMAX - 1; n++ )
      header->dims[n] = 1;
  }

  /* Check type depth. */
  if ( ( str = header->data_type ) ) {
    while ( !isdigit( (int)( *str ) ) )
      str++;
    if ( strtoll( str, NULL, 10 ) != header->dims[LFB_DMAX-1] )
      lf_warning( "bit depth does not match data_type" );
  }

  /* Check encoding. */
  while ( isspace( (int)( *tail ) ) )
    tail++;
  if ( strcmp( tail, "raw256\n" ) ) {
    lf_error( "bad encoding (expecting raw256)" );
    return 2;
  }

  /* Check total size of array. */
  for ( i = 0, n = 1; i < LFB_DMAX; i++, n *= header->dims[i] )
    if ( n > INT64_MAX/header->dims[i] ) {
      lf_error( "number of bits exceeds INT64_MAX" );
      return 2;
    }

  /* Allocate and fill data if requested. */
  if ( data ) {
    n = ( n%8 ? n/8 + 1 : n/8 );
    if ( posix_memalign( data, 8, n ) ) {
      lf_error( "could not allocate data pointer" );
      return 2;
    }
    memset( *data, 0, n );
    if ( ( i = fread( *data, 1, n, fp ) ) < n )
      lf_warning( "read %lld bytes, expected %lld", (long long)( i ),
		  (long long)( n ) );
  }

  /* Finished. */
  return 0;
}

/*
<MARKDOWN>
# lfbxWrite(3)

## NAME

`lfbxWrite(3)` - write a BBX file with LoFASM filterbank header

## SYNOPSIS

`#include "lofasmIO.h"`

`int lfbxWrite( FILE *`_fp_`, lfb_hdr *`_header_`, void *`_data_ ` );`

## DESCRIPTION

This function writes a standard LoFASM data file: a BBX file with
specific header fields taken from a `lfb_hdr` structure.  The _fp_
argument should point to the start of an open binary-writable file,
and the _header_ should point to the structure to be populated; both
must be non-NULL.  See the lofasm-filterbak(5) documentation for more
information about the LoFASM filterbank file format.

If _data_ is NULL, then the writing stops at the end of the header,
allowing the calling routine or other subroutine to write the actual
data block.  This is the normal mode of operation, since a program
will often process data in chunks (e.g. by row), and may not have the
entire dataset loaded into memory at once.  However, if _data_ is not
NULL, then it should point to a block of memory of the size specified
by _header_`->dims`: that is, a number of bytes equal to the product
of _header_`->dimv[0]` through _header_`->dimv[LFB_DMAX]`, divided by
8, rounded up.  These bytes will be written directly to the output
file (`raw256` encoding).

If _header_ contains string fields that are NULL, or floating-point
numbers that are NaN, these fields will be *omitted* from the written
header.  When the file is read by lfbxRead(3), these fields will be
appropriately set to NULL or NaN.

## RETURN VALUE

The function returns 0 normally, but may issue a *warning* if a
required or expected header field is missing or set to an unexpected
value.  It returns nonzero, and issues an *error*, if a write
operation failed, if _fp_ or _header_ is NULL, or if _data_`->dims`
does not specify a positive length of data.

## SEE ALSO

lfbxRead(3),
bbx(5),
lofasm-filterbank(5)

</MARKDOWN> */
int
lfbxWrite( FILE *fp, lfb_hdr *header, void *data )
{
  int64_t i, n;      /* index and data length. */
  char *str;         /* pointer to header string */
  int err, warn = 0; /* error and warning flags */

  /* Check arguments. */
  if ( !fp || !header ) {
    if ( !fp )
      lf_error( "null file pointer" );
    if ( !header )
      lf_error( "null header" );
    return 3;
  }
  for ( n = 1, i = 0; i < LFB_DMAX; n *= header->dims[i], i++ )
    if ( header->dims[i] <= 0 ) {
      lf_error( "non-positive dimension dims[%d]=%lld", (int)( i ),
		(long long)( header->dims[i] ) );
      return 3;
    } else if ( n > INT64_MAX/header->dims[i] ) {
      lf_error( "number of bits exceeds INT64_MAX" );
      return 3;
    }

  /* Check required fields. */
  if ( !header->hdr_type || !header->station || !header->channel ||
       !header->data_type || isnan( header->hdr_version ) ||
       isnan( header->start_mjd )  ||
       isnan( header->dim1_start ) || isnan( header->dim1_span ) ||
       isnan( header->dim2_start ) || isnan( header->dim2_span ) ) {
    lf_warning( "required metadata missing" );
    warn = 1;
  }
  if ( header->hdr_type && strcmp( header->hdr_type, "LoFASM-filterbank" ) ) {
    lf_warning( "trying to write non-filterbank data" );
    warn = 1;
  }

  /* Check type depth. */
  if ( ( str = header->data_type ) ) {
    while ( !isdigit( (int)( *str ) ) )
      str++;
    if ( strtoll( str, NULL, 10 ) != header->dims[LFB_DMAX-1] )
      lf_warning( "bit depth does not match data_type" );
  }

  /* Write header comment fields. */
  err = ( fprintf( fp, "%%\002BX\n" ) < 1 );
  if ( !err && header->hdr_type )
    err = ( fprintf( fp, "%%hdr_type: %s\n", header->hdr_type ) < 1 );
  if ( !err && !isnan( header->hdr_version ) ) {
    float f = header->hdr_version; /* version as float */
    if ( f < 1.0 || f > 255.0 || f != floor( f ) ) {
      lf_warning( "header version has wrong endianness" );
      warn = 1;
    } else if ( f != LFB_VERSION ) {
      lf_warning( "writing version %.0f file with version %d"
		  " library", f, LFB_VERSION );
      warn = 1;
    }
    err = ( fprintf( fp, "%%hdr_version: %.0f\n", f) < 1 );
  }
  if ( !err && header->station )
    err = ( fprintf( fp, "%%station: %s\n", header->station ) < 1 );
  if ( !err && header->channel )
    err = ( fprintf( fp, "%%channel: %s\n", header->channel ) < 1 );
  if ( !err && header->start_time )
    err = ( fprintf( fp, "%%start_time: %s\n", header->start_time ) < 1 );
  if ( !err && !isnan( header->time_offset_J2000 ) ) {
    if ( header->time_offset_J2000 == 0 )
      err = ( fprintf( fp, "%%time_offset_J2000: 0 (s)\n" ) < 1 );
    else
      err = ( fprintf( fp, "%%time_offset_J2000: %.16e (s)\n",
		       header->time_offset_J2000 ) < 1 );
  }
  if ( !err && !isnan( header->frequency_offset_DC ) ) {
    if ( header->frequency_offset_DC == 0 )
      err = ( fprintf( fp, "%%frequency_offset_DC: 0 (Hz)\n" ) < 1 );
    else
      err = ( fprintf( fp, "%%frequency_offset_DC: %.16e (Hz)\n",
		       header->frequency_offset_DC ) < 1 );
  }
  if ( !err && header->start_mjd )
    err = ( fprintf( fp, "%%start_mjd: %.16e\n", header->start_mjd ) < 1 );
  if ( !err && header->dim1_label )
    err = ( fprintf( fp, "%%dim1_label: %s\n", header->dim1_label ) < 1 );
  if ( !err && !isnan( header->dim1_start ) )
    err = ( fprintf( fp, "%%dim1_start: %.16e\n", header->dim1_start ) < 1 );
  if ( !err && !isnan( header->dim1_span ) )
    err = ( fprintf( fp, "%%dim1_span: %.16e\n", header->dim1_span ) < 1 );
  if ( !err && header->dim2_label )
    err = ( fprintf( fp, "%%dim2_label: %s\n", header->dim2_label ) < 1 );
  if ( !err && !isnan( header->dim2_start ) )
    err = ( fprintf( fp, "%%dim2_start: %.16e\n", header->dim2_start ) < 1 );
  if ( !err && !isnan( header->dim2_span ) )
    err = ( fprintf( fp, "%%dim2_span: %.16e\n", header->dim2_span ) < 1 );
  if ( !err && header->data_label )
    err = ( fprintf( fp, "%%data_label: %s\n", header->data_label ) < 1 );
  if ( !err && !isnan( header->data_offset ) )
    err = ( fprintf( fp, "%%data_offset: %.16e\n", header->data_offset ) < 1 );
  if ( !err && !isnan( header->data_scale ) )
    err = ( fprintf( fp, "%%data_scale: %.16e\n", header->data_scale ) < 1 );
  if ( !err && header->data_type )
    err = ( fprintf( fp, "%%data_type: %s\n", header->data_type ) < 1 );

  /* Write dimensions and encoding. */
  for ( i = 0; !err && i < LFB_DMAX; i++ )
    err = ( header->dims[i] < 1 ||
	    fprintf( fp, "%lld ", (long long)( header->dims[i] ) ) < 1 );
  err = err || ( fprintf( fp, "raw256\n" ) < 1 );

  /* Write data, if requested. */
  n = ( n%8 ? n/8 + 1 : n/8 );
  if ( !err && data )
    err = ( fwrite( data, 1, n, fp ) < n );

  /* Finished. */
  if ( err ) {
    lf_error( "write error" );
    return 2;
  }
  return 0;
}

/*
<MARKDOWN>
# lfbxFree(3)

## NAME

`lfbxFree(3)` - frees a LoFASM filterbank header

## SYNOPSIS

`#include "lofasmIO.h"`

`int lfbxFree( lfb_hdr *`_header_ ` );`

## DESCRIPTION

This function frees a `lfb_hdr` structure allocated and populated by
(for example) lfbxRead(3).  If you have manually replaced any of the
pointer fields with pointers to static memory, then errors will occur
attempting to free them, so be careful to set these fields to NULL
before calling lfbxFree().

## RETURN VALUE

This function does not return a value.

## SEE ALSO

lfbxRead(3),
lofasm-filterbank(5)

</MARKDOWN> */
void
lfbxFree( lfb_hdr *header )
{
  if ( header->hdr_type )
    free( header->hdr_type );
  if ( header->station )
    free( header->station );
  if ( header->channel )
    free( header->channel );
  if ( header->start_time )
    free( header->start_time );
  if ( header->dim1_label )
    free( header->dim1_label );
  if ( header->dim2_label )
    free( header->dim2_label );
  if ( header->data_label )
    free( header->data_label );
  if ( header->data_type )
    free( header->data_type );
  memset( header, 0, sizeof(lfb_hdr) );
  header->time_offset_J2000 = header->frequency_offset_DC =
    header->dim1_start = header->dim1_span =
    header->dim2_start = header->dim2_span =
    header->data_offset = header->data_scale = strtod( "nan", 0 );
  return;
}
