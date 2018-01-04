#ifndef _CODEC_G726_H_
#define _CODEC_G726_H_

#ifndef NULL
#define NULL	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define	AUDIO_ENCODING_ULAW	(0)	/* ISDN u-law */
#define	AUDIO_ENCODING_ALAW	(1)	/* ISDN A-law */
#define	AUDIO_ENCODING_LINEAR	(2)	/* PCM 2's-complement (0-center) */

#define G726_16        0
#define G726_24        1
#define G726_32        2
#define G726_40        3

#ifdef __cplusplus
extern "C"
{
#endif



int g726_state_create(unsigned char bitrule, unsigned char format, unsigned char **ppHandle);
void g726_state_destroy(unsigned char **ppHandle);
int g726_encode(unsigned char *pHandle, unsigned char *inBuf, unsigned long inLen, unsigned char *outBuf, unsigned long *outLen);
int g726_decode(unsigned char *pHandle, unsigned char *inBuf, unsigned long inLen, unsigned char *outBuf, unsigned long *outLen);



#ifdef __cplusplus
}
#endif


#endif /* _CODEC_G726_H_ */

