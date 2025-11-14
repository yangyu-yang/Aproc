/**
 **************************************************************************************
 * @file    ctrlvars.c
 * @brief   Control Variables Definition
 * 
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include <stdint.h>
#include "debug.h"
#include "app_config.h"
#include "clk.h"
#include "ctrlvars.h"
#include "spi_flash.h"
#include "timeout.h"
#include "delay.h"
#include "breakpoint.h"
#include "nds32_intrinsic.h"
#include "audio_adc.h"
#include "dac.h"
#include "main_task.h"

ControlVariablesContext gCtrlVars;
#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
SyncModuleContext gSyncModule;
#endif

static const uint32_t SupportSampleRateList[13]={
	8000,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000,
//////i2s//////////////////
    88200,
    96000,
    176400,
    192000,
};

const uint16_t HPCList[3]={
	0xffe, //  48k 20Hz  -1.5db
	0xFFC, //  48k 40Hz  -1.5db
	0xFFD, //  32k 40Hz  -1.5db
};

uint16_t SampleRateIndexGet(uint32_t SampleRate)
{
	volatile uint32_t i;
	for(i = 0; i < sizeof(SupportSampleRateList)/sizeof(SupportSampleRateList[0]); i++)
	{
		if(SampleRate == SupportSampleRateList[i])
		{
			break;
		}
	}
	if(i == 13)
	{
		i =0;
	}
	return i;
}
//系统变量初始化
void CtrlVarsInit(void)
{
	APP_DBG("[SYS]: Loading control vars as default\n");

	memset(&gCtrlVars,  0, sizeof(gCtrlVars));
	//音频系统硬件模块变量初始化
	DefaultParamgsInit();
}

//各个模块默认参数设置函数
void DefaultParamgsInit(void)
{
//	memset(&gCtrlVars,  0, sizeof(gCtrlVars));
	//for system control 0x01
//	gCtrlVars.AutoRefresh = AutoRefresh_ALL_PARA;

	memcpy(&gCtrlVars.HwCt, AudioEffect_Parambin_GetCurHardwareConfig(), sizeof(gCtrlVars.HwCt));

    //system define
	gCtrlVars.sample_rate		= AudioCoreMixSampleRateGet(DefaultNet);
	gCtrlVars.sample_rate_index = SampleRateIndexGet(gCtrlVars.sample_rate);

	//scramble默认开启，设置成POS_NEG
	gCtrlVars.HwCt.DAC0Ct.dac_scramble = POS_NEG + 1;
	//L R数据反，默认交换数据
#if (CFG_CHIP_SEL == CFG_CHIP_BP1524A1) || (CFG_CHIP_SEL == CFG_CHIP_BP1524A2) || (CFG_CHIP_SEL == CFG_CHIP_AP1524A1) || (CFG_CHIP_SEL == CFG_CHIP_BP1564A1) || (CFG_CHIP_SEL == CFG_CHIP_BP1564A2)
	gCtrlVars.HwCt.DAC0Ct.dac_out_mode = MODE1;
#else
	gCtrlVars.HwCt.DAC0Ct.dac_out_mode = MODE0;
#endif
}

void AudioLineSelSet(int8_t ana_input_ch)
{
	//模拟通道先配置为NONE，防止上次配置通道残留，然后再配置需要的模拟通道 

	AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,  LINEIN_NONE);
	AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT, LINEIN_NONE);

    //AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	//AudioADC_MicBias1Enable(1);
	//AudioADC_DynamicElementMatch(ADC0_MODULE, TRUE, TRUE);

	//--------------------line 1-----------------------------------------//
	if(ANA_INPUT_CH_LINEIN1 == ana_input_ch)
	{
		if(gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_en)
		{
			//APP_DBG("LINE 1L En\n");
			AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,LINEIN1_LEFT);
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT,  LINEIN1_LEFT,  31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
		}
		if(gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_en)
		{
			//APP_DBG("LINE 1R En\n");
			AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT,LINEIN1_RIGHT);
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT,  31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_gain);
		}
	}

	//--------------------line 2-----------------------------------------//
	if(ANA_INPUT_CH_LINEIN2 == ana_input_ch)
	{
		if(gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_en)
		{
			//APP_DBG("LINE 2L En\n");
			AudioADC_PGASel(ADC0_MODULE, CHANNEL_LEFT,LINEIN2_LEFT);
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT,  LINEIN2_LEFT,  31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
		}
		if(gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_en)
		{
			//APP_DBG("LINE 2R En\n");
			AudioADC_PGASel(ADC0_MODULE, CHANNEL_RIGHT,LINEIN2_RIGHT);
			AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN2_RIGHT,  31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_gain);
		}
	}
}

void AudioLine3MicSelect(void)
{
	if(gCtrlVars.HwCt.ADC1PGACt.pga_mic_enable)
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT,MIC_LEFT);
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT,  MIC_LEFT, 31 - gCtrlVars.HwCt.ADC1PGACt.pga_mic_gain);//0db bypass
//		gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode = Single;
	}
	else
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT,LINEIN_NONE);
	}
}

void AudioAnaChannelSet(int8_t ana_input_ch)
{
	gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_en = 0;
	gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_en = 0;

	if(ANA_INPUT_CH_LINEIN1 == ana_input_ch || ANA_INPUT_CH_LINEIN2 == ana_input_ch)
	{
		gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_en = 1;
		gCtrlVars.HwCt.ADC0PGACt.pga_aux_r_en = 1;
		AudioLineSelSet(ana_input_ch);
	}
}

//音效参数更新之后同步更新模拟Gain和数字Vol
//只更新增益相关参数，其他参数比如通道选择不会同步更新，必须由SDK代码来实现
void AudioCodecGainUpdata(void)
{
//	uint16_t dac_l, dac_r;
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_LEFT,  LINEIN1_LEFT,  31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
	AudioADC_PGAGainSet(ADC0_MODULE, CHANNEL_RIGHT, LINEIN1_RIGHT,  31 - gCtrlVars.HwCt.ADC0PGACt.pga_aux_l_gain);
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT,  MIC_LEFT, 31 - gCtrlVars.HwCt.ADC1PGACt.pga_mic_gain);//0db bypass

	AudioADC_HighPassFilterConfig(ADC0_MODULE, HPCList[gCtrlVars.HwCt.ADC0DigitalCt.adc_hpc]);
	AudioADC_HighPassFilterSet(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.adc_dc_blocker_en);
	AudioADC_ChannelSwap(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.adc_lr_swap);
	AudioADC_VolSet(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.adc_dig_l_vol, gCtrlVars.HwCt.ADC0DigitalCt.adc_dig_l_vol);
	AudioADC_DigitalMute(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.adc_mute, gCtrlVars.HwCt.ADC0DigitalCt.adc_mute);

	AudioADC_HighPassFilterConfig(ADC1_MODULE, HPCList[gCtrlVars.HwCt.ADC1DigitalCt.adc_hpc]);
	AudioADC_HighPassFilterSet(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.adc_dc_blocker_en);
	AudioADC_ChannelSwap(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.adc_lr_swap);
	AudioADC_VolSet(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.adc_dig_l_vol, gCtrlVars.HwCt.ADC1DigitalCt.adc_dig_l_vol);
	AudioADC_DigitalMute(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.adc_mute, gCtrlVars.HwCt.ADC1DigitalCt.adc_mute);

	AudioDAC_VolSet(DAC0, gCtrlVars.HwCt.DAC0Ct.dac_dig_l_vol, gCtrlVars.HwCt.DAC0Ct.dac_dig_r_vol);
#ifndef CFG_AUDIO_WIDTH_24BIT
	AudioDAC_DoutModeSet(DAC0, gCtrlVars.HwCt.DAC0Ct.dac_out_mode, WIDTH_16_BIT);
#else
	AudioDAC_DoutModeSet(DAC0, gCtrlVars.HwCt.DAC0Ct.dac_out_mode, WIDTH_24_BIT_2);
#endif
	if(gCtrlVars.HwCt.DAC0Ct.dac_scramble == 0)
	{
		AudioDAC_ScrambleDisable(DAC0);
	}
	else
	{
		AudioDAC_ScrambleEnable(DAC0);
		AudioDAC_ScrambleModeSet(DAC0,(SCRAMBLE_MODULE)gCtrlVars.HwCt.DAC0Ct.dac_scramble - 1);
	}
	if(gCtrlVars.HwCt.DAC0Ct.dac_dither)
	{
		AudioDAC_DitherEnable(DAC0);
	}
	else
	{
		AudioDAC_DitherDisable(DAC0);
	}
}

#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
__attribute__((section(".driver.isr"))) void Timer4Interrupt(void)
{
	gSyncModule.gSyncTimerCnt++;
	Timer_InterruptFlagClear(SYNC_TIMER_INDEX, UPDATE_INTERRUPT_SRC);
	SyncModule_Get();
}

void SyncModule_Init(void)
{
	GIE_ENABLE();
	NVIC_EnableIRQ(Timer4_IRQn);
	gSyncModule.gSyncTimerCnt = 0;
	gSyncModule.gRefreshFlag = 0;
	gSyncModule.gClearDoneFlag = 0;
	gSyncModule.dat = 0;
	gSyncModule.clkRatio = 4;
	Clock_SyncCtrl_Set(I2S_BCLK_SEL, 1);
	Clock_SyncCtrl_Set(MDAC_MCLK_SEL, 1);
	Timer_Config(SYNC_TIMER_INDEX, SYNC_TIMER_OUT_VALUE, 0);
	Timer_InterrputSrcEnable(SYNC_TIMER_INDEX, UPDATE_INTERRUPT_SRC);
	Timer_Start(SYNC_TIMER_INDEX);
	Clock_SyncCtrl_Set(START_EN, 1);
	Clock_SyncCtrl_Set(UPDATE_DONE_CLR, 1);
}

void SyncModule_Reset(void)
{
	uint32_t SampleRate = I2S_SampleRateGet(CFG_RES_I2S_MODULE);
	if(IsSelectMclkClk1(SampleRate))
	{
		gSyncModule.clkRatio = (AUDIO_PLL_CLK1_FREQ * 4 / I2S_SampleRateGet(CFG_RES_I2S_MODULE)) / 64;
	}
	else
	{
		gSyncModule.clkRatio = (AUDIO_PLL_CLK2_FREQ * 4 / I2S_SampleRateGet(CFG_RES_I2S_MODULE)) / 64;
	}
	DBG("BCLK_MCLK_RATIO = %d\n", gSyncModule.clkRatio);
	Clock_SyncCtrl_Set(I2S_BCLK_SEL, 0);
	Clock_SyncCtrl_Set(MDAC_MCLK_SEL, 0);
	Clock_SyncCtrl_Set(START_EN, 0);
	Clock_SyncCtrl_Set(UPDATE_DONE_CLR, 0);
	gSyncModule.gSyncTimerCnt = 0;
	gSyncModule.gRefreshFlag = 0;
	gSyncModule.gClearDoneFlag = 0;
	gSyncModule.dat = 0;
	Clock_SyncCtrl_Set(I2S_BCLK_SEL, 1);
	Clock_SyncCtrl_Set(MDAC_MCLK_SEL, 1);
	Clock_SyncCtrl_Set(START_EN, 1);
	Clock_SyncCtrl_Set(UPDATE_DONE_CLR, 1);
}

void SyncModule_Get(void)
{
	if(gSyncModule.gRefreshFlag)//上次处理还没有处理完，这次不再使用，准备使用下次
		return;

	//如果再次进来后，还是没清除，则这次结果不用
//	if(SREG_CLK_SYNC_CTRL.CLK_SYNC_CNT_UPDATE_DONE)
//	{
//		gClearDoneFlag = 1;
//		return;
//	}
	Clock_SyncCtrl_Set(UPDATE_EN, 1);
	WaitUs(3);
	gSyncModule.gI2sBclkCnt = Clock_ClkCnt_Get(I2S_BCLK_CNT1);
	gSyncModule.gI2sBclkCnt = (gSyncModule.gI2sBclkCnt << 32)  + Clock_ClkCnt_Get(I2S_BCLK_CNT0);
	gSyncModule.gDacMclkCnt = Clock_ClkCnt_Get(MDAC_MCLK_CNT1);
	gSyncModule.gDacMclkCnt = (gSyncModule.gDacMclkCnt << 32)  + Clock_ClkCnt_Get(MDAC_MCLK_CNT0);
	gSyncModule.gRefreshFlag = 1;
}

void SyncModule_Process(void)
{
	if(gSyncModule.gRefreshFlag || gSyncModule.gClearDoneFlag)
	{
		uint16_t defVal;

		if(gSyncModule.gRefreshFlag)
		{
			gSyncModule.gMClkFromBCLK = gSyncModule.gI2sBclkCnt  * 1000000 / SYNC_TIMER_OUT_VALUE * gSyncModule.clkRatio / gSyncModule.gSyncTimerCnt;
			gSyncModule.gMClkFromDPLL = gSyncModule.gDacMclkCnt  * 1000000 / SYNC_TIMER_OUT_VALUE / gSyncModule.gSyncTimerCnt;
//			DBG("gMClkFromBCLK = %f, gMClkFromDPLL = %f,  ", gSyncModule.gMClkFromBCLK*1.0, gSyncModule.gMClkFromDPLL*1.0);

			if(gSyncModule.gMClkFromBCLK > gSyncModule.gMClkFromDPLL)
			{
				defVal = gSyncModule.gMClkFromBCLK - gSyncModule.gMClkFromDPLL;
				if(defVal > 100)
				{
					gSyncModule.dat = 10;
					*(uint32_t *)0x40026008 += gSyncModule.dat;//defVal/10 * 5 + 1;
				}
				else
				{
					gSyncModule.dat = 5;
					if(defVal > 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
					}
					else if(defVal < 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
						else
						{
							gSyncModule.dat = 1;
						}
					}
					*(uint32_t *)0x40026008 += gSyncModule.dat;
				}

				gSyncModule.defVal_bak = defVal;
				//DBG("+%d, +%d, 0x%08x\n", defVal, defVal > 50?10:1, *(uint32_t *)0x40026008);
//				DBG("+%d, +%d, 0x%08x\n", defVal, gSyncModule.dat, *(uint32_t *)0x40026008);
			}
			else if(gSyncModule.gMClkFromBCLK < gSyncModule.gMClkFromDPLL)
			{
				defVal = gSyncModule.gMClkFromDPLL - gSyncModule.gMClkFromBCLK;
				gSyncModule.dat = 0;
				if(defVal > 100)
				{
					gSyncModule.dat = 10;
					*(uint32_t *)0x40026008 -= gSyncModule.dat;//defVal/10 * 5 + 1;
				}
				else
				{
					gSyncModule.dat = 5;
					if(defVal > 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
					}
					else if(defVal < 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
						else
						{
							gSyncModule.dat = 1;
						}
					}
					*(uint32_t *)0x40026008 -= gSyncModule.dat;
				}
				gSyncModule.defVal_bak = defVal;
				//DBG("-%d, -%d, 0x%08x\n", defVal, defVal > 50?10:1, *(uint32_t *)0x40026008);
//				DBG("-%d, -%d, 0x%08x\n", defVal, gSyncModule.dat, *(uint32_t *)0x40026008);
			}
		}

//		DBG("\n");
		gSyncModule.gRefreshFlag = 0;
		gSyncModule.gClearDoneFlag = 0;
	}
}
#endif

