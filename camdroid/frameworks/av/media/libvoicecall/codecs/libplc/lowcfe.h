/*
  ============================================================================
   File: lowcfe.h                                            V.1.0-24.MAY-2005
  ============================================================================

                     UGST/ITU-T G711 Appendix I PLC MODULE

                          GLOBAL FUNCTION PROTOTYPES

   History:
   24.May.05	v1.0	First version <AT&T>
						Integration in STL2005 <Cyril Guillaume & Stephane Ragot - stephane.ragot@francetelecom.com>
  ============================================================================
*/
#ifndef __LOWCFE_C_H__
#define __LOWCFE_C_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LowcFE_c {
	int PITCH_MIN;		/* minimum allowed pitch, 100 Hz */
	int PITCH_MAX;		/* maximum allowed pitch, 66 Hz */
	int PITCH_DIFF;
	int POVERLAP_MAX;	/* maximum pitch OLA window */
	int HISTORY_LEN;	/* history buffer length*/
	int NDEC;
	int	CORR_LEN;		/* 20 msec correlation length */
	int CORR_BUF_LEN;	/* correlation buffer length */
	int	CORR_MIN_POWER;	/* minimum power */
	int	EOVERLAP_INCR;	/* end OLA increment per frame, 4ms */
	int	FRAME_SIZE;		/* 10 msec */

	int	erasecnt;		/* consecutive erased frames */
	int	poverlap;		/* overlap based on pitch */
	int	poffset;		/* offset into pitch period */
	int	pitch;			/* pitch estimate */
	int	pitch_buf_len;		/* current pitch buffer length */

	short	*pitch_buf_end;		/* end of pitch buffer */
	short	*pitch_buf_start;	/* start of pitch buffer */
	short	*pitch_buf;			/* buffer for cycles of speech */
	short	*last_q;			/* saved last quarter wavelengh */
	short	*tmp;				/* temp buffer */
	short	*history;			/* history buffer */

	short	atten_fac;
	short	atten_incr;

} LowcFE_c;

/* public functions */
LowcFE_c* g711plc_init(int sample_rate, int frame_size);	/* constructor */
void g711plc_destroy(LowcFE_c*);	/*cleanup */
void g711plc_dofe(LowcFE_c*, short *s);	/* synthesize speech for erasure */
void g711plc_addtohistory(LowcFE_c*, short *s);
		/* add a good frame to history buffer */

#ifdef __cplusplus
}
#endif

#endif  /* __LOWCFE_C_H__ */
