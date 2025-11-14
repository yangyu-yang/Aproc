/**
 **************************************************************************************
 * @file    audio_effect_parambin.c
 * @brief   Audio effect ctrl for Roboeffect param bin
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <nds32_utils_math.h>
#include <string.h>
#include "flash_table.h"
#include "debug.h"
#include "audio_effect_parambin.h"
#include "audio_core_adapt.h"
#include "rtos_api.h"
#include "breakpoint.h"
#include "main_task.h"
#include "roboeffect_prot.h"


static const EnumStringMap audio_source_map[] = {
    #define XX_MAP(name, str) {name, str},
    AUDIO_SOURCE_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};
static const EnumStringMap audio_sink_map[] = {
    #define XX_MAP(name, str) {name, str},
    AUDIO_SINK_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};

static const EnumStringMap audio_effect_name_map[] = {
    #define XX_MAP(name, str) {name, str},
	AUDIO_EFFECT_NAME_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};

static const EnumStringMap audio_effect_preset_map[] = {
    #define XX_MAP(name, str) {name, str},
	AUDIO_EFFECT_PRESET_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};

static char *AudioEffect_err_str[] = {
	"ROBOEFFECT_EFFECT_NOT_EXISTED",
	"ROBOEFFECT_EFFECT_PARAMS_NOT_FOUND",
	"ROBOEFFECT_INSUFFICIENT_MEMORY",
	"ROBOEFFECT_EFFECT_INIT_FAILED",
	"ROBOEFFECT_ILLEGAL_OPERATION",
	"ROBOEFFECT_EFFECT_LIB_NOT_MATCH_1",
	"ROBOEFFECT_EFFECT_LIB_NOT_MATCH_2",
	"ROBOEFFECT_ADDRESS_NOT_EXISTED",
	"ROBOEFFECT_PARAMS_ERROR",
	"ROBOEFFECT_FRAME_SIZE_ERROR",
	"ROBOEFFECT_MEMORY_SIZE_QUERY_ERROR",
	"ROBOEFFECT_EFFECT_VER_NOT_MATCH_ERROR",
	"ROBOEFFECT_LIB_VER_NOT_MATCH_ERROR",
	"ROBOEFFECT_3RD_PARTY_LIB_NOT_MATCH_ERROR",
	"ROBOEFFECT_PARAMBIN_ERROR",
	"ROBOEFFECT_CONTEXT_MEMORY_ERROR",
	"ROBOEFFECT_PARAMBIN_DATA_NOT_FOUND",
	"ROBOEFFECT_PARAMBIN_DATA_VER_ERROR",
};

AudioEffectParambinContext AudioEffectParambin;


static int get_enum_value(const char *str, const EnumStringMap *map, int size) {
    for (int i = 0; i < size; i++) {
        if (strcmp(map[i].enum_str, str) == 0) {
            return map[i].enum_val;
        }
    }
    return -1;
}

static bool AudioEffect_Parambin_Check(void)
{
	uint32_t AUDIOEFFECT_FLASHBIN_ADDRESS = AudioEffect_Parambin_GetFlashEffectAddr();

	if(AudioEffectParambin.user_effect_parameters)
	{
		osPortFree(AudioEffectParambin.user_effect_parameters);
		AudioEffectParambin.user_effect_parameters = NULL;
	}
	if(AudioEffectParambin.user_effect_list)
	{
		osPortFree(AudioEffectParambin.user_effect_list);
		AudioEffectParambin.user_effect_list = NULL;
	}

	if(roboeffect_parambin_check_whole_bin((uint8_t *)AUDIOEFFECT_FLASHBIN_ADDRESS, NULL) == ROBOEFFECT_ERROR_OK)
	{
		uint32_t param_size = 0;
//		if(need_switch_mode & SWITCH_MODE_PARAMBIN_MODE)//! force get flow and mode index from flashbin
//		{
//			roboeffect_parambin_flow_pair *pair = roboeffect_parambin_get_current_flow(AUDIOEFFECT_FLASHBIN_ADDRESS, NULL);
//			//* DU has only one pair of index
//			AudioEffectParambin.flow_index = pair[0].flow_index;
//			AudioEffectParambin.param_mode_index = pair[0].param_mode_index;
//		}
//		 APP_DBG("FLOW: %d, %d\n", AudioEffectParambin.flow_index, AudioEffectParambin.param_mode_index);

		if((AudioEffectParambin.flow_data = roboeffect_parambin_get_flow_by_index((uint8_t *)AUDIOEFFECT_FLASHBIN_ADDRESS, AudioEffectParambin.flow_index, &AudioEffectParambin.flow_size)) != NULL)
		{
			uint8_t *temp_data, *mode_data_out = NULL, *codec_data_out = NULL;
			// uint16_t temp_size;
			temp_data = roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_SCRIPT, FALSE, NULL);
			AudioEffectParambin.user_flow_script = temp_data + 4 + 4;
			AudioEffectParambin.user_flow_script_len = *((uint32_t*)(temp_data + 4)) - 2;//-2 is crc bytes

//			AudioEffectParambin.user_effect_list = (roboeffect_effect_list_info *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_EFFECTS_INFO, TRUE, NULL);
			AudioEffectParambin.user_effect_list = osPortMalloc(sizeof(roboeffect_effect_list_info));
			memcpy(AudioEffectParambin.user_effect_list,
					roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_EFFECTS_INFO, TRUE, NULL),
					sizeof(roboeffect_effect_list_info));
			AudioEffectParambin.user_effect_list_arry = (roboeffect_exec_effect_info *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_EFFECTS_LIST, TRUE, NULL);

			AudioEffectParambin.user_effect_steps = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);

			AudioEffectParambin.sub_type_data = roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);
			roboeffect_parambin_get_mode_data_by_index(AudioEffectParambin.sub_type_data, AudioEffectParambin.param_mode_index, &mode_data_out, &param_size, &codec_data_out, NULL);
//			AudioEffectParambin.user_effect_parameters = mode_data_out;
			AudioEffectParambin.user_effect_parameters = osPortMalloc(GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(mode_data_out) * sizeof(uint8_t));
			memcpy(AudioEffectParambin.user_effect_parameters, mode_data_out, GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(mode_data_out) * sizeof(uint8_t));
			if(codec_data_out)
				AudioEffectParambin.user_codec_parameters = codec_data_out;
			// temp_size = param_hdr->codec_param_len;

			AudioEffectParambin.source_io = (roboeffect_io_unit *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_SOURCE, TRUE, NULL);
			AudioEffectParambin.sink_io = (roboeffect_io_unit *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_SINK, TRUE, NULL);
			AudioEffectParambin.steps_flow = (uint32_t *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin.flow_data, AudioEffectParambin.flow_size, ROBO_PB_SUBTYPE_STEPS, TRUE, NULL);

			if(AudioEffectParambin.user_flow_script && AudioEffectParambin.user_effect_list && AudioEffectParambin.user_effect_steps
					&& AudioEffectParambin.sub_type_data && mode_data_out && AudioEffectParambin.source_io && AudioEffectParambin.sink_io)
			{
//				APP_DBG("[AudioEffect_Parambin] AudioEffect_Parambin_Check ok!\n");
				return TRUE;
			}
			else
			{
				APP_DBG("[AudioEffect_Parambin] WARNING! AudioEffect_Parambin_Check have null pointer!\n");
			}
		}
	}
	else
	{
		APP_DBG("[AudioEffect_Parambin] ERROR! AudioEffect_Parambin_Check failed!\n");
	}
	return FALSE;
}

uint32_t AudioEffect_Parambin_GetFlashEffectAddr(void)
{
	return get_effect_data_addr();
}

void *AudioEffect_Parambin_GetSourceBuffer(AUDIO_CORE_SOURCE_NUM source_index)
{
	if((AudioEffectParambin.context_memory == NULL) || (source_index < 0 || source_index >= AUDIO_CORE_SOURCE_MAX_NUM))
		return NULL;

	const roboeffect_io_unit *device_node = AudioEffectParambin.source_io;

	if(((device_node = roboeffect_parambin_get_io_by_name(AudioEffectParambin.context_memory, audio_source_map[source_index].enum_str)) != NULL) && IO_UNIT_VALID(device_node))
		return roboeffect_get_source_buffer(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node));
//	for(uint8_t i = 0; i < AudioEffectParambin.user_effect_steps->src_unit_num; i++, device_node++)
//	{
////		if(IS_IO_MATCH(main_context, audio_source_map[source_index], device_node))
//		if(strcmp((audio_source_map[source_index].enum_str), roboeffect_parambin_get_io_name(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node))) == 0 && IO_UNIT_VALID(device_node))
//			return roboeffect_get_source_buffer(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node));
//	}
//	APP_DBG("[AudioEffect_Parambin] ERROR! source %d not found buffer\n", source_index);
	return NULL;
}

void *AudioEffect_Parambin_GetSinkBuffer(AUDIO_CORE_SINK_NUM sink_index)
{
	if((AudioEffectParambin.context_memory == NULL) || (sink_index < 0 || sink_index >= AUDIO_CORE_SINK_MAX_NUM))
		return NULL;

	const roboeffect_io_unit *device_node = AudioEffectParambin.sink_io;

	if(((device_node = roboeffect_parambin_get_io_by_name(AudioEffectParambin.context_memory, audio_sink_map[sink_index].enum_str)) != NULL) && IO_UNIT_VALID(device_node))
		return roboeffect_get_sink_buffer(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node));
//	for(uint8_t i = 0; i < AudioEffectParambin.user_effect_steps->des_unit_num; i++, device_node++)
//	{
////		if(IS_IO_MATCH(main_context, audio_sink_map[sink_index], device_node))
//		if(strcmp((audio_sink_map[sink_index].enum_str), roboeffect_parambin_get_io_name(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node))) == 0 && IO_UNIT_VALID(device_node))
//			return roboeffect_get_sink_buffer(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node));
//	}
//	APP_DBG("[AudioEffect_Parambin] ERROR! sink %d not found buffer\n", sink_index);
	return NULL;
}

int8_t AudioEffect_Parambin_GetSourceEnum(const char *str)
{
	int8_t size = sizeof(audio_source_map) / sizeof(EnumStringMap);
    return (AUDIO_CORE_SOURCE_NUM)get_enum_value(str, audio_source_map, size);
}

int8_t AudioEffect_Parambin_GetSinkEnum(const char *str)
{
	int8_t size = sizeof(audio_sink_map) / sizeof(EnumStringMap);
    return (AUDIO_CORE_SINK_NUM)get_enum_value(str, audio_sink_map, size);
}

uint16_t AudioEffect_Parambin_GetTotalFlowNum(void)
{
	roboeffect_parambin_header *hdr_ptr = (roboeffect_parambin_header *)AudioEffect_Parambin_GetFlashEffectAddr();

	if(hdr_ptr != NULL)
	{
		return hdr_ptr->flow_cnt;
	}
	return 0;
}

uint16_t AudioEffect_Parambin_GetTotalParamNumInFlow(uint16_t flow_index)
{
	uint8_t *flow_data;
	uint32_t flow_size = 0;

	if((flow_data = roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), flow_index, &flow_size)) != NULL)
	{
		AudioEffect_parambin_mode1_brief *sub_type_data = (AudioEffect_parambin_mode1_brief *)roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);

		if(sub_type_data != NULL)
		{
			return sub_type_data->mode_type_cnt;
		}
	}
	return 0;
}

bool AudioEffect_Parambin_Init(void)
{
	int32_t audioeffect_memory_size = 0;
	ROBOEFFECT_ERROR_CODE audioeffect_ret;

	if(AudioEffectParambin.context_memory)
	{
		osPortFree(AudioEffectParambin.context_memory);
		AudioEffectParambin.context_memory = NULL;
	}

	roboeffect_prot_init();
	DataCacheInvalidAll();

	if(!AudioEffect_Parambin_EffectModeIsRunning(AudioEffectParambin.flow_index, AudioEffectParambin.param_mode_index))//just refresh but not init all
	{
		//* check flashbin and set global pointers.
		AudioEffect_Parambin_Check();
	}

	audioeffect_memory_size = roboeffect_parambin_estimate_memory_size(AudioEffectParambin.flow_data, AudioEffectParambin.param_mode_index, AudioEffectParambin.user_effect_list);
	APP_DBG("flow : %s mode:%d memory_size:%ld\n", AudioEffectParambin.flow_data, AudioEffectParambin.param_mode_index, audioeffect_memory_size);
	if(audioeffect_memory_size < 0)
	{
		APP_DBG("Get context size failed, Error: %s\n", AudioEffect_err_str[audioeffect_memory_size+256]);
		return FALSE;
	}
	if((AudioEffectParambin.context_memory = osPortMallocFromEnd(audioeffect_memory_size)) == NULL)
	{
		APP_DBG("context_memory is NULL\n");
		return FALSE;
	}

	if(ROBOEFFECT_ERROR_OK != (audioeffect_ret = roboeffect_parambin_init(AudioEffectParambin.context_memory, audioeffect_memory_size,
			AudioEffectParambin.flow_data, AudioEffectParambin.param_mode_index, AudioEffectParambin.user_effect_list)) )
	{
		APP_DBG("roboeffect_init failed. Error: %s\n", AudioEffect_err_str[audioeffect_ret+256]);
		osPortFree(AudioEffectParambin.context_memory);
		AudioEffectParambin.context_memory = NULL;
		return FALSE;
	}

	//After effect init done, AudioCore know what frame size should be set.
	for(uint8_t net = 0; net < MaxNet; net++)
	{
		AudioCoreFrameSizeSet(net, roboeffect_parambin_estimate_frame_size(AudioEffectParambin.flow_data, AudioEffectParambin.param_mode_index, AudioEffectParambin.user_effect_list));
		AudioCoreMixSampleRateSet(net, AudioEffectParambin.user_effect_list->sample_rate);
		APP_DBG("set net %d samplerate:%ld, framesize:%d\n", net , AudioCoreMixSampleRateGet(net), AudioCoreFrameSizeGet(net));
	}

	APP_DBG("Roboeffect init done!\n");
	/*********************************************************debug info**********************************************************************/
