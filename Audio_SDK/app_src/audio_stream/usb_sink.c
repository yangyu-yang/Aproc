
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_usb_sink_Init();
static bool AudioStream_usb_sink_Deinit();
static bool AudioStream_usb_sink_Reset();

audio_io_config_t usb_sink_config =
{
	.dma_id = 255,//255 mean not need DMA
	.init = AudioStream_usb_sink_Init,
	.deinit = AudioStream_usb_sink_Deinit,
	.reset = AudioStream_usb_sink_Reset,
};

static bool AudioStream_usb_sink_Init()
{

}

static bool AudioStream_usb_sink_Deinit()
{

}

static bool AudioStream_usb_sink_Reset()
{

}
