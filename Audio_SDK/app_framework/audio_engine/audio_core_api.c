/**
 **************************************************************************************
 * @file    audio_core.c
 * @brief   audio core 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include <nds32_intrinsic.h>
#include "main_task.h"
#include "audio_core_service.h"
#include "ctrlvars.h"
#include "mcu_circular_buf.h"
#include "beep.h"
#include "dma.h"

#ifdef CFG_APP_BT_MODE_EN
#include "bt_config.h"
#include "bt_play_api.h"
#include "bt_manager.h"
#if BT_HFP_SUPPORT
#include "bt_hf_api.h"
#endif
#endif
#include "dac_interface.h"
#include "audio_vol.h"

/*******************************************************************************************************************************
 *
 *				 |***GetAdapter***|	  			 |***********CoreProcess***********|			  |***SetAdapter***|
 * ************	 ******************	 **********	 ***********************************  **********  ******************  **********
 *	SourceFIFO*->*SRCFIFO**SRAFIFO*->*PCMFrame*->*PreGainEffect**MixNet**GainEffect*->*PCMFrame*->*SRAFIFO**SRCFIFO*->*SinkFIFO*
 * ************  ******************	 **********	 ***********************************  **********  ******************  **********
 * 				 |*Context*|																			 |*Context*|
 *
 *******************************************************************************************************************************/

typedef enum
{
	AC_RUN_CHECK,//用于检测是否需要暂停任务，如果需要暂停任务，则停留再该状态
	AC_RUN_GET,
	AC_RUN_PROC,
	AC_RUN_PUT,
}AudioCoreRunState;

static AudioCoreRunState AudioState = AC_RUN_CHECK;
AudioCoreContext		AudioCore;

extern uint32_t gSysTick;

void AudioCoreSourcePcmFormatConfig(uint8_t Index, uint16_t Format)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Channels = Format;
	}
}

void AudioCoreSourceEnable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = TRUE;
	}
}

void AudioCoreSourceDisable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = FALSE;
	}
}

bool AudioCoreSourceIsEnable(uint8_t Index)
{
	return AudioCore.AudioSource[Index].Enable;
}

void AudioCoreSourceMute(uint8_t Index, bool IsLeftMute, bool IsRightMute)
{
	if(IsLeftMute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = TRUE;
	}
	if(IsRightMute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = TRUE;
	}
}

void AudioCoreSourceUnmute(uint8_t Index, bool IsLeftUnmute, bool IsRightUnmute)
{
	if(IsLeftUnmute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = FALSE;
	}
	if(IsRightUnmute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = FALSE;
	}
}

void AudioCoreSourceConfig(uint8_t Index, AudioCoreSource* Source)
{
	memcpy(&AudioCore.AudioSource[Index], Source, sizeof(AudioCoreSource));
}

void AudioCoreSinkEnable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = TRUE;
}

void AudioCoreSinkDisable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = FALSE;
}

bool AudioCoreSinkIsEnable(uint8_t Index)
{
	return AudioCore.AudioSink[Index].Enable;
}

void AudioCoreSinkConfig(uint8_t Index, AudioCoreSink* Sink)
{
	memcpy(&AudioCore.AudioSink[Index], Sink, sizeof(AudioCoreSink));
}


void AudioCoreProcessConfig(AudioCoreProcessFunc AudioEffectProcess)
{
	AudioCore.AudioEffectProcess = AudioEffectProcess;
}

///**
// * @func        AudioCoreConfig
// * @brief       AudioCore参数块，本地化API
// * @param       AudioCoreContext *AudioCoreCt
// * @Output      None
// * @return      None
// * @Others      外部配置的参数块，复制一份到本地
// */
//void AudioCoreConfig(AudioCoreContext *AudioCoreCt)
//{
//	memcpy(&AudioCore, AudioCoreCt, sizeof(AudioCoreContext));
//}

bool AudioCoreInit(void)
{
	DBG("AudioCore init\n");
	return TRUE;
}

void AudioCoreDeinit(void)
{
	AudioState = AC_RUN_CHECK;
}

/**
 * @func        AudioCoreRun
 * @brief       音源拉流->音效处理+混音->推流
 * @param       None
 * @Output      None
 * @return      None
 * @Others      当前由audioCoreservice任务保障此功能有效持续。
 * Record
 */
