/**
 **************************************************************************************
 * @file    audio_effect_parambin.h
 * @brief   Audio effect ctrl for Roboeffect param bin
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_EFFECT_PARAM_BIN_H__
#define __AUDIO_EFFECT_PARAM_BIN_H__
#include "roboeffect_api.h"
#include "effects_param_v3.h"
#include "core_d1088.h"


/**************************** source + sink + effect_name start*********************************************/
#define AUDIO_SOURCE_ENUM_MAP(XX) \
    XX(MIC_SOURCE_NUM,            "MIC_SOURCE") \
    XX(APP_SOURCE_NUM,            "APP_SOURCE") \
    XX(REMIND_SOURCE_NUM,         "REMIND_SOURCE") \
    XX(PLAYBACK_SOURCE_NUM,       "REC_SOURCE") \
    XX(I2S_MIX_SOURCE_NUM,        "I2S_MIX_SOURCE") \
    XX(I2S_MIX2_SOURCE_NUM,       "I2S_MIX2_SOURCE") \
    XX(USB_SOURCE_NUM,            "USB_SOURCE") \
    XX(LINEIN_MIX_SOURCE_NUM,     "LINEIN_MIX_SOURCE") \

#define AUDIO_SINK_ENUM_MAP(XX) \
    XX(AUDIO_DAC0_SINK_NUM,          "DAC0_SINK") \
    XX(AUDIO_RECORDER_SINK_NUM,      "REC_SINK") \
    XX(AUDIO_APP_SINK_NUM,           "APP_SINK") \
    XX(AUDIO_STEREO_SINK_NUM,        "STEREO_SINK") \
    XX(AUDIO_I2S_MIX_OUT_SINK_NUM,   "I2S_MIX_SINK") \
    XX(AUDIO_SPDIF_SINK_NUM,         "SPDIF_SINK") \
    XX(AUDIO_BT_SOURCE_SINK_NUM,     "BT_SOURCE_SINK") \

typedef enum _AUDIO_CORE_SOURCE_NUM {
    #define XX_ENUM(name, str) name,
    AUDIO_SOURCE_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
    AUDIO_CORE_SOURCE_MAX_NUM
} AUDIO_CORE_SOURCE_NUM;
typedef enum _AUDIO_CORE_SINK_NUM {
    #define XX_ENUM(name, str) name,
    AUDIO_SINK_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
    AUDIO_CORE_SINK_MAX_NUM
} AUDIO_CORE_SINK_NUM;

#define AUDIO_EFFECT_NAME_ENUM_MAP(XX) \
    XX(EFFECT_MUSICGAIN,		"music_gain") \
    XX(EFFECT_MICGAIN,			"mic_gain") \
    XX(EFFECT_PREGAIN,			"preGain") \
    XX(EFFECT_MUSICEQ,			"music_EQ") \
    XX(EFFECT_MICEQ,			"mic_EQ") \
    XX(EFFECT_MUSICSNDET,		"silence_detector_music") \
    XX(EFFECT_MICSNDET,			"silence_detector_mic")

typedef enum _AUDIO_EFFECT_NAME_NUM {
    #define XX_ENUM(name, str) name,
	AUDIO_EFFECT_NAME_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
	EFFECT_MAX
} AUDIO_EFFECT_NAME_NUM;

#define AUDIO_EFFECT_PRESET_ENUM_MAP(XX) \
    XX(PRESET_EQ_FLAT,				"FLAT") \
    XX(PRESET_EQ_CLASSIC,			"CLASSIC") \
    XX(PRESET_EQ_POP,				"POP") \
    XX(PRESET_EQ_ROCK,				"ROCK") \
    XX(PRESET_EQ_JAZZ,				"JAZZ") \
    XX(PRESET_EQ_VOCALBOOST,		"VOCAL_BOOST") \
    XX(PRESET_GAIN_VOL,				"VOL")

typedef enum _AUDIO_EFFECT_PRESET_NUM {
    #define XX_ENUM(name, str) name,
	AUDIO_EFFECT_PRESET_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
	PRESET_MAX
} AUDIO_EFFECT_PRESET_NUM;

typedef struct {
    int enum_val;
    const char *enum_str;
} EnumStringMap;
/**************************** source + sink + effect_name end*********************************************/

