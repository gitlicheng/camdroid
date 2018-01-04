#ifndef __DATA_STRUCT_H__
#define __DATA_STRUCT_H__

#define NAT_CMD_SET_RECORDER_QUALITY      10
#define NAT_CMD_SET_RECORDER_DURATION     20
#define NAT_CMD_SET_PHOTO_QUALITY         30
#define NAT_CMD_SET_WHITE_BALANCE         40
#define NAT_CMD_SET_EXPOSURE              50
#define NAT_CMD_SET_BOOT_RECORDER         60
#define NAT_CMD_SET_MUTE                  70
#define NAT_CMD_SET_REAR_VIEW_MIRROR      80
#define NAT_CMD_SET_TIME                  90
#define NAT_CMD_SET_LANGUAGE              100
#define NAT_CMD_SET_SMART_DETECT          110
#define NAT_CMD_SET_LANE_LINE_CALIBRATION 120
#define NAT_CMD_SET_IMPACT_SENSITIVITY    130
#define NAT_CMD_SET_MOTION_DETECT         140
#define NAT_CMD_SET_WATERMARK             150
#define NAT_CMD_FORMAT_TF_CARD            160
#define NAT_CMD_SCREEN_SLEEP              170
#define NAT_CMD_PLAY_RECORDER             180
#define NAT_CMD_GET_FILE_LIST             190
#define NAT_CMD_REQUEST_FILE              200
#define NAT_CMD_REQUEST_FILE_FINISH       201
#define NAT_CMD_CHECK_TF_CARD             210
#define NAT_CMD_DELETE_FILE               220
#define NAT_CMD_RECORD_CTL                230
#define NAT_CMD_TURN_OFF_WIFI             240
#define NAT_CMD_SET_WIFI                  250
#define NAT_CMD_SET_SYNC_FILE             260
#define NAT_CMD_GET_CONFIG                270
#define NAT_CMD_REQUEST_VIDEO             280
#define NAT_CMD_TAKE_PHOTO                290
#define NAT_CMD_RECORD_SWITCH             300

#define NAT_CMD_SET_VIDEO_SIZE            5010
#define NAT_CMD_SET_VIDEO_SIZE_ACK        5011

#define NAT_WIFI_CONNECT                  5020
#define NAT_WIFI_CONNECT_ACK              5021

#define NAT_CMD_SET_AUTO_TICK             5031

#define  NAT_CMD_LIMIT_SPEED        	 5040
#define  NAT_CMD_LIMIT_SPEED_ACK       5041

#define NAT_CMD_UVC_DISCONNECT			  5051


#define NAT_EVENT_SOS_FILE                1

#define NAT_CMD_APK_TO_CDR                1
#define NAT_CMD_CDR_TO_APK                0

typedef struct __aw_cdr_net_config {
	int record_quality;
	int record_duration;
	int photo_quality;
	int white_balance;
	int exposure;
	int boot_record;
	int mute;
	int rear_view_mirror;
	int language;
	int smart_detect;
	int lane_line_calibration;
	int impact_sensitivity;
	int motion_detect;
	int watermark;
	int record_switch;
	int dev_type;
	char firmware_version[32];
//	int tf_remain_capacity;
//	int tf_total_capacity;
} aw_cdr_net_config;

typedef struct __aw_cdr_tf_capacity {
	unsigned long long remain;
	unsigned long long total;
} aw_cdr_tf_capacity;

typedef struct __aw_cdr_file_trans {
	int from_to;
	char filename[256];
	int local_port;
	char local_ip[20];
	int remote_port;
	char remote_ip[20];
	char url[1024];
} aw_cdr_file_trans;

//typedef struct __aw_cdr_net_string {
//	int max_len;
//	char string[];
//} aw_cdr_net_string, aw_cdr_file_list, aw_cdr_rtsp_url;

typedef struct __aw_cdr_play_record_ctl {
	int ctl_name; // seek = 0;
	int time;     // unit second
} aw_cdr_play_record_ctl;

typedef struct __aw_cdr_wifi_setting {
	char old_pwd[64];
	char new_pwd[64];
} aw_cdr_wifi_setting;

typedef struct __aw_cdr_set_video_quality_index {
	int video_quality_index;
} aw_cdr_set_video_quality_index;

typedef struct __aw_cdr_wifi_state {
	int state;
} aw_cdr_wifi_state;

typedef struct __aw_cdr_limit_speed {
	int speedlimit;
} aw_cdr_limit_speed;

#endif
