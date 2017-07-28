// 2>&-### SELF-EXTRACTING DOCUMENTATION ###############################
// 2>&-#                                                               #
// 2>&-# Run "bash <thisfile> > <docfile.md>" to extract documentation #
// 2>&-#                                                               #
// 2>&-#################################################################
// 2>&-; awk '/^<\/MARKDOWN>/{f=0};f;/^<MARKDOWN>/{f=1}' $0; exit 0

/************************************************************************
markdown_parser.c - Functions for parsing markdown files using a
                    PEG grammar.

This module has dependencies to routines in markdown_peg.c (raw PEG
processing) and charvector.c (variable-length string functions), which
should be distributed with this file.

Much of this module simply consolidates the modules
parsing_functions.c, utility_functions.c, markdown_output.c, odf.c,
and markdown_lib.c, written by John MacFarlane.  The following notice
accompanies those files.

  (c) 2008 John MacFarlane (jgm at berkeley dot edu).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License or the MIT
  license.  See LICENSE for details.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

Recent modifications are copyright (c) 2016 Teviet Creighton.
These modifications include:

  - Removing all external library dependencies; specifically the
    GString manipulation functions in libglib-2.0 (replaced with
    charvector routines in charvector.c).

  - Adding Unix manpage post-processor routines, in the section
    MARKDOWN TO MANPAGE at the end of this module.

************************************************************************/


/************************************************************************
From parsing_functions.c - Functions for parsing markdown and
                           freeing element lists.
************************************************************************/

/* These yy_* functions come from markdown_parser.c which is
 * generated from markdown_parser.leg
 * */
typedef int (*yyrule)();

extern int yyparse();
extern int yyparsefrom(yyrule);
extern int yy_References();
extern int yy_Notes();
extern int yy_Doc();

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "charvector.h"
#include "markdown_parser.h"

static void free_element_contents(element elt);

/* free_element_list - free list of elements recursively */
void free_element_list(element * elt) {
    element * next = NULL;
    while (elt != NULL) {
        next = elt->next;
        free_element_contents(*elt);
        if (elt->children != NULL) {
            free_element_list(elt->children);
            elt->children = NULL;
        }
        free(elt);
        elt = next;
    }
}

/* free_element_contents - free element contents depending on type */
static void free_element_contents(element elt) {
    switch (elt.key) {
      case STR:
      case SPACE:
      case RAW:
      case HTMLBLOCK:
      case HTML:
      case VERBATIM:
      case CODE:
      case NOTE:
        free(elt.contents.str);
        elt.contents.str = NULL;
        break;
      case LINK:
      case IMAGE:
      case REFERENCE:
        free(elt.contents.link->url);
        elt.contents.link->url = NULL;
        free(elt.contents.link->title);
        elt.contents.link->title = NULL;
        free_element_list(elt.contents.link->label);
        free(elt.contents.link);
        elt.contents.link = NULL;
        break;
      default:
        ;
    }
}

/* free_element - free element and contents */
void free_element(element *elt) {
    free_element_contents(*elt);
    free(elt);
}

element * parse_references(char *string, int extensions) {

    char *oldcharbuf;
    syntax_extensions = extensions;

    oldcharbuf = charbuf;
    charbuf = string;
    yyparsefrom(yy_References);    /* first pass, just to collect references */
    charbuf = oldcharbuf;

    return references;
}

element * parse_notes(char *string, int extensions, element *reference_list) {

    char *oldcharbuf;
    notes = NULL;
    syntax_extensions = extensions;

    if (extension(EXT_NOTES)) {
        references = reference_list;
        oldcharbuf = charbuf;
        charbuf = string;
        yyparsefrom(yy_Notes);     /* second pass for notes */
        charbuf = oldcharbuf;
    }

    return notes;
}

element * parse_markdown(char *string, int extensions, element *reference_list, element *note_list) {

    char *oldcharbuf;
    syntax_extensions = extensions;
    references = reference_list;
    notes = note_list;

    oldcharbuf = charbuf;
    charbuf = string;

    yyparsefrom(yy_Doc);

    charbuf = oldcharbuf;          /* restore charbuf to original value */
    return parse_result;

}

/************************************************************************
From utility_functions.c - List manipulation functions, element
                           constructors, and macro definitions for leg
                           markdown parser.
************************************************************************/

/**********************************************************************

  List manipulation functions

 ***********************************************************************/

/* cons - cons an element onto a list, returning pointer to new head */
element * cons(element *new, element *list) {
    assert(new != NULL);
    new->next = list;
    return new;
}

/* reverse - reverse a list, returning pointer to new list */
element *reverse(element *list) {
    element *new = NULL;
    element *next = NULL;
    while (list != NULL) {
        next = list->next;
        new = cons(list, new);
        list = next;
    }
    return new;
}

/* concat_string_list - concatenates string contents of list of STR elements.
 * Frees STR elements as they are added to the concatenation. */
void concat_string_list(charvector *result, element *list) {
    element *next;
    while (list != NULL) {
        assert(list->key == STR);
        assert(list->contents.str != NULL);
        charvector_append(result, list->contents.str);
        next = list->next;
        free_element(list);
        list = next;
    }
    return;
}

/**********************************************************************

  Global variables used in parsing

 ***********************************************************************/

char *charbuf = "";     /* Buffer of characters to be parsed. */
element *references = NULL;    /* List of link references found. */
element *notes = NULL;         /* List of footnotes found. */
element *parse_result;  /* Results of parse. */
int syntax_extensions;  /* Syntax extensions selected. */

/**********************************************************************

  Auxiliary functions for parsing actions.
  These make it easier to build up data structures (including lists)
  in the parsing actions.

 ***********************************************************************/

/* mk_element - generic constructor for element */
element * mk_element(int key) {
    element *result = malloc(sizeof(element));
    result->key = key;
    result->children = NULL;
    result->next = NULL;
    result->contents.str = NULL;
    return result;
}

/* mk_str - constructor for STR element */
element * mk_str(char *string) {
    element *result;
    assert(string != NULL);
    result = mk_element(STR);
    result->contents.str = strdup(string);
    return result;
}

/* mk_str_from_list - makes STR element by concatenating a
 * reversed list of strings, adding optional extra newline */
element * mk_str_from_list(element *list, bool extra_newline) {
    element *result;
    charvector c = {};
    concat_string_list(&c, reverse(list));
    if (extra_newline)
        charvector_append(&c, "\n");
    result = mk_element(STR);
    result->contents.str = c.str;
    return result;
}

/* mk_list - makes new list with key 'key' and children the reverse of 'lst'.
 * This is designed to be used with cons to build lists in a parser action.
 * The reversing is necessary because cons adds to the head of a list. */
element * mk_list(int key, element *lst) {
    element *result;
    result = mk_element(key);
    result->children = reverse(lst);
    return result;
}

/* mk_link - constructor for LINK element */
element * mk_link(element *label, char *url, char *title) {
    element *result;
    result = mk_element(LINK);
    result->contents.link = malloc(sizeof(mdlink));
    result->contents.link->label = label;
    result->contents.link->url = strdup(url);
    result->contents.link->title = strdup(title);
    return result;
}

/* extension = returns true if extension is selected */
bool extension(int ext) {
    return (syntax_extensions & ext);
}

/* match_inlines - returns true if inline lists match (case-insensitive...) */
bool match_inlines(element *l1, element *l2) {
    while (l1 != NULL && l2 != NULL) {
        if (l1->key != l2->key)
            return false;
        switch (l1->key) {
        case SPACE:
        case LINEBREAK:
        case ELLIPSIS:
        case EMDASH:
        case ENDASH:
        case APOSTROPHE:
            break;
        case CODE:
        case STR:
        case HTML:
            if (strcasecmp(l1->contents.str, l2->contents.str) == 0)
                break;
            else
                return false;
        case EMPH:
        case STRONG:
        case LIST:
        case SINGLEQUOTED:
        case DOUBLEQUOTED:
            if (match_inlines(l1->children, l2->children))
                break;
            else
                return false;
        case LINK:
        case IMAGE:
            return false;  /* No links or images within links */
        default:
            fprintf(stderr, "match_inlines encountered unknown key = %d\n", l1->key);
            exit(EXIT_FAILURE);
            break;
        }
        l1 = l1->next;
        l2 = l2->next;
    }
    return (l1 == NULL && l2 == NULL);  /* return true if both lists exhausted */
}

/* find_reference - return true if link found in references matching label.
 * 'link' is modified with the matching url and title. */
bool find_reference(mdlink *result, element *label) {
    element *cur = references;  /* pointer to walk up list of references */
    mdlink *curitem;
    while (cur != NULL) {
        curitem = cur->contents.link;
        if (match_inlines(label, curitem->label)) {
            *result = *curitem;
            return true;
        }
        else
            cur = cur->next;
    }
    return false;
}

/* find_note - return true if note found in notes matching label.
if found, 'result' is set to point to matched note. */

bool find_note(element **result, char *label) {
   element *cur = notes;  /* pointer to walk up list of notes */
   while (cur != NULL) {
       if (strcmp(label, cur->contents.str) == 0) {
           *result = cur;
           return true;
       }
       else
           cur = cur->next;
   }
   return false;
}

/************************************************************************
From markdown_output.c - functions for printing Elements parsed by
                         markdown_peg.
************************************************************************/

static int extensions;
static int odf_type = 0;

static void print_html_string(charvector *out, char *str, bool obfuscate);
static void print_html_element_list(charvector *out, element *list, bool obfuscate);
static void print_html_element(charvector *out, element *elt, bool obfuscate);
static void print_latex_string(charvector *out, char *str);
static void print_latex_element_list(charvector *out, element *list);
static void print_latex_element(charvector *out, element *elt);
static void print_groff_string(charvector *out, char *str);
static void print_groff_mm_element_list(charvector *out, element *list);
static void print_groff_mm_element(charvector *out, element *elt, int count);
static void print_odf_code_string(charvector *out, char *str);
static void print_odf_string(charvector *out, char *str);
static void print_odf_element_list(charvector *out, element *list);
static void print_odf_element(charvector *out, element *elt);
static bool list_contains_key(element *list, int key);

/**********************************************************************

  Utility functions for printing

 ***********************************************************************/

static int padded = 2;      /* Number of newlines after last output.
                               Starts at 2 so no newlines are needed at start.
                               */

static element *endnotes = NULL; /* List of endnotes to print after main content. */
static int notenumber = 0;  /* Number of footnote. */

/* pad - add newlines if needed */
static void pad(charvector *out, int num) {
    while (num-- > padded)
        charvector_append(out, "\n");;
    padded = num;
}

/* determine whether a certain element is contained within a given list */
static bool list_contains_key(element *list, int key) {
    element *step = NULL;

    step = list;
    while ( step != NULL ) {
        if (step->key == key) {
            return true;
        }
        if (step->children != NULL) {
            if (list_contains_key(step->children, key)) {
                return true;
            }
        }
       step = step->next;
    }
    return false;
}

/**********************************************************************

  Functions for printing Elements as HTML

 ***********************************************************************/

/* print_html_string - print string, escaping for HTML  
 * If obfuscate selected, convert characters to hex or decimal entities at random */
static void print_html_string(charvector *out, char *str, bool obfuscate) {
    while (*str != '\0') {
        switch (*str) {
        case '&':
            charvector_append(out, "&amp;");
            break;
        case '<':
            charvector_append(out, "&lt;");
            break;
        case '>':
            charvector_append(out, "&gt;");
            break;
        case '"':
            charvector_append(out, "&quot;");
            break;
        default:
	  if (obfuscate && ((int) *str >= 0)){
                if (rand() % 2 == 0)
                    charvector_append_printf(out, "&#%d;", (int) *str);
                else
                    charvector_append_printf(out, "&#x%x;", (unsigned int) *str);
            }
            else
                charvector_append_c(out, *str);
        }
    str++;
    }
}

/* print_html_element_list - print a list of elements as HTML */
static void print_html_element_list(charvector *out, element *list, bool obfuscate) {
    while (list != NULL) {
        print_html_element(out, list, obfuscate);
        list = list->next;
    }
}

/* add_endnote - add an endnote to global endnotes list. */
static void add_endnote(element *elt) {
  element *node = (element *)malloc( sizeof(element) );
  node->children = elt;
  endnotes = cons( node, endnotes );
}

