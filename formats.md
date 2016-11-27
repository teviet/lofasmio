# abx(5)

## Name

`abx` - Ascii Bit Array format

## Synopsis

> `%ABX`  
> `%` _comment_  
> :  
> _dim1_ ... _dimN_ _encoding_  
> _data_

## Description

ABX is a general format for storing an _N_-dimensional arrays of bits
in ASCII format.  The format consists of 0 or more lines of comments,
a line of _N_+1 metadata tokens, and an encoded data block.  The
comments and metadata together are referred to as the header block.
Conventionally, ABX files have a `.abx` filename extension.

### Header

Header comments are lines beginning with `%` characters.  The ABX
format makes no specification as to what, if any, metadata may appear
in these lines: a parser may optionally skip to the first line
beginning with a character other than `%`.  However, by convention,
the first line of an ABX file should consist of four characters
(bytes) identifying it as an ABX file:

> `%ABX`

Routines that produce ABX files should always write this line, but
routines that read ABX files need not demand it (for instance if they
are receiving a stream from a preprocessor).

After the header comment lines (if any), there is a single line of
_N_+1 whitespace-separated tokens of metadata, where _N_ is the number
of dimensions in the array:

> _dim1_ ... _dimN_ _encoding_

followed immediately by a newline `\n`.  The whitespace separating the
tokens is arbitrary so long as it does not include a newline: a parser
can safely read the entire line before counting tokens to determine
the value of _N_.  The _dim#_ tokens are positive integers giving the
length of that dimension of the array.  The _encoding_ token
identifies how the bits in the array are rendered as ASCII: see below.
There should be no whitespace between the _encoding_ token and the
newline character, and the encoded data block begins on the byte
following the newline character.

### Data

All data encodings operate by first "flattening" the multidimensional
bit array into a 1-dimensional array in standard _dim1_-major order:
that is, iterating over _dimN_ in the innermost loop, then
successively over lower dimensions, with the iteration over _dim1_
having the longest period.  If the total number of bits is not a
multiple of 8, the flattened array is padded at the end with zero-bits
to give an exact number of bytes.  The resulting byte sequence is then
encoded as a sequence of characters.

At present, three ABX encodings are supported, storing bit data in the
form of printable ASCII characters:

`raw16`:
    Each byte (8 bits) of data is encoded as a pair of hexadecimal
    digits 00 through ff (or FF; the encoding is not case sensitive).
    This achieves simple readability and printability at the expense
    of a 2:1 increase in storage space over raw binary.  The parser
    ignores whitespace, which can thus be inserted at will, even
    between the digits of a single byte.  Normally this is used to
    insert newline `\n` characters at appropriate intervals (usually
    every 80 characters) for ease of display.  Any non-whitespace
    non-hexadecimal digit signals the end of data.

`float`:
    Every 4 bytes (32 bits) of data is encoded as an IEEE 754
    signle-precision floating point number and written in ASCII in
    base-10 scientific notation, using the printf format `"% 16.8e"`.
    This has sufficient precision that a single-precision float
    written according to this format will generate exactly the same
    number when re-read.  However, this encoding should only be used
    for bit arrays that represent actual IEEE 754 single-precision
    floats, since an arbitrary bit sequence is *not* guaranteed to be
    restored after writing and re-reading.  (Specifically, any set of
    32 bits where bits 24 through 31 are all 1, and at least one of
    the bits from 1 to 23 is also 1, are all represented as `nan`.)
    As partial enforcement of this, the routine will fail if the last
    dimension _dimN_ is not divisible by 32.  Any string not parseable
    as a float by scanf signals the end of data.

`double`:
    Like `float`, except that every 8 bytes (64 bits) of data is
    encoded as an IEEE 754 double-precision floating point number,
    written with printf format `"% 25.16e`.  The last dimension _dimN_
    must be divisible by 64.

Two additional ABX encodings are planned, but not yet implemented.

`raw64`:
    This encoding will use one of several possible email- or web-safe
    base-64 encodings, using 4 characters to represent every 3 bytes
    of binary data.

`raw85`:
    This encoding will use the PostScript ASCII85 base-85 encoding,
    which typically requires only 5 characters for every 4 bytes of
    binary data.

## See Also

readBX(3),
writeBX(3),
bbx(5)


<!-- ############################################################### -->


# bbx(5)

## Name

`bbx` - Binary Bit Array format

## Synopsis

