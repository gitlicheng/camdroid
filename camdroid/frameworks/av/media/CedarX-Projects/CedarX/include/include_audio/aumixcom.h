
#ifndef	___aumixcommon__
#define	___aumixcommon__
typedef	struct __PcmInfo__{
//input infor
	unsigned int		SampleRate;
	short*	PCMPtr;
	unsigned int		PcmLen;
	unsigned int		Chan;
}PcmInfo;

typedef	struct ___AudioMix__{
//input para
	PcmInfo	*OutputA;
	PcmInfo	*InputB;
	PcmInfo	*TempBuf;
	//PcmInfo	*Output;
//	unsigned int	ByPassflag; //0: need mix A+B ==¡·A  1£ºbypass   A==>A
//output para
	short*	MixbufPtr;
	unsigned int	MixLen;
	unsigned int    Mixch;

}AudioMix;
int		do_AuMIX(AudioMix	*AMX);
int		AudioResample(PcmInfo *Input,PcmInfo *Output);
#endif
