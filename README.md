lofasmio(7)  2016-08-04
=======================

Name
----

`lofasmio` - reading, writing, and preprocessing routines for LoFASM

Synopsis
--------

<INCLUDE> PROVIDES - lists the programs and functions provided by `lofasmio`

Description
-----------

This suite provides basic functionality for reading, writing, viewing,
and processing LoFASM data.  Currenltly all LoFASM-parsing functions
are in the module `lofasmIO.c`, along with several simple data
analysis programs that may serve as examples.


### Installing

<INCLUDE> INSTALL - gives installation instructions


### Documentation

The above installation, in particular the `make man` command, should
extract the documentation into man pages in the `doc` subdirectory.
You can view them with:

> `man doc/`_topic_`.`_sec_

where _sec_ is `1` for programs, `3` for C functions, `5` for data
formats, or `7` for general documentation, and _topic_ is the name of
the program, function, or format.  For example, this document should
appear in `doc/lofasmio.7`.  See the **SYNOPSIS** above for a list of
other topics.  You may install the man pages in your man(1) search
path if desired.

Program documentation can also be viewed with _prog_ `-H`.


### Copying

<INCLUDE> COPYING - gives copyright information


### Contributing

<INCLUDE> CONTRIBUTING - gives basic `lofasmio` coding guidelines
