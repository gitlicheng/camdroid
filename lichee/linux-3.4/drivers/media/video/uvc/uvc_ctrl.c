/*
 *      uvc_ctrl.c  --  USB Video Class driver - Controls
 *
 *      Copyright (C) 2005-2009
 *          Laurent Pinchart (laurent.pinchart@skynet.be)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <asm/atomic.h>

#include "uvcvideo.h"

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 17)
#include <linux/slab.h>
#include <linux/uaccess.h>
#endif

#define UVC_CTRL_DATA_CURRENT	0
#define UVC_CTRL_DATA_BACKUP	1
#define UVC_CTRL_DATA_MIN	2
#define UVC_CTRL_DATA_MAX	3
#define UVC_CTRL_DATA_RES	4
#define UVC_CTRL_DATA_DEF	5
#define UVC_CTRL_DATA_LAST	6

static  int chip_type =  0 ;

/* ------------------------------------------------------------------------
 * Controls
 */

static struct uvc_control_info uvc_ctrls[] = {
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_BRIGHTNESS_CONTROL,
		.index		= 0,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_CONTRAST_CONTROL,
		.index		= 1,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_HUE_CONTROL,
		.index		= 2,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_SATURATION_CONTROL,
		.index		= 3,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_SHARPNESS_CONTROL,
		.index		= 4,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_GAMMA_CONTROL,
		.index		= 5,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
		.index		= 6,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
		.index		= 7,
		.size		= 4,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_BACKLIGHT_COMPENSATION_CONTROL,
		.index		= 8,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE|UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_GAIN_CONTROL,
		.index		= 9,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_POWER_LINE_FREQUENCY_CONTROL,
		.index		= 10,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_HUE_AUTO_CONTROL,
		.index		= 11,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL,
		.index		= 12,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL,
		.index		= 13,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_DIGITAL_MULTIPLIER_CONTROL,
		.index		= 14,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL,
		.index		= 15,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL,
		.index		= 16,
		.size		= 1,
		.flags		= UVC_CONTROL_GET_CUR,
	},
	{
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_ANALOG_LOCK_STATUS_CONTROL,
		.index		= 17,
		.size		= 1,
		.flags		= UVC_CONTROL_GET_CUR,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_SCANNING_MODE_CONTROL,
		.index		= 0,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_AE_MODE_CONTROL,
		.index		= 1,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_GET_RES
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_AE_PRIORITY_CONTROL,
		.index		= 2,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
		.index		= 3,
		.size		= 4,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL,
		.index		= 4,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_FOCUS_ABSOLUTE_CONTROL,
		.index		= 5,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_FOCUS_RELATIVE_CONTROL,
		.index		= 6,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN
				| UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_RES
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_IRIS_ABSOLUTE_CONTROL,
		.index		= 7,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_IRIS_RELATIVE_CONTROL,
		.index		= 8,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_ZOOM_ABSOLUTE_CONTROL,
		.index		= 9,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_ZOOM_RELATIVE_CONTROL,
		.index		= 10,
		.size		= 3,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN
				| UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_RES
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_PANTILT_ABSOLUTE_CONTROL,
		.index		= 11,
		.size		= 8,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_PANTILT_RELATIVE_CONTROL,
		.index		= 12,
		.size		= 4,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN
				| UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_RES
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_ROLL_ABSOLUTE_CONTROL,
		.index		= 13,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_RANGE
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_ROLL_RELATIVE_CONTROL,
		.index		= 14,
		.size		= 2,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN
				| UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_RES
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_FOCUS_AUTO_CONTROL,
		.index		= 17,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_GET_DEF | UVC_CONTROL_RESTORE,
	},
	{
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_PRIVACY_CONTROL,
		.index		= 18,
		.size		= 1,
		.flags		= UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
				| UVC_CONTROL_RESTORE | UVC_CONTROL_AUTO_UPDATE,
	},
};

static struct uvc_menu_info power_line_frequency_controls[] = {
	{ 0, "Disabled" },
	{ 1, "50 Hz" },
	{ 2, "60 Hz" },
};

static struct uvc_menu_info exposure_auto_controls[] = {
	{ 2, "Auto Mode" },
	{ 1, "Manual Mode" },
	{ 4, "Shutter Priority Mode" },
	{ 8, "Aperture Priority Mode" },
};

static __s32 uvc_ctrl_get_zoom(struct uvc_control_mapping *mapping,
	__u8 query, const __u8 *data)
{
	__s8 zoom = (__s8)data[0];

	switch (query) {
	case UVC_GET_CUR:
		return (zoom == 0) ? 0 : (zoom > 0 ? data[2] : -data[2]);

	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_RES:
	case UVC_GET_DEF:
	default:
		return data[2];
	}
}

static void uvc_ctrl_set_zoom(struct uvc_control_mapping *mapping,
	__s32 value, __u8 *data)
{
	data[0] = value == 0 ? 0 : (value > 0) ? 1 : 0xff;
	data[2] = min((int)abs(value), 0xff);
}

static struct uvc_control_mapping uvc_ctrl_mappings[] = {
	{
		.id		= V4L2_CID_BRIGHTNESS,
		.name		= "Brightness",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_BRIGHTNESS_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id		= V4L2_CID_CONTRAST,
		.name		= "Contrast",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_CONTRAST_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_HUE,
		.name		= "Hue",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_HUE_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id		= V4L2_CID_SATURATION,
		.name		= "Saturation",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_SATURATION_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_SHARPNESS,
		.name		= "Sharpness",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_SHARPNESS_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_GAMMA,
		.name		= "Gamma",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_GAMMA_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_BACKLIGHT_COMPENSATION,
		.name		= "Backlight Compensation",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_BACKLIGHT_COMPENSATION_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_GAIN,
		.name		= "Gain",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_GAIN_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_POWER_LINE_FREQUENCY,
		.name		= "Power Line Frequency",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_POWER_LINE_FREQUENCY_CONTROL,
		.size		= 2,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_MENU,
		.data_type	= UVC_CTRL_DATA_TYPE_ENUM,
		.menu_info	= power_line_frequency_controls,
		.menu_count	= ARRAY_SIZE(power_line_frequency_controls),
	},
	{
		.id		= V4L2_CID_HUE_AUTO,
		.name		= "Hue, Auto",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_HUE_AUTO_CONTROL,
		.size		= 1,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_BOOLEAN,
		.data_type	= UVC_CTRL_DATA_TYPE_BOOLEAN,
	},
	{
		.id		= V4L2_CID_EXPOSURE_AUTO,
		.name		= "Exposure, Auto",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_AE_MODE_CONTROL,
		.size		= 4,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_MENU,
		.data_type	= UVC_CTRL_DATA_TYPE_BITMASK,
		.menu_info	= exposure_auto_controls,
		.menu_count	= ARRAY_SIZE(exposure_auto_controls),
	},
	{
		.id		= V4L2_CID_EXPOSURE_AUTO_PRIORITY,
		.name		= "Exposure, Auto Priority",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_AE_PRIORITY_CONTROL,
		.size		= 1,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_BOOLEAN,
		.data_type	= UVC_CTRL_DATA_TYPE_BOOLEAN,
	},
	{
		.id		= V4L2_CID_EXPOSURE_ABSOLUTE,
		.name		= "Exposure (Absolute)",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
		.size		= 32,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_AUTO_WHITE_BALANCE,
		.name		= "White Balance Temperature, Auto",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL,
		.size		= 1,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_BOOLEAN,
		.data_type	= UVC_CTRL_DATA_TYPE_BOOLEAN,
	},
	{
		.id		= V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.name		= "White Balance Temperature",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_AUTO_WHITE_BALANCE,
		.name		= "White Balance Component, Auto",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL,
		.size		= 1,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_BOOLEAN,
		.data_type	= UVC_CTRL_DATA_TYPE_BOOLEAN,
	},
	{
		.id		= V4L2_CID_BLUE_BALANCE,
		.name		= "White Balance Blue Component",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id		= V4L2_CID_RED_BALANCE,
		.name		= "White Balance Red Component",
		.entity		= UVC_GUID_UVC_PROCESSING,
		.selector	= UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL,
		.size		= 16,
		.offset		= 16,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_SIGNED,
	},
	{
		.id		= V4L2_CID_FOCUS_ABSOLUTE,
		.name		= "Focus (absolute)",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_FOCUS_ABSOLUTE_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_FOCUS_AUTO,
		.name		= "Focus, Auto",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_FOCUS_AUTO_CONTROL,
		.size		= 1,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_BOOLEAN,
		.data_type	= UVC_CTRL_DATA_TYPE_BOOLEAN,
	},
/*
	{
		.id		= V4L2_CID_IRIS_ABSOLUTE,
		.name		= "Iris, Absolute",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_IRIS_ABSOLUTE_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_IRIS_RELATIVE,
		.name		= "Iris, Relative",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_IRIS_RELATIVE_CONTROL,
		.size		= 8,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_SIGNED,
	},
*/
	{
		.id		= V4L2_CID_ZOOM_ABSOLUTE,
		.name		= "Zoom, Absolute",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_ZOOM_ABSOLUTE_CONTROL,
		.size		= 16,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		.id		= V4L2_CID_ZOOM_CONTINUOUS,
		.name		= "Zoom, Continuous",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_ZOOM_RELATIVE_CONTROL,
		.size		= 0,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_INTEGER,
		.data_type	= UVC_CTRL_DATA_TYPE_SIGNED,
		.get		= uvc_ctrl_get_zoom,
		.set		= uvc_ctrl_set_zoom,
	},
	{
		.id		= V4L2_CID_PRIVACY,
		.name		= "Privacy",
		.entity		= UVC_GUID_UVC_CAMERA,
		.selector	= UVC_CT_PRIVACY_CONTROL,
		.size		= 1,
		.offset		= 0,
		.v4l2_type	= V4L2_CTRL_TYPE_BOOLEAN,
		.data_type	= UVC_CTRL_DATA_TYPE_BOOLEAN,
	},
};