extern uint32_t 	IsAudioCorePause;
extern uint32_t 	IsAudioCorePauseMsgSend;
void AudioProcessMain(void);
__attribute__((optimize("Og")))
void AudioCoreRun(void)
{
	bool ret;
	switch(AudioState)
	{
		case AC_RUN_CHECK:
			if(IsAudioCorePause == TRUE)
			{
				if(IsAudioCorePauseMsgSend == TRUE)
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_AUDIO_CORE_HOLD;
					MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);

					IsAudioCorePauseMsgSend = FALSE;
				}
				return;
			}
		case AC_RUN_GET:
			AudioCoreIOLenProcess();
#ifdef CFG_APP_BT_MODE_EN
			//audio core检测:蓝牙模式下数据播空监控
			extern void BtPlayACBtMonitor(uint16_t empty_flag);
			BtPlayACBtMonitor(SOURCE_BIT_GET(AudioCore.FrameReady, APP_SOURCE_NUM));
#endif
			ret = AudioCoreSourceSync();
			if(ret == FALSE)
			{
				return;
			}

		case AC_RUN_PROC:
			AudioProcessMain();
			AudioState = AC_RUN_PUT;

		case AC_RUN_PUT:
			ret = AudioCoreSinkSync();
			if(ret == FALSE)
			{
//				AudioCoreIOLenProcess();
				return;
			}
			AudioState = AC_RUN_CHECK;
			break;
		default:
			break;
	}
}

//音效处理函数，主入口
//将mic通路数据剥离出来统一处理
//mic通路数据和具体模式无关
//提示音通路无音效，剥离后在sink端混音。
void AudioProcessMain(void)
{
	AudioCoreSourceMuteApply();
#ifdef CFG_FUNC_RECORDER_EN
	if(AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].Enable == TRUE)
	{
		if(AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].Channels == 1)
		{
			uint16_t i;
			for(i = SOURCEFRAME(PLAYBACK_SOURCE_NUM) * 2 - 1; i > 0; i--)
			{
				AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf[i / 2];
			}
		}
	}
#endif
//	if(AudioCore.AudioSource[APP_SOURCE_NUM].Active == TRUE)////music buff
//	{
//		#if (BT_HFP_SUPPORT) && defined(CFG_APP_BT_MODE_EN)
//		if(GetSystemMode() != ModeBtHfPlay)
//		#endif
//		{
//			if(AudioCore.AudioSource[APP_SOURCE_NUM].Channels == 1)
//			{
//				uint16_t i;
//				for(i = SOURCEFRAME(APP_SOURCE_NUM) * 2 - 1; i > 0; i--)
//				{
//					AudioCore.AudioSource[APP_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[APP_SOURCE_NUM].PcmInBuf[i / 2];
//				}
//			}
//		}
//	}
		
#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(AudioCore.AudioSource[REMIND_SOURCE_NUM].Active == TRUE)////remind buff
	{
		if(AudioCore.AudioSource[REMIND_SOURCE_NUM].Channels == 1)
		{
			uint16_t i;
			for(i = SOURCEFRAME(REMIND_SOURCE_NUM) * 2 - 1; i > 0; i--)
			{
				AudioCore.AudioSource[REMIND_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[REMIND_SOURCE_NUM].PcmInBuf[i / 2];
			}
		}
	}	
#endif


	if(AudioCore.AudioEffectProcess != NULL)
	{
		AudioCore.AudioEffectProcess((AudioCoreContext*)&AudioCore);
	}
	
#ifdef CFG_FUNC_BEEP_EN
    if(AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].Active == TRUE)   ////dacx buff
	{
		Beep(AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf, SINKFRAME(AUDIO_DAC0_SINK_NUM));
	}
#endif
    AudioCoreSinkMuteApply();
}

//音量淡入淡出
#define MixerFadeVolume(a, b, c)  	\
    if(a > b + c)		    \
	{						\
		a -= c;				\
	}						\
	else if(a + c < b)	   	\
	{						\
		a += c;				\
	}						\
	else					\
	{						\
		a = b;				\
	}


