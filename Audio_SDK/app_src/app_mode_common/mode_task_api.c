/*
 * mode_task_api.c
 *
 *  Created on: Mar 30, 2021
 *      Author: piwang
 */
#include "main_task.h"
#include "audio_core_service.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "ctrlvars.h"
#include "breakpoint.h"
#include "remind_sound.h"
#include "i2s_interface.h"
#include "dma.h"
#include "watchdog.h"
//service
#include "bt_manager.h"
#include "audio_vol.h"
#include "remind_sound.h"
#include "hdmi_in_api.h"
#include "audio_effect_control.h"
#include "clk.h"
#include "communication.h"
//app
#include "bt_stack_service.h"
#if (BT_AVRCP_VOLUME_SYNC)
#include "bt_app_avrcp_deal.h"
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
#include "spdif_out.h"
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
extern bool I2S_MixInit(bool hf_mode_flag);
extern bool I2S_MixDeinit(void);
#endif
#ifdef CFG_FUNC_I2S_MIX2_MODE
extern bool I2S_Mix2Init(bool hf_mode_flag);
extern bool I2S_Mix2Deinit(void);
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
extern bool LineInMixPlayInit(void);
extern bool LineInMixPlayDeinit(void);
#endif
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
extern bool UsbDevicePlayMixInit(void);
extern bool UsbDevicePlayMixDeinit(void);
#endif

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
extern int32_t RemindMp3DecoderInit(void);
extern int32_t RemindMp3DecoderDeinit(void);
#endif

//const DACParamCt DACDefaultParamCt =
//{
//#ifdef CHIP_DAC_USE_DIFF
//	.DACModel = DAC_Diff,
//#else
//	.DACModel = DAC_Single,
//#endif
//
//#ifdef CHIP_DAC_USE_PVDD16
//	.PVDDModel = PVDD16,
//#else
//	.PVDDModel = PVDD33,
//#endif
//	.DACLoadStatus = DAC_NOLoad,
//	.DACEnergyModel = DACCommonEnergy,
//
//#ifdef CFG_VCOM_DRIVE_EN
//	.DACVcomModel = Direct,
//#else
//	.DACVcomModel = Disable,
//#endif
//};

#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE  //USB MIX通道，使用 USB_SOURCE_NUM
	uint32_t	UsbAudioSourceNum = APP_SOURCE_NUM;
#endif

//#ifdef CFG_APP_USB_PLAY_MODE_EN
//void WaitUdiskUnlock(void)
//{
//	if(GetSysModeState(ModeUDiskAudioPlay) == ModeStateDeinit)
//	{
//		while(osMutexLock_1000ms(UDiskMutex) != TRUE)WDG_Feed();
//	}
//}
//#endif
//
//#ifdef CFG_APP_CARD_PLAY_MODE_EN
//void SDCardForceExitFuc(void)
//{
//	if(GetSysModeState(ModeCardAudioPlay) == ModeStateDeinit)
//	{
//		SDCard_Force_Exit();
//	}
//}
//#endif

#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
void AudioOutSampleRateSet(uint32_t SampleRate)
{
	extern uint32_t IsBtHfMode(void);
	const char *flow_name = (char *)AudioEffect_Parambin_GetFlowNameByIndex(mainAppCt.effect_flow_index);
	if(IsBtHfMode() || (flow_name && (strcmp("hfp", flow_name) == 0)))
		return;

	if((SampleRate == 11025) || (SampleRate == 22050) || (SampleRate == 44100)
			|| (SampleRate == 88200) || (SampleRate == 176400))
	{
		SampleRate = 44100;
	}
	else
	{
		SampleRate = 48000;
	}

	if(AudioCoreMixSampleRateGet(DefaultNet) == SampleRate)
		return;
	APP_DBG("SampleRate: %d --> %d\n", (int)AudioCoreMixSampleRateGet(DefaultNet), (int)SampleRate);

#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioDAC0_SampleRateChange(SampleRate);
	gCtrlVars.HwCt.DAC0Ct.dac_mclk_source = Clock_AudioMclkGet(AUDIO_DAC0);
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	AudioI2S_SampleRateChange(CFG_RES_I2S_MODULE,SampleRate);
	if(CFG_RES_I2S_MODULE == I2S0_MODULE)
		gCtrlVars.HwCt.I2S0Ct.i2s_mclk_source = Clock_AudioMclkGet(AUDIO_I2S0);
	else
		gCtrlVars.HwCt.I2S1Ct.i2s_mclk_source = Clock_AudioMclkGet(AUDIO_I2S1);
#endif

#if CFG_RES_MIC_SELECT
	AudioADC_SampleRateChange(ADC1_MODULE,SampleRate);
	gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source = Clock_AudioMclkGet(AUDIO_ADC1);
#endif

	AudioEffect_UpdateSamplerateFramesize(SampleRate, AudioCoreFrameSizeGet(DefaultNet));
}
#endif

void PauseAuidoCore(void)
{
	while(GetAudioCoreServiceState() != TaskStatePaused)
	{
		AudioCoreServicePause();
		osTaskDelay(2);
	}
}