// Houston 2011/08/08 XU ctrls ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define SONIX_SN9C291_SERIES_CHIPID 0x90
#define SONIX_SN9C292_SERIES_CHIPID 0x92

// SONiX Extension Unit GUID
#define UVC_GUID_SONIX_SYS_HW_CTRL				{0x70, 0x33, 0xf0, 0x28, 0x11, 0x63, 0x2e, 0x4a, 0xba, 0x2c, 0x68, 0x90, 0xeb, 0x33, 0x40, 0x16}	//Extension Unit 3
#define UVC_GUID_SONIX_USR_HW_CTRL				{0x94, 0x73, 0xDF, 0xDD, 0x3E, 0x97, 0x27, 0x47, 0xBE, 0xD9, 0x04, 0xED, 0x64, 0x26, 0xDC, 0x67}	//Extension Unit 4
//#define UVC_GUID_SONIX_USR_HW_CTRL				{0x3F, 0xAE, 0x12, 0x28, 0xD7, 0xBC, 0x11, 0x4E, 0xA3, 0x57, 0x6F, 0x1E, 0xDE, 0xF7, 0xD6, 0x1D}	//Extension Unit 4

#define V4L2_CID_BASE_EXTCTR_SONIX				0x0A0c4501
#define V4L2_CID_BASE_SONIX						V4L2_CID_BASE_EXTCTR_SONIX
#define V4L2_CID_ASIC_RW_SONIX					V4L2_CID_BASE_SONIX+1
#define V4L2_CID_FRAME_INFO_SONIX				V4L2_CID_BASE_SONIX+2
#define V4L2_CID_H264_CTRL_SONIX				V4L2_CID_BASE_SONIX+3
#define V4L2_CID_MJPG_CTRL_SONIX				V4L2_CID_BASE_SONIX+4
#define V4L2_CID_OSD_CTRL_SONIX					V4L2_CID_BASE_SONIX+5
#define V4L2_CID_MOTION_DETECTION_SONIX			V4L2_CID_BASE_SONIX+6
#define V4L2_CID_IMG_SETTING_SONIX				V4L2_CID_BASE_SONIX+7
#define V4L2_CID_MULTI_STREAM_CTRL_SONIX		V4L2_CID_BASE_SONIX+8
#define V4L2_CID_LAST_EXTCTR_SONIX				V4L2_CID_MULTI_STREAM_CTRL_SONIX

// ----------------------------- XU Control Selector ------------------------------------
#define XU_SONIX_SYS_ASIC_READ      			0x01
#define XU_SONIX_SYS_FRAME_INFO					0x06
#define XU_SONIX_SYS_H264_CTRL					0x07
#define XU_SONIX_SYS_MJPG_CTRL					0x08
#define XU_SONIX_SYS_OSD_CTRL					0x09
#define XU_SONIX_SYS_MOTION_DETECTION			0x0A
#define XU_SONIX_SYS_IMG_SETTING				0x0B

#define XU_SONIX_USR_FRAME_INFO					0x01
#define XU_SONIX_USR_H264_CTRL					0x02
#define XU_SONIX_USR_MJPG_CTRL					0x03
#define XU_SONIX_USR_OSD_CTRL					0x04
#define XU_SONIX_USR_MOTION_DETECTION			0x05
#define XU_SONIX_USR_IMG_SETTING				0x06
#define XU_SONIX_USR_MULTI_STREAM_CTRL			0x07

// ----------------------------- XU Control Selector ------------------------------------
#define LENGTH_OF_SONIX_XU_SYS_CTR				(7)
#define LENGTH_OF_SONIX_XU_USR_CTR				(7)

// chkuo add, define extension unit length
#define LENGTH_OF_SONIX_XU						(11)

static struct uvc_xu_control_info sonix_xu_sys_ctrls[] =
{
    {
      .entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector = XU_SONIX_SYS_ASIC_READ,
      .index    = 0,
      .size     = 4,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector = XU_SONIX_SYS_FRAME_INFO,
      .index    = 1,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector = XU_SONIX_SYS_H264_CTRL,
      .index    = 2,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector = XU_SONIX_SYS_MJPG_CTRL,
      .index    = 3,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector = XU_SONIX_SYS_OSD_CTRL,
      .index    = 4,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector = XU_SONIX_SYS_MOTION_DETECTION,
      .index    = 5,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector = XU_SONIX_SYS_IMG_SETTING,
      .index    = 6,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
};

static struct uvc_xu_control_info sonix_xu_usr_ctrls[] =
{
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_FRAME_INFO,
      .index    = 0,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_H264_CTRL,
      .index    = 1,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MJPG_CTRL,
      .index    = 2,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_OSD_CTRL,
      .index    = 3,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
                  UVC_CONTROL_GET_DEF | UVC_CONTROL_AUTO_UPDATE | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MOTION_DETECTION,
      .index    = 4,
      .size     = 24,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_IMG_SETTING,
      .index    = 5,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },
    {
      .entity   = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector = XU_SONIX_USR_MULTI_STREAM_CTRL,
      .index    = 6,
      .size     = LENGTH_OF_SONIX_XU,
      .flags    = UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR
    },	
};

