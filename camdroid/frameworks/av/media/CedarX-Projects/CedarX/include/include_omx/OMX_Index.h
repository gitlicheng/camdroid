/*
 * Copyright (c) 2005 The Khronos Group Inc. 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions: 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 *
 */

/** @file OMX_Index.h
 *  The OMX_Index header file contains the definitions for both applications
 *  and components .
 */


#ifndef OMX_Index_h
#define OMX_Index_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Each OMX header must include all required header files to allow the
 *  header to compile without errors.  The includes below are required
 *  for this header file to compile successfully 
 */
#include <include_omx/OMX_Types.h>


/** The OMX_INDEXTYPE enumeration is used to select a structure when either
 *  getting or setting parameters and/or configuration data.  Each entry in 
 *  this enumeration maps to an OMX specified structure.  When the 
 *  OMX_GetParameter, OMX_SetParameter, OMX_GetConfig or OMX_SetConfig methods
 *  are used, the second parameter will always be an entry from this enumeration
 *  and the third entry will be the structure shown in the comments for the entry.
 *  For example, if the application is initializing a cropping function, the 
 *  OMX_SetConfig command would have OMX_ConfigInputCrop as the second parameter 
 *  and would send a pointer to an initialized OMX_RECTTYPE structure as the 
 *  third parameter.
 *  
 *  The enumeration entries named with the OMX_Config prefix are sent using
 *  the OMX_SetConfig command and the enumeration entries named with the
 *  OMX_PARAM_ prefix are sent using the OMX_SetParameter command.
 */