> `%^BBX`  
> `%` _comment_  
> :  
> _dim1_ ... _dimN_ _encoding_  
> _data_

## Description

BBX is a general format for storing an _N_-dimensional arrays of bits
with an ASCII header.  The format consists of 0 or more lines of
comments, a line of _N_+1 metadata tokens, and an encoded data block.
The comments and metadata together are referred to as the header
block.  Conventionally, BBX files have a `.bbx` filename extension.

### Header

Header comments are lines beginning with `%` characters.  The BBX
format makes no specification as to what, if any, metadata may appear
in these lines: a parser may optionally skip to the first line
beginning with a character other than `%`.  However, by convention,
the first line of an BBX file should consist of four characters
(bytes) identifying it as a BBX file:

> `%^BBX`

Here, `^B`, or "control-b" represents the octal byte `002` (ASCII
"start-of-text" control character).  It indicates to programs such as
file(1) that the file contains binary data that is not safe to display
on terminals.  Nonetheless, the `^B` character itself is silently
skipped by most terminals, so you can safely display a BBX file's
header, up to and including the first line not beginning with `%`.

After the header comment lines (if any), there is a single line of
_N_+1 whitespace-separated tokens of metadata, where _N_ is the number
of dimensions in the array:

> _dim1_ ... _dimN_ _encoding_

followed immediately by a newline `\n`.  The whitespace separating the
tokens is arbitrary so long as it does not include a newline: a parser
can safely read the entire line before counting tokens to determine
the value of _N_.  The _dim#_ tokens are positive integers giving the
length of that dimension of the array.  The _encoding_ token
identifies how the bits in the array are recorded in the _data_ block:
see below.  There should be no whitespace between the _encoding_ token
and the newline character, and the encoded data block begins on the
byte following the newline character.

### Data

All data encodings operate by first "flattening" the multidimensional
bit array into a 1-dimensional array in standard _dim1_-major order:
that is, iterating over _dimN_ in the innermost loop, then
successively over lower dimensions, with the iteration over _dim1_
having the longest period.  If the total number of bits is not a
multiple of 8, the flattened array is padded at the end with zero-bits
to give an exact number of bytes.  The resulting byte sequence is then
encoded as a sequence of characters.

At present, there is only one supported BBX encoding scheme:

`raw256`:
    Each byte is written directly to the file.  That is, the data
    block of the file will be an exact copy of the bit array as it is
    stored in memory.  This is a factor of 2 more compressed than the
    ABX `raw16` encoding, at the expense of readability.  No
    characters can be used to break, pad, or terminate the data: the
    only way to signal a premature end of data is to raise an
    end-of-file or error flag within the file stream.

Future versions of the BBX format may incorporate various compression
schemes; however, at present, it is simplest to use a separate
compression format such as gzip(1) to compress a BBX file in its
entirety.

## See Also

readBX(3),
abx(5)


<!-- ############################################################### -->


# lofasm-filterbank(5)

## Name

`lofasm-filterbank` - LoFASM data storage and transfer format

## Synopsis

LoFASM filterbank data shall be stored in bbx(5) format compressed by
gzip(1).  The uncompressed file has the following form:

> `%^BBX`  
> `%hdr_type: LoFASM-filterbank`  
> `%hdr_version: 0000803F`  
> `%station: ` _stationID_  
> `%channel: ` _channelID_  
> `%start_time: ` _ISO_8601_timestamp_  
> `%time_offset_J2000: 0 (s)`  
> `%frequency_offset_DC: 0 (Hz)`  
> `%dim1_label: time (s)`  
> `%dim1_start: ` _tstart_  
> `%dim1_span: ` _tspan_  
> `%dim2_label: frequency (Hz)`  
> `%dim2_start: ` _fstart_  
> `%dim2_span: ` _fspan_  
> `%data_label: ` _quantity_ ` (`_units_`)`  
> `%data_offset: ` _doff_  
> `%data_scale: ` _dscale_  
> `%data_type: ` _type_  
> _dim1_ ... _dimN_ _encoding_  
> _data_

## Description

