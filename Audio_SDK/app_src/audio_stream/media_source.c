
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_media_source_Init();
static bool AudioStream_media_source_Deinit();
static bool AudioStream_media_source_Reset();

audio_io_config_t media_source_config =
{
	.dma_id = PERIPHERAL_ID_SDIO_RX,
	.init = AudioStream_media_source_Init,
	.deinit = AudioStream_media_source_Deinit,
	.reset = AudioStream_media_source_Reset,
};


static bool AudioStream_media_source_Init()
{

	APP_DBG("MEDIA source init!\n");
}

static bool AudioStream_media_source_Deinit()
{

}

static bool AudioStream_media_source_Reset()
{

}
