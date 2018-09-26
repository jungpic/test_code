#ifndef __AUDIO_CTRL_H__
#define __AUDIO_CTRL_H__

#define SIMPLE_DELAY_CTR	8000//80000

#define VS10XX_WRITE_SIZE	32

//#define ENCODING_LIMIT	0x30 // CBR 32kbps
//#define ENCODING_LIMIT	0x60 // CBR 64kbps
//#define ENCODING_LIMIT	0xC0 // CBR 128kbps
//#define ENCODING_LIMIT	240 // CBR 160kbps
//#define ENCODING_LIMIT	0x120 // 48000Hz, CBR 192kbps

#define DEFAULT_COP_SPK_VOLUME   0
#define DEFAULT_COP_MIC_VOLUME	 0

#define DEFAULT_MIC_VOL_LEVEL	50
#define DEFAULT_SPK_VOL_LEVEL	50

#define SAVE_COP_SPK_MIC_VOLUME	0

#undef PEI_MONITORING_SAVE

int audio_init(void);
void audio_exit(void);

int check_codec_HW_error_1(void);
int check_codec_HW_error_2(void);
int check_codec_HW_error_3(void);
int check_codec_HW_error_4(void);

int decoding_start(void);
int decoding_stop(void);

int vs10xx_enc0_buffer_clear(void);
int vs10xx_enc1_buffer_clear(void);

void __cop_pa_mic_codec_swreset(void);
void __cop_pa_mic_codec_start(void);
void cop_pa_mic_codec_start(void);
void cop_pa_mic_codec_stop(void);
void vs10xx_dec_queue_flush(void);

void cop_pa_mic_codec_init(void);	// Passive PA & OCC PA
//void cop_occ_codec_init(void);
void cop_monitoring_codec_init(void);

/* <<<< VOIP */
void cop_voip_call_codec_swreset(void);
void cop_voip_call_codec_init(void);
void cop_voip_call_codec_start(void);
void cop_voip_call_codec_stop(void);
int discard_voip_call_mic_encoding_data(void);
int get_mic_enc_wav_frame(void);//unsigned char *wav_buf);
//void write_voip_call_buf_to_dev(unsigned char *dec_buf, unsigned int dec_size);
void print_voip_call_codec_setting(void);
void reset_voip_frame_info(void);
/* VOIP >>>> */

void __cop_occ_pa_and_occ_call_codec_swreset(void);
void __cop_occ_pa_and_occ_call_codec_start(void);
void cop_occ_pa_and_occ_call_codec_start(void);

void cop_occ_pa_and_occ_call_codec_stop(void);

int cop_call_mic_volume_mute_set(void);

int set_cop_call_mic_audio_switch_on(void);
int cop_call_mic_volume_set(void);

void cop_monitoring_encoding_start(void);
void cop_monitoring_encoding_stop(void);



int cop_speaker_volume_mute_set(void);
int cop_speaker_volume_set(void);

int fire_cop_speaker_volume_set(void);

int deadman_cop_speaker_volume_mute_set(void);
int deadman_cop_speaker_volume_set(void);

int cop_pa_mic_volume_mute_set(void);
//int cop_pa_mic_volume_set(unsigned char vol_value);
int cop_pa_and_call_mic_volume_set();
void mic_volume_step_up(int add_val, int active);
void mic_volume_step_down(int sub_val, int active);
void inspk_volume_step_up(int add_val, int active);
void inspk_volume_step_down(int sub_val, int active);

int occ_speaker_volume_mute_set(void);
int occ_speaker_volume_set(unsigned char vol_value);

int occ_mic_volume_set(unsigned char atten);
int occ_mic_volume_mute_set(void);

void spk_volume_step_up(int add_val, int active);
void spk_volume_step_down(int sub_val, int active);
void inspk_volume_step_up(int add_val, int active);
void outspk_volume_step_down(int sub_val, int active);

void raw_volume_step_up(int active);

int running_call_audio_sending_to_occ_out(int stop);

int running_cop_pa_and_call_mic_audio_sending(void);
int running_call_audio_sending(int passive_also_on, int mon_also_on, int cantalk);

int running_voip_call_audio_sending(void);

int running_occ_pa_and_occ_call_audio_sending(int cantalk, int is_occ_pa, int is_occ_call);
int running_call_mic_audio_sending(int passive_also_on);

int __discard_cop_pa_and_call_mic_encoding_data(void);
int __discard_occ_pa_and_call_mic_encoding_data(void);
int __discard_occ_pa_mic_encoding_data_for_test(void);
int __discard_monitoring_encoding_data(void);

int running_bcast_recv_and_dec(int type, char *buf, int len, int run, int stop, int check_header);

int get_dec_recv_len_remain(void);

void occ_pa_packet_self_play(int len, int stop);

int running_monitoring_audio_sending(int file_fd, int send_en);

void set_check_ssrc_valid(int set);

void start_play_beep(void);
void stop_play_beep(void);
int running_play_beep(void);

void start_play_fire_alarm(void);
void stop_play_fire_alarm(void);
int running_play_fire_alarm(void);

void start_play_deadman_alarm(void);
void stop_play_deadman_alarm(void);
int running_play_deadman_alarm(void);

void set_check_different_ssrc_valid(int set);
void clear_old_rtp_seq(void);
void clear_old_ssrc(void);
int get_one_flame_play_time(void);
unsigned int get_save_start_ssrc(void);