// mapping for SONiX XU system ctrls
static struct uvc_xu_control_mapping sonix_xu_sys_mappings[] =
{
    {
      .id        = V4L2_CID_ASIC_RW_SONIX,
      .name      = "SONiX: Asic Read",
      .entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector  = XU_SONIX_SYS_ASIC_READ,
      .size      = 4,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_SIGNED
    },
    {
      .id        = V4L2_CID_FRAME_INFO_SONIX,
      .name      = "SONiX: H264 Format",
      .entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector  = XU_SONIX_SYS_FRAME_INFO,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_H264_CTRL_SONIX,
      .name      = "SONiX: H264 Control",
      .entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector  = XU_SONIX_SYS_H264_CTRL,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_MJPG_CTRL_SONIX,
      .name      = "SONiX: MJPG Control",
      .entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector  = XU_SONIX_SYS_MJPG_CTRL,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_OSD_CTRL_SONIX,
      .name      = "SONiX: OSD Control",
      .entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector  = XU_SONIX_SYS_OSD_CTRL,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_MOTION_DETECTION_SONIX,
      .name      = "SONiX: Motion Detection",
      .entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector  = XU_SONIX_SYS_MOTION_DETECTION,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_IMG_SETTING_SONIX,
      .name      = "SONiX: Image Setting",
      .entity    = UVC_GUID_SONIX_SYS_HW_CTRL,
      .selector  = XU_SONIX_SYS_IMG_SETTING,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
};

// mapping for SONiX XU user ctrls
static struct uvc_xu_control_mapping sonix_xu_usr_mappings[] =
{
    {
      .id        = V4L2_CID_FRAME_INFO_SONIX,
      .name      = "SONiX: H264 Format",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_FRAME_INFO,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_H264_CTRL_SONIX,
      .name      = "SONiX: H264 Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_H264_CTRL,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_MJPG_CTRL_SONIX,
      .name      = "SONiX: MJPG Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MJPG_CTRL,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_OSD_CTRL_SONIX,
      .name      = "SONiX: OSD Control",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_OSD_CTRL,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_RAW
    },
    {
      .id        = V4L2_CID_MOTION_DETECTION_SONIX,
      .name      = "SONiX: Motion Detection",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MOTION_DETECTION,
      .size      = 24,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_IMG_SETTING_SONIX,
      .name      = "SONiX: Image Setting",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_IMG_SETTING,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },
    {
      .id        = V4L2_CID_MULTI_STREAM_CTRL_SONIX,
      .name      = "SONiX: Multi Stram Control ",
      .entity    = UVC_GUID_SONIX_USR_HW_CTRL,
      .selector  = XU_SONIX_USR_MULTI_STREAM_CTRL,
      .size      = LENGTH_OF_SONIX_XU,
      .offset    = 0,
      .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
      .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED
    },	
};
// Houston 2011/08/08 XU ctrls ----------------------------------------------------------


/* ------------------------------------------------------------------------
 * Utility functions
 */

static inline __u8 *uvc_ctrl_data(struct uvc_control *ctrl, int id)
{
	return ctrl->data + id * ctrl->info->size;
}

static inline int uvc_test_bit(const __u8 *data, int bit)
{
	return (data[bit >> 3] >> (bit & 7)) & 1;
}

static inline void uvc_clear_bit(__u8 *data, int bit)
{
	data[bit >> 3] &= ~(1 << (bit & 7));
}

/* Extract the bit string specified by mapping->offset and mapping->size
 * from the little-endian data stored at 'data' and return the result as
 * a signed 32bit integer. Sign extension will be performed if the mapping
 * references a signed data type.
 */
static __s32 uvc_get_le_value(struct uvc_control_mapping *mapping,
	__u8 query, const __u8 *data)
{
	int bits = mapping->size;
	int offset = mapping->offset;
	__s32 value = 0;
	__u8 mask;

	data += offset / 8;
	offset &= 7;
	mask = ((1LL << bits) - 1) << offset;

	for (; bits > 0; data++) {
		__u8 byte = *data & mask;
		value |= offset > 0 ? (byte >> offset) : (byte << (-offset));
		bits -= 8 - (offset > 0 ? offset : 0);
		offset -= 8;
		mask = (1 << bits) - 1;
	}

	/* Sign-extend the value if needed. */
	if (mapping->data_type == UVC_CTRL_DATA_TYPE_SIGNED)
		value |= -(value & (1 << (mapping->size - 1)));

	return value;
}

/* Set the bit string specified by mapping->offset and mapping->size
 * in the little-endian data stored at 'data' to the value 'value'.
 */
static void uvc_set_le_value(struct uvc_control_mapping *mapping,
	__s32 value, __u8 *data)
{
	int bits = mapping->size;
	int offset = mapping->offset;
	__u8 mask;

	data += offset / 8;
	offset &= 7;

	for (; bits > 0; data++) {
		mask = ((1LL << bits) - 1) << offset;
		*data = (*data & ~mask) | ((value << offset) & mask);
		value >>= offset ? offset : 8;
		bits -= 8 - offset;
		offset = 0;
	}
}

/* ------------------------------------------------------------------------
 * Terminal and unit management
 */

static const __u8 uvc_processing_guid[16] = UVC_GUID_UVC_PROCESSING;
static const __u8 uvc_camera_guid[16] = UVC_GUID_UVC_CAMERA;
static const __u8 uvc_media_transport_input_guid[16] =
	UVC_GUID_UVC_MEDIA_TRANSPORT_INPUT;

static int uvc_entity_match_guid(struct uvc_entity *entity, __u8 guid[16])
{
	switch (UVC_ENTITY_TYPE(entity)) {
	case UVC_ITT_CAMERA:
		return memcmp(uvc_camera_guid, guid, 16) == 0;

	case UVC_ITT_MEDIA_TRANSPORT_INPUT:
		return memcmp(uvc_media_transport_input_guid, guid, 16) == 0;

	case UVC_VC_PROCESSING_UNIT:
		return memcmp(uvc_processing_guid, guid, 16) == 0;

	case UVC_VC_EXTENSION_UNIT:
		return memcmp(entity->extension.guidExtensionCode,
			      guid, 16) == 0;

	default:
		return 0;
	}
}

/* ------------------------------------------------------------------------
 * UVC Controls
 */

static void __uvc_find_control(struct uvc_entity *entity, __u32 v4l2_id,
	struct uvc_control_mapping **mapping, struct uvc_control **control,
	int next)
{
	struct uvc_control *ctrl;
	struct uvc_control_mapping *map;
	unsigned int i;

	if (entity == NULL)
		return;

	for (i = 0; i < entity->ncontrols; ++i) {
		ctrl = &entity->controls[i];
		if (ctrl->info == NULL)
			continue;

		list_for_each_entry(map, &ctrl->info->mappings, list) {
			if ((map->id == v4l2_id) && !next) {
				*control = ctrl;
				*mapping = map;
				return;
			}

			if ((*mapping == NULL || (*mapping)->id > map->id) &&
			    (map->id > v4l2_id) && next) {
				*control = ctrl;
				*mapping = map;
			}
		}
	}
}

struct uvc_control *uvc_find_control(struct uvc_video_chain *chain,
	__u32 v4l2_id, struct uvc_control_mapping **mapping)
{
	struct uvc_control *ctrl = NULL;
	struct uvc_entity *entity;
	int next = v4l2_id & V4L2_CTRL_FLAG_NEXT_CTRL;

	*mapping = NULL;

	/* Mask the query flags. */
	v4l2_id &= V4L2_CTRL_ID_MASK;

	/* Find the control. */
	list_for_each_entry(entity, &chain->entities, chain) {
		__uvc_find_control(entity, v4l2_id, mapping, &ctrl, next);
		if (ctrl && !next)
			return ctrl;
	}

	if (ctrl == NULL && !next)
		uvc_trace(UVC_TRACE_CONTROL, "Control 0x%08x not found.\n",
				v4l2_id);

	return ctrl;
}

static int uvc_ctrl_populate_cache(struct uvc_video_chain *chain,
	struct uvc_control *ctrl)
{
	int ret;

