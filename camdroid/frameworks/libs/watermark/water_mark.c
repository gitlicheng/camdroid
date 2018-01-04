#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LOG_TAG "Water_mark"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <cutils/log.h>

#include <water_mark.h>

//only the fg_width is 8 pixels align and the fg_height is 2 pixels align
//#define	USE_NEON

// bg_width			background width
// bg_height        background height

// left             foreground position of left
// top              foreground position of top
// fg_width         foreground width
// fg_height        foreground height

// bg_y           	point to the background YUV420sp format y component data
// bg_c           	point to the background YUV420sp format c component data
// fg_y           	point to the foreground YUV420sp format y component data
// fg_c           	point to the foreground YUV420sp format c component data
// alph             point to foreground alph value

void yuv420sp_blending(int bg_width, int bg_height, int left, unsigned int top, int fg_width, int fg_height,
					   unsigned char *bg_y, unsigned char *bg_c, unsigned char *fg_y, unsigned char *fg_c, unsigned char *alph)
{
	unsigned char *bg_y_p = NULL;
	unsigned char *bg_c_p = NULL;
	int i = 0;
	int j = 0;

	unsigned char *bg_y_p_line2 = NULL;
	unsigned char *fg_y_line2 = NULL;
	unsigned char *alph_line2 = NULL;

	bg_y_p = bg_y + top * bg_width + left;
	bg_c_p = bg_c + (top >>1)*bg_width + left;

	bg_y_p_line2 = bg_y_p + bg_width;
	fg_y_line2 = fg_y + fg_width;
	alph_line2 = alph + fg_width;
	
#ifdef USE_NEON
	for(i = 0; i<(int)fg_height/2; i++)
		{
			for(j=0; j< (int)fg_width/8; j++)
			{
				asm volatile (
						
   				        "vld1.u8         {d0}, [%[bg_y_p]]              \n\t"
   				        "vld1.u8         {d7}, [%[bg_y_p_line2]]        \n\t"
   				        "vld1.u8         {d3}, [%[bg_c_p]]              \n\t"
   				         
   				        "vld1.u8         {d2}, [%[fg_y]]!               \n\t"
   				        "vld1.u8         {d9}, [%[fg_y_line2]]!         \n\t"
   				        "vld1.u8         {d4}, [%[fg_c]]!               \n\t"

						"vld1.u8         {d1}, [%[alph]]!               \n\t"
   				        "vld1.u8         {d8}, [%[alph_line2]]!         \n\t"
   				        

					    "pld         	 [%[bg_y_p],#64]               \n\t"
					    "pld         	 [%[bg_y_p_line2],#64]        \n\t"
					    "pld         	 [%[bg_c_p],#64]             \n\t"
					    
					    "pld         	 [%[fg_y],#64]               \n\t"
					    "pld         	 [%[fg_y_line2],#64]          \n\t"
    					"pld         	 [%[fg_c],#64]               \n\t"

						"pld         	 [%[alph],#64]               \n\t"
						"pld         	 [%[alph_line2],#64]          \n\t" 
						
    					
    					"mov         	 lr,   #255              		\n\t"
    					"vdup.u8		 d5,   lr						\n\t"
    					"mov         	 lr,   #1              			\n\t"
    					"vdup.u8		 d12,   lr						\n\t"
    					
    					"vsub.u8		 d6,   d5,	d1					\n\t"
    					"vmull.u8		 q15,  d6,	d0					\n\t"
    					"vmlal.u8		 q15,  d2,	d1					\n\t"
    					"vmlal.u8		 q15,  d12,	d0					\n\t"

    					"vsub.u8		 d10,  d5,	d8					\n\t"
    					"vmull.u8		 q12,  d10,	d7					\n\t"
    					"vmlal.u8		 q12,  d9,	d8					\n\t"
    					"vmlal.u8		 q12,  d12,	d7					\n\t"
    					
    
    					"vmull.u8		 q14,  d6,	d3					\n\t"
    					"vmlal.u8		 q14,  d4,	d1					\n\t"
    					"vmlal.u8		 q14,  d12,	d3					\n\t"
    					
    
    					"vshrn.u16		 d27,  q14,	#8					\n\t"
    					"vshrn.u16		 d26,  q15,	#8					\n\t"
    					"vshrn.u16		 d23,  q12,	#8					\n\t"
    
    					"vst1.u8		 {d26},  [%[bg_y_p]]!			\n\t"
    					"vst1.u8		 {d23},  [%[bg_y_p_line2]]!		\n\t"
    					"vst1.u8		 {d27},  [%[bg_c_p]]!			\n\t"
    				    : [bg_y_p] "+r" (bg_y_p), [alph] "+r" (alph), [fg_y] "+r" (fg_y), [bg_c_p] "+r" (bg_c_p), [fg_c] "+r" (fg_c), [bg_y_p_line2] "+r" (bg_y_p_line2), [alph_line2] "+r" (alph_line2), [fg_y_line2] "+r" (fg_y_line2)
    				    :  //[srcY] "r" (srcY)
    				    : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d19", "d20", "d21", "d22", "d23", "d26", "d27", "d29", "d30", "d31"
    				    );
				}
			alph 		+= fg_width;
			alph_line2 	+= fg_width;
			fg_y 		+= fg_width;
			fg_y_line2 	+= fg_width;
			
			bg_y_p 		+= 2*bg_width - fg_width;
			bg_y_p_line2+= 2*bg_width - fg_width;
            bg_c_p		+= bg_width - fg_width;
	}
#else
	for(i = 0; i<(int)fg_height; i++)
	{
		if((i&1) == 0)
		{
			for(j=0; j< (int)fg_width; j++)
			{
				*bg_y_p = ((256 - *alph)*(*bg_y_p) + (*fg_y++)*(*alph))>>8;
				*bg_c_p = ((256 - *alph)*(*bg_c_p) + (*fg_c++)*(*alph))>>8;
					
				alph++;
				bg_y_p++;
				bg_c_p++;
			}

			bg_c_p = bg_c_p + bg_width - fg_width;
		}
		else
		{
			for(j=0; j< (int)fg_width; j++)
			{
				*bg_y_p = ((256 - *alph)*(*bg_y_p) + (*fg_y++)*(*alph))>>8;
				alph++;
				bg_y_p++;
			}
		}
		bg_y_p = bg_y_p + bg_width - fg_width;
	}
#endif
}