#ifdef CFG_RES_AUDIO_I2SOUT_EN
void AudioI2sOutParamsSet(void)
{
	I2SParamCt i2s_set;
	i2s_set.IsMasterMode = CFG_RES_I2S_MODE;// 0:master 1:slave
	i2s_set.SampleRate = CFG_PARA_I2S_SAMPLERATE; //外设采样率
	i2s_set.I2sFormat = I2S_FORMAT_I2S;
#ifdef	CFG_AUDIO_WIDTH_24BIT
	i2s_set.I2sBits = I2S_LENGTH_24BITS;
#else
	i2s_set.I2sBits = I2S_LENGTH_16BITS;
#endif
	i2s_set.I2sTxRxEnable = 1;

	i2s_set.TxPeripheralID = PERIPHERAL_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE;

	i2s_set.TxBuf = (void*)mainAppCt.I2SFIFO;

	i2s_set.TxLen = mainAppCt.I2SFIFO_LEN;

	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S_MCLK_GPIO));//mclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S_LRCLK_GPIO));//lrclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S_BCLK_GPIO));//bclk
	GPIO_PortAModeSet(GET_I2S_GPIO_PORT(I2S_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S_DOUT_GPIO));//do

	I2S_AlignModeSet(CFG_RES_I2S_MODULE, I2S_LOW_BITS_ACTIVE);
	AudioI2S_Init(CFG_RES_I2S_MODULE, &i2s_set);//

#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
	if(CFG_RES_I2S_MODULE == I2S0_MODULE)
		Clock_AudioMclkSel(AUDIO_I2S0, gCtrlVars.HwCt.I2S0Ct.i2s_mclk_source > 2 ? (gCtrlVars.HwCt.I2S0Ct.i2s_mclk_source - 1):gCtrlVars.HwCt.I2S0Ct.i2s_mclk_source);
	else
		Clock_AudioMclkSel(AUDIO_I2S1, gCtrlVars.HwCt.I2S1Ct.i2s_mclk_source > 2 ? (gCtrlVars.HwCt.I2S1Ct.i2s_mclk_source - 1):gCtrlVars.HwCt.I2S1Ct.i2s_mclk_source);
#else
	if(CFG_RES_I2S_MODULE == I2S0_MODULE)
		gCtrlVars.HwCt.I2S0Ct.i2s_mclk_source = Clock_AudioMclkGet(AUDIO_I2S0);
	else
		gCtrlVars.HwCt.I2S1Ct.i2s_mclk_source = Clock_AudioMclkGet(AUDIO_I2S1);
#endif
}
#endif

