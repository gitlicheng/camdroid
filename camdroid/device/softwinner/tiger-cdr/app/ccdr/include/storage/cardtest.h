#ifndef __FILETEST_H_INCLUDE__
#define __FILETEST_H_INCLUDE__

struct TestInfo{
	int mCycles;
	int mPackageSize;
	long long mFileSize;
};

struct TestResult{
	double mWriteSpeed;
	double mReadSpeed;
};

//测试卡速度
//dirpath:		卡的挂载点路径
//test_result:	用于存放测试结果
//test_info:	指定测试方式，若为NULL，则按默认方式测试
//status:		保存测试状态，定位错误，目前未使用
//返回值：		true表示测试成功，false表示测试过程中存在错误
extern "C" bool Get_Card_Speed(char *dirpath, struct TestResult *test_result, struct TestInfo* test_info, int *status);

//对卡进行一次全盘检测
//dirpath:		卡的挂载点路径
//status:		保存测试状态，定位错误，目前未使用
//返回值：		true表示测试成功，false表示测试过程中存在错误
extern "C" bool Check_Card_Ok(char *dirpath, int *status);

#endif