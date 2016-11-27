// 2>&-### SELF-EXTRACTING DOCUMENTATION ###############################
// 2>&-#                                                               #
// 2>&-# Run "bash <thisfile> > <docfile.md>" to extract documentation #
// 2>&-#                                                               #
// 2>&-#################################################################
// 2>&-; awk '/^<\MARKDOWN>/{f=0};f;/^<MARKDOWN>/{f=1}' $0; exit 0

/***********************************************************************
markdown_parser.h
Copyright (c) 2008 John MacFarlane (jgm at berkeley dot edu).

Markdown to manpage additions:
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
#ifndef MARKDOWN_PARSER_H
#define MARKDOWN_PARSER_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "charvector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MARKDOWN TO MANPAGE **************************************************/
int markdown_to_manpage( const char *markdown, const char *out );
int markdown_to_manterm( const char *markdown, FILE *fp );
char *markdown_to_man_str( const char *markdown );
int markdown_to_man_out( const char *markdown );
/************************************************************************/

enum markdown_extensions {
    EXT_SMART            = 0x01,
    EXT_NOTES            = 0x02,
    EXT_FILTER_HTML      = 0x04,
    EXT_FILTER_STYLES    = 0x08,
    EXT_STRIKE           = 0x10
};

enum markdown_formats {
    HTML_FORMAT,
    LATEX_FORMAT,
    GROFF_MM_FORMAT,
    ODF_FORMAT
};

void markdown_to_charvector(charvector *out, char *text, int extensions,
			    int output_format);
char * markdown_to_string(char *text, int extensions, int output_format);

#ifdef __cplusplus
}
#endif

/* vim: set ts=4 sw=4 : */


/* Information (label, URL and title) for a link. */
struct MDLink {
    struct Element   *label;
    char             *url;
    char             *title;    
};

typedef struct MDLink mdlink;

/* Union for contents of an Element (string, list, or link). */
union Contents {
    char             *str;
    struct MDLink    *link;
};

/* Types of semantic values returned by parsers. */ 
enum keys { LIST,   /* A generic list of values.  For ordered and bullet lists, see below. */
            RAW,    /* Raw markdown to be processed further */
            SPACE,
            LINEBREAK,
            ELLIPSIS,
            EMDASH,
            ENDASH,
            APOSTROPHE,
            SINGLEQUOTED,
            DOUBLEQUOTED,
            STR,
            LINK,
            IMAGE,
            CODE,
            HTML,
            EMPH,
            STRONG,
            STRIKE,
            PLAIN,
            PARA,
            LISTITEM,
            BULLETLIST,
            ORDEREDLIST,
            H1, H2, H3, H4, H5, H6,  /* Code assumes that these are in order. */
            BLOCKQUOTE,
            VERBATIM,
            HTMLBLOCK,
            HRULE,
            REFERENCE,
            NOTE
          };

/* Semantic value of a parsing action. */
struct Element {
    int               key;
    union Contents    contents;
    struct Element    *children;
    struct Element    *next;
};

typedef struct Element element;

element * parse_references(char *string, int extensions);
element * parse_notes(char *string, int extensions, element *reference_list);
element * parse_markdown(char *string, int extensions, element *reference_list, element *note_list);
void free_element_list(element * elt);
void free_element(element *elt);
void print_element_list(charvector *out, element *elt, int format, int exts);


/* utility_functions.h - List manipulation functions, element
 * constructors, and macro definitions for leg markdown parser. */


/* cons - cons an element onto a list, returning pointer to new head */
element * cons(element *new, element *list);

/* reverse - reverse a list, returning pointer to new list */
element *reverse(element *list);
/* concat_string_list - concatenates string contents of list of STR elements.
 * Frees STR elements as they are added to the concatenation. */
void concat_string_list(charvector *result, element *list);
/**********************************************************************

  Global variables used in parsing

 ***********************************************************************/

extern char *charbuf;          /* Buffer of characters to be parsed. */
extern element *references;    /* List of link references found. */
extern element *notes;         /* List of footnotes found. */
extern element *parse_result;  /* Results of parse. */
extern int syntax_extensions;  /* Syntax extensions selected. */

/**********************************************************************

  Auxiliary functions for parsing actions.
  These make it easier to build up data structures (including lists)
  in the parsing actions.

 ***********************************************************************/

/* mk_element - generic constructor for element */
element * mk_element(int key);

/* mk_str - constructor for STR element */
element * mk_str(char *string);

/* mk_str_from_list - makes STR element by concatenating a
 * reversed list of strings, adding optional extra newline */
element * mk_str_from_list(element *list, bool extra_newline);

/* mk_list - makes new list with key 'key' and children the reverse of 'lst'.
 * This is designed to be used with cons to build lists in a parser action.
 * The reversing is necessary because cons adds to the head of a list. */
element * mk_list(int key, element *lst);

/* mk_link - constructor for LINK element */
element * mk_link(element *label, char *url, char *title);
/* extension = returns true if extension is selected */
bool extension(int ext);

/* match_inlines - returns true if inline lists match (case-insensitive...) */
bool match_inlines(element *l1, element *l2);

/* find_reference - return true if link found in references matching label.
 * 'link' is modified with the matching url and title. */
bool find_reference(mdlink *result, element *label);

/* find_note - return true if note found in notes matching label.
if found, 'result' is set to point to matched note. */

bool find_note(element **result, char *label);

void print_odf_header(charvector *out);
void print_odf_footer(charvector *out);

#endif