	if (ctrl->info->flags & UVC_CONTROL_GET_DEF) {
		ret = uvc_query_ctrl(chain->dev, UVC_GET_DEF, ctrl->entity->id,
				     chain->dev->intfnum, ctrl->info->selector,
				     uvc_ctrl_data(ctrl, UVC_CTRL_DATA_DEF),
				     ctrl->info->size);
		if (ret < 0)
			return ret;
	}

	if (ctrl->info->flags & UVC_CONTROL_GET_MIN) {
		ret = uvc_query_ctrl(chain->dev, UVC_GET_MIN, ctrl->entity->id,
				     chain->dev->intfnum, ctrl->info->selector,
				     uvc_ctrl_data(ctrl, UVC_CTRL_DATA_MIN),
				     ctrl->info->size);
		if (ret < 0)
			return ret;
	}
	if (ctrl->info->flags & UVC_CONTROL_GET_MAX) {
		ret = uvc_query_ctrl(chain->dev, UVC_GET_MAX, ctrl->entity->id,
				     chain->dev->intfnum, ctrl->info->selector,
				     uvc_ctrl_data(ctrl, UVC_CTRL_DATA_MAX),
				     ctrl->info->size);
		if (ret < 0)
			return ret;
	}
	if (ctrl->info->flags & UVC_CONTROL_GET_RES) {
		ret = uvc_query_ctrl(chain->dev, UVC_GET_RES, ctrl->entity->id,
				     chain->dev->intfnum, ctrl->info->selector,
				     uvc_ctrl_data(ctrl, UVC_CTRL_DATA_RES),
				     ctrl->info->size);
		if (ret < 0)
			return ret;
	}

	ctrl->cached = 1;
	return 0;
}

int uvc_query_v4l2_ctrl(struct uvc_video_chain *chain,
	struct v4l2_queryctrl *v4l2_ctrl)
{
	struct uvc_control *ctrl;
	struct uvc_control_mapping *mapping;
	struct uvc_menu_info *menu;
	unsigned int i;
	int ret;

	ctrl = uvc_find_control(chain, v4l2_ctrl->id, &mapping);
	if (ctrl == NULL)
		return -EINVAL;

	memset(v4l2_ctrl, 0, sizeof *v4l2_ctrl);
	v4l2_ctrl->id = mapping->id;
	v4l2_ctrl->type = mapping->v4l2_type;
	strlcpy(v4l2_ctrl->name, mapping->name, sizeof v4l2_ctrl->name);
	v4l2_ctrl->flags = 0;

	if (!(ctrl->info->flags & UVC_CONTROL_GET_CUR))
		v4l2_ctrl->flags |= V4L2_CTRL_FLAG_WRITE_ONLY;
	if (!(ctrl->info->flags & UVC_CONTROL_SET_CUR))
		v4l2_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	if (!ctrl->cached) {
		ret = uvc_ctrl_populate_cache(chain, ctrl);
		if (ret < 0)
			return ret;
	}

	if (ctrl->info->flags & UVC_CONTROL_GET_DEF) {
		v4l2_ctrl->default_value = mapping->get(mapping, UVC_GET_DEF,
				uvc_ctrl_data(ctrl, UVC_CTRL_DATA_DEF));
	}

	switch (mapping->v4l2_type) {
	case V4L2_CTRL_TYPE_MENU:
		v4l2_ctrl->minimum = 0;
		v4l2_ctrl->maximum = mapping->menu_count - 1;
		v4l2_ctrl->step = 1;

		menu = mapping->menu_info;
		for (i = 0; i < mapping->menu_count; ++i, ++menu) {
			if (menu->value == v4l2_ctrl->default_value) {
				v4l2_ctrl->default_value = i;
				break;
			}
		}

		return 0;

	case V4L2_CTRL_TYPE_BOOLEAN:
		v4l2_ctrl->minimum = 0;
		v4l2_ctrl->maximum = 1;
		v4l2_ctrl->step = 1;
		return 0;

	case V4L2_CTRL_TYPE_BUTTON:
		v4l2_ctrl->minimum = 0;
		v4l2_ctrl->maximum = 0;
		v4l2_ctrl->step = 0;
		return 0;

	default:
		break;
	}

	if (ctrl->info->flags & UVC_CONTROL_GET_MIN)
		v4l2_ctrl->minimum = mapping->get(mapping, UVC_GET_MIN,
				     uvc_ctrl_data(ctrl, UVC_CTRL_DATA_MIN));

	if (ctrl->info->flags & UVC_CONTROL_GET_MAX)
		v4l2_ctrl->maximum = mapping->get(mapping, UVC_GET_MAX,
				     uvc_ctrl_data(ctrl, UVC_CTRL_DATA_MAX));

	if (ctrl->info->flags & UVC_CONTROL_GET_RES)
		v4l2_ctrl->step = mapping->get(mapping, UVC_GET_RES,
				  uvc_ctrl_data(ctrl, UVC_CTRL_DATA_RES));

	return 0;
}


/* --------------------------------------------------------------------------
 * Control transactions
 *
 * To make extended set operations as atomic as the hardware allows, controls
 * are handled using begin/commit/rollback operations.
 *
 * At the beginning of a set request, uvc_ctrl_begin should be called to
 * initialize the request. This function acquires the control lock.
 *
 * When setting a control, the new value is stored in the control data field
 * at position UVC_CTRL_DATA_CURRENT. The control is then marked as dirty for
 * later processing. If the UVC and V4L2 control sizes differ, the current
 * value is loaded from the hardware before storing the new value in the data
 * field.
 *
 * After processing all controls in the transaction, uvc_ctrl_commit or
 * uvc_ctrl_rollback must be called to apply the pending changes to the
 * hardware or revert them. When applying changes, all controls marked as
 * dirty will be modified in the UVC device, and the dirty flag will be
 * cleared. When reverting controls, the control data field
 * UVC_CTRL_DATA_CURRENT is reverted to its previous value
 * (UVC_CTRL_DATA_BACKUP) for all dirty controls. Both functions release the
 * control lock.
 */
int uvc_ctrl_begin(struct uvc_video_chain *chain)
{
	return mutex_lock_interruptible(&chain->ctrl_mutex) ? -ERESTARTSYS : 0;
}

static int uvc_ctrl_commit_entity(struct uvc_device *dev,
	struct uvc_entity *entity, int rollback)
{
	struct uvc_control *ctrl;
	unsigned int i;
	int ret;

	if (entity == NULL)
		return 0;

	for (i = 0; i < entity->ncontrols; ++i) {
		ctrl = &entity->controls[i];
		if (ctrl->info == NULL)
			continue;

		/* Reset the loaded flag for auto-update controls that were
		 * marked as loaded in uvc_ctrl_get/uvc_ctrl_set to prevent
		 * uvc_ctrl_get from using the cached value.
		 */
		if (ctrl->info->flags & UVC_CONTROL_AUTO_UPDATE)
			ctrl->loaded = 0;

		if (!ctrl->dirty)
			continue;

		if (!rollback)
			ret = uvc_query_ctrl(dev, UVC_SET_CUR, ctrl->entity->id,
				dev->intfnum, ctrl->info->selector,
				uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
				ctrl->info->size);
		else
			ret = 0;

		if (rollback || ret < 0)
			memcpy(uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
			       uvc_ctrl_data(ctrl, UVC_CTRL_DATA_BACKUP),
			       ctrl->info->size);

		ctrl->dirty = 0;

		if (ret < 0)
			return ret;
	}

