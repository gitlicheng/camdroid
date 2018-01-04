#ifndef _CAMERA_H
#define _CAMERA_H

#define CAM_CNT 2

enum{
	CAM_CSI = 0,
	CAM_UVC,
	CAM_TVD
};

#define CAM_CSI_NET CAM_CNT+CAM_CSI
#define CAM_UVC_NET CAM_CNT+CAM_UVC

typedef enum {
	PicResolution2M = 0,
	PicResolution5M,
	PicResolution8M,
	PicResolution12M,
}PicResolution_t;

//#define CSI_1080P
//#define UVC_1080P	//has no effect on recorder

//limit the time of loop recorder (unit: min)
#define LIMIT_LOOP_TIME_MIN 10

#ifdef CSI_1080P

	/* Real Definition  */
	#define REAL_CSI_W 1920
	#define REAL_CSI_H 1080

	/* Bitrate FHD */
	typedef enum {
		VideoQlt_STANDARD_csi_M = 12,
		VideoQlt_HIGH_csi_M     = 13,
		VideoQlt_SUPER_csi_M    = 14,
	}VideoQlt_t_csi;

	/* Bitrate HD */
	typedef enum {
		VideoQlt_STANDARD_csi_M_lite = 12,
		VideoQlt_HIGH_csi_M_lite     = 13,
		VideoQlt_SUPER_csi_M_lite    = 14,
	}VideoQlt_t_csi_lite;

	/* Video Definition */
	typedef enum {
		VideoSize_FHD_W_csi = 1920,
		VideoSize_FHD_H_csi = 1080,
		VideoSize_HD_W_csi  = 1280,
		VideoSize_HD_H_csi  = 720,
	}VideoSize_t_csi;

	/* Preview Size */
	#define PREVIEW_W_csi 320
	#define PREVIEW_H_csi 480

	/* FrameRate */
	#define FRONT_CAM_FRAMERATE 30
#else
	#define REAL_CSI_W 1280
	#define REAL_CSI_H 720
	typedef enum {
		VideoQlt_STANDARD_csi_M = 9,
		VideoQlt_HIGH_csi_M     = 10,
		VideoQlt_SUPER_csi_M    = 12,
	}VideoQlt_t_csi;

	typedef enum {
		VideoQlt_STANDARD_csi_M_lite = 6,
		VideoQlt_HIGH_csi_M_lite     = 7,
		VideoQlt_SUPER_csi_M_lite    = 8,
	}VideoQlt_t_csi_lite;

	typedef enum {
		VideoSize_FHD_W_csi = 1920,
		VideoSize_FHD_H_csi = 1080,
		VideoSize_HD_W_csi  = 1280,
		VideoSize_HD_H_csi  = 720,
	}VideoSize_t_csi;

	#define PREVIEW_W_csi 640
	#define PREVIEW_H_csi 360

	#define FRONT_CAM_FRAMERATE 30
#endif

#ifdef UVC_1080P
	#define REAL_UVC_W 1920
	#define REAL_UVC_H 1080

	typedef enum {
		VideoQlt_STANDARD_csi_M = 8,
		VideoQlt_HIGH_csi_M     = 8,
		VideoQlt_SUPER_csi_M    = 8,
	}VideoQlt_t_uvc;

	typedef enum {
		VideoSize_FHD_W_uvc = 1920,
		VideoSize_FHD_H_uvc = 1080,
		VideoSize_HD_W_uvc  = 1920,
		VideoSize_HD_H_uvc  = 1080,
	}VideoSize_t_uvc;

	#define PREVIEW_W_uvc 640
	#define PREVIEW_H_uvc 360

	#define BACK_CAM_FRAMERATE 25
#else
	#define REAL_UVC_W 640
	#define REAL_UVC_H 480
	typedef enum {
		VideoQlt_STANDARD_uvc_M = 2,
		VideoQlt_HIGH_uvc_M     = 2,
		VideoQlt_SUPER_uvc_M    = 2,
	}VideoQlt_t_uvc;

	typedef enum {
		VideoSize_FHD_W_uvc = 640,
		VideoSize_FHD_H_uvc = 480,
		VideoSize_HD_W_uvc  = 640,
		VideoSize_HD_H_uvc  = 480,
	}VideoSize_t_uvc;

	#define PREVIEW_W_uvc 320
	#define PREVIEW_H_uvc 240

	#define BACK_CAM_FRAMERATE 25
	/* take picture mode
	 *  define BACK_PICTURE_SOURCE_MODE: KeepSourceSize
	 *  otherwise: UseParameterPictureSize
	 */
	#define BACK_PICTURE_SOURCE_MODE
#endif

#define FRONT_CAM_BITRATE (VideoQlt_SUPER_csi_M<<20)
#define BACK_CAM_BITRATE (VideoQlt_SUPER_uvc_M<<20)

#define UVC_FLIP

#endif