/* print_html_element - print an element as HTML */
static void print_html_element(charvector *out, element *elt, bool obfuscate) {
    int lev;
    switch (elt->key) {
    case SPACE:
        charvector_append_printf(out, "%s", elt->contents.str);
        break;
    case LINEBREAK:
        charvector_append(out, "<br/>\n");
        break;
    case STR:
        print_html_string(out, elt->contents.str, obfuscate);
        break;
    case ELLIPSIS:
        charvector_append(out, "&hellip;");
        break;
    case EMDASH:
        charvector_append(out, "&mdash;");
        break;
    case ENDASH:
        charvector_append(out, "&ndash;");
        break;
    case APOSTROPHE:
        charvector_append(out, "&rsquo;");
        break;
    case SINGLEQUOTED:
        charvector_append(out, "&lsquo;");
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append(out, "&rsquo;");
        break;
    case DOUBLEQUOTED:
        charvector_append(out, "&ldquo;");
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append(out, "&rdquo;");
        break;
    case CODE:
        charvector_append(out, "<code>");
        print_html_string(out, elt->contents.str, obfuscate);
        charvector_append(out, "</code>");
        break;
    case HTML:
        charvector_append_printf(out, "%s", elt->contents.str);
        break;
    case LINK:
        if (strstr(elt->contents.link->url, "mailto:") == elt->contents.link->url)
            obfuscate = true;  /* obfuscate mailto: links */
        charvector_append(out, "<a href=\"");
        print_html_string(out, elt->contents.link->url, obfuscate);
        charvector_append(out, "\"");
        if (strlen(elt->contents.link->title) > 0) {
            charvector_append(out, " title=\"");
            print_html_string(out, elt->contents.link->title, obfuscate);
            charvector_append(out, "\"");
        }
        charvector_append(out, ">");
        print_html_element_list(out, elt->contents.link->label, obfuscate);
        charvector_append(out, "</a>");
        break;
    case IMAGE:
        charvector_append(out, "<img src=\"");
        print_html_string(out, elt->contents.link->url, obfuscate);
        charvector_append(out, "\" alt=\"");
        print_html_element_list(out, elt->contents.link->label, obfuscate);
        charvector_append(out, "\"");
        if (strlen(elt->contents.link->title) > 0) {
            charvector_append(out, " title=\"");
            print_html_string(out, elt->contents.link->title, obfuscate);
            charvector_append(out, "\"");
        }
        charvector_append(out, " />");
        break;
    case EMPH:
        charvector_append(out, "<em>");
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append(out, "</em>");
        break;
    case STRONG:
        charvector_append(out, "<strong>");
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append(out, "</strong>");
        break;
    case STRIKE:
        charvector_append(out, "<del>");
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append(out, "</del>");
        break;
    case LIST:
        print_html_element_list(out, elt->children, obfuscate);
        break;
    case RAW:
        /* Shouldn't occur - these are handled by process_raw_blocks() */
        assert(elt->key != RAW);
        break;
    case H1: case H2: case H3: case H4: case H5: case H6:
        lev = elt->key - H1 + 1;  /* assumes H1 ... H6 are in order */
        pad(out, 2);
        charvector_append_printf(out, "<h%1d>", lev);
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append_printf(out, "</h%1d>", lev);
        padded = 0;
        break;
    case PLAIN:
        pad(out, 1);
        print_html_element_list(out, elt->children, obfuscate);
        padded = 0;
        break;
    case PARA:
        pad(out, 2);
        charvector_append(out, "<p>");
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append(out, "</p>");
        padded = 0;
        break;
    case HRULE:
        pad(out, 2);
        charvector_append(out, "<hr />");
        padded = 0;
        break;
    case HTMLBLOCK:
        pad(out, 2);
        charvector_append_printf(out, "%s", elt->contents.str);
        padded = 0;
        break;
    case VERBATIM:
        pad(out, 2);
        charvector_append_printf(out, "%s", "<pre><code>");
        print_html_string(out, elt->contents.str, obfuscate);
        charvector_append_printf(out, "%s", "</code></pre>");
        padded = 0;
        break;
    case BULLETLIST:
        pad(out, 2);
        charvector_append_printf(out, "%s", "<ul>");
        padded = 0;
        print_html_element_list(out, elt->children, obfuscate);
        pad(out, 1);
        charvector_append_printf(out, "%s", "</ul>");
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(out, 2);
        charvector_append_printf(out, "%s", "<ol>");
        padded = 0;
        print_html_element_list(out, elt->children, obfuscate);
        pad(out, 1);
        charvector_append(out, "</ol>");
        padded = 0;
        break;
    case LISTITEM:
        pad(out, 1);
        charvector_append(out, "<li>");
        padded = 2;
        print_html_element_list(out, elt->children, obfuscate);
        charvector_append(out, "</li>");
        padded = 0;
        break;
    case BLOCKQUOTE:
        pad(out, 2);
        charvector_append(out, "<blockquote>\n");
        padded = 2;
        print_html_element_list(out, elt->children, obfuscate);
        pad(out, 1);
        charvector_append(out, "</blockquote>");
        padded = 0;
        break;
    case REFERENCE:
        /* Nonprinting */
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt->contents.str == 0) {
            add_endnote(elt);
            ++notenumber;
            charvector_append_printf(out, "<a class=\"noteref\" id=\"fnref%d\" href=\"#fn%d\" title=\"Jump to note %d\">[%d]</a>",
                notenumber, notenumber, notenumber, notenumber);
        }
        break;
    default: 
        fprintf(stderr, "print_html_element encountered unknown element key = %d\n", elt->key); 
        exit(EXIT_FAILURE);
    }
}

static void print_html_endnotes(charvector *out) {
    int counter = 0;
    element *note;
    element *note_elt;
    if (endnotes == NULL) 
        return;
    note = reverse(endnotes);
    charvector_append(out, "<hr/>\n<ol id=\"notes\">");
    while (note != NULL) {
        note_elt = note->children;
        counter++;
        pad(out, 1);
        charvector_append_printf(out, "<li id=\"fn%d\">\n", counter);
        padded = 2;
        print_html_element_list(out, note_elt->children, false);
        charvector_append_printf(out, " <a href=\"#fnref%d\" title=\"Jump back to reference\">[back]</a>", counter);
        pad(out, 1);
        charvector_append(out, "</li>");
        note = note->next;
    }
    pad(out, 1);
    charvector_append(out, "</ol>");
    free_element_list(endnotes);
}

/**********************************************************************

  Functions for printing Elements as LaTeX

 ***********************************************************************/

/* print_latex_string - print string, escaping for LaTeX */
static void print_latex_string(charvector *out, char *str) {
    while (*str != '\0') {
        switch (*str) {
          case '{': case '}': case '$': case '%':
          case '&': case '_': case '#':
            charvector_append_printf(out, "\\%c", *str);
            break;
        case '^':
            charvector_append(out, "\\^{}");
            break;
        case '\\':
            charvector_append(out, "\\textbackslash{}");
            break;
        case '~':
            charvector_append(out, "\\ensuremath{\\sim}");
            break;
        case '|':
            charvector_append(out, "\\textbar{}");
            break;
        case '<':
            charvector_append(out, "\\textless{}");
            break;
        case '>':
            charvector_append(out, "\\textgreater{}");
            break;
        default:
            charvector_append_c(out, *str);
        }
    str++;
    }
}

/* print_latex_element_list - print a list of elements as LaTeX */
static void print_latex_element_list(charvector *out, element *list) {
    while (list != NULL) {
        print_latex_element(out, list);
        list = list->next;
    }
}

/* print_latex_element - print an element as LaTeX */
static void print_latex_element(charvector *out, element *elt) {
    /*int lev;*/
    int i;
    switch (elt->key) {
    case SPACE:
        charvector_append_printf(out, "%s", elt->contents.str);
        break;
    case LINEBREAK:
        charvector_append(out, "\\\\\n");
        break;
    case STR:
        print_latex_string(out, elt->contents.str);
        break;
    case ELLIPSIS:
        charvector_append(out, "\\ldots{}");
        break;
    case EMDASH: 
        charvector_append(out, "---");
        break;
    case ENDASH: 
        charvector_append(out, "--");
        break;
    case APOSTROPHE:
        charvector_append(out, "'");
        break;
    case SINGLEQUOTED:
        charvector_append(out, "`");
        print_latex_element_list(out, elt->children);
        charvector_append(out, "'");
        break;
    case DOUBLEQUOTED:
        charvector_append(out, "``");
        print_latex_element_list(out, elt->children);
        charvector_append(out, "''");
        break;
    case CODE:
        charvector_append(out, "\\texttt{");
        print_latex_string(out, elt->contents.str);
        charvector_append(out, "}");
        break;
    case HTML:
        /* don't print HTML */
        break;
    case LINK:
        charvector_append_printf(out, "\\href{%s}{", elt->contents.link->url);
        print_latex_element_list(out, elt->contents.link->label);
        charvector_append(out, "}");
        break;
    case IMAGE:
        charvector_append_printf(out, "\\includegraphics{%s}", elt->contents.link->url);
        break;
    case EMPH:
        charvector_append(out, "\\emph{");
        print_latex_element_list(out, elt->children);
        charvector_append(out, "}");
        break;
    case STRONG:
        charvector_append(out, "\\textbf{");
        print_latex_element_list(out, elt->children);
        charvector_append(out, "}");
        break;
    case STRIKE:
        charvector_append(out, "\\sout{");
        print_latex_element_list(out, elt->children);
        charvector_append(out, "}");
        break;
    case LIST:
        print_latex_element_list(out, elt->children);
        break;
    case RAW:
        /* Shouldn't occur - these are handled by process_raw_blocks() */
        assert(elt->key != RAW);
        break;
    case H1: case H2: case H3:
        pad(out, 2);
        /* lev = elt->key - H1 + 1;  assumes H1 ... H6 are in order */
        charvector_append(out, "\\");
        for (i = elt->key; i > H1; i--)
            charvector_append(out, "sub");
        charvector_append(out, "section{");
        print_latex_element_list(out, elt->children);
        charvector_append(out, "}");
        padded = 0;
        break;
    case H4: case H5: case H6:
        pad(out, 2);
        charvector_append(out, "\\noindent\\textbf{");
        print_latex_element_list(out, elt->children);
        charvector_append(out, "}");
        padded = 0;
        break;
    case PLAIN:
        pad(out, 1);
        print_latex_element_list(out, elt->children);
        padded = 0;
        break;
    case PARA:
        pad(out, 2);
        print_latex_element_list(out, elt->children);
        padded = 0;
        break;
    case HRULE:
        pad(out, 2);
        charvector_append(out, "\\begin{center}\\rule{3in}{0.4pt}\\end{center}\n");
        padded = 0;
        break;
    case HTMLBLOCK:
        /* don't print HTML block */
        break;
    case VERBATIM:
        pad(out, 1);
        charvector_append(out, "\\begin{verbatim}\n");
        print_latex_string(out, elt->contents.str);
        charvector_append(out, "\n\\end{verbatim}");
        padded = 0;
        break;
    case BULLETLIST:
        pad(out, 1);
        charvector_append(out, "\\begin{itemize}");
        padded = 0;
        print_latex_element_list(out, elt->children);
        pad(out, 1);
        charvector_append(out, "\\end{itemize}");
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(out, 1);
        charvector_append(out, "\\begin{enumerate}");
        padded = 0;
        print_latex_element_list(out, elt->children);
        pad(out, 1);
        charvector_append(out, "\\end{enumerate}");
        padded = 0;
        break;
    case LISTITEM:
        pad(out, 1);
        charvector_append(out, "\\item ");
        padded = 2;
        print_latex_element_list(out, elt->children);
        charvector_append(out, "\n");
        break;
    case BLOCKQUOTE:
        pad(out, 1);
        charvector_append(out, "\\begin{quote}");
        padded = 0;
        print_latex_element_list(out, elt->children);
        pad(out, 1);
        charvector_append(out, "\\end{quote}");
        padded = 0;
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt->contents.str == 0) {
            charvector_append(out, "\\footnote{");
            padded = 2;
            print_latex_element_list(out, elt->children);
            charvector_append(out, "}");
            padded = 0; 
        }
        break;
    case REFERENCE:
        /* Nonprinting */
        break;
    default: 
        fprintf(stderr, "print_latex_element encountered unknown element key = %d\n", elt->key); 
        exit(EXIT_FAILURE);
    }
}

/**********************************************************************

  Functions for printing Elements as groff (mm macros)

 ***********************************************************************/

static bool in_list_item = false; /* True if we're parsing contents of a list item. */

/* print_groff_string - print string, escaping for groff */
static void print_groff_string(charvector *out, char *str) {
    /* escape dots if it is the first character */
    if (*str == '.') {
        charvector_append(out, "\\[char46]");
        str++;
    }

    while (*str != '\0') {
        switch (*str) {
        case '\\':
            charvector_append(out, "\\e");
            break;
        default:
            charvector_append_c(out, *str);
        }
    str++;
    }
}

/* print_groff_mm_element_list - print a list of elements as groff ms */
static void print_groff_mm_element_list(charvector *out, element *list) {
    int count = 1;
    while (list != NULL) {
        print_groff_mm_element(out, list, count);
        list = list->next;
        count++;
    }
}