	return 0;
}

int __uvc_ctrl_commit(struct uvc_video_chain *chain, int rollback)
{
	struct uvc_entity *entity;
	int ret = 0;

	/* Find the control. */
	list_for_each_entry(entity, &chain->entities, chain) {
		ret = uvc_ctrl_commit_entity(chain->dev, entity, rollback);
		if (ret < 0)
			goto done;
	}

done:
	mutex_unlock(&chain->ctrl_mutex);
	return ret;
}

int uvc_ctrl_get(struct uvc_video_chain *chain,
	struct v4l2_ext_control *xctrl)
{
	struct uvc_control *ctrl;
	struct uvc_control_mapping *mapping;
	struct uvc_menu_info *menu;
	unsigned int i;
	int ret;

	ctrl = uvc_find_control(chain, xctrl->id, &mapping);
	if (ctrl == NULL || (ctrl->info->flags & UVC_CONTROL_GET_CUR) == 0)
		return -EINVAL;

	if (!ctrl->loaded) {
		ret = uvc_query_ctrl(chain->dev, UVC_GET_CUR, ctrl->entity->id,
				chain->dev->intfnum, ctrl->info->selector,
				uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
				ctrl->info->size);
		if (ret < 0)
			return ret;

		ctrl->loaded = 1;
	}

	xctrl->value = mapping->get(mapping, UVC_GET_CUR,
		uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT));

	if (mapping->v4l2_type == V4L2_CTRL_TYPE_MENU) {
		menu = mapping->menu_info;
		for (i = 0; i < mapping->menu_count; ++i, ++menu) {
			if (menu->value == xctrl->value) {
				xctrl->value = i;
				break;
			}
		}
	}

	return 0;
}

int uvc_ctrl_set(struct uvc_video_chain *chain,
	struct v4l2_ext_control *xctrl)
{
	struct uvc_control *ctrl;
	struct uvc_control_mapping *mapping;
	s32 value;
	u32 step;
	s32 min;
	s32 max;
	int ret;

	ctrl = uvc_find_control(chain, xctrl->id, &mapping);
	if (ctrl == NULL || (ctrl->info->flags & UVC_CONTROL_SET_CUR) == 0)
		return -EINVAL;

	/* Clamp out of range values. */
	switch (mapping->v4l2_type) {
	case V4L2_CTRL_TYPE_INTEGER:
		if (!ctrl->cached) {
			ret = uvc_ctrl_populate_cache(chain, ctrl);
			if (ret < 0)
				return ret;
		}

		min = mapping->get(mapping, UVC_GET_MIN,
				   uvc_ctrl_data(ctrl, UVC_CTRL_DATA_MIN));
		max = mapping->get(mapping, UVC_GET_MAX,
				   uvc_ctrl_data(ctrl, UVC_CTRL_DATA_MAX));
		step = mapping->get(mapping, UVC_GET_RES,
				    uvc_ctrl_data(ctrl, UVC_CTRL_DATA_RES));
		if (step == 0)
			step = 1;

		xctrl->value = min + (xctrl->value - min + step/2) / step * step;
		xctrl->value = clamp(xctrl->value, min, max);
		value = xctrl->value;
		break;

	case V4L2_CTRL_TYPE_BOOLEAN:
		xctrl->value = clamp(xctrl->value, 0, 1);
		value = xctrl->value;
		break;

	case V4L2_CTRL_TYPE_MENU:
		if (xctrl->value < 0 || xctrl->value >= mapping->menu_count)
			return -ERANGE;
		value = mapping->menu_info[xctrl->value].value;
		break;

	default:
		value = xctrl->value;
		break;
	}

	/* If the mapping doesn't span the whole UVC control, the current value
	 * needs to be loaded from the device to perform the read-modify-write
	 * operation.
	 */
	if (!ctrl->loaded && (ctrl->info->size * 8) != mapping->size) {
		if ((ctrl->info->flags & UVC_CONTROL_GET_CUR) == 0) {
			memset(uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
				0, ctrl->info->size);
		} else {
			ret = uvc_query_ctrl(chain->dev, UVC_GET_CUR,
				ctrl->entity->id, chain->dev->intfnum,
				ctrl->info->selector,
				uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
				ctrl->info->size);
			if (ret < 0)
				return ret;
		}

		ctrl->loaded = 1;
	}

	/* Backup the current value in case we need to rollback later. */
	if (!ctrl->dirty) {
		memcpy(uvc_ctrl_data(ctrl, UVC_CTRL_DATA_BACKUP),
		       uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
		       ctrl->info->size);
	}

	mapping->set(mapping, value,
		uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT));

	ctrl->dirty = 1;
	ctrl->modified = 1;
	return 0;
}

/* --------------------------------------------------------------------------
 * Dynamic controls
 */

int uvc_xu_ctrl_query(struct uvc_video_chain *chain,
	struct uvc_xu_control *xctrl, int set)
{
	struct uvc_entity *entity;
	struct uvc_control *ctrl = NULL;
	unsigned int i, found = 0;
	__u8 *data;
	int ret;

	/* Find the extension unit. */
	list_for_each_entry(entity, &chain->entities, chain) {
		if (UVC_ENTITY_TYPE(entity) == UVC_VC_EXTENSION_UNIT &&
		    entity->id == xctrl->unit)
			break;
	}

	if (entity->id != xctrl->unit) {
		uvc_trace(UVC_TRACE_CONTROL, "Extension unit %u not found.\n",
			xctrl->unit);
		return -EINVAL;
	}

	/* Find the control. */
	for (i = 0; i < entity->ncontrols; ++i) {
		ctrl = &entity->controls[i];
		if (ctrl->info == NULL)
			continue;

		if (ctrl->info->selector == xctrl->selector) {
			found = 1;
			break;
		}
	}

	if (!found) {
		uvc_trace(UVC_TRACE_CONTROL, "Control %pUl/%u not found.\n",
			entity->extension.guidExtensionCode, xctrl->selector);
		return -EINVAL;
	}

	/* Validate control data size. */
	if (ctrl->info->size != xctrl->size)
		return -EINVAL;

	if ((set && !(ctrl->info->flags & UVC_CONTROL_SET_CUR)) ||
	    (!set && !(ctrl->info->flags & UVC_CONTROL_GET_CUR)))
		return -EINVAL;

	if (mutex_lock_interruptible(&chain->ctrl_mutex))
		return -ERESTARTSYS;

	memcpy(uvc_ctrl_data(ctrl, UVC_CTRL_DATA_BACKUP),
	       uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
	       xctrl->size);
	data = uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT);

	if (set && copy_from_user(data, xctrl->data, xctrl->size)) {
		ret = -EFAULT;
		goto out;
	}

#if 0	//for debug printf xu control - set  (add by cjc)
	if(set)
	{
		int ii=0;
		printk("\n set [");
		for(ii=0;ii<11;ii++)
		{
			printk("0x%02x ", xctrl->data[ii]);
		}
		printk("]\n ");
	}
#endif
	
	ret = uvc_query_ctrl(chain->dev, set ? UVC_SET_CUR : UVC_GET_CUR,
			     xctrl->unit, chain->dev->intfnum, xctrl->selector,
			     data, xctrl->size);
	if (ret < 0)
		goto out;

	if (!set && copy_to_user(xctrl->data, data, xctrl->size)) {
		ret = -EFAULT;
		goto out;
	}

