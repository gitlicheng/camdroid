
#ifndef __BP_DEVICES_H__
#define __BP_DEVICES_H__

#include "hal_bp.h"

struct bp_device_desc {
    char *name;
    char *board_name;

    struct bp_ops *ops;
    struct other_ops *other_ops;
};

extern struct bp_device_desc bp_devices[];
extern int bp_devices_count;

/* MU509 */
extern struct bp_ops mu509_ops;

/* EM55 */
extern struct bp_ops em55_ops;

/*USI6276*/
extern struct bp_ops usi6276_ops;

/* DEMO */
extern struct bp_ops demo_ops;


#endif

