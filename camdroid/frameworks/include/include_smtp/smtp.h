#ifndef WL_SMTP_H
#define WL_SMTP_H 1

#ifdef __cplusplus
extern "C" {
#endif


#define MAIL_PORT 25
#define NAME_FROM_SIZE 128
#define NAME_to_SIZE 128
#define ADD_FROM_SIZE 128
#define ADD_TO_SIZE 128
#define HOST_SIZE 128
#define SUBJECT_SIZE 128
#define FILEPATH_SIZE 128
#define CONTENT_SIZE 65536
#define ATTACH_NUM 3

typedef struct
{
	char name_from[NAME_FROM_SIZE];	//来自名字
	char name_to[NAME_to_SIZE];		//发往名字
	char add_from[ADD_FROM_SIZE];	//来自的邮箱
	char add_to[ADD_TO_SIZE];		//发往的邮箱
	char subject[SUBJECT_SIZE];		//主题
	char *filebuf[ATTACH_NUM];		//附件buf							
	char *filename[ATTACH_NUM];		//附件名，带后缀，如File.jpeg
	unsigned int filesz[ATTACH_NUM];//附件buf长度
	char content[CONTENT_SIZE];		//信件正文内容
	unsigned short port;			//邮件服务器端口号															


}struct_send_mail;

int send_mail(struct_send_mail *msg);



#ifdef __cplusplus
}
#endif

#endif /* wl_smtp.h */