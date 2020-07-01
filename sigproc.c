#include <bsd/stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sigproc.h"

#if 0
inline double sigproc_ra(double ra) {
		float hh = (180.0f / M_PI) * (24.0f / 360.0f) * ra;
		float mm = (hh - int(hh))  * 60.0f;
		float ss = (mm - int(mm))  * 60.0f;
		return ( int(hh) * 1e4 ) + ( int(mm) * 1e2 ) + ss;
}

inline double sigproc_dec(double dec) {
		float dd = (180.0f / M_PI) * fabs(dec);
		float mm = (dd - int(dd))  * 60.0f;
		float ss = (mm - int(mm))  * 60.0f;
		return ( int(dd) * 1e4 ) + ( int(mm) * 1e2 ) + ss;
}
#endif

void send (const char * str, FILE * fp) {
		int len = strlen (str);
		// first goes the length of the str
		fwrite (&len, sizeof (int), 1, fp);
		// and then the str itself
		fwrite (str, sizeof(char), len, fp);
}

void sendf (float f, FILE * fp) {
		fwrite (&f, sizeof(float), 1, fp);
}

void sendi (int i, FILE * fp) {
		fwrite (&i, sizeof(int), 1, fp);
}

void sendd (double d, FILE * fp) {
		fwrite (&d, sizeof(double), 1, fp);
}

void write_header ( sfb_hdr* hp, FILE * fp ) {
		// dance 
		send ("HEADER_START", fp);
	 // source_name
	 send ("source_name", fp); 
	 send (hp->name, fp);
	 // barycentric
	 send  ("barycentric", fp);
	 sendi (0, fp);
	 // telescope ID
	 send  ("telescope_id", fp);
	 sendi (hp->stationid, fp);
	 // positions
	 send ("src_raj", fp);
	 sendd (0.0, fp);
	 send ("src_dej", fp);
	 sendd (0.0, fp);
	 // datatype
	 send ("data_type", fp);
	 sendi (1, fp);
	 // freq
	 send ("fch1", fp);
	 sendd (hp->fch1, fp);
	 send ("foff", fp);
	 sendd (hp->foff, fp);
	 // data
	 send ("nchans", fp);
	 sendi (hp->nchans, fp);
	 send ("nbits", fp);
	 sendi (hp->nbits, fp);
	 send ("nbits", fp);
	 sendi (hp->nbits, fp);
	 send ("nifs", fp);
	 sendi (1, fp);
	 // time
	 send ("tstart", fp);
	 sendd (hp->tstart, fp);
	 send ("tsamp", fp);
	 sendd (hp->tsamp, fp);
	 // end
		send ("HEADER_END", fp);
}

void lf2sg_hdr (lfb_hdr * l, sfb_hdr * s) {
		/* Pointing defaults since lofasm doesn't point */
		s->ra  = 0.0f;
		s->dec = 0.0f;
		/* Station ID defaults to 0 */
		s->stationid = 0;
		/* Name defaults to ABC */
		strcpy (s->name, "ABC");
		/* Dimensions */
		s->nchans = l->dims[1];
		s->npol   = l->dims[2];
		s->nifs   = 1;
		/* float */
		s->nbits  = 32;
		/* frequency going down */
		s->fch1  = ( l->frequency_offset_DC + l->dim2_start + l->dim2_span )/1E6;
		s->foff  = -1 * l->dim2_span / l->dims[1] / 1E6;
		/* time */
		s->tsamp  = l->dim1_span / l->dims[0];
		s->tstart = l->start_mjd;
}