//	APP_DBG("list: %d, %d\n", AudioEffectParambin.user_effect_list->sample_rate, AudioEffectParambin.user_effect_list->frame_size);
//	APP_DBG("steps: %d, %d, %d, %d\n", AudioEffectParambin.user_effect_steps->step_num, AudioEffectParambin.user_effect_steps->mem_used,
//			AudioEffectParambin.user_effect_steps->src_unit_num, AudioEffectParambin.user_effect_steps->des_unit_num);
//	const roboeffect_io_unit *device_node = AudioEffectParambin.source_io;
//	for(uint8_t i = 0; i < AudioEffectParambin.user_effect_steps->src_unit_num; i++, device_node++)
//	{
//		if(IO_UNIT_VALID(device_node))
//			APP_DBG("source %d, %s\n", i, roboeffect_parambin_get_io_name(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node)));
//	}
//	device_node = AudioEffectParambin.sink_io;
//	for(uint8_t i = 0; i < AudioEffectParambin.user_effect_steps->des_unit_num; i++, device_node++)
//	{
//		if(IO_UNIT_VALID(device_node))
//			APP_DBG("sink %d, %s\n", i, roboeffect_parambin_get_io_name(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node)));
//	}

//	for(uint8_t i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
//	{
//		if(AudioEffect_Parambin_GetSourceBuffer(i))
//			APP_DBG("source %d buf ok\n", i);
//	}
//	for(uint8_t i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
//	{
//		if(AudioEffect_Parambin_GetSinkBuffer(i))
//			APP_DBG("sink %d buf ok\n", i);
//	}
//	uint8_t *Hwct = AudioEffect_Parambin_GetCurHardwareConfig();
//	for(uint8_t i = 0; i < 200;)
//	{
//		APP_DBG("0x%x, 0x%x\n", Hwct[i++], Hwct[i++]);
//	}
//	for(uint16_t i = 0; i < AudioEffect_Parambin_GetTotalFlowNum(); i++)
//	{
//		APP_DBG("flow %d: %s, param:%d\n", i, AudioEffect_Parambin_GetFlowNameByIndex(i), AudioEffect_Parambin_GetTotalParamNumInFlow(i));
//	}
//	uint32_t *passwd = roboeffect_parambin_get_password(AudioEffect_Parambin_GetFlashEffectAddr());
//	APP_DBG("password:0x%x\n", passwd[0]);
//	APP_DBG("addr preGain:0x%x|0x%x, music_EQ:0x%x|0x%x\n", roboeffect_parambin_get_addr_by_name(AudioEffectParambin.context_memory, "preGain"),
//														AudioEffect_Parambin_GetEffectAddr(EFFECT_PREGAIN),
//														roboeffect_parambin_get_addr_by_name(AudioEffectParambin.context_memory, "music_EQ"),
//														AudioEffect_Parambin_GetEffectAddr(EFFECT_MUSICEQ));
//	int16_t *param;
//	if((param = AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_FLAT)) != NULL)
//	{
//		for(uint8_t i = 0; i < 50;)
//		{
//			APP_DBG("EQ i:%d---0x%x\n", i, param[i++]);
//		}
//	}
//	uint32_t step;
//	if((param = AudioEffect_Parambin_PreSetEffectParam_Get(EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, PRESET_GAIN_VOL, &step)) != NULL)
//	{
//		APP_DBG("step:%d\n", step);
//		for(uint8_t i = 0; i < 32; i++)
//		{
//			APP_DBG("VOL i:%d---%d\n", i, param[i]);
//		}
//	}
//	uint8_t *mode_data_out;
//	uint32_t param_size;
//	int16_t parambin_check = roboeffect_parambin_get_preset_param_by_name(AudioEffectParambin.sub_type_data, 0x88, 0xff, "FLAT", &mode_data_out, &param_size);
//	if(parambin_check == ROBOEFFECT_ERROR_OK)
//	{
//		for(int i = 0; i < param_size/2; i++)
//		{
//			printf("EQ %d-->0x%x\n", i, ((int16_t*)mode_data_out)[i]);
//		}
//	}
//	parambin_check = roboeffect_parambin_get_preset_param_by_name(AudioEffectParambin.sub_type_data, 0x8B, 0x01, "VOL", &mode_data_out, &param_size);
//	if(parambin_check == ROBOEFFECT_ERROR_OK)
//	{
//		for(int i = 0; i < param_size/2; i++)
//		{
//			printf("VOL %d-->%d\n", i, ((int16_t*)mode_data_out)[i]);
//		}
//	}
	/*********************************************************debug info**********************************************************************/
	return TRUE;
}