void AudioCoreSourceMuteApply(void)
{
	uint32_t i;
	uint8_t source_index;
	uint16_t LeftVol, RightVol, TargetVol, LeftVolStep, RightVolStep;
	bool mute;

	for(source_index = 0; source_index < AUDIO_CORE_SOURCE_MAX_NUM; source_index++)
	{
		mute = AudioCore.AudioSource[source_index].LeftMuteFlag || AudioCore.AudioSource[source_index].RightMuteFlag || mainAppCt.gSysVol.MuteFlag
				|| IsAudioCorePause || (!AudioCore.AudioSource[source_index].FrameReady);
		if((!AudioCore.AudioSource[source_index].Active) || (!AudioCore.AudioSource[source_index].Enable) || (mute == AudioCore.AudioSource[source_index].MuteFlagbk))
		{
//			if(AudioCoreSourceToRoboeffect(source_index) != AUDIOCORE_SOURCE_SINK_ERROR)
			if(AudioEffect_Parambin_GetSourceBuffer(source_index))
			{
				if(!AudioCore.AudioSource[source_index].Active || !AudioCore.AudioSource[source_index].Enable
						|| mainAppCt.gSysVol.MuteFlag || AudioCore.AudioSource[source_index].LeftMuteFlag)
				{
//					memset(roboeffect_get_source_buffer(AudioEffect.context_memory, AudioCoreSourceToRoboeffect(source_index)),
//										0, roboeffect_get_buffer_size(AudioEffect.context_memory));
					memset(AudioEffect_Parambin_GetSourceBuffer(source_index), 0, roboeffect_get_buffer_size(AudioEffectParambin.context_memory));
					AudioCore.AudioSource[source_index].FrameReady = FALSE;
				}
			}
			AudioCore.AudioSource[source_index].MuteFlagbk = mute;
			continue;
		}

		TargetVol = mute ? 0:4096;
		LeftVol = 4096 - TargetVol;
		RightVol = LeftVol;
		LeftVolStep = 4096 / SOURCEFRAME(source_index) + (4096 % SOURCEFRAME(source_index) ? 1 : 0);
		RightVolStep = LeftVolStep;
#ifdef CFG_AUDIO_WIDTH_24BIT
		if(AudioCore.AudioSource[source_index].BitWidth == PCM_DATA_24BIT_WIDTH //24bit 数据
		 || AudioCore.AudioSource[source_index].BitWidthConvFlag     //在AudioCoreSourceGet扩充到24bit
			)
		{
			if(AudioCore.AudioSource[source_index].Channels == 2)
			{
				for(i=0; i < SOURCEFRAME(source_index); i++)
				{
					AudioCore.AudioSource[source_index].PcmInBuf[2 * i + 0] = __nds32__clips((((int64_t)AudioCore.AudioSource[source_index].PcmInBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (24)-1);
					AudioCore.AudioSource[source_index].PcmInBuf[2 * i + 1] = __nds32__clips((((int64_t)AudioCore.AudioSource[source_index].PcmInBuf[2 * i + 1]) * RightVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(AudioCore.AudioSource[source_index].Channels == 1)
			{
				for(i=0; i<SOURCEFRAME(source_index); i++)
				{
					AudioCore.AudioSource[source_index].PcmInBuf[i] = __nds32__clips((((int64_t)AudioCore.AudioSource[source_index].PcmInBuf[i]) * LeftVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		else
#endif
		{
			int16_t * PcmInBuf = (int16_t *)AudioCore.AudioSource[source_index].PcmInBuf;
			if(AudioCore.AudioSource[source_index].Channels == 2)
			{
				for(i=0; i < SOURCEFRAME(source_index); i++)
				{
					PcmInBuf[2 * i + 0] = __nds32__clips((((int32_t)PcmInBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (16)-1);
					PcmInBuf[2 * i + 1] = __nds32__clips((((int32_t)PcmInBuf[2 * i + 1]) * RightVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(AudioCore.AudioSource[source_index].Channels == 1)
			{
				for(i=0; i<SOURCEFRAME(source_index); i++)
				{
					PcmInBuf[i] = __nds32__clips((((int32_t)PcmInBuf[i]) * LeftVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		AudioCore.AudioSource[source_index].MuteFlagbk = mute;
	}
}

void AudioCoreSinkMuteApply(void)
{
	uint32_t i;
	uint8_t sink_index;
	uint16_t LeftVol, RightVol, TargetVol, LeftVolStep, RightVolStep;
	bool mute;

	for(sink_index = 0; sink_index < AUDIO_CORE_SINK_MAX_NUM; sink_index++)
	{
		mute = AudioCore.AudioSink[sink_index].LeftMuteFlag || AudioCore.AudioSink[sink_index].RightMuteFlag || mainAppCt.gSysVol.MuteFlag || IsAudioCorePause;
		if((!AudioCore.AudioSink[sink_index].Active) || (!AudioCore.AudioSink[sink_index].Enable) || (mute == AudioCore.AudioSink[sink_index].MuteFlagbk))
		{
//			if(AudioCoreSinkToRoboeffect(sink_index) != AUDIOCORE_SOURCE_SINK_ERROR)
			if(AudioEffect_Parambin_GetSinkBuffer(sink_index))
			{
				if(!AudioCore.AudioSink[sink_index].Active || !AudioCore.AudioSink[sink_index].Enable
						|| mainAppCt.gSysVol.MuteFlag || AudioCore.AudioSink[sink_index].LeftMuteFlag)
				{
//					memset(roboeffect_get_sink_buffer(AudioEffect.context_memory, AudioCoreSinkToRoboeffect(sink_index)),
//										0, roboeffect_get_buffer_size(AudioEffect.context_memory));
					memset(AudioEffect_Parambin_GetSinkBuffer(sink_index), 0, roboeffect_get_buffer_size(AudioEffectParambin.context_memory));
				}
			}
			AudioCore.AudioSink[sink_index].MuteFlagbk = mute;
			continue;
		}

		TargetVol = mute ? 0:4096;
		LeftVol = 4096 - TargetVol;
		RightVol = LeftVol;
		LeftVolStep = 4096 / SINKFRAME(sink_index) + (4096 % SINKFRAME(sink_index) ? 1 : 0);
		RightVolStep = LeftVolStep;

#ifdef CFG_AUDIO_WIDTH_24BIT
		if(AudioCore.AudioSink[sink_index].BitWidth == PCM_DATA_24BIT_WIDTH //24bit 数据
		 //|| AudioCore.AudioSink[sink_index].BitWidthConvFlag     //在AudioCoreSinkSet扩充到24bit,此时还是16bit
			)
		{
			if(AudioCore.AudioSink[sink_index].Channels == 2)
			{
				for(i=0; i < SINKFRAME(sink_index); i++)
				{
					AudioCore.AudioSink[sink_index].PcmOutBuf[2 * i + 0] = __nds32__clips((((int64_t)AudioCore.AudioSink[sink_index].PcmOutBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (24)-1);
					AudioCore.AudioSink[sink_index].PcmOutBuf[2 * i + 1] = __nds32__clips((((int64_t)AudioCore.AudioSink[sink_index].PcmOutBuf[2 * i + 1]) * RightVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(AudioCore.AudioSink[sink_index].Channels == 1)
			{
				for(i=0; i<SINKFRAME(sink_index); i++)
				{
					AudioCore.AudioSink[sink_index].PcmOutBuf[i] = __nds32__clips((((int64_t)AudioCore.AudioSink[sink_index].PcmOutBuf[i]) * LeftVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		else
#endif
		{
			int16_t * PcmOutBuf = (int16_t *)AudioCore.AudioSink[sink_index].PcmOutBuf;
			if(AudioCore.AudioSink[sink_index].Channels == 2)
			{
				for(i=0; i < SINKFRAME(sink_index); i++)
				{
					PcmOutBuf[2 * i + 0] = __nds32__clips((((int32_t)PcmOutBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (16)-1);
					PcmOutBuf[2 * i + 1] = __nds32__clips((((int32_t)PcmOutBuf[2 * i + 1]) * RightVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(AudioCore.AudioSink[sink_index].Channels == 1)
			{
				for(i=0; i<SINKFRAME(sink_index); i++)
				{
					PcmOutBuf[i] = __nds32__clips((((int32_t)PcmOutBuf[i]) * LeftVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		AudioCore.AudioSink[sink_index].MuteFlagbk = mute;
	}
}
