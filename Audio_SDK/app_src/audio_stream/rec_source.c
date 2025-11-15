
#include "audio_stream_api.h"
#include "dma.h"


static bool AudioStream_rec_source_Init();
static bool AudioStream_rec_source_Deinit();
static bool AudioStream_rec_source_Reset();

audio_io_config_t rec_source_config =
{
	.dma_id = PERIPHERAL_ID_SDIO_TX,
	.init = AudioStream_rec_source_Init,
	.deinit = AudioStream_rec_source_Deinit,
	.reset = AudioStream_rec_source_Reset,
};


static bool AudioStream_rec_source_Init()
{

	APP_DBG("REC source init!\n");
}

static bool AudioStream_rec_source_Deinit()
{

}

static bool AudioStream_rec_source_Reset()
{

}