// bg_width			background width
// bg_height        background height

// left             foreground position of left
// top              foreground position of top
// fg_width         foreground width
// fg_height        foreground height
// bg_y           	point to the background YUV420sp format y component data

// return value : 0 dark, 1 bright

int region_bright_or_dark(int bg_width, int bg_height, int left, int top, int fg_width, int fg_height, unsigned char *bg_y)
{
	unsigned char *bg_y_p = NULL;

	int i = 0;
	int j = 0;
	int bright_line_number = 0;
	int value = 0;

	bg_y_p = bg_y + top * bg_width + left;
	
	#ifdef USE_NEON
	int value_line2 = 0;
	unsigned char* bg_y_p_line2 = bg_y_p + bg_width;

	int *pValue = &value;
	int *pValue_line2 = &value_line2;

	for(i = 0; i<(int)fg_height/2; i++)
	{
		value = 0;
		value_line2 = 0;
		asm volatile (
   				        "vld1.u8         {d0,d1,d2}, [%[bg_y_p]]              \n\t"
   				        "vld1.u8         {d3,d4,d5}, [%[bg_y_p_line2]]              \n\t"
						
    					
    					"vpaddl.u8       d31,  d0          				\n\t"
    					"vpaddl.u8       d30,  d1          				\n\t"
    					"vpaddl.u8       d29,  d2          				\n\t"
						"vpadd.u16       d28,  d31, d30          		\n\t"
						"vpadd.u16       d27,  d28, d29          		\n\t"
						"vpaddl.u16      d28,  d27		         		\n\t"
						"vpaddl.u32      d31,  d28		         		\n\t"
		
    					"vpaddl.u8       d30,  d3          				\n\t"
    					"vpaddl.u8       d29,  d4          				\n\t"
    					"vpaddl.u8       d28,  d5          				\n\t"
						"vpadd.u16       d27,  d30, d29          		\n\t"
						"vpadd.u16       d26,  d27, d28          		\n\t"
						"vpaddl.u16      d27,  d26		         		\n\t"
						"vpaddl.u32      d30,  d27		         		\n\t"
    
    					"vmov.u32		 r0, d31[0]  					\n\t"
    					"vmov.u32		 r1, d30[0]  					\n\t"
    					"STR		 	 r0, [%[pValue]]  				\n\t"
    					"STR		 	 r1, [%[pValue_line2]]  		\n\t"

						
    				    : [bg_y_p] "+r" (bg_y_p), [bg_y_p_line2] "+r" (bg_y_p_line2), [pValue] "+r" (pValue), [pValue_line2] "+r" (pValue_line2)
    				    :  //[srcY] "r" (srcY)
    				    : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d26", "d27", "d28", "d29", "d30", "d31"
    				    );
		
		value = value/fg_width;
		if(value > 128) {
			bright_line_number++;
		}	

		value_line2 = value_line2/fg_width;
		if(value_line2 > 128) {
			bright_line_number++;
		}	

		bg_y_p = bg_y_p + 2*bg_width;
		bg_y_p_line2 = bg_y_p_line2 + 2*bg_width;
	}
#else
	for(i = 0; i<(int)fg_height; i++)
	{
		value = 0;
		for(j=0; j< (int)fg_width; j++)
		{
			value += *bg_y_p++;
		}
		value = value/fg_width;

		if(value > 128) {
			bright_line_number++;
		}		

		bg_y_p = bg_y_p + bg_width - fg_width;
	}
#endif

	if(bright_line_number > fg_height/2) {
		return 1;
	}

	return 0;
}