void AudioEffect_Parambin_Deinit(void)
{
	if(AudioEffectParambin.context_memory)
	{
		osPortFree(AudioEffectParambin.context_memory);
		AudioEffectParambin.context_memory = NULL;
	}
	if(AudioEffectParambin.user_effect_parameters)
	{
		osPortFree(AudioEffectParambin.user_effect_parameters);
		AudioEffectParambin.user_effect_parameters = NULL;
	}
	if(AudioEffectParambin.user_effect_list)
	{
		osPortFree(AudioEffectParambin.user_effect_list);
		AudioEffectParambin.user_effect_list = NULL;
	}
	memset(&AudioEffectParambin, 0, sizeof(AudioEffectParambinContext));
}

const void *AudioEffect_Parambin_GetCurHardwareConfig(void)
{
	if(AudioEffectParambin.user_codec_parameters == NULL)
		return NULL;
	return AudioEffectParambin.user_codec_parameters;
}

const void *AudioEffect_Parambin_GetCurEffectParams(void)
{
	if(AudioEffectParambin.user_effect_parameters == NULL)
		return NULL;
	return AudioEffectParambin.user_effect_parameters;
}

void AudioEffect_Parambin_RefreshSourceSinkBuf(void)
{
	for(uint8_t source_index = 0; source_index < AUDIO_CORE_SOURCE_MAX_NUM; source_index++)
	{
		if(AudioEffect_Parambin_GetSourceBuffer(source_index))
		{
			AudioCore.AudioSource[source_index].PcmInBuf = AudioEffect_Parambin_GetSourceBuffer(source_index);
		}
	}
	for(uint8_t sink_index = 0; sink_index < AUDIO_CORE_SINK_MAX_NUM; sink_index++)
	{
		if(AudioEffect_Parambin_GetSinkBuffer(sink_index))
		{
			AudioCore.AudioSink[sink_index].PcmOutBuf = AudioEffect_Parambin_GetSinkBuffer(sink_index);
		}
	}
}

