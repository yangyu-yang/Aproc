
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_bt_source_Init();
static bool AudioStream_bt_source_Deinit();
static bool AudioStream_bt_source_Reset();

audio_io_config_t bt_source_config =
{
	.dma_id = 255,//255 mean not need DMA
	.init = AudioStream_bt_source_Init,
	.deinit = AudioStream_bt_source_Deinit,
	.reset = AudioStream_bt_source_Reset,
};


static bool AudioStream_bt_source_Init()
{

	APP_DBG("BT source init!\n");
}

static bool AudioStream_bt_source_Deinit()
{

}

static bool AudioStream_bt_source_Reset()
{

}
