static const char *version = "\
md2man version 1.1\n\
Copyright (c) 2016 Teviet Creighton.\n\
\n\
Based on peg-markdown version 0.4.14,\n\
Copyright (c) 2008-2009 John MacFarlane.\n\
\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License or the MIT\n\
license.  See LICENSE for details.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n\
General Public License for more details.\n\
\n";

static const char *usage = "\
Usage: %s [OPTION]... [INFILE]... [> OUTFILE]\n\
Convert markdown document(s) to Unix manpage(s).\n\
\n\
  -h, --help         print this usage information\n\
  -H, --man          display the program's man page\n\
      --manpage      print the program's man page (groff)\n\
      --markdown     print the program's man page (markdown)\n\
  -V, --version      print version and copyright information\n\
  -o, --output=OUT   print individual manpages to directory OUT\n\
  -b, --begin=BTAG   start parsing after line beginning with BTAG\n\
  -e, --end=ETAG     stop parsing before line beginning with ETAG\n\
  -q, --quit=QTAG    terminates all parsing at line beginning with QTAG\n\
  -i, --insert=ITAG  insert file at line beginning with ITAG\n\
  -s, --strip=L[+R]  strip L and R characters from start and end of line\n\
  -p, --preprocess   do extraction and stripping, but don't convert\n\
\n";

static const char *description = "\
# md2man(1) 2016-07-11\n\
\n\
## NAME\n\
\n\
`md2man(1)` - convert markdown to Unix manpage\n\
\n\
## SYNOPSIS\n\
\n\
`md2man` [_OPTION_]... [_INFILE_]... [`>` _OUTFILE_]\n\
\n\
## DESCRIPTION\n\
\n\
This program reads markdown-formatted (.md) input, and writes output\n\
in the groff_man(7) format used for Unix manpages.  Zero or more input\n\
files are specified on the command line, with multiple input files\n\
being concatenated; if none are specified, the program reads from\n\
standard input.  An input filename of `-` also indicates standard\n\
input.  The formatted manpage is written to standard output (which\n\
will usually be piped to less(1) or redirected to _OUTFILE_).  The\n\
`--directory` option will instead write each top-level document\n\
section to its own file; see below.\n\
\n\
The program is essentially a front end to the markdown_to_manpage(3)\n\
function, an adaptation of John MacFarlane's PEG-markdown parser.  See\n\
that function's documentation for details.\n\
\n\
## OPTIONS\n\
\n\
`-h, --help`:\n\
    Prints basic usage information to `stdout` and exits.\n\
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
    Prints version and copyright information and exits.\n\
\n\
`-o, --output=`_OUT_:\n\
    Instead of collecting all output into the standard output stream,\n\
    this option writes each block beginning with a top-level header to\n\
    its own manpage file in the directory given by _OUT_ (which must\n\
    already exist).  Headers should begin with `#`_topic_`(`_N_`)`,\n\
    which will write to the file _topic_`.`_N_ in the specified\n\
    directory.\n\
\n\
    If _OUT_ begins with a `|` (pipe) character, then the subsequent\n\
    string will instead be a command to which the collected output\n\
    will be piped: there may be circumstances where this is preferred\n\
    over simply redirecting the output of `md2man`.\n\
