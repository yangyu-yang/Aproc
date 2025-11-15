
#include "audio_stream_api.h"
#include "dma.h"


static bool AudioStream_usb_source_Init();
static bool AudioStream_usb_source_Deinit();
static bool AudioStream_usb_source_Reset();

audio_io_config_t usb_source_config =
{
	.dma_id = 255,//255 mean not need DMA
	.init = AudioStream_usb_source_Init,
	.deinit = AudioStream_usb_source_Deinit,
	.reset = AudioStream_usb_source_Reset,
};


static bool AudioStream_usb_source_Init()
{

	APP_DBG("USB source init!\n");
}

static bool AudioStream_usb_source_Deinit()
{

}

static bool AudioStream_usb_source_Reset()
{

}
