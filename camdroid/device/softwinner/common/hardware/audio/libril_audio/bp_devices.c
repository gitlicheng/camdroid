
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "bp_devices.h"

struct bp_device_desc bp_devices[] = {
    {
        .name = "demo",
        .board_name = "demo_plan",
        .ops = &demo_ops,
        .other_ops = NULL,
    },
    {
        .name = "mu509",
        .board_name = "xxx1_plan",
        .ops = &mu509_ops,
        .other_ops = NULL,
    },
    {
        .name = "em55",
        .board_name = "xxx2_plan",
        .ops = &em55_ops,
        .other_ops = NULL,
    },
    {
    	.name = "usi6276",
		.board_name = "xxx3_plan",
		.ops = &usi6276_ops,
		.other_ops = NULL,
    }
};

int bp_devices_count = sizeof(bp_devices) / sizeof(struct bp_device_desc);


