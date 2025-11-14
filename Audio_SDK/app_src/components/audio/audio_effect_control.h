/**
 **************************************************************************************
 * @file    audio_effect_control.h
 * @brief   Audio Effect control API
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_EFFECT_CONTROL_H__
#define __AUDIO_EFFECT_CONTROL_H__


typedef enum _EQ_MODE
{
    EQ_MODE_FLAT,
	EQ_MODE_CLASSIC,
	EQ_MODE_POP,
	EQ_MODE_ROCK,
	EQ_MODE_JAZZ,
	EQ_MODE_VOCAL_BOOST,
} EQ_MODE;

void AudioEffect_Parambin_EffectModeAutoSwitch(void);

void AudioEffect_EffectMode_Refresh(void);

void AudioEffect_Params_Sync(void);

uint8_t AudioEffect_EqMode_Set(uint8_t EqMode);

uint8_t AudioEffect_effect_status_Get(AUDIO_EFFECT_NAME_NUM effect_index);

void AudioEffect_effect_enable(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t enable);

void AudioEffect_Msg_Process(uint32_t Msg);

bool AudioEffect_Msg_Check(uint32_t Msg);

void AudioEffect_UpdateSamplerateFramesize(uint32_t sample_rate, uint32_t frame_size);

uint32_t AudioEffect_GetUserEffectParamCRCLen(uint8_t *effect_param);

uint32_t AudioEffect_GetUserHWCfgCRCLen(uint8_t *effect_param);
#endif //__AUDIO_EFFECT_CONTROL_H__
