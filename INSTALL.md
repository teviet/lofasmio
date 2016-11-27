The `lofasmio` package and documentation can be compiled with the
provided Makefile.  After unpacking, change to the package directory
and run the following:

> `make` - build the library and utility programs  
> `make man` - build the documentation in the `doc` subdirectory  
> `make check` - run a simple test script  

Customization requires editing the Makefile.  Usually this involves
setting one or more variables documented in the top section of the
file.


### Dependencies

`lofasmio` requires zlib(3) and the BSD function funopen(3) to allow
transparent access to gzipped data files.  If your system does not
have these readily available, edit the Makefile and change the `ZLIB`
variable to `no`.  You will have to decompress gzipped LoFASM files
(and, if desired, compress any resulting files) with separate calls to
gunzip(1) and gzip(1).

The files `charvector.c`, `charvector.h`, `markdown_parser.c`,
`markdown_parser.h`, `markdown_peg.c`, and `md2man.c` are used by
`lofasmio` to generate its own man pages.  They are not included in
`liblofasmio`, and are not needed in order to read and write LoFASM
data.
