#ifndef __LM1971_CTRL_H__
#define __LM1971_CTRL_H__

#define IOC_LM1971DEV_MAGIC  'B'
#define LM1971_SET_VOLUME   _IO(IOC_LM1971DEV_MAGIC, 10)
#define LM1971_SET_MUTE         _IO(IOC_LM1971DEV_MAGIC, 11)
#define LM1971_SET_MAX_VOLUME   _IO(IOC_LM1971DEV_MAGIC, 12)

#define PA_AND_CALL_MIC_LM1971_LOAD	4
#define COP_SPK_LM1971_LOAD	2

#define OCC_SPK_LM1971_LOAD	1
#define OCC_MIC_LM1971_LOAD	3
//#define CALL_MIC_LM1971_LOAD	2 // IZMIR: 4

#define LM1971_VOLUME_MAX_VALUE	0
#define LM1971_VOLUME_MIN_VALUE	96 //80

int cop_occ_pa_mic_volume_set(unsigned char vol_value);
int cop_occ_pa_mic_volume_set_max(void);
int cop_occ_pa_mic_volume_set_mute(void);

#endif // __LM1971_CTRL_H__
