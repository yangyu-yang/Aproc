
#include "audio_stream_api.h"
#include "dma.h"
#include "dac_interface.h"
#include "audio_core_adapt.h"
#include "audio_core_api.h"
#include "main_task.h"

static bool AudioStream_dac0_sink_Init();
static bool AudioStream_dac0_sink_Deinit();
static bool AudioStream_dac0_sink_Reset();

audio_io_config_t dac0_sink_config =
{
	.dma_id = PERIPHERAL_ID_AUDIO_DAC0_TX,
	.init = AudioStream_dac0_sink_Init,
	.deinit = AudioStream_dac0_sink_Deinit,
	.reset = AudioStream_dac0_sink_Reset,
};

const DACParamCt DACDefaultParamCt =
{
#ifdef CHIP_DAC_USE_DIFF
	.DACModel = DAC_Diff,
#else
	.DACModel = DAC_Single,
#endif

#ifdef CHIP_DAC_USE_PVDD16
	.PVDDModel = PVDD16,
#else
	.PVDDModel = PVDD33,
#endif
	.DACLoadStatus = DAC_NOLoad,
	.DACEnergyModel = DACCommonEnergy,

#ifdef CFG_VCOM_DRIVE_EN
	.DACVcomModel = Direct,
#else
	.DACVcomModel = Disable,
#endif
};

static bool AudioStream_dac0_sink_Init()
{
	APP_DBG("DAC0 sink init!\n");
	AudioCoreIO AudioIOSet;

#ifdef CFG_RES_AUDIO_DAC0_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.FrameSize[0] * 2 ;
#ifdef	CFG_AUDIO_WIDTH_24BIT
	AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
	AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
#endif
	if(!AudioCoreSinkIsInit(AUDIO_DAC0_SINK_NUM))
	{
		mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.DACFIFO = (uint32_t*)osPortMalloc(mainAppCt.DACFIFO_LEN);//DAC fifo
		if(mainAppCt.DACFIFO != NULL)
		{
			memset(mainAppCt.DACFIFO, 0, mainAppCt.DACFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DACFIFO error\n");
			return FALSE;
		}

		//sink0
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioIOSet.Net);
		AudioIOSet.DataIOFunc = AudioDAC0_DataSet;
		AudioIOSet.LenGetFunc = AudioDAC0_DataSpaceLenGet;
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC0_SINK_NUM))
		{
			DBG("Dac init error");
			return FALSE;
		}
		uint16_t BitWidth;
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		BitWidth = 24;
	#else
		BitWidth = 16;
	#endif
		AudioDAC_Init((DACParamCt *)&DACDefaultParamCt,AudioCoreMixSampleRateGet(DefaultNet),BitWidth, (void*)mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN, NULL, 0);

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(AUDIO_DAC0, gCtrlVars.HwCt.DAC0Ct.dac_mclk_source > 2 ? (gCtrlVars.HwCt.DAC0Ct.dac_mclk_source - 1):gCtrlVars.HwCt.DAC0Ct.dac_mclk_source);
	#else
		gCtrlVars.HwCt.DAC0Ct.dac_mclk_source = Clock_AudioMclkGet(AUDIO_DAC0);
	#endif
	}
#endif
	return TRUE;
}

static bool AudioStream_dac0_sink_Deinit()
{

}

static bool AudioStream_dac0_sink_Reset()
{

}