//配置系统标准通路
bool ModeCommonInit(void)
{
	AudioCoreIO AudioIOSet;
	uint16_t FifoLenStereo;
	uint32_t sampleRate;

	if(!AudioEffect_Parambin_Init())
	{
//		if(GetSysModeState(ModeSafe) == ModeStateSusend)
//		{
//			SetSysModeState(ModeSafe, ModeStateReady);
//		}
		return FALSE;
	}
	AudioEffect_Params_Sync();

	sampleRate  = AudioCoreMixSampleRateGet(DefaultNet);
	FifoLenStereo = AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2;//立体声8倍大小于帧长，单位byte

	DefaultParamgsInit();	//refresh local hardware config params(just storage not set)

//	//////////申请DMA fifo
//#ifdef CFG_RES_AUDIO_DAC0_EN
//	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
//	AudioIOSet.Depth = AudioCore.FrameSize[0] * 2 ;
//#ifdef	CFG_AUDIO_WIDTH_24BIT
//	AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
//	AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
//#endif
//	if(!AudioCoreSinkIsInit(AUDIO_DAC0_SINK_NUM))
//	{
//		mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
//		mainAppCt.DACFIFO = (uint32_t*)osPortMalloc(mainAppCt.DACFIFO_LEN);//DAC fifo
//		if(mainAppCt.DACFIFO != NULL)
//		{
//			memset(mainAppCt.DACFIFO, 0, mainAppCt.DACFIFO_LEN);
//		}
//		else
//		{
//			APP_DBG("malloc DACFIFO error\n");
//			return FALSE;
//		}
//
//		//sink0
//		AudioIOSet.Adapt = STD;
//		AudioIOSet.Sync = TRUE;
//		AudioIOSet.Channels = 2;
//		AudioIOSet.Net = DefaultNet;
//		AudioIOSet.DataIOFunc = AudioDAC0_DataSet;
//		AudioIOSet.LenGetFunc = AudioDAC0_DataSpaceLenGet;
//		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC0_SINK_NUM))
//		{
//			DBG("Dac init error");
//			return FALSE;
//		}
//		uint16_t BitWidth;
//	#ifdef	CFG_AUDIO_WIDTH_24BIT
//		BitWidth = 24;
//	#else
//		BitWidth = 16;
//	#endif
//		AudioDAC_Init((DACParamCt *)&DACDefaultParamCt,sampleRate,BitWidth, (void*)mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN, NULL, 0);
//
//	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
//        Clock_AudioMclkSel(AUDIO_DAC0, gCtrlVars.HwCt.DAC0Ct.dac_mclk_source > 2 ? (gCtrlVars.HwCt.DAC0Ct.dac_mclk_source - 1):gCtrlVars.HwCt.DAC0Ct.dac_mclk_source);
//	#else
//		gCtrlVars.HwCt.DAC0Ct.dac_mclk_source = Clock_AudioMclkGet(AUDIO_DAC0);
//	#endif
//	}
//	else//sam add,20230221
//	{
//		AudioDAC0_SampleRateChange(sampleRate);
//		gCtrlVars.HwCt.DAC0Ct.dac_mclk_source = Clock_AudioMclkGet(AUDIO_DAC0);
//	#ifdef	CFG_AUDIO_WIDTH_24BIT
//		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
//		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
//	#endif
//	}
//	AudioCoreSinkEnable(AUDIO_DAC0_SINK_NUM);
//#endif

	//Mic1 analog  = Soure0.
	//AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	// AudioADC_MicBias1Enable(1);
//#if CFG_RES_MIC_SELECT
//	if(!AudioCoreSourceIsInit(MIC_SOURCE_NUM))
//	{
//		mainAppCt.ADCFIFO = (uint32_t*)osPortMalloc(FifoLenStereo);//ADC fifo
//		if(mainAppCt.ADCFIFO != NULL)
//		{
//			memset(mainAppCt.ADCFIFO, 0, FifoLenStereo);
//		}
//		else
//		{
//			APP_DBG("malloc ADCFIFO error\n");
//			return FALSE;
//		}
//
//		AudioADC_DynamicElementMatch(ADC1_MODULE, TRUE, TRUE);
////		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1);
////		AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2);
////		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 15, 4);//0db bypass
////		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 15, 4);
//
//		//Mic1   digital
//	#ifdef CFG_AUDIO_WIDTH_24BIT
//		AudioADC_DigitalInit(ADC1_MODULE, sampleRate,ADC_WIDTH_24BITS,(void*)mainAppCt.ADCFIFO, FifoLenStereo);
//	#else
//		AudioADC_DigitalInit(ADC1_MODULE, sampleRate,ADC_WIDTH_16BITS,(void*)mainAppCt.ADCFIFO, FifoLenStereo);
//	#endif
//
//	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
//        Clock_AudioMclkSel(AUDIO_ADC1, gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source > 2 ? (gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source - 1):gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source);
//	#else
//		gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source = Clock_AudioMclkGet(AUDIO_ADC1);
//	#endif
//		//Soure0.
//		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
//		AudioIOSet.Adapt = STD;
//		AudioIOSet.Sync = TRUE;
//		AudioIOSet.Channels = 1;
//		AudioIOSet.Net = DefaultNet;
//		AudioIOSet.DataIOFunc = AudioADC1_DataGet;
//		AudioIOSet.LenGetFunc = AudioADC1_DataLenGet;
//	#ifdef	CFG_AUDIO_WIDTH_24BIT
//		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
//		AudioIOSet.IOBitWidthConvFlag = 0;//需要数据进行位宽扩展
//	#endif
//		if(!AudioCoreSourceInit(&AudioIOSet, MIC_SOURCE_NUM))
//		{
//			DBG("mic Source error");
//			return FALSE;
//		}
//	}
//#ifdef CFG_ADCDAC_SEL_LOWPOWERMODE
//	AudioADC_AnaInit(ADC1_MODULE,CHANNEL_LEFT,MIC_LEFT,gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode,ADCLowEnergy,31 - gCtrlVars.HwCt.ADC1PGACt.pga_mic_gain);
//#else
//	AudioADC_AnaInit(ADC1_MODULE,CHANNEL_LEFT,MIC_LEFT,gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode,ADCCommonEnergy,31 - gCtrlVars.HwCt.ADC1PGACt.pga_mic_gain);
//#endif // CFG_ADCDAC_SEL_LOWPOWERMODE
//	//MADC_MIC_PowerUP(SingleEnded);
//	AudioCoreSourceEnable(MIC_SOURCE_NUM);
//#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!AudioCoreSourceIsInit(REMIND_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = SRC_ONLY;
		AudioIOSet.SampleRate = sampleRate;//初始值
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = 1;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.DataIOFunc = RemindDataGet;
		AudioIOSet.LenGetFunc = RemindDataLenGet;

	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
		AudioIOSet.IOBitWidthConvFlag = 1;//需要数据进行位宽扩展
	#endif
		if(!AudioCoreSourceInit(&AudioIOSet, REMIND_SOURCE_NUM))
		{
			DBG("remind source error!\n");
			SoftFlagRegister(SoftFlagNoRemind);
			return FALSE;
		}
	#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
		RemindMp3DecoderInit();
	#endif
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.FrameSize[0] * 2 ;
#ifdef	CFG_AUDIO_WIDTH_24BIT
	AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
	AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
#endif
	if(!AudioCoreSinkIsInit(AUDIO_I2SOUT_SINK_NUM))
	{
		mainAppCt.I2SFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.I2SFIFO = (uint32_t*)osPortMalloc(mainAppCt.I2SFIFO_LEN);//I2S fifo
		if(mainAppCt.I2SFIFO != NULL)
		{
			memset(mainAppCt.I2SFIFO, 0, mainAppCt.I2SFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc I2SFIFO error\n");
			return FALSE;
		}

#if((CFG_RES_I2S_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S_IN_SYNC_EN))
		// Master 或不开微调
		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
			AudioIOSet.Adapt = STD;//SRC_ONLY
		else
			AudioIOSet.Adapt = SRC_ONLY;
#else//slave
		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
			AudioIOSet.Adapt = STD;//SRA_ONLY;//CLK_ADJUST_ONLY;
		else
			AudioIOSet.Adapt = SRC_ONLY;//SRC_SRA;//SRC_ADJUST;
#endif
		AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
		if(CFG_RES_I2S_MODULE == 0)
		{
			AudioIOSet.DataIOFunc = AudioI2S0_DataSet;
			AudioIOSet.LenGetFunc = AudioI2S0_DataSpaceLenGet;
		}
		else
		{
			AudioIOSet.DataIOFunc = AudioI2S1_DataSet;
			AudioIOSet.LenGetFunc = AudioI2S1_DataSpaceLenGet;
		}


		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2SOUT_SINK_NUM))
		{
			DBG("I2S out init error");
			return FALSE;
		}

		AudioI2sOutParamsSet();
		AudioCoreSinkEnable(AUDIO_I2SOUT_SINK_NUM);
		AudioCoreSinkAdjust(AUDIO_I2SOUT_SINK_NUM, TRUE);
	}
	else//sam add,20230221
	{
		I2S_SampleRateSet(CFG_RES_I2S_MODULE, sampleRate);
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].Sync = TRUE;		
	#endif
	}
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN

	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.FrameSize[0] * 2 ;
