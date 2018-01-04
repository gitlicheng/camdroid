#ifndef ONVIF_IPC_H
#define ONVIF_IPC_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VideoEncoderToken_MainStream "MainStream"
#define VideoEncoderToken_SubStream "SubStream"
#define VideoEncoderToken_JPEG "jpeg"	//截图


//========================================================================================
//
//					IPC端Onvif初始化方法
//
//========================================================================================
//初始化ONVIF的IPC端
void * init_onvif(void * cookies);
//========================================================================================
//
//					IPC端Onvif启动方法
//
//========================================================================================
//开启Onvif服务，此函数为整个Onvif的启动点，后续需要注册有关回调函数
int start_onvif(void * ovf_env);
//停止Onvif服务
void stop_onvif(void * ovf_env);
//销毁Onvif
void destroy_onvif(void *ovf_env);
//========================================================================================
//
//					*IPC的IP地址设置*<接口已弃用>
//
//========================================================================================
//*****设置IPC的IP地址,参数：ip地址，点分十进制格式字符串
//int SetIPtoIPC(char * ip);
//========================================================================================
//
//					*获取IPC的IP地址*
//
//========================================================================================
//获取IPC的IP地址，参数：获取的ip
//				   返回值：成功返回0，失败返回-1
typedef int (*GetIPfromIPC)(void *cookies, char * ip);
//注册回调函数，参数：回调函数名
void registerGetIPfromIPC(void * ovf_env, GetIPfromIPC _getIPfromIPC);
//========================================================================================
//
//					获取设备信息的调用接口
//
//========================================================================================
//*****获取设备生产商，参数：填充设备生产商字符串
typedef int(*GetDeviceInformationResponse_Manufacturer)(void *cookies, char *Manufacturer);
//注册回调函数，参数：回调函数名
void registerGetDeviceInformationResponse_Manufacturer(void * ovf_env, GetDeviceInformationResponse_Manufacturer _getDeviceInformationResponse_Manufacturer);
//========================================================================================
//*****获取设备型号，参数：填充设备型号字符串
typedef int (*GetDeviceInformationResponse_Model)(void *cookies, char *Model);
//注册回调函数，参数：回调函数名
void registerGetDeviceInformationResponse_Model(void * ovf_env, GetDeviceInformationResponse_Model _getDeviceInformationResponse_Model);
//========================================================================================
//*****获取设备固件版本，参数：填充设备固件版本字符串
typedef int (*GetDeviceInformationResponse_FirmwareVersion)(void *cookies, char *FirmwareVersion);
//注册回调函数，参数：回调函数名
void registerGetDeviceInformationResponse_FirmwareVersion(void * ovf_env, GetDeviceInformationResponse_FirmwareVersion _getDeviceInformationResponse_FirmwareVersion);
//========================================================================================
//*****获取设备序列号，参数：填充设备序列号字符串
typedef int (*GetDeviceInformationResponse_SerialNumber)(void *cookies, char *SerialNumber);
//注册回调函数，参数：回调函数名
void registerGetDeviceInformationResponse_SerialNumber(void * ovf_env, GetDeviceInformationResponse_SerialNumber _getDeviceInformationResponse_SerialNumber);
//========================================================================================
//*****获取设备硬件ID，参数：填充设备硬件ID字符串
typedef int (*GetDeviceInformationResponse_HardwareId)(void *cookies, char *HardwareId);
//注册回调函数，参数：回调函数名
void registerGetDeviceInformationResponse_HardwareId(void * ovf_env, GetDeviceInformationResponse_HardwareId _getDeviceInformationResponse_HardwareId);
//========================================================================================
//
//					获取/设置设备日期时间的调用接口
//
//========================================================================================
//设置日期时间类型，参数：手动为0；NTP校时为1
typedef	int (*SetSystemDateAndTime_DateTimeType)(void *cookies, int DateTimeType);
//注册回调函数，参数：回调函数名
void registerSetSystemDateAndTime_DateTimeType(void * ovf_env, SetSystemDateAndTime_DateTimeType _setSystemDateAndTime_DateTimeType);
//========================================================================================
//设置夏时制，参数：无效为0；有效为1
typedef int (*SetSystemDateAndTime_DaylightSavings)(void *cookies, int DaylightSavings);
//注册回调函数，参数：回调函数名
void registerSetSystemDateAndTime_DaylightSavings(void * ovf_env, SetSystemDateAndTime_DaylightSavings _setSystemDateAndTime_DaylightSavings);
//========================================================================================
//设置时区，参数：填充时区字符串
typedef int (*SetSystemDateAndTime_TimeZone)(void *cookies, char *TimeZone);
//注册回调函数，参数：回调函数名
void registerSetSystemDateAndTime_TimeZone(void * ovf_env, SetSystemDateAndTime_TimeZone  _setSystemDateAndTime_TimeZone);
//========================================================================================
//*****设置UTC时间，参数：填充struct tm结构体
typedef int (*SetSystemDateAndTime_UTCDateTime)(void *cookies, struct tm *UTCDateTime);
//注册回调函数，参数：回调函数名
void registerSetSystemDateAndTime_UTCDateTime(void * ovf_env, SetSystemDateAndTime_UTCDateTime _setSystemDateAndTime_UTCDateTime);
//========================================================================================
//获取日期时间类型，参数：手动为0；NTP校时为1
typedef int (*GetSystemDateAndTimeResponse_DateTimeType)(void *cookies, int *DateTimeType);
//注册回调函数，参数：回调函数名
void registerGetSystemDateAndTimeResponse_DateTimeType(void * ovf_env, GetSystemDateAndTimeResponse_DateTimeType _getSystemDateAndTimeResponse_DateTimeType);
//========================================================================================
//获取夏时制，参数：无效为0；有效为1
typedef int (*GetSystemDateAndTimeResponse_DaylightSavings)(void *cookies, int *DaylightSavings);
//注册回调函数，参数：回调函数名
void registerGetSystemDateAndTimeResponse_DaylightSavings(void * ovf_env, GetSystemDateAndTimeResponse_DaylightSavings _getSystemDateAndTimeResponse_DaylightSavings);
//========================================================================================
//获取时区，参数：填充时区字符串
typedef int (*GetSystemDateAndTimeResponse_TimeZone)(void *cookies, char *TimeZone);
//注册回调函数，参数：回调函数名
void registerGetSystemDateAndTimeResponse_TimeZone(void * ovf_env, GetSystemDateAndTimeResponse_TimeZone _getSystemDateAndTimeResponse_TimeZone);
//========================================================================================
//*****获取UTC时间，参数：填充struct tm结构体
typedef int (*GetSystemDateAndTimeResponse_UTCDateTime)(void *cookies, struct tm *UTCDateTime);
//注册回调函数，参数：回调函数名
void registerGetSystemDateAndTimeResponse_UTCDateTime(void * ovf_env, GetSystemDateAndTimeResponse_UTCDateTime _getSystemDateAndTimeResponse_UTCDateTime);
//========================================================================================
//*****获取本地时间，参数：填充struct tm结构体
typedef int (*GetSystemDateAndTimeResponse_LocalDateTime)(void *cookies, struct tm *LocalDateTime);
//注册回调函数，参数：回调函数名
void registerGetSystemDateAndTimeResponse_LocalDateTime(void * ovf_env, GetSystemDateAndTimeResponse_LocalDateTime _getSystemDateAndTimeResponse_LocalDateTime);
//========================================================================================
//
//					获取设备能力的调用接口
//
//========================================================================================
//获取设备分析的能力，参数：RuleSupport（0为不支持，1为支持），AnalyticsModuleSupport（0为不支持，1为支持）  注：如果只能确定设备有分析视频的功能，而不确定具体的参数该如何设置，可以先设置为0，之后再讨论修改，后面接口的情况同样这样处理。
typedef int (*GetCapabilitiesResponse_Analytics)(void *cookies, int *RuleSupport, int *AnalyticsModuleSupport);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Analytics(void * ovf_env, GetCapabilitiesResponse_Analytics _getCapabilitiesResponse_Analytics);
//========================================================================================
//获取设备网络能力，参数：IPFilter,  ZeroConfiguration, 	 IPVersion6,  DynDNS,  Dot11Configuration(0为不支持，1为支持）
typedef int (*GetCapabilitiesResponse_Device_Network)(void *cookies, int *IPFilter,	int *ZeroConfiguration, int *IPVersion6, int *DynDNS, int *Dot11Configuration);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Device_Network(void * ovf_env, GetCapabilitiesResponse_Device_Network _getCapabilitiesResponse_Device_Network);
//========================================================================================
//获取设备系统能力，参数：RemoteDiscovery，SystemBackup，SystemLogging，FirmwareUpgrade(0为不支持，1为支持）
typedef int (*GetCapabilitiesResponse_Device_System)(void *cookies, int *RemoteDiscovery,int *SystemBackup,int *SystemLogging,int *FirmwareUpgrade);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Device_System(void * ovf_env, GetCapabilitiesResponse_Device_System _getCapabilitiesResponse_Device_System);
//========================================================================================
//获取设备IO能力，参数：InputConnectors个数，RelayOutputs个数
typedef int (*GetCapabilitiesResponse_Device_IO)(void *cookies, int *InputConnectors,int *RelayOutputs);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Device_IO(void * ovf_env, GetCapabilitiesResponse_Device_IO _getCapabilitiesResponse_Device_IO);
//========================================================================================
//获取设备安全能力，参数：各种协议支持情况，0为不支持，1为支持
typedef int (*GetCapabilitiesResponse_Device_Security)(void *cookies, int *TLS1_x002e1, int *TLS1_x002e2, int *OnboardKeyGeneration, int *AccessPolicyConfig, int *X_x002e509Token, int *SAMLToken, int *KerberosToken, int *RELToken);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Device_Security(void * ovf_env, GetCapabilitiesResponse_Device_Security _getCapabilitiesResponse_Device_Security);
//========================================================================================
//获取设备事件能力，参数：事件支持情况，0为不支持，1为支持
typedef int (*GetCapabilitiesResponse_Events)(void *cookies, int *WSSubscriptionPolicySupport,int *WSPullPointSupport, int *WSPausableSubscriptionManagerInterfaceSupport);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Events(void * ovf_env, GetCapabilitiesResponse_Events _getCapabilitiesResponse_Events);
//========================================================================================
//获取设备图像能力，参数：无      (注：图像功能一般是有的，此处只需要实现一个空函数并注册就行)
typedef int (*GetCapabilitiesResponse_Imaging)(void *cookies);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Imaging(void * ovf_env, GetCapabilitiesResponse_Imaging _getCapabilitiesResponse_Imaging);
//========================================================================================
//获取设备多媒体能力，参数：RTPMulticast（多播），RTP_USCORETCP（RTP/TCP），RTP_USCORERTSP_USCORETCP（RTP/RTSP/TCP）0为不支持，1为支持
typedef int (*GetCapabilitiesResponse_Media)(void *cookies, int *RTPMulticast,  int *RTP_USCORETCP,int *RTP_USCORERTSP_USCORETCP);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_Media(void * ovf_env, GetCapabilitiesResponse_Media _getCapabilitiesResponse_Media);
//========================================================================================
//获得设备PTZ能力，参数：无      (注：如果有此功能就注册)
typedef int (*GetCapabilitiesResponse_PTZ)(void *cookies);
//注册回调函数，参数：回调函数名
void registerGetCapabilitiesResponse_PTZ(void * ovf_env, GetCapabilitiesResponse_PTZ _getCapabilitiesResponse_PTZ);
//========================================================================================
//
//					获取/设置主机名的调用接口
//
//========================================================================================
//获取主机名是否来自DHCP，参数：不从DHCP获取主机名为0，从DHCP获取主机名为1
typedef int (*GetHostnameResponse_FromDHCP)(void *cookies, int *FromDHCP);
//注册回调函数，参数：回调函数名
void registerGetHostnameResponse_FromDHCP(void * ovf_env, GetHostnameResponse_FromDHCP _getHostnameResponse_FromDHCP);
//========================================================================================
//*****获取主机名，参数：填充主机名字符串
typedef int (*GetHostnameResponse_Name)(void *cookies, char *Name);
//注册回调函数，参数：回调函数名
void registerGetHostnameResponse_Name(void * ovf_env, GetHostnameResponse_Name _getHostnameResponse_Name);
//========================================================================================
//*****设置主机名，参数：设置的主机名字符串
typedef int (*SetHostname)(void *cookies, char *Name);
//注册回调函数，参数：回调函数名
void registerSetHostname(void * ovf_env, SetHostname _setHostname);
//========================================================================================
//
//					获取图片设置值的调用接口
//
//========================================================================================
//获取背光补偿模式，参数：0为关闭，1为打开
typedef int (*GetImagingSettingsResponse_BacklightCompensationMode)(void *cookies, int *BacklightCompensationMode);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_BacklightCompensationMode(void * ovf_env, GetImagingSettingsResponse_BacklightCompensationMode _getImagingSettingsResponse_BacklightCompensationMode);
//========================================================================================
//*****获取亮度，参数：填充亮度值
typedef int (*GetImagingSettingsResponse_Brightness)(void *cookies, float *Brightness);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_Brightness(void * ovf_env, GetImagingSettingsResponse_Brightness _getImagingSettingsResponse_Brightness);
//========================================================================================
//*****获取色彩饱和度，参数：填充色彩饱和度值
typedef int (*GetImagingSettingsResponse_ColorSaturation)(void *cookies, float *ColorSaturation);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_ColorSaturation(void * ovf_env, GetImagingSettingsResponse_ColorSaturation _getImagingSettingsResponse_ColorSaturation);
//========================================================================================
//*****获取对比度，参数：填充对比度值
typedef int (*GetImagingSettingsResponse_Contrast)(void *cookies, float *Contrast);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_Contrast(void * ovf_env, GetImagingSettingsResponse_Contrast _getImagingSettingsResponse_Contrast);
//========================================================================================
//*****获取锐度，参数：填充锐度值
typedef int (*GetImagingSettingsResponse_Sharpness)(void *cookies, float *Sharpness);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_Sharpness(void * ovf_env, GetImagingSettingsResponse_Sharpness _getImagingSettingsResponse_Sharpness);
//========================================================================================
//获取曝光模式，参数：0为自动，1为手动
typedef int (*GetImagingSettingsResponse_ExposureMode)(void *cookies, int *ExposureMode);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_ExposureMode(void * ovf_env, GetImagingSettingsResponse_ExposureMode _getImagingSettingsResponse_ExposureMode);
//========================================================================================
//获取最小曝光时间，参数：填充最小曝光时间值
typedef int (*GetImagingSettingsResponse_MinExposureTime)(void *cookies, float *MinExposureTime);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_MinExposureTime(void * ovf_env, GetImagingSettingsResponse_MinExposureTime _getImagingSettingsResponse_MinExposureTime);
//========================================================================================
//获取最大曝光时间，参数：填充最大曝光时间值
typedef int (*GetImagingSettingsResponse_MaxExposureTime)(void *cookies, float *MaxExposureTime);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_MaxExposureTime(void * ovf_env, GetImagingSettingsResponse_MaxExposureTime _getImagingSettingsResponse_MaxExposureTime);
//========================================================================================
//获取最小曝光补偿，参数：填充最小曝光补偿值
typedef int (*GetImagingSettingsResponse_MinGain)(void *cookies, float *MinGain);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_MinGain(void * ovf_env, GetImagingSettingsResponse_MinGain _getImagingSettingsResponse_MinGain);
//========================================================================================
//获取最大曝光补偿，参数：填充最大曝光补偿值
typedef int (*GetImagingSettingsResponse_MaxGain)(void *cookies, float *MaxGain);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_MaxGain(void * ovf_env, GetImagingSettingsResponse_MaxGain _getImagingSettingsResponse_MaxGain);
//========================================================================================
//获取红外线模式，参数：0为打开，1为关闭，2为自动
typedef int (*GetImagingSettingsResponse_IrCutFilterMode)(void *cookies, int *IrCutFilterMode);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_IrCutFilterMode(void * ovf_env, GetImagingSettingsResponse_IrCutFilterMode _getImagingSettingsResponse_IrCutFilterMode);
//========================================================================================
//获取宽动态模式，参数：0为关闭，1为打开
typedef int (*GetImagingSettingsResponse_WideDynamicMode)(void *cookies, int *WideDynamicMode);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_WideDynamicMode(void * ovf_env, GetImagingSettingsResponse_WideDynamicMode _getImagingSettingsResponse_WideDynamicMode);
//========================================================================================
//获取白平衡模式，参数：0为自动，1为手动
typedef int (*GetImagingSettingsResponse_WhiteBalanceMode)(void *cookies, int *WhiteBalanceMode);
//注册回调函数，参数：回调函数名
void registerGetImagingSettingsResponse_WhiteBalanceMode(void * ovf_env, GetImagingSettingsResponse_WhiteBalanceMode _getImagingSettingsResponse_WhiteBalanceMode);
//========================================================================================
//
//					设置图片设置值的调用接口
//
//========================================================================================
//设置背光补偿模式，参数：0为关闭，1为打开
typedef int (*SetImagingSettings_BacklightCompensationMode)(void *cookies, int BacklightCompensationMode);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_BacklightCompensationMode(void * ovf_env, SetImagingSettings_BacklightCompensationMode _setImagingSettings_BacklightCompensationMode);
//========================================================================================
//*****设置亮度，参数：亮度值
typedef int (*SetImagingSettings_Brightness)(void *cookies, float Brightness);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_Brightness(void * ovf_env, SetImagingSettings_Brightness _setImagingSettings_Brightness);
//========================================================================================
//*****设置色彩饱和度，参数：色彩饱和度值
typedef int (*SetImagingSettings_ColorSaturation)(void *cookies, float ColorSaturation);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_ColorSaturation(void * ovf_env, SetImagingSettings_ColorSaturation _setImagingSettings_ColorSaturation);
//========================================================================================
//*****设置对比度，参数：对比度值
typedef int (*SetImagingSettings_Contrast)(void *cookies, float Contrast);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_Contrast(void * ovf_env, SetImagingSettings_Contrast _setImagingSettings_Contrast);
//========================================================================================
//*****设置锐度，参数：锐度值
typedef int (*SetImagingSettings_Sharpness)(void *cookies, float Sharpness);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_Sharpness(void * ovf_env, SetImagingSettings_Sharpness _setImagingSettings_Sharpness);
//========================================================================================
//设置曝光模式，参数：0为自动，1为手动
typedef int (*SetImagingSettings_ExposureMode)(void *cookies, int ExposureMode);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_ExposureMode(void * ovf_env, SetImagingSettings_ExposureMode _setImagingSettings_ExposureMode);
//========================================================================================
//设置最小曝光时间，参数：最小曝光时间值
typedef	int (*SetImagingSettings_MinExposureTime)(void *cookies, float MinExposureTime);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_MinExposureTime(void * ovf_env, SetImagingSettings_MinExposureTime _setImagingSettings_MinExposureTime);
//========================================================================================
//设置最大曝光时间，参数：最大曝光时间值
typedef	int (*SetImagingSettings_MaxExposureTime)(void *cookies, float MaxExposureTime);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_MaxExposureTime(void * ovf_env, SetImagingSettings_MaxExposureTime _setImagingSettings_MaxExposureTime);
//========================================================================================
//设置最小曝光补偿，参数：最小曝光补偿值
typedef int (*SetImagingSettings_MinGain)(void *cookies, float MinGain);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_MinGain(void * ovf_env, SetImagingSettings_MinGain _setImagingSettings_MinGain);
//========================================================================================
//设置最大曝光补偿，参数：最大曝光补偿值
typedef int (*SetImagingSettings_MaxGain)(void *cookies, float MaxGain);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_MaxGain(void * ovf_env, SetImagingSettings_MaxGain _setImagingSettings_MaxGain);
//========================================================================================
//设置红外线模式，参数：0为打开，1为关闭，2为自动
typedef int (*SetImagingSettings_IrCutFilterMode)(void *cookies, int IrCutFilterMode);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_IrCutFilterMode(void * ovf_env, SetImagingSettings_IrCutFilterMode _setImagingSettings_IrCutFilterMode);
//========================================================================================
//设置宽动态模式，参数：0为关闭，1为打开
typedef int (*SetImagingSettings_WideDynamicMode)(void *cookies, int WideDynamicMode);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_WideDynamicMode(void * ovf_env, SetImagingSettings_WideDynamicMode _setImagingSettings_WideDynamicMode);
//========================================================================================
//设置白平衡模式，参数：0为自动，1为手动
typedef int (*SetImagingSettings_WhiteBalanceMode)(void *cookies, int WhiteBalanceMode);
//注册回调函数，参数：回调函数名
void registerSetImagingSettings_WhiteBalanceMode(void * ovf_env, SetImagingSettings_WhiteBalanceMode _setImagingSettings_WhiteBalanceMode);
//========================================================================================
//
//					获取图片选项的调用接口
//
//========================================================================================
//获取背光补偿模式选项，参数：size表示模式的选项个数，Mode为模式值的和，模式有关闭和打开两种，关闭模式值为0，打开模式值为1。注：size和Mode值应该匹配，例如，size为1，Mode为1，表示背光补偿模式只有打开模式选项，size为1，Mode为0，表示背光补偿模式只有关闭模式选项，size为2，Mode为1（0+1的和），表示背光补偿模式有打开和关闭两种模式选项。
typedef int (*GetOptionsResponse_BacklightCompensation_Mode)(void *cookies, int *size, int *Mode);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_BacklightCompensation_Mode(void * ovf_env, GetOptionsResponse_BacklightCompensation_Mode _getOptionsResponse_BacklightCompensation_Mode);
//========================================================================================
//获取背光补偿等级选项，参数：等级的最小值和最大值
typedef int (*GetOptionsResponse_BacklightCompensation_Level)(void *cookies, float *Min, float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_BacklightCompensation_Level(void * ovf_env, GetOptionsResponse_BacklightCompensation_Level _getOptionsResponse_BacklightCompensation_Level);
//========================================================================================
//*****获取亮度值的选项，参数：亮度值的最小值和最大值
typedef int (*GetOptionsResponse_Brightness)(void *cookies, float *Min,float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_Brightness(void * ovf_env, GetOptionsResponse_Brightness _getOptionsResponse_Brightness);
//========================================================================================
//*****获取色彩饱和度值的选项，参数：色彩饱和度值的最小值和最大值
typedef int (*GetOptionsResponse_ColorSaturation)(void *cookies, float *Min,float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_ColorSaturation(void * ovf_env, GetOptionsResponse_ColorSaturation _getOptionsResponse_ColorSaturation);
//========================================================================================
//*****获取对比度值的选项，参数：对比度值的最小值和最大值
typedef	int (*GetOptionsResponse_Contrast)(void *cookies, float *Min,float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_Contrast(void * ovf_env, GetOptionsResponse_Contrast _getOptionsResponse_Contrast);
//========================================================================================s
//获取曝光模式的选项，参数：size为模式种类个数，Mode为模式值的和，自动模式值为0，手动模式值为1
typedef int (*GetOptionsResponse_Exposure_Mode)(void *cookies, int *size,int *Mode);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_Exposure_Mode(void * ovf_env, GetOptionsResponse_Exposure_Mode _getOptionsResponse_Exposure_Mode);
//========================================================================================
//获取曝光优先级的选项，参数：size为优先级种类个数，Priority为优先级值的和，低噪优先级值为0，帧率优先级值为1
typedef int (*GetOptionsResponse_Exposure_Priority)(void *cookies, int *size,int *Priority);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_Exposure_Priority(void * ovf_env, GetOptionsResponse_Exposure_Priority _getOptionsResponse_Exposure_Priority);
//========================================================================================
//获取曝光时间的选项，参数：最小曝光时间的最小值和最大值，最大曝光时间的最小值和最大值
typedef int (*GetOptionsResponse_Exposure_ExposureTime)(void *cookies, int *MinExposureTime_Min,int *MinExposureTime_Max, int *MaxExposureTime_Min,int *MaxExposureTime_Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_Exposure_ExposureTime(void * ovf_env, GetOptionsResponse_Exposure_ExposureTime _getOptionsResponse_Exposure_ExposureTime);
//========================================================================================
//获取曝光补偿的选项，参数：最小曝光的最小值和最大值，最大曝光的最小值和最大值
typedef int (*GetOptionsResponse_Exposure_Gain)(void *cookies, int *MinGain_Min,int *MinGain_Max,int *MaxGain_Min,int *MaxGain_Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_Exposure_Gain(void * ovf_env, GetOptionsResponse_Exposure_Gain _getOptionsResponse_Exposure_Gain);
//========================================================================================
//获取红外线模式的选项，参数：size为红外线模式的种类个数，IrCutFilterModes为红外线模式值的和，打开模式值为0，关闭模式值为1，自动模式值为2。注：例如，打开模式+关闭模式：size = 2，IrCutFilterModes = 0+1 = 1
typedef int (*GetOptionsResponse_IrCutFilterModes)(void *cookies, int *size,int *IrCutFilterModes);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_IrCutFilterModes(void * ovf_env, GetOptionsResponse_IrCutFilterModes _getOptionsResponse_IrCutFilterModes);
//========================================================================================
//*****获取锐度值的选项，参数：锐度值的最小值和最大值
typedef int (*GetOptionsResponse_Sharpness)(void *cookies, float *Min,float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_Sharpness(void * ovf_env, GetOptionsResponse_Sharpness  _getOptionsResponse_Sharpness);
//========================================================================================
//获取宽动态范围模式的选项，参数：size为模式的种类个数，Mode为模式值的和，关闭模式值为0，打开模式值为1
typedef	int (*GetOptionsResponse_WideDynamicRange_Mode)(void *cookies, int *size,int *Mode);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_WideDynamicRange_Mode(void * ovf_env, GetOptionsResponse_WideDynamicRange_Mode _getOptionsResponse_WideDynamicRange_Mode);
//========================================================================================
//获取宽动态范围等级的选项，参数：等级的最小值和最大值
typedef int (*GetOptionsResponse_WideDynamicRange_Level)(void *cookies, float *Min,float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_WideDynamicRange_Level(void * ovf_env, GetOptionsResponse_WideDynamicRange_Level _getOptionsResponse_WideDynamicRange_Level);
//========================================================================================
//获取白平衡模式的选项，参数：size为模式的种类个数，Mode为模式值的和，自动模式值为0，手动模式值为1
typedef int (*GetOptionsResponse_WhiteBalance_Mode)(void *cookies, int *size,int *Mode);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_WhiteBalance_Mode(void * ovf_env, GetOptionsResponse_WhiteBalance_Mode _getOptionsResponse_WhiteBalance_Mode);
//========================================================================================
//获取白平衡Yr补偿的选项，参数：Yr补偿最小值和最大值
typedef int (*GetOptionsResponse_WhiteBalance_YrGain)(void *cookies, float *Min,float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_WhiteBalance_YrGain(void * ovf_env, GetOptionsResponse_WhiteBalance_YrGain _getOptionsResponse_WhiteBalance_YrGain);
//========================================================================================
//获取白平衡Yb补偿的选项，参数：Yb补偿最小值和最大值
typedef int (*GetOptionsResponse_WhiteBalance_YbGain)(void *cookies, float *Min,float *Max);
//注册回调函数，参数：回调函数名
void registerGetOptionsResponse_WhiteBalance_YbGain(void * ovf_env, GetOptionsResponse_WhiteBalance_YbGain _getOptionsResponse_WhiteBalance_YbGain);
//========================================================================================
//
//					获取视频流的Uri
//
//========================================================================================
//*****获取视频流Uri，参数：StreamToken,用字符串标识视频流，“MainStream”标识主码流，“SubStream”标识子码流; Uri用于保存视频uri
typedef	int (*GetStreamUriResponse_Uri)(void *cookies, char *StreamToken, char * Uri);
//注册回调函数，参数：回调函数名
void registerGetStreamUriResponse_Uri(void * ovf_env, GetStreamUriResponse_Uri  _getStreamUriResponse_Uri);
//========================================================================================
//
//					获取视频截图的Uri	
//
//========================================================================================
//*****获取视频截图Uri，参数：StreamToken,用字符串标识视频流，“MainStream”标识主码流，“SubStream”标识子码流;Uri用于保存视频截图uri
typedef int (*GetSnapshotUriResponse_Uri)(void *cookies, char *StreamToken, char * Uri);
//注册回调函数，参数：回调函数名
void registerGetSnapshotUriResponse_Uri(void * ovf_env, GetSnapshotUriResponse_Uri _getSnapshotUriResponse_Uri);
//========================================================================================
//
//					获取编码器配置的调用接口
//
//========================================================================================
//获取编码器的编码类型，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；Encoding：0为JPEG，1为MPEG4，2为H264
typedef int (*GetVideoEncoderConfigurationResponse_Encoding)(void *cookies, const char *token, int *Encoding);
//注册回调函数，参数：回调函数名
void registerGetVideoEncoderConfigurationResponse_Encoding(void * ovf_env, GetVideoEncoderConfigurationResponse_Encoding _getVideoEncoderConfigurationResponse_Encoding);
//========================================================================================
//*****获取编码器的编码分辨率，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；分辨率的宽和高
typedef int (*GetVideoEncoderConfigurationResponse_Resolution)(void *cookies, const char *token, int *Width, int *Height);
//注册回调函数，参数：回调函数名
void registerGetVideoEncoderConfigurationResponse_Resolution(void * ovf_env, GetVideoEncoderConfigurationResponse_Resolution _getVideoEncoderConfigurationResponse_Resolution);
//========================================================================================
//获取编码器的编码质量，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；编码质量
typedef int (*GetVideoEncoderConfigurationResponse_Quality)(void *cookies, const char *token, float *Quality);
//注册回调函数，参数：回调函数名
void registerGetVideoEncoderConfigurationResponse_Quality(void * ovf_env, GetVideoEncoderConfigurationResponse_Quality _getVideoEncoderConfigurationResponse_Quality);
//========================================================================================
//*****获取编码器的码率控制，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；帧率、编码间隔、比特率
typedef int (*GetVideoEncoderConfigurationResponse_RateControl)(void *cookies, const char *token, int *FrameRateLimit, int *EncodingInterval, int *BitrateLimit);
//注册回调函数，参数：回调函数名
void registerGetVideoEncoderConfigurationResponse_RateControl(void * ovf_env, GetVideoEncoderConfigurationResponse_RateControl _getVideoEncoderConfigurationResponse_RateControl);
//========================================================================================
//获取编码器的H264配置，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；Govlength（H264编码相关，目前不清楚含义），
//							  H264Profile:0为Baseline，1为Main，2为Extended，3为High
typedef int (*GetVideoEncoderConfigurationResponse_H264)(void *cookies, const char *token, int *GovLength, int *H264Profile);
//注册回调函数，参数：回调函数名
void registerGetVideoEncoderConfigurationResponse_H264(void * ovf_env, GetVideoEncoderConfigurationResponse_H264 _getVideoEncoderConfigurationResponse_H264);
//========================================================================================
//获取编码器的多播属性，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；
//                            Type指定多播的地址类型，0为IPv4，1为IPv6，IPv4Address和IPv6Address依据Type值只用到一个；Port为端口号、TTL为生存时间，
//                            AutoStart为自动开始设置,0为关闭，1为打开
typedef int (*GetVideoEncoderConfigurationResponse_Multicast)(void *cookies, const char *token, int *Type, char *IPv4Address, char *IPv6Address, int *Port, int *TTL, int *AutoStart);
//注册回调函数，参数：回调函数名
void registerGetVideoEncoderConfigurationResponse_Multicast(void * ovf_env, GetVideoEncoderConfigurationResponse_Multicast _getVideoEncoderConfigurationResponse_Multicast);
//========================================================================================
//获取编码器的会话超时，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；超时时间值
typedef int (*GetVideoEncoderConfigurationResponse_SessionTimeout)(void *cookies, const char *token, long long *SessionTimeout);
//注册回调函数，参数：回调函数名
void registerGetVideoEncoderConfigurationResponse_SessionTimeout(void * ovf_env, GetVideoEncoderConfigurationResponse_SessionTimeout _getVideoEncoderConfigurationResponse_SessionTimeout);
//========================================================================================
//
//					设置编码器配置的调用接口
//
//========================================================================================
//设置编码器的编码类型，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；Encoding：0为JPEG，1为MPEG4，2为H264
typedef int (*SetVideoEncoderConfiguration_Encoding)(void *cookies, const char *token, int Encoding);
//注册回调函数，参数：回调函数名
void registerSetVideoEncoderConfiguration_Encoding(void * ovf_env, SetVideoEncoderConfiguration_Encoding _setVideoEncoderConfiguration_Encoding);
//========================================================================================
//*****设置编码器的编码分辨率，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；分辨率的宽和高
typedef int (*SetVideoEncoderConfiguration_Resolution)(void *cookies, const char *token, int Width, int Height);
//注册回调函数，参数：回调函数名
void registerSetVideoEncoderConfiguration_Resolution(void * ovf_env, SetVideoEncoderConfiguration_Resolution _setVideoEncoderConfiguration_Resolution);
//========================================================================================
//设置编码器的编码质量，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；编码质量
typedef int (*SetVideoEncoderConfiguration_Quality)(void *cookies, const char *token, float Quality);
//注册回调函数，参数：回调函数名
void registerSetVideoEncoderConfiguration_Quality(void * ovf_env, SetVideoEncoderConfiguration_Quality _setVideoEncoderConfiguration_Quality);
//========================================================================================
//*****设置编码器的码率控制，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；帧率、编码间隔、比特率
typedef int (*SetVideoEncoderConfiguration_RateControl)(void *cookies, const char *token, int FrameRateLimit, int EncodingInterval, int BitrateLimit);
//注册回调函数，参数：回调函数名
void registerSetVideoEncoderConfiguration_RateControl(void * ovf_env, SetVideoEncoderConfiguration_RateControl _setVideoEncoderConfiguration_RateControl);
//========================================================================================
//设置编码器的H264配置，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；Govlength（H264编码相关，目前不清楚含义），
//							  H264Profile:0为Baseline，1为Main，2为Extended，3为High
typedef int (*SetVideoEncoderConfiguration_H264)(void *cookies, const char *token, int GovLength, int H264Profile);
//注册回调函数，参数：回调函数名
void registerSetVideoEncoderConfiguration_H264(void * ovf_env, SetVideoEncoderConfiguration_H264 _setVideoEncoderConfiguration_H264);
//========================================================================================
//设置编码器的多播属性，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；
//                            Type指定多播的地址类型，0为IPv4，1为IPv6，IPv4Address和IPv6Address依据Type值只用到一个；Port为端口号、TTL为生存时间，
//                            AutoStart为自动开始设置,0为关闭，1为打开
typedef int (*SetVideoEncoderConfiguration_Multicast)(void *cookies, const char *token, int Type, char *IPv4Address, char *IPv6Address, int Port, int TTL, int AutoStart);
//注册回调函数，参数：回调函数名
void registerSetVideoEncoderConfiguration_Multicast(void * ovf_env, SetVideoEncoderConfiguration_Multicast _setVideoEncoderConfiguration_Multicast);
//========================================================================================
//设置编码器的会话超时，参数：token指定要设置的编码器，“MainStream”为主码编码器，“SubStream”为子码编码器；超时时间值
typedef int (*SetVideoEncoderConfiguration_SessionTimeout)(void *cookies, const char *token, long long SessionTimeout);
//注册回调函数，参数：回调函数名
void registerSetVideoEncoderConfiguration_SessionTimeout(void * ovf_env, SetVideoEncoderConfiguration_SessionTimeout _setVideoEncoderConfiguration_SessionTimeout);
//========================================================================================
//
//					获取OSD信息的调用接口
//
//========================================================================================
//*****获取OSD的显示位置，参数：type标记OSD的种类("DateTime"标识时间，"TextString"标识文本字符串)，OSD显示坐标x和y
typedef int (*GetOSD_Position)(void *cookies, char *type, float *x, float *y);
//注册回调函数，参数：回调函数名
void registerGetOSD_Position(void * ovf_env, GetOSD_Position _getOSD_Position);
//*****获取OSD的存在状态，参数：type标记OSD的种类("DateTime"标识时间，"TextString"标识文本字符串),OSD状态为不存在flag为0，OSD状态为存在flag为1
//					      返回值:失败返回值为-1，成功返回值为0
typedef int (*GetOSD_Status)(void *cookies, char *type, int *flag);
//注册回调函数，参数：回调函数名
void registerGetOSD_Status(void * ovf_env, GetOSD_Status _getOSD_Status);
//*****获取OSD的文本字符串，参数：OSD的文本字符串
typedef int (*GetOSD_Text)(void *cookies, char * text);
//注册回调函数，参数：回调函数名
void registerGetOSD_Text(void * ovf_env, GetOSD_Text _getOSD_Text);
//========================================================================================
//
//					设置OSD信息的调用接口
//
//========================================================================================
//*****传给IPC设备OSD图片的URL，参数：type标记OSD的种类("TextString"标识文本字符串的URL，"ShowWords"标识OSD中显示的文字)，Url图片URL或者是OSD中的文字
typedef int (*SetOSD_URL)(void *cookies, char *type, char *Url);
//注册回调函数，参数：回调函数名
void registerSetOSD_URL(void * ovf_env, SetOSD_URL _setOSD_URL);
//*****设置OSD的显示位置，参数：type标记OSD的种类("DateTime"标识时间，"TextString"标识文本字符串)，OSD显示坐标x和y
typedef int (*SetOSD_Position)(void *cookies, char *type, float x, float y);
//注册回调函数，参数：回调函数名
void registerSetOSD_Position(void * ovf_env, SetOSD_Position _setOSD_Position);
//========================================================================================
//
//					创建OSD的调用接口
//
//========================================================================================
//*****创建对应的OSD，参数：type标记OSD的种类("DateTime"标识时间，"TextString"标识文本字符串)
typedef int (*CreateTheOSD)(void *cookies, char *type);
//注册回调函数，参数：回调函数名
void registerCreateTheOSD(void * ovf_env, CreateTheOSD _createTheOSD);
//========================================================================================
//
//					删除OSD的调用接口
//
//========================================================================================
//*****删除对应的OSD，参数：type标记OSD的种类("DateTime"标识时间，"TextString"标识文本字符串)
typedef int (*DeleteTheOSD)(void *cookies, char *type);
//注册回调函数，参数：回调函数名
void registerDeleteTheOSD(void * ovf_env, DeleteTheOSD _deleteTheOSD);
//========================================================================================
//
//					移动侦测的调用接口
//
//========================================================================================
//*****打开移动侦测功能，参数：无
typedef int (*OpenMotionDetector)(void *cookies);
//注册回调函数，参数：回调函数名
void registerOpenMotionDetector(void * ovf_env, OpenMotionDetector _openMotionDetector);
//*****关闭移动侦测功能，参数：无
typedef int (*CloseMotionDetector)(void *cookies);
//注册回调函数，参数：回调函数名
void registerCloseMotionDetector(void * ovf_env, CloseMotionDetector _closeMotionDetector);
//*****侦测到移动事件，参数：flag标识报警信息的存在状态，0表示没有报警信息，1表示有报警信息
//					   返回值：失败返回值为-1,成功返回值为0
typedef int (*MotionAlarm)(void *cookies, int * flag);
//注册回调函数，参数：回调函数名
void registerMotionAlarm(void * ovf_env, MotionAlarm _motionAlarm);
//*****侦测功能的开启状态，参数：type标识侦测类型，flag标识状态，关闭时为0，开启时为1
//					       返回值：失败返回值为-1，成功返回值为0
typedef int (*DetectorOpenOrNot)(void *cookies, char * type, int *flag);
//注册回调函数，参数：回调函数名
void registerDetectorOpenOrNot(void * ovf_env, DetectorOpenOrNot _detectorOpenOrNot);
//*****设定侦测区域，参数：size为buff的格式长度，buff为区域设定内容
typedef int (*SetDetectionArea)(void *cookies, int size, unsigned char * buff);
//注册回调函数，参数：回调函数名
void registerSetDetectionArea(void * ovf_env, SetDetectionArea _setDetectionArea);
//========================================================================================
//
//					NTP客户端的调用接口
//
//========================================================================================
//*****打开NTP客户端，参数：无
typedef int (*OpenNTP)(void *cookies);
//注册回调函数，参数：回调函数名
void registerOpenNTP(void * ovf_env, OpenNTP _openNTP);
//*****关闭NTP客户端，参数：无
typedef int (*CloseNTP)(void *cookies);
//注册回调函数，参数：回调函数名
void registerCloseNTP(void * ovf_env, CloseNTP _closeNTP);
//*****NTP功能的开启状态，参数：flag标识NTP开启状态，0表示关闭状态，1表示开启状态
//                        返回值：失败返回值为-1，成功返回值为0
typedef int (*TheNTPOpenOrNot)(void *cookies, int *flag);
//注册回调函数，参数：回调函数名
void registerTheNTPOpenOrNot(void * ovf_env, TheNTPOpenOrNot _theNTPOpenOrNot);



#ifdef __cplusplus
}
#endif

#endif /* onvif_ipc.h */