/* print_groff_mm_element - print an element as groff ms */
static void print_groff_mm_element(charvector *out, element *elt, int count) {
    int lev;
    switch (elt->key) {
    case SPACE:
        charvector_append_printf(out, "%s", elt->contents.str);
        padded = 0;
        break;
    case LINEBREAK:
        pad(out, 1);
        charvector_append(out, ".br\n");
        padded = 0;
        break;
    case STR:
        print_groff_string(out, elt->contents.str);
        padded = 0;
        break;
    case ELLIPSIS:
        charvector_append(out, "...");
        break;
    case EMDASH:
        charvector_append(out, "\\[em]");
        break;
    case ENDASH:
        charvector_append(out, "\\[en]");
        break;
    case APOSTROPHE:
        charvector_append(out, "'");
        break;
    case SINGLEQUOTED:
        charvector_append(out, "`");
        print_groff_mm_element_list(out, elt->children);
        charvector_append(out, "'");
        break;
    case DOUBLEQUOTED:
        charvector_append(out, "\\[lq]");
        print_groff_mm_element_list(out, elt->children);
        charvector_append(out, "\\[rq]");
        break;
    case CODE:
        charvector_append(out, "\\fC");
        print_groff_string(out, elt->contents.str);
        charvector_append(out, "\\fR");
        padded = 0;
        break;
    case HTML:
        /* don't print HTML */
        break;
    case LINK:
        print_groff_mm_element_list(out, elt->contents.link->label);
        charvector_append_printf(out, " (%s)", elt->contents.link->url);
        padded = 0;
        break;
    case IMAGE:
        charvector_append(out, "[IMAGE: ");
        print_groff_mm_element_list(out, elt->contents.link->label);
        charvector_append(out, "]");
        padded = 0;
        /* not supported */
        break;
    case EMPH:
        charvector_append(out, "\\fI");
        print_groff_mm_element_list(out, elt->children);
        charvector_append(out, "\\fR");
        padded = 0;
        break;
    case STRONG:
        charvector_append(out, "\\fB");
        print_groff_mm_element_list(out, elt->children);
        charvector_append(out, "\\fR");
        padded = 0;
        break;
    case STRIKE:
        charvector_append(out, "\\c\n.ST \"");
        print_groff_mm_element_list(out, elt->children);
        charvector_append(out, "\"");
        pad(out, 1);
        break;
    case LIST:
        print_groff_mm_element_list(out, elt->children);
        padded = 0;
        break;
    case RAW:
        /* Shouldn't occur - these are handled by process_raw_blocks() */
        assert(elt->key != RAW);
        break;
    case H1: case H2: case H3: case H4: case H5: case H6:
        lev = elt->key - H1 + 1;
        pad(out, 1);
        charvector_append_printf(out, ".H %d \"", lev);
        print_groff_mm_element_list(out, elt->children);
        charvector_append(out, "\"");
        padded = 0;
        break;
    case PLAIN:
        pad(out, 1);
        print_groff_mm_element_list(out, elt->children);
        padded = 0;
        break;
    case PARA:
        pad(out, 1);
        if (!in_list_item || count != 1)
            charvector_append(out, ".P\n");
        print_groff_mm_element_list(out, elt->children);
        padded = 0;
        break;
    case HRULE:
        pad(out, 1);
        charvector_append(out, "\\l'\\n(.lu*8u/10u'");
        padded = 0;
        break;
    case HTMLBLOCK:
        /* don't print HTML block */
        break;
    case VERBATIM:
        pad(out, 1);
        charvector_append(out, ".VERBON 2\n");
        print_groff_string(out, elt->contents.str);
        charvector_append(out, ".VERBOFF");
        padded = 0;
        break;
    case BULLETLIST:
        pad(out, 1);
        charvector_append(out, ".BL");
        padded = 0;
        print_groff_mm_element_list(out, elt->children);
        pad(out, 1);
        charvector_append(out, ".LE 1");
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(out, 1);
        charvector_append(out, ".AL");
        padded = 0;
        print_groff_mm_element_list(out, elt->children);
        pad(out, 1);
        charvector_append(out, ".LE 1");
        padded = 0;
        break;
    case LISTITEM:
        pad(out, 1);
        charvector_append(out, ".LI\n");
        in_list_item = true;
        padded = 2;
        print_groff_mm_element_list(out, elt->children);
        in_list_item = false;
        break;
    case BLOCKQUOTE:
        pad(out, 1);
        charvector_append(out, ".DS I\n");
        padded = 2;
        print_groff_mm_element_list(out, elt->children);
        pad(out, 1);
        charvector_append(out, ".DE");
        padded = 0;
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt->contents.str == 0) {
            charvector_append(out, "\\*F\n");
            charvector_append(out, ".FS\n");
            padded = 2;
            print_groff_mm_element_list(out, elt->children);
            pad(out, 1);
            charvector_append(out, ".FE\n");
            padded = 1; 
        }
        break;
    case REFERENCE:
        /* Nonprinting */
        break;
    default: 
        fprintf(stderr, "print_groff_mm_element encountered unknown element key = %d\n", elt->key); 
        exit(EXIT_FAILURE);
    }
}

/**********************************************************************

  Functions for printing Elements as ODF

 ***********************************************************************/

/* print_odf_code_string - print string, escaping for HTML and saving newlines 
*/
static void print_odf_code_string(charvector *out, char *str) {
    char *tmp;
    while (*str != '\0') {
        switch (*str) {
        case '&':
            charvector_append(out, "&amp;");
            break;
        case '<':
            charvector_append(out, "&lt;");
            break;
        case '>':
            charvector_append(out, "&gt;");
            break;
        case '"':
            charvector_append(out, "&quot;");
            break;
        case '\n':
            charvector_append(out, "<text:line-break/>");
            break;
        case ' ':
            tmp = str;
            tmp++;
            if (*tmp == ' ') {
                tmp++;
                if (*tmp == ' ') {
                    tmp++;
                    if (*tmp == ' ') {
                        charvector_append(out, "<text:tab/>");
                        str = tmp;
                    } else {
                        charvector_append(out, " ");
                    }
                } else {
                    charvector_append(out, " ");
                }
            } else {
                charvector_append(out, " ");
            }
            break;
        default:
               charvector_append_c(out, *str);
        }
    str++;
    }
}

/* print_odf_string - print string, escaping for HTML and saving newlines */
static void print_odf_string(charvector *out, char *str) {
    char *tmp;
    while (*str != '\0') {
        switch (*str) {
        case '&':
            charvector_append(out, "&amp;");
            break;
        case '<':
            charvector_append(out, "&lt;");
            break;
        case '>':
            charvector_append(out, "&gt;");
            break;
        case '"':
            charvector_append(out, "&quot;");
            break;
        case '\n':
            tmp = str;
            tmp--;
            if (*tmp == ' ') {
                tmp--;
                if (*tmp == ' ') {
                    charvector_append(out, "<text:line-break/>");
                } else {
                    charvector_append(out, "\n");
                }
            } else {
                charvector_append(out, "\n");
            }
            break;
        case ' ':
            tmp = str;
            tmp++;
            if (*tmp == ' ') {
                tmp++;
                if (*tmp == ' ') {
                    tmp++;
                    if (*tmp == ' ') {
                        charvector_append(out, "<text:tab/>");
                        str = tmp;
                    } else {
                        charvector_append(out, " ");
                    }
                } else {
                    charvector_append(out, " ");
                }
            } else {
                charvector_append(out, " ");
            }
            break;
        default:
               charvector_append_c(out, *str);
        }
    str++;
    }
}

/* print_odf_element_list - print an element list as ODF */
static void print_odf_element_list(charvector *out, element *list) {
    while (list != NULL) {
        print_odf_element(out, list);
        list = list->next;
    }
}

/* print_odf_element - print an element as ODF */
static void print_odf_element(charvector *out, element *elt) {
    int lev;
    int old_type = 0;
    switch (elt->key) {
    case SPACE:
        charvector_append_printf(out, "%s", elt->contents.str);
        break;
    case LINEBREAK:
        charvector_append(out, "<text:line-break/>");
        break;
    case STR:
        print_html_string(out, elt->contents.str, 0);
        break;
    case ELLIPSIS:
        charvector_append(out, "&hellip;");
        break;
    case EMDASH:
        charvector_append(out, "&mdash;");
        break;
    case ENDASH:
        charvector_append(out, "&ndash;");
        break;
    case APOSTROPHE:
        charvector_append(out, "&rsquo;");
        break;
    case SINGLEQUOTED:
        charvector_append(out, "&lsquo;");
        print_odf_element_list(out, elt->children);
        charvector_append(out, "&rsquo;");
        break;
    case DOUBLEQUOTED:
        charvector_append(out, "&ldquo;");
        print_odf_element_list(out, elt->children);
        charvector_append(out, "&rdquo;");
        break;
    case CODE:
        charvector_append(out, "<text:span text:style-name=\"Source_20_Text\">");
        print_html_string(out, elt->contents.str, 0);
        charvector_append(out, "</text:span>");
        break;
    case HTML:
        break;
    case LINK:
        charvector_append(out, "<text:a xlink:type=\"simple\" xlink:href=\"");
        print_html_string(out, elt->contents.link->url, 0);
        charvector_append(out, "\"");
        if (strlen(elt->contents.link->title) > 0) {
            charvector_append(out, " office:name=\"");
            print_html_string(out, elt->contents.link->title, 0);
            charvector_append(out, "\"");
        }
        charvector_append(out, ">");
        print_odf_element_list(out, elt->contents.link->label);
        charvector_append(out, "</text:a>");
        break;
    case IMAGE:
        charvector_append(out, "<draw:frame text:anchor-type=\"as-char\"\ndraw:z-index=\"0\" draw:style-name=\"fr1\" svg:width=\"95%%\"");
        charvector_append(out, ">\n<draw:text-box><text:p><draw:frame text:anchor-type=\"as-char\" draw:z-index=\"1\" ");
        charvector_append(out, "><draw:image xlink:href=\"");
        print_odf_string(out, elt->contents.link->url);
        charvector_append(out,"\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\" draw:filter-name=\"&lt;All formats&gt;\"/>\n</draw:frame></text:p>");
        charvector_append(out, "</draw:text-box></draw:frame>\n");
        break;
    case EMPH:
        charvector_append(out,
            "<text:span text:style-name=\"MMD-Italic\">");
        print_odf_element_list(out, elt->children);
        charvector_append(out, "</text:span>");
        break;
    case STRONG:
        charvector_append(out,
            "<text:span text:style-name=\"MMD-Bold\">");
        print_odf_element_list(out, elt->children);
        charvector_append(out, "</text:span>");
        break;
    case STRIKE:
        charvector_append(out,
            "<text:span text:style-name=\"StrikeThrough\">");
        print_odf_element_list(out, elt->children);
        charvector_append(out, "</text:span>");
        break;
    case LIST:
        print_odf_element_list(out, elt->children);
        break;
    case RAW:
        /* Shouldn't occur - these are handled by process_raw_blocks() */
        assert(elt->key != RAW);
        break;
    case H1: case H2: case H3: case H4: case H5: case H6:
        lev = elt->key - H1 + 1;  /* assumes H1 ... H6 are in order */
        charvector_append_printf(out, "<text:h text:outline-level=\"%d\">", lev);
        print_odf_element_list(out, elt->children);
        charvector_append(out, "</text:h>\n");
        padded = 0;
        break;
    case PLAIN:
        print_odf_element_list(out, elt->children);
        padded = 0;
        break;
    case PARA:
        charvector_append(out, "<text:p");
        switch (odf_type) {
            case BLOCKQUOTE:
                charvector_append(out," text:style-name=\"Quotations\"");
                break;
            case CODE:
                charvector_append(out," text:style-name=\"Preformatted Text\"");
                break;
            case VERBATIM:
                charvector_append(out," text:style-name=\"Preformatted Text\"");
                break;
            case ORDEREDLIST:
            case BULLETLIST:
                charvector_append(out," text:style-name=\"P2\"");
                break;
            case NOTE:
                charvector_append(out," text:style-name=\"Footnote\"");
                break;
            default:
                charvector_append(out," text:style-name=\"Standard\"");
                break;
        }
        charvector_append(out, ">");
        print_odf_element_list(out, elt->children);
        charvector_append(out, "</text:p>\n");
        break;
    case HRULE:
        charvector_append(out,"<text:p text:style-name=\"Horizontal_20_Line\"/>\n");
        break;
    case HTMLBLOCK:
        /* don't print HTML block */
        /* but do print HTML comments for raw ODF */
        if (strncmp(elt->contents.str,"<!--",4) == 0) {
            /* trim "-->" from end */
            elt->contents.str[strlen(elt->contents.str)-3] = '\0';
            charvector_append_printf(out, "%s", &elt->contents.str[4]);
        }
        break;
    case VERBATIM:
        old_type = odf_type;
        odf_type = VERBATIM;
        charvector_append(out, "<text:p text:style-name=\"Preformatted Text\">");
        print_odf_code_string(out, elt->contents.str);
        charvector_append(out, "</text:p>\n");
        odf_type = old_type;
        break;
    case BULLETLIST:
        if ((odf_type == BULLETLIST) ||
            (odf_type == ORDEREDLIST)) {
            /* I think this was made unnecessary by another change.
            Same for ORDEREDLIST below */
            /*  charvector_append(out, "</text:p>"); */
        }
        old_type = odf_type;
        odf_type = BULLETLIST;
        charvector_append_printf(out, "%s", "<text:list>");
        print_odf_element_list(out, elt->children);
        charvector_append_printf(out, "%s", "</text:list>");
        odf_type = old_type;
        break;
    case ORDEREDLIST:
        if ((odf_type == BULLETLIST) ||
            (odf_type == ORDEREDLIST)) {
            /* charvector_append(out, "</text:p>"); */
        }
        old_type = odf_type;
        odf_type = ORDEREDLIST;
        charvector_append_printf(out, "%s", "<text:list>\n");
        print_odf_element_list(out, elt->children);
        charvector_append_printf(out, "%s", "</text:list>\n");
        odf_type = old_type;
        break;
    case LISTITEM:
        charvector_append(out, "<text:list-item>\n");
        if (elt->children->children->key != PARA) {
            charvector_append(out, "<text:p text:style-name=\"P2\">");
        }
        print_odf_element_list(out, elt->children);

        if ((list_contains_key(elt->children,BULLETLIST) ||
            (list_contains_key(elt->children,ORDEREDLIST)))) {
            } else {
                if (elt->children->children->key != PARA) {
                    charvector_append(out, "</text:p>");
                }
            }
        charvector_append(out, "</text:list-item>\n");
        break;
    case BLOCKQUOTE:
        old_type = odf_type;
        odf_type = BLOCKQUOTE;
        print_odf_element_list(out, elt->children);
        odf_type = old_type;
        break;
    case REFERENCE:
        break;
    case NOTE:
        old_type = odf_type;
        odf_type = NOTE;
        /* if contents.str == 0 then print; else ignore - like above */
        if (elt->contents.str == 0) {
            charvector_append(out, "<text:note text:id=\"\" text:note-class=\"footnote\"><text:note-body>\n");
            print_odf_element_list(out, elt->children);
            charvector_append(out, "</text:note-body>\n</text:note>\n");
       }
        elt->children = NULL;
        odf_type = old_type;
        break;
        break;  default:
        fprintf(stderr, "print_odf_element encountered unknown element key = %d\n", elt->key);
        exit(EXIT_FAILURE);
    }
}

