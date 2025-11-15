
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_bt_source_sink_Init();
static bool AudioStream_bt_source_sink_Deinit();
static bool AudioStream_bt_source_sink_Reset();

audio_io_config_t bt_source_sink_config =
{
	.dma_id = 255,//255 mean not need DMA
	.init = AudioStream_bt_source_sink_Init,
	.deinit = AudioStream_bt_source_sink_Deinit,
	.reset = AudioStream_bt_source_sink_Reset,
};


static bool AudioStream_bt_source_sink_Init()
{
	APP_DBG("BT_SOURCE sink init!\n");
}

static bool AudioStream_bt_source_sink_Deinit()
{

}

static bool AudioStream_bt_source_sink_Reset()
{

}
