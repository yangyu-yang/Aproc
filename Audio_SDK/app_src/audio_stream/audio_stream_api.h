/**
 **************************************************************************************
 * @file    audio_stream_api.h
 * @brief   Audio Stream(input & output)
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_STREAM_API_H__
#define __AUDIO_STREAM_API_H__
#include "type.h"
#include "debug.h"
#include "app_config.h"

typedef struct {
    uint8_t dma_id;
    uint8_t bit_width;
    uint8_t ch_num;
    bool (*init)(void);
    bool (*deinit)(void);
    bool (*reset)(void);
    void (*process)(void);
    bool is_initialized;
} audio_io_config_t;

void AudioStreamInit();
#endif
