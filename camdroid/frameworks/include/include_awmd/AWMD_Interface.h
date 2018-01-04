#ifndef AWMD_INTERFACE_H
#define AWMD_INTERFACE_H

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct AWMDVar
{

	int state;
	void* bk;
	void* scene;
	void* analyze;
	void* para;

//	void *pData;

}AWMDVar;

/*------------------------------*/
//vpRun return value
//1:motion
//2:roi shelter
//4:camera shelter
//3:motion + (roi shelter),other and so on.
/*------------------------------*/
typedef struct AWMD_
{
	AWMDVar variable;

	//========= real function interface ==============================
	int (*vpInit)(AWMDVar* p_awmd_var,int sensitivity_level);

	//================================================================

	//========= fake function interface ==============================
	int (*vpSetROIMask)(AWMDVar* p_awmd_var,unsigned char* p_mask,int width,int height);
	const unsigned char* (*vpGetMask)(AWMDVar* p_awmd_var);
	int (*vpSetSensitivityLevel)(AWMDVar* p_awmd_var,int  sensitivity_level);
	int (*vpSetShelterPara)(AWMDVar* p_awmd_var,int b_shelter_detect,int b_roi_shelter_detect);
	int (*vpRun)(unsigned char* frame_buf,AWMDVar* p_awmd_var);
	int (*vpReset)(AWMDVar* p_awmd_var);
	int (*vpResize)(void* scene,unsigned char* src_img ,unsigned char* dst_img);
	//================================================================

}AWMD;

AWMD* allocAWMD(int width,int height,int width_resized,int height_resized, int count_threshold);
int freeAWMD(AWMD *ptr);

#ifdef __cplusplus
	}
#endif

#endif // AWMD_INTERFACE_H
