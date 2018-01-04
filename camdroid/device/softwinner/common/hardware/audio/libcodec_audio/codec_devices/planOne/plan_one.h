

#ifndef __PLAN_ONE_H__
#define __PLAN_ONE_H__

extern int plan_one_init_voice(void);
extern void plan_one_exit_voice(void);
extern int plan_one_start_bt_voice(int up_vol);
extern int plan_one_stop_bt_voice(void);
extern int plan_one_start_bt_record(void);
extern int plan_one_stop_bt_record(void);
extern int plan_one_mixer_buf(char *buffer, int bytes);
extern int plan_one_init(void);
extern void plan_one_exit(void);

#endif


