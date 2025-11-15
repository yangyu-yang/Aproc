
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_i2s0_sink_Init();
static bool AudioStream_i2s0_sink_Deinit();
static bool AudioStream_i2s0_sink_Reset();
static bool AudioStream_i2s1_sink_Init();
static bool AudioStream_i2s1_sink_Deinit();
static bool AudioStream_i2s1_sink_Reset();

audio_io_config_t i2s0_sink_config =
{
	.dma_id = PERIPHERAL_ID_I2S0_TX,
	.init = AudioStream_i2s0_sink_Init,
	.deinit = AudioStream_i2s0_sink_Deinit,
	.reset = AudioStream_i2s0_sink_Reset,
};

audio_io_config_t i2s1_sink_config =
{
	.dma_id = PERIPHERAL_ID_I2S1_TX,
	.init = AudioStream_i2s1_sink_Init,
	.deinit = AudioStream_i2s1_sink_Deinit,
	.reset = AudioStream_i2s1_sink_Reset,
};


static bool AudioStream_i2s0_sink_Init()
{

	APP_DBG("I2S0 sink init!\n");
}

static bool AudioStream_i2s0_sink_Deinit()
{

}

static bool AudioStream_i2s0_sink_Reset()
{

}

static bool AudioStream_i2s1_sink_Init()
{

	APP_DBG("I2S1 sink init!\n");
}

static bool AudioStream_i2s1_sink_Deinit()
{

}

static bool AudioStream_i2s1_sink_Reset()
{

}
