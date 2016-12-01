// 2>&-### SELF-EXTRACTING DOCUMENTATION ###############################
// 2>&-#                                                               #
// 2>&-# Run "bash <thisfile> > <docfile.md>" to extract documentation #
// 2>&-#                                                               #
// 2>&-#################################################################
// 2>&-; awk '/^<\/MARKDOWN>/{f=0};f;/^<MARKDOWN>/{f=1}' $0; exit 0

/***********************************************************************
charvector.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "charvector.h"

/* Insert src string into cv->str at position offset.  Return cv->str
   or NULL on error. */
char *
charvector_insert( charvector *cv, const char *src, size_t offset )
{
  int n, m; /* lengths of cv->str and src */

  /* Do error checking and get lengths. */
  if ( !cv || ( cv->str && cv->str[cv->size-1] ) )
    return NULL;
  n = ( cv->str ? strlen( cv->str ) : 0 );
  if ( offset > n )
    return NULL;
  if ( !src || !src[0] )
    return cv->str;
  m = strlen( src );

  /* Reallocate space if needed. */
  if ( n + m + 1 > cv->size ) {
    size_t s = CHARVECTOR_BLOCKSIZE;
    char *tmp;
    while ( s < n + m + 1 )
      s *= 2;
    if ( !( tmp = (char *)( calloc( s, 1 ) ) ) )
      return NULL;
    if ( offset > 0 )
      memcpy( tmp, cv->str, offset );
    memcpy( tmp + offset, src, m );
    if ( n - offset > 0 )
      memcpy( tmp + offset + m, cv->str + offset, n - offset );
    if ( cv->str )
      free( cv->str );
    cv->str = tmp;
    cv->size = s;
    return tmp;
  }

  /* Otherwise insert in-place. */
  if ( n - offset > 0 )
    memmove( cv->str + offset + m, cv->str + offset, n - offset );
  memcpy( cv->str + offset, src, m );
  return cv->str;
}


/* Append src string to end of cv->str.  Return cv->str or NULL on
   error. */
char *
charvector_append( charvector *cv, const char *src )
{
  int n, m; /* lengths of cv->str and src */

  /* Do error checking and get lengths. */
  if ( !cv || ( cv->str && cv->str[cv->size-1] ) )
    return NULL;
  n = ( cv->str ? strlen( cv->str ) : 0 );
  if ( !src || !src[0] )
    return cv->str;
  m = strlen( src );

  /* Reallocate space if needed. */
  if ( n + m + 1 > cv->size ) {
    size_t s = CHARVECTOR_BLOCKSIZE;
    char *tmp;
    while ( s < n + m + 1 )
      s *= 2;
    if ( !( tmp = (char *)( calloc( s, 1 ) ) ) )
      return NULL;
    memcpy( tmp, cv->str, n );
    memcpy( tmp + n, src, m );
    if ( cv->str )
      free( cv->str );
    cv->str = tmp;
    cv->size = s;
    return tmp;
  }

  /* Otherwise insert in-place. */
  memcpy( cv->str + n, src, m );
  return cv->str;
}


/* Append single character c to end of cv->str.  Return cv->str or
   NULL on error. */
char *
charvector_append_c( charvector *cv, char c )
{
  int n; /* length of cv->str */

  /* Do error checking and get lengths. */
  if ( !cv || ( cv->str && cv->str[cv->size-1] ) )
    return NULL;
  n = ( cv->str ? strlen( cv->str ) : 0 );

  /* Reallocate space if needed. */
  if ( n + 2 > cv->size ) {
    size_t s = CHARVECTOR_BLOCKSIZE;
    char *tmp;
    while ( s < n + 2 )
      s *= 2;
    if ( !( tmp = (char *)( calloc( s, 1 ) ) ) )
      return NULL;
    memcpy( tmp, cv->str, n );
    tmp[n] = c;
    if ( cv->str )
      free( cv->str );
    cv->str = tmp;
    cv->size = s;
    return tmp;
  }

  /* Otherwise insert in-place. */
  cv->str[n] = c;
  return cv->str;
}


/* Append printf-formatted output to cv->str, using specified format
   and variable argument object.  Return cv->str or NULL on error.  */
char *
charvector_append_vprintf( charvector *cv, const char *format, va_list list )
{
  int n = vsnprintf( NULL, 0, format, list );
  char *str = (char *)malloc( ( n + 1 )*sizeof(char) );
  if ( !str )
    return NULL;
  vsnprintf( str, n + 1, format, list );
  if ( !charvector_append( cv, str ) ) {
    free( str );
    return NULL;
  }
  free( str );
  return cv->str;
}


/* Append printf-formatted output to cv->str, using specified format
   and variable argument list.  Return cv->str or NULL on error. */
