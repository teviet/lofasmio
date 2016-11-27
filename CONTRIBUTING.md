If you wish to contribute code to the `lofasmio` distribution, you can
start by examining the provided programs.  The lftest(1) and
lfslice(1) programs in particular give simple examples of the standard
file I/O and argument parsing.

To take advantage of the automated manpage generation, follow these
guidelines.

  * For functions, use the tags `<MARKDOWN>` and `</MARKDOWN>` within
    a comment block to indicate a block of markdown-formatted
    documentation.  These tags must occur at the start of a line.
    Within the markdown documentation, top-level headers of the form
    `#`_topic_`(`_sec_`)` start the man page for the specified _topic_
    in the given section _sec_, producing a file _topic_`.`_sec_ in
    the `doc` directory.  Level-2 headers should include `## NAME`,
    `## SYNOPSIS`, `## DESCRIPTION`, `## RETURN VALUE`, and `## SEE
    ALSO`.  See md2man(1) and markdown_to_manpage(3) for more details.

  * For programs, the `--markdown` option must be recognized and cause
    it to print out a markdown-formatted man page.  Normally this text
    is stored in a string `description` within the program; calling
    `markdown_to_man_out(description)` allows the program to display
    its own formatted man page.  See markdown_to_man_out(3) for more
    details.  Suggested subsections are as above, but with `## EXIT
    STATUS` replacing `## RETURN VALUE`.

The existing programs are -- perhaps obsessively -- meticulous about
cleaning up system resources (freeing memory, closing files) before
exiting, even in cases of abnormal exit status.  Most modern-day
kernels will take care of these things for you at the process level.
Unless you expect your code to run on minimal kernels (e.g. embedded
systems), you may find in simpler to leave these out of your code.
**However,** you should still take care to check the inputs to your
functions, and test the return codes of the functions you call, per
good coding practices.  The sample code in the **EXAMPLE** section
follows this approach.