//total data length  	---- 2 Bytes
//Effect Version		---- 3 Bytes
//Roboeffect Version  	---- 3 Bytes
//3rd part data length	---- 2 Bytes
//3rd part data
//MV effect data
#define GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(p) 	((uint16_t)(((uint16_t)((p)[1] << 8) | (p)[0]) + 2))
#define GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(p) ((uint16_t)((uint16_t)((p)[9] << 8) | (p)[8]))
#define GET_USER_EFFECT_PARAMETERS_DATA_LEN(p)		(GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(p) - 8 - GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(p) - 2)
#define GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(p) 	((uint8_t*)((p) + 8 + GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(p) + 2))

typedef struct _AudioEffectParambinContext
{
	uint8_t *context_memory;
	roboeffect_effect_list_info *user_effect_list;
	roboeffect_exec_effect_info *user_effect_list_arry;
	const roboeffect_effect_steps_table *user_effect_steps;
	unsigned char *user_effect_parameters;
	const unsigned char *user_codec_parameters;
	const unsigned char *user_flow_script;
	const roboeffect_io_unit *source_io;
	const roboeffect_io_unit *sink_io;
	uint32_t user_flow_script_len;
	const uint32_t *steps_flow;
	uint8_t *sub_type_data;
	uint8_t *flow_data;
	uint32_t flow_size;
	uint16_t flow_index;
	uint16_t param_mode_index;
}AudioEffectParambinContext;

extern AudioEffectParambinContext AudioEffectParambin;

typedef struct _AudioEffect_parambin_mode1_brief
{
	uint16_t item_type;
	uint16_t item_len;
	uint16_t mode_type_cnt;
	uint16_t preset_type_cnt;
	uint32_t base_address;
} AudioEffect_parambin_mode1_brief;

uint32_t AudioEffect_Parambin_GetFlashEffectAddr(void);

void *AudioEffect_Parambin_GetSourceBuffer(AUDIO_CORE_SOURCE_NUM source_index);

void *AudioEffect_Parambin_GetSinkBuffer(AUDIO_CORE_SINK_NUM sink_index);

int8_t AudioEffect_Parambin_GetSourceEnum(const char *str);

int8_t AudioEffect_Parambin_GetSinkEnum(const char *str);

uint16_t AudioEffect_Parambin_GetTotalFlowNum(void);

uint16_t AudioEffect_Parambin_GetTotalParamNumInFlow(uint16_t flow_index);

bool AudioEffect_Parambin_Init(void);

void AudioEffect_Parambin_Deinit(void);

const void *AudioEffect_Parambin_GetCurHardwareConfig(void);

const void *AudioEffect_Parambin_GetCurEffectParams(void);

void AudioEffect_Parambin_RefreshSourceSinkBuf(void);

uint8_t *AudioEffect_Parambin_GetTempEffectParamByAddr(uint8_t effect_addr);

uint8_t *AudioEffect_Parambin_GetFlashEffectParamByAddr(uint8_t effect_addr);

void AudioEffect_Parambin_UpdateFlashEffectParamsByAddr(uint8_t effect_addr, uint8_t param_index, uint8_t *param_input);

void AudioEffect_Parambin_UpdateFlashEffectParams(void);

void AudioEffect_Parambin_UpdateTempEffectParamsByAddr(uint8_t effect_addr, uint8_t param_index, uint8_t *param_input);

void AudioEffect_Parambin_SwitchEffectModeByIndex(uint16_t flow_index, uint16_t param_mode_index);

void AudioEffect_Parambin_SwitchEffectModeByName(const char *mode_name);

uint8_t *AudioEffect_Parambin_GetFlowNameByIndex(uint16_t flow_index);

uint8_t AudioEffect_Parambin_GetEffectAddr(AUDIO_EFFECT_NAME_NUM effect_index);

ROBOEFFECT_ERROR_CODE AudioEffect_Parambin_EffectParam_Set(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index, int16_t param);

ROBOEFFECT_ERROR_CODE AudioEffect_Parambin_EffectAllParams_Set(AUDIO_EFFECT_NAME_NUM effect_index, int16_t *param);

int16_t AudioEffect_Parambin_EffectParam_Get(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index);

int16_t* AudioEffect_Parambin_EffectAllParams_Get(AUDIO_EFFECT_NAME_NUM effect_index);

int16_t* AudioEffect_Parambin_PreSetEffectParam_Get(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index, AUDIO_EFFECT_PRESET_NUM preset_index, uint32_t *preset_param_size);

int16_t* AudioEffect_Parambin_PreSetEffectAllParams_Get(AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index);

bool AudioEffect_Parambin_EffectModeIsRunning(uint16_t flow_index, uint16_t param_mode_index);
#endif //__AUDIO_EFFECT_PARAM_BIN_H__