#ifdef	CFG_AUDIO_WIDTH_24BIT
	AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;//0,16bit,1:24bit
	AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
#endif
	if(!AudioCoreSinkIsInit(AUDIO_SPDIF_SINK_NUM))
	{
		mainAppCt.SPDIF_OUT_FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		mainAppCt.SPDIF_OUT_FIFO = (uint32_t*)osPortMalloc(mainAppCt.SPDIF_OUT_FIFO_LEN);//I2S fifo
		if(mainAppCt.SPDIF_OUT_FIFO != NULL)
		{
			memset(mainAppCt.SPDIF_OUT_FIFO, 0, mainAppCt.SPDIF_OUT_FIFO_LEN);
		}
		else
		{
			APP_DBG("malloc SPDIF_OUT_FIFO error\n");
			return FALSE;
		}

#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
		AudioIOSet.Adapt = STD;
#else
		AudioIOSet.Adapt = SRC_ONLY;
#endif
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = DefaultNet;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = sampleRate;//根据实际外设选择

		AudioIOSet.DataIOFunc = AudioSpdifTXDataSet;
		AudioIOSet.LenGetFunc = AudioSpdifTXDataSpaceLenGet;



		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_SPDIF_SINK_NUM))
		{
			DBG("spdif out init error");
			return FALSE;
		}
		AudioSpdifOutParamsSet();
		AudioCoreSinkEnable(AUDIO_SPDIF_SINK_NUM);
		AudioCoreSinkAdjust(AUDIO_SPDIF_SINK_NUM, TRUE);
	}
	else//sam add,20230221
	{
		SPDIF_SampleRateSet(SPDIF_OUT_NUM,sampleRate);
	#ifdef	CFG_AUDIO_WIDTH_24BIT
		AudioCore.AudioSink[AUDIO_SPDIF_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
		AudioCore.AudioSink[AUDIO_SPDIF_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
		AudioCore.AudioSink[AUDIO_SPDIF_SINK_NUM].Sync = TRUE;
	#endif
	}
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
	//注意 是否需要模式过滤，部分模式无需记忆和恢复
//	if(GetSystemMode() != ModeIdle)
	{
		BackupInfoUpdata(BACKUP_SYS_INFO);
	}
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
	if(!I2S_MixInit(FALSE))
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_I2S_MIX2_MODE
	if(!I2S_Mix2Init(FALSE))
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
	if(!LineInMixPlayInit())
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
	if(!UsbDevicePlayMixInit())
	{
		return FALSE;
	}
#endif

#if BT_SOURCE_SUPPORT
	{
		AudioCoreSink *Sink = &AudioCore.AudioSink[AUDIO_BT_SOURCE_SINK_NUM];

		//BT SOURCE复用DAC的PcmOutBuf，并且AUDIO_BT_SOURCE_SINK_NUM为16BIT数据
		//先保证DAC 24bit数据使用完以后，BT SOURCE才可以转16bit数据
#ifdef	CFG_AUDIO_WIDTH_24BIT
		Sink->BitWidth = PCM_DATA_24BIT_WIDTH;
		Sink->BitWidthConvFlag = 1;	//BT SOURCE转16bit数据,时序上保证DAC先使用完才可以轮到BT SOURCE使用
#endif
		Sink->Adapt	= STD;
		Sink->Sync = FALSE;
		Sink->Channels = 2;
		Sink->Net = DefaultNet;
		Sink->PcmOutBuf =  AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;//复用DAC的PcmOutBuf
		Sink->SpaceLenFunc = GetBtSourceFifoSpaceLength;
		Sink->DataSetFunc = SbcSourceEncode;
		AudioCoreSinkEnable(AUDIO_BT_SOURCE_SINK_NUM);
	}
#endif
	AudioDAC_Enable(DAC0);

	AudioEffect_CommunicationQueue_Init();
	return TRUE;
}

//释放公共通路，
void ModeCommonDeinit(void)
{
	SoftFlagRegister(SoftFlagAudioCoreSourceIsDeInit);
#ifdef CFG_RES_AUDIO_DAC0_EN
//	AudioCoreSinkDisable(AUDIO_DAC0_SINK_NUM);
	AudioDAC_Disable(DAC0);
	AudioDAC_FuncReset(DAC0);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC0_TX);
//	AudioDAC_Reset(DAC0);
	if(mainAppCt.DACFIFO != NULL)
	{
		osPortFree(mainAppCt.DACFIFO);
		mainAppCt.DACFIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_DAC0_SINK_NUM);
#endif
#if CFG_RES_MIC_SELECT
	AudioCoreSourceDisable(MIC_SOURCE_NUM);
	vTaskDelay(5);
	AudioADC_Disable(ADC1_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_ADC1_RX);
	if(mainAppCt.ADC1FIFO != NULL)
	{
		APP_DBG("ADC1FIFO\n");
		osPortFree(mainAppCt.ADC1FIFO);
		mainAppCt.ADC1FIFO = NULL;
	}
	AudioCoreSourceDeinit(MIC_SOURCE_NUM);
#endif
#if defined(CFG_RES_AUDIO_I2SOUT_EN)
	I2S_ModuleDisable(CFG_RES_I2S_MODULE);
	extern void RST_I2SModule(I2S_MODULE I2SModuleIndex);
	RST_I2SModule(CFG_RES_I2S_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE);

	if(mainAppCt.I2SFIFO != NULL)
	{
		APP_DBG("I2SFIFO\n");
		osPortFree(mainAppCt.I2SFIFO);
		mainAppCt.I2SFIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_I2SOUT_SINK_NUM);
#endif

#if defined(CFG_RES_AUDIO_SPDIFOUT_EN)
	SPDIF_ModuleDisable(SPDIF_OUT_NUM);
	DMA_InterruptFlagClear(SPDIF_OUT_DMA_ID, DMA_DONE_INT);
	DMA_InterruptFlagClear(SPDIF_OUT_DMA_ID, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(SPDIF_OUT_DMA_ID, DMA_ERROR_INT);
	DMA_ChannelDisable(SPDIF_OUT_DMA_ID);

	if(mainAppCt.SPDIF_OUT_FIFO != NULL)
	{
		APP_DBG("SPDIF OUT FIFO\n");
		osPortFree(mainAppCt.SPDIF_OUT_FIFO);
		mainAppCt.SPDIF_OUT_FIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_SPDIF_SINK_NUM);
#endif

//#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
	AudioCoreSourceDeinit(REMIND_SOURCE_NUM);
	RemindSoundAudioPlayEnd();
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	RemindMp3DecoderDeinit();
#endif
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
	I2S_MixDeinit();
#endif
#ifdef CFG_FUNC_I2S_MIX2_MODE
	I2S_Mix2Deinit();
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
	LineInMixPlayDeinit();
#endif
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
	UsbDevicePlayMixDeinit();
#endif

#if BT_SOURCE_SUPPORT
	AudioCoreSinkDisable(AUDIO_BT_SOURCE_SINK_NUM);
#endif

	AudioEffect_Parambin_Deinit();
	AudioEffect_CommunicationQueue_Deinit();
}

bool AudioIoCommonForHfp(uint16_t gain)
{
//	AudioCoreIO AudioIOSet;
//	uint16_t FifoLenStereo;
////	uint16_t FifoLenMono = SampleLen * 2 * 2;//单声到4倍大小于帧长，单位byte
//	uint32_t sampleRate;
//
//	if(!AudioEffect_Parambin_Init())
//	{
////		if(GetSysModeState(ModeSafe) == ModeStateSusend)
////		{
////			SetSysModeState(ModeSafe,ModeStateReady);
////		}
//		MessageContext		msgSend;
//		msgSend.msgId = MSG_DEVICE_SERVICE_MODE_ERROR;
//		MessageSend(GetMainMessageHandle(), &msgSend);
//		return FALSE;
//	}
//	sampleRate = AudioCoreMixSampleRateGet(DefaultNet);
//	APP_DBG("Systerm HFP SampleRate: %ld\n", sampleRate);
//
//	FifoLenStereo = AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2;//立体声8倍大小于帧长，单位byte
//
//	DefaultParamgsInit();	//refresh local hardware config params(just storage not set)
//
//#if CFG_RES_MIC_SELECT
//	AudioCoreSourceDisable(MIC_SOURCE_NUM);
//
//	if(!AudioCoreSourceIsInit(MIC_SOURCE_NUM))
//	{
//		//Mic1 analog  = Soure0.
////		AudioADC_AnaInit();
//		//AudioADC_VcomConfig(1);//MicBias en
//		// AudioADC_MicBias1Enable(1);
//		mainAppCt.ADCFIFO = (uint32_t*)osPortMalloc(FifoLenStereo);//ADC fifo
//		if(mainAppCt.ADCFIFO != NULL)
//		{
//			memset(mainAppCt.ADCFIFO, 0, FifoLenStereo);
//		}
//		else
//		{
//			APP_DBG("malloc ADCFIFO error\n");
//			return FALSE;
//		}
//
//		AudioADC_DynamicElementMatch(ADC1_MODULE, TRUE, TRUE);
////		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1);
////		AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2);
////		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 15, 4);//0db bypass
////		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 15, 4);
//
//		//Mic1   digital
//		AudioADC_DigitalInit(ADC1_MODULE, sampleRate,ADC_WIDTH_16BITS,(void*)mainAppCt.ADCFIFO,FifoLenStereo);
//
//	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
//        Clock_AudioMclkSel(AUDIO_DAC0, gCtrlVars.HwCt.DAC0Ct.dac_mclk_source > 2 ? (gCtrlVars.HwCt.DAC0Ct.dac_mclk_source - 1):gCtrlVars.HwCt.DAC0Ct.dac_mclk_source);
//	#else
//		gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source = Clock_AudioMclkGet(AUDIO_ADC1);
//	#endif
//		//Soure0.
//		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
//		AudioIOSet.Adapt = STD;
//		AudioIOSet.Sync = TRUE;
//		AudioIOSet.Channels = 1;
//		AudioIOSet.Net = DefaultNet;
//		AudioIOSet.DataIOFunc = AudioADC1_DataGet;
//		AudioIOSet.LenGetFunc = AudioADC1_DataLenGet;
//#ifdef CFG_AUDIO_WIDTH_24BIT
//		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;			//0,16bit,1:24bit
//		AudioIOSet.IOBitWidthConvFlag = 0;	//需要数据位宽不扩展
//#endif
//		if(!AudioCoreSourceInit(&AudioIOSet, MIC_SOURCE_NUM))
//		{
//			DBG("mic Source error");
//			return FALSE;
//		}
//		//MADC_MIC_PowerUP(SingleEnded);
//#ifdef CFG_ADCDAC_SEL_LOWPOWERMODE
//		AudioADC_AnaInit(ADC1_MODULE,CHANNEL_LEFT,MIC_LEFT,gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode,ADCLowEnergy,gain);
//#else
//		AudioADC_AnaInit(ADC1_MODULE,CHANNEL_LEFT,MIC_LEFT,gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode,ADCCommonEnergy,gain);
//#endif // CFG_ADCDAC_SEL_LOWPOWERMODE
//		AudioCoreSourceEnable(MIC_SOURCE_NUM);
//	}
//	else //采样率等 重配
//	{
//		AudioCoreSourceDisable(MIC_SOURCE_NUM);
////		AudioADC_AnaInit();
//		//AudioADC_VcomConfig(1);//MicBias en
////		AudioADC_MicBias1Enable(1);
//
//		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, MIC_LEFT, gain);
////		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, gain);
//
//		//Mic1	 digital
//		memset(mainAppCt.ADCFIFO, 0, FifoLenStereo);
//		AudioADC_DigitalInit(ADC1_MODULE, sampleRate,ADC_WIDTH_16BITS, (void*)mainAppCt.ADCFIFO, FifoLenStereo);
//
//	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
//        Clock_AudioMclkSel(AUDIO_ADC1, gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source > 2 ? (gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source - 1):gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source);
//	#else
//		gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source = Clock_AudioMclkGet(AUDIO_ADC1);
//	#endif
//	}
//#endif
//
//#ifdef CFG_FUNC_REMIND_SOUND_EN
//	if(!AudioCoreSourceIsInit(REMIND_SOURCE_NUM))
//	{
//		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
//
//		AudioIOSet.Adapt = SRC_ONLY;
//		AudioIOSet.SampleRate = sampleRate;//初始值
//		AudioIOSet.Sync = FALSE;
//		AudioIOSet.Channels = 1;
//		AudioIOSet.Net = DefaultNet;
//		AudioIOSet.DataIOFunc = RemindDataGet;
//		AudioIOSet.LenGetFunc = RemindDataLenGet;
//
//#ifdef	CFG_AUDIO_WIDTH_24BIT
//		AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
//		AudioIOSet.IOBitWidthConvFlag = 0;//需要数据进行位宽扩展
//#endif
//		if(!AudioCoreSourceInit(&AudioIOSet, REMIND_SOURCE_NUM))
//		{
//			DBG("remind source error!\n");
//			SoftFlagRegister(SoftFlagNoRemind);
//			return FALSE;
//		}
//#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
//		RemindMp3DecoderInit();
//#endif
//	}
//#endif
//
//
//#ifdef CFG_RES_AUDIO_DAC0_EN
//	AudioCoreSinkDisable(AUDIO_DAC0_SINK_NUM);
//	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
//	AudioIOSet.Depth = AudioCore.FrameSize[DefaultNet] * 2 ;
//#ifdef	CFG_AUDIO_WIDTH_24BIT
//	AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
//	AudioIOSet.IOBitWidthConvFlag = 1;//DAC 24bit ,sink最后一级输出时需要转变为24bi
//#endif
//	if(!AudioCoreSinkIsInit(AUDIO_DAC0_SINK_NUM))
//	{
//		mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
//		mainAppCt.DACFIFO = (uint32_t*)osPortMalloc(mainAppCt.DACFIFO_LEN);//DAC fifo
//		if(mainAppCt.DACFIFO != NULL)
//		{
//			memset(mainAppCt.DACFIFO, 0, mainAppCt.DACFIFO_LEN);
//		}
//		else
//		{
//			APP_DBG("malloc DACFIFO error\n");
//			return FALSE;
//		}
//		//sink0
//
//		AudioIOSet.Adapt = STD;
//		AudioIOSet.Sync = TRUE;
//		AudioIOSet.Channels = 2;
//		AudioIOSet.Net = DefaultNet;
//		AudioIOSet.DataIOFunc = AudioDAC0_DataSet;
//		AudioIOSet.LenGetFunc = AudioDAC0_DataSpaceLenGet;
//
//		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC0_SINK_NUM))
//		{
//			DBG("Dac init error");
//			return FALSE;
//		}
//
//		uint16_t BitWidth;
//	#ifdef	CFG_AUDIO_WIDTH_24BIT
//		BitWidth = 24;
//	#else
//		BitWidth = 16;
//	#endif
//		AudioDAC_Init((DACParamCt *)&DACDefaultParamCt,sampleRate,BitWidth, (void*)mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN, NULL, 0);
//
//	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
//        Clock_AudioMclkSel(AUDIO_ADC1, gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source > 2 ? (gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source - 1):gCtrlVars.HwCt.ADC1DigitalCt.adc_mclk_source);
//	#else
//		gCtrlVars.HwCt.DAC0Ct.dac_mclk_source = Clock_AudioMclkGet(AUDIO_DAC0);
//	#endif
//	}
//	else
//	{
//		AudioDAC0_SampleRateChange(sampleRate);
//		gCtrlVars.HwCt.DAC0Ct.dac_mclk_source = Clock_AudioMclkGet(AUDIO_DAC0);
//		APP_DBG("mode task io set\n");
//#ifdef	CFG_AUDIO_WIDTH_24BIT
//		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
//		AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
//#endif
//	}
//	AudioCoreSinkEnable(AUDIO_DAC0_SINK_NUM);
//#endif
//
//#ifdef CFG_FUNC_I2S_MIX_MODE
//	if(!I2S_MixInit(TRUE))
//	{
//		return FALSE;
//	}
//#endif
//
//
//#ifdef CFG_FUNC_I2S_MIX2_MODE
//	if(!I2S_Mix2Init(TRUE))
//	{
//		return FALSE;
//	}
//#endif
//
//#ifdef CFG_RES_AUDIO_I2SOUT_EN
//
//	AudioIOSet.Depth = AudioCore.FrameSize[DefaultNet] * 2 ;
//#ifdef	CFG_AUDIO_WIDTH_24BIT
//	AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
//	AudioIOSet.IOBitWidthConvFlag = 1;//不需要做位宽转换处理
//#endif
//	if(!AudioCoreSinkIsInit(AUDIO_I2SOUT_SINK_NUM))
//	{
//		mainAppCt.I2SFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
//		mainAppCt.I2SFIFO = (uint32_t*)osPortMalloc(mainAppCt.I2SFIFO_LEN);//I2S fifo
//
//		if(mainAppCt.I2SFIFO != NULL)
//		{
//			memset(mainAppCt.I2SFIFO, 0, mainAppCt.I2SFIFO_LEN);
//		}
//		else
//		{
//			APP_DBG("malloc I2SFIFO error\n");
//			return FALSE;
//		}
//
//#if((CFG_RES_I2S_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S_IN_SYNC_EN))
//		// Master 或不开微调
//		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
//			AudioIOSet.Adapt = STD;//SRC_ONLY
//		else
//			AudioIOSet.Adapt = SRC_ONLY;
//#else//slave
//		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
//			AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;
//		else
//			AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;
//#endif
//		AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
//		AudioIOSet.Channels = 2;
//		AudioIOSet.Net = DefaultNet;
//		AudioIOSet.HighLevelCent = 60;
//		AudioIOSet.LowLevelCent = 40;
//		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
//		if(CFG_RES_I2S_MODULE == 0)
//		{
//			AudioIOSet.DataIOFunc = AudioI2S0_DataSet;
//			AudioIOSet.LenGetFunc = AudioI2S0_DataSpaceLenGet;
//		}
//		else
//		{
//			AudioIOSet.DataIOFunc = AudioI2S1_DataSet;
//			AudioIOSet.LenGetFunc = AudioI2S1_DataSpaceLenGet;
//		}
//
//		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2SOUT_SINK_NUM))
//		{
//			DBG("I2S out init error");
//			return FALSE;
//		}
//
//		AudioI2sOutParamsSet();
//		AudioCoreSinkEnable(AUDIO_I2SOUT_SINK_NUM);
//		AudioCoreSinkAdjust(AUDIO_I2SOUT_SINK_NUM, TRUE);
//	}
//	else
//	{
//		I2S_SampleRateSet(CFG_RES_I2S_MODULE, sampleRate);
//	#ifdef	CFG_AUDIO_WIDTH_24BIT
//		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidth = AudioIOSet.IOBitWidth;
//		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].BitWidthConvFlag = AudioIOSet.IOBitWidthConvFlag;
//		AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].Sync = FALSE;
//	#endif
//	}
//#endif
//
//	AudioDAC_Enable(DAC0);
//	return TRUE;
}

//各模式下的通用消息处理, 共有的提示音在此处理，因此要求调用次API前，确保APP running状态。避免解码器没准备好。
void CommonMsgProccess(uint16_t Msg)
{
	uint32_t sample_rate, frame_size = 0;

#if defined(CFG_FUNC_DISPLAY_EN)
	MessageContext	msgSend;
#endif
	if(SoftFlagGet(SoftFlagDiscDelayMask) && Msg == MSG_NONE)
	{
		Msg = MSG_BT_STATE_DISCONNECT;
	}

	switch(Msg)
	{
		case MSG_MENU://菜单键
			APP_DBG("menu key\n");
			AudioPlayerMenu();
			break;
#ifdef VD51_REDMINE_13199
		case MSG_RGB_MODE:
			if(IsRgbLedModeMenuExit())
				RgbLedModeMenuEnter();
			else
				RgbLedModeMenuExit();
			break;
		case MSG_MUTE:
			if(!IsRgbLedModeMenuExit())
			{
				RgbLedModeSet();
			#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
				break;
			}
#else
		case MSG_MUTE:
#endif			
			APP_DBG("MSG_MUTE\n");
			#ifdef  CFG_APP_HDMIIN_MODE_EN
			extern HDMIInfo         *gHdmiCt;
			if(GetSystemMode() == ModeHdmiAudioPlay)
			{
				if(IsHDMISourceMute() == TRUE)
					HDMISourceUnmute();
				else
					HDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteStatus = IsHDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteflag = 2;
			}
			else
			#endif
			{
				HardWareMuteOrUnMute();
			}
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MUTE;
            MessageSend(GetSysModeMsgHandle(), &msgSend);
			#endif
			break;
#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_PLAY_SYNC_VOLUME_CHANGED:
			APP_DBG("MSG_BT_PLAY_SYNC_VOLUME_CHANGED\n");
#if (BT_AVRCP_VOLUME_SYNC)
			AudioMusicVolSet(GetBtSyncVolume());
#endif
			break;
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
		case MSG_EFFECTMODE_SWITCH:
			APP_DBG("MSG_EFFECTMODE_SWITCH\n");
			AudioEffect_Parambin_EffectModeAutoSwitch();
			if(AudioEffect_Parambin_GetEffectListInfoByIndex(mainAppCt.effect_flow_index, mainAppCt.effect_param_mode_index, &sample_rate, &frame_size))
			{
				if((AudioCoreMixSampleRateGet(DefaultNet) == sample_rate) && (AudioCoreFrameSizeGet(DefaultNet) == frame_size))
				{
					AudioEffect_EffectMode_Refresh();
				}
				else  //sample_rate & frame_size change, need reinit all
				{
					APP_DBG("sampleRate or framesize change ,sysMode reboot!\n");
//					SysCurModeReboot();
				}
			}
			else
			{
				APP_DBG("!!!prambin error, please check!!!\n");
			}
			gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
			break;
		case MSG_EFFECT_REFRESH:
			APP_DBG("MSG_EFFECT_REFRESH\n");
			if(AudioEffect_Parambin_GetEffectListInfoByIndex(mainAppCt.effect_flow_index, mainAppCt.effect_param_mode_index, &sample_rate, &frame_size))
			{
				if((AudioCoreMixSampleRateGet(DefaultNet) == sample_rate) && (AudioCoreFrameSizeGet(DefaultNet) == frame_size))
				{
					AudioEffect_EffectMode_Refresh();
				}
				else  //sample_rate & frame_size change, need reinit all
				{
					APP_DBG("sampleRate or framesize change ,sysMode reboot!\n");
//					SysCurModeReboot();
				}
			}
			else
			{
				APP_DBG("!!!prambin error, please check!!!\n");
			}
			break;
#endif
		#ifdef CFG_FUNC_RTC_EN
		case MSG_RTC_SET_TIME:
			RTC_ServiceModeSelect(0,0);
			break;

		case MSG_RTC_SET_ALARM:
			RTC_ServiceModeSelect(0,1);
			break;

		case MSG_RTC_DISP_TIME:
			RTC_ServiceModeSelect(0,2);
			break;

		case MSG_RTC_UP:
			RTC_RtcUp();
			break;

		case MSG_RTC_DOWN:
			RTC_RtcDown();
			break;

		#ifdef CFG_FUNC_SNOOZE_EN
		case MSG_RTC_SNOOZE:
			if(mainAppCt.AlarmRemindStart)
			{
				mainAppCt.AlarmRemindCnt = 0;
				mainAppCt.AlarmRemindStart = 0;
				mainAppCt.SnoozeOn = 1;
				mainAppCt.SnoozeCnt = 0;
			}
			break;
		#endif

		#endif //end of  CFG_FUNC_RTC_EN

		case MSG_REMIND1:
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			RemindSoundServiceItemRequest(SOUND_REMIND_SHANGYIS,FALSE);
			#endif
			break;

		case MSG_DEVICE_SERVICE_BATTERY_LOW:
			//RemindSound request
			APP_DBG("MSG_DEVICE_SERVICE_BATTERY_LOW\n");
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			RemindSoundServiceItemRequest(SOUND_REMIND_DLGUODI, FALSE);
			#endif
			break;

#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_OPEN_ACCESS:
			if (GetBtManager()->btAccessModeEnable != BT_ACCESSBLE_GENERAL)
			{
				GetBtManager()->btAccessModeEnable = BT_ACCESSBLE_GENERAL;
				BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
				DBG("open bt access\n");
			}
			break;
			
		//蓝牙连接断开消息,用于提示音
		case MSG_BT_STATE_CONNECTED:
			APP_DBG("[BT_STATE]:BT Connected...\n");
			//异常回连过程中，不提示连接断开提示音
			SetBtUserState(BT_USER_STATE_CONNECTED);

#ifdef BT_ACCESS_MODE_SET_BY_POWER_ON_TIMEOUT
			btManager.btvisibilityDelayOn = FALSE;
#endif
			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

			if(btManager.btDutModeEnable)
				break;

			//if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(RemindSoundServiceItemRequest(SOUND_REMIND_CONNECT, REMIND_PRIO_SYS|REMIND_ATTR_NEED_HOLD_PLAY))
				{
					if(!SoftFlagGet(SoftFlagWaitBtRemindEnd)&&SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						SoftFlagRegister(SoftFlagWaitBtRemindEnd);
					}
				}
				else
				#endif
				{
					if(SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						MessageContext		msgSend;
						SoftFlagDeregister(SoftFlagDelayEnterBtHf);

						msgSend.msgId = MSG_DEVICE_SERVICE_ENTER_BTHF_MODE;
						MessageSend(GetMainMessageHandle(), &msgSend);
					}
				}
			}
			break;

		case MSG_BT_STATE_DISCONNECT:
			APP_DBG("[BT_STATE]:BT Disconnected...\n");
			SoftFlagDeregister(SoftFlagDiscDelayMask);
			SetBtUserState(BT_USER_STATE_DISCONNECTED);
			
			if(btManager.btDutModeEnable)
				break;

			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

			//异常回连过程中，不提示连接断开提示音
			//if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(GetSystemMode() != ModeIdle)
				{
					RemindSoundServiceItemRequest(SOUND_REMIND_DISCONNE, REMIND_PRIO_SYS|REMIND_ATTR_NEED_HOLD_PLAY);
				}
				#endif
			}
			break;
#endif

		default:
			#ifdef CFG_FUNC_DISPLAY_EN
			//display
			Display(Msg);
			#endif
			#ifdef CFG_ADC_LEVEL_KEY_EN
			AdcLevelMsgProcess(Msg);
			#endif

//			refresh_addr = 0;//AudioMsgProccess(Msg);
			break;
	}
}
