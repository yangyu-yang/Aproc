
#include "audio_stream_api.h"
#include "dma.h"

static bool AudioStream_hdmi_source_Init();
static bool AudioStream_hdmi_source_Deinit();
static bool AudioStream_hdmi_source_Reset();

audio_io_config_t hdmi_source_config =
{
	.dma_id = PERIPHERAL_ID_SPDIF0_RX,
	.init = AudioStream_hdmi_source_Init,
	.deinit = AudioStream_hdmi_source_Deinit,
	.reset = AudioStream_hdmi_source_Reset,
};


static bool AudioStream_hdmi_source_Init()
{

	APP_DBG("HDMI source init!\n");
}

static bool AudioStream_hdmi_source_Deinit()
{

}

static bool AudioStream_hdmi_source_Reset()
{

}
