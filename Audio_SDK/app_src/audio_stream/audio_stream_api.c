/**
 **************************************************************************************
 * @file    audio_stream_api.c
 * @brief   Audio Stream(input & output)
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "audio_core_adapt.h"
#include "audio_core_api.h"
#include "dma.h"
#include "audio_effect_process.h"


audio_io_config_t* audio_get_source_config(AUDIO_CORE_SOURCE_NUM source) {
    switch(source) {
        #define XX_CASE(name, str, config_ptr) case name: return config_ptr;
        AUDIO_SOURCE_ENUM_MAP(XX_CASE)
        #undef XX_CASE
        default: return NULL;
    }
}

audio_io_config_t* audio_get_sink_config(AUDIO_CORE_SINK_NUM sink) {
    switch(sink) {
        #define XX_CASE(name, str, config_ptr) case name: return config_ptr;
        AUDIO_SINK_ENUM_MAP(XX_CASE)
        #undef XX_CASE
        default: return NULL;
    }
}

bool AudioStream_Source_Init()
{
	audio_io_config_t *io_source;

	for(uint8_t i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(AudioEffect_Parambin_GetSourceBuffer(i))
		{
			io_source = audio_get_source_config(i);
			if(io_source->init())
			{
				io_source->is_initialized = TRUE;
				AudioCoreSourceEnable(i);
			}
			else
			{
				APP_DBG("Source %d init error!\n", i);
			}
		}
	}
	return TRUE;
}

bool AudioStream_Sink_Init()
{
	audio_io_config_t *io_sink;

	for(uint8_t i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		if(AudioEffect_Parambin_GetSinkBuffer(i))
		{
			io_sink = audio_get_sink_config(i);
			if(io_sink->init())
			{
				io_sink->is_initialized = TRUE;
				AudioCoreSinkEnable(i);
			}
			else
			{
				APP_DBG("Sink %d init error!\n", i);
			}
		}
	}
	return TRUE;
}

bool AudioStream_Source_Deinit()
{
	audio_io_config_t *io_source;

	for(uint8_t i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(AudioEffect_Parambin_GetSourceBuffer(i))
		{
			io_source = audio_get_source_config(i);
			io_source->deinit();
		}
	}
	return TRUE;
}

bool AudioStream_Sink_Deinit()
{
	audio_io_config_t *io_sink;

	for(uint8_t i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		if(AudioEffect_Parambin_GetSinkBuffer(i))
		{
			io_sink = audio_get_sink_config(i);
			io_sink->deinit();
		}
	}
	return TRUE;
}

bool AudioStream_DMA_Init()
{
    #define MAX_DMA_CHANNELS 6
    #define DMA_ID_NO_DMA 0xFF

    uint8_t gDmaChannelMap[MAX_DMA_CHANNELS];
    audio_io_config_t *io_config;
    uint8_t dma_count = 0;
    bool dma_used[PERIPHERAL_ID_MAX] = {false};

    // 初始化映射表
    for(uint8_t i = 0; i < MAX_DMA_CHANNELS; i++)
    {
        gDmaChannelMap[i] = DMA_ID_NO_DMA;
    }

    // 处理所有已初始化的source
    for(uint8_t i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
    {
        io_config = audio_get_source_config(i);
        if(!io_config) continue; // 跳过无效配置

        // 只处理已初始化的source
        if(io_config->is_initialized)
        {
            // 跳过不使用DMA的情况
            if(io_config->dma_id == DMA_ID_NO_DMA) continue;

            // 验证DMA ID
            if(io_config->dma_id >= PERIPHERAL_ID_MAX || dma_used[io_config->dma_id])
            {
                printf("Error: Invalid or duplicate DMA ID %d for source %d\n",
                       io_config->dma_id, i);
                return FALSE;
            }

            if(dma_count >= MAX_DMA_CHANNELS)
            {
                printf("Error: Too many DMA channels\n");
                return FALSE;
            }

            dma_used[io_config->dma_id] = true;
            gDmaChannelMap[dma_count++] = io_config->dma_id;
            printf("Source %d (initialized) uses DMA ID %d\n", i, io_config->dma_id);
        }
    }

    // 处理所有已初始化的sink
    for(uint8_t i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
    {
        io_config = audio_get_sink_config(i);
        if(!io_config) continue; // 跳过无效配置

        // 只处理已初始化的sink
        if(io_config->is_initialized)
        {
            if(io_config->dma_id == DMA_ID_NO_DMA) continue;

            if(io_config->dma_id >= PERIPHERAL_ID_MAX || dma_used[io_config->dma_id])
            {
                printf("Error: Invalid or duplicate DMA ID %d for sink %d\n",
                       io_config->dma_id, i);
                return FALSE;
            }

            if(dma_count >= MAX_DMA_CHANNELS)
            {
                printf("Error: Too many DMA channels\n");
                return FALSE;
            }

            dma_used[io_config->dma_id] = true;
            gDmaChannelMap[dma_count++] = io_config->dma_id;
            printf("Sink %d (initialized) uses DMA ID %d\n", i, io_config->dma_id);
        }
    }

    // 设置DMA分配表
    DMA_ChannelAllocTableSet(gDmaChannelMap);
    printf("DMA Init: %d/%d channels configured for initialized\n",
           dma_count, MAX_DMA_CHANNELS);

    return TRUE;

    #undef MAX_DMA_CHANNELS
    #undef DMA_ID_NO_DMA
}

void AudioStreamInit()
{
	if(!AudioEffect_Parambin_Init())
	{
		return FALSE;
	}
	AudioEffect_Params_Sync();

	DefaultParamgsInit();	//refresh local hardware config params(just storage not set)
//	roboeffect_prot_init();

	AudioStream_Source_Init();
	AudioStream_Sink_Init();
	AudioStream_DMA_Init();
	AudioCoreProcessConfig((void*)AudioMusicProcess);

	AudioCoreServiceResume();
}

void AudioStreamDeinit()
{
	PauseAuidoCore();
}
