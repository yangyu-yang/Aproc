/**
 **************************************************************************************
 * @file    audio_vol.h
 * @brief   audio syetem vol set here
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-1-7 15:42:47$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __AUDIO_VOL_H__
#define __AUDIO_VOL_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "app_config.h"
#include "mode_task.h"

#if (BT_AVRCP_VOLUME_SYNC) && defined(CFG_APP_BT_MODE_EN)
	#define CFG_PARA_MAX_VOLUME_NUM		        (16)	//SDK 16 级音量,针对iphone手机蓝牙音量同步功能定制，音量表16级能一一对应手机端音量级别
	#define CFG_PARA_SYS_VOLUME_DEFAULT			(12)	//SDK默认音量
#else
	#define CFG_PARA_MAX_VOLUME_NUM		        (32)	//SDK 32 级音量
	#define CFG_PARA_SYS_VOLUME_DEFAULT			(32)	//SDK默认音量
#endif

void AudioAPPDigitalGianProcess(SysModeNumber AppMode);
bool IsAudioPlayerMute(void);
void AudioPlayerMenu(void);
void AudioPlayerMenuCheck(void);
void AudioMusicVolSet(uint8_t musicVol);
void AudioMusicVolPureSet(uint8_t musicVol);
void AudioHfVolSet(uint8_t HfVol);

uint8_t AudioMusicVolGet(void);
void AudioMusicVolDown(void);
void AudioMusicVolUp(void);
void SystemVolUp(void);
void SystemVolDown(void);
void SystemVolSet(void);
void SystemVolSync(void);

uint8_t BtAbsVolume2VolLevel(uint8_t absValue);
uint8_t BtLocalVolLevel2AbsVolme(uint8_t localValue);
void AudioMicVolDown(void);
void AudioMicVolUp(void);
void AudioMicVolSet(uint8_t micVol);
void HardWareMuteOrUnMute(void);

#ifdef  CFG_APP_HDMIIN_MODE_EN
void HDMISourceMute(void);
void HDMISourceUnmute(void);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif

