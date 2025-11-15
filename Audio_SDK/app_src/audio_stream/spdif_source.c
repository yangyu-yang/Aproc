
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_spdif_source_Init();
static bool AudioStream_spdif_source_Deinit();
static bool AudioStream_spdif_source_Reset();
static void AudioStream_spdif_source_Process();

audio_io_config_t spdif_source_config =
{
	.dma_id = PERIPHERAL_ID_SPDIF0_RX,
	.init = AudioStream_spdif_source_Init,
	.deinit = AudioStream_spdif_source_Deinit,
	.reset = AudioStream_spdif_source_Reset,
	.process = AudioStream_spdif_source_Process,
};


static bool AudioStream_spdif_source_Init()
{

	APP_DBG("SPDIF source init!\n");
}

static bool AudioStream_spdif_source_Deinit()
{

}

static bool AudioStream_spdif_source_Reset()
{

}

static void AudioStream_spdif_source_Process()
{

}
