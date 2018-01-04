//----------------------------------------------//
//	H264 NALU c source code						//
//----------------------------------------------//

#include <linux/videodev2.h>
#include "nalu.h"

unsigned char* FindNextH264StartCode(unsigned char *pBuf, unsigned char *pBuf_end)
{
    unsigned char zero_cnt;
    zero_cnt = 0;

    while(1)
    {
        if(pBuf == pBuf_end)
        {
            break;
        }
        if(*pBuf == 0)
        {
            zero_cnt++;
        }
        else if((zero_cnt >= 3) && (*pBuf == 1))
        {
            zero_cnt = 0;
            pBuf++;
            break;
        }
        else
        {
            zero_cnt = 0;
        }
        pBuf++;
    }
    return pBuf;
}

unsigned int Ue(unsigned char *pBuff, unsigned int nLen, unsigned int *nStartBit)
{
	unsigned int i=0;
    unsigned int nZeroNum = 0;
	unsigned long dwRet = 0;
	
    while((*nStartBit) < (nLen * 8))
    {
        if(pBuff[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8)))
            break;

        nZeroNum++;
        (*nStartBit)++;
    }
    (*nStartBit) ++;

    for(i=0; i<nZeroNum; i++)
    {
        dwRet <<= 1;
        if((pBuff[(*nStartBit) / 8]) & (0x80 >> ((*nStartBit) % 8)))
        {
            dwRet += 1;
        }
        (*nStartBit)++;
    }
    return (1 << nZeroNum) - 1 + dwRet;
}

int Se(unsigned char *pBuff, unsigned int nLen, unsigned int *nStartBit)
{
#if 0
    int UeVal=Ue(pBuff,nLen,*nStartBit);
	double k=UeVal;
	int nValue=ceil(k/2);
    
    if(UeVal % 2==0)
        nValue=-nValue;

    return nValue;
#else
    int UeVal=Ue(pBuff,nLen,nStartBit);
	int nValue = UeVal / 2;
	if( nValue > 0) nValue += UeVal & 0x01;
	if(UeVal % 2==0)
		nValue=-nValue;

	return nValue;
#endif
}

unsigned long u(unsigned int BitCount,unsigned char * buf,unsigned int *nStartBit)
{
    unsigned long dwRet = 0;
	unsigned int i = 0;
	
	for(i=0; i<BitCount; i++)
    {
        dwRet <<= 1;
        if(buf[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8)))
        {
            dwRet += 1;
        }
        (*nStartBit)++;
    }

    return dwRet;
}

bool h264_decode_seq_parameter_set(unsigned char * buf, unsigned int nLen, int *Width, int *Height)
{
    unsigned int StartBit=0;
    int forbidden_zero_bit, nal_ref_idc, nal_unit_type;
	
	forbidden_zero_bit=u(1,buf,&StartBit);
    nal_ref_idc=u(2,buf,&StartBit);
    nal_unit_type=u(5,buf,&StartBit);
	
    if(nal_unit_type==7)
    {
		int profile_idc, constraint_set0_flag, constraint_set1_flag, constraint_set2_flag, constraint_set3_flag, reserved_zero_4bits, level_idc;
		int seq_parameter_set_id, log2_max_frame_num_minus4, pic_order_cnt_type;
		int num_ref_frames, gaps_in_frame_num_value_allowed_flag, pic_width_in_mbs_minus1, pic_height_in_map_units_minus1;
		
		profile_idc=u(8,buf,&StartBit);
        constraint_set0_flag=u(1,buf,&StartBit);//(buf[1] & 0x80)>>7;
        constraint_set1_flag=u(1,buf,&StartBit);//(buf[1] & 0x40)>>6;
        constraint_set2_flag=u(1,buf,&StartBit);//(buf[1] & 0x20)>>5;
        constraint_set3_flag=u(1,buf,&StartBit);//(buf[1] & 0x10)>>4;
        reserved_zero_4bits=u(4,buf,&StartBit);
        level_idc=u(8,buf,&StartBit);
		
        seq_parameter_set_id=Ue(buf,nLen,&StartBit);

        if((profile_idc == 100) || (profile_idc == 110) || (profile_idc == 122) || (profile_idc == 144))
        {
			int chroma_format_idc, residual_colour_transform_flag, bit_depth_luma_minus8, bit_depth_chroma_minus8, qpprime_y_zero_transform_bypass_flag;
			int seq_scaling_matrix_present_flag;
            
			chroma_format_idc=Ue(buf,nLen,&StartBit);
			residual_colour_transform_flag = 0;
			
            if(chroma_format_idc == 3)
                residual_colour_transform_flag=u(1,buf,&StartBit);

            bit_depth_luma_minus8=Ue(buf,nLen,&StartBit);
            bit_depth_chroma_minus8=Ue(buf,nLen,&StartBit);
            qpprime_y_zero_transform_bypass_flag=u(1,buf,&StartBit);
            seq_scaling_matrix_present_flag=u(1,buf,&StartBit);

            if(seq_scaling_matrix_present_flag)
            {
				int seq_scaling_list_present_flag[8];
				int i=0;
                for( i = 0; i < 8; i++ )
                {
                    seq_scaling_list_present_flag[i]=u(1,buf,&StartBit);
                }
            }
        }
        
		log2_max_frame_num_minus4=Ue(buf,nLen,&StartBit);
        pic_order_cnt_type=Ue(buf,nLen,&StartBit);
		
        if(pic_order_cnt_type == 0)
        {
            int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,&StartBit);
        }
        else if(pic_order_cnt_type == 1)
        {
            int delta_pic_order_always_zero_flag=u(1,buf,&StartBit);
            int offset_for_non_ref_pic=Se(buf,nLen,&StartBit);
            int offset_for_top_to_bottom_field=Se(buf,nLen,&StartBit);
            int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,&StartBit);
#if 0
            int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];
            for(int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
                offset_for_ref_frame[i]=Se(buf,nLen,&StartBit);

            delete [] offset_for_ref_frame;
#else
			int offset_for_ref_frame = 0;
			int i = 0;
            for(i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
                offset_for_ref_frame =Se(buf,nLen,&StartBit);
#endif

        }

        num_ref_frames=Ue(buf,nLen,&StartBit);
        gaps_in_frame_num_value_allowed_flag=u(1,buf,&StartBit);
        pic_width_in_mbs_minus1=Ue(buf,nLen,&StartBit);
        pic_height_in_map_units_minus1=Ue(buf,nLen,&StartBit);

        *Width=(pic_width_in_mbs_minus1+1)*16;
        *Height=(pic_height_in_map_units_minus1+1)*16;
		
        return true;
    }
    else
    {
        return false;
    }
}
