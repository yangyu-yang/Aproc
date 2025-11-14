/**
 **************************************************************************************
 * @file    audio_effect_control.c
 * @brief   Implementation of Audio Effect Control API
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <nds32_utils_math.h>
#include <string.h>
#include "audio_effect_parambin.h"
#include "audio_effect_control.h"
#include "audio_vol.h"
#include "main_task.h"
#include "breakpoint.h"
#include "audio_core_service.h"
#include "communication.h"


void AudioEffect_Parambin_EffectModeAutoSwitch(void)
{
    uint16_t flow_target = 0;
    uint16_t param_target = 0;
    uint16_t max_flows = AudioEffect_Parambin_GetTotalFlowNum();
    bool found_valid_flow = FALSE;
    uint16_t original_flow = AudioEffectParambin.flow_index;
    uint16_t original_param = AudioEffectParambin.param_mode_index;

    // 循环查找符合条件的模式
    do {
        // 基础索引递增逻辑
        if (AudioEffectParambin.param_mode_index >= (AudioEffect_Parambin_GetTotalParamNumInFlow(AudioEffectParambin.flow_index) - 1))
        {
            if (AudioEffectParambin.flow_index >= (max_flows - 1))
            {
                flow_target = 0;
                param_target = 0;
            }
            else
            {
                flow_target = AudioEffectParambin.flow_index + 1;
                param_target = 0;
            }
        }
        else
        {
            flow_target = AudioEffectParambin.flow_index;
            param_target = AudioEffectParambin.param_mode_index + 1;
        }

        // 检查是否为hfp模式
        const char *flow_name = (char *)AudioEffect_Parambin_GetFlowNameByIndex(flow_target);
        if (flow_name && (strcmp("hfp", flow_name) == 0))
        {
            // 保存当前索引并尝试下一个
            AudioEffectParambin.flow_index = flow_target;
            AudioEffectParambin.param_mode_index = param_target;

            // 边界检查：如果遍历完所有模式仍未找到有效值
            if (AudioEffectParambin.flow_index == original_flow &&
                AudioEffectParambin.param_mode_index == original_param)
            {
                APP_DBG("All flows are hfp, no valid mode found!\n");
                break;
            }
        }
        else
        {
            found_valid_flow = TRUE;
            break;
        }
    } while (1);

    // 只有找到有效值才执行切换
    if (found_valid_flow)
    {
        AudioEffect_Parambin_SwitchEffectModeByIndex(flow_target, param_target);
    }
    else
    {
        // 恢复原始状态
        AudioEffectParambin.flow_index = original_flow;
        AudioEffectParambin.param_mode_index = original_param;
    }

	AudioEffect_EffectMode_Refresh();
}

void AudioEffect_EffectMode_Refresh(void)
{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}

	PauseAuidoCore();
//	AudioEffect_Parambin_Deinit();
	AudioEffect_Parambin_Init();

	AudioEffect_Parambin_RefreshSourceSinkBuf();

	SoftFlagDeregister(SoftFlagAudioCoreSourceIsDeInit);
	DefaultParamgsInit();
	AudioCodecGainUpdata();//update hardware config

	////Audio Core & Audioeffect音量配置
	AudioEffect_Params_Sync();

	AudioCoreServiceResume();

//	gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;

	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}

#ifdef CFG_FUNC_BREAKPOINT_EN
	BackupInfoUpdata(BACKUP_SYS_INFO);
#endif
}

void AudioEffect_Params_Sync(void)
{
	if(AudioEffectParambin.context_memory == NULL)
		return;

	AudioAPPDigitalGianProcess(mainAppCt.SysCurrentMode);
	SystemVolSet();
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	AudioEffect_EqMode_Set(mainAppCt.EqMode);
#endif

//	gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;

#ifdef CFG_FUNC_BREAKPOINT_EN
	BackupInfoUpdata(BACKUP_SYS_INFO);
#endif
}

uint8_t AudioEffect_EqMode_Set(uint8_t EqMode)
{
	int16_t ret;

	switch(EqMode)
	{
		case EQ_MODE_FLAT:
			ret = AudioEffect_Parambin_EffectAllParams_Set(EFFECT_MUSICEQ, (int16_t *)AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_FLAT));
		break;
		case EQ_MODE_CLASSIC:
			ret = AudioEffect_Parambin_EffectAllParams_Set(EFFECT_MUSICEQ, (int16_t *)AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_CLASSIC));
		break;
		case EQ_MODE_POP:
			ret = AudioEffect_Parambin_EffectAllParams_Set(EFFECT_MUSICEQ, (int16_t *)AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_POP));
		break;
		case EQ_MODE_ROCK:
			ret = AudioEffect_Parambin_EffectAllParams_Set(EFFECT_MUSICEQ, (int16_t *)AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_ROCK));
		break;
		case EQ_MODE_JAZZ:
			ret = AudioEffect_Parambin_EffectAllParams_Set(EFFECT_MUSICEQ, (int16_t *)AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_JAZZ));
		break;
		case EQ_MODE_VOCAL_BOOST:
			ret = AudioEffect_Parambin_EffectAllParams_Set(EFFECT_MUSICEQ, (int16_t *)AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_VOCALBOOST));
		break;
		default:
			break;
	}

	if(ret != ROBOEFFECT_ERROR_OK)
		APP_DBG("AudioEffect_EqMode_Set failed!:%d\n", ret);

	return AudioEffect_Parambin_GetEffectAddr(EFFECT_MUSICEQ);
}

uint8_t AudioEffect_effect_status_Get(AUDIO_EFFECT_NAME_NUM effect_index)
{
	if(AudioEffectParambin.context_memory == NULL)
		return 0;
	if(!AudioEffect_Parambin_GetEffectAddr(effect_index))
		return 0;

	return roboeffect_get_effect_status(AudioEffectParambin.context_memory, AudioEffect_Parambin_GetEffectAddr(effect_index));
};

void AudioEffect_effect_enable(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t enable)
{
	if(AudioEffectParambin.context_memory == NULL)
		return;
	if(!AudioEffect_Parambin_GetEffectAddr(effect_index))
		return;

	AudioEffect_Parambin_UpdateTempEffectParamsByAddr(AudioEffect_Parambin_GetEffectAddr(effect_index), 0, &enable);
	AudioEffect_Parambin_UpdateFlashEffectParams();

	AudioEffect_EffectMode_Refresh();
	gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(effect_index);
};

void AudioEffect_Msg_Process(uint32_t Msg)
{
	switch(Msg)
	{
		case MSG_MUSIC_VOLUP:
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(EFFECT_MUSICGAIN)) != 0)
			{
				AudioMusicVolUp();
			}
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
			break;
		case MSG_MUSIC_VOLDOWN:
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(EFFECT_MUSICGAIN)) != 0)
			{
				AudioMusicVolDown();
			}
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
			break;
#if CFG_RES_MIC_SELECT
		case MSG_MIC_VOLUP:
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(EFFECT_MICGAIN)) != 0)
			{
				AudioMicVolUp();
			}
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
			break;
		case MSG_MIC_VOLDOWN:
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(EFFECT_MICGAIN)) != 0)
			{
				AudioMicVolDown();
			}
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
			break;
#endif
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
		case MSG_EQ:
			APP_DBG("MSG_EQ\n");
			mainAppCt.EqMode = (++mainAppCt.EqMode) > EQ_MODE_VOCAL_BOOST ? EQ_MODE_FLAT:mainAppCt.EqMode;
			gCtrlVars.AutoRefresh = AudioEffect_EqMode_Set(mainAppCt.EqMode);
		#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
		#endif
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
			break;
#endif
		case MSG_MIC_FIRST:
			APP_DBG("MSG_MIC_FIRST\n");
			#ifdef CFG_FUNC_SHUNNING_EN
			mainAppCt.ShunningMode = !mainAppCt.ShunningMode;
			APP_DBG("ShunningMode = %d\n", mainAppCt.ShunningMode);
			#endif
			break;
		default:
			break;
	}
}

bool AudioEffect_Msg_Check(uint32_t Msg)
{
	if(Msg > MSG_EFFECTCONTROL_START && Msg < MSG_EFFECTCONTROL_END)
	{
		MessageContext msgSend;
		msgSend.msgId = Msg;
		MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
		return TRUE;
	}
	return FALSE;
}

void AudioEffect_UpdateSamplerateFramesize(uint32_t sample_rate, uint32_t frame_size)
{
	AudioEffectParambin.user_effect_list->sample_rate = sample_rate;
	AudioEffectParambin.user_effect_list->frame_size = frame_size;

	AudioEffect_EffectMode_Refresh();
}

uint32_t AudioEffect_GetUserEffectParamCRCLen(uint8_t *effect_param)
{
	uint8_t *params = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(AudioEffectParambin.user_effect_parameters);
	uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(AudioEffectParambin.user_effect_parameters);
	uint32_t crcLen = 0;

	for(uint32_t i = 0; i < data_len; i++)
	{
		crcLen += params[i];
	}

	return crcLen;
}

uint32_t AudioEffect_GetUserHWCfgCRCLen(uint8_t *effect_param)
{
	uint32_t crcLen = 0;
	uint32_t paramsLen = sizeof(gCtrlVars.HwCt);

	for(uint32_t i = 0; i < paramsLen; i++)
	{
		crcLen += effect_param[i];
	}

	return crcLen;
}