uint8_t *AudioEffect_Parambin_GetFlashEffectParam(uint8_t effect_addr)
{
    if (AudioEffectParambin.user_effect_parameters == NULL)
    {
        return NULL;
    }

//    uint16_t total_data_len = GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(AudioEffectParambin.user_effect_parameters);
//    uint16_t third_data_len = GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(AudioEffectParambin.user_effect_parameters);
    uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(AudioEffectParambin.user_effect_parameters);
    uint8_t *effectParams = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(AudioEffectParambin.user_effect_parameters);
    uint16_t remaining = data_len;

    while (remaining > 0)
    {
        if (effectParams[0] == effect_addr)
        {
            return effectParams;
        } else {
            uint8_t len = effectParams[1];
            if (len + 2 > remaining)
            {
                break;
            }
            effectParams += (len + 2);
            remaining -= (len + 2);
        }
    }
    return NULL;
}

uint8_t *AudioEffect_Parambin_GetTempEffectParamByAddr(uint8_t effect_addr)
{
    if (AudioEffectParambin.user_effect_parameters == NULL)
    {
        return NULL;
    }
    if(effect_addr == 0)
    {
    	return AudioEffectParambin.user_effect_parameters;
    }

//    uint16_t total_data_len = GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(AudioEffectParambin.user_effect_parameters);
//    uint16_t third_data_len = GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(AudioEffectParambin.user_effect_parameters);
    uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(AudioEffectParambin.user_effect_parameters);
    uint8_t *effectParams = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(AudioEffectParambin.user_effect_parameters);
    uint16_t remaining = data_len;

    while (remaining > 0)
    {
        if (effectParams[0] == effect_addr)
        {
            return effectParams;
        }
        else
        {
            uint8_t len = effectParams[1];
            if (len + 2 > remaining)
            {
                break;
            }
            effectParams += (len + 2);
            remaining -= (len + 2);
        }
    }
    return NULL;
}

