### Programs:

`lfslice(1)` - extract frequency band from LoFASM data  
`lfchop(1)` - extract frequency band from LoFASM data  
`lfcat(1)` - concatenate lofasm-filterbank(5) files  
`lftest(1)` - test lofasm routines  
`bxresample(1)` - resize abx(5) or bbx(5) files  
`lftype(1)` - change type of a lofasm-filterbank(5) or related file  
`lfplot2d(1)` - plot lofasm-filterbank(5) data in two dimensions  
`lfstats(1)` - get statistics of lofasm-filterbank(5) file  
`lfmed(1)` - perform running medians on a lofasm-filterbank(5) file  
`lfmean(1)` - perform running means on a lofasm-filterbank(5) file  
`lfplot(1)` - plot lofasm-filterbank(5) data  

### From  lofasmIO.h:

`lf_error(3), lf_warning(3), lf_info(3)` - print status messages  

### From  lofasmIO.c:

`lfopen(3)` - open a compressed or uncompressed file  
`lfdopen(3)` - allow compressed access to an open file descriptor  
`bxReadData(3)` - read data from an abx(5) or bbx(5) file  
`bxRead(3)` - read an abx(5) or bbx(5) file  
`bxSkipHeader(3)` - skip to start of ABX/BBX data block  
`bxWriteData(3)` - write data to an abx(5) or bbx(5) file  
`bxWrite(3)` - write an abx(5) or bbx(5) file  
`lfbxRead(3)` - read a BBX file with LoFASM filterbank header  
`lfbxWrite(3)` - write a BBX file with LoFASM filterbank header  
`lfbxFree(3)` - frees a LoFASM filterbank header  
