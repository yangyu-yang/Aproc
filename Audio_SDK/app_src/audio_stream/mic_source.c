
#include "audio_stream_api.h"
#include "dma.h"
#include "audio_adc.h"
#include "adc_interface.h"
#include "audio_core_adapt.h"
#include "audio_core_api.h"
#include "main_task.h"


static bool AudioStream_mic_source_Init();
static bool AudioStream_mic_source_Deinit();
static bool AudioStream_mic_source_Reset();

audio_io_config_t mic_source_config =
{
	.dma_id = PERIPHERAL_ID_AUDIO_ADC1_RX,
	.init = AudioStream_mic_source_Init,
	.deinit = AudioStream_mic_source_Deinit,
	.reset = AudioStream_mic_source_Reset,
};


static bool AudioStream_mic_source_Init()
{
	APP_DBG("MIC source init!\n");

	AudioCoreIO	AudioIOSet;
	uint16_t FifoLenStereo = AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2;//立体声8倍大小于帧长，单位byte

#if CFG_RES_MIC_SELECT
	if(!AudioCoreSourceIsInit(MIC_SOURCE_NUM))
	{
		mainAppCt.ADC1FIFO = (uint32_t*)osPortMalloc(FifoLenStereo);//ADC fifo
		if(mainAppCt.ADC1FIFO != NULL)
		{
			memset(mainAppCt.ADC1FIFO, 0, FifoLenStereo);
		}
		else
		{
			APP_DBG("malloc ADCFIFO error\n");
			return FALSE;
		}

		AudioADC_DynamicElementMatch(ADC1_MODULE, TRUE, TRUE);
		//Mic1   digital
	#ifdef CFG_AUDIO_WIDTH_24BIT
		AudioADC_DigitalInit(ADC1_MODULE, AudioCoreMixSampleRateGet(DefaultNet),ADC_WIDTH_24BITS,(void*)mainAppCt.ADC1FIFO, FifoLenStereo);
	#else
		AudioADC_DigitalInit(ADC1_MODULE, AudioCoreMixSampleRateGet(DefaultNet),ADC_WIDTH_16BITS,(void*)mainAppCt.ADC1FIFO, FifoLenStereo);
	#endif

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(AUDIO_ADC1, gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source > 2 ? (gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source - 1):gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source);
	#else
		gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source = Clock_AudioMclkGet(AUDIO_ADC1);
	#endif
		//Soure0.
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 1;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioIOSet.Net);
		AudioIOSet.DataIOFunc = AudioADC1_DataGet;
		AudioIOSet.LenGetFunc = AudioADC1_DataLenGet;
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 0;//需要数据进行位宽扩展
	#endif
		if(!AudioCoreSourceInit(&AudioIOSet, MIC_SOURCE_NUM))
		{
			DBG("mic Source error");
			return FALSE;
		}
	}
#ifdef CFG_ADCDAC_SEL_LOWPOWERMODE
	AudioADC_AnaInit(ADC1_MODULE,CHANNEL_LEFT,MIC_LEFT,gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode,ADCLowEnergy,31 - gCtrlVars.HwCt.ADC1PGACt.pga_mic_gain);
#else
	AudioADC_AnaInit(ADC1_MODULE,CHANNEL_LEFT,MIC_LEFT,gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode,ADCCommonEnergy,31 - gCtrlVars.HwCt.ADC1PGACt.pga_mic_gain);
#endif // CFG_ADCDAC_SEL_LOWPOWERMODE
	//MADC_MIC_PowerUP(SingleEnded);
#endif

}

static bool AudioStream_mic_source_Deinit()
{

}

static bool AudioStream_mic_source_Reset()
{

}