/**********************************************************************

  Parameterized function for printing an Element.

 ***********************************************************************/

void print_element_list(charvector *out, element *elt, int format, int exts) {
    /* Initialize globals */
    endnotes = NULL;
    notenumber = 0;

    extensions = exts;
    padded = 2;  /* set padding to 2, so no extra blank lines at beginning */
    switch (format) {
    case HTML_FORMAT:
        print_html_element_list(out, elt, false);
        if (endnotes != NULL) {
            pad(out, 2);
            print_html_endnotes(out);
        }
        break;
    case LATEX_FORMAT:
        print_latex_element_list(out, elt);
        break;
    case GROFF_MM_FORMAT:
        if (extensions & EXT_STRIKE) {
          charvector_append(out,
              ".de ST\n.nr width \\w'\\\\$1'\n\\Z@\\v'-.25m'\\l'\\\\n[width]u'@\\\\$1\\c\n..\n.\n");
        }
        print_groff_mm_element_list(out, elt);
        break;
    case ODF_FORMAT:
        print_odf_header(out);
        charvector_append(out, "<office:body>\n<office:text>\n");
        if (elt != NULL) print_odf_element_list(out,elt);
        print_odf_footer(out);
        break;
    default:
        fprintf(stderr, "print_element - unknown format = %d\n", format); 
        exit(EXIT_FAILURE);
    }
}

/************************************************************************
From odf.c - Utility routines to enable ODF support in peg-multimarkdown.
***********************************************************************/

void print_odf_header(charvector *out){
    
    /* Insert required XML header */
    charvector_append(out,
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
"<office:document xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\"\n" \
"     xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\"\n" \
"     xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\"\n" \
"     xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\"\n" \
"     xmlns:draw=\"urn:oasis:names:tc:opendocument:xmlns:drawing:1.0\"\n" \
"     xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\"\n" \
"     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n" \
"     xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n" \
"     xmlns:meta=\"urn:oasis:names:tc:opendocument:xmlns:meta:1.0\"\n" \
"     xmlns:number=\"urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0\"\n" \
"     xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\"\n" \
"     xmlns:chart=\"urn:oasis:names:tc:opendocument:xmlns:chart:1.0\"\n" \
"     xmlns:dr3d=\"urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0\"\n" \
"     xmlns:math=\"http://www.w3.org/1998/Math/MathML\"\n" \
"     xmlns:form=\"urn:oasis:names:tc:opendocument:xmlns:form:1.0\"\n" \
"     xmlns:script=\"urn:oasis:names:tc:opendocument:xmlns:script:1.0\"\n" \
"     xmlns:config=\"urn:oasis:names:tc:opendocument:xmlns:config:1.0\"\n" \
"     xmlns:ooo=\"http://openoffice.org/2004/office\"\n" \
"     xmlns:ooow=\"http://openoffice.org/2004/writer\"\n" \
"     xmlns:oooc=\"http://openoffice.org/2004/calc\"\n" \
"     xmlns:dom=\"http://www.w3.org/2001/xml-events\"\n" \
"     xmlns:xforms=\"http://www.w3.org/2002/xforms\"\n" \
"     xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n" \
"     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" \
"     xmlns:rpt=\"http://openoffice.org/2005/report\"\n" \
"     xmlns:of=\"urn:oasis:names:tc:opendocument:xmlns:of:1.2\"\n" \
"     xmlns:xhtml=\"http://www.w3.org/1999/xhtml\"\n" \
"     xmlns:grddl=\"http://www.w3.org/2003/g/data-view#\"\n" \
"     xmlns:tableooo=\"http://openoffice.org/2009/table\"\n" \
"     xmlns:field=\"urn:openoffice:names:experimental:ooo-ms-interop:xmlns:field:1.0\"\n" \
"     xmlns:formx=\"urn:openoffice:names:experimental:ooxml-odf-interop:xmlns:form:1.0\"\n" \
"     xmlns:css3t=\"http://www.w3.org/TR/css3-text/\"\n" \
"     office:version=\"1.2\"\n" \
"     grddl:transformation=\"http://docs.oasis-open.org/office/1.2/xslt/odf2rdf.xsl\"\n" \
"     office:mimetype=\"application/vnd.oasis.opendocument.text\">\n");
    
    /* Font Declarations */
    charvector_append(out, "<office:font-face-decls>\n" \
    "   <style:font-face style:name=\"Courier New\" svg:font-family=\"'Courier New'\"\n" \
    "                    style:font-adornments=\"Regular\"\n" \
    "                    style:font-family-generic=\"modern\"\n" \
    "                    style:font-pitch=\"fixed\"/>\n" \
    "</office:font-face-decls>\n");
    
    /* Append basic style information */
    charvector_append(out, "<office:styles>\n" \
    "<style:style style:name=\"Standard\" style:family=\"paragraph\" style:class=\"text\">\n" \
    "      <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0.15in\"" \
    "     fo:text-align=\"justify\" style:justify-single-word=\"false\"/>\n" \
    "   </style:style>\n" \
    "<style:style style:name=\"Preformatted_20_Text\" style:display-name=\"Preformatted Text\"\n" \
    "             style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"html\">\n" \
    "   <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0in\" fo:text-align=\"start\"\n" \
    "                               style:justify-single-word=\"false\"/>\n" \
    "   <style:text-properties style:font-name=\"Courier New\" fo:font-size=\"11pt\"\n" \
    "                          style:font-name-asian=\"Courier New\"\n" \
    "                          style:font-size-asian=\"11pt\"\n" \
    "                          style:font-name-complex=\"Courier New\"\n" \
    "                          style:font-size-complex=\"11pt\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Source_20_Text\" style:display-name=\"Source Text\"\n" \
    "             style:family=\"text\">\n" \
    "   <style:text-properties style:font-name=\"Courier New\" style:font-name-asian=\"Courier New\"\n" \
    "                          style:font-name-complex=\"Courier New\"\n" \
    "                          fo:font-size=\"11pt\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"List\" style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"list\">\n" \
    "   <style:paragraph-properties fo:text-align=\"start\" style:justify-single-word=\"false\"/>\n" \
    "   <style:text-properties style:font-size-asian=\"12pt\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Quotations\" style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"html\">\n" \
    "   <style:paragraph-properties fo:margin-left=\"0.3937in\" fo:margin-right=\"0.3937in\" fo:margin-top=\"0in\"\n" \
    "                               fo:margin-bottom=\"0.1965in\"\n" \
    "                               fo:text-align=\"justify\"" \
    "                               style:justify-single-word=\"false\"" \
    "                               fo:text-indent=\"0in\"\n" \
    "                               style:auto-text-indent=\"false\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Table_20_Heading\" style:display-name=\"Table Heading\"\n" \
    "             style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Table_20_Contents\"\n" \
    "             style:class=\"extra\">\n" \
    "   <style:paragraph-properties fo:text-align=\"center\" style:justify-single-word=\"false\"\n" \
    "                               text:number-lines=\"false\"\n" \
    "                               text:line-number=\"0\"/>\n" \
    "   <style:text-properties fo:font-weight=\"bold\" style:font-weight-asian=\"bold\"\n" \
    "                          style:font-weight-complex=\"bold\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Horizontal_20_Line\" style:display-name=\"Horizontal Line\"\n" \
    "             style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"html\">\n" \
    "   <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0.1965in\"\n" \
    "                               style:border-line-width-bottom=\"0.0008in 0.0138in 0.0008in\"\n" \
    "                               fo:padding=\"0in\"\n" \
    "                               fo:border-left=\"none\"\n" \
    "                               fo:border-right=\"none\"\n" \
    "                               fo:border-top=\"none\"\n" \
    "                               fo:border-bottom=\"0.0154in double #808080\"\n" \
    "                               text:number-lines=\"false\"\n" \
    "                               text:line-number=\"0\"\n" \
    "                               style:join-border=\"false\"/>\n" \
    "   <style:text-properties fo:font-size=\"6pt\" style:font-size-asian=\"6pt\" style:font-size-complex=\"6pt\"/>\n" \
    "</style:style>\n" \
    "</office:styles>\n");

    /* Automatic style information */
    charvector_append(out, "<office:automatic-styles>" \
    "   <style:style style:name=\"MMD-Italic\" style:family=\"text\">\n" \
    "      <style:text-properties fo:font-style=\"italic\" style:font-style-asian=\"italic\"\n" \
    "                             style:font-style-complex=\"italic\"/>\n" \
    "   </style:style>\n" \
    "   <style:style style:name=\"MMD-Bold\" style:family=\"text\">\n" \
    "      <style:text-properties fo:font-weight=\"bold\" style:font-weight-asian=\"bold\"\n" \
    "                             style:font-weight-complex=\"bold\"/>\n" \
    "   </style:style>\n" \
    "   <style:style style:name=\"StrikeThrough\" style:family=\"text\">\n" \
    "      <style:text-properties style:text-line-through-style=\"solid\"/>\n" \
    "   </style:style>\n" \
    "<style:style style:name=\"MMD-Table\" style:family=\"paragraph\" style:parent-style-name=\"Standard\">\n" \
    "   <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0.05in\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"MMD-Table-Center\" style:family=\"paragraph\" style:parent-style-name=\"MMD-Table\">\n" \
    "   <style:paragraph-properties fo:text-align=\"center\" style:justify-single-word=\"false\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"MMD-Table-Right\" style:family=\"paragraph\" style:parent-style-name=\"MMD-Table\">\n" \
    "   <style:paragraph-properties fo:text-align=\"right\" style:justify-single-word=\"false\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"P2\" style:family=\"paragraph\" style:parent-style-name=\"Standard\"\n" \
    "             style:list-style-name=\"L2\">\n" \
    "<style:paragraph-properties fo:text-align=\"start\" style:justify-single-word=\"false\"/>\n" \
    "</style:style>\n" \
	"<style:style style:name=\"fr1\" style:family=\"graphic\" style:parent-style-name=\"Frame\">\n" \
	"   <style:graphic-properties style:print-content=\"false\" style:vertical-pos=\"top\"\n" \
	"                             style:vertical-rel=\"baseline\"\n" \
	"                             fo:padding=\"0in\"\n" \
	"                             fo:border=\"none\"\n" \
	"                             style:shadow=\"none\"/>\n" \
	"</style:style>\n" \
    "</office:automatic-styles>\n" \
    "<style:style style:name=\"P1\" style:family=\"paragraph\" style:parent-style-name=\"Standard\"\n" \
    "             style:list-style-name=\"L1\"/>\n" \
    "<text:list-style style:name=\"L1\">\n" \
    "   <text:list-level-style-bullet />\n" \
    "</text:list-style>\n" \
    "<text:list-style style:name=\"L2\">\n" \
    "   <text:list-level-style-number />\n" \
    "</text:list-style>\n");
}