uint8_t *AudioEffect_Parambin_GetFlashEffectParamByAddr(uint8_t effect_addr)
{
	uint8_t *effectParams = NULL;
	roboeffect_parambin_get_mode_data_by_index(AudioEffectParambin.sub_type_data, AudioEffectParambin.param_mode_index, &effectParams, NULL, NULL, NULL);

    if(effect_addr == 0)
    {
    	return effectParams;
    }

    uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(effectParams);
    uint16_t remaining = data_len;
    effectParams = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(effectParams);

    while (remaining > 0)
    {
        if (effectParams[0] == effect_addr)
        {
            return effectParams;
        } else {
            uint8_t len = effectParams[1];
            if (len + 2 > remaining)
            {
                break;
            }
            effectParams += (len + 2);
            remaining -= (len + 2);
        }
    }
    return NULL;
}

#define CFG_FLASH_SECTOR_SIZE		(4096)//4KB
static bool AudioEffect_Parambin_FlashUpdate(uint32_t flash_addr, uint8_t *write_data, uint32_t length)
{
    if (write_data == NULL || length == 0)
    {
        return FALSE;
    }

    uint32_t AUDIOEFFECT_FLASHBIN_ADDRESS = AudioEffect_Parambin_GetFlashEffectAddr();
    if (flash_addr < AUDIOEFFECT_FLASHBIN_ADDRESS)
    {
        return FALSE; // 地址非法
    }

    static uint8_t sector_buf[CFG_FLASH_SECTOR_SIZE] = {0};
    uint32_t total_written = 0;

    while (total_written < length)
    {
        // 计算当前扇区信息
        uint32_t sector_offset = flash_addr - AUDIOEFFECT_FLASHBIN_ADDRESS;
        uint32_t sector_index = sector_offset / CFG_FLASH_SECTOR_SIZE;
        uint32_t sector_start = AUDIOEFFECT_FLASHBIN_ADDRESS + sector_index * CFG_FLASH_SECTOR_SIZE;
        uint32_t sector_internal_offset = sector_offset % CFG_FLASH_SECTOR_SIZE;
        // 计算本次可写入长度
        uint32_t write_len = CFG_FLASH_SECTOR_SIZE - sector_internal_offset;
        if (write_len >= (length - total_written))
        {
            write_len = length - total_written;
        }
//        DBG("%ld, %ld, 0x%x, %ld, %ld, %ld, %ld\n", sector_offset, sector_index, sector_start, sector_internal_offset, length, write_len, total_written);

        // 读取整个扇区
        if (SpiFlashRead(sector_start, sector_buf, CFG_FLASH_SECTOR_SIZE, 1) == FLASH_NONE_ERR)
        {
            // 擦除扇区
            SpiFlashErase(SECTOR_ERASE, sector_start / CFG_FLASH_SECTOR_SIZE, 0);
        }

        // 合并数据到缓冲区
        memcpy(sector_buf + sector_internal_offset, write_data + total_written, write_len);

        // 写入整个扇区
        if (SpiFlashWrite(sector_start, sector_buf, CFG_FLASH_SECTOR_SIZE, 0) != FLASH_NONE_ERR)
        {
            APP_DBG("AudioEffect_Parambin_FlashUpdate ERROR!\n");
            return FALSE;
        }

        flash_addr += write_len;
        total_written += write_len;
    }
    return TRUE;
}