\n\
    Examples: `\"\"` or `\".\"` will write to separate files in the\n\
    current directory.  `\"|\"` will write collected output to`stdout`\n\
    (same as if this option weren't given).  `\"./|\"` will write\n\
    separate files to a directory named `|`.\n\
\n\
`-b, --begin=`_BTAG_:\n\
    Starts parsing only after a line that begins with the string\n\
    _BTAG_.  The line containing the tag itself is not parsed (unless\n\
    parsing had already started due to another instance of _BTAG_).\n\
\n\
    In combination with the `--end` option, below, this allows you to\n\
    embed markdown documentation blocks within other files, such as a\n\
    code source files.\n\
\n\
`-e, --end=`_ETAG_:\n\
    Stops parsing at a line that begins with the string _ETAG_.  The\n\
    line containing the tag itself is not parsed.  The file will\n\
    continue to be read, and if a tag specified by the `--begin`\n\
    option (above) is encountered, parsing will resume.\n\
\n\
    If the same tag is specified for both the `--begin` and `--end`\n\
    options, then the tag toggles parsing on and off (starting with\n\
    off).\n\
\n\
`-q, --quit=`_QTAG_:\n\
    Stops reading input at a line that begins with the string _QTAG_,\n\
    and parses only the text read up to that point.  Unlike the\n\
    `--end` option, the program will read no further input even if\n\
    there are subsequent `--begin` tags.  If parsing was underway when\n\
    _QTAG_ was encountered, the line beginning with that tag is not\n\
    parsed.\n\
\n\
`-i, --insert=`_ITAG_:\n\
    If a line begins with _ITAG_, take the token following _ITAG_ as\n\
    the name of a file.  Open and parse that file before continuing\n\
    with the current stream.  The effect is equivalent to inserting\n\
    the contents of the named file into the current stream: in\n\
    particular, the file may itself contain occurences of _BTAG_,\n\
    _ETAG_, _QTAG_ (above) or another _ITAG_.  A global list of\n\
    filenames is maintained to guard against circular inclusions.\n\
\n\
    The token (filename) after _ITAG_ ends at the first whitespace,\n\
    end of line, or end of input.  A backslash character `\\` can be\n\
    used to escape whitespace: any character following the `\\`,\n\
    except for a newline or nul byte, is included literally in the\n\
    filename.  Thus, for instance, `\\\\` is a literal backslash.\n\
\n\
`-s, --strip=`_L_[`+`_R_]:\n\
    Removes _L_ characters from the start, and, optionally, _R_\n\
    characters from the end, of each line parsed.  The two (positive)\n\
    integers should be delimited by a `+` character, interpreted as\n\
    the sign of the second number.\n\
\n\
    This option is particularly useful when embedding documentation\n\
    within source files with the `--begin` and `--end` options, above.\n\
    For example, shell script comments may require a `#` character at\n\
    the start of each line; a block of markdown documentation can be\n\
    accomodated with `--strip=1` (or `2` if a single leading space is\n\
    also to be skipped).  For another example, if you comment C files\n\
    with blocks always of the form\n\
\n\
>>    `/* ` _comment_ ` */`\n\
\n\
:\n\
    then `--strip=3+3` will skip over both comment delimiters (plus\n\
    the single space of padding).\n\
\n\
    Note that the `--begin`, `--end`, `--quit`, and `--insert` options\n\
    specify tags that appear at the beginning of input lines; the\n\
    beginning is computed after stripping _L_ characters (to allow\n\
    these tags to appear inside comment blocks).\n\
\n\
`-p, --preprocess`:\n\
    Write a preprocessed markdown file.  That is, perform any\n\
    concatenation of the input files, along with any processing\n\
    specified by the `--begin`, `--end`, `--quit`, `--insert`, and\n\
    `--strip` options, but do not perform the actual conversion to\n\
    manpage format.  This option ignores the `--directory` option (the\n\
    text is not parsed to the point of identifying markdown headers).\n\
\n\
## EXIT STATUS\n\
\n\
The program returns 0, or 1 if it could not parse a command-line\n\
option or open a file, or 2 on a memory error.\n\
\n\
## SEE ALSO\n\
\n\
markdown_to_manpage(3),\n\
markdown_to_manterm(3),\n\
charvector(5)\n\
\n";

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include "markdown_parser.h"
#include "charvector.h"

static const char short_opts[] = "hHVo:b:e:q:i:s:p";
static const struct option long_opts[] = {
  { "help", 0, 0, 'h' },
  { "man", 0, 0, 'H' },
  { "manpage", 0, 0, 0 },
  { "markdown", 0, 0, 0 },
  { "version", 0, 0, 'V' },
  { "output", 1, 0, 'o' },
  { "begin", 1, 0, 'b' },
  { "end", 1, 0, 'e' },
  { "quit", 1, 0, 'q' },
  { "insert", 1, 0, 'i' },
  { "strip", 1, 0, 's' },
  { "preprocess", 0, 0, 'p' },
  { 0, 0, 0, 0} };

#define LEN 1024 /* character buffer size */

/* Global list of nested input files. */
#define INDEPTH 64
static const char *inlist[INDEPTH] = {};
static int nin = 0;
static char *prog;

/* Global variables for parsing/skipping/inserting text. */
static int parse = 1;       /* whether to parse current section */
static char *begin = NULL;  /* tag to start parsing */
static char *end = NULL;    /* tag to stop parsing */
static char *quit = NULL;   /* tag to terminate all further parsing */
static char *insert = NULL; /* tag to insert a file */
static int l = 0, r = 0;    /* characters to strip from left and right */

int
loadFile( charvector *text, const char *fname )
{
  char buf[LEN];   /* file input buffer */
  char *a, *b, *c; /* pointers to buf or cv->str */
  FILE *fp;        /* input file pointer */
  int i;           /* index over file list/trim characters */

  /* Test fname against stack of included files, and open it. */
  if ( !text || !fname ) {
    fprintf( stderr, "%s: null pointer passed to loadFile\n", prog );
    return 1;
  }
  if ( !fname[0] ) {
    fprintf( stderr, "%s: empty name passed to loadFile\n", prog );
    return 1;
  }
  if ( nin >= INDEPTH ) {
    fprintf( stderr, "%s: reach maxumum %d nested includes; skipping %s",
	     prog, nin, fname );
    return 1;
  }
  for ( i = 0; i < nin; i++ )
    if ( !strcmp( fname, inlist[i] ) ) {
      fprintf( stderr, "%s: circular include; skipping second instance of %s\n",
	       prog, fname );
      return 1;
    }
  if ( !strcmp( fname, "-" ) )
    fp = stdin;
  else if ( !( fp = fopen( fname, "r" ) ) ) {
    fprintf( stderr, "%s: could not open %s; skipping\n", prog, fname );
    return 1;
  }
  inlist[nin++] = fname;

  /* Loop over lines of input. */
  do {
    /* Check if line starts with an instruction tag. */
    a = fgets( buf, LEN, fp );
    if ( !a )
      break;
    if ( strlen( a ) < l )
      continue;
    if ( quit && !strncmp( buf + l, quit, strlen( quit ) ) )
      break;
    if ( insert && !strncmp( buf + l, insert, strlen( insert ) ) ) {
      a += strlen( insert ) + l;
      while ( isspace( (int)( *a ) ) )
	a++;
      for ( b = c = a; *b && !isspace( (int)( *b ) ); b++, c++ ) {
	if ( *b == '\\' )
	  b++;
	if ( !( *c = *b ) )
	  break;
      }
      *c = '\0';
      if ( loadFile( text, a ) > 1 )
	return 2;
    }
    else if ( ( !parse && begin && !strncmp( buf, begin, strlen( begin ) ) ) ||
	      ( parse && end && !strncmp( buf, end, strlen( end ) ) ) ) {
      parse = !parse;
      while ( a && buf[ strlen( buf ) - 1 ] != '\n' )
	a = fgets( buf, LEN, fp );
    } else if ( !parse ) {
      while ( a && buf[ strlen( buf ) - 1 ] != '\n' )
	a = fgets( buf, LEN, fp );
    }
    /* Copy line into text, stripping first l and last r characters. */
    else {
      int start = 1; /* whether this is start of line */
      do {
	if ( start && strlen( buf ) <= r + l )
	  b = charvector_append_c( text, '\n' );
	else {
	  b = charvector_append( text, buf + ( start ? l : 0 ) );
	  if ( b && r && buf[ strlen( buf ) - 1 ] == '\n' ) {
	    for ( i = 0, b = text->str + strlen( text->str ) - 1; i < r ;
		  i++, b-- )
	      b[0] = '\0';
	    b[0] = '\n';
	  }
	}
	if ( !b ) {
	  fprintf( stderr, "%s: memory error\n", prog );
	  charvector_free( text );
	  if ( fp != stdin )
	    fclose( fp );
	  nin--;
	  return 2;
	}
	start = 0;
      } while ( buf[ strlen( buf ) - 1 ] != '\n' &&
		( a = fgets( buf, LEN, fp ) ) );
    }
  } while ( a );

  /* Finished with this file. */
  if ( fp != stdin )
    fclose( fp );
  nin--;
  return 0;
}


int
main( int argc, char **argv )
{
  int opt, lopt;        /* option character and index */
  int pre = 0;          /* whether to write preprocessed markdown */
  char *out = NULL;     /* output directory or pipe */
  charvector text = {}; /* input file as variable-length string */
  int flag = 0;         /* return code or "no arguments" flag */

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
    case 'o':
      out = optarg;
      break;
    case 'b':
      begin = optarg;
      parse = 0;
      break;
    case 'e':
      end = optarg;
      break;
    case 'q':
      quit = optarg;
      break;
    case 'i':
      insert = optarg;
      break;
    case 's':
      if ( sscanf( optarg, "%d%d", &l, &r ) < 1 ) {
	fprintf( stderr, "%s: could not read argument to -s,--strip"
		 " option\n", argv[0] );
	return 1;
      } else if ( l + r >= LEN ) {
	fprintf( stderr, "%s: total characters to strip must be less"
		 " than input buffer %d\n", argv[0], LEN );
	return 1;
      } else if ( l < 0 || r < 0 ) {
	fprintf( stderr, "%s: argument(s) to -s,--strip must be"
		 " positive\n", argv[0] );
	return 1;
      }
      break;
    case 'p':
      pre = 1;
      break;
    case '?': case ':':
      fprintf( stderr, "Try %s --help for more information\n", argv[0] );
      return 1;
    default:
      fprintf( stderr, "%s: internal error parsing option code %c\n"
	       "Try %s --help for more information\n", argv[0],
	       opt, argv[0] );
      return 1;
    }
  }

  /* Read files from command line, or stdin if there are no
     command-line arguments (set by flag). */
  prog = argv[0];
  for ( flag = ( optind >= argc ); flag || optind < argc;
	flag = 0, optind++ )
    loadFile( &text, ( flag ? "-" : argv[optind] ) );

  /* Parse and write results. */
  if ( pre )
    flag = fputs( text.str, stdout );
  else
    flag = markdown_to_manpage( text.str, out );
  charvector_free( &text );
  if ( flag )
    fprintf( stderr, "%s: write error\n", argv[0] );
  return flag;
}
