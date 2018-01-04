#ifndef __NATCLIENT_INTERFACE__
#define __NATCLIENT_INTERFACE__

#ifdef __cplusplus
extern "C"{
#endif	

typedef struct 
{
	char devName[24];
	void * session;
}NATMainThreadArgV;

//获取用户名的函数指针；
//返回值：0，成功； 非0 失败；
//参数：usrname,获取成功用户名会写入该字符数组；
//      usertype, 获取的账户的类型信息；  0，管理员（admin）   1, 普通用户（user）     
//      cookie， 应用程序句柄；
typedef int (* GetUserNameFunc)(char * username, int usertype, void * cookie);

//获取指定用户名的账户密码的函数指针；
//返回值：0，成功； 非0 失败；
//参数：username,  要获取密码的账户的用户名；
//      password, 指定的username的密码信息； 
//      cookie， 应用程序句柄；
typedef int (* GetPassWordFunc)(int usertype, char* password, void *);

//设置指定用户名的账户密码的函数指针；
//返回值：0，成功； 非0 失败；
//参数：username,  要设置密码的账户的用户名；
//      password, 设置username的密码信息； 
//      cookie， 应用程序句柄；
typedef int (* SetPassWordFunc)(int usertype, char* password, void *);


void natMain(void * in);

void RegisterGetUserNameFunc(GetUserNameFunc, void *);
void RegisterGetPassWordFunc(GetPassWordFunc, void *);
void RegisterSetPassWordFunc(SetPassWordFunc, void *);

#ifdef __cplusplus
}
#endif	

#endif	//__NATCLIENT_INTERFACE__