char *
charvector_append_printf( charvector *cv, const char *format, ... )
{
  char *ret;
  va_list list;
  va_start( list, format );
  ret = charvector_append_vprintf( cv, format, list );
  va_end( list );  
  return ret;
}


/* Successively search for find string in cv->str, replacing it with
   replace string, and then continuing to search from the end of the
   replacement.  Returns cv->str or NULL on error. */
char *
charvector_replace( charvector *cv, const char *find, const char *replace )
{
  int l, m, n;       /* lengths of cv->str, find, and replace */
  char *srcp, *cvp; /* pointers within source and destination */
  char *tmp = NULL;  /* temporary storage for reallocated string */
  size_t s = CHARVECTOR_BLOCKSIZE; /* size of reallocated string */

  /* Do error checking and get lengths. */
  if ( !cv || !cv->str || cv->str[cv->size-1] || !find || !find[0] )
    return NULL;
  m = strlen( find );
  n = ( replace ? strlen( replace ) : 0 );

  /* Allocate new destination string if necessary. */
  if ( n > m ) {
    for ( l = 0, srcp = cv->str; *srcp; )
      if ( !strncmp( srcp, find, m ) ) {
	l++;
	srcp += m;
      } else
	srcp++;
    l = strlen( cv->str ) + l*( n - m ) + 1;
    while ( s < l )
      s *= 2;
    if ( !( tmp = (char *)( calloc( s, 1 ) ) ) )
      return NULL;
  }

  /* Do replacement. */
  for ( srcp = cv->str, cvp = ( tmp ? tmp : cv->str ); *srcp; )
    if ( !strncmp( srcp, find, m ) ) {
      if ( replace )
	memcpy( cvp, replace, n );
      srcp += m;
      cvp += n;
    } else
      *( cvp++ ) = *( srcp++ );

  /* Set return values. */
  if ( tmp ) {
    free( cv->str );
    cv->str = tmp;
    cv->size = s;
  } else if ( ( l = cvp - cv->str ) < cv->size - 1 )
    memset( cvp, 0, cv->size - 1 - l );
  return cv->str;
}


/* Zero out the unused portion of cv->str.  Return cv->str or NULL on
   error. */
char *
charvector_pad( charvector *cv )
{
  int n;
  if ( !cv || !cv->str || cv->str[cv->size-1] )
    return NULL;
  if ( ( n = strlen( cv->str ) ) < cv->size - 2 )
    memset( cv->str + n + 1, 0, cv->size - n - 2 );
  return cv->str;
}


/* Deallocate unneded space in cv->str.  Retuen cv->str or NULL on
   error. */
char *
charvector_trim( charvector *cv )
{
  int n;
  if ( !cv || !cv->str || cv->str[cv->size-1] )
    return NULL;
  if ( ( n = strlen( cv->str ) ) > 0 && n < cv->size - 1 ) {
    char *tmp = (char *)realloc( cv->str, n + 1 );
    if ( !tmp )
      return NULL;
    cv->str = tmp;
    cv->size = n + 1;
  } else if ( n == 0 ) {
    free( cv->str );
    memset( cv, 0, sizeof(charvector) );
  }
  return cv->str;
}


/* Set the amount of space allocated to cv->str to size.  If truncate
   is nonzero, force the allocated space to be this size even if that
   means truncating cv->str (it will still have a nul terminator).  If
   truncate is zero, allocated space will not be reduced below
   strlen(cv->str)+1.  Return cv->str or NULL on error. */
char *
charvector_resize( charvector *cv, size_t size, int truncate )
{
  int n;
  if ( !cv || ( cv->str && cv->str[cv->size-1] ) )
    return NULL;
  if ( !cv->str && size > 0 ) {
    cv->str = (char *)calloc( size, 1 );
    cv->size = ( cv->str ? size : 0 );
    return cv->str;
  }
  if ( ( n = strlen( cv->str ) ) > cv->size - 1 && !truncate )
    size = n + 1;
  if ( size > 0 && size != cv->size ) {
    char *tmp = (char *)realloc( cv->str, size );
    if ( !tmp )
      return NULL;
    if ( n < size - 1 )
      memset( tmp + n + 1, 0, size - n - 1 );
    else
      tmp[size-1] = '\0';
    cv->str = tmp;
    cv->size = size;
  } else if ( size == 0 ) {
    free( cv->str );
    memset( cv, 0, sizeof(charvector) );
  }
  return cv->str;
}


/* Deallocate cv->str if needed, and set *cv to be empty.  No return
   value. */
void
charvector_free( charvector *cv )
{
  if ( cv->str )
    free( cv->str );
  memset( cv, 0, sizeof(charvector) );
  return;
}