typedef enum OMX_INDEXTYPE {

    OMX_IndexComponentStartUnused = 0x01000000,
    OMX_IndexParamPriorityMgmt, /**< reference: OMX_PRIORITYMGMTTYPE */
    OMX_IndexParamAudioInit, /**< reference: OMX_PORT_PARAM_TYPE  */
    OMX_IndexParamImageInit, /**< reference: OMX_PORT_PARAM_TYPE  */
    OMX_IndexParamVideoInit, /**< reference: OMX_PORT_PARAM_TYPE  */
    OMX_IndexParamOtherInit, /**< reference: OMX_PORT_PARAM_TYPE  */

    OMX_IndexPortStartUnused = 0x02000000,
    OMX_IndexParamPortDefinition, /**< reference: OMX_PARAM_PORTDEFINITIONTYPE */
    OMX_IndexParamCompBufferSupplier, /**< reference: OMX_PARAM_BUFFERSUPPLIERTYPE (*/ 
    OMX_IndexReservedStartUnused = 0x03000000,


    /* Audio parameters and configurations */
    OMX_IndexAudioStartUnused = 0x04000000,
    OMX_IndexParamAudioPortFormat, /**< reference: OMX_AUDIO_PARAM_PORTFORMATTYPE */
    OMX_IndexParamAudioPcm,    /**< reference: OMX_AUDIO_PARAM_PCMMODETYPE */
    OMX_IndexParamAudioAac,    /**< reference: OMX_AUDIO_PARAM_AACPROFILETYPE */
    OMX_IndexParamAudioRa,     /**< reference: OMX_AUDIO_PARAM_RATYPE */
    OMX_IndexParamAudioMp3,    /**< reference: OMX_AUDIO_PARAM_MP3TYPE */
    OMX_IndexParamAudioAdpcm,  /**< reference: OMX_AUDIO_PARAM_ADPCMTYPE */
    OMX_IndexParamAudioG723,   /**< reference: OMX_AUDIO_PARAM_G723TYPE */
    OMX_IndexParamAudioG729,   /**< reference: OMX_AUDIO_PARAM_G729TYPE */
    OMX_IndexParamAudioAmr,    /**< reference: OMX_AUDIO_PARAM_AMRTYPE */
    OMX_IndexParamAudioWma,    /**< reference: OMX_AUDIO_PARAM_WMATYPE */
    OMX_IndexParamAudioSbc,    /**< reference: OMX_AUDIO_PARAM_SBCTYPE */
    OMX_IndexParamAudioMidi,   /**< reference: OMX_AUDIO_PARAM_MIDITYPE */
    OMX_IndexParamAudioGsm_FR, /**< reference: OMX_AUDIO_PARAM__GSMFRTYPE */
    OMX_IndexParamAudioMidiLoadUserSound, /**< reference: OMX_AUDIO_PARAM_MIDILOADUSERSOUNDTYPE */
    OMX_IndexParamAudioG726,     /**< reference: OMX_AUDIO_PARAM_G726TYPE */
    OMX_IndexParamAudioGsm_EFR,  /**< reference: OMX_AUDIO_PARAM__GSMEFRTYPE */
    OMX_IndexParamAudioGsm_HR,   /**< reference: OMX_AUDIO_PARAM__GSMHRTYPE */
    OMX_IndexParamAudioPdc_FR,   /**< reference: OMX_AUDIO_PARAM__PDCFRTYPE */
    OMX_IndexParamAudioPdc_EFR,  /**< reference: OMX_AUDIO_PARAM__PDCEFRTYPE */
    OMX_IndexParamAudioPdc_HR,   /**< reference: OMX_AUDIO_PARAM__PDCHRTYPE */
    OMX_IndexParamAudioTdma_FR,  /**< reference: OMX_AUDIO_PARAM__TDMAFRTYPE */
    OMX_IndexParamAudioTdma_EFR, /**< reference: OMX_AUDIO_PARAM__TDMAEFRTYPE */
    OMX_IndexParamAudioQcelp8,   /**< reference: OMX_AUDIO_PARAM__QCELP8TYPE */
    OMX_IndexParamAudioQcelp13,  /**< reference: OMX_AUDIO_PARAM__QCELP13TYPE */
    OMX_IndexParamAudioEvrc,     /**< reference: OMX_AUDIO_PARAM__EVRCTYPE */
    OMX_IndexParamAudioSmv,      /**< reference: OMX_AUDIO_PARAM__SMVTYPE */
    OMX_IndexParamAudioVorbis,   /**< reference: OMX_AUDIO_PARAM__VORBISTYPE */

    OMX_IndexConfigAudioMidiImmediateEvent, /**< OMX_AUDIO_CONFIG_MIDIIMMEDIATEEVENTTYPE */
    OMX_IndexConfigAudioMidiControl, /**< reference: OMX_AUDIO_CONFIG_MIDICONTROLTYPE */
    OMX_IndexConfigAudioMidiSoundBankProgram, /**< reference: OMX_AUDIO_CONFIG_MIDISOUNDBANKPROGRAMTYPE */
    OMX_IndexConfigAudioMidiStatus,  /**< reference: OMX_AUDIO_CONFIG_MIDISTATUSTYPE */
    OMX_IndexConfigAudioMidiMetaEvent, /**< reference: OMX_AUDIO_CONFIG_MIDIMETAEVENTTYPE */
    OMX_IndexConfigAudioMidiMetaEventData, /**< reference: OMX_AUDIO_CONFIG_MIDIMETAEVENTDATATYPE */
    OMX_IndexConfigAudioVolume,      /**< reference: OMX_AUDIO_CONFIG_VOLUMETYPE */
    OMX_IndexConfigAudioBalance,     /**< reference: OMX_AUDIO_CONFIG_BALANCETYPE */
    OMX_IndexConfigAudioChannelMute, /**< reference: OMX_AUDIO_CONFIG_CHANNELMUTETYPE */
    OMX_IndexConfigAudioMute,        /**< reference: OMX_AUDIO_CONFIG_MUTETYPE */
    OMX_IndexConfigAudioLoudness,    /**< reference: OMX_AUDIO_CONFIG_LOUDNESSTYPE */
    OMX_IndexConfigAudioEchoCancelation, /**< reference: OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE */
    OMX_IndexConfigAudioNoiseReduction,  /**< reference: OMX_AUDIO_CONFIG_NOISEREDUCTIONTYPE */
    OMX_IndexConfigAudioBass,        /**< reference: OMX_AUDIO_CONFIG_BASSTYPE */
    OMX_IndexConfigAudioTreble,      /**< reference: OMX_AUDIO_CONFIG_TREBLETYPE */
    OMX_IndexConfigAudioStereoWidening, /**< reference: OMX_AUDIO_CONFIG_STEREOWIDENINGTYPE */
    OMX_IndexConfigAudioChorus,      /**< reference: OMX_AUDIO_CONFIG_CHORUSTYPE */
    OMX_IndexConfigAudioEqualizer,   /**< reference: OMX_AUDIO_CONFIG_EQUALIZERTYPE */
    OMX_IndexConfigAudioReverberation, /**< reference: OMX_AUDIO_CONFIG_REVERBERATIONTYPE */

    /* Image specific parameters and configurations */
    OMX_IndexImageStartUnused = 0x05000000,
    OMX_IndexParamImagePortFormat,   /**< reference: OMX_IMAGE_PARAM_PORTFORMATTYPE */
    OMX_IndexParamFlashControl,      /**< refer to OMX_IMAGE_PARAM_FLASHCONTROLTYPE */
    OMX_IndexConfigFocusControl,     /**< refer to OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE */
    OMX_IndexParamQFactor,           /**< refer to OMX_IMAGE_PARAM_QFACTORTYPE */
    OMX_IndexParamQuantizationTable, /**< refer to OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE */
    OMX_IndexParamHuffmanTable,      /**< For jpeg, refer to OMX_IMAGE_PARAM_HUFFMANTTABLETYPE */

    /* Video specific parameters and configurations */
    OMX_IndexVideoStartUnused = 0x06000000,
    OMX_IndexParamVideoPortFormat,   /**< reference: OMX_VIDEO_PARAM_PORTFORMATTYPE */
    OMX_IndexParamVideoQuantization, /**< reference: OMX_VIDEO_PARAM_QUANTIZATIONPARAMTYPE */
    OMX_IndexParamVideoFastUpdate,   /**< reference: OMX_VIDEO_PARAM_VIDEOFASTUPDATETYPE */
    OMX_IndexParamVideoBitrate,      /**< reference: OMX_VIDEO_PARAM_BITRATETYPE */
    OMX_IndexParamVideoMotionVector,    /**< reference: OMX_VIDEO_PARAM_MOTIONVECTORTYPE */
    OMX_IndexParamVideoIntraRefresh,    /**< reference: OMX_VIDEO_PARAM_INTRAREFRESHTYPE */
    OMX_IndexParamVideoErrorCorrection, /**< reference: OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE */
    OMX_IndexParamVideoVBSMC, /**< reference:OMX_VIDEO_PARAM_VBSMCTYPE */
    OMX_IndexParamVideoMpeg2, /**< reference:OMX_VIDEO_PARAM_MPEG2TYPE */
    OMX_IndexParamVideoMpeg4, /**< reference: OMX_VIDEO_CONFIG_MPEG4TYPE */
    OMX_IndexParamVideoWmv,   /**< reference:OMX_VIDEO_PARAM_WMVTYPE */
    OMX_IndexParamVideoRv,    /**< reference:OMX_VIDEO_PARAM_RVTYPE */
    OMX_IndexParamVideoAvc,   /**< reference:OMX_VIDEO_PARAM_AVCTYPE */
    OMX_IndexParamVideoH263,  /**< reference:OMX_VIDEO_PARAM_H263TYPE */

    /* Image & Video common Configurations */
    OMX_IndexCommonStartUnused = 0x07000000,
    OMX_IndexParamCommonDeblocking, /**< reference: OMX_PARAM_DEBLOCKINGTYPE */
    OMX_IndexParamCommonSensorMode, /**< reference: OMX_PARAM_SENSORMODETYPE */
    OMX_IndexParamCommonInterleave, /** reference: OMX_PARAM_INTERLEAVETYPE */
    OMX_IndexConfigCommonColorFormatConversion,   /**< reference: OMX_CONFIG_COLORCONVERSIONTYPE */
    OMX_IndexConfigCommonScale,  /**< reference: OMX_CONFIG_SCALEFACTORTYPE */
    OMX_IndexConfigCommonImageFilter, /**< reference: OMX_CONFIG_IMAGEFILTERTYPE */
    OMX_IndexConfigCommonColorEnhancement, /**< reference: OMX_CONFIG_COLORENHANCEMENTTYPE */
    OMX_IndexConfigCommonColorKey, /**< reference: OMX_CONFIG_COLORKEYTYPE */
    OMX_IndexConfigCommonColorBlend, /**< reference: OMX_CONFIG_COLORBLENDTYPE */
    OMX_IndexConfigCommonFrameStabilisation, /**< reference: OMX_CONFIG_FRAMESTABTYPE */
    OMX_IndexConfigCommonRotate, /**< reference: OMX_CONFIG_ROTATIONTYPE */
    OMX_IndexConfigCommonMirror, /**< reference: OMX_CONFIG_MIRRORTYPE */
    OMX_IndexConfigCommonOutputPosition, /**< reference: OMX_CONFIG_POINTTYPE */
    OMX_IndexConfigCommonInputCrop, /**< reference: OMX_CONFIG_RECTTYPE */
    OMX_IndexConfigCommonOutputCrop, /**< reference: OMX_CONFIG_RECTTYPE */
    OMX_IndexConfigCommonDigitalZoom,  /**< reference: OMX_SCALEFACTORTYPE */
    OMX_IndexConfigCommonOpticalZoom, /**< reference: OMX_SCALEFACTORTYPE*/
    OMX_IndexConfigCommonWhiteBalance, /**< reference: OMX_CONFIG_WHITEBALCONTROLTYPE */
    OMX_IndexConfigCommonExposure, /**< reference: OMX_CONFIG_EXPOSURECONTROLTYPE */
    OMX_IndexConfigCommonContrast, /**< reference to OMX_CONFIG_CONTRASTTYPE */
    OMX_IndexConfigCommonBrightness, /**< reference to OMX_CONFIG_BRIGHTNESSTYPE */
    OMX_IndexConfigCommonBacklight, /**< reference to OMX_CONFIG_BACKLIGHTTYPE */
    OMX_IndexConfigCommonGamma, /**< reference to OMX_CONFIG_GAMMATYPE */
    OMX_IndexConfigCommonSaturation, /**< reference to OMX_CONFIG_SATURATIONTYPE */
    OMX_IndexConfigCommonLightness, /**< reference to OMX_CONFIG_LIGHTNESSTYPE */
    OMX_IndexConfigCommonExclusionRect, /** reference: OMX_CONFIG_RECTTYPE */
    OMX_IndexConfigCommonDithering, /**< reference: OMX_TIME_CONFIG_DITHERTYPE */
    OMX_IndexConfigCommonPlaneBlend, /** reference: OMX_CONFIG_PLANEBLENDTYPE */
    //OMX_IndexVendorSetDynamicRotate,/** dynamic rotate*/


    /* Reserved Configuration range */
    OMX_IndexOtherStartUnused = 0x08000000,
    OMX_IndexParamOtherPortFormat, /**< reference: OMX_OTHER_PARAM_PORTFORMATTYPE */
    OMX_IndexConfigOtherPower, /**< reference: OMX_OTHER_CONFIG_POWERTYPE */
    OMX_IndexConfigOtherStats, /**< reference: OMX_OTHER_CONFIG_STATSTYPE */


    /* Reserved Time range */
    OMX_IndexTimeStartUnused = 0x09000000,
    OMX_IndexConfigTimeScale, /**< reference: OMX_TIME_CONFIG_SCALETYPE */
    OMX_IndexConfigTimeClockState,  /**< reference: OMX_TIME_CONFIG_CLOCKSTATETYPE */
    OMX_IndexConfigTimeActiveRefClock, /**< reference: OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE */
    OMX_IndexConfigTimeCurrentMediaTime, /**< reference: OMX_TIME_CONFIG_TIMESTAMPTYPE (read only)*/
    OMX_IndexConfigTimeCurrentWallTime, /**< reference: OMX_TIME_CONFIG_TIMESTAMPTYPE (read only)*/
    OMX_IndexConfigTimeCurrentAudioReference, /**< reference: OMX_TIME_CONFIG_TIMESTAMPTYPE (write only) */
    OMX_IndexConfigTimeCurrentVideoReference, /**< reference: OMX_TIME_CONFIG_TIMESTAMPTYPE (write only) */
    OMX_IndexConfigTimeMediaTimeRequest, /**< reference: OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE (write only) */
    OMX_IndexConfigTimeClientStartTime,  /**<reference:  OMX_TIME_CONFIG_TIMESTAMPTYPE (write only) */
    OMX_IndexConfigTimePosition,  /**< reference: OMX_TIME_CONFIG_TIMESTAMPTYPE */
    OMX_IndexConfigTimeSeekMode,  /**< reference: OMX_TIME_CONFIG_SEEKMODETYPE */
    OMX_IndexConfigTimeRelativeMediaTime, /*add by vendor*/
    OMX_IndexConfigWallTimeBase,


    /* Reserved Configuration range */


    /* Vendor specific area */
    OMX_IndexIndexVendorStartUnused = 0xFF000000,
    OMX_IndexVendorSetDemuxType,
    OMX_IndexVendorGetDemuxType,

    OMX_IndexVendorDisableTrack,
    OMX_IndexVendorGetMediaInfo,
    OMX_IndexVendorGetPortParam,
    OMX_IndexVendorSetTunnelInfo,
    OMX_IndexVendorReConfigAudioInfo,
    OMX_IndexVendorSetDataSource,
    OMX_IndexVendorSetPortCallback,
    OMX_IndexVendorSetAsRefClock,
    OMX_IndexVendorAdjustClock,
    OMX_IndexVendorConfigTimeClientForceStart,
    OMX_IndexVendorSeekToPosition,
    OMX_IndexVendorSetDecodeMode,
    OMX_IndexVendorSwitchAudio,
    OMX_IndexVendorSetStreamEof,
    OMX_IndexVendorClearStreamEof,
    OMX_IndexVendorGetFilePosition,
    OMX_IndexVendorSelectAudioOut,
    OMX_IndexVendorInitInstance,
    OMX_IndexVendorSetFileSize,
    OMX_IndexVendorSetSpecialFlags,
    OMX_IndexVendorResetAnyway,
    OMX_IndexVendorSetVolume,
    OMX_IndexVendorSetVideoPortDef,

    OMX_IdexVendorStreamFormatFile,
    OMX_IndexVendorDisable3D,
    OMX_IndexVendorSet3DSourceFormat,
    OMX_IndexVendorGet3DSourceFormat,
    //OMX_IndexVendorSetAnaglaghType,
    //OMX_IndexVendorOpenAnaglathTransform,
    //OMX_IndexVendorCloseAnaglathTransform,

    //OMX_IndexVendorSetDisplayMode,
    OMX_IndexDataReady,
    OMX_IndexVendorIsRequstingData,

    OMX_IndexVendorSetIdxSubFile,
    OMX_IndexVendorAVSyncException,

    OMX_IndexVendorGetScreenInfo,

    OMX_IndexVendorSetCedarvRotation,
    OMX_IndexVendorSetCedarvMaxOutputWidth,
    OMX_IndexVendorSetCedarvMaxOutputHeight,
    OMX_IndexVendorSetCedarvOutputSetting,  //set vdeclib's output ePixelFormat.

    OMX_IndexVendorDisableProprityTrack,
    OMX_IndexVendorSetAudioChannelMute,
    OMX_IndexVendorSetAudioChannelNums,
    OMX_IndexVendorGetSysTime,

    OMX_IndexVendorDropBFrameInDecoder,
    OMX_IndexVendorKeyFrameDecoded,

    OMX_IndexVendorSetSoftChipVersion,
    //OMX_IndexVendorSetCedarvContext,
    OMX_IndexVendorSetOrgDemuxType,
    OMX_IndexVendorSetMaxResolution,

    OMX_IndexVendorAdjustClock2, //for StreamingSource (wifi display)

    //clock component command
    OMX_IndexVendor_ClockComponent_SetVps = 0xFF000200,      //set vpsspeed to clock_component. pComponentConfigStructure = (__s32)vpsspeed, -40~100
    OMX_IndexVendor_ClockComponent_GetVps,

    //audio render component command
    OMX_IndexVendor_AudioRenderComponent_SetVps = 0xFF000300,      //set vpsspeed to audio render. pComponentConfigStructure = (__s32)vpsspeed, -40~100

    //video render component command
    OMX_IndexVendor_VideoRenderComponent_SetVRenderMode = 0xFF000400,      //set VideoRender mode. VideoRender_GUI

    //video decode component
    OMX_IndexVendor_VideoDecodeComponent_SetDisplayFrameRequestMode = 0xFF000500,   //how to malloc display_frame. 0:malloc self; 1:malloc from GUI
    
    OMX_IndexVendorSwitchSubtilte = 0xFF001000,
    OMX_IndexVendorDisableSubtilte,
    OMX_IndexVendorSetSubtitleType,
	OMX_IndexGetParamSubtitle,
	OMX_IndexSetParamSubtitle,
	OMX_IndexVendorSwitchDiffStyleSubtitle,
    OMX_IndexVendorSwitchSeamlessAudio,
    OMX_IndexVendorDisableMediaType,
    OMX_IndexVendorPlayBDFile,
    OMX_IndexVendorInitSubrender,

	// star add for recorder
	OMX_IndexVendorSetFP = 0xFF100000,  //for old method of RecRender
	OMX_IndexVendorSetVideoInfo,
	OMX_IndexVendorSetAudioInfo,
	OMX_IndexVendorSetPreviewInfo,
	OMX_IndexVendorGetDuration,
	OMX_IndexVendorGetFileSize,
	OMX_IndexVendorCacheState,  //get cache state from recRender or EncodeComponent
	OMX_IndexVendorSetRecMode,
	OMX_IndexVendorSetTimeLapse,
	OMX_IndexVendorSetAvsCounter,
	OMX_IndexVendorSetMuxerMode,    //for old method of RecRender
	OMX_IndexVendorGetExtraData,
	OMX_IndexVendorSetExtraData,
	OMX_IndexVendorSetURL,
	//OMX_IndexVendorSetAudioRecorderMode,
	OMX_IndexVendorSetRecMotionDetectionCB,
	//OMX_IndexVendorSetMotionParam,
	OMX_IndexVendorSetVEncBitrate,
	OMX_IndexVendorSetVEncKeyFrameInterval,
	OMX_IndexVendor_Venc_SetOutCacheTime,
	OMX_IndexVendorResetVEncKeyFrame,   //immediately encode a key frame.

	OMX_IndexVendorProbe,		//* for the user demux module to probe.

	OMX_IndexVendorSetRecRenderCallbackInfo,
	OMX_IndexVendorGetDuarations,
	OMX_IndexVendorForceStopNuCachedSource,
	// OMX_IndexVendorSetMuxerMode is for old method.
    // OMX_IndexConfigVendorAddOutputSinkInfo, OMX_CommandVendorAddOutputSinkInfo, OMX_CommandVendorRemoveOutputSinkInfo are for new method. Donot use two methods at the same time.
	OMX_IndexConfigVendorAddOutputSinkInfo,       //for RecRender
	OMX_IndexConfigVendorSwitchFd,  //new method for RecRender, OMX_IndexVendorSetFP.
	OMX_IndexConfigVendorSetSdcardState,

	//add by yaosen 2013-1-29
    OMX_IndexVendorGetCurrentCachedDataSize,
    OMX_IndexVendorGetBitrate,

    OMX_IndexVendorSetCacheParameter,
    OMX_IndexVendorGetCacheParameter,

	OMX_IndexVendorGetVBVFrameNum,
    OMX_IndexVendorSetCedarvVBVSize,

    OMX_IndexVendorSetDefaultLowWaterThreshold,
    OMX_IndexVendorSetDefaultHighWaterThreshold,

    //add by weihongqiang for IPTV
    OMX_IndexVendorSetAVSync,
    OMX_IndexVendorSetStreamSourceType,
    OMX_IndexVendorClearBuffer,
    OMX_IndexVendorSwitchAuioChannnel,

    OMX_IndexVendorGetMediaType,

    OMX_IndexVendorSetSubtitleInfo,
	// add by liuxioayan for subtitle delay
	OMX_IndexVendorSetSubDelay,

	OMX_IndexVendorSetDuration,

	//OMX_IndexVendorSetFallocateSync,
	//OMX_IndexVendorSetFallocateSize,
	
	OMX_IndexVendorSetImpactFileDuration,
    OMX_IndexVendorSetFsWriteMode,
    OMX_IndexVendorSetFsSimpleCacheSize,

    OMX_IndexMax = 0x7FFFFFFF

} OMX_INDEXTYPE;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
