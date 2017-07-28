// 2>&-### SELF-EXTRACTING DOCUMENTATION ###############################
// 2>&-#                                                               #
// 2>&-# Run "bash <thisfile> > <docfile.md>" to extract documentation #
// 2>&-#                                                               #
// 2>&-#################################################################
// 2>&-; awk '/^<\/MARKDOWN>/{f=0};f;/^<MARKDOWN>/{f=1}' $0; exit 0

/***********************************************************************
charvector.h
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

#ifndef _CHARVECTOR_H
#define _CHARVECTOR_H
#ifdef  __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define CHARVECTOR_BLOCKSIZE 1024

/*
<MARKDOWN>
# charvector(3), charvector_append(3), charvector_append_c(3), charvector_append_vprintf(3), charvector_append_printf(3), charvector_insert(3), charvector_replace(3), charvector_pad(3), charvector_trim(3), charvector_resize(3), charvector_free(3) 2016-07-13

## NAME

`charvector(3)` - a resizeable string structure

## SYNOPSIS

`#include "charvector.h"`

`char *charvector_append( charvector *`_cv_`, const char *`_src_ `);`

`char *charvector_append_c( charvector *`_cv_`, char` _c_ `);`

`char *charvector_append_vprintf( charvector *`_cv_`, const char *`_format_`,
                                 va_list` _list_ `);`

`char *charvector_append_printf( charvector *`_cv_`, const char *`_format_`,
                                ... );`

`char *charvector_insert( charvector *`_cv_`, const char *`_src_`,
                         size_t` _offset_ `);`

`char *charvector_replace( charvector *`_cv_`, const char *`_find_`,
                          const char *`_replace_ `);`

`char *charvector_pad( charvector *`_cv_ `);`

`char *charvector_trim( charvector *`_cv_ `);`

`char *charvector_resize( charvector *`_cv_`, size_t` _size_`, int` _truncate_ `);`

`void charvector_free( charvector *`_cv_ `);`

## DESCRIPTION

These routines manipulate charvectors: structures containing a
dynamically-allocated character array along with its allocated size.
The definition of a charvector is:

    typedef struct {
        char *str;
        size_t size;
    } charvector;

</MARKDOWN> */
typedef struct {
  char *str;
  size_t size;
} charvector;
/*
<MARKDOWN>

There is no explicit function to initialize a charvector.  Simply
initialize it to an empty structure and start appending to it; the
various charvector routines will (re)allocate space as necessary.

    charvector cv = {};
    charvector_append( &cv, "Hello, world!" );

A charvector is intended as a transparent data type.  In particular,
the `str` pointer may be passed and manipulated as an ordinary string
outside the scope of its original charvector structure.  However, if
the charvector fields are to be set directly, the user must ensure
that:

  1. The `str` element is an allocated freeable pointer (or NULL).

  2. The `size` element is not greater than the allocated size.

  3. The value of `str`[`size`-1] is `\0`.

The routines in `charvector.h` will usually test the third condition
to ensure that the string is in fact nul-terminated.  When allocating
new memory, these routines will also pad all allocated space following
the nul terminator of `str` with nul bytes: this is a security feature
to prevent random system memory from appearing in the array.  You can
apply such padding to user-supplied strings using the charvector_pad()
function.

When a charvector function needs to increase the allocated size of the
character array, it will generally increase it to a power of 2 times a
basic size stored in the `#define`d constant `CHARVECTOR_BLOCKSIZE`.
This means that the allocated size will typically be no more than 2
times what is needed, while the number of allocations required to
build a large string incrementally scales logarithmically with its
size.  If necessary, charvector_trim() can be called to remove excess
padding.

### Charvectors Are Not Gstrings

The `GString` structure and functions defined in `glib.h` provide much
greater functionality than charvectors, but also constitute a
substantial non-standard external dependency on any program using
them.  By contrast, charvectors are a lightweight substitute, with
only one module and header file, meant to be included within other
projects rather than installed separately.

### Charvector Functions

charvector_append() and charvector_append_c() add a nul-terminated
string _src_ or a single character _c_ to the end of _cv_`->str`,
reallocating _cv_`->str` and increasing _cv_`->size` as needed.

charvector_append_vprintf() and charvector_append_printf() produce
formatted output in the style of printf(3), according to the specified
_format_ and subsequent argument(s), and append it to the end of
_cv_`->str`: charvector_append_printf() sets up a `va_list` object to
handle its variable argument list and then calls
charvector_append_vprintf(), and this function in turn calls
vasprintf(3) to generate the output string.  See printf(3) for more on
the _format_ specification and arguments.

charvector_insert() inserts a nul-terminated string _src_ into
_cv_`->str` starting at byte _offset_.  The trailing portion of
_cv_`->str` is not overwritten but pushed back to follow the inserted
string.  The value of _offset_ must be less than or equal to than the
original length of _cv_`->str` (equality simply appends _src_ to
_cv_`->str`); otherwise an error is returned.

charvector_replace() searches _cv_`->str` for all occurrences of the
string _find_, replacing each in turn with the string _replace_.  Both
_find_ and _replace_ must be nul-terminated strings.  As above,
_cv_`->str` will be reallocated as needed, though this won't occur if
_replace_ is the same length as _find_ or shorter.

charvector_trim() reallocates _cv_`->str` down to size
`strlen(`_cv_`->str)+1`, the minimum needed to store the string and
its nul terminator.  If the size is already this minimum, then it does
nothing.  This step should generally be done only at the end of all
charvector manipulaton (if at all); e.g. before shipping _cv_`->str`
out to a routine that will treat it as a fixed-length string.

charvector_pad() sets to zero all elements of _cv_`->str` `from
strlen(`_cv_`->str)` (i.e. the first occurence of nul) to
_cv_`->size`-1.  This is primarily useful for sanitizing user input;
all charvector routines that allocate space for the string will do
such padding by default.

charvector_resize() reallocates _cv_`->str` to the specified _size_
(setting _cv_`->size` accordingly).  If _cv_`->str` is longer than
_size_-1, the behaviour is governed by the _truncate_ flag: if
_truncate_ is nonzero (true), then the requested _size_ will be
honored, and _cv_`->str` will be shortened and truncated with a nul
byte at index _size_-1; whereas if _truncate_ is zero (false), then
the size will not be less than the length of _cv_`->str` plus its nul
terminator (equivalent to charvector_trim(3)).  If called with _size_
zero and _truncate_ nonzero, this function is equivalent to
charvector_free().

charvector_free() simply deallocates _cv_`->str` (if non-NULL), and
sets `*`_cv_ to an empty structure.  It is mostly a convenience: there
is no implied requirement that charvectors be freed with this
function.  In fact routines that use charvectore may well return just
the `str` pointer, which must then be deallocated with free(3) outside
the scope of the original charvector structure.

## RETURN VALUE

charvector_free() does not return a value.  All other functions return
the pointer _cv_`->str` under normal conditions.  If internal memory
allocation failed, or if passed bad arguments, they will return NULL
instead.

## EXAMPLE

Compile the following test program with `gcc -Wall -D_GNU_SOURCE` and
run the result.  (The `-D` flag is needed to make the declaration of
vasprintf(3) visible in `stdio.h`.)

    #include <stdio.h>
    #include <stdlib.h>
    #include "charvector.h"

    int main( int argv, char *argv[] )
    {
        char *c = NULL;
        charvector cv = {};
        c = charvector_append( &cv, "Hello, world!" );
        c = c && charvector_append_printf( &cv,
            " You're %f years old.", 4.5e9 );
        c = c && charvector_append_c( &cv, '\n' );
        c = c && charvector_insert( &cv, " Happy birthday!", 13 );
        c = c && charvector_replace( &cv, "world", "Jupiter" );
        if ( c )
            fputs( c, stdout );
        charvector_free( &cv );
        exit( c ? EXIT_SUCCESS : EXIT_FAILURE );
    }

## SEE ALSO

vasprintf(3)

</MARKDOWN> */
char *charvector_append( charvector *cv, const char *src );
char *charvector_append_c( charvector *cv, char c );
char *charvector_append_vprintf( charvector *cv, const char *format,
				 va_list list );
char *charvector_append_printf( charvector *cv, const char *format, ... );
char *charvector_insert( charvector *cv, const char *src, size_t offset );
char *charvector_replace( charvector *cv, const char *find,
			  const char *replace );
char *charvector_pad( charvector *cv );
char *charvector_trim( charvector *cv );
char *charvector_resize( charvector *cv, size_t size, int truncate );
void charvector_free( charvector *cv );

#ifdef  __cplusplus
#if 0
{
#endif
}
#endif
#endif
