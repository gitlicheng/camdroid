#define MAX_RECTS 10

typedef struct Rects
{
	int x[MAX_RECTS];
	int y[MAX_RECTS];
	int width[MAX_RECTS];
	int height[MAX_RECTS];
	int num;
}Rects;
	
extern "C" void *awInit(int width,int height);
extern "C" void awRun(void *faceDetectContext,unsigned char *frameData);
extern "C" void awRelease(void *faceDetectContext);
extern "C" void awGetRect(void * faceDetectContext,Rects * rects);

