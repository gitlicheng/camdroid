#include <stdio.h>
#include "g72x.h"
#include "codec_g726.h"
#include <stdlib.h>
#include <assert.h>

typedef signed short sample_t;

typedef struct {
	struct g726_state_s  *gs;
	unsigned char bitrule;
	unsigned char coding;
} g726_t;

int	g726_state_create(unsigned char bitrule, unsigned char coding, unsigned char **state)
{
	g726_t *g;

	assert(bitrule>=G726_16 && bitrule<=G726_40);
	assert(coding==AUDIO_ENCODING_ALAW || coding==AUDIO_ENCODING_ULAW || coding==AUDIO_ENCODING_LINEAR);
	assert(state != NULL);

	g = (g726_t *)malloc(sizeof(g726_t));
	if(g == NULL) {
		return FALSE;
	}

	g->bitrule = bitrule;
	g->coding = coding;
	g->gs = (struct g726_state_s*)malloc(sizeof(struct g726_state_s));
	if(g->gs == NULL){
		free(g);
		return FALSE;
	}
	g726_init_state(g->gs);

	*state = (unsigned char*)g;
	return TRUE;
}

void	g726_state_destroy(unsigned char **state)
{
	g726_t *g;

	assert(state != NULL);

	g = (g726_t *)*state;
	free(g->gs);
	free(g);
	*state = (unsigned char*)NULL;
}

/* G726 packing: compatible with MPG440 ADPCM packing format */
static int g726_pack(unsigned char *buf, unsigned char *cw, unsigned char num_cw, int bps)
{
	switch(bps)
	{
	case 2: 
		assert(num_cw == 16);	

		buf[0] = (cw[0]<<6)  | (cw[1]<<4)  | (cw[2]<<2)  | (cw[3]);
		buf[1] = (cw[4]<<6)  | (cw[5]<<4)  | (cw[6]<<2)  | (cw[7]);
		buf[2] = (cw[8]<<6)  | (cw[9]<<4)  | (cw[10]<<2) | (cw[11]);
		buf[3] = (cw[12]<<6) | (cw[13]<<4) | (cw[14]<<2) | (cw[15]);

		break;
	case 3: assert(num_cw == 10);
	
		buf[0] = (cw[0]<<3 | cw[1]);
		buf[1] = (cw[2]<<5 | cw[3]<<2 | cw[4]>>1);
		buf[2] = (cw[4]<<7 | cw[5]<<4 | cw[6]<<1 | cw[7]>>2);
		buf[3] = (cw[7]<<6 | cw[8]<<3 | cw[9]);

		//buf[0] = 0;
		//buf[1] = 0;
		//buf[2] = 0;
		//buf[3] = 0;
		//                   //2
		//buf[0] |= cw[0]<<3;  //3
		//buf[0] |= cw[1];    //3

		//buf[1] |= cw[2]<<5;//3
		//buf[1] |= cw[3]<<2;//3
		//buf[1] |= cw[4]>>1;//2

		//buf[2] |= cw[4]<<7;//1
		//buf[2] |= cw[5]<<4;//3
		//buf[2] |= cw[6]<<1;//3
		//buf[2] |= cw[7]>>2;//1

		//buf[3] |= cw[7]<<6;//2
		//buf[3] |= cw[8]<<3;//3
		//buf[3] |= cw[9];   //3
		break;
	case 4: assert(num_cw == 8);
		buf[0] = (cw[0]<<4 | cw[1]);
		buf[1] = (cw[2]<<4 | cw[3]);
		buf[2] = (cw[4]<<4 | cw[5]);
		buf[3] = (cw[6]<<4 | cw[7] );

		//buf[0] |= cw[0]<<4; 
		//buf[0] |= cw[1];

		//buf[1] |= cw[2]<<4;
		//buf[1] |= cw[3];

		//buf[2] |= cw[4]<<4;
		//buf[2] |= cw[5];

		//buf[3] |= cw[6]<<4;
		//buf[3] |= cw[7];
		break;
	case 5: assert(num_cw == 6);
		buf[0] = (cw[0]<<1 | cw[1]>>4);
		buf[1] = (cw[1]<<4 | cw[2]>>1);
		buf[2] = (cw[2]<<7 | cw[3]<<2 | cw[4]>>3);
		buf[3] = (cw[4]<<5 | cw[5]);
		//			  //2
		//buf[0] |= cw[0]<<1; //5
		//buf[0] |= cw[1]>>4; //1

		//buf[1] |= cw[1]<<4; //4
		//buf[1] |= cw[2]>>1; //4
		//
		//buf[2] |= cw[2]<<7; //1
		//buf[2] |= cw[3]<<2; //5
		//buf[2] |= cw[4]>>3; //2

		//buf[3] |= cw[4]<<5; //3
		//buf[3] |= cw[5];    //5
		break;
	default:
		assert(FALSE);
		break;
	}

	return 4;
}


