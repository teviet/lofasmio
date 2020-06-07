#ifndef _SIGPROC_H
#define _SIGPROC_H

#include "lofasmIO.h"

typedef struct Header {
  // station
  int stationid;
  // string
  char name[16];
  // positions
  double ra, dec;
  // frequency
  double fch1, foff;
  // time
  double tsamp, tstart;
  // memory
  int nbits, nchans, nifs, npol;
} sfb_hdr;


void lf2sg_hdr (lfb_hdr * l, sfb_hdr * s);

void write_header ( sfb_hdr * , FILE * );

#endif