// bg_width			background width
// bg_height        background height

// left             foreground position of left
// top              foreground position of top
// fg_width         foreground width
// fg_height        foreground height

// bg_y           	point to the background YUV420sp format y component data
// bg_c           	point to the background YUV420sp format c component data
// fg_y           	point to the foreground YUV420sp format y component data
// fg_c           	point to the foreground YUV420sp format c component data
// alph             point to foreground alph value

void yuv420sp_blending_adjust_brightness(int bg_width, int bg_height, int left, int top, int fg_width, int fg_height,
					   unsigned char *bg_y, unsigned char *bg_c, unsigned char *fg_y, unsigned char *fg_c, unsigned char *alph,
					   int is_brightness)
{
	unsigned char *bg_y_p = NULL;
	unsigned char *bg_c_p = NULL;

	int i = 0;
	int j = 0;

	if (bg_y == NULL || bg_c == NULL || fg_y == NULL || fg_c == NULL || alph == NULL) {
		ALOGE("<yuv420sp_blending_adjust_brightness>NULL pointer error!");
		return;
	}

	
	unsigned char *bg_y_p_line2 = NULL;
	unsigned char *fg_y_line2 = NULL;
	unsigned char *alph_line2 = NULL;
	
	bg_y_p = bg_y + top * bg_width + left;
	bg_c_p = bg_c + (top >>1)*bg_width + left;

	bg_y_p_line2 = bg_y_p + bg_width;
	fg_y_line2 = fg_y + fg_width;
	alph_line2 = alph + fg_width;
		
	#ifdef USE_NEON
	if(is_brightness) {
		for(i = 0; i<(int)fg_height/2; i++)
		{
			for(j=0; j< (int)fg_width/8; j++)
			{
				
				asm volatile (
				        "vld1.u8         {d0}, [%[bg_y_p]]              \n\t"
   				        "vld1.u8         {d7}, [%[bg_y_p_line2]]        \n\t"
   				        "vld1.u8         {d3}, [%[bg_c_p]]              \n\t"
   				         
   				        "vld1.u8         {d2}, [%[fg_y]]!               \n\t"
   				        "vld1.u8         {d9}, [%[fg_y_line2]]!         \n\t"
   				        "vld1.u8         {d4}, [%[fg_c]]!               \n\t"

						"vld1.u8         {d1}, [%[alph]]!               \n\t"
   				        "vld1.u8         {d8}, [%[alph_line2]]!         \n\t"
   				        

					    "pld         	 [%[bg_y_p],#64]               \n\t"
					    "pld         	 [%[bg_y_p_line2],#64]        \n\t"
					    "pld         	 [%[bg_c_p],#64]             \n\t"
					    
					    "pld         	 [%[fg_y],#64]               \n\t"
					    "pld         	 [%[fg_y_line2],#64]          \n\t"
    					"pld         	 [%[fg_c],#64]               \n\t"

						"pld         	 [%[alph],#64]               \n\t"
						"pld         	 [%[alph_line2],#64]          \n\t" 
					
						"mov         	 lr,   #255              		\n\t"
						"vdup.u8		 d5,   lr						\n\t"
						"mov         	 lr,   #1              			\n\t"
    					"vdup.u8		 d12,   lr						\n\t"
    					
						"vsub.u8		 d6,   d5,	d1					\n\t"
						"vmull.u8		 q15,  d6,	d0					\n\t"
						"vsub.u8		 d10,  d5,	d2					\n\t"
						"vmlal.u8		 q11,  d10,	d1					\n\t"
						"vaddl.u8		 q10,  d0,	d1					\n\t"
						"vadd.u16		 q15,  q10,	q11					\n\t"

						"vsub.u8		 d11,  d5,	d8					\n\t"
    					"vmull.u8		 q12,  d11,	d7					\n\t"
    					"vsub.u8		 d10,  d5,	d9					\n\t"
    					"vmlal.u8		 q11,  d9,	d8					\n\t"
    					"vaddl.u8		 q10,  d7,	d8					\n\t"
						"vadd.u16		 q12,  q10,	q11					\n\t"

						"vmull.u8		 q14,  d6,	d3					\n\t"
						"vmlal.u8		 q14,  d4,	d1					\n\t"
						"vmlal.u8		 q14,  d12,	d3					\n\t"

						"vshrn.u16		 d27,  q14,	#8					\n\t"
						"vshrn.u16		 d26,  q15,	#8					\n\t"
						"vshrn.u16		 d23,  q12,	#8					\n\t"

						"vst1.u8		 {d26},  [%[bg_y_p]]!			\n\t"
						"vst1.u8		 {d27},  [%[bg_c_p]]!			\n\t"
						"vst1.u8		 {d23},  [%[bg_y_p_line2]]!		\n\t"
				        : [bg_y_p] "+r" (bg_y_p), [alph] "+r" (alph), [fg_y] "+r" (fg_y), [bg_c_p] "+r" (bg_c_p), [fg_c] "+r" (fg_c), [bg_y_p_line2] "+r" (bg_y_p_line2), [alph_line2] "+r" (alph_line2), [fg_y_line2] "+r" (fg_y_line2)
    				    :  //[srcY] "r" (srcY)
    				    : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
    				   );
			}
			alph 		+= fg_width;
			alph_line2 	+= fg_width;
			fg_y 		+= fg_width;
			fg_y_line2 	+= fg_width;
			
			bg_y_p 		+= 2*bg_width - fg_width;
			bg_y_p_line2+= 2*bg_width - fg_width;
		}
	} else {
		for(i = 0; i<(int)fg_height/2; i++)
		{
			for(j=0; j< (int)fg_width/8; j++)
			{
				asm volatile (
						
   				        "vld1.u8         {d0}, [%[bg_y_p]]              \n\t"
   				        "vld1.u8         {d7}, [%[bg_y_p_line2]]        \n\t"
   				        "vld1.u8         {d3}, [%[bg_c_p]]              \n\t"
   				         
   				        "vld1.u8         {d2}, [%[fg_y]]!               \n\t"
   				        "vld1.u8         {d9}, [%[fg_y_line2]]!         \n\t"
   				        "vld1.u8         {d4}, [%[fg_c]]!               \n\t"

						"vld1.u8         {d1}, [%[alph]]!               \n\t"
   				        "vld1.u8         {d8}, [%[alph_line2]]!         \n\t"
   				        

					    "pld         	 [%[bg_y_p],#64]               \n\t"
					    "pld         	 [%[bg_y_p_line2],#64]        \n\t"
					    "pld         	 [%[bg_c_p],#64]             \n\t"
					    
					    "pld         	 [%[fg_y],#64]               \n\t"
					    "pld         	 [%[fg_y_line2],#64]          \n\t"
    					"pld         	 [%[fg_c],#64]               \n\t"

						"pld         	 [%[alph],#64]               \n\t"
						"pld         	 [%[alph_line2],#64]          \n\t" 
						
    					
    					"mov         	 lr,   #255              		\n\t"
    					"vdup.u8		 d5,   lr						\n\t"
    					"mov         	 lr,   #1              			\n\t"
    					"vdup.u8		 d12,   lr						\n\t"
    					
    					"vsub.u8		 d6,   d5,	d1					\n\t"
    					"vmull.u8		 q15,  d6,	d0					\n\t"
    					"vmlal.u8		 q15,  d2,	d1					\n\t"
    					"vmlal.u8		 q15,  d12,	d0					\n\t"

    					"vsub.u8		 d10,  d5,	d8					\n\t"
    					"vmull.u8		 q12,  d10,	d7					\n\t"
    					"vmlal.u8		 q12,  d9,	d8					\n\t"
    					"vmlal.u8		 q12,  d12,	d7					\n\t"
    					
    
    					"vmull.u8		 q14,  d6,	d3					\n\t"
    					"vmlal.u8		 q14,  d4,	d1					\n\t"
    					"vmlal.u8		 q14,  d12,	d3					\n\t"
    					
    
    					"vshrn.u16		 d27,  q14,	#8					\n\t"
    					"vshrn.u16		 d26,  q15,	#8					\n\t"
    					"vshrn.u16		 d23,  q12,	#8					\n\t"
    
    					"vst1.u8		 {d26},  [%[bg_y_p]]!			\n\t"
    					"vst1.u8		 {d23},  [%[bg_y_p_line2]]!		\n\t"
    					"vst1.u8		 {d27},  [%[bg_c_p]]!			\n\t"
    				    : [bg_y_p] "+r" (bg_y_p), [alph] "+r" (alph), [fg_y] "+r" (fg_y), [bg_c_p] "+r" (bg_c_p), [fg_c] "+r" (fg_c), [bg_y_p_line2] "+r" (bg_y_p_line2), [alph_line2] "+r" (alph_line2), [fg_y_line2] "+r" (fg_y_line2)
    				    :  //[srcY] "r" (srcY)
    				    : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d19", "d20", "d21", "d22", "d23", "d26", "d27", "d29", "d30", "d31"
    				    );
				}
			alph 		+= fg_width;
			alph_line2 	+= fg_width;
			fg_y 		+= fg_width;
			fg_y_line2 	+= fg_width;
			
			bg_y_p 		+= 2*bg_width - fg_width;
			bg_y_p_line2+= 2*bg_width - fg_width;
            bg_c_p		+= bg_width - fg_width;
		}
	}
#else
	if(is_brightness) {
			for(i = 0; i<(int)fg_height; i++)
			{
				if((i&1) == 0)
				{
					for(j=0; j< (int)fg_width; j++)
					{
						*bg_y_p = ((256 - *alph)*(*bg_y_p) + (256 - (*fg_y++))*(*alph))>>8;
						*bg_c_p = ((256 - *alph)*(*bg_c_p) + (*fg_c++)*(*alph))>>8;
						
						alph++;
						bg_y_p++;
						bg_c_p++;
					}
					bg_c_p = bg_c_p + bg_width - fg_width;
				}
				else
				{
					for(j=0; j< (int)fg_width; j++)
					{
						*bg_y_p = ((256 - *alph)*(*bg_y_p) + (256 - (*fg_y++))*(*alph))>>8;
						alph++;
						bg_y_p++;
					}
				}
	
				bg_y_p = bg_y_p + bg_width - fg_width;
			}
		} else {
			for(i = 0; i<(int)fg_height; i++)
			{
				if((i&1) == 0)
				{
					for(j=0; j< (int)fg_width; j++)
					{
						*bg_y_p = ((256 - *alph)*(*bg_y_p) + (*fg_y++)*(*alph))>>8;
						*bg_c_p = ((256 - *alph)*(*bg_c_p) + (*fg_c++)*(*alph))>>8;
						
						alph++;
						bg_y_p++;
						bg_c_p++;
					}
					bg_c_p = bg_c_p + bg_width - fg_width;
				}
				else
				{
					for(j=0; j< (int)fg_width; j++)
					{
						*bg_y_p = ((256 - *alph)*(*bg_y_p) + (*fg_y++)*(*alph))>>8;
						alph++;
						bg_y_p++;
					}
				}
	
				bg_y_p = bg_y_p + bg_width - fg_width;
			}
		}
#endif	
}