void print_odf_footer(charvector *out) {
    charvector_append(out, "</office:text>\n</office:body>\n</office:document>");
}

/************************************************************************
From markdown_lib.c - markdown in C using a PEG grammar.
***********************************************************************/

#define TABSTOP 4

/* preformat_text - allocate and copy text buffer while
 * performing tab expansion. */
static void preformat_text(charvector *buf, char *text) {
    char next_char;
    int charstotab;

    int len = 0;

    charstotab = TABSTOP;
    while ((next_char = *text++) != '\0') {
        switch (next_char) {
        case '\t':
            while (charstotab > 0)
                charvector_append_c(buf, ' '), len++, charstotab--;
            break;
        case '\n':
            charvector_append_c(buf, '\n'), len++, charstotab = TABSTOP;
            break;
        default:
            charvector_append_c(buf, next_char), len++, charstotab--;
        }
        if (charstotab == 0)
            charstotab = TABSTOP;
    }
    charvector_append(buf, "\n\n");
    return;
}

/* print_tree - print tree of elements, for debugging only.
static void print_tree(element * elt, int indent) {
    int i;
    char * key;
    while (elt != NULL) {
        for (i = 0; i < indent; i++)
            fputc(' ', stderr);
        switch (elt->key) {
            case LIST:               key = "LIST"; break;
            case RAW:                key = "RAW"; break;
            case SPACE:              key = "SPACE"; break;
            case LINEBREAK:          key = "LINEBREAK"; break;
            case ELLIPSIS:           key = "ELLIPSIS"; break;
            case EMDASH:             key = "EMDASH"; break;
            case ENDASH:             key = "ENDASH"; break;
            case APOSTROPHE:         key = "APOSTROPHE"; break;
            case SINGLEQUOTED:       key = "SINGLEQUOTED"; break;
            case DOUBLEQUOTED:       key = "DOUBLEQUOTED"; break;
            case STR:                key = "STR"; break;
            case LINK:               key = "LINK"; break;
            case IMAGE:              key = "IMAGE"; break;
            case CODE:               key = "CODE"; break;
            case HTML:               key = "HTML"; break;
            case EMPH:               key = "EMPH"; break;
            case STRONG:             key = "STRONG"; break;
            case PLAIN:              key = "PLAIN"; break;
            case PARA:               key = "PARA"; break;
            case LISTITEM:           key = "LISTITEM"; break;
            case BULLETLIST:         key = "BULLETLIST"; break;
            case ORDEREDLIST:        key = "ORDEREDLIST"; break;
            case H1:                 key = "H1"; break;
            case H2:                 key = "H2"; break;
            case H3:                 key = "H3"; break;
            case H4:                 key = "H4"; break;
            case H5:                 key = "H5"; break;
            case H6:                 key = "H6"; break;
            case BLOCKQUOTE:         key = "BLOCKQUOTE"; break;
            case VERBATIM:           key = "VERBATIM"; break;
            case HTMLBLOCK:          key = "HTMLBLOCK"; break;
            case HRULE:              key = "HRULE"; break;
            case REFERENCE:          key = "REFERENCE"; break;
            case NOTE:               key = "NOTE"; break;
            default:                 key = "?";
        }
        if ( elt->key == STR ) {
            fprintf(stderr, "0x%p: %s   '%s'\n", (void *)elt, key, elt->contents.str);
        } else {
            fprintf(stderr, "0x%p: %s\n", (void *)elt, key);
        }
        if (elt->children)
            print_tree(elt->children, indent + 4);
        elt = elt->next;
    }
} */

/* process_raw_blocks - traverses an element list, replacing any RAW elements with
 * the result of parsing them as markdown text, and recursing into the children
 * of parent elements.  The result should be a tree of elements without any RAWs. */
static element * process_raw_blocks(element *input, int extensions, element *references, element *notes) {
    element *current = NULL;
    element *last_child = NULL;
    char *contents;
    current = input;

    while (current != NULL) {
        if (current->key == RAW) {
            /* \001 is used to indicate boundaries between nested lists when there
             * is no blank line.  We split the string by \001 and parse
             * each chunk separately. */
            contents = strtok(current->contents.str, "\001");
            current->key = LIST;
            current->children = parse_markdown(contents, extensions, references, notes);
            last_child = current->children;
            while ((contents = strtok(NULL, "\001"))) {
                while (last_child->next != NULL)
                    last_child = last_child->next;
                last_child->next = parse_markdown(contents, extensions, references, notes);
            }
            free(current->contents.str);
            current->contents.str = NULL;
        }
        if (current->children != NULL)
            current->children = process_raw_blocks(current->children, extensions, references, notes);
        current = current->next;
    }
    return input;
}

/*
<MARKDOWN>
# markdown_to_charvector(3) 2016-07-14

## NAME

`markdown_to_charvector(3)` - convert markdown text to various formats

## SYNOPSIS

`#include "markdown_parser.h"`  
`#include "charvector.h"`

`void markdown_to_charvector( charvector *`_out_`, char *`_text_`,
                             int` _extensions_`, int` _output_format_ `);`

## DESCRIPTION

This function converts markdown-formatted _text_ to a formatted
variable-length string _out_ in the format specified by
_output_format_, which may be one of the following `enum` constants:

`HTML_FORMAT`  
`LATEX_FORMAT`  
`GROFF_MM_FORMAT`  
`ODF_FORMAT`

The _extensions_ variable is a bit-mask of flags turining on various
parsing extensions:

`EXT_SMART`:
  Use smart typography extension.

`EXT_NOTES`:
  Use notes extension.

`EXT_FILTER_HTML`:
  Filter out raw HTML.

`EXT_FILTER_STYLES`:
  Filter out HTML styles.

`EXT_STRIKE`:
  Use strike-through extension.

## RETURN VALUE

This function does not return a result.

## SEE ALSO

markdown_to_string(3),
charvector(5)

</MARKDOWN> */
/* markdown_to_charvector - convert markdown text to output format specified.
 * Output *out should be freed with charvector_free(). */
void markdown_to_charvector( charvector *out, char *text,
			     int extensions, int output_format) {
    element *result;
    element *references;
    element *notes;
    charvector formatted_text = {};

    preformat_text(&formatted_text, text);

    references = parse_references(formatted_text.str, extensions);
    notes = parse_notes(formatted_text.str, extensions, references);
    result = parse_markdown(formatted_text.str, extensions, references, notes);

    result = process_raw_blocks(result, extensions, references, notes);

    charvector_free(&formatted_text);

    print_element_list( out, result, output_format, extensions);

    free_element_list(result);
    free_element_list(references);
    return;
}

/*
<MARKDOWN>
# markdown_to_string(3) 2016-07-14

## NAME

`markdown_to_string(3)` - convert markdown text to various formats

## SYNOPSIS

`#include "markdown_parser.h"`

`char *markdown_to_string( char *`_text_`, int` _extensions_`,
                          int` _output_format_ `);`

## DESCRIPTION

This function converts markdown-formatted _text_ to the format
specified by _output_format_, which may be one of the following `enum`
constants:

`HTML_FORMAT`  
`LATEX_FORMAT`  
`GROFF_MM_FORMAT`  
`ODF_FORMAT`

The _extensions_ variable is a bit-mask of flags turining on various
parsing extensions:

`EXT_SMART`:
  Use smart typography extension.

`EXT_NOTES`:
  Use notes extension.

`EXT_FILTER_HTML`:
  Filter out raw HTML.

`EXT_FILTER_STYLES`:
  Filter out HTML styles.

`EXT_STRIKE`:
  Use strike-through extension.

This function is simply a front end and wrapper for
markdown_to_charvector(3), hiding any references to the charvector(5)
type.

## RETURN VALUE

The formatted result is returned in a nul-terminated string, allocated
by the function, which must be freed using free(3) when no longer
needed.

## SEE ALSO

markdown_to_charvector(3)

</MARKDOWN> */
/* markdown_to_string - convert markdown text to the output format specified.
 * Returns a null-terminated string, which must be freed after use. */
char * markdown_to_string(char *text, int extensions, int output_format) {
    charvector out = {};
    markdown_to_charvector( &out, text, extensions, output_format);
    return out.str;
}

/* vim:set ts=4 sw=4: */

/************************************************************************
MARKDOWN TO MANPAGE

Copywright (c) 2016, Teviet Creighton
************************************************************************/


/* Function to write a null-terminated line to stream fp, along with a
   trailing '\n'.  Along the way, any character grouping of the form
   command() or command(n), where n is any number and command is a set
   of non-space non-parenthesis characters, will be replaced with a
   standalone line:

     .BR command (n)

   This follows the manpage convention for setting apart references to
   other manpages, allowing subsequent parsers to set up appropriate
   hyperlinks. */
int
write_manpage_line( char *line, FILE *fp )
{
  char *b, *c; /* end and beginning of command(n) substring */

  /* Skip blank lines. */
  if ( !( *line ) ) {
    fputs( "\n", fp );
    return 0;
  } 

  /* Advance b throgh line, looking for command(n) substrings. */
  for ( b = line; ( b = strchr( b, ')' ) ); ) {
    c = b++;
    while ( c > line && isdigit( (int)( *(--c) ) ) )
      ;
    if ( c == line || *c != '(' ||
	 !isgraph( (int)( *(--c) ) ) || strchr( "()", *c ) )
      continue;
    /* It matches.  Move c to the start of the substring. */
    while ( c >= line && isgraph( (int)( *c ) ) && !strchr( "()", *c ) )
      c--;
    c++;
    /* Print line up to this point. */
    if ( line < c ) {
      while ( line < c )
	fputc( (int)( *(line++) ), fp );
      fputc( '\n', fp );
    }
    /* Print standalone line .BR command (n) */
    fputs( ".BR ", fp );
    while ( *line != '(' )
      fputc( (int)( *(line++) ), fp );
    fputc( ' ', fp );
    while ( *line && !isspace( (int)( *line ) ) )
      fputc( (int)( *(line++) ), fp );
    fputc( '\n', fp );
    /* Start next line at next token. */
    while ( isspace( (int)( *line ) ) )
      line++;
  }

  /* Print remainder of line (or entire line if no command(n)
     substring was found). */
  if ( *line ) {
    fputs( line, fp );
    fputs( "\n", fp );
  }
  return 0;
}

#define MATCH( str1, str2 ) ( !strncmp( (str1), (str2), strlen( str2 ) ) )
#define LEN 1024 /* length of name or line buffer */

