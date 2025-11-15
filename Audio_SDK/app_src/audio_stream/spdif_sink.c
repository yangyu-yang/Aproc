
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_spdif_sink_Init();
static bool AudioStream_spdif_sink_Deinit();
static bool AudioStream_spdif_sink_Reset();

audio_io_config_t spdif_sink_config =
{
	.dma_id = PERIPHERAL_ID_SPDIF1_TX,
	.init = AudioStream_spdif_sink_Init,
	.deinit = AudioStream_spdif_sink_Deinit,
	.reset = AudioStream_spdif_sink_Reset,
};

static bool AudioStream_spdif_sink_Init()
{

}

static bool AudioStream_spdif_sink_Deinit()
{

}

static bool AudioStream_spdif_sink_Reset()
{

}
