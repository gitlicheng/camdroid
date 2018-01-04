#ifndef __NAT__INTERFACE__
#define __NAT__INTERFACE__

#ifdef __cplusplus
extern "C"{
#endif	

#include <common/MediaListItem.h>
#include <common/WifiApListItem.h>

typedef struct 
{
	char devName[24];
	void * session;
	void * audio_session;
	void * recorder_session;
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
	
/* device info */
typedef int (*GetDeviceInfoFunc)(void * cookie, int what, char *info);
/* device info */

/* time */
typedef int (*SetDatetimeSyncModeFunc)(void * cookie, const unsigned int mode);
typedef int (*GetDatetimeSyncModeFunc)(void * cookie, unsigned int *mode);
typedef int (*SetTimeZoneFunc)(void * cookie, const char *tz);
typedef int (*GetTimeZoneFunc)(void * cookie, char *tz);
typedef int (*SetDateTimeFunc)(void * cookie, const struct tm *datetime);
typedef int (*GetDateTimeFunc)(void * cookie, struct tm *datetime);
/* time */

/* wifi setting */
typedef int (*SetWifiEnableFunc)(void * cookie, unsigned int enable);
typedef int (*GetWifiEnableFunc)(void * cookie, unsigned int *enable);
typedef int (*GetWifiApListFunc)(void * cookie, struct WifiApListItem **plist);
typedef int (*ConnectWifiApFunc)(void * cookie, const char *ssid, const char *pw, const char *encryption);
typedef int (*GetConnectedWifiApFunc)(void * cookie, char *ssid);
typedef int (*ForgetWifiApFunc)(void * cookie, const char *ssid);
/* wifi setting */

/* video stream */
typedef int (*GetVideoUrlFunc)(void * cookie, char *url);
typedef int (*SetVideoSizeFunc)(void * cookie, const unsigned int width, const unsigned int height);
typedef int (*GetVideoSizeFunc)(void * cookie, unsigned int *width, unsigned int *height);
typedef int (*SetVideoFramerateFunc)(void * cookie, const unsigned int framerate);
typedef int (*GetVideoFramerateFunc)(void * cookie, unsigned int *framerate);
typedef int (*SetVideoBitrateFunc)(void * cookie, const unsigned int bitrate);
typedef int (*GetVideoBitrateFunc)(void * cookie, unsigned int *bitrate);
/* video stream */

/* camera setting */
typedef int (*GetBrightnessMinMaxFunc)(void * cookie, int *max, int *min);
typedef int (*SetBrightnessFunc)(void * cookie, const int value);
typedef int (*GetBrightnessFunc)(void * cookie, int *value);
typedef int (*GetContrastMinMaxFunc)(void * cookie, int *max, int *min);
typedef int (*SetContrastFunc)(void * cookie, const int value);
typedef int (*GetContrastFunc)(void * cookie, int *value);
typedef int (*GetSaturationMinMaxFunc)(void * cookie, int *max, int *min);
typedef int (*SetSaturationFunc)(void * cookie, const int value);
typedef int (*GetSaturationFunc)(void * cookie, int *value);
typedef int (*GetSharpnessMinMaxFunc)(void * cookie, int *max, int *min);
typedef int (*SetSharpnessFunc)(void * cookie, const int value);
typedef int (*GetSharpnessFunc)(void * cookie, int *value);	
/* camera setting */

/* motion detect */
typedef int (*SetMDEnableFunc)(void * cookie, const unsigned int enable);
typedef int (*GetMDEnableFunc)(void * cookie, unsigned int *enable);
typedef int (*SetMDSensitivityFunc)(void * cookie, const int level);
typedef int (*GetMDSensitivityFunc)(void * cookie, int *level);
typedef int (*SetMDRegionMaskFunc)(void * cookie ,const unsigned char *regionMask, const unsigned int size);
typedef int (*GetMDRegionMaskFunc)(void * cookie, unsigned char *regionMask, unsigned int size);
typedef int (*SetMDCoverDetectFunc)(void * cookie, const unsigned int fullCover, const unsigned int regionCover);
typedef int (*GetMDCoverDetectFunc)(void * cookie, unsigned int *fullCover, unsigned int *regionCover);
typedef int (*GetMAlarmFunc)(void * cookie, int *alarm);
typedef int (*SetMAlarmEmailFunc)(void * cookie, const char *email);
typedef int (*GetMAlarmEmailFunc)(void * cookie, char *email);
typedef int (*SetMotionCaptureEnableFunc)(void * cookie, const unsigned int enable);
typedef int (*GetMotionCaptureEnableFunc)(void * cookie, unsigned int *enable);
typedef int (*GetMAlarmCapturesFunc)(void * cookie, long long pts, void *list);
/* motion detect */

/* osd setting */
typedef int (*SetOsdDatetimeEnableFunc)(void * cookie, const unsigned int enable);
typedef int (*GetOsdDatetimeEnableFunc)(void * cookie, unsigned int *enable);
typedef int (*SetOsdChannelEnableFunc)(void * cookie, const unsigned int enable);
typedef int (*GetOsdChannelEnableFunc)(void * cookie, unsigned int *enable);
typedef int (*SetOsdDatetimePositionFunc)(void * cookie, const unsigned int x, const unsigned int y);
typedef int (*GetOsdDatetimePositionFunc)(void * cookie, unsigned int *x, unsigned int *y);
typedef int (*SetOsdChannelPositionFunc)(void * cookie, const unsigned int x, const unsigned int y);
typedef int (*GetOsdChannelPositionFunc)(void * cookie, unsigned int *x, unsigned int *y);
typedef int (*SetOsdChannelNameFunc)(void * cookie, const char *name);
typedef int (*GetOsdChannelNameFunc)(void * cookie, char *name);
typedef int (*SetOsdChannelUrlFunc)(void * cookie, const char *url);
/* osd setting */

/* motor control*/
typedef int (*MotorRollFunc)(void * cookie,const int which,const int direction, const int angle);
/* motor control*/

/* voice intercom */
typedef int (*GetVoiceUrlFunc)(void * cookie, char *url);
typedef int (*StartVoiceCallFunc)(void * cookie, const char *url, int rtsp_port, int rtp_ip, int rtp_port);
typedef int (*StopVoiceCallFunc)(void * cookie);
typedef int (*SetVoiceVolumeFunc)(void * cookie, const int vol);
typedef int (*GetVoiceVolumeFunc)(void * cookie, int *vol);
/* voice intercom */

/* media */
typedef int (*GetMediaListFunc)(void * cookie, const char *mediaType, struct MediaListItem **list);
typedef int (*GetMediaUrlFunc)(void * cookie, const char *name, char *url);
typedef int (*GetDownloadUrlFunc)(void * cookie, const char *name, char *url);
typedef int (*StartMediaPlayFunc)(void * cookie, const char *name);
typedef int (*StopMediaPlayFunc)(void * cookie);
typedef int (*IsMediaPlayingFunc)(void * cookie, unsigned int *isplaying);
typedef int (*SetMediaPlayModeFunc)(void * cookie, const unsigned int mode);
typedef int (*GetMediaPlayModeFunc)(void * cookie, unsigned int mode);
//	int removeMedias(const char *mediaType, list<string> *list);
typedef int (*FormatDiskFunc)(void * cookie, const int flag);
/* media */

/* recorder */
typedef int (*SetRecordEnableFunc)(void * cookie, const unsigned int enable);
typedef int (*GetRecordEnableFunc)(void * cookie, unsigned int *enable);
typedef int (*SetVoiceRecordEnableFunc)(void * cookie, const unsigned int enable);
typedef int (*GetVoiceRecordEnableFunc)(void * cookie, unsigned int *enable);
typedef int (*SetRecordPlanFunc)(void * cookie, const int from, const int to, const int mode);
typedef int (*GetRecordPlanFunc)(void * cookie, int *from, int *to, int *mode);
/* recorder */

typedef int (*GetIpFunc)(void * cookie, char *ipstr);
typedef int (*DeleteFile)(void * cookie, char *name);
typedef int (*GetCardStatus)(void * cookie);
typedef long long (*GetStorageTatolSize)(void *cookie);
typedef long long (*GetStorageFreeSize)(void *cookie);
typedef int (*GetMuteMode)(void *cookie, int *mute);
typedef int (*SetMuteMode)(void *cookie,int mute);
typedef int (*GetMediaPlayerName)(void *cookie,char *name);
typedef int (*ParserSeekTo)(void *cookie,long long timeUs);
typedef int (*GetDuration)(void *cookie);
typedef int (*StartAutoTest)(void *cookie);
typedef int (*StopAutoTest)(void *cookie);
typedef int (*SetIFramesNumInterval)(void *cookie,int nMaxKeyItl);
typedef int (*GetFirmwareVersion)(void *cookie,char *version);
typedef int (*OtaUpDate)(void *cookie,char *downLoadUrl,char *version);

/**************************************************************************
* len_in(in): length of cmd
* cmd(in):    xml
* len_out(in): max length of msg_out
* msg_out(in): format of output must be <key1>out1</key1><key2>out2</key2>
* return value: if success, return 0. otherwise, return -1
**************************************************************************/
typedef int (*HandleEasycamCmd)(void* cookie, int len_in, const char* cmd, int len_out, char* msg_out);

typedef struct NativeFunc
{
GetUserNameFunc									getUsername;				
GetPassWordFunc                                 getPassword;	 
SetPassWordFunc                                 setPassword;             
GetDeviceInfoFunc                               getDeviceinfo;           
SetDatetimeSyncModeFunc                         setDatetimeSyncmode;     
GetDatetimeSyncModeFunc                         getDatetimeSyncmode;     
SetTimeZoneFunc                                 setTimezone;             
GetTimeZoneFunc                                 getTimezone;             
SetDateTimeFunc                                 setDatetime;             
GetDateTimeFunc                                 getDatetime;             
SetWifiEnableFunc                               setWifiEnable;           
GetWifiEnableFunc                               getWifiEnable;           
GetWifiApListFunc                               getWifiAplist;           
ConnectWifiApFunc                               connectWifiap;           
GetConnectedWifiApFunc                          getConnectedWifiap;      
ForgetWifiApFunc                                forgetWifiap;            
GetVideoUrlFunc                                 getVideoUrl;             
SetVideoSizeFunc                                setVideoSize;            
GetVideoSizeFunc                                getVideoSize;            
SetVideoFramerateFunc                           setVideoFramerate;       
GetVideoFramerateFunc                           getVideoFramerate;       
SetVideoBitrateFunc                             setVideoBitrate;         
GetVideoBitrateFunc                             getVideoBitrate;         
GetBrightnessMinMaxFunc                         getBrightnessMinmax;     
SetBrightnessFunc                               setBrightness;           
GetBrightnessFunc                               getBrightness;           
GetContrastMinMaxFunc                           getContrastMinmax;       
SetContrastFunc                                 setContrast;             
GetContrastFunc                                 getContrast;             
GetSaturationMinMaxFunc                         getSaturationMinmax;     
SetSaturationFunc                               setSaturation;           
GetSaturationFunc                               getSaturation;           
GetSharpnessMinMaxFunc                          getSharpnessMinmax;      
SetSharpnessFunc                                setSharpness;            
GetSharpnessFunc                                getSharpness;            
SetMDEnableFunc                                 setMDEnable;             
GetMDEnableFunc                                 getMDEnable;             
SetMDSensitivityFunc                            setMDSensitivity;        
GetMDSensitivityFunc                            getMDSensitivity;        
SetMDRegionMaskFunc                             setMDRegionmask;         
GetMDRegionMaskFunc                             getMDRegionmask;         
SetMDCoverDetectFunc                            setMDCoverdetect;        
GetMDCoverDetectFunc                            getMDCoverdetect;        
GetMAlarmFunc                                   getMAlarm;               
SetMAlarmEmailFunc                              setMAlarmemail;          
GetMAlarmEmailFunc                              getMAlarmemail;          
SetMotionCaptureEnableFunc                      setMotionCaptureenable;  
GetMotionCaptureEnableFunc                      getMotionCaptureenable;  
GetMAlarmCapturesFunc                           getMalarmCaptures;       
SetOsdDatetimeEnableFunc                        setOSDDatetimeEnable;    
GetOsdDatetimeEnableFunc                        getOSDDtetimeEnable;    
SetOsdChannelEnableFunc                         setOSDChannelEnable;     
GetOsdChannelEnableFunc                         getOSDChannelEnable;     
SetOsdDatetimePositionFunc                      setOSDDatetimePosition;  
GetOsdDatetimePositionFunc                      getPSDDatetimePosition;  
SetOsdChannelPositionFunc                       setOSDChannelPosition;   
GetOsdChannelPositionFunc                       getOSDChannelPosition;   
SetOsdChannelNameFunc                           setOSDChannelName;       
GetOsdChannelNameFunc                           getOSDChannelName;       
SetOsdChannelUrlFunc                            setOSDChannelUrl;        
MotorRollFunc                                   motorRoll;               
GetVoiceUrlFunc                                 getVoiceUrl;             
StartVoiceCallFunc                              startVoiceCall;          
StopVoiceCallFunc                               stopVoiceCall;           
SetVoiceVolumeFunc                              setVoiceVolume;          
GetVoiceVolumeFunc                              getVoiceVolume;          
GetMediaListFunc                                getMediaList;            
GetMediaUrlFunc                                 getMediaUrl;
GetDownloadUrlFunc								getDownloadUrl;
StartMediaPlayFunc                              startMediaPlay;          
StopMediaPlayFunc                               stopMediaPlay;           
IsMediaPlayingFunc                              isMediaPlaying;          
SetMediaPlayModeFunc                            setMediaPlayMode;        
GetMediaPlayModeFunc                            getMediaPlayMode;        
FormatDiskFunc                                  formatDisk;              
SetRecordEnableFunc                             setRecordEnable;         
GetRecordEnableFunc                             getRecordEnable;         
SetVoiceRecordEnableFunc                        setVoiceRecordEnable;    
GetVoiceRecordEnableFunc                        getVoiceRecordEnable;    
SetRecordPlanFunc                               setRecordPlan;           
GetRecordPlanFunc	                            getRecordPlan;	 
GetIpFunc										getIp;
DeleteFile										deleteFile;
GetCardStatus									getCardStatus;
GetStorageTatolSize							    getStorageTatolSize;
GetStorageFreeSize                              getStorageFreeSize;
GetMuteMode                                     getMuteMode;
SetMuteMode                                     setMuteMode;
GetMediaPlayerName                              getMediaPlayerName;
ParserSeekTo                                    parserSeekTo;
GetDuration                                     getDuration;
StartAutoTest                                   startAutoTest;
StopAutoTest                                    stopAutoTest;
SetIFramesNumInterval                           setIFramesNumInterval;
OtaUpDate                                       otaUpDate;
GetFirmwareVersion                              getFirmwareVersion;
HandleEasycamCmd                                handleEasycamCmd;
}NativeFunc;

void* NatMain(void * in);
void ExitNatMain(void);

void RegisterGetUserNameFunc(GetUserNameFunc, void *);
void RegisterGetPassWordFunc(GetPassWordFunc, void *);
void RegisterSetPassWordFunc(SetPassWordFunc, void *);

void RegisterNativeFunctions(NativeFunc allNativeFunc,void *);
void UnRegisterNativeFunctions(void);

#ifdef __cplusplus
}
#endif	

#endif	//__NAT__INTERFACE__