/*
<MARKDOWN>
# markdown_to_manpage(3) 2017-04-19

## NAME

`markdown_to_manpage(3)` - write manpage(s) from markdown text

## SYNOPSIS

`#include "markdown_parser.h"`

`int markdown_to_manpage( char *`_markdown_`, const char *`_out_ `);`

## DESCRIPTION

This function parses markdown-formatted text from the string
_markdown_ into Unix manpage format described in groff_man(7).  The
output is written to `stdout`, piped to a shell command, or written to
individual files in a specified directory, depending on the _out_
argument.

If _out_ is NULL, then the output is written to `stdout`.

If _out_ is a string (presumed nul-terminated) with a `|` (pipe)
character as its first character, then the rest of the string is a
command to pe opened with popen(3): the output of
markdown_to_manpage() will be piped to the input of the specified
command.  If `|` is the only character, or is followed only by
whitespace, the output reverts to `stdout`, as if _out_ were NULL.
(Note that `|cat` produces similar results, but formally passes
through the `cat` command invoked from the shell.)

Any other string _out_ is taken to be the name of a directory to which
individual manpages will be written.  The function assumes that each
manpage is introduced by a top-level markdown header of the form:

> `#` _topic_`(`_sec_`)`

where _topic_ is the name of the program or function, and _sec_ is the
manual section (normally 1 for programs and 3 for functions).  This
header and the subsequent block of text will be written to the file
_topic_`.`_sec_ in the directory specified by _out_, until another
top-level header specifies a new file.  Any text preceding the first
top-level header will be sent to `stdout` as above.

If several titles _topic1_`(`_sec1_`), `_topic2_`(`_sec2_`), ...`
appear on the subject line delimited by commas, the first is used as
the man page title, and will be given a full man page.  However,
subsequent topics will be given their own man pages _topicN_`.`_secN_
containing the groff(1) source directive `.so
man`_sec1_`/`_topic1_`.`_sec1_ which will point man(1) to the full man
page.

Optionally, the topic(s) and section(s) in the header may be followed
by a modification date in the form `YYYY-MM-DD`, the source (module or
library providing the command), and the manual title; see man-pages(7)
for more information.  The case (lower or upper) of characters in
_topic_ should match the actual name of the function or program, which
will also be the name of its manpage file; markdown_to_manpage()
convert this name to uppercase within the manpage itself.

### Parsing Conventions

The underlying markdown parser is essentially the PEG-markdown parser
written by John MacFarlane (c) 2008, distributed under the GPL
license; the function called is markdown_to_string(3).  This parser is
nicely self-contained and written in C.  It is, however, somewhat
opaque, and its output format is groff-mm(7) instead of groff-man(7).
Rather than rewrite the parser itself, this function applies
post-processing to convert the format and impose various Unix manpage
conventions.

This function also implements a somewhat kludgy description list
markdown element, frequenctly found on manpages but conspicuously
absent from standard markdown syntax.  A description list is
identified by a line ending in a colon character (the description
term), followed by one or more indented lines (the description
definition).  The indentation of the definition is arbitrary, and it
may contain multiple paragraphs with the same indentation, separated
by blank lines.  At present, no other block-level elements within the
definition are supported (though there are workarounds: e.g. ending
the description list, adding the desired block element with extra
indentation, and then resuming the description list with an empty
description term; note however that nested description lists are still
problematic).  The program implements this element by tricking the
markdown parser into thinking the term and definition are a single
paragraph, then applying its own post-processing to the parser output.

## RETURN VALUE

The function returns 0 normally, 1 if it cannot open an output file or
pipe, or 2 on a memory error.  If writing to a pipe,
markdown_to_manpage() returns the exit status of the child process.

## EXAMPLE

Compile the following program along with source files
`markdown_parser.c`, `markdown_peg.c`, and `charvector.c` (included).
It reads markdown from standard input and writes a manpage to standard
output.

    #include <stdio.h>
    #include <stdlib.h>
    #include "markdown_parser.h"
    #include "charvector.h"

    int
    main( int argv, char *argv[] )
    {
        charvector text = {};
        char buffer[1024];
        int n = 1;
        while ( fgets( buffer, 1024, stdin ) )
            n = n && charvector_append( &text, buffer );
        n = n && markdown_to_manpage( text.str, NULL );
        charvector_free( &text );
        exit( n ? EXIT_FAILURE : EXIT_SUCCESS );
    }

## SEE ALSO

md2man(1),
markdown_to_string(3)

</MARKDOWN> */
int markdown_to_manpage( const char *markdown, const char *out )
{
  int i, j, k;                  /* indecies */
  int verb = 0;                 /* whether we're in verbatim mode */
  int first = 0;                /* whether we're reading first topic name */
  charvector preprocessed = {}; /* text with preprocessing */
  charvector converted = {};    /* output of markdown parser */
  char *line, *next, *nnext;    /* pointers to start and end of lines */
  char buf[LEN];                /* topic name or line indentation buffer */
  char topic[LEN];              /* topic.sec string */
  char *section = NULL;         /* .sec part of topic.sec string */
  char path[LEN];               /* file path */
  char list[LEN] = {};          /* nested list types */
  char ul = '-', ol = 'a';      /* list item specifiers */
  int itemno[LEN];              /* nested list item counts */
  FILE *fp = stdout, *fp1;      /* output file pointers */

  /* Check input arguments.  If out is NULL or specifies a pipe to no
     command, write to stdout as per default. */
  if ( !markdown )
    return 2;
  if ( out && out[0] == '|' ) {
    for ( i = 1; out[i] && isspace( (int)( out[i] ) ); i++ )
      ;
    if ( !out[i] )
      out = NULL;
    else if ( !( fp = popen( out + i, "w" ) ) )  {
      fputs( "markdown_to_manpage: could not open pipe\n", stderr );
      return 1;
    }
  }

  /* Custom preprocessing for description lists.  Following the
     initial term: line, multiple paragraphs with the same indentation
     are part of the same description.  The blank lines between these
     paragraphs should remain blank lines, not be replaced with groff
     .PP macros (which reset the margins).  So to trick the markdown
     parser, replace these blank lines with single ':' characters.
     After parsing, a postprocessor will then replace these isolated
     ':' characters with blank lines. */
  if ( !charvector_append( &preprocessed, markdown ) ) {
    if ( out && out[0] == '|' )
      pclose( fp );
    else if ( fp != stdout )
      fclose( fp );
    return 2;
  }
  for ( line = preprocessed.str; ( line = strstr( line, ":\n " ) ) ; ) {
    line += 2;
    for ( i = 1; line[i] == ' '; i++ )
      ;
    if ( i >= LEN )
      continue;
    snprintf( buf, LEN, "%*s", i, "" );
    while ( 1 ) {
      while ( ( next = strchr( line, '\n' ) ) && !strncmp( next + 1, buf, i ) )
	line = next + 1;
      line = next + 1;
      if ( line[0] != '\n' || strncmp( line + 1, buf, i ) )
	break;
      j = line - preprocessed.str;
      charvector_insert( &preprocessed, ":", j );
      line = preprocessed.str + j + 2;
    }
  }

  /* Convert to groff-mm. */
  markdown_to_charvector( &converted, preprocessed.str, 0, GROFF_MM_FORMAT );
  charvector_free( &preprocessed );

  /* Replace some font macros.  The manpage convention is that literal
     commands should be bold as well as fixed-width. */
  if ( !charvector_replace( &converted, "\\fC", "\\fB\\fC" ) ) {
    if ( out && out[0] == '|' )
      pclose( fp );
    else if ( fp != stdout )
      fclose( fp );
    return 2;
  }

  /* Replace groff-mm macros with standard manpage macros. */
  for ( line = converted.str; line; line = ( next ? next + 1 : NULL ) ) {
    if ( ( next = strchr( line, '\n' ) ) )
      *next = '\0';
    /* Level 1 sections are manpage titles.  The title is normally
       given as command(sec) with possible additional arguments; this
       must be parsed into .TH COMMAND sec with following arguments.
       If out is not NULL, we must also open the appropriate output
       file or pipe. */
    if ( MATCH( line, ".H 1" ) ) {
      i = 5;
      first = 1;
      do {
	for ( i++; isspace( line[i] ); i++ )
	  ;
	for ( j = 0; line[i] && line[i+1] && j < LEN - 1 &&
		!isspace( (int)( line[i] ) ) && line[i] != '('; i++, j++ )
	  buf[j] = line[i];
	buf[ ( k = j++ ) ] = '.';
	for ( i++; line[i] && line[i+1] && isspace( (int)( line[i] ) ); i++ )
	  ;
	for ( ; line[i] && line[i+1] && j < LEN &&
		!isspace( (int)( line[i] ) ) && line[i] != ')'; i++, j++ )
	  buf[j] = line[i];
	if ( j >= LEN ) {
	  fputs( "markdown_to_manpage: section name too long\n", stderr );
	  charvector_free( &converted );
	  if ( out && out[0] == '|' )
	    return pclose( fp );
	  if ( fp != stdout )
	    fclose( fp );
	  return 2;
	}
	buf[j] = '\0';
	if ( out && out[0] != '|' ) {
	  if ( first && fp != stdout )
	    fclose( fp );
	  if ( snprintf( path, LEN, "%s/%s", out, buf ) >= LEN - 1 ) {
	    fprintf( stderr, "markdown_to_manpage: path too long"
		     " (max %d characters)\n", LEN - 1 );
	    charvector_free( &converted );
	    return 1;
	  }
	  if ( !( fp1 = fopen( path, "w" ) ) ) {
	    fprintf( stderr, "markdown_to_manpage: could not open %s\n",
		     path );
	    charvector_free( &converted );
	    return 1;
	  }
	  if ( first )
	    fp = fp1;
	  else {
	    fprintf( fp1, ".so man%s/%s\n", section + 1, topic );
	    fclose( fp1 );
	  }
	}
	if ( first ) {
	  strcpy( topic, buf );
	  section = strchr( topic, '.' );
	  first = 0;
	}
      } while ( line[++i] == ',' );
      for ( j = 0; topic + j < section; j++ )
	topic[j] = toupper( topic[j] );
      topic[j] = ' ';
      fputs( ".TH ", fp );
      fputs( topic, fp );
      fputc( ' ', fp );
      for ( ; line[i] && line[i+1]; i++ )
	fputc( line[i], fp );
      fputc( '\n', fp );
    }
    /* Level 2 sections are "sections", level 3 and lower are
       "subsections". */
    else if ( MATCH( line, ".H 2" ) ) {
      fputs( ".SH ", fp );
      for ( j = 4; line[j]; j++ )
	line[j] = toupper( line[j] );
      write_manpage_line( line + 4, fp );
    } else if ( MATCH( line, ".H" ) ) {
      fputs( ".SS ", fp );
      write_manpage_line( line + 4, fp );
    }
    /* Paragraph, blockquote, and code blocks. */
    else if ( MATCH( line, ".P" ) )
      fputs( ( list[0] ? ".IP\n" : ".PP\n" ), fp );
    else if ( MATCH( line, ".DS I" ) )
      fputs( ".PP\n.RS\n", fp );
    else if ( MATCH( line, ".DE" ) )
      fputs( ".RE\n", fp );
    else if ( MATCH( line, ".VERBON" ) ) {
      fputs( "\n.RS\n.EX\n", fp );
      verb = 1;
    } else if ( MATCH( line, ".VERBOFF" ) ) {
      fputs( ".EE\n.RE\n", fp );
      verb = 0;
    }
    /* Ordered and unordered lists. */
    else if ( MATCH( line, ".BL" ) ) {
      if ( list[0] )
	fputs( ".RS\n", fp );
      itemno[ strlen( list ) ] = 0;
      ul = ( ul == '*' ? '-' : '*' );
      if ( strlen( list ) < LEN - 1 )
	list[ strlen( list ) ] = ul;
    } else if ( MATCH( line, ".AL" ) ) {
      if ( list[0] )
	fputs( ".RS\n", fp );
      itemno[ strlen( list ) ] = 0;
      ol = ( ol == '1' ? 'a' : '1' );
      if ( strlen( list ) < LEN - 1 )
	list[ strlen( list ) ] = ol;
    } else if ( MATCH( line, ".LE" ) ) {
      int level = strlen( list ) - 1;
      ul = ( list[level] == '*' ? '-' : ( list[level] == '-' ? '*' : ul ) );
      ol = ( list[level] == '1' ? 'a' : ( list[level] == 'a' ? '1' : ol ) );
      list[level] = '\0';
      if ( list[0] )
	fputs( ".RE\n", fp );
    } else if ( MATCH( line, ".LI" ) ) {
      int level = strlen( list ) - 1;
      if ( list[level] == '*' )
	fputs( ".IP \\(bu\n", fp );
      else if ( list[level] == '-' )
	fputs( ".IP \\(em\n", fp );
      else if ( list[level] == '1' )
	fprintf( fp, ".IP %d.\n", itemno[level] + 1 );
      else if ( list[level] == 'a' ) {
	int num = ( itemno[level] > 25 ? 25 : itemno[level] );
	fprintf( fp, ".IP %c.\n", 'a' + num );
      } else
	fputs( ".IP .\n", fp );
      itemno[level] += 1;
    }
    /* Special postprocessing for description lists. */
    else if ( !isspace( (int)( *line ) ) && next && next[-1] == ':' ) {
      for ( nnext = next; nnext && nnext[-1] == ':' &&
	      !isspace( (int)( nnext[1] ) ); )
	nnext = strchr( nnext + 1, '\n' );
      if ( nnext && nnext[-1] == ':' && isspace( nnext[1] ) ) {
	if ( strlen( line ) > 1 ) {
	  next[-1] = '\0';
	  fputs( ".TP\n", fp );
	  write_manpage_line( line, fp );
	} else
	  fputs( ".IP\n", fp );
	for ( line = next + 1; line < nnext;
	      line = ( next ? next + 1 : NULL ) ) {
	  next = strchr( line, '\n' );
	  next[-1] = '\0';
	  fputs( ".TQ\n", fp );
	  write_manpage_line( line, fp );
	}
	for ( line = next + 1; line; line = ( next ? next + 1 : NULL ) ) {
	  if ( ( next = strchr( line, '\n' ) ) )
	    *next = '\0';
	  if ( isspace( (int)( *line ) ) ) {
	    for ( i = 1; isspace( (int)( line[i] ) ); i++ )
	      ;
	    write_manpage_line( line + i, fp );
	  } else if ( !strcmp( line, ":" ) )
	    fputs( "\n", fp );
	  else {
	    *next = '\n';
	    next = line - 1;
	    break;
	  }
	}
      } else
	write_manpage_line( line, fp );
    }

    /* Default: just write the line. */
    else if ( verb ) {
      fputs( line, fp );
      fputc( '\n', fp );
    } else
      write_manpage_line( line, fp );
  }

  /* Finished. */
  charvector_free( &converted );
  if ( out && out[0] == '|' )
    return pclose( fp );
  if ( fp != stdout )
    fclose( fp );
  return 0;
}

