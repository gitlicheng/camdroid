
#ifndef __HAL_BP_H__
#define __HAL_BP_H__

#include "bp_utils.h"

struct bp_ops {
    int (*get_tty_dev)(char *name);
    int (*set_call_volume)(ril_audio_path_type_t path, int vol);
    int (*set_call_path)(ril_audio_path_type_t path);
    int (*set_call_at)(char *at);
};

struct other_ops {
    int (*other_op)(char *name);
};


struct bp_client {
    struct bp_ops *bp_ops;
};

#endif