int file_open_mon_save(void);
int file_close_mon_save(int fd);

#define dVS10xx_SCI_MODE					0x00
#define dVS10xx_SCI_STATUS					0x01
#define dVS10xx_SCI_BASS					0x02
#define dVS10xx_SCI_CLOCKF					0x03
#define dVS10xx_SCI_DECODE_TIME				0x04
#define dVS10xx_SCI_AUDATA					0x05
#define dVS10xx_SCI_WRAM					0x06
#define dVS10xx_SCI_WRAMADDR				0x07
#define dVS10xx_SCI_HDAT0					0x08
#define dVS10xx_SCI_HDAT1					0x09
#define dVS10xx_SCI_AIADDR					0x0A
#define dVS10xx_SCI_VOL						0x0B
#define dVS10xx_SCI_AICTRL0					0x0C
#define dVS10xx_SCI_AICTRL1					0x0D
#define dVS10xx_SCI_AICTRL2					0x0E
#define dVS10xx_SCI_AICTRL3					0x0F

/*	SCI_MODE(RW)	*/
#define dVS10xx_SM_CLK_RANGE				(1<<15)
#define dVS10xx_SM_LINE1					(1<<14)
#define dVS10xx_SM_ADPCM					(1<<12)
#define dVS10xx_SM_SDINEW					(1<<11)
#define dVS10xx_SM_SDISHARE					(1<<10)
#define dVS10xx_SM_SDIORD					(1<<9)
#define dVS10xx_SM_DACT						(1<<8)
#define dVS10xx_SM_EARSPEAKER_HI			(1<<7)
#define dVS10xx_SM_STREAM					(1<<6)
#define dVS10xx_SM_TESTS					(1<<5)
#define dVS10xx_SM_EARSPEAKER_LO			(1<<4)
#define dVS10xx_SM_CANCEL					(1<<3)
#define dVS10xx_SM_RESET					(1<<2)
#define dVS10xx_SM_LAYER12					(1<<1)
#define dVS10xx_SM_DIFF						(1<<0)

/*	SCL_STATUS(RW)	*/
#define dVS10xx_SS_DO_NOT_JUMP				(1<<15)
#define dVS10xx_SS_SWING(n)					((n)<<12)
#define dVS10xx_SS_VCM_OVERLOAD				(1<<11)
#define dVS10xx_SS_VCM_DISABLE				(1<<10)
#define dVS10xx_SS_VER(n)					((n)<<4)
#define dVS10xx_SS_APDOWN2					(1<<3)
#define dVS10xx_SS_APDOWN1					(1<<2)
#define dVS10xx_SS_AD_CLOCK					(1<<1)
#define dVS10xx_SS_REFERENCE_SEL			(1<<0)

/*	SCL_BASS(RW)	*/
#define dVS10xx_ST_AMPLITUDE(n)				((n)<<12)
#define dVS10xx_ST_FREQLIMIT(n)				((n)<<8)
#define dVS10xx_SB_AMPLITUDE(n)				((n)<<4)
#define dVS10xx_SB_FREQLIMIT(n)				((n)<<0)

/*	SCL_CLOCKF(RW)	*/
#define dVS10xx_SC_MULT(n)					((n)<<13)
#define dVS10xx_SC_ADD(n)					((n)<<11)
#define dVS10xx_SC_FREQ(n)					((n)<<0)

/*	dVS10xx_SCI_WRAMADDR(RW) - Extra Parameter	*/
#define dVS1053_INT_ENABLE					0xC01A

#define dVS10xx_EP_version					0x1E02
#define dVS10xx_EP_config1					0x1E03
#define dVS10xx_EP_playSpeed				0x1E04
#define dVS10xx_EP_byteRate					0x1E05
#define dVS10xx_EP_endFillByte				0x1E06
#define dVS10xx_EP_jumpPoint				0x1E16
#define dVS10xx_EP_lastestJump				0x1E26
#define dVS10xx_EP_positionMsec				0x1E27
#define dVS10xx_EP_resync					0x1E29
/*	WMA	*/
#define dVS10xx_EP_curPacketSize			0x1E2A
#define dVS10xx_EP_packetSize				0x1E2C
/*	AAC	*/
#define dVS10xx_EP_sceFoundMask				0x1E2A
#define dVS10xx_EP_cpeFOundMask				0x1E2B
#define dVS10xx_EP_1feFoundMask				0x1E2C
#define dVS10xx_EP_playSelect				0x1E2D
#define dVS10xx_EP_dynCompress				0x1E2E
#define dVS10xx_EP_dynBoost					0x1E2F
#define dVS10xx_EP_sbrAndPsStatus			0x1E30
/*	Midi	*/
#define dVS10xx_EP_bytesLeft				0x1E2A
/*	Ogg Vorbis	*/
#define dVS10xx_EP_gain						0x1E2A

#define dVS10xx_SM_TESTS_SINE_PLAY			0
#define dVS10xx_SM_TESTS_SINE_STOP			1
#define dVS10xx_SM_TESTS_PIN				2
#define dVS10xx_SM_TESTS_SCI				3
#define dVS10xx_SM_TESTS_MEMORY				4
#define dVS10xx_SM_TESTS_NEWSINE			5
#define dVS10xx_SM_TESTS_SWEEP				6

#endif
