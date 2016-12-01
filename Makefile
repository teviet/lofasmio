# Makefile for lofasmio package.  See README.md for more information.
#
# This package has few dependencies and should require minimal
# configuration.  Configuration options should be applied by directly
# editing this Makefile, and then running:
#
#     make        - compile programs and library libimutils.a
#     make man    - generate manpages in doc subdirectory
#     make check  - run a simple test script
#
# The most common option is to disable native zlib (gzip/gunzip)
# compression.  Change ZLIB to no if you do not have zlib in a
# standard location, or if your system does not have the funopen C
# function in stdio.h (used to transparently access zlib functions).

ZLIB = yes

# The rest of this Makefile uses standard options and default methods,
# and will normally not require modification.

# Standard options.
VERSION = `cat VERSION`
LFB_VERSION = `version=\`cat VERSION\`;echo $${version%%.*}`
CC = gcc
LDLIBS = -lm
CFLAGS = -g -Wall -O3 -DVERSION="\"$(VERSION)\"" -DLFB_VERSION=$(LFB_VERSION)
ifeq ($(ZLIB),yes)
LDLIBS += -lz
else
CFLAGS += -DNO_ZLIB
endif

# List of source files.
HEADERS = lofasmIO.h
ALLHEADERS = markdown_parser.h charvector.h $(HEADERS)
OBJS = lofasmIO.o
ALLOBJS = markdown_peg.o markdown_parser.o charvector.o $(OBJS)
LIBS = liblofasmio.a($(OBJS))
PROGS = lfslice lfchop lfcat lftest bxresample lftype lfplot2d lfstats \
	lfmed lfmean lfplot
ALLPROGS = md2man $(PROGS)
DISTFILES = Makefile README.md INSTALL.md CONTRIBUTING.md COPYING.md LICENSE \
	VERSION formats.md $(ALLHEADERS) $(ALLOBJS:.o=.c) $(ALLPROGS:=.c)
DATAFILES = 20160619_000326_AA.bbx.gz 20160619_000828_AA.bbx.gz
DATAUNZIP = $(DATAFILES:.gz=)
DIST = lofasmio-$(VERSION)

# The basic build targets need only standard (implicit) rules.
all: $(ALLPROGS) $(LIBS)
$(ALLPROGS): $(ALLOBJS)
$(ALLOBJS): $(ALLHEADERS)
lib: $(LIBS)

# A more elaborate rule for the package to generate its own documentation.
man: $(DISTFILES) PROVIDES.md
	mkdir -p doc
	./md2man -odoc formats.md
	./md2man -b"<MARKDOWN>" -e"</MARKDOWN>" -i"<INCLUDE>" -odoc *.c *.h
	for file in $(PROGS); do ./$$file --manpage > doc/$$file.1; done
PROVIDES.md: $(PROGS) $(HEADERS) $(OBJS:.o=.c)
	-rm PROVIDES.md
	echo -e "### Programs:\n" >> PROVIDES.md
	for file in $(PROGS); \
		do ./$$file --markdown | \
		awk 'c&&!--c{print $$0 "  "};/^## NAME/{c=2}' \
			>> PROVIDES.md; done
	for file in $(HEADERS) $(OBJS:.o=.c); \
		do echo -e "\n### From " $$file":\n" \
			>> PROVIDES.md; \
		awk -e '/^<\/MARKDOWN>/{f=0};f&&c&&!--c{print $$0 "  "};' \
			-e '/^## NAME/{f&&c=2};/^<MARKDOWN>/{f=1}' $$file \
			>> PROVIDES.md; done

# Simple test pipelines for ZLIB and non-ZLIB versions
check: $(PROGS)
ifeq ($(ZLIB),yes)
	./lfcat -v3 -p nan $(DATAFILES:%=testdata/%) testplot.bbx.gz
	./lfslice -v3 -f 0+88e6 testplot.bbx.gz | \
	./lfplot2d -v3 -x 704 -y 600 -c hot -l 1e3 -s 1e-3 -p - testplot.eps
	rm testplot.bbx.gz
else
	for file in $(DATAUNZIP); do gunzip -c testdata/$$file.gz > $$file; done
	./lfcat -v3 -p nan $(DATAUNZIP) testplot.bbx
	./lfslice -v3 -f 0+88e6 testplot.bbx | \
	./lfplot2d -v3 -x 704 -y 600 -c hot -l 1e3 -s 1e-3 -p - testplot.eps
	rm testplot.bbx $(DATAUNZIP)
endif
	@echo -e "\x1b[32;1mSUCCESS:\x1b[0m output in testplot.eps"

# Create/restore uncompiled distribution.
dist: $(DIST).tar.gz
$(DIST).tar.gz: $(DISTFILES) $(DATAFILES:%=testdata/%)
	-rm -rf $(DIST)
	mkdir $(DIST)
	cp $(DISTFILES) $(DIST)
	mkdir $(DIST)/testdata
	cp $(DATAFILES:%=testdata/%) $(DIST)/testdata
	tar -czf $(DIST).tar.gz $(DIST)
	-rm -rf $(DIST)
distclean:
	rm -f $(ALLOBJS) $(ALLPROGS) PROVIDES.md liblofasmio.a testplot.eps
	rm -rf doc
