
#ifndef __RECORD_H__
#define __RECORD_H__

#include "codec_utils.h"
#include "hal_codec.h"

int set_record_source(bool is_record_from_adc);
int start_record(int record_buf_size);
void stop_record(void);
int init_record(struct codec_client *client);
void exit_record();


extern struct phone_common_record_ops phone_common_record_ops;

#endif