#if 0	//for debug xu control - get (add by cjc)
	if(!set)
	{
		int ii=0;
		printk("\n get[");
		for(ii=0;ii<11;ii++)
		{
			printk("0x%02x ", xctrl->data[ii]);
		}
		printk("]\n ");
	}
#endif
	
out:
	if (ret)
		memcpy(uvc_ctrl_data(ctrl, UVC_CTRL_DATA_CURRENT),
		       uvc_ctrl_data(ctrl, UVC_CTRL_DATA_BACKUP),
		       xctrl->size);

	mutex_unlock(&chain->ctrl_mutex);
	return ret;
}

/* --------------------------------------------------------------------------
 * Suspend/resume
 */

/*
 * Restore control values after resume, skipping controls that haven't been
 * changed.
 *
 * TODO
 * - Don't restore modified controls that are back to their default value.
 * - Handle restore order (Auto-Exposure Mode should be restored before
 *   Exposure Time).
 */
int uvc_ctrl_resume_device(struct uvc_device *dev)
{
	struct uvc_control *ctrl;
	struct uvc_entity *entity;
	unsigned int i;
	int ret;

	/* Walk the entities list and restore controls when possible. */
	list_for_each_entry(entity, &dev->entities, list) {

		for (i = 0; i < entity->ncontrols; ++i) {
			ctrl = &entity->controls[i];

			if (ctrl->info == NULL || !ctrl->modified ||
			    (ctrl->info->flags & UVC_CONTROL_RESTORE) == 0)
				continue;

			printk(KERN_INFO "restoring control %pUl/%u/%u\n",
				ctrl->info->entity, ctrl->info->index,
				ctrl->info->selector);
			ctrl->dirty = 1;
		}

		ret = uvc_ctrl_commit_entity(dev, entity, 0);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* --------------------------------------------------------------------------
 * Control and mapping handling
 */

static void uvc_ctrl_add_ctrl(struct uvc_device *dev,
	struct uvc_control_info *info)
{
	struct uvc_entity *entity;
	struct uvc_control *ctrl = NULL;
	int ret, found = 0;
	unsigned int i;

	list_for_each_entry(entity, &dev->entities, list) {
		if (!uvc_entity_match_guid(entity, info->entity))
			continue;

		for (i = 0; i < entity->ncontrols; ++i) {
			ctrl = &entity->controls[i];
			if (ctrl->index == info->index) {
				found = 1;
				break;
			}
		}

		if (found)
			break;
	}

	if (!found)
		return;

	if (UVC_ENTITY_TYPE(entity) == UVC_VC_EXTENSION_UNIT) {
		/* Check if the device control information and length match
		 * the user supplied information.
		 */
		__u32 flags;
		__le16 size;
		__u8 inf;

		ret = uvc_query_ctrl(dev, UVC_GET_LEN, ctrl->entity->id,
			dev->intfnum, info->selector, (__u8 *)&size, 2);
		if (ret < 0) {
			uvc_trace(UVC_TRACE_CONTROL,
				"GET_LEN failed on control %pUl/%u (%d).\n",
				info->entity, info->selector, ret);
			return;
		}

		if (info->size != le16_to_cpu(size)) {
			uvc_trace(UVC_TRACE_CONTROL, "Control %pUl/%u size "
				"doesn't match user supplied value.\n",
				info->entity, info->selector);
			return;
		}

		ret = uvc_query_ctrl(dev, UVC_GET_INFO, ctrl->entity->id,
			dev->intfnum, info->selector, &inf, 1);
		if (ret < 0) {
			uvc_trace(UVC_TRACE_CONTROL,
				"GET_INFO failed on control %pUl/%u (%d).\n",
				info->entity, info->selector, ret);
			return;
		}

		flags = info->flags;
		if (((flags & UVC_CONTROL_GET_CUR) && !(inf & (1 << 0))) ||
		    ((flags & UVC_CONTROL_SET_CUR) && !(inf & (1 << 1)))) {
			uvc_trace(UVC_TRACE_CONTROL, "Control %pUl/%u flags "
				"don't match supported operations.\n",
				info->entity, info->selector);
			return;
		}
	}

	ctrl->info = info;
	ctrl->data = kmalloc(ctrl->info->size * UVC_CTRL_DATA_LAST, GFP_KERNEL);
	uvc_trace(UVC_TRACE_CONTROL, "Added control %pUl/%u to device %s "
		"entity %u\n", ctrl->info->entity, ctrl->info->selector,
		dev->udev->devpath, entity->id);
}

int uvc_ctrl_delete_info(struct uvc_control_info *info)
{
	struct uvc_control_info *ctrl;
	int ret = 0;

	/* Find matching controls by walking the devices, entities and
	 * controls list.
	 */
	mutex_lock(&uvc_driver.ctrl_mutex);

	/* First check if the list contains a control matching the new one.
	 * Bail out if it does.
	 */
	list_for_each_entry(ctrl, &uvc_driver.controls, list) {
	    if (memcmp(ctrl->entity, info->entity, 16) == 0) {
//			uvc_trace(UVC_TRACE_CONTROL,
//				"\n\nrichard before Control %pUl/%u ctrl->entity=%pUl ctrl->selector =%u .\n\n",
//				info->entity, info->selector,ctrl->entity,ctrl->selector);
			memset(ctrl->entity,0, 16);
		}
	}

	mutex_unlock(&uvc_driver.ctrl_mutex);
	return ret;
}

/*
 * Add an item to the UVC control information list, and instantiate a control
 * structure for each device that supports the control.
 */
int uvc_ctrl_add_info(struct uvc_control_info *info)
{
	struct uvc_control_info *ctrl;
	struct uvc_device *dev;
	int ret = 0;

	/* Find matching controls by walking the devices, entities and
	 * controls list.
	 */
	mutex_lock(&uvc_driver.ctrl_mutex);

	/* First check if the list contains a control matching the new one.
	 * Bail out if it does.
	 */
	list_for_each_entry(ctrl, &uvc_driver.controls, list) {
		if (memcmp(ctrl->entity, info->entity, 16))
			continue;

		if (ctrl->selector == info->selector) {
			uvc_trace(UVC_TRACE_CONTROL,
				"Control %pUl/%u is already defined.\n",
				info->entity, info->selector);
			ret = -EEXIST;
			goto end;
		}
		if (ctrl->index == info->index) {
			uvc_trace(UVC_TRACE_CONTROL,
				"Control %pUl/%u would overwrite index %d.\n",
				info->entity, info->selector, info->index);
			ret = -EEXIST;
			goto end;
		}
	}

	list_for_each_entry(dev, &uvc_driver.devices, list)
		uvc_ctrl_add_ctrl(dev, info);

	INIT_LIST_HEAD(&info->mappings);
	list_add_tail(&info->list, &uvc_driver.controls);
end:
	mutex_unlock(&uvc_driver.ctrl_mutex);
	return ret;
}

int uvc_ctrl_add_mapping(struct uvc_control_mapping *mapping)
{
	struct uvc_control_info *info;
	struct uvc_control_mapping *map;
	int ret = -EINVAL;

	if (mapping->get == NULL)
		mapping->get = uvc_get_le_value;
	if (mapping->set == NULL)
		mapping->set = uvc_set_le_value;

	if (mapping->id & ~V4L2_CTRL_ID_MASK) {
		uvc_trace(UVC_TRACE_CONTROL, "Can't add mapping '%s' with "
			"invalid control id 0x%08x\n", mapping->name,
			mapping->id);
		return -EINVAL;
	}

	mutex_lock(&uvc_driver.ctrl_mutex);
	list_for_each_entry(info, &uvc_driver.controls, list) {
		if (memcmp(info->entity, mapping->entity, 16) ||
			info->selector != mapping->selector)
			continue;

		if (info->size * 8 < mapping->size + mapping->offset) {
			uvc_trace(UVC_TRACE_CONTROL,
				"Mapping '%s' would overflow control %pUl/%u\n",
				mapping->name, info->entity, info->selector);
			ret = -EOVERFLOW;
			goto end;
		}

		/* Check if the list contains a mapping matching the new one.
		 * Bail out if it does.
		 */
		list_for_each_entry(map, &info->mappings, list) {
			if (map->id == mapping->id) {
				uvc_trace(UVC_TRACE_CONTROL, "Mapping '%s' is "
					"already defined.\n", mapping->name);
				ret = -EEXIST;
				goto end;
			}
		}

		mapping->ctrl = info;
		list_add_tail(&mapping->list, &info->mappings);
		uvc_trace(UVC_TRACE_CONTROL,
			"Adding mapping %s to control %pUl/%u.\n",
			mapping->name, info->entity, info->selector);

		ret = 0;
		break;
	}
end:
	mutex_unlock(&uvc_driver.ctrl_mutex);
	return ret;
}

/*
 * Prune an entity of its bogus controls using a blacklist. Bogus controls
 * are currently the ones that crash the camera or unconditionally return an
 * error when queried.
 */
static void
uvc_ctrl_prune_entity(struct uvc_device *dev, struct uvc_entity *entity)
{
	static const struct {
		struct usb_device_id id;
		u8 index;
	} blacklist[] = {
		{ { USB_DEVICE(0x13d3, 0x509b) }, 9 }, /* Gain */
		{ { USB_DEVICE(0x1c4f, 0x3000) }, 6 }, /* WB Temperature */
		{ { USB_DEVICE(0x5986, 0x0241) }, 2 }, /* Hue */
	};

	u8 *controls;
	unsigned int size;
	unsigned int i;

	if (UVC_ENTITY_TYPE(entity) != UVC_VC_PROCESSING_UNIT)
		return;

	controls = entity->processing.bmControls;
	size = entity->processing.bControlSize;

	for (i = 0; i < ARRAY_SIZE(blacklist); ++i) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 21)
		if ((blacklist[i].id.idVendor !=
		     le16_to_cpu(dev->udev->descriptor.idVendor)) ||
		    (blacklist[i].id.idProduct !=
		     le16_to_cpu(dev->udev->descriptor.idProduct)))
#else
		if (!usb_match_one_id(dev->intf, &blacklist[i].id))
#endif
			continue;

		if (blacklist[i].index >= 8 * size ||
		    !uvc_test_bit(controls, blacklist[i].index))
			continue;

		uvc_trace(UVC_TRACE_CONTROL, "%u/%u control is black listed, "
			"removing it.\n", entity->id, blacklist[i].index);

		uvc_clear_bit(controls, blacklist[i].index);
	}
}