/*
<DEPRECATED MD>
# markdown_to_manterm(3) 2016-07-11

## NAME

`markdown_to_manterm(3)` - format markdown text for basic terminal display

## SYNOPSIS

`#include "markdown_parser.h"`

`int markdown_to_manterm( char *`_markdown_`, FILE *`_fp_ `);`

## DESCRIPTION

This function parses markdown-formatted text from the string
_markdown_ into a rough approximation of a manpage processed by
man(1), writing the result to _fp_ (usually a terminal).  If _fp_ is
NULL, output is written to `stdout`.

The function is useful in situations where man(1) may not be
available, or if the user just wants "good enough" manpage-style
output without invoking a full markdown parser with
markdown_to_manpage(3).  If you don't try anything too fancy, most
markdown will render nicely with markdown_to_manterm() while still
producing a proper manpage with markdown_to_manpage(3).

The main constraint on the input is that markdown_to_manterm() makes
no attempt to hyphenate, justify, or adjust linebreaks in the text.
Linebreaks and other whitespace are written exactly as they appear in
the raw markdown (with a few exceptions).  Nonetheless, as long as the
markdown was written with human readability in mind, with reasonable
line lengths (e.g. no more than 72 characters per line), the results
should look fine.

### Parsing Conventions

The function applies the following basic rules to mimic the output of
man(1):

  * All lines are indented 7 spaces, unless they begin with one or
    more `#` characters.  Lines beginning with one or two `#`
    characters are unindented, lines with three or more are indented 4
    spaces.

  * For lines beginning with one `#` character, the first token is
    raised to uppercase.  For lines beginning with two `#` characters,
    the entire line is raised to uppercase.  For lines beginning with
    two or more `#` characters, the line is boldfaced.  Leading and
    trailing `#` characters are removed.

  * If the indentation increases by 4 or more characters, and the
    previous line did not end with a `:` character, this signals the
    start of a "code" block.  Any subsequent line that has less than
    this initial indentation signals the end of the code block.
    Within the code block, "special" characters are printed literally.
    But note that for lines beginning with `*` or _N_`.` where _N_ is
    a numeral, it is the start of text after the item label that
    determines the "base" indentation for subsequent lines; a code
    block within a list item must be indented at least 4 characters
    more than the list item text.

  * Any `>` characters that precedes any other text on a line is
    replaced with 7 spaces, unless we are inside a code block (above).

  * Text delimited by **\`**...**\`** characters, `**`...`**`
    characters, or `__`..`__` characters is boldfaced.  Text delimited
    by `*`...`*` or `_`...`_` characters is underlined.  Backticks
    suppress any other formatting on the enclosed text (including
    backslashes; see below).  Note that if `*` is the first
    non-whitespace character on a line, and is followed by a space,
    then it is assumed to be a list label and is rendered literally;
    also a `_` character directly adjoining alphanumeric characters on
    either side is rendered literally.

  * A `\` character, unless it is inside a backtick-delimited span,
    causes the next character to be printed as-is, ignoring any
    "special" meaning.  Thus, `\\` prints a literal backslash
    character.

  * Other than as specified above, all whitespace, including
    linebreaks, are rendered as in the original text.

Boldface and underline formatting is performed using ASCII escape
codes: `ESC[1m;` for bold, `ESC[4m;` for underline, `ESC[0m;` for
normal (where `ESC` is the ASCII "escape" character, octal `\033`).
This will have the desired result on most terminal displays.  No other
font adjustments are performed: for best results, the terminal should
have a fixed-width font.

## RETURN VALUE

The function returns 0, or 1 if passed bad arguments.

## SEE ALSO

md2man(1),
markdown_to_manpage(3)

</MARKDOWN> */
int markdown_to_manterm_old( const char *markdown, FILE *fp )
{
  const char *c, *d;  /* pointers within markdown */
  char *md = NULL;    /* copy of markdown */
  int emph = 0;       /* emphasis mode */
  int verb = 0;       /* verbatim mode */
  int code = 0;       /* block verbatim mode */
  int flag = 0;       /* flag for other conditions */
  int indent = 0;     /* indentation of current line */
  int previndent = 0; /* indentation of surrounding block */
  int base = 0;       /* added indentation to non-header blocks */

  /* Check inputs. */
  if ( !markdown || !fp )
    return 1;

  /* Preprocess to replace description list ':' characters with
     nonprintable '\001' characters. */
  if ( ( md = (char *)calloc( strlen( markdown ) + 1, sizeof(char) ) ) ) {
    char *b;
    strcpy( md, markdown );
    markdown = md;
    for ( b = strchr( markdown + 1, '\n' ); b && *b; ) {
      if ( b[-1] == ':' ) {
	for ( d = b; !isspace( (int)( d[1] ) ) && ( d = strchr( d + 1, '\n' ) )
		&& d[-1] == ':'; )
	  ;
	if ( d && d[-1] == ':' )
	  for ( ; b && b <= d; b = strchr( b + 1, '\n' ) )
	    b[-1] = '\001';
      } else
	b = strchr( b + 1, '\n' );
    }
  }

  /* Loop over lines. */
  for ( c = markdown; *c; c++ ) {

    /* Find end of line. */
    if ( !( d = strchr( c, '\n' ) ) )
      d = c + strlen( c ) - 1;
    while ( d > c && isspace( (int)( *d ) ) )
      d--;
    if ( d == c ) {
      while ( *c != '\n' )
	c++;
      fputc( '\n', fp );
      continue;
    }

    /* Process headers. */
    if ( *c == '#' ) {
      base = 7;
      while ( d > c && ( *d == '#' || isspace( (int)( *d ) ) ) )
	d--;
      if ( d == c ) {
	while ( *c != '\n' )
	  c++;
	fputc( '\n', fp );
	continue;
      }
      if ( *(++c) != '#' ) {
	while ( c <= d && isspace( (int)( *c ) ) )
	  c++;
	while ( c <= d && !isspace( (int)( *c ) ) )
	  fputc( toupper( *(c++) ), fp );
	while ( c <= d )
	  fputc( *(c++), fp );
	while ( *c && *c != '\n' )
	  c++;
	fputc( '\n', fp );
      } else {
	fputs( "\x1b[1m", fp );
	if ( *(++c) != '#' ) {
	  while ( c <= d && isspace( (int)( *c ) ) )
	    c++;
	  while ( c <= d )
	    fputc( toupper( *(c++) ), fp );
	} else {
	  fputs( "   ", fp );
	  while ( c <= d && *c == '#' )
	    c++;
	  while ( c <= d && isspace( (int)( *c ) ) )
	    c++;
	  while ( c <= d )
	    fputc( *(c++), fp );
	}
	while ( *c && *c != '\n' )
	  c++;
	if ( c[0] == '\n' &&  c[1] == '\n' )
	  c++;
	fputs( "\x1b[0m\n", fp );
      }
      emph = verb = 0;
      continue;
    }

    /* Write leading whitespace, including blockquote spaces, plus
       list labels. */
    for ( flag = indent = 0; c <= d; c++ ) {
      if ( isspace( (int)( *c ) ) ) {
	fputc( *c, fp );
	indent = indent + ( flag ? 0 : 1 );
      } else if ( *c == '>' ) {
	if ( code && indent >= previndent )
	  break;
	fputs( ( verb ? ">" : "       " ), fp );
	flag = 0;
      } else
	break;
    }
    if ( c != d )
      fprintf( fp, "%*s", base, "" );
    if ( !code && !verb && indent >= previndent + 4 ) {
      const char *a = c;
      while ( a > markdown && *a != '\n' )
	a--;
      if ( a != markdown && a[-1] != '\001' ) {
	code = 1;
	if ( emph )
	  fputs( "\x1b[0m", fp );
	emph = 0;
      }
      previndent = indent;
    } else if ( code && indent < previndent ) {
      code = 0;
      previndent = indent;
    } else if ( !code && !verb ) {
      previndent = indent;
      if ( strchr( "*+-", c[0] ) && isspace( (int)( c[1] ) ) ) {
	fputc( *(c++), fp );
	previndent++;
	while ( isspace( (int)( *c ) ) ) {
	  fputc( *(c++), fp );
	  previndent++;
	}
      } else if ( isdigit( (int)( c[0] ) ) && c[1] == '.' &&
		  isspace( (int)( c[2] ) ) ) {
	fprintf( fp, "%.2s", c );
	c += 2;
	previndent += 2;
	while ( isspace( (int)( *c ) ) ) {
	  fputc( *(c++), fp );
	  previndent++;
	}
      }
    }

    /* Write rest of line. */
    for ( ; c <= d; c++ ) {

      /* Within block verbatim mode. */
      if ( code )
	fputc( *c, fp );

      /* Within verbatim mode. */
      else if ( verb ) {
	if ( *c == '`' ) {
	  verb = 0;
	  fputs( ( emph == 1 ? "\x1b[0m\x1b[4m" :
		   ( emph == 0 ? "\x1b[0m" : "" ) ), fp );
	} else
	  fputc( *c, fp );
      }

      /* Escape sequence. */
      else if ( *c == '\\' )
	fputc( *(++c), fp );

      /* Start verbatim mode. */
      else if ( *c == '`' ) {
	verb = 1;
	fputs( ( emph == 1 ? "\x1b[0m\x1b[1m" :
		 ( emph == 0 ? "\x1b[1m" : "" ) ), fp );
      }

      /* Emphasis delimiter. */
      else if ( strchr( "*_", *c ) ) {
	if ( *c == '_' && isalnum( c[-1] ) && isalnum( c[1] ) )
	  fputc( '_', fp );
	else if ( emph == 2 && c[1] == c[0] ) {
	  emph = 0;
	  fputs( "\x1b[0m", fp );
	  c++;
	} else if ( emph == 1 ) {
	  fputs( "\x1b[0m", fp );
	  emph = 0;
	} else if ( c[1] == c[0] ) {
	  fputs( "\x1b[1m", fp );
	  c++;
	  emph = 2;
	} else {
	  fputs( "\x1b[4m", fp );
	  emph = 1;
	}
      }

      /* Special nonprinting code. */
      else if ( *c == '\001' )
	;

      /* Ordinary character. */
      else
	fputc( *c, fp );
    }

    /* Continue to end of line. */
    while ( *c != '\n' )
      c++;
    fputc( '\n', fp );
  }

  fputs( c, fp );
  if ( md )
    free( md );
  return 0;
}


