
#ifndef __FACE_DETECTION_API_H___
#define __FACE_DETECTION_API_H___

#include <utils/StrongPointer.h>

namespace android {

typedef int (*face_notify_cb)(int cmd, void * data, void *user);

enum FACE_NOTITY_CMD{
	FACE_NOTITY_CMD_REQUEST_FRAME,
	FACE_NOTITY_CMD_RESULT,
	FACE_NOTITY_CMD_POSITION,
	FACE_NOTITY_CMD_REQUEST_ORIENTION,
};

class CFaceDetection;

enum FACE_OPS_CMD
{
	FACE_OPS_CMD_START,
	FACE_OPS_CMD_STOP,
	FACE_OPS_CMD_REGISTE_USER,
};

struct FocusArea_t
{
	int x;
	int y;
	int x1;
	int y1;
};

struct FaceDetectionDev
{
	void * user;
	sp<CFaceDetection> priv;
	void (*setCallback)(FaceDetectionDev * dev, face_notify_cb cb);
	int (*ioctrl)(FaceDetectionDev * dev, int cmd, int para0, int para1);
};

extern int CreateFaceDetectionDev(FaceDetectionDev ** dev);
extern void DestroyFaceDetectionDev(FaceDetectionDev * dev);

}

#endif	// __FACE_DETECTION_API_H__
