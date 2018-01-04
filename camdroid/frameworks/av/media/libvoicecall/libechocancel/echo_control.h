
#ifndef _ECHO_CONTROL_H_
#define _ECHO_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* EQ filter type definition */
typedef enum
{
    /* low pass shelving filter */
    LOWPASS_SHELVING,
    /* band pass peak filter */
    BANDPASS_PEAK,
    /* high pass shelving filter */
    HIHPASS_SHELVING
}eq_ftype_t;

/* overlap of window */
typedef enum
{
    OVERLAP_FIFTY,
    OVERLAP_SEVENTY_FIVE,
}overlap_t;

/* noise type for noise suppression */
typedef enum
{
    STATIONAL,
    LOW_NONSTATIONAL,
    MEDIUM_NONSTATIONAL,
    HIGH_NONSTATIONAL

}sta_t;

/* dynamic range control parameters */
typedef struct
{
    /* sampling rate */
    int sampling_rate;
    /* */
    int target_level;
    /* */
    int max_gain;
    /* */
    int min_gain;
    /* */
    int attack_time;
    /* */
    int release_time;
    /* */
    int noise_threshold;

}drcLog_prms_t;

/* bulk delay compensation parameters */
typedef struct
{
    /* sampling rate, should be equal to the SR of echo canceller */
    int sampling_rate;
    /* processing block size, should be equal to the frame size of echo canceller */
    int block_size;
    /* max delay for the far end signal */
    int max_delay;
    /*  delay magin, this is used for delay estimation variation
        in some case, the variation could cause uncaulity
        delay_margin shold be positive and litter than max_delay
    */
    int delay_margin;
    /* delay for the near end signal */
    int near_delay;
}bdc_prms_t;

/* noise suppression parameters */
typedef struct
{
    /* sampling rate */
    int sampling_rate;
    /* maximum amount of suppression allowed for noise reduction */
    int max_suppression;
    /* overlap of window */
    overlap_t overlap_percent;
    /* stationarity of noise */
    sta_t nonstat;
}ns_prms_t;


/* equalizer parameters */
typedef struct
{
    /* boost/cut gain in dB */
    int G;
    /* cutoff/center frequency in Hz */
    int fc;
    /* quality factor */
    int Q;
    /* filter type */
    eq_ftype_t type;
}eq_core_prms_t;

typedef struct
{
    /*num of items(biquad)*/
    int biq_num;
    /* sampling rate */
    int sampling_rate;
    /* eq parameters for generate IIR coeff*/
    eq_core_prms_t* core_prms;
}eq_prms_t;

/* Nonlinear Processing parameters parameters */
typedef struct
{
    /* nlp overlap of window */
    overlap_t nlp_overlap_percent;
}nlp_prms_t;


/* echo cancellation parameters */
typedef struct
{
    /* sampling rate */
    int sampling_rate;
    /* echo path length, Unit ms, should be times of 8 */
    int tail_length;
    /* processing frame size, Unit ms, only 8ms is supported until now */
    int frame_size;
    /* Nonlinear Processing switch */
    int enable_nlp;
    /* Nonlinear Processing parameters */
    nlp_prms_t nlp_prms;
}aec_prms_t;

/* total init parameters for echo control */
typedef struct
{

    /* echo canceller switch */
    int enable_aec;
    /* echo canceller parameters */
    aec_prms_t aec_prms;
    /* bulk delay compensator switch */
    int enable_bdc;
    /* bulk delay estimator parameters */
    bdc_prms_t bdc_prms;
    /* clock drift compensator switch */
    int enable_cdc;
    /* dynamic range control switch */
    int enable_txdrc;
    /* transmitter drc  parameters */
    drcLog_prms_t txdrc_prms;
    /* receiver drc */
    int enable_rxdrc;
    /*receiver drc  parameters */
    drcLog_prms_t rxdrc_prms;
    /* transmitter equalizer */
    int enable_txeq;
    /* transmitter equalizer parameters */
    eq_prms_t txeq_prms;
    /* receiver equalizer */
    int enable_rxeq;
    /* receiver equalizer parameters */
    eq_prms_t rxeq_prms;
    /* noise suppressor switch */
    int enable_ns;
    /* noise suppression parameters */
    ns_prms_t ns_prms;
    /* fade in to suppress the initial echo */
    int enable_txfade;
}ec_prms_t;

