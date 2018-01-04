

#ifndef __PLAN_TWO_H__
#define __PLAN_TWO_H__

extern int plan_two_init_voice(void);
extern void plan_two_exit_voice(void);
extern int plan_two_start_bt_voice(int up_vol);
extern int plan_two_stop_bt_voice(void);
extern int plan_two_start_record(void);
extern int plan_two_stop_record(void);
extern int plan_two_mixer_buf(char *buffer, int bytes);
extern int plan_two_init(void);
extern void plan_two_exit(void);

extern int plan_two_start_voice(void);
extern int plan_two_stop_voice(void);
#endif