int	g726_encode(unsigned char *pHandle, unsigned char *inBuf, unsigned long inLen, unsigned char *outBuf, unsigned long *outLen)
{	
	int sampleCount;
	g726_t *g;
	int i;
	unsigned char cw[16];
	unsigned char *out;
	int consume=0;
	unsigned char coding;

	assert(pHandle != NULL);
	assert(inBuf != NULL);
	assert(outBuf != NULL);
	assert(inLen > 0);
	assert(outLen != NULL);	
	
	out = outBuf;
	g = (g726_t *)pHandle;
	coding = g->coding;

	if(coding == AUDIO_ENCODING_LINEAR)
	{
		register signed short *s;
		s = (signed short *)inBuf;

		sampleCount = inLen>>1;
		switch(g->bitrule) 
		{
		case G726_16:
			//sampleCount -= 16;
			for(i = 0; i < sampleCount; i += 16)
			{
				cw[0] = g726_16_encoder(s[0],  coding, g->gs);
				cw[1] = g726_16_encoder(s[1],  coding, g->gs);
				cw[2] = g726_16_encoder(s[2],  coding, g->gs);
				cw[3] = g726_16_encoder(s[3],  coding, g->gs);
				cw[4] = g726_16_encoder(s[4],  coding, g->gs);
				cw[5] = g726_16_encoder(s[5],  coding, g->gs);
				cw[6] = g726_16_encoder(s[6],  coding, g->gs);
				cw[7] = g726_16_encoder(s[7],  coding, g->gs);
				cw[8] = g726_16_encoder(s[8],  coding, g->gs);
				cw[9] = g726_16_encoder(s[9],  coding, g->gs);
				cw[10] = g726_16_encoder(s[10], coding, g->gs);
				cw[11] = g726_16_encoder(s[11], coding, g->gs);
				cw[12] = g726_16_encoder(s[12], coding, g->gs);
				cw[13] = g726_16_encoder(s[13], coding, g->gs);
				cw[14] = g726_16_encoder(s[14], coding, g->gs);
				cw[15] = g726_16_encoder(s[15], coding, g->gs);

				s += 16;
				out += g726_pack(out, cw, 16, 2);	
				consume += 32;
			}
			break;

		case G726_24:
			//sampleCount -= 10;
			for(i = 0; i < sampleCount; i+=10)
			{
				cw[0] = g726_24_encoder(s[0], coding, g->gs);
				cw[1] = g726_24_encoder(s[1], coding, g->gs);
				cw[2] = g726_24_encoder(s[2], coding, g->gs);
				cw[3] = g726_24_encoder(s[3], coding, g->gs);
				cw[4] = g726_24_encoder(s[4], coding, g->gs);

				cw[5] = g726_24_encoder(s[5], coding, g->gs);
				cw[6] = g726_24_encoder(s[6], coding, g->gs);
				cw[7] = g726_24_encoder(s[7], coding, g->gs);
				cw[8] = g726_24_encoder(s[8], coding, g->gs);
				cw[9] = g726_24_encoder(s[9], coding, g->gs);

				s += 10;
				out += g726_pack(out, cw, 10, 3);
				consume += 20;
			}
			break;
		case G726_32:
			//sampleCount -= 8;
			for(i = 0; i < sampleCount; i+=8)
			{
				cw[0] = g726_32_encoder(s[0], coding, g->gs);
				cw[1] = g726_32_encoder(s[1], coding, g->gs);
				cw[2] = g726_32_encoder(s[2], coding, g->gs);
				cw[3] = g726_32_encoder(s[3], coding, g->gs);
				cw[4] = g726_32_encoder(s[4], coding, g->gs);

				cw[5] = g726_32_encoder(s[5], coding, g->gs);
				cw[6] = g726_32_encoder(s[6], coding, g->gs);
				cw[7] = g726_32_encoder(s[7], coding, g->gs);

				s += 8;
				out += g726_pack(out, cw, 8, 4);
				consume += 16;
			}
			break;
		case G726_40:
			//sampleCount -= 6;
			for(i = 0; i < sampleCount; i+=6)
			{
				cw[0] = g726_40_encoder(s[0], coding, g->gs);
				cw[1] = g726_40_encoder(s[1], coding, g->gs);
				cw[2] = g726_40_encoder(s[2], coding, g->gs);
				cw[3] = g726_40_encoder(s[3], coding, g->gs);
				cw[4] = g726_40_encoder(s[4], coding, g->gs);

				cw[5] = g726_40_encoder(s[5], coding, g->gs);

				s += 6;
				out += g726_pack(out, cw, 6, 5);
				consume += 12;
			}
			break;
		default:
			assert(FALSE);
			break;
		}
	}
	else
	{
		register unsigned char* s2;
		s2 = (unsigned char *)inBuf;
		sampleCount = inLen;

		switch(g->bitrule) 
		{
		case G726_16:
			//sampleCount -= 16;
			for(i = 0; i < sampleCount; i += 16)
			{
				cw[0] = g726_16_encoder(s2[0],  coding, g->gs);
				cw[1] = g726_16_encoder(s2[1],  coding, g->gs);
				cw[2] = g726_16_encoder(s2[2],  coding, g->gs);
				cw[3] = g726_16_encoder(s2[3],  coding, g->gs);
				cw[4] = g726_16_encoder(s2[4],  coding, g->gs);
				cw[5] = g726_16_encoder(s2[5],  coding, g->gs);
				cw[6] = g726_16_encoder(s2[6],  coding, g->gs);
				cw[7] = g726_16_encoder(s2[7],  coding, g->gs);
				cw[8] = g726_16_encoder(s2[8],  coding, g->gs);
				cw[9] = g726_16_encoder(s2[9],  coding, g->gs);
				cw[10] = g726_16_encoder(s2[10], coding, g->gs);
				cw[11] = g726_16_encoder(s2[11], coding, g->gs);
				cw[12] = g726_16_encoder(s2[12], coding, g->gs);
				cw[13] = g726_16_encoder(s2[13], coding, g->gs);
				cw[14] = g726_16_encoder(s2[14], coding, g->gs);
				cw[15] = g726_16_encoder(s2[15], coding, g->gs);

				s2 += 16;
				out += g726_pack(out, cw, 16, 2);	
				consume += 16;
			}
			break;
		case G726_24:
			//sampleCount -= 10;
			for(i = 0; i < sampleCount; i+=10)
			{
				cw[0] = g726_24_encoder(s2[0], coding, g->gs);
				cw[1] = g726_24_encoder(s2[1], coding, g->gs);
				cw[2] = g726_24_encoder(s2[2], coding, g->gs);
				cw[3] = g726_24_encoder(s2[3], coding, g->gs);
				cw[4] = g726_24_encoder(s2[4], coding, g->gs);

				cw[5] = g726_24_encoder(s2[5], coding, g->gs);
				cw[6] = g726_24_encoder(s2[6], coding, g->gs);
				cw[7] = g726_24_encoder(s2[7], coding, g->gs);
				cw[8] = g726_24_encoder(s2[8], coding, g->gs);
				cw[9] = g726_24_encoder(s2[9], coding, g->gs);

				s2 += 10;
				out += g726_pack(out, cw, 10, 3);
				consume += 10;
			}
			break;
		case G726_32:
			//sampleCount -= 8;
			for(i = 0; i < sampleCount; i+=8)
			{
				cw[0] = g726_32_encoder(s2[0], coding, g->gs);
				cw[1] = g726_32_encoder(s2[1], coding, g->gs);
				cw[2] = g726_32_encoder(s2[2], coding, g->gs);
				cw[3] = g726_32_encoder(s2[3], coding, g->gs);
				cw[4] = g726_32_encoder(s2[4], coding, g->gs);

				cw[5] = g726_32_encoder(s2[5], coding, g->gs);
				cw[6] = g726_32_encoder(s2[6], coding, g->gs);
				cw[7] = g726_32_encoder(s2[7], coding, g->gs);

				s2 += 8;
				out += g726_pack(out, cw, 8, 4);
				consume += 8;
			}
			break;
		case G726_40:
			//sampleCount -= 6;
			for(i = 0; i < sampleCount; i+=6)
			{
				cw[0] = g726_40_encoder(s2[0], coding, g->gs);
				cw[1] = g726_40_encoder(s2[1], coding, g->gs);
				cw[2] = g726_40_encoder(s2[2], coding, g->gs);
				cw[3] = g726_40_encoder(s2[3], coding, g->gs);
				cw[4] = g726_40_encoder(s2[4], coding, g->gs);

				cw[5] = g726_40_encoder(s2[5], coding, g->gs);

				s2 += 6;
				out += g726_pack(out, cw, 6, 5);
				consume += 6;
			}
			break;
		default:
			assert(FALSE);
			break;
		}
	}


	*outLen =  (out - outBuf);
	return consume;
}

