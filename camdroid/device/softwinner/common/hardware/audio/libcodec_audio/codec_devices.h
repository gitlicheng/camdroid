
#ifndef __BP_DEVICES_H__
#define __BP_DEVICES_H__

#include "hal_codec.h"

struct codec_device_desc {
    char *name;
    char *plan_name;

    struct normal_ops *normal_ops;
    struct fm_ops *fm_ops;
    struct factory_ops *factory_ops;
    struct phone_ops *phone_ops;
    struct other_ops *other_ops;
    int (*device_init)(void);
    void (*device_exit)(void);
};

extern struct codec_device_desc codec_devices[];
extern int codec_devices_count;


/*pad as default*/
extern struct normal_ops pad_normal_ops;
extern struct fm_ops pad_fm_ops;
extern int pad_init(void);
extern void pad_exit(void);

/*There're four layout for a31s pad phone*/
/*hardware board,like em55, bp <--analog-->codec, FM<--analog-->codec,  BT<--pcm-->codec   PLAN_ONE*/  
extern struct normal_ops plan_one_normal_ops;
extern struct fm_ops plan_one_fm_ops;
extern struct factory_ops plan_one_factory_ops;
extern struct phone_ops plan_one_phone_ops;
extern int plan_one_init(void);
extern void plan_one_exit(void);

/*hardware board, bp <--pcm-->codec, FM<--analog-->codec,  BT<--pcm-->codec  PLAN_TWO*/
extern struct normal_ops plan_two_normal_ops;
extern struct fm_ops plan_two_fm_ops;
extern struct factory_ops plan_two_factory_ops;
extern struct phone_ops plan_two_phone_ops;
extern int plan_two_init(void);
extern void plan_two_exit(void);

/*hardware board, bp <--pcm-->codec, PLAN_V3*/
extern struct normal_ops plan_v3_normal_ops;
extern int plan_v3_init(void);
extern void plan_v3_exit(void);

/*hardware board, bp <--analog-->codec,  (BT,FM)<--pcm-->codec  PLAN_THREE*/
extern struct codec_ops bp_Analog_bt_fm_Pcm_ops;


/*hardware board, bp <--pcm-->codec, (BT,FM)<--pcm-->codec  PLAN_ALL_PCM*/
extern struct codec_ops all_Pcm_ops;

#endif