//effect params
//addr:	 1 byte
//length:1 byte
//enable:1 byte
//params:length-1 byte
void AudioEffect_Parambin_UpdateFlashEffectParamsByAddr(uint8_t effect_addr, uint8_t param_index, uint8_t *param_input)
{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
    uint8_t *params = AudioEffect_Parambin_GetFlashEffectParamByAddr(effect_addr);
    if (params == NULL)
    {
        APP_DBG("Error: Invalid effect_addr 0x%02X\n", effect_addr);
        return;
    }

    uint8_t addr = params[0];
    uint8_t len = params[1];
    if (addr != effect_addr || len < 1)
    {
        APP_DBG("Error: Corrupted parameter block\n");
        return;
    }

//	 DBG("%02X: %d, %d\n", addr, param_index, param_input[0]);
    // 更新参数
    if (param_index == 0)//only enable/disable
    {
        if (param_input == NULL) return;
        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, 1);
    }
    else if (param_index == 0xFF)//all parameters
    {
        if (param_input == NULL || len < 1) return;
        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, len);
    }
    else//other parameter, only one parameter configurated
    {
        if (param_index < 1 || param_index > (len - 1)/2)
        {
            APP_DBG("Error: Invalid param_index %d\n", param_index);
            return;
        }
        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 3 + (param_index - 1)*2), param_input, 2);
    }
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
}

void AudioEffect_Parambin_UpdateFlashEffectParams(void)
{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
    uint8_t *flash_params = AudioEffect_Parambin_GetFlashEffectParamByAddr(0);
    uint8_t *temp_params = AudioEffect_Parambin_GetTempEffectParamByAddr(0);
    if (flash_params == NULL || temp_params == NULL)
    {
        APP_DBG("Error: Invalid params point\n");
        return;
    }

    AudioEffect_Parambin_FlashUpdate((uint32_t)(flash_params), temp_params, GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(temp_params));

	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
}

void AudioEffect_Parambin_UpdateTempEffectParamsByAddr(uint8_t effect_addr, uint8_t param_index, uint8_t *param_input)
{
//	if(IsAudioPlayerMute() == FALSE)
//	{
//		HardWareMuteOrUnMute();
//	}
    uint8_t *params = AudioEffect_Parambin_GetTempEffectParamByAddr(effect_addr);
    if (params == NULL)
    {
        APP_DBG("Error: Invalid effect_addr 0x%02X\n", effect_addr);
        return;
    }

    uint8_t addr = params[0];
    uint8_t len = params[1];
    if (addr != effect_addr || len < 1)
    {
        APP_DBG("Error: Corrupted parameter block\n");
        return;
    }

//	 DBG("%02X: %d, %d\n", addr, param_index, param_input[0]);
    // 更新参数
    if (param_index == 0)//only enable/disable
    {
        if (param_input == NULL) return;
//        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, 1);
        memcpy((params + 2), param_input, 1);
    }
    else if (param_index == 0xFF)//all parameters
    {
        if (param_input == NULL || len < 1) return;
//        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, len);
        memcpy((params + 2), param_input, len);
    }
    else//other parameter, only one parameter configurated
    {
        if (param_index < 1 || param_index > (len - 1)/2)
        {
            APP_DBG("Error: Invalid param_index %d\n", param_index);
            return;
        }
//        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 3 + (param_index - 1)*2), param_input, 2);
        memcpy((params + 3 + (param_index - 1)*2), param_input, 2);
    }
//	if(IsAudioPlayerMute() == TRUE)
//	{
//		HardWareMuteOrUnMute();
//	}
}

