
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "codec_devices.h"

struct codec_device_desc codec_devices[] = {
    {
    .name = "v3",
    .plan_name = "PLAN_V3",
    .normal_ops = &plan_v3_normal_ops,
    .fm_ops     = NULL,
    .factory_ops= NULL,
    .phone_ops  = NULL,
    .other_ops  = NULL,
    .device_init = plan_v3_init,
    .device_exit = plan_v3_exit,
    },
//    {
//        .name = "pad",
//        .plan_name = "PAD",
//   	.normal_ops = &pad_normal_ops,
//	.fm_ops     = &pad_fm_ops,
//	.factory_ops= NULL,
//	.phone_ops  = NULL,
//        .other_ops  = NULL,
//	.device_init = pad_init,
//	.device_exit = pad_exit,
//    },
//    {
//        .name = "bp_fm_Analog_bt_Pcm",
//        .plan_name = "PLAN_ONE",
//   	.normal_ops = &plan_one_normal_ops,
//	.fm_ops     = &plan_one_fm_ops,
//	.factory_ops= &plan_one_factory_ops,
//	.phone_ops  = &plan_one_phone_ops,
//        .other_ops  = NULL,
//	.device_init = plan_one_init,
//	.device_exit = plan_one_exit,
//    },
//    {
//        .name = "bp_pcm_fm_Analog_bt_Pcm",
//        .plan_name = "PLAN_TWO",
//   	.normal_ops = &plan_two_normal_ops,
//	.fm_ops     = &plan_two_fm_ops,
//	.factory_ops= &plan_two_factory_ops,
//	.phone_ops  = &plan_two_phone_ops,
//        .other_ops  = NULL,
//	.device_init = plan_two_init,
//	.device_exit = plan_two_exit,
//
//    },
};

int codec_devices_count = sizeof(codec_devices) / sizeof(struct codec_device_desc);