/*
 * Initialize device controls.
 */
int uvc_ctrl_init_device(struct uvc_device *dev)
{
	struct uvc_control_info *info;
	struct uvc_control *ctrl;
	struct uvc_entity *entity;
	unsigned int i;

	/* Walk the entities list and instantiate controls */
	list_for_each_entry(entity, &dev->entities, list) {
		unsigned int bControlSize = 0, ncontrols = 0;
		__u8 *bmControls = NULL;

		if (UVC_ENTITY_TYPE(entity) == UVC_VC_EXTENSION_UNIT) {
			bmControls = entity->extension.bmControls;
			bControlSize = entity->extension.bControlSize;
		} else if (UVC_ENTITY_TYPE(entity) == UVC_VC_PROCESSING_UNIT) {
			bmControls = entity->processing.bmControls;
			bControlSize = entity->processing.bControlSize;
		} else if (UVC_ENTITY_TYPE(entity) == UVC_ITT_CAMERA) {
			bmControls = entity->camera.bmControls;
			bControlSize = entity->camera.bControlSize;
		}

		uvc_ctrl_prune_entity(dev, entity);

		for (i = 0; i < bControlSize; ++i)
			ncontrols += hweight8(bmControls[i]);

		if (ncontrols == 0)
			continue;

		entity->controls = kzalloc(ncontrols*sizeof *ctrl, GFP_KERNEL);
		if (entity->controls == NULL)
			return -ENOMEM;

		entity->ncontrols = ncontrols;

		ctrl = entity->controls;
		for (i = 0; i < bControlSize * 8; ++i) {
			if (uvc_test_bit(bmControls, i) == 0)
				continue;

			ctrl->entity = entity;
			ctrl->index = i;
			ctrl++;
		}
	}

	/* Walk the controls info list and associate them with the device
	 * controls, then add the device to the global device list. This has
	 * to be done while holding the controls lock, to make sure
	 * uvc_ctrl_add_info() will not get called in-between.
	 */
	mutex_lock(&uvc_driver.ctrl_mutex);
	list_for_each_entry(info, &uvc_driver.controls, list)
		uvc_ctrl_add_ctrl(dev, info);

	list_add_tail(&dev->list, &uvc_driver.devices);
	mutex_unlock(&uvc_driver.ctrl_mutex);

	return 0;
}

/*
 * Cleanup device controls.
 */
void uvc_ctrl_cleanup_device(struct uvc_device *dev)
{
	struct uvc_entity *entity;
	unsigned int i;

	/* Remove the device from the global devices list */
	mutex_lock(&uvc_driver.ctrl_mutex);
	if (dev->list.next != NULL)
		list_del(&dev->list);
	mutex_unlock(&uvc_driver.ctrl_mutex);

	list_for_each_entry(entity, &dev->entities, list) {
		for (i = 0; i < entity->ncontrols; ++i)
			kfree(entity->controls[i].data);

		kfree(entity->controls);
	}
}

void uvc_ctrl_init(void)
{
	struct uvc_control_info *ctrl = uvc_ctrls;
	struct uvc_control_info *cend = ctrl + ARRAY_SIZE(uvc_ctrls);
	struct uvc_control_mapping *mapping = uvc_ctrl_mappings;
	struct uvc_control_mapping *mend =
		mapping + ARRAY_SIZE(uvc_ctrl_mappings);

	for (; ctrl < cend; ++ctrl)
		uvc_ctrl_add_info(ctrl);

	for (; mapping < mend; ++mapping)
		uvc_ctrl_add_mapping(mapping);
}

