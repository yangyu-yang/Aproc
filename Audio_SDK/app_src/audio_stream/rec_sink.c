
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_rec_sink_Init();
static bool AudioStream_rec_sink_Deinit();
static bool AudioStream_rec_sink_Reset();

audio_io_config_t rec_sink_config =
{
	.dma_id = PERIPHERAL_ID_SDIO_RX,
	.init = AudioStream_rec_sink_Init,
	.deinit = AudioStream_rec_sink_Deinit,
	.reset = AudioStream_rec_sink_Reset,
};

static bool AudioStream_rec_sink_Init()
{

}

static bool AudioStream_rec_sink_Deinit()
{

}

static bool AudioStream_rec_sink_Reset()
{

}