/*
<MARKDOWN>
# markdown_to_man_str(3) 2016-11-20

## NAME

`markdown_to_man_str(3)` - format markdown to simple man(1)-like output

## SYNOPSIS

`#include "markdown_parser.h"`

`char *markdown_to_man_str( char *`_markdown_ `);`

## DESCRIPTION

This function parses markdown-formatted text from the string
_markdown_ into a plain-text approximation of a manpage processed by
man(1), The output is an ordinary printable text string, plus three
special characters: `\001` signifies the start of (return to)
unadorned text, `\002` signifies the start of singly-emphasized
(underlined or italic) text, and `\003` signifies the start of
doubly-emphasized (boldfaced) text.  Output procedures can replace
these with appropriate device control codes to generate prettified
output.

The function is useful in situations where man(1) may not be available
to display groff-formated manpages, or if the user just wants "good
enough" manpage-style output without invoking a full markdown parser
with markdown_to_manpage(3).

The main constraint on the input is that markdown_to_man_str() makes
no attempt to hyphenate, justify, or adjust linebreaks in the text.
Linebreaks and other whitespace are written exactly as they appear in
the raw markdown (with a few exceptions).  Nonetheless, if the output
is in a monospaced font, and the input markdown was written with human
readability in mind and with reasonable line lengths (e.g. no more
than 72 characters per line), the results should look fine.

### Parsing Conventions

The function applies the following basic rules to mimic the output of
man(1):

  * Headers are lines beginning with one or more `#` characters.
    Lines beginning with one or two `#` characters are unindented,
    lines with three or more are indented 4 spaces.  All subsequent
    non-header lines are indented 7 spaces (though lines preceding the
    first header are unindented).  A single blank line following a
    header is suppressed.  Note that underlined headers are not
    supported.

  * For lines beginning with one `#` character, the first token is
    raised to uppercase.  For lines beginning with two `#` characters,
    the entire line is raised to uppercase.  For lines beginning with
    two or more `#` characters, the line is boldfaced.  Leading and
    trailing `#` characters are removed.

  * A group of one or more lines ending in `:`, followed by one or
    more indented lines, is treated as a "description list", and the
    `:` characters are not printed.

  * If the indentation increases by 4 or more characters, and the
    previous line did not end with a `:` character, this signals the
    start of a "code" block.  Any subsequent line that has less than
    this initial indentation signals the end of the code block.
    Within the code block, "special" characters are printed literally.
    But note that for lines beginning with `*` or _N_`.` where _N_ is
    a numeral, it is the start of text after the item label that
    determines the "base" indentation for subsequent lines; a code
    block within a list item must be indented at least 4 characters
    more than the list item text.

  * Any `>` characters that precedes any other text on a line is
    replaced with 7 spaces, unless we are inside a code block (above).

  * Text delimited by **\`**...**\`** characters, `**`...`**`
    characters, or `__`..`__` characters is boldfaced.  Text delimited
    by `*`...`*` or `_`...`_` characters is underlined.  Backticks
    suppress any other formatting on the enclosed text (including
    backslashes; see below).  Note that if `*` is the first
    non-whitespace character on a line, and is followed by a space,
    then it is assumed to be a list label and is rendered literally;
    also a `_` character directly adjoining alphanumeric characters on
    either side is rendered literally.

  * A `\` character, unless it is inside a backtick-delimited span,
    causes the next character to be printed as-is, ignoring any
    "special" meaning.  Thus, `\\` prints a literal backslash
    character.

  * Other than as specified above, all whitespace, including
    linebreaks, are rendered as in the original text.

## RETURN VALUE

The function returns the formatted nul-terminated string, which should
be de-allocated with free(3) when no longer needed.  It returns NULL
if allocation fails or if _markdown_ is NULL.  Badly-formatted
_markdown_ will result in badly-formatted output, not an error.

## SEE ALSO

md2man(1),
markdown_to_manpage(3),
markdown_to_manterm(3),
markdown_to_man_out(3)

</MARKDOWN> */
char *markdown_to_man_str( const char *markdown )
{
  char *md, *c, *d;   /* copy of markdown and pointers within it */
  charvector cv = {}; /* resizeable output string */
  int emph = 0;       /* emphasis mode */
  int verb = 0;       /* verbatim mode */
  int code = 0;       /* block verbatim mode */
  int flag = 0;       /* flag for other conditions */
  int indent = 0;     /* indentation of current line */
  int previndent = 0; /* indentation of surrounding block */
  int base = 0;       /* added indentation to non-header blocks */

  /* Check inputs. */
  if ( !markdown )
    return NULL;

  /* Copy markdown. */
  if ( !( md = (char *)calloc( strlen( markdown ) + 1, sizeof(char) ) ) )
    return NULL;
  strcpy( md, markdown );

  /* Preprocess to replace description list ':' characters with
     nonprintable '\001' characters. */
  for ( c = strchr( md + 1, '\n' ); c && *c; ) {
    if ( c[-1] == ':' ) {
      for ( d = c; !isspace( (int)( d[1] ) ) && ( d = strchr( d + 1, '\n' ) )
	      && d[-1] == ':'; )
	;
      if ( d && d[-1] == ':' )
	for ( ; c && c <= d; c = strchr( c + 1, '\n' ) )
	  c[-1] = '\001';
    } else
      c = strchr( c + 1, '\n' );
  }

  /* Loop over lines. */
  for ( c = md; *c; c++ ) {

    /* Find end of line. */
    if ( !( d = strchr( c, '\n' ) ) )
      d = c + strlen( c ) - 1;
    while ( d > c && isspace( (int)( *d ) ) )
      d--;
    if ( d == c ) {
      while ( *c != '\n' )
	c++;
      charvector_append_c( &cv, '\n' );
      continue;
    }

    /* Process headers. */
    if ( *c == '#' ) {
      base = 7;
      while ( d > c && ( *d == '#' || isspace( (int)( *d ) ) ) )
	d--;
      if ( d == c ) {
	while ( *c != '\n' )
	  c++;
	charvector_append_c( &cv, '\n' );
	continue;
      }
      if ( *(++c) != '#' ) {
	while ( c <= d && isspace( (int)( *c ) ) )
	  c++;
	while ( c <= d && !isspace( (int)( *c ) ) )
	  charvector_append_c( &cv, toupper( *(c++) ) );
	while ( c <= d )
	  charvector_append_c( &cv, *(c++) );
	while ( *c && *c != '\n' )
	  c++;
	charvector_append( &cv, "\n\n" );
      } else {
	charvector_append_c( &cv, '\003' );
	if ( *(++c) != '#' ) {
	  while ( c <= d && isspace( (int)( *c ) ) )
	    c++;
	  while ( c <= d )
	    charvector_append_c( &cv, toupper( *(c++) ) );
	} else {
	  charvector_append( &cv, "   " );
	  while ( c <= d && *c == '#' )
	    c++;
	  while ( c <= d && isspace( (int)( *c ) ) )
	    c++;
	  while ( c <= d )
	    charvector_append_c( &cv, *(c++) );
	}
	while ( *c && *c != '\n' )
	  c++;
	if ( c[0] == '\n' &&  c[1] == '\n' )
	  c++;
	charvector_append( &cv, "\001\n" );
      }
      emph = verb = 0;
      continue;
    }

    /* Write leading whitespace, including blockquote spaces, plus
       list labels. */
    for ( flag = indent = 0; c <= d; c++ ) {
      if ( isspace( (int)( *c ) ) ) {
	charvector_append_c( &cv, *c );
	indent = indent + ( flag ? 0 : 1 );
      } else if ( *c == '>' ) {
	if ( code && indent >= previndent )
	  break;
	charvector_append( &cv, ( verb ? ">" : "       " ) );
	flag = 0;
      } else
	break;
    }
    if ( c != d )
      charvector_append_printf( &cv, "%*s", base, "" );
    if ( !code && !verb && indent >= previndent + 4 ) {
      const char *a = c;
      while ( a > markdown && *a != '\n' )
	a--;
      if ( a != markdown && a[-1] != '\001' ) {
	code = 1;
	if ( emph )
	  charvector_append_c( &cv, '\001' );
	emph = 0;
      }
      previndent = indent;
    } else if ( code && indent < previndent ) {
      code = 0;
      previndent = indent;
    } else if ( !code && !verb ) {
      previndent = indent;
      if ( strchr( "*+-", c[0] ) && isspace( (int)( c[1] ) ) ) {
	charvector_append_c( &cv, *(c++) );
	previndent++;
	while ( isspace( (int)( *c ) ) ) {
	  charvector_append_c( &cv, *(c++) );
	  previndent++;
	}
      } else if ( isdigit( (int)( c[0] ) ) && c[1] == '.' &&
		  isspace( (int)( c[2] ) ) ) {
	charvector_append_printf( &cv, "%.2s", c );
	c += 2;
	previndent += 2;
	while ( isspace( (int)( *c ) ) ) {
	  charvector_append_c( &cv, *(c++) );
	  previndent++;
	}
      }
    }

    /* Write rest of line. */
    for ( ; c <= d; c++ ) {

      /* Within block verbatim mode. */
      if ( code )
	charvector_append_c( &cv, *c );

      /* Within verbatim mode. */
      else if ( verb ) {
	if ( *c == '`' ) {
	  verb = 0;
	  charvector_append( &cv, ( emph == 1 ? "\001\002" :
				    ( emph == 0 ? "\001" : "" ) ) );
	} else
	  charvector_append_c( &cv, *c );
      }

      /* Escape sequence. */
      else if ( *c == '\\' )
	charvector_append_c( &cv, *(++c) );

      /* Start verbatim mode. */
      else if ( *c == '`' ) {
	verb = 1;
	charvector_append( &cv, ( emph == 1 ? "\001\003" :
				  ( emph == 0 ? "\003" : "" ) ) );
      }

      /* Emphasis delimiter. */
      else if ( strchr( "*_", *c ) ) {
	if ( *c == '_' && isalnum( c[-1] ) && isalnum( c[1] ) )
	  charvector_append_c( &cv, '_' );
	else if ( emph == 2 && c[1] == c[0] ) {
	  emph = 0;
	  charvector_append_c( &cv, '\001' );
	  c++;
	} else if ( emph == 1 ) {
	  charvector_append_c( &cv, '\001' );
	  emph = 0;
	} else if ( c[1] == c[0] ) {
	  charvector_append_c( &cv, '\003' );
	  c++;
	  emph = 2;
	} else {
	  charvector_append_c( &cv, '\002' );
	  emph = 1;
	}
      }

      /* Special nonprinting code. */
      else if ( *c == '\001' )
	;

      /* Ordinary character. */
      else
	charvector_append_c( &cv, *c );
    }

    /* Continue to end of line. */
    while ( *c && *c != '\n' )
      c++;
    charvector_append_c( &cv, '\n' );
  }

  /* Finished. */
  free( md );
  return cv.str;
}


/*
<MARKDOWN>
# markdown_to_manterm(3) 2016-07-11

## NAME

`markdown_to_manterm(3)` - format markdown text for basic terminal display

## SYNOPSIS

`#include "markdown_parser.h"`

`int markdown_to_manterm( char *`_markdown_`, FILE *`_fp_ `);`

## DESCRIPTION

This function parses markdown-formatted text from the string
_markdown_ into a rough approximation of a manpage processed by
man(1), writing the result to _fp_ (usually a terminal).  If _fp_ is
NULL, output is written to `stdout`.

The function uses markdown_to_man_str(3), not the full
markdown_to_manpage(3) parser, to convert _markdown_ to a manpage-like
format.  See the markdown_to_man_str(3) documentation for a discussion
of the conventions and caveats of this ad-hoc parsing scheme.  As
noted there, this function is used when manpage-like output is desired
without actually having to pass groff-formatted pages to man(1).

Internally, the function simply takes the result of
markdown_to_man_str(3), and replaces the special characters `\001`,
`\002`, `\003` with the ANSI terminal escape sequences `ESC[0m`
(normal), `ESC[4m` (underline), or `ESC[1m` (bold) respectively, where
`ESC` is the ASCII "escape" character, octal `\033`.  This will have
the desired result on most terminal displays.  No other font
adjustments are performed: for best results, the terminal should have
a fixed-width font.

## RETURN VALUE

The function returns 0, or 1 if passed bad arguments.

## SEE ALSO

md2man(1),
markdown_to_manpage(3),
markdown_to_man_out(3),
markdown_to_man_str(3)

</MARKDOWN> */
int markdown_to_manterm( const char *markdown, FILE *fp )
{
  char *man, *c;

  /* Check inputs. */
  if ( !markdown || !fp )
    return 1;
  if ( !( man = markdown_to_man_str( markdown ) ) )
    return 2;

  /* Write output. */
  for ( c = man; *c; c++ )
    if ( *c == '\001' )
      fputs( "\x1b[0m", fp );
    else if ( *c == '\002' )
      fputs( "\x1b[4m", fp );
    else if ( *c == '\003' )
      fputs( "\x1b[1m", fp );
    else
      fputc( *c, fp );
  return 0;
}


/*
<MARKDOWN>
# markdown_to_man_out(3) 2016-07-22

## NAME

`markdown_to_man_out(3)` - display markdown manpage on standard output

## SYNOPSIS

`#include "markdown_parser.h"`

`int markdown_to_man_out( char *`_markdown_ `);`

## DESCRIPTION

This program formats _markdown_ text as a manpage and writes it to
standard output, using various external display tools.

First, the function formats the text in groff-man(7) format and
attempts to pipe the result to the man(1) program, allowing that
program to handle final display.

Second, if that fails, the function formats the text in ANSI terminal
output format using markdown_to_manterm(3), and attempts to pipe the
result to less(1) for paginated display.

Third, if that fails, the function simply writes the ANSI terminal
output from markdown_to_manterm(3) directly to standard output.

## RETURN VALUE

The function returns 0 if piping to man(1) was successful, or 1 if
piping to less(1) was successful, or 2 otherwise.

## SEE ALSO

less(1),
man(1),
md2man(1),
markdown_to_manpage(3),
markdown_to_manterm(3)

</MARKDOWN> */
int markdown_to_man_out( const char *markdown )
{
  int i = 0, j = 0; /* return codes */
  FILE *fp;         /* output pipe */
  if ( markdown_to_manpage( markdown, "|man -l - 2>/dev/null" ) ) {
    if ( !( fp = popen( "less 2>/dev/null", "w" ) ) ) {
      markdown_to_manterm( markdown, stdout );
      return 2;
    }
    i = markdown_to_manterm( markdown, fp );
    j = pclose( fp );
    if ( i || j ) {
      markdown_to_manterm( markdown, stdout );
      return 2;
    }
    return 1;
  }
  return 0;
}
