
#include "audio_stream_api.h"
#include "dma.h"
#include "audio_adc.h"
#include "adc_interface.h"
#include "audio_core_adapt.h"
#include "audio_core_api.h"
#include "ctrlvars.h"
#include "main_task.h"

static bool AudioStream_linein_source_Init();
static bool AudioStream_linein_source_Deinit();
static bool AudioStream_linein_source_Reset();

audio_io_config_t linein_source_config =
{
	.dma_id = PERIPHERAL_ID_AUDIO_ADC0_RX,
	.init = AudioStream_linein_source_Init,
	.deinit = AudioStream_linein_source_Deinit,
	.reset = AudioStream_linein_source_Reset,
};


bool LineInPlayResMalloc(uint16_t SampleLen)
{
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	AudioIOSet.Adapt = STD;
	AudioIOSet.Sync = TRUE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = DefaultNet;
	AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioIOSet.Net);
	AudioIOSet.DataIOFunc = AudioADC0_DataGet;
	AudioIOSet.LenGetFunc = AudioADC0_DataLenGet;

#ifdef	CFG_AUDIO_WIDTH_24BIT
	AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
	AudioIOSet.IOBitWidthConvFlag = 0;//需要数据进行位宽扩展
#endif
	//AudioIOSet.
	if(!AudioCoreSourceInit(&AudioIOSet, LINEIN_SOURCE_NUM))
	{
		DBG("Line source error!\n");
		return FALSE;
	}

	mainAppCt.ADC0FIFO_len = SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 2;
	//LineIn5  digital (DMA)
	mainAppCt.ADC0FIFO = (uint32_t*)osPortMalloc(mainAppCt.ADC0FIFO_len);
	if(mainAppCt.ADC0FIFO == NULL)
	{
		return FALSE;
	}
	memset(mainAppCt.ADC0FIFO, 0, mainAppCt.ADC0FIFO_len);

	return TRUE;
}

static bool AudioStream_linein_source_Init()
{
	APP_DBG("LINEIN source init!\n");

	if(!LineInPlayResMalloc(AudioCoreFrameSizeGet(DefaultNet)))
	{
		APP_DBG("LineInPlay Res Error!\n");
		return FALSE;
	}

	AudioAnaChannelSet(CHIP_LINEIN_CHANNEL);

	AUDIO_BitWidth BitWidth = ADC_WIDTH_16BITS;
#ifdef	CFG_AUDIO_WIDTH_24BIT
	if(AudioCoreSourceBitWidthGet(LINEIN_SOURCE_NUM) == PCM_DATA_24BIT_WIDTH)
		BitWidth = ADC_WIDTH_24BITS;
#endif

	AudioADC_DigitalInit(ADC0_MODULE, AudioCoreMixSampleRateGet(DefaultNet),BitWidth, (void*)mainAppCt.ADC0FIFO, mainAppCt.ADC0FIFO_len);

#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
    Clock_AudioMclkSel(AUDIO_ADC0, gCtrlVars.HwCt.ADC0DigitalCt.adc_mclk_source > 2 ? (gCtrlVars.HwCt.ADC0DigitalCt.adc_mclk_source - 1):gCtrlVars.HwCt.ADC0DigitalCt.adc_mclk_source);
#else
	gCtrlVars.HwCt.ADC0DigitalCt.adc_mclk_source = Clock_AudioMclkGet(AUDIO_ADC0);
#endif

#if (CHIP_LINEIN_CHANNEL == ANA_INPUT_CH_LINEIN1)
#ifdef CFG_ADCDAC_SEL_LOWPOWERMODE
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_LEFT,LINEIN1_LEFT,Single,ADCLowEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_RIGHT,LINEIN1_RIGHT,Single,ADCLowEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_gain);
#else
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_LEFT,LINEIN1_LEFT,Single,ADCCommonEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_RIGHT,LINEIN1_RIGHT,Single,ADCCommonEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_gain);
#endif // CFG_ADCDAC_SEL_LOWPOWERMODE
#endif
#if (CHIP_LINEIN_CHANNEL == ANA_INPUT_CH_LINEIN2)
	GPIO_PortBModeSet(GPIOB0,0);
	GPIO_PortBModeSet(GPIOB1,0);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB0);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB0);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB1);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB1);

#ifdef CFG_ADCDAC_SEL_LOWPOWERMODE
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_LEFT,LINEIN2_LEFT,Single,ADCLowEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_RIGHT,LINEIN2_RIGHT,Single,ADCLowEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_gain);
#else
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_LEFT,LINEIN2_LEFT,Single,ADCCommonEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
	AudioADC_AnaInit(ADC0_MODULE,CHANNEL_RIGHT,LINEIN2_RIGHT,Single,ADCCommonEnergy,31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_gain);
#endif // CFG_ADCDAC_SEL_LOWPOWERMODE
#endif

	AudioCodecGainUpdata();//update hardware config
	return TRUE;
}

static bool AudioStream_linein_source_Deinit()
{

}

static bool AudioStream_linein_source_Reset()
{

}