/** !!!new effect mode will be switched after AudioEffect_EffectMode_Refresh() or AudioEffect_Parambin_Init() **/
void AudioEffect_Parambin_SwitchEffectModeByIndex(uint16_t flow_index, uint16_t param_mode_index)
{
    APP_DBG("Switch Effect Mode %d, %d\n", flow_index, param_mode_index);
	if(flow_index < AudioEffect_Parambin_GetTotalFlowNum())
	{
		if(param_mode_index < AudioEffect_Parambin_GetTotalParamNumInFlow(flow_index))
		{
			AudioEffectParambin.flow_index = flow_index;
			AudioEffectParambin.param_mode_index = param_mode_index;
		}
		else
		{
	        APP_DBG("Error: Invalid param_mode_index %d\n", param_mode_index);
			return;
		}
	}
	else
	{
        APP_DBG("Error: Invalid flow_index %d\n", flow_index);
		return;
	}
}

//void AudioEffect_Parambin_SwitchEffectModeByName(const char *mode_name)
//{
//    uint16_t flow_target = 0;
//    uint16_t param_target = 0;
//    uint16_t max_flows = AudioEffect_Parambin_GetTotalFlowNum();
//    bool found_mode = FALSE;
//    uint16_t original_flow = AudioEffectParambin.flow_index;
//    uint16_t original_param = AudioEffectParambin.param_mode_index;
//
//    // 如果传入的模式名称为空，则直接返回
//    if (mode_name == NULL || strlen(mode_name) == 0)
//    {
//        APP_DBG("Invalid mode name.\n");
//        return;
//    }
//
//    // 遍历所有流和参数，查找目标模式
//    for (flow_target = 0; flow_target < max_flows; flow_target++)
//    {
//        uint16_t max_params_in_flow = AudioEffect_Parambin_GetTotalParamNumInFlow(flow_target);
//        for (param_target = 0; param_target < max_params_in_flow; param_target++)
//        {
//            // 获取当前流的名称
//            const char *flow_name = (char *)AudioEffect_Parambin_GetFlowNameByIndex(flow_target);
//
//            // 如果流名称不为空且与目标模式名称匹配
//            if (flow_name && strcmp(mode_name, flow_name) == 0)
//            {
//                // 切换到目标模式
//                AudioEffectParambin.flow_index = flow_target;
//                AudioEffectParambin.param_mode_index = param_target;
//                found_mode = TRUE;
//                break;
//            }
//        }
//
//        // 如果找到了目标模式，退出外层循环
//        if (found_mode)
//        {
//            break;
//        }
//    }
//
//    // 如果找到了目标模式，执行切换
//    if (found_mode)
//    {
//        AudioEffect_Parambin_SwitchEffectModeByIndex(flow_target, param_target);
//    }
//    else
//    {
//        // 如果没有找到目标模式，恢复原始状态
//        AudioEffectParambin.flow_index = original_flow;
//        AudioEffectParambin.param_mode_index = original_param;
//        APP_DBG("Mode '%s' not found, restored to original mode.\n", mode_name);
//    }
//}


/** !!!new effect mode will be switched after AudioEffect_EffectMode_Refresh() or AudioEffect_Parambin_Init() **/
void AudioEffect_Parambin_SwitchEffectModeByName(const char *mode_name)
{
	uint16_t flow_target = 0;
	uint16_t param_target = 0;
	uint16_t max_flows = AudioEffect_Parambin_GetTotalFlowNum();
	bool found_mode = FALSE;
	uint16_t original_flow = AudioEffectParambin.flow_index;
	uint16_t original_param = AudioEffectParambin.param_mode_index;
	uint8_t *flow_data;
	uint32_t flow_size = 0;

	// 如果传入的模式名称为空，则直接返回
	if (mode_name == NULL || strlen(mode_name) == 0)
	{
		APP_DBG("Invalid mode name.\n");
		return;
	}

    // 遍历所有流和参数，查找目标模式
    for (flow_target = 0; flow_target < max_flows; flow_target++)
    {
        uint16_t max_params_in_flow = AudioEffect_Parambin_GetTotalParamNumInFlow(flow_target);
        for (param_target = 0; param_target < max_params_in_flow; param_target++)
        {
    		uint8_t *mode_data_by_name = NULL, *mode_data_by_index = NULL;
        	if((flow_data = roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), flow_target, &flow_size)) != NULL)
        	{
        		uint8_t *sub_type_data = roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);
        		roboeffect_parambin_get_mode_param_by_name(sub_type_data, mode_name, &mode_data_by_name, NULL);
        		roboeffect_parambin_get_mode_data_by_index(sub_type_data, param_target, &mode_data_by_index, NULL, NULL, NULL);
        	}

            if (mode_data_by_name && mode_data_by_index && (mode_data_by_name == mode_data_by_index))
            {
                // 切换到目标模式
                AudioEffectParambin.flow_index = flow_target;
                AudioEffectParambin.param_mode_index = param_target;
                found_mode = TRUE;
                break;
            }
        }

        // 如果找到了目标模式，退出外层循环
        if (found_mode)
        {
            break;
        }
    }

	// 如果找到了目标模式，执行切换
	if (found_mode)
	{
		AudioEffect_Parambin_SwitchEffectModeByIndex(flow_target, param_target);
	}
	else
	{
		// 如果没有找到目标模式，恢复原始状态
		AudioEffectParambin.flow_index = original_flow;
		AudioEffectParambin.param_mode_index = original_param;
		APP_DBG("Mode '%s' not found, restored to original mode.\n", mode_name);
	}
}