// Houston 2011/06/20 XU ctrls +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int uvc_xu_ctrl_add( struct uvc_xu_control_info *xu_info, struct uvc_xu_control_mapping *xu_map)
{
	int ret=0;
	struct uvc_control_info *info;
	struct uvc_control_mapping *map;

	DbgPrint(My_TRACE_XU,"Adding XU Ctrls - %s\n", xu_map->name);

	/* try to add one controls */
	info = kzalloc(sizeof *info, GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	memcpy(info->entity, xu_info->entity, sizeof xu_info->entity);
	info->index = xu_info->index;
	info->selector = xu_info->selector;
	info->size = xu_info->size;
	info->flags = xu_info->flags;

	info->flags |= UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX |
	UVC_CONTROL_GET_RES | UVC_CONTROL_GET_DEF;
	uvc_ctrl_delete_info(info);

	ret = uvc_ctrl_add_info(info);
	if (ret < 0)
	{
		if(ret == -EEXIST)
			DbgPrint(My_TRACE_XU,"Add XU Ctrls \"%s\" already exist!\n", xu_map->name);
		else
			DbgPrint(My_TRACE_XU,"Adding XU Ctrls \"%s\" - Failed \n", xu_map->name);
		kfree(info);
		return ret;
	}
	else
		DbgPrint(My_TRACE_XU,"Adding XU Ctrls \"%s\" - Success \n", xu_map->name);

	/* after adding the controls, add the mapping now */
	map = kzalloc(sizeof *map, GFP_KERNEL);
	if (map == NULL)
		return -ENOMEM;

	DbgPrint(My_TRACE_XU,"Mapping XU Ctrls - %s\n", xu_map->name);

	map->id = xu_map->id;
	memcpy(map->name, xu_map->name, sizeof xu_map->name);
	memcpy(map->entity,xu_map->entity, sizeof xu_map->entity);
	map->selector = xu_map->selector;
	map->size = xu_map->size;
	map->offset = xu_map->offset;
	map->v4l2_type = xu_map->v4l2_type;
	map->data_type = xu_map->data_type;

	ret = uvc_ctrl_add_mapping(map);
	if (ret < 0)
	{
		if(ret == -EEXIST)
			DbgPrint(My_TRACE_XU,"Map XU Ctrls \"%s\" already exist!\n", xu_map->name);
		else
			DbgPrint(My_TRACE_XU,"Mapping XU Ctrls \"%s\" - Failed ", xu_map->name);
		kfree(map);
		return ret;
	}
	else
		DbgPrint(My_TRACE_XU,"Mapping XU Ctrls \"%s\" - Success \n", xu_map->name);

	return ret;
}

int uvc_xu_ctrl_init(struct uvc_device *dev)
{
	int i=0;
	int ret=0;
	int length = 0;
	unsigned int chip;
	struct uvc_xu_control_info *xu_infos;
	struct uvc_xu_control_mapping *xu_mappings;

	// Add xu READ ASIC first
	ret = uvc_xu_ctrl_add( &sonix_xu_sys_ctrls[i], &sonix_xu_sys_mappings[i]);
	if (ret < 0) return ret;
			
	// Read chip
	ret = uvc_xu_ctrll_ReadChip(dev, &chip);
	if (ret < 0) return ret;

	printk("chip is 0x%x\n", chip);
	// Decide which xu set had been add
	if(chip == CHIP_SNC291A)
	{
		xu_infos = sonix_xu_sys_ctrls;
		xu_mappings = sonix_xu_sys_mappings;
		i = 1;
		length = LENGTH_OF_SONIX_XU_SYS_CTR;
	}
	else if((chip == CHIP_SNC291B)||(chip == CHIP_SNC292A))
	{
		xu_infos = sonix_xu_usr_ctrls;
		xu_mappings = sonix_xu_usr_mappings;
		i = 0;
		length = LENGTH_OF_SONIX_XU_USR_CTR;
	}
	else
	{
		DbgPrint(My_TRACE_XU,"Unknown chip id 0x%x\n", chip);
		return -1;
	}

	// Add other xu accroding chip ID
	for ( ; i<length; i++ )
	{
		ret = uvc_xu_ctrl_add( &xu_infos[i], &xu_mappings[i]);
		if (ret < 0) break;
	}
	return ret;
}

int uvc_xu_ctrll_ReadChip(struct uvc_device *dev, int *chip)
{
	int ret = 0;
	__u8 ctrldata[4];
	struct uvc_xu_control xctrl;

	DbgPrint(My_TRACE_XU,"uvc_xu_ctrll_ReadChip ==>\n");

	*chip = CHIP_NONE;

#if 0
	xctrl.unit = 3;
#else
	xctrl.unit = 4;
#endif

	xctrl.selector = XU_SONIX_SYS_ASIC_READ;
	xctrl.size = 4;
	xctrl.data = ctrldata;

	xctrl.data[0] = 0x1f;
	xctrl.data[1] = 0x10;
	xctrl.data[2] = 0x0;
	xctrl.data[3] = 0xFF;/* Dummy Write */

	// Dummy Write
	ret = uvc_query_ctrl(dev, UVC_SET_CUR, xctrl.unit, dev->intfnum, xctrl.selector, xctrl.data, xctrl.size);
	if(ret < 0)
	{
		DbgPrint(My_TRACE_XU,"uvc_xu_ctrl_query(Dummy Write) - (%i) == \n",ret);
		return ret;
	}

	// Asic Read
	xctrl.data[3] = 0x00;
	ret = uvc_query_ctrl(dev, UVC_GET_CUR, xctrl.unit, dev->intfnum, xctrl.selector, xctrl.data, xctrl.size);
	if(ret < 0)
	{
		DbgPrint(My_TRACE_XU," uvc_xu_ctrl_query(Read ChipID) - FAILED (%i) == \n",ret);
		return ret;
	}

	DbgPrint(My_TRACE_XU,"   == uvc_xu_ctrll_ReadChip Success == \n");
	DbgPrint(My_TRACE_XU,"      ASIC READ data[0] : %x\n", xctrl.data[0]);
	DbgPrint(My_TRACE_XU,"      ASIC READ data[1] : %x\n", xctrl.data[1]);
	DbgPrint(My_TRACE_XU,"      ASIC READ data[2] : %x (Chip ID)\n", xctrl.data[2]);
	DbgPrint(My_TRACE_XU,"      ASIC READ data[3] : %x\n", xctrl.data[3]);

	printk("SONGHAN DSP236(1004),CHIP ID:0x25 CHIP_SONGHAN_DSP_DSP236,xctrl.data[2]=0x%x\n",xctrl.data[2]);

	if(0x25 == xctrl.data[2])
	{
		printk("SONGHAN DSP236(1004),CHIP ID:0x25 CHIP_SONGHAN_DSP_DSP236\n");
		chip_type = CHIP_SONGHAN_DSP_DSP236 ;
    }

	if(xctrl.data[2] == SONIX_SN9C291_SERIES_CHIPID)
	{
		*chip = CHIP_SNC291A;
	}	
	if(xctrl.data[2] == SONIX_SN9C292_SERIES_CHIPID)
	{
		xctrl.data[0] = 0x07;		//DRAM SIZE
		xctrl.data[1] = 0x16;
		xctrl.data[2] = 0x0;
		xctrl.data[3] = 0xFF;		/* Dummy Write */

		/* Dummy Write */
		ret = uvc_query_ctrl(dev, UVC_SET_CUR, xctrl.unit, dev->intfnum, xctrl.selector, xctrl.data, xctrl.size);
		if(ret < 0)
		{
			DbgPrint(My_TRACE_XU,"uvc_xu_ctrl_query(Dummy Write) - (%i) == \n",ret);
			return ret;
		}

		/* Asic Read */
		xctrl.data[3] = 0x00;
		ret = uvc_query_ctrl(dev, UVC_GET_CUR, xctrl.unit, dev->intfnum, xctrl.selector, xctrl.data, xctrl.size);
		if(ret < 0)
		{
			DbgPrint(My_TRACE_XU," uvc_xu_ctrl_query(Read ChipID) - FAILED (%i) == \n",ret);
			return ret;
		}
		
		if(xctrl.data[2] == SONIX_SN9C292_DDR_64M)
			*chip = CHIP_SNC292A;
		else if(xctrl.data[2] == SONIX_SN9C292_DDR_16M)
			*chip = CHIP_SNC291B;			
	}

	DbgPrint(My_TRACE_XU,"uvc_xu_ctrll_ReadChip is (%x)<==\n",*chip);
	return ret;
}
// Houston 2011/06/20 XU ctrls ------------------------------------------------------------------------------
