#ifndef __IPCMSG__H__
#define __IPCMSG__H__

#ifndef bool
#define bool char
#endif 

#ifndef true
#define true 1
#endif 

#ifndef false
#define false 0
#endif 
typedef struct
{
	int     check;			//校验值，必须为0xabcd123
	char    findData[8];	//识别字符串，为IPCamera
}DevFind;

typedef struct
{
	int 	check;			//0xabcd123
	char 	devName[16];	//设备名称
	short	port;			//服务端口号
}DevFindAck;


typedef struct
{
	int		check;			//校验值，为0xabcd123
	int		iLength;		//整个消息包长度
	char    Content[];		//消息内容
}DPMsg;







#endif //#ifdef __IPCMSG__H__