uint8_t *AudioEffect_Parambin_GetFlowNameByIndex(uint16_t flow_index)
{
	if(flow_index >= AudioEffect_Parambin_GetTotalFlowNum())
	{
		return NULL;
	}
	return roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), flow_index, NULL);
}

uint8_t AudioEffect_Parambin_GetEffectAddr(AUDIO_EFFECT_NAME_NUM effect_index)
{
	if(effect_index >= EFFECT_MAX)
	{
		return 0;
	}
	return roboeffect_parambin_get_addr_by_name(AudioEffectParambin.context_memory, audio_effect_name_map[effect_index].enum_str);
}

ROBOEFFECT_ERROR_CODE AudioEffect_Parambin_EffectParam_Set(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index, int16_t param)
{
	if((AudioEffectParambin.context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return -1;

	return roboeffect_set_effect_parameter(AudioEffectParambin.context_memory,
										   AudioEffect_Parambin_GetEffectAddr(effect_index),
										   param_index, (int16_t *)&param);
}

ROBOEFFECT_ERROR_CODE AudioEffect_Parambin_EffectAllParams_Set(AUDIO_EFFECT_NAME_NUM effect_index, int16_t *param)
{
	if((AudioEffectParambin.context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return -1;

	return roboeffect_set_effect_parameter(AudioEffectParambin.context_memory,
											   AudioEffect_Parambin_GetEffectAddr(effect_index),
											   0xff, param);
}

int16_t AudioEffect_Parambin_EffectParam_Get(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index)
{
	if((AudioEffectParambin.context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return -1;

	int16_t *param = (int16_t *)roboeffect_get_effect_parameter(AudioEffectParambin.context_memory, AudioEffect_Parambin_GetEffectAddr(effect_index), param_index);
	return *param;
}

int16_t* AudioEffect_Parambin_EffectAllParams_Get(AUDIO_EFFECT_NAME_NUM effect_index)
{
	if((AudioEffectParambin.context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return NULL;

	return (int16_t *)roboeffect_get_effect_parameter(AudioEffectParambin.context_memory, AudioEffect_Parambin_GetEffectAddr(effect_index), 0xff);
}


int16_t* AudioEffect_Parambin_PreSetEffectParam_Get(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index, AUDIO_EFFECT_PRESET_NUM preset_index, uint32_t *preset_param_size)
{
	if((AudioEffectParambin.context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return NULL;

	uint8_t *param;
	if(ROBOEFFECT_ERROR_OK != roboeffect_parambin_get_preset_param_by_name(AudioEffectParambin.sub_type_data, AudioEffect_Parambin_GetEffectAddr(effect_index),
												 param_index, audio_effect_preset_map[preset_index].enum_str, &param, preset_param_size))
	{
		return NULL;
	}
	(*preset_param_size) /= 2;

	return (int16_t *)param;
}

int16_t* AudioEffect_Parambin_PreSetEffectAllParams_Get(AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index)
{
	if((AudioEffectParambin.context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return NULL;

	uint8_t *param;
	if(ROBOEFFECT_ERROR_OK != roboeffect_parambin_get_preset_param_by_name(AudioEffectParambin.sub_type_data, AudioEffect_Parambin_GetEffectAddr(effect_index),
												 0xff, audio_effect_preset_map[preset_index].enum_str, &param, NULL))
	{
		return NULL;
	}

	return (int16_t *)param + 1;
}

bool AudioEffect_Parambin_EffectModeIsRunning(uint16_t flow_index, uint16_t param_mode_index)
{
	uint8_t *flow_data;
	uint32_t flow_size = 0;
	uint8_t *codec_data_out = NULL;
	uint8_t *mode_data_out = NULL;
	uint32_t AUDIOEFFECT_FLASHBIN_ADDRESS = AudioEffect_Parambin_GetFlashEffectAddr();

	if(roboeffect_parambin_check_whole_bin((uint8_t *)AUDIOEFFECT_FLASHBIN_ADDRESS, NULL) == ROBOEFFECT_ERROR_OK)
	{
		if((flow_data = roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), flow_index, &flow_size)) != NULL)
		{
			uint8_t *sub_type_data = roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);
			roboeffect_parambin_get_mode_data_by_index(sub_type_data, param_mode_index, &mode_data_out, NULL, &codec_data_out, NULL);
			if(AudioEffectParambin.user_codec_parameters == codec_data_out)
				return TRUE;
		}
	}
	return FALSE;
}