int	g726_decode(unsigned char *pHandle, unsigned char *inBuf, unsigned long inLen, unsigned char *outBuf, unsigned long *outLen)
{
	unsigned char *In, *InEnd;
	
	g726_t *g;
	int consume=0;
	unsigned char coding;
	
	assert(pHandle != NULL);
	assert(inBuf != NULL);
	assert(outBuf != NULL);
	assert(outLen != NULL);
	
	g = (g726_t *)pHandle;
	coding = g->coding;

	In = inBuf;
	InEnd = inBuf + inLen;
	
	*outLen = 0;

	if(coding == AUDIO_ENCODING_LINEAR){
		sample_t *Out;
		Out = (signed short *)outBuf;
		switch(g->bitrule)
		{
		case G726_16:
			for (;In<InEnd;++In,Out+=4)
			{
				Out[0] = (sample_t)g726_16_decoder(In[0] >> 6, coding, g->gs);
				Out[1] = (sample_t)g726_16_decoder(In[0] >> 4, coding, g->gs);
				Out[2] = (sample_t)g726_16_decoder(In[0] >> 2, coding, g->gs);
				Out[3] = (sample_t)g726_16_decoder(In[0], coding, g->gs);

				consume++;
				*outLen += 8;
			}
			break;
		case G726_24:
			InEnd -= 4;
			for (;In<InEnd;In+=4,Out+=10)
			{
				Out[0] = (sample_t)g726_24_decoder(In[0] >> 3,coding, g->gs);
				Out[1] = (sample_t)g726_24_decoder(In[0], coding, g->gs);
				Out[2] = (sample_t)g726_24_decoder(In[1]>>5, coding, g->gs);
				Out[3] = (sample_t)g726_24_decoder(In[1]>>2, coding, g->gs);
				Out[4] = (sample_t)g726_24_decoder((In[1]<<1) | (In[2]>>7), coding, g->gs);
				Out[5] = (sample_t)g726_24_decoder(In[2]>>4, coding, g->gs);
				Out[6] = (sample_t)g726_24_decoder(In[2]>>1, coding, g->gs);
				Out[7] = (sample_t)g726_24_decoder((In[2]<<2) | (In[3]>>6), coding, g->gs);
				Out[8] = (sample_t)g726_24_decoder(In[3]>>3, coding, g->gs);
				Out[9] = (sample_t)g726_24_decoder(In[3], coding, g->gs);

				consume += 4;
				*outLen += 20;
			}
			break;
		case G726_32:
			for (;In<InEnd;++In,Out+=2)
			{
				Out[0] = (sample_t)g726_32_decoder(In[0] >> 4, coding, g->gs);
				Out[1] = (sample_t)g726_32_decoder(In[0], coding, g->gs);

				consume++;
				*outLen += 4;
			}
			break;
		case G726_40:
			InEnd -= 4;
			for (;In<InEnd;In+=4,Out+=6)
			{
				Out[0] = (sample_t)g726_40_decoder(In[0] >> 1,coding, g->gs);
				Out[1] = (sample_t)g726_40_decoder((In[0] << 4) | (In[1] >> 4),coding, g->gs);
				Out[2] = (sample_t)g726_40_decoder(((In[1] << 1) | (In[2] >> 7)),coding, g->gs);
				Out[3] = (sample_t)g726_40_decoder((In[2] >> 2),coding, g->gs);
				Out[4] = (sample_t)g726_40_decoder((In[2] << 3) | (In[3] >> 5),coding, g->gs);
				Out[5] = (sample_t)g726_40_decoder(In[3],coding, g->gs);

				consume += 4;
				*outLen += 12;
			}
			break;
		}
	}
	else{
		unsigned char *Out;
		Out = (unsigned char *)outBuf;
		switch(g->bitrule)
		{
		case G726_16:
			for (;In<InEnd;++In,Out+=4)
			{
				Out[0] = (sample_t)g726_16_decoder(In[0] >> 6, coding, g->gs);
				Out[1] = (sample_t)g726_16_decoder(In[0] >> 4, coding, g->gs);
				Out[2] = (sample_t)g726_16_decoder(In[0] >> 2, coding, g->gs);
				Out[3] = (sample_t)g726_16_decoder(In[0], coding, g->gs);

				consume++;
				*outLen += 4;
			}
			break;
		case G726_24:
			InEnd -= 4;
			for (;In<InEnd;In+=4,Out+=10)
			{
				Out[0] = (sample_t)g726_24_decoder(In[0] >> 3,coding, g->gs);
				Out[1] = (sample_t)g726_24_decoder(In[0], coding, g->gs);
				Out[2] = (sample_t)g726_24_decoder(In[1]>>5, coding, g->gs);
				Out[3] = (sample_t)g726_24_decoder(In[1]>>2, coding, g->gs);
				Out[4] = (sample_t)g726_24_decoder((In[1]<<1) | (In[2]>>7), coding, g->gs);
				Out[5] = (sample_t)g726_24_decoder(In[2]>>4, coding, g->gs);
				Out[6] = (sample_t)g726_24_decoder(In[2]>>1, coding, g->gs);
				Out[7] = (sample_t)g726_24_decoder((In[2]<<2) | (In[3]>>6), coding, g->gs);
				Out[8] = (sample_t)g726_24_decoder(In[3]>>3, coding, g->gs);
				Out[9] = (sample_t)g726_24_decoder(In[3], coding, g->gs);

				consume += 4;
				*outLen += 10;
			}
			break;
		case G726_32:
			for (;In<InEnd;++In,Out+=2)
			{
				Out[0] = (sample_t)g726_32_decoder(In[0] >> 4, coding, g->gs);
				Out[1] = (sample_t)g726_32_decoder(In[0], coding, g->gs);

				consume++;
				*outLen += 2;
			}
			break;
		case G726_40:
			InEnd -= 4;
			for (;In<InEnd;In+=4,Out+=6)
			{
				Out[0] = (sample_t)g726_40_decoder(In[0] >> 1,coding, g->gs);
				Out[1] = (sample_t)g726_40_decoder((In[0] << 4) | (In[1] >> 4),coding, g->gs);
				Out[2] = (sample_t)g726_40_decoder(((In[1] << 1) | (In[2] >> 7)),coding, g->gs);
				Out[3] = (sample_t)g726_40_decoder((In[2] >> 2),coding, g->gs);
				Out[4] = (sample_t)g726_40_decoder((In[2] << 3) | (In[3] >> 5),coding, g->gs);
				Out[5] = (sample_t)g726_40_decoder(In[3],coding, g->gs);

				consume += 4;
				*outLen += 6;
			}
			break;
		}
	}

	return consume;
}


