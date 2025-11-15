
#include "audio_stream_api.h"
#include "dma.h"


static bool AudioStream_i2s0_source_Init();
static bool AudioStream_i2s0_source_Deinit();
static bool AudioStream_i2s0_source_Reset();
static bool AudioStream_i2s1_source_Init();
static bool AudioStream_i2s1_source_Deinit();
static bool AudioStream_i2s1_source_Reset();

audio_io_config_t i2s0_source_config =
{
	.dma_id = PERIPHERAL_ID_I2S0_RX,
	.init = AudioStream_i2s0_source_Init,
	.deinit = AudioStream_i2s0_source_Deinit,
	.reset = AudioStream_i2s0_source_Reset,
};

audio_io_config_t i2s1_source_config =
{
	.dma_id = PERIPHERAL_ID_I2S1_RX,
	.init = AudioStream_i2s1_source_Init,
	.deinit = AudioStream_i2s1_source_Deinit,
	.reset = AudioStream_i2s1_source_Reset,
};


static bool AudioStream_i2s0_source_Init()
{

	APP_DBG("I2S0 source init!\n");
}

static bool AudioStream_i2s0_source_Deinit()
{

}

static bool AudioStream_i2s0_source_Reset()
{

}

static bool AudioStream_i2s1_source_Init()
{

	APP_DBG("I2S1 source init!\n");
}

static bool AudioStream_i2s1_source_Deinit()
{

}

static bool AudioStream_i2s1_source_Reset()
{

}