/* control command for echo control */
typedef enum
{

    /* enum code for GET */
    /* get the delay estimator data*/
    EC_BDC_GET_DELAY=0XA0101,
    /* get the delay estimator data size, in bytes */
    EC_BDC_GET_DELAYSIZE,

    /* get the size of impulse response in bytes */
    EC_AEC_GET_IRSIZE=0xA0201,
    /* get the impulse response of the adaptive filter(echo path)*/
    EC_AEC_GET_IR,
    /* get the frequency domain coeff size in bytes */
    EC_AEC_GET_FCOEFFSIZE,
    /* get the frequency domain coeffs */
    EC_AEC_GET_FCOEFF,
    /* get the nonlinearprocessing bypass mode */
    EC_AEC_GET_NLPBYPASS,

    /* get the noise suppression bypass mode */
    EC_NS_GET_BYPASS=0XA0301,
    /* get the noise suppression level */
    EC_NS_GET_NSLEVEL,


    /* enum code for SET */
    /* set the delay estimator data */
    EC_BDC_SET_DELAY=0XB0101,
    /* set bdc delay estimator datasize */
    EC_BDC_SET_DELAYSIZE,

    /* set the size of impulse response in bytes*/
    EC_AEC_SET_IRSIZE=0XB0201,
    /* set the time domain impluse response of the adaptive filter */
    EC_AEC_SET_IR,
    /* set the frequency domain coeffs size in bytes */
    EC_AEC_SET_FCOEFFSIZE,
    /* set the frequency domain coeffs of the adaptive filter */
    EC_AEC_SET_FCOEFF,
    /* set the nonlinearprocessing bypass mode */
    EC_AEC_SET_NLPBYPASS,

    /* set the noise suppression bypass mode */
    EC_NS_SET_BYPASS=0XB0301,
    /* set the noise suppression level */
    EC_NS_SET_NSLEVEL,


}ec_cmd_t;

/*
    description:
        create and initialise echo controller handle
    prms:
        prms: settings
    return value:
        echo controller handle

*/
void* ec_create(ec_prms_t* prms);
/*
    description
        decreate echo controller handle
    prms:
        handle: the handle
    return value:
        none
*/
void ec_destroy(void* handle);
/*
    description:
        process one frame
    prms:
        handle: then ec handle
        far_signal: far end signal, whose length should be equal to frame size when do initialization
        near_signal: mic signal, whose length should be equal to frame size
        out_transmitter: the processed signal, which will be transmit to far end
        out_receiver: the processed signal, which will be playbacked in the speaker
    return value:
        -1: some error
        0: success
*/
int ec_process_frame(void* handle, short* near_signal, short* far_signal, short* out_transmitter, short* out_receiver);

/*
    description
        control command
    prms
        handle: ec handle
        cmd:    command type
        arg:    arguments according to the command type
*/
int ec_command(void* handle, ec_cmd_t cmd, void* arg);

/*
    调用示例

    FILE* fspk;
    FILE* fmic;
    FILE* fout;
    void* echo_controller = NULL;
    ec_prms_t ec_prms;
    int item1, items2, bufsize;
    short* playbuf;
    short* captbuf;
    short* outbuf

    fspk = fopen(....);
    fmic = fopen(....);
    fout = fopen(....);

    ec_prms.tail_length = 64; // tail length 64 ms
    ec_prms.frame_size = 8;   // frame_size 8ms
    ec_prms.sampling_rate = 8000; //sampling rate 8000 Hz
    ec_prms.enable_bdc = 1;
    ec_prms.enable_cdc = 0;
    ec_prms.enable_drc = 1;
    echo_controller = ec_create(&ec_prms);
    bufsize = ec_prms.frame_size * ec_prms.sampling_rate / 1000; //frame size
    playbuf = malloc(sizeof(short)*bufsize);
    captbuf = malloc(sizeof(short)*bufsize);
    outbuf = malloc(sizeof(short)*bufsize);

    items1 = fread(playbuf, sizeof(short), bufsize, fspk);
    items2 = fread(captbuf, sizeof(short), bufsize, fmic);

    while(items1 == bufsize && items2 == bufsize)
    {
        //buf内包含样本点数需等于bufsize
        ec_process_frame(ec_controller, captbuf, playbuf, outbuf);
        fwrite(outbuf, sizof(short), bufsize, fout);

        items1 = fread(playbuf, sizeof(short), bufsize, fspk);
        items2 = fread(captbuf, sizeof(short), bufsize, fmic);

    }





    free(playbuf);
    free(captbuf);
    free(outbuf);


    ec_destroy(ec_controller);

    fclose(fspk);
    fclose(fmic);
    fclose(fout);

*/

#ifdef __cplusplus
}
#endif


#endif