This document describes the format used to store and transmit LoFASM
filterbank data.  Raw LoFASM data consists of time-frequency
spectograms (filterbanks) of real power and complex cross-power.  The
following standard format is mandated by the LoFASM data analysis
working group:

 1. The standard archival format for LoFASM filterbank data shall be a
    bbx(5) [Binary Bit Array (BBX)](#abx-bbx-format) file, with
    metadata headers as specified in the [LoFASM
    metadata](#lofasm-metadata) section, compressed using
    [gzip(1)](#gzip-utilities) deflation.  The standard filename
    extension for these files is `.bbx.gz` (compressed) or `.bbx`
    (uncompressed).

      * The bbx(5) format preserves all relevant detector data and
        metadata, has a header that is both human- and
        machine-readable, is broadly extendable, and the data block
        can be loaded directly into computer memory as numerical data
        for subsequent analysis.

      * gzip(1) is broadly supported, achieves good lossless
        compression on this type of data, and includes data integrity
        checksums for archiving and transmission.  It is also a simple
        single-pass algorithm, meaning that it is fast (unlikely to
        bottleneck analysis codes), especially when reading just the
        head of the compressed file.  Thus the metadata of each file
        remains readily accessible with minimal overhead.

      * Standard C libraries and Python modules exist for reading
        gzipped data.  Standard generic cross-platform code will be
        made available for parsing and writing BBX files.

 2. Data from LoFASM stations shall be converted to this standard
    format as soon as possible after reading from the instrument
    hardware/firmware.

      * This will maximize the utility of the data integrity checks,
        and will minimize resources required for storage and
        transmission of data.

      * Downstream analysis code should expect to receive data in this
        standard format.

 3. Analysis software working with LoFASM data should store persistent
    intermediate data products in standard LoFASM format, where
    practical.

      * This will maximize interoperability of software tools
        developed by LoFASM data analysts.

<a id="abx-bbx-format"></a>
## BBX Format

The Binary Bit Array (BBX) format is described in full in the bbx(5)
documentation.  Briefly, it is a generic container format for storing
a multidimensional array of bits.  A minimal BBX file contains the
bare minimum of information needed to specify the dimensions of the
array and the representation of the bit sequence:

> _dim1_ ... _dimN_ _encoding_  
> _data_

where _dim1_ through _dimN_ are base-10 integers in ordinary ASCII
text specifying the lengths of each array dimension, and _encoding_ is
an ASCII string specifying the method of representing the bit data
(see below), followed immediately by a newline `\n` character.  The
encoded data begins on the byte immediately following the newline.

The metadata line may be preceded by zero or more lines of comments,
where each comment line begins with an ASCII `%` character.  Comments
should consist only of printable ASCII text, with one exception: The
first line of a BBX file is conventionally

> `%^BBX`

where `^B` is the octal `002` byte (ASCII "start-of-text" control
character).  Its presence is mostly to notify programs such as file(1)
that the file contains non-terminal-safe binary data.

Additional comment lines are not specified by the BBX file format, but
some are required for LoFASM filterbanks; see below.  The comments and
final metadata line are referred to as the header block, and the
remaining portion is the data block.

At present the only BBX _encoding_ supported is `raw256`, specifying
that the _data_ block consists of the raw bytes of the flattened bit
array (stepping though _dimN_ in the innermost loop and _dim1_ in the
outermost).

### ABX format

Although the BBX format is the LoFASM standard, in situations where
terminal- or email-safe files are required, there is the ASCII Bit
Array (ABX) format.  Files conventionally have a `.abx` filename
extension, the first header comment line is `%ABX`, and the _encoding_
is one of several ASCII encoding schemes.  See abx(5) documentation
for more details.


<a id="lofasm-metadata"></a>
## LoFASM Metadata

The following discussion specifies the *additional* requirements on
BBX files used to store LoFASM filterbanks (spectrograms).

### Array Dimensions

LoFASM filterbanks are spectrograms with time as the first dimension
and frequency as the second dimension, storing either real power
spectra or complex cross spectra computed at equal-spaced time steps
and frequency bins.  In the array dimension metadata line, _dim1_ will
be the number of time steps and _dim2_ the number of frequency bins in
the file.

The _dim3_ indicates the number of physical quantities (components)
stored at each time and frequency.  This is 1 for real power spectra,
2 (real and imaginary components) for complex cross-spectra.

The final dimension _dim4_ is the bit depth of the data type used to
store the components.  Currently, for raw archived LoFASM data, this
is always 64 (storing a double-precision float).

### Comment Fields

While the ABX/BBX standard does not require *any* comment fields, the
standard "LoFASM-flavoured" BBX format *does*, in order for the
encoded data to be meaningful.  Following the standard `%\002BX\n`
"magic" byte sequence, LoFASM/BBX comments are of the form
`%`_key_`:`_value_, specifying some or all of the following keys.  The
order given here is conventional, but not mandated: a parser should
expect to read the *entire* header and parse its contents
non-sequentially.

`%hdr_type: LoFASM-filterbank` (required):
    Indicates this particular flavour of BBX file, and the expected
    fields.

`%hdr_version:` _version_ (required):
    Indicates the version of LoFASM-filterbank metadata.  The
    _version_ tag is 8 hexadecimal digit characters `0` through `F`
    encoding the version number as a single-precision float.  For
    example, version 1 (described by this document) is encoded as
    `0000803F`; see [Endianness](#endianness), below.

`%station:` _stationID_ (required):
    The LoFASM station identifier; currently one of `1`, `2`, `3`, or
    `4`.

`%channel:` _polarization_ (required):
    The channel or trunk-line ring/polarization ID tag for this data
    stream.  Conventionally _polarization_ values of `AA`, `BB`, `CC`,
    `DD` represent autocorrelation spectra (power spectra) of a
    specific polarization of a specific antenna ring, while `AB`,
    `AC`, `AD`, `BC`, `BD`, `CD` represent cross-correlation spectra
    (cross spectra) between different polarizations and/or different
    rings.  See documentation of specific LoFASM sites for definitions
    of these labels.

`%start_time:` _timestamp_ (optional):
    The start time of the first time sample of the spectrogram in this
    file, in a human-readable format.  Note that this is provided
    solely for the convenience of users scanning file headers by eye;
    analysis software should use instead the combination of
    `%time_offset_J2000:` and `%dim1_start:` fields to determine the
    actual start time.

`%time_offset_J2000:` _toffset_ `(`_units_`)` (optional):
    The reference time for this file (usually the same across a group
    of files), relative to the standard astronomical J2000 epoch.
    _toffset_ should be a number parseable as a double-precision float
    (real64), while _units_ should be a standard SI time unit.  Note
    that the actual start of data is given by the `%dim1_start:`
    comment (below), relative to this reference time.  At present, all
    raw archived LoFASM data files use `0 (s)` as their offset; see
    [Time and Frequency References](#time-and-frequency-references),
    below.  If the comment is absent, this is assumed.

`%frequency_offset_DC:` _foffset_ `(`_units_`)` (optional):
    The reference frequency for this file (usually the same across a
    group of files), relative to the true physical zero frequency
    (DC).  Note that the actual start of data is given by the
    `%dim2_start:` comment (below), relative to this reference
    frequency.  At present, all raw archived LoFASM data files use `0
    (Hz)` as their offset; this is the assumption if the comment is
    absent.

`%dim1_label: time (s)` (optional):
    The physical quantity and units for the first dimension of the
    data array.  For LoFASM filterbanks, the first dimension is always
    time, in units of seconds; this is also the assumption if the
    comment is absent.

`%dim1_start:` _tstart_ (required):
    The start time of the first time sample of the spectrogram in this
    file, relative to the reference time specified by
    `%time_offset_J2000:` (above).  It should be parsed as a
    double-precision float (real64), in the units specified by the
    preceding `%dim1_label:`.  See [Time and Frequency
    References](#time-and-frequency-references), below.

`%dim1_span:` _tspan_ (required):
    The total time span covered by this this file, parsed as a
    double-precision float (real64), in the units specified by the
    preceding `%dim1_label:`.  The stepsize between successive time
    samples in the spectrogram will be *tspan*/*dim1*, where _dim1_ is
    the first array dimension length.

`%dim2_label: frequency (Hz)` (optional):
    The physical quantity and units for the second dimension of the
    data array.  For LoFASM filterbanks, the second dimension is
    always frequency, in units of hertz; this is also the assumption
    if the comment is absent.

`%dim2_start:` _fstart_ (required):
    The frequency of the first frequency sample of the spectrogram in
    this file, relative to the reference frequency specified by
    `%frequency_offset_DC:` (above).  It should be parsed as a
    double-precision float (real64), in the units specified by the
    preceding `%dim2_label:`.  See [Time and Frequency
    References](#time-and-frequency-references), below.

`%dim2_span:` _fspan_ (required):
    The total frequency span covered by this this file, parsed as a
    double-precision float (real64), in the units specified by the
    preceding `%dim2_label:`.  The stepsize between successive
    frequency bins in the spectrogram will be *fspan*/*dim2*, where
    _dim2_ is the second array dimension length.

`%data_label:` _quantity_ `(`_units_`)` (optional):
    The physical quantity and units for the values stored in the data
    array.  For LoFASM filterbanks, _quantity_ is either
    `power spectrum` or `cross spectrum`.  Data coming directly off of
    the instrument normally has _units_ of `arbitrary`: calibration to
    physical SI units occurs downstream in the data analysis chain.
    This comment field is important mainly for post-calibration data
    products, not for archived raw LoFASM data.

`%data_offset:` _doff_ (optional):
    Any offset required to convert the stored data into a rational
    physical quantity in the units specified in the `%data_label:`.
    See below.

`%data_scale:` _dscale_ (optional):
    Any scale factor required to convert the stored data into a
    physical quantity with the units specified in the `%data_label:`.

    The combination of `%data_offset:` and `%data_scale:` specifies a
    linear combination required to convert the recorded number into a
    rational physical quantity:

>>  _quantity_ = ( _doff_ + _dscale_ x _datum_ ) _units_

:
    If absent, _doff_=0 and _dscale_=1 are assumed.  For uncalibrated
    data, these numbers are meaningless.

`%data_type:` _type_ (required):
    The storage type used to represent the data, usually one of
    `real32` or `real64`, or occasionally `int32` or `int64`.  Other
    types may be used for special applications.  Raw archived data is
    currently always `real64`.  In any case, the number in the type
    specification should be the bit depth of the storage type, and
    should always match the final dimension length _dimN_ in the bit
    array dimensions line.

Other comments may appear in LoFASM data files, so long as they do not
conflict with the above comment fields.  General-purpose parsers
should present all comments to the calling routine; higher level
routines or parsers for specific applications may skip over
unrecognized fields, but should tolerate their presence.

Applications using the LoFASM data format to store non-filterbank data
may modify some of these fields while maintaining others.  The
`%hdr_type:` field should be adjusted accordingly.


<a id="time-and-frequency-references"></a>
### Time and Frequency References ###

As noted above, LoFASM data files currently fix the reference time
(epoch) to be exactly the J2000 epoch (i.e. Julian date 2451545.0),
and the reference frequency to be DC.  Spectrogram start times and
lower frequency bounds are specified by double-precision floats read
from the `%dim1_start:` and `%dim2_start:` comment headers.

This makes it simple to compare frequencies and start times between
two LoFASM files, and is adequate for spectrograms that have been
downsampled to timesteps on the order of milliseconds.  However, note
that a double-precision time offset, starting from J2000, will
currently only allow timestamps that are multiples of 2^-25 s (about
30ns).  After early January 2017, timestamps will always be multiples
of 2^-24 s (about 60 ns).  In principle, then, if LoFASM were to
produce baseband data files with timestamps precise to within
1/(200MHz) = 5ns, more precision would be needed.

The `%time_offset_J2000:` field allows this by specifying a different
reference time for a given data file or group of files.  For instance,
by specifying the preceding midnight as the time reference, any
`%dim1_start:` within that day would be precise to within about 14.5
picoseconds: enough to perform interferometry down to wavelengths of a
few millimeters or frequencies of a few tens of GHz.

The value of the `%time_offset_J2000:` should convert to an exact
integer number of seconds, and any comparisons among epochs must be
done to exact precision, with any floating-point uncertainty in the
timestamp is thus entirely subsumed in the `%dim1_start:` field.  If
_toffset_ is stored as a double-precision float in units of seconds,
integer operations will still be exact for timestamps within 2^53
seconds (about 285 million years) of the J2000 epoch.  The requirement
that _toffset_ be an integer number of seconds limits the precision of
`%dim1_start:` to 2^-53 seconds (around 0.1 femtoseconds, or 33
nanometres wavelength), but we are unlikely to require or even be
capable of such timing precision for the foreseeable future.

The relationship between `%frequency_offset_DC:` and `%dim2_start:` is
analogous.  However, since LoFASM signals all lie within 100MHz of DC,
a double-precision `%dim2_start:` field alone specifies frequencies to
within about 7.5 nHz, which is more than enough precision for LoFASM
purposes (it would take about 4 years of data to be able to resolve
frequency differences this small).  This field would potentially be
useful for other instruments that generate high-frequency but
low-bandwidth heterodyned data.

The human-readable `%start_time:` field is only for the convenience of
users examining file headers by eye, and should not be used for
numerical analysis.  As such, it is optional, and its precise format
and precision are not specified.  However, current archived LoFASM
files use UTC timestamps following the ISO 8601 extended standard with
microsecond precision:

> *yyyy*-*mm*-*dd*T*hh*:*mm*:*ss*.*ssssss*Z

<a id="endianness"></a>
### Endianness ###

"Endianness" refers to the order in which individual bytes of a
multi-byte storage type are indexed in memory.  The intuitive order,
with the most significant byte of a multi-byte integer appearing
first, is known as "big-endian", and is the standard convention for
many network storage and transfer applications.  However, the common
Intel x86 processors (used by the LoFASM control computers and most
computers currently employed for LoFASM data analysis) are
"little-endian", with the *least* significant byte appearing first.
Therefore, for ease of reading and writing by LoFASM systems, LoFASM
archival data files have their encoded byte sequences in little-endian
order.  This means that, on x86 systems, you should be able to read
the binary data block of a LoFASM file directly into memory, assign it
to a double-precision float array, and use the data without further
conversion.

To check that your processor has the same endianness as a LoFASM file
being read, you should explicitly examine the `%hdr_version:` comment
field: Convert the 8 hexadecimal digits of the version tag into 4
consecutive bytes, cast this to a single-precision floating-point
number, and see that the resulting number is an exact positive integer
less than 256.  (Note that the first 512 integers, when stored as
floats and byte-swapped, convert to tiny fractions of magnitude 3e-38
or less, and thus could not be mistaken for a valid version number.)
The general C codes available for parsing LoFASM data include this
check.

Similarly, when writing LoFASM data, you should explicitly generate
the `%hdr_version:` tag on your machine, so that subsequent analysis
codes can check that they are being run on a machine with the same
endianness as was used to store the data.  Again, this is done
automatically by the provided C codes.

Endianness conversion utilities may be provided if and as they prove
necessary; for now, only the above consistency check is mandated.


<a id="gzip-utilities"></a>
gzip Utilities
--------------

GNU zip gzip(1) is a common compression format and suite of programs
available on a variety of platforms.  The zlib(3) C library and Python
module include functions for reading and writing gzip files.

### Useful Commands

The following commands will work from the command prompt of nearly any
Unix-style shell.  They may be placed in a bash(1) shell script for
ease of use.

  * To compress _filename_ replacing it with compressed
    _filename_`.gz`:

    >> `gzip` _filename_

  * To decompress _filename_`.gz` replacing it with _filename_:

    >> `gzip -d` _filename_`.gz`

    or, alternatively,  

    >> `gunzip` _filename_`.gz`

  * To decompress all or part of a file, preserving the original, use:

    >> `gunzip -c` _filename_`.gz` _output_specifier_

    where _output_specifier_ should be either a **redirection** to an
    output file "`>` _outfile_", or a **pipe** to another program's
    standard input "`|` _program_".

    **Warning:** Do not leave off the _output_specifier_.  Doing so
    will display the decompressed file on your terminal, most likely
    causing it to lock up.

  * To view the header of a LoFASM file _filename_`.gz` on the
    terminal:

    >> `gunzip -c` _filename_`.gz | sed -e '/^[^%]/q'`

    As above, the ouput may be redirected to a file or piped to a
    program rather than displaying on the screen, by appending a
    suitable output specifier.  However, on LoFASM-formatted files, it
    is safe to allow the output of this command to display on the
    terminal.

    Note that gunzip(1) need only decompress enough of the file to
    extract the header, which is computationally trivial compared to
    decompressing the entire file.

  * To split the header and data portions of _filename_`.gz` to
    _hdrfile_ and _datfile_, respectively, preserving the original
    file:

    >> `gunzip -c` _filename_`.gz | sed -n -e '1,/^[^%]/w` _hdrfile_  
    >> `/^[^%]/,$p | tail -n +2 >` _datfile_

    Note that the linebreak in the above code block *is* significant.

### Useful Functions

The C module `lofasmIO.c` and associated header file `lofasmIO.h`
provide useful functions for reading and writing LoFASM-formatted
data.  See the documentation in those files for details.  Some things
to note:

  * Use the lfopen(3) function to open a named file that may or may
    not be compressed with gzip(1).  When compiled with zlib(3), this
    will transparently decompress or compress all reading or writing
    calls to that file pointer.

  * Use the lfdopen(3) function to perform a similar operation on
    other already-open file streams, such as standard input or
    standard output.

## See Also

gzip(1),
lfopen(3),
lfdopen(3),
lfbxRead(3),
lfbxWrite(3),
zlib(3),
bbx(5),
lfb_hdr(5)
