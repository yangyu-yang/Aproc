
#include "audio_stream_api.h"
#include "dma.h"


static bool AudioStream_remind_source_Init();
static bool AudioStream_remind_source_Deinit();
static bool AudioStream_remind_source_Reset();

audio_io_config_t remind_source_config =
{
	.dma_id = 255,
	.init = AudioStream_remind_source_Init,
	.deinit = AudioStream_remind_source_Deinit,
	.reset = AudioStream_remind_source_Reset,
};


static bool AudioStream_remind_source_Init()
{

	APP_DBG("REMIND source init!\n");
}

static bool AudioStream_remind_source_Deinit()
{

}

static bool AudioStream_remind_source_Reset()
{

}