int watermark_blending(BackGroudLayerInfo *bg_info, WaterMarkInfo *wm_info, ShowWaterMarkParam *wm_Param)
{
	int i;
	int id;
	int total_width = 0;
	int pos_x;

	for (i = 0; i < wm_Param->number; ++i) {
		id = wm_Param->id_list[i];
		total_width += wm_info->single_pic[id].width;
	}
	if(total_width > bg_info->width) {
		ALOGE("<watermark_blending> error region(total_width=%d, bg_width=%d)", total_width, bg_info->width);
		return -1;
	}

	pos_x = wm_Param->pos.x;
	for(i = 0; i < wm_Param->number; i++)
	{
		id = wm_Param->id_list[i];
		yuv420sp_blending(bg_info->width, bg_info->height, pos_x, wm_Param->pos.y, 
										wm_info->single_pic[id].width, wm_info->single_pic[id].height,
										bg_info->y, bg_info->c,
						   				wm_info->single_pic[id].y, wm_info->single_pic[id].c,
						   				wm_info->single_pic[id].alph);
			pos_x += wm_info->single_pic[id].width;
	}
	return 0;
}

int watermark_blending_ajust_brightness(BackGroudLayerInfo *bg_info, WaterMarkInfo *wm_info, ShowWaterMarkParam *wm_Param,
	AjustBrightnessParam *ajust_Param)
{
	int i;
	int id;
	int is_brightness;
	int total_width = 0;
	int pos_x;

	for (i = 0; i < wm_Param->number; ++i) {
		id = wm_Param->id_list[i];
		total_width += wm_info->single_pic[id].width;
	}
	if(total_width > bg_info->width) {
		ALOGE("<watermark_blending_ajust_brightness> error region(total_width=%d, bg_width=%d)", total_width, bg_info->width);
		return -1;
	}

	if(ajust_Param->cur_frame > ajust_Param->recycle_frame)
	{
		ajust_Param->cur_frame = 0;
	} else {
		ajust_Param->cur_frame++;
	}

	pos_x = wm_Param->pos.x;
	if(ajust_Param->cur_frame == 0)
	{
		for(i = 0; i<wm_Param->number; i++)
		{
			id = wm_Param->id_list[i];
			is_brightness =  region_bright_or_dark(bg_info->width, bg_info->height, pos_x, wm_Param->pos.y,
												wm_info->single_pic[id].width, wm_info->single_pic[id].height, bg_info->y);
			ajust_Param->Brightness[i] = is_brightness;

			yuv420sp_blending_adjust_brightness(bg_info->width, bg_info->height, pos_x, wm_Param->pos.y,
											wm_info->single_pic[id].width, wm_info->single_pic[id].height,
											bg_info->y, bg_info->c,
											wm_info->single_pic[id].y, wm_info->single_pic[id].c,
											wm_info->single_pic[id].alph,
											is_brightness);
			pos_x += wm_info->single_pic[id].width;
		}
	}
	else
	{
		for(i = 0; i<wm_Param->number; i++)
		{
			id = wm_Param->id_list[i];
			
			is_brightness = ajust_Param->Brightness[i];
			
			yuv420sp_blending_adjust_brightness(bg_info->width, bg_info->height, pos_x, wm_Param->pos.y,
											wm_info->single_pic[id].width, wm_info->single_pic[id].height,
											bg_info->y, bg_info->c,
											wm_info->single_pic[id].y, wm_info->single_pic[id].c,
											wm_info->single_pic[id].alph,
											is_brightness);
			pos_x += wm_info->single_pic[id].width;
		}
	}
	
	return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


