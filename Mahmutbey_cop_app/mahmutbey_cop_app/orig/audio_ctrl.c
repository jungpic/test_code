#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "audio_multicast.h"
#include "audio_ctrl.h"
#include "lm1971_ctrl.h"
#include "vs1053.h"
#include "gpio_key_dev.h"

#include "cob_process.h"
#include "lcd.h"

#define DEBUG_LINE					1
#if (DEBUG_LINE == 1)
#define DBG_LOG(fmt, args...)	\
	do { 			\
		fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);	\
	} while(0)
#else
#define DBG_LOG(fmt, args...)
#endif

#define VOIP_CALL_DEV_NAME	"/dev/vs1053-0"
#define ENC_MIC_DEV_NAME	"/dev/vs1053-1"
#define ENC_MON_DEV_NAME	"/dev/vs1053-2"
#define DEC_SPK_DEV_NAME	"/dev/vs1053-3"

extern int __silence_mp3_48000_64kbps_sizes[14];
extern char *__silence_mp3_48000_64kbps[];

#define VOLUME_DEV_NAME	"/dev/lm1971"

#define RING_BEEP_FILENAME		"/mnt/data/RingBeep.mp3"
#define DEADMAN_ALARM_FILENAME	"/mnt/data/DeadManAlarm.mp3"
//#define FIRE_ALARM_FILENAME		"/mnt/data/FireAlarm.mp3"

extern int check_avc_cycle_data(void);

static int read_codec_setup(void);

//static void VS1053_Decode_Init(int fd, double fGain);
//static unsigned short VS1053_Read_endFillByte(int fd);
static int vs10xx_is_ready(int fd);

static int read_ring_beep_file(char *fname);
static int read_deadman_alarm_file(char *fname);
//static int read_fire_alarm_file(char *fname);

static unsigned int SSRCold = 0;
static unsigned short Seqold = 0;
static unsigned int SaveStartSSRC = 0;

static int one_flame_play_time;
void set_indicate_codec_timeout_occured(int v);

extern long cal_clock_delay(struct timespec *old_t, struct timespec *cur_t);

static void WELL512Init(void);
static void RTPPacketInit(RTP_Packet_t* pPkt);
static void RTPVoIP_PacketInit(RTP_VoIP_Packet_t* pPkt);

static int vs10xx_set_swreset(int fd);
static void vs10xx_set_reg(int fd, unsigned char reg, unsigned short value);
static unsigned short vs10xx_get_reg(int fd, unsigned char reg, unsigned char bChgEndian);
//static int test_ring_beep(int fd);

/*static int recovery_cop_spk_volume(void);*/

static int __decoding_start(void);

static unsigned int cop_pa_mic_WaitWord;
static unsigned int cop_voip_call_mic_WaitWord;
static unsigned int occ_pa_and_occ_call_mic_WaitWord;
static unsigned int monitor_WaitWord;

#define CODEC_CONFIG_SETUP_FILE	"/mnt/data/codec_config.txt"
static int vs1063_enc_AEC_enable;
static int vs1063_enc_ADC_MODE; /* 0 ~ 4 */
static int vs1063_enc_LINE1_enable;

static int vs1063_cop_pa_enc_freq;
static int vs1063_call_enc_freq;
static int vs1063_mon_enc_freq;

static int vs1063_cop_pa_enc_GAIN_val;
static int vs1063_call_enc_GAIN_val;
static int vs1063_mon_enc_GAIN_val;

static int vs1063_cop_pa_enc_bitrate;
static int vs1063_call_enc_bitrate;
static int vs1063_mon_enc_bitrate;

static int __COP_PA_AND_CALL_ENCODING_LIMIT;
static int __CALL_ENCODING_LIMIT;
static int __MON_ENCODING_LIMIT;

static int fd_volume;
static int fd_ringbeep;
static int fd_deadmanalarm;
//static int fd_firealarm;

static int fd_voip_call;
static int fd_mic_enc;
static int fd_mon_enc;
static int fd_spk_dec;

static int RingBeep_BufLen;
static int RingBeep_idx;;
static char *BufRingBeep;

static int occ_mic_vol_set_from_other_func;
static int pa_mic_vol_set_from_other_func;
static int cop_spk_vol_set_from_other_func;

static int DeadmanAlarm_running;
static int DeadmanAlarm_BufLen;
static int DeadmanAlarm_idx;
static char *BufDeadmanAlarm;

//static int FireAlarm_running;
//static int FireAlarm_BufLen;
//static int FireAlarm_idx;
//static char *BufFireAlarm;

static char dummy[2052];

#define REMAIN_BUF_SIZE	(512*1024)
//static char remain_buf[8192];
static char *remain_buf;
static int dec_remain_len = 0;

static unsigned short cop_pa_enc_tmp_buf[512];
static unsigned short occ_call_enc_tmp_buf[512];
static unsigned short voip_enc_tmp_buf[512];
static unsigned short mon_enc_tmp_buf[512];

static struct vs1053_sciburstreg cop_pa_sciburstreg;
static struct vs1053_sciburstreg cop_voip_call_sciburstreg;
static struct vs1053_sciburstreg occ_pa_and_call_sciburstreg;
static struct vs1053_sciburstreg mon_sciburstreg;

static RTP_Packet_t cop_pa_and_call_enc_tx_packet;
static RTP_Packet_t occ_pa_and_call_enc_tx_packet;
static RTP_Packet_t mon_enc_tx_packet;

static RTP_VoIP_Packet_t pei_voip_call_enc_tx_packet;

static unsigned short enc_cop_pa_and_call_tx_rtpseq;
static unsigned short occ_pa_and_call_tx_rtpseq;
static unsigned short enc_mon_rtpseq;
static unsigned short pei_voip_call_enc_tx_rtpseq;

static unsigned int enc_cop_pa_and_call_tx_rtpTS;
static unsigned int occ_pa_and_call_tx_rtpTS;
static unsigned int enc_mon_rtpTS;
static unsigned int pei_voip_call_enc_tx_rtpTS;

static int cop_pa_and_call_tx_pkt_len;
static int occ_pa_and_call_tx_pkt_len;
static int mon_tx_pkt_len;
static int pei_voip_call_tx_pkt_len;

static int enable_check_different_ssrc;
static int enable_check_same_ssrc;

/* <<<< VOIP */
#define MAX_FRAME_SIZE 2048
//unsigned char G711_buf[MAX_FRAME_SIZE];
unsigned char G711_recv_buf[MAX_FRAME_SIZE];
int recommendDecodeSize;
int maxSamplingSize;
int minSamplingSize;
unsigned int frame_size;
int pei_call_valid=0;
/* VOIP >>>> */

/*
static unsigned short __mpeg_v1_l3_bitrate_table[] = {
    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1
};
*/

static int __mpeg_l3_frame_length[3][16] = {
/* 44100 */ {                0,    32000*72*2/44100,   40000*72*2/44100,  48000*72*2/44100,
              56000*72*2/44100,    64000*72*2/44100,   80000*72*2/44100,  96000*72*2/44100,
             112000*72*2/44100,   128000*72*2/44100,  160000*72*2/44100, 192000*72*2/44100,
             224000*72*2/44100,   256000*72*2/44100,  320000*72*2/44100,              0 },

/* 48000 */ {                0,  32000*72*2/48000,  40000*72*2/48000,  48000*72*2/48000,
              56000*72*2/48000,  64000*72*2/48000,  80000*72*2/48000,  96000*72*2/48000,
             112000*72*2/48000, 128000*72*2/48000, 160000*72*2/48000, 192000*72*2/48000,
             224000*72*2/48000, 256000*72*2/48000, 320000*72*2/48000,              0 },

/* 32000 */ {                0,  32000*72*2/32000,  40000*72*2/32000,  48000*72*2/32000,
              56000*72*2/32000,  64000*72*2/32000,  80000*72*2/32000,  96000*72*2/32000,
             112000*72*2/32000, 128000*72*2/32000, 160000*72*2/32000, 192000*72*2/32000,
             224000*72*2/32000, 256000*72*2/32000, 320000*72*2/32000,              0 },
};

static int __mpeg_l3_frame_play_time_nsec[3] = {
    26122449, /* 44100 */
    24000000, /* 48000 */
    24000000, /* 32000 */
    //36000000, /* 32000 */
};

static int RxDecFrameLen;

/* <<<< VOIP */
static int VS1063_SCI_AEC_ENABLE; 
static int VS1063_SCI_CLOCKF;
static int VS1063_SCI_VOL;
static int VS1063_SCI_AICTRL0;
static int VS1063_SCI_AICTRL1;
static int VS1063_SCI_AICTRL2;
static int VS1063_SCI_AICTRL3;

static int VS1063_SCI_MODE;
static int VS1063_SCI_ADAPT_ADDR;
static int VS1063_SCI_ADAPT_VAL;
static int VS1063_SCI_PATCH;
static int VS1063_SCI_MIC_GAIN_ADDR;
static int VS1063_SCI_MIC_GAIN_VAL;
static int VS1063_SCI_SPK_GAIN_ADDR;
static int VS1063_SCI_SPK_GAIN_VAL;
static int VS1063_SCI_AEC_PASS_ADDR;
static int VS1063_SCI_AEC_PASS_VAL;
static int VS1063_SCI_COEFFICIENTS;
static int VS1063_SCI_AEC_COEFFICIENTS_VAL;

static int VS1063_ENCODER_LIMIT;
static int VS1063_DECODER_LIMIT;
static int VOIP_ENCODING_LIMIT;
/* VOIP >>>>> */

#define MIC_VOL_SETUP_FILE	"/mnt/data/cop_mic_vol.txt"
#define SPK_VOL_SETUP_FILE	"/mnt/data/cop_spk_vol.txt"
#define CUR_PAMP_VOL_LEVEL_FILE	"/mnt/data/cur_pamp_vol_level.txt"

#if SAVE_COP_SPK_MIC_VOLUME
#define CUR_VOL_LEVEL_FILE	"/mnt/data/cur_vol_level.txt"
#endif

#define MIC_VOLUME	1
#define SPK_VOLUME	2

static char mic_setup_vol_data[21] = {
  /* 0   5  10  15  20  25  30  35  40  45  50 55 60 65 70 75 80 85 90 95 100 */
    96, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};
static char spk_setup_vol_data[21] = {
  /* 0   5  10  15  20  25  30  35  40  45  50 55 60 65 70 75 80 85 90 95 100 */
    96, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

static char mic_setup_vol[21];
static char spk_setup_vol[21];
static char inspk_setup_vol[21];
static char outspk_setup_vol[21];

int mic_vol_level;
int spk_vol_level;
int inspk_vol_level;
int outspk_vol_level;

int pa_mic_volume_enable;
int spk_volume_enable;

static unsigned char get_mic_atten_value(unsigned char value)
{
    unsigned char atten, vol;
    int remain;

    if (value > 0) {
        remain = value % 5;
        vol = value - remain;
        if (remain >= 3)
            vol += 5;
        atten = mic_setup_vol[vol/5];
    }
    else {
        vol = 0;
        atten = mic_setup_vol[0];
    }

    DBG_LOG("[ MIC Vol : %d - atten : %d ]\n", vol, atten);

    return atten;
}

static unsigned char get_spk_atten_value(unsigned char value)
{
    unsigned char atten, vol;
    int remain;

    if (value > 0) {
        remain = value % 5;
        vol = value - remain;
        if (remain >= 3)
            vol += 5;
        atten = spk_setup_vol[vol/5];
    }
    else {
        vol = 0;
        atten = spk_setup_vol[0];
    }

    DBG_LOG("[ SPK Vol %d - atten:%d ]\n", vol, atten);

    return atten;
}

#if SAVE_COP_SPK_MIC_VOLUME
static int read_volume_level(char *fname)
{
    int ret = 0;
    int i, fd, rlen;
    char buf[64], ch;
    int val;

    mic_vol_level = -1;
    spk_vol_level = -1;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", fname);
        mic_vol_level = DEFAULT_MIC_VOL_LEVEL;
        spk_vol_level = DEFAULT_SPK_VOL_LEVEL;
        return -1;
    }

    i = 0;
    buf[i] = 0;
    while (1) {
    	rlen = read(fd, &ch, 1);
       	if (rlen <= 0)
    		break;
       	if (ch == 0x0A || ch == 0x0D) {
        	buf[i] = 0;
                i = 0;
                if (strncmp(buf, "MIC_LEVEL:", 10) == 0) {
             	    val = strtol(&buf[10], NULL, 0);
                    if (val >= 0 && val <= 100) {
                        mic_vol_level = val; 
                        printf("< MIC VOL: %d >\n", mic_vol_level);
                    }
                }
                else if (strncmp(buf, "SPK_LEVEL:", 10) == 0) {
             	    val = strtol(&buf[10], NULL, 0);
                    if (val >= 0 && val <= 100) {
                        spk_vol_level = val; 
                        printf("< SPK VOL: %d >\n", spk_vol_level);
                    }
                }
       	}
       	else {
         	buf[i++] = ch;
        }
    }

    if (mic_vol_level < 0)
        mic_vol_level = DEFAULT_MIC_VOL_LEVEL;
    if (spk_vol_level < 0)
        spk_vol_level = DEFAULT_SPK_VOL_LEVEL;

    close(fd);

    return ret;
}
#endif

void update_pamp_volume_level(void)
{
	read_pamp_volume_level(CUR_PAMP_VOL_LEVEL_FILE);
}

int read_pamp_volume_level(char *fname)
{
    int ret = 0;
    int i, fd, rlen;
    char buf[64], ch;
    int val;

    inspk_vol_level = -1;
    outspk_vol_level = -1;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", fname);
        inspk_vol_level = DEFAULT_SPK_VOL_LEVEL;
        outspk_vol_level = DEFAULT_SPK_VOL_LEVEL;
        return -1;
    }

    i = 0;
    buf[i] = 0;
    while (1) {
    	rlen = read(fd, &ch, 1);
       	if (rlen <= 0)
    		break;
       	if (ch == 0x0A || ch == 0x0D) {
        	buf[i] = 0;
                i = 0;
                if (strncmp(buf, "PAMP_INSPK_LEVEL:", 17) == 0) {
             	    val = strtol(&buf[17], NULL, 0);
                    if (val >= 0 && val <= 100) {
                        inspk_vol_level = val; 
                        printf("< PAMP IN SPK VOL: %d >\n", inspk_vol_level);
                    }
                }
                else if (strncmp(buf, "PAMP_OUTSPK_LEVEL:", 18) == 0) {
             	    val = strtol(&buf[18], NULL, 0);
                    if (val >= 0 && val <= 100) {
                        outspk_vol_level = val; 
                        printf("< PAMP OUT SPK VOL: %d >\n", outspk_vol_level);
                    }
                }
       	} else {
         	buf[i++] = ch;
        }
    }

    if (inspk_vol_level < 0)
        inspk_vol_level = DEFAULT_SPK_VOL_LEVEL;
    if (outspk_vol_level < 0)
        outspk_vol_level = DEFAULT_SPK_VOL_LEVEL;

    close(fd);

    return ret;
}

static int write_pamp_volume_level(char *fname, int inspk_vol, int outspk_vol)
{
	int ret = 0;
	int fd, len;
	char buf[64];

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0) {
		printf("Cannot open %s\n", fname);
		return -1;
	}

	sprintf(buf, "PAMP_INSPK_LEVEL:%d\n", inspk_vol);
	len = strlen(buf);
	write(fd, buf, len);

	sprintf(buf, "PAMP_OUTSPK_LEVEL:%d\n", outspk_vol);
	len = strlen(buf);
	write(fd, buf, len);

	close(fd);

	return ret;
}

static int write_volume_level(char *fname, int mic_vol, int spk_vol)
{
    int ret = 0;
    int fd, len;
    char buf[64];

    fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        printf("Cannot open %s\n", fname);
        return -1;
    }

    sprintf(buf, "MIC_LEVEL:%d\n", mic_vol);
    len = strlen(buf);
    write(fd, buf, len);

    sprintf(buf, "SPK_LEVEL:%d\n", spk_vol);
    len = strlen(buf);
    write(fd, buf, len);

    close(fd);

    return ret;
}

static int read_volume_setup(char *fname, char *vol_buf, int check_volume)
{
    int ret = 0;
    int i, fd, rlen;
    char buf[64], ch;
    int val, vol_idx = 0;
    char *s;

    DBG_LOG("\n");
    fd = open(fname, O_RDONLY);
	if (fd < 0) {
		printf("Cannot open %s\n", fname);

		while(vol_idx < 22){
			if(check_volume == MIC_VOLUME)
				vol_buf[vol_idx] = mic_setup_vol_data[vol_idx];
			else if(check_volume == SPK_VOLUME)
				vol_buf[vol_idx] = spk_setup_vol_data[vol_idx];
			else
				printf("%s, Can't check MIC or SPK\n", __func__);

			vol_idx++;
		}
	}
	else{
		i = 0;
		buf[i] = 0;
		while (1) {
			rlen = read(fd, &ch, 1);
			//printf("len: %d\n", rlen);
			if (rlen <= 0)
				break;
			//printf("%02X ", ch);
			if (ch == 0x0A || ch == 0x0D) {
				buf[i] = 0;
				i = 0;
				s = strchr(buf, '-');
				if (s) {
					s++;
					val = strtol(s, NULL, 0);
					//printf("VAL: %d\n", val);
					vol_buf[vol_idx++] = val;
					if (vol_idx > 20)
						break;
				}
			}
			else {
				buf[i++] = ch;
			}

		}

		close(fd);
	}
    //printf("\n");

    printf("VOL: ");
    for (i = 0; i < 21; i++) {
        printf("%02d ", vol_buf[i]);
    }
    printf("\n");

    return ret;
}

int audio_init(void)
{
    int ret = 0;

    enable_check_different_ssrc = 0;
    enable_check_same_ssrc = 1;

    cop_pa_and_call_tx_pkt_len = 0;
    occ_pa_and_call_tx_pkt_len = 0;
    mon_tx_pkt_len = 0;
	pei_voip_call_tx_pkt_len = 0;

	pei_voip_call_enc_tx_rtpseq = (unsigned short)0x0001;
	pei_voip_call_enc_tx_rtpTS = 0;	// 8kbps

    enc_cop_pa_and_call_tx_rtpseq = 1;
    enc_cop_pa_and_call_tx_rtpTS = 2160;

    mic_vol_level = DEFAULT_MIC_VOL_LEVEL;
    spk_vol_level = DEFAULT_SPK_VOL_LEVEL;

    inspk_vol_level = DEFAULT_SPK_VOL_LEVEL;
    outspk_vol_level = DEFAULT_SPK_VOL_LEVEL;

    pa_mic_volume_enable = 0;
    spk_volume_enable = 0;

    read_volume_setup(MIC_VOL_SETUP_FILE, mic_setup_vol, MIC_VOLUME);
    read_volume_setup(SPK_VOL_SETUP_FILE, spk_setup_vol, SPK_VOLUME);
    read_volume_setup(SPK_VOL_SETUP_FILE, inspk_setup_vol, SPK_VOLUME);
    read_volume_setup(SPK_VOL_SETUP_FILE, outspk_setup_vol, SPK_VOLUME);

#if SAVE_COP_SPK_MIC_VOLUME
    read_volume_level(CUR_VOL_LEVEL_FILE);
#else
	mic_vol_level = DEFAULT_MIC_VOL_LEVEL;
	spk_vol_level = DEFAULT_SPK_VOL_LEVEL;
#endif
    read_pamp_volume_level(CUR_PAMP_VOL_LEVEL_FILE);

    read_codec_setup();

    fd_voip_call = -1;
	fd_mic_enc = -1;
	fd_mon_enc = -1;
	fd_spk_dec = -1;

    fd_voip_call = open(VOIP_CALL_DEV_NAME, O_RDWR);
    if (fd_voip_call < 0) {
        printf("Voip Codec DEV OPEN ERROR: %s\n", VOIP_CALL_DEV_NAME);
        return -1;
    }

    fd_mic_enc = open(ENC_MIC_DEV_NAME, O_RDWR);
    if (fd_mic_enc < 0) {
        printf("MIC Encoder DEV OPEN ERROR: %s\n", ENC_MIC_DEV_NAME);
        return -1;
    }

    fd_mon_enc = open(ENC_MON_DEV_NAME, O_RDWR);
    if (fd_mon_enc< 0) {
        printf("MON Encorder DEV OPEN ERROR: %s\n", ENC_MON_DEV_NAME);
        return -1;
    }

    fd_spk_dec = open(DEC_SPK_DEV_NAME, O_WRONLY);
    if (fd_spk_dec < 0) {
        printf("SPK Decoder DEV OPEN ERROR: %s\n", DEC_SPK_DEV_NAME);
        return -1;
    }

    fd_volume = open(VOLUME_DEV_NAME, O_RDWR);
    if (fd_volume < 0) {
        printf("VOL DEV OPEN ERROR: %s\n", VOLUME_DEV_NAME);
        return -1;
    }

    cop_speaker_volume_mute_set();
    cop_pa_mic_volume_mute_set();

    BufRingBeep = NULL;
    RingBeep_BufLen = 0;
    RingBeep_idx = 0;
    fd_ringbeep = read_ring_beep_file(RING_BEEP_FILENAME);

    occ_mic_vol_set_from_other_func = 0;
    pa_mic_vol_set_from_other_func = 0;
    cop_spk_vol_set_from_other_func = 0;

    DeadmanAlarm_running = 0;
    BufDeadmanAlarm = NULL;
    DeadmanAlarm_BufLen = 0;
    DeadmanAlarm_idx = 0;
    fd_deadmanalarm = read_deadman_alarm_file(DEADMAN_ALARM_FILENAME);

    //FireAlarm_running = 0;
    //BufFireAlarm = NULL;
    //FireAlarm_BufLen = 0;
    //FireAlarm_idx = 0;
    //fd_firealarm    = read_fire_alarm_file(FIRE_ALARM_FILENAME);

    cop_pa_mic_codec_init();
	cop_voip_call_codec_init();
    //cop_occ_codec_init();
    cop_monitoring_codec_init();

    WELL512Init();

    cop_pa_sciburstreg.reg = dVS10xx_SCI_HDAT0;
    cop_pa_sciburstreg.size = __COP_PA_AND_CALL_ENCODING_LIMIT;
    cop_pa_sciburstreg.data = cop_pa_enc_tmp_buf;

    cop_voip_call_sciburstreg.reg = dVS10xx_SCI_HDAT0;
    cop_voip_call_sciburstreg.size = VS1063_ENCODER_LIMIT/2;
    cop_voip_call_sciburstreg.data = voip_enc_tmp_buf;

    occ_pa_and_call_sciburstreg.reg = dVS10xx_SCI_HDAT0;
    occ_pa_and_call_sciburstreg.size = __CALL_ENCODING_LIMIT;
    occ_pa_and_call_sciburstreg.data = occ_call_enc_tmp_buf;

    mon_sciburstreg.reg = dVS10xx_SCI_HDAT0;
    mon_sciburstreg.size = __MON_ENCODING_LIMIT;
    mon_sciburstreg.data = mon_enc_tmp_buf;

	pei_voip_call_tx_pkt_len = sizeof(pei_voip_call_enc_tx_packet.rtpHeader) + VS1063_ENCODER_LIMIT;
	RTPVoIP_PacketInit(&pei_voip_call_enc_tx_packet);

    cop_pa_and_call_tx_pkt_len = sizeof(cop_pa_and_call_enc_tx_packet.rtpHeader)
						+ sizeof(cop_pa_and_call_enc_tx_packet.MPAHeader)
                       + __COP_PA_AND_CALL_ENCODING_LIMIT * 2;
    RTPPacketInit(&cop_pa_and_call_enc_tx_packet);

    occ_pa_and_call_tx_pkt_len = sizeof(occ_pa_and_call_enc_tx_packet.rtpHeader)
					+ sizeof(occ_pa_and_call_enc_tx_packet.MPAHeader)
					+ __CALL_ENCODING_LIMIT * 2;
    RTPPacketInit(&occ_pa_and_call_enc_tx_packet);

    mon_tx_pkt_len = sizeof(mon_enc_tx_packet.rtpHeader)
					+ sizeof(mon_enc_tx_packet.MPAHeader)
					+ __MON_ENCODING_LIMIT * 2;
    RTPPacketInit(&mon_enc_tx_packet);

    memset(dummy, 0, 2052);

    remain_buf = malloc(REMAIN_BUF_SIZE);

    __decoding_start();

    return ret;
}

int check_codec_HW_error_1(void)
{
    int ret = 0;

    if (fd_voip_call <= 0)
        ret = 1;

    return ret;
}

int check_codec_HW_error_2(void)
{
    int ret = 0;

    if (fd_mic_enc <= 0)
        ret = 1;

    return ret;
}

int check_codec_HW_error_3(void)
{
    int ret = 0;

    if (fd_mon_enc <= 0)
        ret = 1;

    return ret;
}

int check_codec_HW_error_4(void)
{
    int ret = 0;

    if (fd_spk_dec <= 0)
        ret = 1;

    return ret;
}


#if 1
int decoding_start(void)
{
    dec_remain_len = 0;
    SSRCold = -1;
    Seqold = 0;

    return 0;
}
#else
int decoding_start(void) { }
#endif

static int __decoding_start(void)
{
    int ret;

    ret = vs10xx_set_swreset(fd_spk_dec);
    if (ret != 0)
        printf("<<< DEC SWRESET FAIL >>>\n");
    else
        printf("< DEC SWRESET OK >\n");

    dec_remain_len = 0;
    SSRCold = -1;
    Seqold = -1;

    return 0;
}

int decoding_stop(void)
{
    return 0;
}

void audio_exit(void)
{
    if (fd_voip_call > 0)
        close(fd_voip_call);

    if (fd_mic_enc > 0)
        close(fd_mic_enc);

    if (fd_mon_enc > 0)
        close(fd_mon_enc);

    if (fd_spk_dec > 0)
        close(fd_spk_dec);

    if (fd_volume > 0)
        close(fd_volume);

    if (BufRingBeep)
        free (BufRingBeep);

    if (BufDeadmanAlarm)
        free (BufDeadmanAlarm);
#if 0
    if (BufFireAlarm)
        free (BufFireAlarm);
#endif
}

#ifdef PEI_MONITORING_SAVE
int file_open_mon_save(void)
{
    int fd;
    char buf[128];
    time_t cur;
    struct tm *ptm;

    cur = time(NULL);
    ptm = localtime(&cur);
    strftime(buf, sizeof(buf), "/mnt/pei_monitor/%Y%m%d_%I%M%S.mp3", ptm);
    //strftime(buf, sizeof(buf), "/mnt/data/save_monitoring/%Y%m%d_%I%M%S.mp3", ptm);

    fd = open(buf, O_CREAT | O_WRONLY);
    if (fd > 0) {
        printf("<< MON FILE OPEN : %s >>\n", buf);
    }
    else
        printf("< MON FILE OPEN error : %s >>\n", buf);

    return fd;
}
#endif

int file_close_mon_save(int fd)
{
    if (fd > 0) {
        close (fd);
    }

    return 0;
}

static int read_ring_beep_file(char *fname)
{
    int ret, fd, len;
    struct stat st;
    char *buf;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", fname);
        return -1;
    }

    ret = fstat(fd, &st);
    if (ret != 0) {
        printf("Cannot get stat from %s\n", fname);
        close(fd);
        return -1;
    }

    len = st.st_size;

    buf = malloc(len);
    if (buf == NULL) {
        printf("ERROR, Cannot allocate memory (%s).\n", fname);
        close(fd);
        return -1;
    }

    ret = read(fd, buf, len);
    if (ret != len) {
        printf("READ ERROR(%s), RET: %d, len: %d\n", fname, ret, len);
        close(fd);
        fd = -1;
    }

    BufRingBeep = buf;
    RingBeep_BufLen = len;
    RingBeep_idx = 0;

    printf("< BEEP FILE SIZE: %d >\n", len);

    close(fd);

    return 0;
}

void start_play_beep(void)
{
    RingBeep_idx = 0;
}

void stop_play_beep(void)
{
    int i, ret, n, m, framelen, wlen = 0;
    unsigned short rWord, rWord2;
    int mp3_header, pad;

#if 0
    if (DeadmanAlarm_running || FireAlarm_running)
#else
    if (DeadmanAlarm_running)
#endif
        return;

    printf("< BEEP STOP");

for (i = 0; i < 64; i++) {
    mp3_header  = BufRingBeep[RingBeep_idx+0] << 24;
    mp3_header |= BufRingBeep[RingBeep_idx+1] << 16;
    mp3_header |= BufRingBeep[RingBeep_idx+2] << 8;
    mp3_header |= BufRingBeep[RingBeep_idx+3];

    n = (mp3_header >> 10) & 0x3;
    m = (mp3_header >> 12) & 0xF;
    framelen = __mpeg_l3_frame_length[n][m];
    pad = mp3_header >> 9;
    pad &= 1;
    framelen += pad;

    rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
    rWord |= dVS10xx_SM_CANCEL;
    vs10xx_set_reg(fd_spk_dec, dVS10xx_SCI_MODE, rWord);

    while (framelen > 0) {
        if (framelen  > VS10XX_WRITE_SIZE)
            wlen = VS10XX_WRITE_SIZE;
        else
            wlen = framelen;

        /* 2013//11/13 */
        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

        ret = write(fd_spk_dec, &BufRingBeep[RingBeep_idx], wlen);
        if (ret == wlen) {
            rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
            if ((rWord & dVS10xx_SM_CANCEL) == 0)
                break;

            RingBeep_idx += wlen;
            framelen -= wlen;
            if (RingBeep_BufLen == RingBeep_idx) {
                RingBeep_idx = 0;
            }
        }
    }
}

    rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT0, 0);
    rWord2 = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT1, 0);

    if (!rWord && !rWord2)
        printf("-OK >\n");
    else
        printf("- FAIL >\n");

    RingBeep_idx = 0;
}

int running_play_beep(void)
{
    int ret = 0, n, m, framelen;
    int wlen = 0;
    int mp3_header, pad;
    static int internel_ctr = 0;

#if 0
    if (DeadmanAlarm_running || FireAlarm_running)
#else
    if (DeadmanAlarm_running)
#endif
        return 0;

    if (fd_spk_dec < 0)
        return -1;

    //if (!vs10xx_is_ready(fd_spk_dec)) return 0;

    mp3_header  = BufRingBeep[RingBeep_idx+0] << 24;
    mp3_header |= BufRingBeep[RingBeep_idx+1] << 16;
    mp3_header |= BufRingBeep[RingBeep_idx+2] << 8;
    mp3_header |= BufRingBeep[RingBeep_idx+3];

    n = (mp3_header >> 10) & 0x3;
    m = (mp3_header >> 12) & 0xF;
    framelen = __mpeg_l3_frame_length[n][m];
    pad = mp3_header >> 9;
    pad &= 1;
    framelen += pad;

//printf("\n2, %s, n:%d, m:%d, framelen:%d, pad:%d\n", __func__, n, m, framelen, pad);
    while (framelen > 0) {
        if (framelen  > VS10XX_WRITE_SIZE)
            wlen = VS10XX_WRITE_SIZE;
        else
            wlen = framelen;

//printf("%s, mp3_header:%X, framelen:%d, idx:%d, wlen:%d\n", __func__, mp3_header, framelen, RingBeep_idx, wlen);
        ret = write(fd_spk_dec, &BufRingBeep[RingBeep_idx], wlen);
        if (ret == wlen) {
//printf("==%s, idx:%d, wlen:%d\n", __func__, RingBeep_idx, wlen);
            RingBeep_idx += wlen;
            framelen -= wlen;
            if (RingBeep_BufLen == RingBeep_idx) {
//printf("==%s, idx = 0\n", __func__);
                RingBeep_idx = 0;
            }
        }

        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

        internel_ctr++;
        if (internel_ctr >= 2) {
            internel_ctr = 0;
            check_avc_cycle_data(); // 2014/11/27
        }
    }

    return ret;
}

#if 0
static int read_fire_alarm_file(char *fname)
{
	int ret, fd, len;
	struct stat st;
	char *buf;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		printf("Cannot open %s\n", fname);
		return -1;
	}

	ret = fstat(fd, &st);
	if (ret != 0) {
		printf("Cannot get stat from %s\n", fname);
		close(fd);
		return -1;
	}

	len = st.st_size;

	buf = malloc(len);
	if (buf == NULL) {
		printf("ERROR, Cannot allocate memory (%s).\n", fname);
		close(fd);
		return -1;
	}

	ret = read(fd, buf, len);
	if (ret != len) {
		printf("READ ERROR(%s), RET: %d, len: %d\n", fname, ret, len);
		close(fd);
		fd = -1;
	}

	BufFireAlarm = buf;
	FireAlarm_BufLen = len;
	FireAlarm_idx = 0;

	printf("< FIRE ALARM FILE SIZE: %d >\n", len);

	close(fd);

	return 0;
}
#endif

static int read_deadman_alarm_file(char *fname)
{
    int ret, fd, len;
    struct stat st;
    char *buf;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", fname);
        return -1;
    }

    ret = fstat(fd, &st);
    if (ret != 0) {
        printf("Cannot get stat from %s\n", fname);
        close(fd);
        return -1;
    }

    len = st.st_size;

    buf = malloc(len);
    if (buf == NULL) {
        printf("ERROR, Cannot allocate memory (%s).\n", fname);
        close(fd);
        return -1;
    }

    ret = read(fd, buf, len);
    if (ret != len) {
        printf("READ ERROR(%s), RET: %d, len: %d\n", fname, ret, len);
        close(fd);
        fd = -1;
    }

    BufDeadmanAlarm = buf;
    DeadmanAlarm_BufLen = len;
    DeadmanAlarm_idx = 0;

    printf("< DEADMAN ALARM FILE SIZE: %d >\n", len);

    close(fd);

    return 0;
}

#if 0
void start_play_fire_alarm(void)
{
    FireAlarm_idx = 0;
    FireAlarm_running = 1;
}
#endif

void start_play_deadman_alarm(void)
{
    DeadmanAlarm_idx = 0;
    DeadmanAlarm_running = 1;
}

#if 0
void stop_play_fire_alarm(void)
{
	int i, ret, n, m, framelen, wlen = 0;
	unsigned short rWord, rWord2;
	int mp3_header, pad;

	FireAlarm_idx = 0;
	FireAlarm_running = 0;

	printf("< FIRE ALARM STOP");

	for (i = 0; i < 64; i++) {
		mp3_header  = BufFireAlarm[FireAlarm_idx+0] << 24;
		mp3_header |= BufFireAlarm[FireAlarm_idx+1] << 16;
		mp3_header |= BufFireAlarm[FireAlarm_idx+2] << 8;
		mp3_header |= BufFireAlarm[FireAlarm_idx+3];

		n = (mp3_header >> 10) & 0x3;
		m = (mp3_header >> 12) & 0xF;
		framelen = __mpeg_l3_frame_length[n][m];
		pad = mp3_header >> 9;
		pad &= 1;
		framelen += pad;

		rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
		rWord |= dVS10xx_SM_CANCEL;
		vs10xx_set_reg(fd_spk_dec, dVS10xx_SCI_MODE, rWord);

		while (framelen > 0) {
			if (framelen  > VS10XX_WRITE_SIZE)
				wlen = VS10XX_WRITE_SIZE;
			else
				wlen = framelen;

			__discard_cop_pa_and_call_mic_encoding_data();
			__discard_occ_pa_and_call_mic_encoding_data();

			ret = write(fd_spk_dec, &BufFireAlarm[FireAlarm_idx], wlen);
			if (ret == wlen) {
				rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
				if ((rWord & dVS10xx_SM_CANCEL) == 0)
					break;
				FireAlarm_idx += wlen;
				framelen -= wlen;
				if (FireAlarm_BufLen == FireAlarm_idx) {
					FireAlarm_idx = 0;
				}
			}
		}
	}
	rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT0, 0);
	rWord2 = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT1, 0);

	if (!rWord && !rWord2)
		printf("-OK >\n");
	else
		printf("- FAIL >\n");
}
#endif

void stop_play_deadman_alarm(void)
{
    int i, ret, n, m, framelen, wlen = 0;
    unsigned short rWord, rWord2;
    int mp3_header, pad;

    DeadmanAlarm_idx = 0;
    DeadmanAlarm_running = 0;

    printf("< DEADMAN STOP");

for (i = 0; i < 64; i++) {
    mp3_header  = BufDeadmanAlarm[DeadmanAlarm_idx+0] << 24;
    mp3_header |= BufDeadmanAlarm[DeadmanAlarm_idx+1] << 16;
    mp3_header |= BufDeadmanAlarm[DeadmanAlarm_idx+2] << 8;
    mp3_header |= BufDeadmanAlarm[DeadmanAlarm_idx+3];

    n = (mp3_header >> 10) & 0x3;
    m = (mp3_header >> 12) & 0xF;
    framelen = __mpeg_l3_frame_length[n][m];
    pad = mp3_header >> 9;
    pad &= 1;
    framelen += pad;

    rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
    rWord |= dVS10xx_SM_CANCEL;
    vs10xx_set_reg(fd_spk_dec, dVS10xx_SCI_MODE, rWord);

    while (framelen > 0) {
        if (framelen  > VS10XX_WRITE_SIZE)
            wlen = VS10XX_WRITE_SIZE;
        else
            wlen = framelen;

        /* 2013//11/13 */
        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

        ret = write(fd_spk_dec, &BufDeadmanAlarm[DeadmanAlarm_idx], wlen);
        if (ret == wlen) {
            rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
            if ((rWord & dVS10xx_SM_CANCEL) == 0)
                break;

            DeadmanAlarm_idx += wlen;
            framelen -= wlen;
            if (DeadmanAlarm_BufLen == DeadmanAlarm_idx) {
                DeadmanAlarm_idx = 0;
            }
        }
    }
}

    rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT0, 0);
    rWord2 = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT1, 0);

    if (!rWord && !rWord2)
        printf("-OK >\n");
    else
        printf("- FAIL >\n");
}

#if 0
int running_play_fire_alarm(void)
{
	int ret = 0, n, m, framelen;
	int wlen = 0;
	int mp3_header, pad;
	static int internel_ctr = 0;

	if (fd_spk_dec < 0)
		 return -1;

	mp3_header  = BufFireAlarm[FireAlarm_idx+0] << 24;
	mp3_header |= BufFireAlarm[FireAlarm_idx+1] << 16;
	mp3_header |= BufFireAlarm[FireAlarm_idx+2] << 8;
	mp3_header |= BufFireAlarm[FireAlarm_idx+3];

	n = (mp3_header >> 10) & 0x3;
	m = (mp3_header >> 12) & 0xF;
	framelen = __mpeg_l3_frame_length[n][m];
	pad = mp3_header >> 9;
	pad &= 1;
	framelen += pad;

    while (framelen > 0) {
        if (framelen  > VS10XX_WRITE_SIZE)
            wlen = VS10XX_WRITE_SIZE;
        else
            wlen = framelen;

        ret = write(fd_spk_dec, &BufFireAlarm[FireAlarm_idx], wlen);
        if (ret == wlen) {
            FireAlarm_idx += wlen;
            framelen -= wlen;
            if (FireAlarm_BufLen == FireAlarm_idx) {
                FireAlarm_idx = 0;
            }
        }

        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

        internel_ctr++;
        if (internel_ctr >= 2) {
            internel_ctr = 0;
            check_avc_cycle_data(); // 2014/11/27
        }
    }

    return ret;
}
#endif

int running_play_deadman_alarm(void)
{
    int ret = 0, n, m, framelen;
    int wlen = 0;
    int mp3_header, pad;
    static int internel_ctr = 0;

    if (fd_spk_dec < 0)
        return -1;

    //if (!vs10xx_is_ready(fd_spk_dec)) return 0;

    mp3_header  = BufDeadmanAlarm[DeadmanAlarm_idx+0] << 24;
    mp3_header |= BufDeadmanAlarm[DeadmanAlarm_idx+1] << 16;
    mp3_header |= BufDeadmanAlarm[DeadmanAlarm_idx+2] << 8;
    mp3_header |= BufDeadmanAlarm[DeadmanAlarm_idx+3];

    n = (mp3_header >> 10) & 0x3;
    m = (mp3_header >> 12) & 0xF;
    framelen = __mpeg_l3_frame_length[n][m];
    pad = mp3_header >> 9;
    pad &= 1;
    framelen += pad;

    while (framelen > 0) {
        if (framelen  > VS10XX_WRITE_SIZE)
            wlen = VS10XX_WRITE_SIZE;
        else
            wlen = framelen;

        ret = write(fd_spk_dec, &BufDeadmanAlarm[DeadmanAlarm_idx], wlen);
        if (ret == wlen) {
            DeadmanAlarm_idx += wlen;
            framelen -= wlen;
            if (DeadmanAlarm_BufLen == DeadmanAlarm_idx) {
                DeadmanAlarm_idx = 0;
            }
        }

        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

        internel_ctr++;
        if (internel_ctr >= 2) {
            internel_ctr = 0;
            check_avc_cycle_data(); // 2014/11/27
        }
    }

    return ret;
}

static unsigned long state[16];
static unsigned int WIdx;
static void WELL512Init(void)
{
    int i = 0;
    state[i] = 0;

    for(i = 0; i < 16; i++)
        state[i] += 73;

    WIdx = 0;
}

static unsigned long WELL512(void)
{
    unsigned long a, b, c, d;

    a = state[WIdx];
    c = state[(WIdx+13)&15];
    b = a^c^(a<<16)^(c<<15);
    c = state[(WIdx+9)&15];
    c ^= (c>>11);
    a = state[WIdx] = b^c;
    d = a^((a<<5)&0xda442d20UL);
    WIdx = (WIdx+15)&15;
    a = state[WIdx];
    state[WIdx] = a^b^d^(a<<2)^(b<<18)^(c<<28);
    return state[WIdx];
}

static void RTPVoIP_PacketInit(RTP_VoIP_Packet_t* pPkt)
{
    RTP_Header_t* pHeader = &pPkt->rtpHeader;

    memset(pPkt, 0, sizeof(RTP_VoIP_Packet_t));

    pHeader->h_info1.V = 2;
    pHeader->h_info1.P = 0;
    pHeader->h_info1.X = 0;
    pHeader->h_info1.CC = 0;

    pHeader->h_info2.M = 0;
    pHeader->h_info2.PT = RTP_PT_PCMA;

    pHeader->seq = 0x0001;
    //pHeader->ts = VS1063_ENCODER_LIMIT;
    pHeader->ts = 0;
    pHeader->SSRC = (unsigned int)WELL512();

    //printf("RTPPacketHeaderInit - Seq : %d:%d\n", pHeader->seq, pPkt->rtpHeader.seq);
}

static void RTPPacketInit(RTP_Packet_t* pPkt)
{
    RTP_Header_t* pHeader = &pPkt->rtpHeader;

    memset(pPkt, 0, sizeof(RTP_Packet_t));

    pHeader->h_info1.V = 2;
    pHeader->h_info1.P = 0;
    pHeader->h_info1.X = 0;
    pHeader->h_info1.CC = 0;

    pHeader->h_info2.M = 1;
    pHeader->h_info2.PT = RTP_PT_MPA;

    pHeader->seq = 1;
    pHeader->ts = 2160;
    pHeader->SSRC = (unsigned int)WELL512();

    //printf("RTPPacketHeaderInit - SSRC : 0x%x\n", pHeader->SSRC);
}

/***** Passive PA & OCC PA - MIC VS1063 *******************************************/
void cop_pa_mic_codec_init(void)
{
    unsigned short setup_val;

    RTPPacketInit(&cop_pa_and_call_enc_tx_packet);

    if (fd_mic_enc < 0) return;

    setup_val = 0;
    if (vs1063_enc_LINE1_enable)
        setup_val |= 1 << 14;
    //setup_val |= (1 << 11) | (1 << 2); // SM_SDINEW | SM_RESET
    setup_val |= dVS10xx_SM_SDINEW;
    printf("<< PA MIC, SCI_MODE:0x%04X, ", setup_val);

    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, setup_val);

    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_BASS, 0);


    printf("%dHz-", vs1063_cop_pa_enc_freq);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL0, vs1063_cop_pa_enc_freq);

    //printf("GAIN:%d, ", vs1063_cop_pa_enc_GAIN_val);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL1, vs1063_cop_pa_enc_GAIN_val);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL2, vs1063_cop_pa_enc_GAIN_val);

    setup_val = vs1063_enc_ADC_MODE + 0x60;
    if (vs1063_enc_AEC_enable)
        setup_val |= 1 << 14;

    //printf("AICTRL3:0x%04X, ", setup_val);
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL3, 0x4062); // AEC enable, LEFT
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL3, setup_val);

    setup_val = 0xE000;
    setup_val += vs1063_cop_pa_enc_bitrate;
    printf("%dkpbs >>\n", vs1063_cop_pa_enc_bitrate);

    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, 0xe020); // CBR 32kbps
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, 0xe040); // CBR 64kbps
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, 0xe080); // CBR 128kbps
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, 0xe0C0); // CBR 192kbps
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, setup_val);

    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<14) |(1<<12) | (1<<11)); // SM_SDINEW | SM_RESET
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AIADDR, 0x50); // Active encoding
}

void __cop_pa_mic_codec_swreset(void)
{
    int ret;

    ret = vs10xx_set_swreset(fd_mic_enc);
    if (ret != 0)
        printf("<<< PA MIC ENC SWRESET FAIL >>>\n");
    else
        printf("<<< PA MIC ENC SWRESET OK >>>\n");
}

#ifdef USE_CONTINUE_COP_PA_ENCORDING
void __cop_pa_mic_codec_start(void)
{
    //unsigned short rWord;
    //int i;
    unsigned short setup_val;

    if (fd_mic_enc < 0) return;

    setup_val = 0;
    if (vs1063_enc_LINE1_enable)
        setup_val |= 1 << 14;
    setup_val |= (1 << 12) | (1 << 11); // SM_SDINEW | SM_RESET

    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, setup_val);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AIADDR, 0x50); // Active encoding

    WIdx++; if (WIdx >= 16) WIdx = 0;
    RTPPacketInit(&cop_pa_and_call_enc_tx_packet);
    enc_cop_pa_and_call_tx_rtpseq = 1;
    enc_cop_pa_and_call_tx_rtpTS = 2160;

    cop_pa_mic_WaitWord = 0;
}

void cop_pa_mic_codec_start(void)
{
	DBG_LOG("\n");
    WIdx++; if (WIdx >= 16) WIdx = 0;
    RTPPacketInit(&cop_pa_and_call_enc_tx_packet);
    enc_cop_pa_and_call_tx_rtpseq = 1;
    enc_cop_pa_and_call_tx_rtpTS = 2160;
}

#else
void cop_pa_mic_codec_start(void)
{
    //unsigned short rWord;
    //int i;
    unsigned short setup_val;

    if (fd_mic_enc < 0) return;

    setup_val = 0;
    if (vs1063_enc_LINE1_enable)
        setup_val |= 1 << 14;
    setup_val |= (1 << 12) | (1 << 11); // SM_SDINEW | SM_RESET

	
	DBG_LOG("\n");
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, setup_val);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AIADDR, 0x50); // Active encoding

    WIdx++; if (WIdx >= 16) WIdx = 0;
    RTPPacketInit(&cop_pa_and_call_enc_tx_packet);
    enc_cop_pa_and_call_tx_rtpseq = 1;
    enc_cop_pa_and_call_tx_rtpTS = 2160;

    cop_pa_mic_WaitWord = 0;
}
#endif

#if 0
void cop_occ_codec_init(void)
{
    unsigned short setup_val;

    RTPPacketInit(&occ_pa_and_call_enc_tx_packet);

    if (fd_mic_enc < 0) return;

    setup_val = 0;
    if (vs1063_enc_LINE1_enable)
        setup_val |= 1 << 14;
    //setup_val |= (1 << 11) | (1 << 2); // SM_SDINEW | SM_RESET
    setup_val |= dVS10xx_SM_SDINEW;
    printf("<< OCC MIC, SCI_MODE:0x%04X, ", setup_val);

    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, 0x5800 | (1<<2)); // SM_ENCODE | SM_LINE1 | SM_SDINEW
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<14) | (1<<11) | (1<<2)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, setup_val);

    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_BASS, 0);

    printf("%dHz- ", vs1063_call_enc_freq);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL0, vs1063_call_enc_freq); // 48Khz

    //printf("GAIN:%d, ", vs1063_call_enc_GAIN_val);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL1, vs1063_call_enc_GAIN_val);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL2, vs1063_call_enc_GAIN_val);

    setup_val = vs1063_enc_ADC_MODE + 0x60;
    if (vs1063_enc_AEC_enable)
        setup_val |= 1 << 14;

    //printf("AICTRL3:0x%04X, ", setup_val);
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL3, 0x4062); // AEC enable, LEFT
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AICTRL3, setup_val);

    setup_val = 0xE000;
    setup_val += vs1063_call_enc_bitrate;
    printf("%dkbps >>\n", vs1063_call_enc_bitrate);

    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, 0xe020); // CBR 32kbps
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, 0xe040); // CBR 64kbps
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, 0xe080); // CBR 128kbps
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_WRAMADDR, setup_val);

#if 0
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<14)|(1<<12)|(1<<11)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AIADDR, 0x50); // Active encoding
#endif
}
#endif

/* <<<< VOIP */
void cop_voip_call_codec_init(void)
{
	int val = 0;

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_CLOCKF, VS1063_SCI_CLOCKF);

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_VOL, VS1063_SCI_VOL);

    printf("%dHz-", VS1063_SCI_AICTRL0);
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_AICTRL0, VS1063_SCI_AICTRL0);
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_AICTRL1, VS1063_SCI_AICTRL1); 
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_AICTRL2, VS1063_SCI_AICTRL2);

	val = VS1063_SCI_AICTRL3;
	if(VS1063_SCI_AEC_ENABLE == 1)
	{
		val |= (1 << 14);
		VS1063_SCI_AICTRL3 = val;
	}
	else
	{
		val = ~val;
		val |= (1 << 14);
		val = ~val;
		//printf("=====yb===== : VS1063_SCI_AICTRL3[%#x]-val[%#x]\n",VS1063_SCI_AICTRL3,val);
		VS1063_SCI_AICTRL3 = val;
	}
	vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_AICTRL3, VS1063_SCI_AICTRL3);

	WIdx++; if (WIdx >= 16) WIdx = 0;
	RTPVoIP_PacketInit(&pei_voip_call_enc_tx_packet);
}

void cop_voip_call_codec_swreset(void)
{
    int ret; 

    ret = vs10xx_set_swreset(fd_voip_call);
    if (ret != 0)
    printf("<<< VOIP-CALL CODEC SWRESET FAIL >>>\n");
    else 
    printf("<<< VOIP-CALL CODEC SWRESET OK >>>\n");
}

void cop_voip_call_codec_start(void)
{
    //unsigned short setup_val;

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_MODE, VS1063_SCI_MODE); /* MODE */

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, VS1063_SCI_ADAPT_ADDR);
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAM, VS1063_SCI_ADAPT_VAL);

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_AIADDR, VS1063_SCI_PATCH); 

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, VS1063_SCI_MIC_GAIN_ADDR);
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAM, VS1063_SCI_MIC_GAIN_VAL);

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, VS1063_SCI_SPK_GAIN_ADDR);
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAM, VS1063_SCI_SPK_GAIN_VAL); 

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, VS1063_SCI_AEC_PASS_ADDR); 
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAM, VS1063_SCI_AEC_PASS_VAL);  /* AEC pass use:0 */

	cop_voip_call_mic_WaitWord = 0;

	WIdx++;
	if (WIdx >= 16)
		WIdx = 0;
	RTPVoIP_PacketInit(&pei_voip_call_enc_tx_packet);

	pei_voip_call_enc_tx_rtpseq = (unsigned short)0x0001;
	pei_voip_call_enc_tx_rtpTS = 0;	// 8kbps
}

#ifdef USE_CONTINUE_CALL_ENCORDING
void cop_voip_call_codec_stop(void)	{}
#else
void cop_voip_call_codec_stop(void)
{
    unsigned short val = 0; 

    val = vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_MODE, 0);
    val |= dVS10xx_SM_CANCEL;
    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_MODE, val);
    printf("[ Cancel Decoding 0x%x]\n", val);
}
#endif
int discard_voip_call_mic_encoding_data(void)
{
    int rctr = 1; 

    if (cop_voip_call_mic_WaitWord < VOIP_ENCODING_LIMIT) {
        cop_voip_call_mic_WaitWord = vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_HDAT1, 0);
		//printf("%s, waitWord: %d.\n", __func__, cop_voip_call_mic_WaitWord);
        return 1;
    }    

    if (cop_voip_call_mic_WaitWord >= (VOIP_ENCODING_LIMIT << 1)) {
		//printf("%s, waitWord: %d.\n", __func__, cop_voip_call_mic_WaitWord);
        rctr = 2; 
    }    

    cop_voip_call_sciburstreg.size = VOIP_ENCODING_LIMIT;
    cop_voip_call_sciburstreg.data = &voip_enc_tmp_buf[0];

    while (rctr > 0) { 
        cop_voip_call_mic_WaitWord -= VOIP_ENCODING_LIMIT;
        ioctl(fd_voip_call, VS1053_CTL_GETSCIBURSTREG, &cop_voip_call_sciburstreg);
        rctr--;
    }    
    return VOIP_ENCODING_LIMIT;
}

int get_mic_enc_wav_frame(void)//unsigned char *wav_buf)
{
    int len=0;

    if (cop_voip_call_mic_WaitWord < VOIP_ENCODING_LIMIT) {
        cop_voip_call_mic_WaitWord = vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_HDAT1, 0);  
        //if(cop_voip_call_mic_WaitWord > 0){
        //  printf("%s, 1.waitWord: %x.\n", __func__, cop_voip_call_mic_WaitWord); 
        return 0;
    }    

    cop_voip_call_mic_WaitWord -= VOIP_ENCODING_LIMIT;

    cop_voip_call_sciburstreg.reg = dVS10xx_SCI_HDAT0;
    cop_voip_call_sciburstreg.size = VOIP_ENCODING_LIMIT;
    cop_voip_call_sciburstreg.data = pei_voip_call_enc_tx_packet.Palyload;

    ioctl(fd_voip_call, VS1053_CTL_GETSCIBURSTREG, &cop_voip_call_sciburstreg);

    len = VOIP_ENCODING_LIMIT*2;  

    //memcpy(wav_buf, (char *)voip_enc_tmp_buf, len);

    return len; 
}

void reset_voip_frame_info(void)
{
	frame_size=0;
	recommendDecodeSize=0;
	maxSamplingSize=0;
	minSamplingSize=1024;
}

void print_voip_call_codec_setting(void)
{
    printf("\n=== VOL set =======================================\n");
    printf(" fd_voip_call[%d]\n",fd_voip_call);
    printf("fd_volume[%d]\n",fd_volume);
    printf("enc_Limit[%d]\n",VS1063_ENCODER_LIMIT);
    printf("  mic_vol[%d]_spk_vol[%d]\n",mic_vol_level,spk_vol_level);
    printf("=== CODEC set =====================================\n");
    printf("1. SCI_CLOCK[0xc800]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_CLOCKF, 0));
    printf("2.    SCI_VOL[0x000]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_VOL,0));
    printf("3.SCI_AICTRL0[8000] -[%d]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_AICTRL0,0));
    printf("4.SCI_AICTRL1[0x400]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_AICTRL1,0));
    printf("5.SCI_AICTRL2[0x400]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_AICTRL2,0));
    printf("6.SCI_AICTRL3[0xc022/c032]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_AICTRL3,0));
    printf("7.  SCI_MODE[0x1c00]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_MODE,0));

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, 0x1e2d);
    printf("8. ADDR[0x1e2d]>>val[0x2]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_WRAM,0));
    printf("9.  SCI_AIADDR[0x50]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_AIADDR,0));

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, 0x2022);
    printf("10.ADDR[0x2022]>>val[0x400]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_WRAM,0));

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, 0x2023);
    printf("11.ADDR[0x2023]>>val[0x400]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_WRAM,0));

    vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_WRAMADDR, 0x2024);
    printf("12.ADDR[0x2024]>>val[0x000]-[0x%x]\n",vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_WRAM,0));
    printf("==================================================\n");
}
/* VOIP >>>> */

void __cop_occ_pa_and_occ_call_codec_swreset(void)
{
    int ret;

    if (fd_mic_enc < 0) return;

    ret = vs10xx_set_swreset(fd_mic_enc);
    if (ret != 0)
        printf("<<< ENC1(OCC-MIC) SWRESET FAIL >>>\n");
    else
        printf("<<< ENC1(OCC-MIC) SWRESET OK >>>\n");
}

void __cop_occ_pa_and_occ_call_codec_start(void)
{
    //unsigned short rWord;
    //int i;
    unsigned short setup_val;

    if (fd_mic_enc < 0) return;

    setup_val = 0;
    if (vs1063_enc_LINE1_enable)
        setup_val |= 1 << 14;
    setup_val |= (1 << 12) | (1 << 11); // SM_SDINEW | SM_RESET

    WIdx++; if (WIdx >= 16) WIdx = 0;
    RTPPacketInit(&occ_pa_and_call_enc_tx_packet);
    occ_pa_and_call_tx_rtpseq = 1;
    occ_pa_and_call_tx_rtpTS  = 2160;

    occ_pa_and_occ_call_mic_WaitWord = 0;

    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<14)|(1<<12)|(1<<11));
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, setup_val);
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_AIADDR, 0x50); // Active encoding
}

void cop_occ_pa_and_occ_call_codec_start(void)
{
    WIdx++; if (WIdx >= 16) WIdx = 0;
    RTPPacketInit(&occ_pa_and_call_enc_tx_packet);
    occ_pa_and_call_tx_rtpseq = 1;
    occ_pa_and_call_tx_rtpTS  = 2160;
}

void cop_monitoring_codec_init(void)
{
    unsigned short setup_val;

    RTPPacketInit(&mon_enc_tx_packet);

    if (fd_mon_enc < 0) return;

    setup_val = 0;

    //vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_MODE, 0x5800 | (1<<2)); // SM_ENCODE | SM_LINE1 | SM_SDINEW
    //vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_MODE, (1<<14) | (1<<11) | (1<<2)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_MODE, dVS10xx_SM_LINE1 | dVS10xx_SM_SDINEW);
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_BASS, 0);

    printf("<< MON ENC FREQ:%d, ", vs1063_mon_enc_freq);
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_AICTRL0, vs1063_mon_enc_freq); // 48Khz

    printf("GAIN:%d, ", vs1063_mon_enc_GAIN_val);
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_AICTRL1, vs1063_mon_enc_GAIN_val);
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_AICTRL2, vs1063_mon_enc_GAIN_val);

    setup_val = 0x60; // 2013/11/12, 0x60 -> 0x61
    if (vs1063_enc_AEC_enable)
        setup_val |= 1 << 14;
    printf("AICTRL3:0x%04X, ", setup_val);
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_AICTRL3, setup_val); // 0x4060, AEC enable, LEFT & RIGHT

    setup_val = 0xE000;
    setup_val += vs1063_mon_enc_bitrate;
    printf("BITRATE:%d(%04X) >>\n", vs1063_mon_enc_bitrate, setup_val);

    //vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_WRAMADDR, 0xe020); // CBR 32kbps
    //vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_WRAMADDR, 0xe040); // CBR 64kbps
    //vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_WRAMADDR, 0xe080); // CBR 128kbps
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_WRAMADDR, setup_val);

#if 0
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_MODE, (1<<14)|(1<<12)|(1<<11)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_AIADDR, 0x50); // Active encoding
#endif
}

void cop_monitoring_encoding_start(void)
{
    //unsigned short rWord;
    //int i;

    if (fd_mon_enc < 0) return;

    WIdx++; if (WIdx >= 16) WIdx = 0;
    RTPPacketInit(&mon_enc_tx_packet);
    enc_mon_rtpseq = 1;
    enc_mon_rtpTS = 2160;

    monitor_WaitWord = 0;

    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_MODE, (1<<14)|(1<<12)|(1<<11)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_AIADDR, 0x50); // Active encoding
}

#ifdef USE_CONTINUE_COP_PA_ENCORDING
void cop_pa_mic_codec_stop(void) {}
#else
void cop_pa_mic_codec_stop(void)
{
    if (fd_mic_enc < 0) return;
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<11) | (1<<2)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<11)); // SM_SDINEW
}
#endif

#ifdef USE_CONTINUE_CALL_ENCORDING
void cop_occ_pa_and_occ_call_codec_stop(void) {}
#else
void cop_occ_pa_and_occ_call_codec_stop(void)
{
    if (fd_mic_enc < 0) return

    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<11) | (1<<2)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, (1<<11)); // SM_SDINEW
}
#endif

void cop_monitoring_encoding_stop(void)
{
    if (fd_mon_enc < 0) return;
    //vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_MODE, (1<<11) | (1<<2)); // SM_SDINEW | SM_RESET
    vs10xx_set_reg(fd_mon_enc, dVS10xx_SCI_MODE, (1<<11)); // SM_SDINEW
}

void mic_volume_step_up(int add_val, int active)
{
    if (mic_vol_level < 100)
        mic_vol_level += add_val;

    if (mic_vol_level > 100)
        mic_vol_level = 100;

    if (pa_mic_volume_enable)
        cop_pa_and_call_mic_volume_set();

    if (add_val > 0)
        vol_ctrl_draw_menu(mic_vol_level, 1, -1, 0, active);
    else
        vol_ctrl_draw_menu(mic_vol_level, 1, spk_vol_level, 0, active);

#if SAVE_COP_SPK_MIC_VOLUME
    if (add_val > 0)
        write_volume_level(CUR_VOL_LEVEL_FILE, mic_vol_level, spk_vol_level);
#endif
}

void inspk_volume_step_up(int add_val, int active)
{
	if(inspk_vol_level == 100)
		return;

    if (inspk_vol_level < 100)
        inspk_vol_level += add_val;


    if (inspk_vol_level > 100)
        inspk_vol_level = 100;

	if (add_val <= 0)
		iovol_ctrl_draw_menu(inspk_vol_level, 1, outspk_vol_level, 0, active);

	if (add_val > 0)
		send_cop2avc_vol_info(0, inspk_vol_level/10, outspk_vol_level/10);
}

void spk_volume_step_up(int add_val, int active)
{
    if (spk_vol_level < 100)
        spk_vol_level += add_val;

    if (spk_vol_level > 100)
        spk_vol_level = 100;

    if (spk_volume_enable)
        cop_speaker_volume_set();

    if (add_val > 0)
        vol_ctrl_draw_menu(-1, 0, spk_vol_level, 1, active);
    else
        vol_ctrl_draw_menu(mic_vol_level, 0, spk_vol_level, 1, active);

#if SAVE_COP_SPK_MIC_VOLUME
    if (add_val > 0)
        write_volume_level(CUR_VOL_LEVEL_FILE, mic_vol_level, spk_vol_level);
#endif
}

void outspk_volume_step_up(int add_val, int active)
{
	if (outspk_vol_level == 100)
		return;

    if (outspk_vol_level < 100)
        outspk_vol_level += add_val;

    if (outspk_vol_level > 100)
        outspk_vol_level = 100;

	if (add_val <= 0)
		iovol_ctrl_draw_menu(inspk_vol_level, 0, outspk_vol_level, 1, active);

	if (add_val > 0)
		send_cop2avc_vol_info(0, inspk_vol_level/10, outspk_vol_level/10);
}

void mic_volume_step_down(int sub_val, int active)
{
    if (mic_vol_level > 0)
        mic_vol_level -= sub_val;

    if (mic_vol_level < 0)
        mic_vol_level = 0;

    if (pa_mic_volume_enable)
        cop_pa_and_call_mic_volume_set();

    if (sub_val > 0)
        vol_ctrl_draw_menu(mic_vol_level, 1, -1, 0, active);
    else
        vol_ctrl_draw_menu(mic_vol_level, 1, spk_vol_level, 0, active);

#if SAVE_COP_SPK_MIC_VOLUME
    if (sub_val > 0)
        write_volume_level(CUR_VOL_LEVEL_FILE, mic_vol_level, spk_vol_level);
#endif
}

void inspk_volume_step_down(int sub_val, int active)
{
	if (inspk_vol_level == 0)
		return;

    if (inspk_vol_level > 0)
        inspk_vol_level -= sub_val;

    if (inspk_vol_level < 0)
        inspk_vol_level = 0;

	if(sub_val <= 0)
		iovol_ctrl_draw_menu(inspk_vol_level, 1, outspk_vol_level, 0, active);

    if (sub_val > 0)
		send_cop2avc_vol_info(0, inspk_vol_level/10, outspk_vol_level/10);
}

void inout_spk_volume_setup(int *global_menu_active, int *iovmenu_active, int *iovol_updown_index, int inspk_vol, int outspk_vol)
{
	if(inspk_vol == 0) {
		inspk_vol_level = 0;
	} else {
		if( (inspk_vol <= 10) && (inspk_vol >= 0) )
			inspk_vol_level = inspk_vol * 10;
		else {
			printf("PAMP IN Speaker Volume Setup Error(%d : %d)\n", inspk_vol, inspk_vol_level);
		}
	}

	if(outspk_vol == 0) {
		outspk_vol_level = 0;
	} else {
		if( (outspk_vol <= 10) && (outspk_vol >= 0) )
		outspk_vol_level = outspk_vol * 10;
	}

	if ( ((*global_menu_active & COP_MENU_MASK) == COP_MENU_PAMP_VOL) && *iovmenu_active) { 
		switch(*iovol_updown_index) {
			case 1:
				iovol_ctrl_draw_menu(inspk_vol_level, 0, outspk_vol_level, 1, 1);
				iovol_ctrl_draw_menu(inspk_vol_level, 1, outspk_vol_level, 0, 1);
				break;

			case 2:
				iovol_ctrl_draw_menu(inspk_vol_level, 1, outspk_vol_level, 0, 1);
				iovol_ctrl_draw_menu(inspk_vol_level, 0, outspk_vol_level, 1, 1);
				break;

			default:
				break;
		}
	}

	write_pamp_volume_level(CUR_PAMP_VOL_LEVEL_FILE, inspk_vol_level, outspk_vol_level);
}

void spk_volume_step_down(int sub_val, int active)
{
    if (spk_vol_level > 0)
        spk_vol_level -= sub_val;

    if (spk_vol_level < 0)
        spk_vol_level = 0;

    if (spk_volume_enable)
        cop_speaker_volume_set();

    if (sub_val > 0)
        vol_ctrl_draw_menu(-1, 0, spk_vol_level, 1, active);
    else
        vol_ctrl_draw_menu(mic_vol_level, 0, spk_vol_level, 1, active);

#if SAVE_COP_SPK_MIC_VOLUME
    if (sub_val > 0)
        write_volume_level(CUR_VOL_LEVEL_FILE, mic_vol_level, spk_vol_level);
#endif
}

void outspk_volume_step_down(int sub_val, int active)
{
	if (outspk_vol_level == 0)
		return;

    if (outspk_vol_level > 0)
        outspk_vol_level -= sub_val;

    if (outspk_vol_level < 0)
        outspk_vol_level = 0;

	if (sub_val <= 0)
		iovol_ctrl_draw_menu(inspk_vol_level, 0, outspk_vol_level, 1, active);

	if (sub_val > 0)
		send_cop2avc_vol_info(0, inspk_vol_level/10, outspk_vol_level/10);
}

void ioraw_volume_step_up(int active)
{
    iovol_ctrl_draw_menu(inspk_vol_level, 0, outspk_vol_level, 0, active);
}

void raw_volume_step_up(int active)
{
    vol_ctrl_draw_menu(mic_vol_level, 0, spk_vol_level, 0, active);
}

int cop_pa_and_call_mic_volume_set(void)
{
    int ret = 0;
    unsigned int vol;

    DBG_LOG("\n");
    pa_mic_vol_set_from_other_func = 1;
#if 0
    if (DeadmanAlarm_running || FireAlarm_running)
#else
    if (DeadmanAlarm_running)
#endif
        return 0;

    DBG_LOG("mic_col_level = %d\n", mic_vol_level);
    vol = get_mic_atten_value(mic_vol_level);
    vol |= PA_AND_CALL_MIC_LM1971_LOAD << 8; /* LOAD 4 */
    ret = ioctl(fd_volume, LM1971_SET_VOLUME, &vol);

    pa_mic_volume_enable = 1;

    return ret;
}

int cop_pa_mic_volume_mute_set(void)
{
    int ret;
    unsigned int vol;

    DBG_LOG("\n");
#if 0
    if (!DeadmanAlarm_running && !FireAlarm_running)
#else
    if (!DeadmanAlarm_running)
#endif
        pa_mic_vol_set_from_other_func = 0;

    vol = LM1971_VOLUME_MIN_VALUE;
    vol |= PA_AND_CALL_MIC_LM1971_LOAD << 8; // LOAD 4
    ret = ioctl(fd_volume, LM1971_SET_MUTE, &vol);

    pa_mic_volume_enable = 0;

    return ret;
}

/*
int cop_call_mic_volume_set(void)
{
    int ret = 0;
    unsigned int vol;

    DBG_LOG("\n");
    occ_mic_vol_set_from_other_func = 1;
#if 0
    if (DeadmanAlarm_running || FireAlarm_running)
#else
    if (DeadmanAlarm_running)
#endif
        return 0;

    DBG_LOG("mic_col_level = %d\n", mic_vol_level);
    vol = get_mic_atten_value(mic_vol_level);
    vol |= CALL_MIC_LM1971_LOAD << 8; // LOAD 2
    ret = ioctl(fd_volume, LM1971_SET_VOLUME, &vol);

    call_mic_volume_enable = 1;

    return ret;
}

int cop_call_mic_volume_mute_set(void)
{
    int ret;
    unsigned int vol;

    DBG_LOG("\n");
#if 0
    if (!DeadmanAlarm_running && !FireAlarm_running)
#else
    if (!DeadmanAlarm_running)
#endif
        occ_mic_vol_set_from_other_func = 0;

    vol = LM1971_VOLUME_MIN_VALUE;
    vol |= CALL_MIC_LM1971_LOAD << 8;
    ret = ioctl(fd_volume, LM1971_SET_MUTE, &vol);

    call_mic_volume_enable = 0;

    return ret;
}
*/
/*************************************************************/

/***** COP SPEAKER Volume  *****************************************/
int fire_cop_speaker_volume_set(void)
{
	int ret = 0;
	unsigned int atten;

	DBG_LOG("\n");

	atten = LM1971_VOLUME_MAX_VALUE;

	DBG_LOG("atten = %d\n", atten);
	atten |= COP_SPK_LM1971_LOAD << 8; /* LOAD 2 */
	ret = ioctl(fd_volume, LM1971_SET_VOLUME, &atten);

	spk_volume_enable = 0;
	return ret;
}

int cop_speaker_volume_set(void)
{
    int ret = 0;
    unsigned int atten;

    DBG_LOG("\n");
    cop_spk_vol_set_from_other_func = 1;

    atten = get_spk_atten_value(spk_vol_level);

    DBG_LOG("atten = %d, spk_vol_level = %d\n", atten, spk_vol_level);
    atten |= COP_SPK_LM1971_LOAD << 8; /* LOAD 2 */
    ret = ioctl(fd_volume, LM1971_SET_VOLUME, &atten);

    spk_volume_enable = 1;

    return ret;
}

int cop_speaker_volume_mute_set(void)
{
    int ret;
    unsigned int vol;

    DBG_LOG("\n");
    cop_spk_vol_set_from_other_func = 0;

    if (DeadmanAlarm_running)
        return 0;

    vol = LM1971_VOLUME_MIN_VALUE;
    vol |= COP_SPK_LM1971_LOAD << 8;	/* LoAD 2 */
    ret = ioctl(fd_volume, LM1971_SET_MUTE, &vol);

    spk_volume_enable = 0;

    return ret;
}


int deadman_cop_speaker_volume_set(void)
{
    int ret = 0;
    unsigned int atten;

    DBG_LOG("\n");
    atten = LM1971_VOLUME_MAX_VALUE;

    DBG_LOG("atten = %d\n", atten);
    atten |= COP_SPK_LM1971_LOAD << 8; /* LOAD 2 */
    ret = ioctl(fd_volume, LM1971_SET_VOLUME, &atten);

    spk_volume_enable = 0;

    return ret;
}



int deadman_cop_speaker_volume_mute_set(void)
{
    int ret;
    unsigned int vol;

    if (pa_mic_vol_set_from_other_func) {
        cop_pa_and_call_mic_volume_set();
        pa_mic_vol_set_from_other_func = 0;
    }

    if (occ_mic_vol_set_from_other_func) {
        occ_mic_vol_set_from_other_func = 0;
        //occ_mic_volume_set();
        set_occ_mic_audio_switch_on();
    }

    if (cop_spk_vol_set_from_other_func) {
        cop_spk_vol_set_from_other_func = 0;
        cop_speaker_volume_set();
        return 0;
    }

    vol = LM1971_VOLUME_MIN_VALUE;
    vol |= COP_SPK_LM1971_LOAD << 8;	/* LoAD 2 */
    ret = ioctl(fd_volume, LM1971_SET_MUTE, &vol);

    spk_volume_enable = 0;

    return ret;
}
/*************************************************************/

/***** OCC SPEAKER and MIC Volume  *****************************************/
int occ_speaker_volume_set(unsigned char atten)
{
    int ret = 0;
    unsigned int vol;

    vol = (unsigned int)atten;
    vol |= OCC_SPK_LM1971_LOAD << 8; // LOAD 1
    ret = ioctl(fd_volume, LM1971_SET_VOLUME, &vol);

    return ret;
}

int occ_speaker_volume_mute_set(void)
{
    int ret;
    unsigned int vol;

    vol = LM1971_VOLUME_MIN_VALUE;
    vol |= OCC_SPK_LM1971_LOAD << 8; /* LOAD 1 */
    ret = ioctl(fd_volume, LM1971_SET_MUTE, &vol);

    return ret;
}

int occ_mic_volume_set(unsigned char atten)
{
    int ret = 0;
    unsigned int vol;

    occ_mic_vol_set_from_other_func = 1;
#if 0
    if (DeadmanAlarm_running || FireAlarm_running)
#else
    if (DeadmanAlarm_running)
#endif
        return 0;

    vol = (unsigned int)atten;
    vol |= OCC_MIC_LM1971_LOAD << 8; // LOAD 3
    ret = ioctl(fd_volume, LM1971_SET_VOLUME, &vol);

    return ret;
}

int occ_mic_volume_mute_set(void)
{
    int ret;
    unsigned int vol;

#if 0
    if (!DeadmanAlarm_running && !FireAlarm_running)
#else
    if (!DeadmanAlarm_running)
#endif
        occ_mic_vol_set_from_other_func = 0;

    vol = LM1971_VOLUME_MIN_VALUE;
    vol |= OCC_MIC_LM1971_LOAD << 8; /* LOAD 3 */
    ret = ioctl(fd_volume, LM1971_SET_MUTE, &vol);

    return ret;
}
/*************************************************************/

static int vs10xx_is_ready(int fd)
{
    int ready;

    ioctl(fd, VS1053_CTL_ISREADY, &ready);

    return ready;
}

static int vs10xx_set_swreset(int fd)
{
    int ret;
    ret = ioctl(fd, VS1053_CTL_SWRESET, 0);
    return ret;
}

static void vs10xx_set_reg(int fd, unsigned char reg, unsigned short value)
{
    struct vs1053_scireg scireg;

    scireg.reg = reg; 
    scireg.msb = (unsigned char) (value >> 8);
    scireg.lsb = (unsigned char) (value & 0xff);
    ioctl(fd, VS1053_CTL_SETSCIREG, &scireg);
}

static unsigned short vs10xx_get_reg(int fd, unsigned char reg, unsigned char bChgEndian)
{
    struct vs1053_scireg scireg;
    unsigned short value;

    scireg.reg = reg; 
    scireg.msb = 0;
    scireg.lsb = 0;
    ioctl(fd, VS1053_CTL_GETSCIREG, &scireg);

#if 0
    if(bChgEndian == 1)
        value = scireg.lsb<<8 | scireg.msb;
    else
#endif
        value = scireg.msb<<8 | scireg.lsb;

    return value;
}

static void vs10xx_queue_flush(int fd)
{
    ioctl(fd, VS1053_CTL_QUE_FLUSH, NULL);
}

#if 0
int vs10xx_enc0_buffer_clear(void)
{
    vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, 0x4800); // SM_LINE1 | SM_SDINEW
    //vs10xx_set_reg(fd_mic_enc, dVS10xx_SCI_MODE, 0x0800); // SM_SDINEW
    //ret = vs10xx_get_reg(fd_mic_enc, dVS10xx_SCI_HDAT1, 0);
//printf("vs10xx_enc0_buffer_clear(), ret: %d\n", ret);

    return 0;
}

void vs10xx_dec_queue_flush(void)
{
    if (fd_spk_dec > 0)
        vs10xx_queue_flush(fd_spk_dec);
}
#endif

#if 0
static int vs10xx_get_volume(int fd, struct vs1053_volume *vol)
{
    int ret;

    ret = ioctl(fd, VS1053_CTL_GETVOLUME, vol);

    return ret;
}

static int vs10xx_set_volume(int fd, struct vs1053_volume *vol)
{
    int ret;

    ret = ioctl(fd, VS1053_CTL_SETVOLUME, vol);

    return ret;
}
#endif

#if 0
static unsigned short VS1053_Read_endFillByte(int fd)
{
    unsigned short endfill;

    vs10xx_set_reg(fd, dVS10xx_SCI_WRAMADDR, dVS10xx_EP_endFillByte);
    endfill = vs10xx_get_reg(fd, dVS10xx_SCI_WRAM, 0);
    //printf("EndFill Bytes: 0x%X\n", endfill);

    return endfill;
}
#endif

static int __cop_pa_and_call_audio_sending(void)
{
    int enc_fd = fd_mic_enc;
    int rctr = 1;
    //int len;

    if (fd_mic_enc < 0) return 0;

    if (cop_pa_mic_WaitWord < __COP_PA_AND_CALL_ENCODING_LIMIT) {
        cop_pa_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
//printf("%s, waitWord: %d.\n", __func__, cop_pa_mic_WaitWord);
        return 0;
    }

    if (cop_pa_mic_WaitWord >= (__COP_PA_AND_CALL_ENCODING_LIMIT*2)) {
//printf("%s, CHKKK1\n", __func__);
        rctr = 2;
    }

    cop_pa_sciburstreg.size = __COP_PA_AND_CALL_ENCODING_LIMIT;
    cop_pa_sciburstreg.data = cop_pa_and_call_enc_tx_packet.Palyload;

    while (rctr > 0) {
//printf("%s, CHKKK2222222222\n", __func__);
        cop_pa_mic_WaitWord -= __COP_PA_AND_CALL_ENCODING_LIMIT;
        ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &cop_pa_sciburstreg);

        send_cop_multi_tx_data((char *)&cop_pa_and_call_enc_tx_packet, cop_pa_and_call_tx_pkt_len, 11);

        if (enc_cop_pa_and_call_tx_rtpseq < 65535)
            enc_cop_pa_and_call_tx_rtpseq++;
        else
            enc_cop_pa_and_call_tx_rtpseq = 1;

        enc_cop_pa_and_call_tx_rtpTS += 2160; // 64kbps
        //enc_cop_pa_and_call_tx_rtpTS += 2351; // 128kbps

        cop_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(enc_cop_pa_and_call_tx_rtpseq);
        cop_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(enc_cop_pa_and_call_tx_rtpTS);

        rctr--;
    }

    return cop_pa_and_call_tx_pkt_len;
}

static int __cop_pa_and_call_audio_discard(void)
{
    int enc_fd = fd_mic_enc;
    int rctr = 1;
    //int len;

    if (fd_mic_enc < 0) return 0;

    if (cop_pa_mic_WaitWord < __COP_PA_AND_CALL_ENCODING_LIMIT) {
        cop_pa_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
//printf("%s, waitWord: %d.\n", __func__, cop_pa_mic_WaitWord);
        return 0;
    }

    if (cop_pa_mic_WaitWord >= (__COP_PA_AND_CALL_ENCODING_LIMIT*2)) {
//printf("%s, CHKKK1\n", __func__);
        rctr = 2;
    }

    cop_pa_sciburstreg.size = __COP_PA_AND_CALL_ENCODING_LIMIT;
    cop_pa_sciburstreg.data = cop_pa_and_call_enc_tx_packet.Palyload;

    while (rctr > 0) {
//printf("%s, CHKKK2222222222\n", __func__);
        cop_pa_mic_WaitWord -= __COP_PA_AND_CALL_ENCODING_LIMIT;
        ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &cop_pa_sciburstreg);

        if (enc_cop_pa_and_call_tx_rtpseq < 65535)
            enc_cop_pa_and_call_tx_rtpseq++;
        else
            enc_cop_pa_and_call_tx_rtpseq = 1;

        enc_cop_pa_and_call_tx_rtpTS += 2160; // 64kbps
        //enc_cop_pa_and_call_tx_rtpTS += 2351; // 128kbps

        cop_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(enc_cop_pa_and_call_tx_rtpseq);
        cop_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(enc_cop_pa_and_call_tx_rtpTS);

        rctr--;
    }

    return cop_pa_and_call_tx_pkt_len;
}

static int __call_audio_getting_to_send_occ_out(int enc_fd)
{
    int rctr = 1;

    if (enc_fd < 0) return 0;

    if (cop_pa_mic_WaitWord < __COP_PA_AND_CALL_ENCODING_LIMIT) {
        cop_pa_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
//printf("%s(), waitWord: %d.\n", __func__, cop_pa_mic_WaitWord);
        return 0;
     }

    /*if (cop_pa_mic_WaitWord >= (__COP_PA_AND_CALL_ENCODING_LIMIT*2)) {
//printf("%s, CHKKK1, %d\n", __func__, cop_pa_mic_WaitWord);
        rctr = 2;
    }*/
    cop_pa_sciburstreg.size = __COP_PA_AND_CALL_ENCODING_LIMIT;
    cop_pa_sciburstreg.data = cop_pa_and_call_enc_tx_packet.Palyload;

    while (rctr > 0) {
        cop_pa_mic_WaitWord -= __COP_PA_AND_CALL_ENCODING_LIMIT;
        ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &cop_pa_sciburstreg);

        rctr--;
    }

    return cop_pa_and_call_tx_pkt_len;
}

static void increase_call_tx_rtpseq(void)
{
	if (enc_cop_pa_and_call_tx_rtpseq < 65535)
		enc_cop_pa_and_call_tx_rtpseq++;
	else
		enc_cop_pa_and_call_tx_rtpseq = 1;

	enc_cop_pa_and_call_tx_rtpTS += 2160; // 64kbps
	//enc_cop_pa_and_call_tx_rtpTS += 2351; // 128kbps

	cop_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(enc_cop_pa_and_call_tx_rtpseq);
	cop_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(enc_cop_pa_and_call_tx_rtpTS);
}

static int __call_audio_sending(int enc_fd, int passive_on, int mon_on, int cantalk)
{
    int rctr = 1;

    if (enc_fd < 0) return 0;

    if (cop_pa_mic_WaitWord < __COP_PA_AND_CALL_ENCODING_LIMIT) {
        cop_pa_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
//    printf("CALL SENDING, waitWord: %d.\n", occ_pa_and_occ_call_mic_WaitWord);
        return 0;
     }

    if (cop_pa_mic_WaitWord >= (__COP_PA_AND_CALL_ENCODING_LIMIT*2)) {
//printf("%s, CHKKK1, %d\n", __func__, cop_pa_mic_WaitWord);
        rctr = 2;
    }
    cop_pa_sciburstreg.size = __COP_PA_AND_CALL_ENCODING_LIMIT;
    cop_pa_sciburstreg.data = cop_pa_and_call_enc_tx_packet.Palyload;

    while (rctr > 0) {
        cop_pa_mic_WaitWord -= __COP_PA_AND_CALL_ENCODING_LIMIT;
        ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &cop_pa_sciburstreg);

        if (cantalk)
            send_call_multi_tx_data((char *)&cop_pa_and_call_enc_tx_packet, cop_pa_and_call_tx_pkt_len);
        if (passive_on)
            send_cop_multi_tx_data_passive_on_call((char *)&cop_pa_and_call_enc_tx_packet, cop_pa_and_call_tx_pkt_len);
        if (mon_on)
            send_cop_multi_mon_data((char *)&cop_pa_and_call_enc_tx_packet, cop_pa_and_call_tx_pkt_len);

		if (cantalk || passive_on || mon_on) {
			if (enc_cop_pa_and_call_tx_rtpseq < 65535)
				enc_cop_pa_and_call_tx_rtpseq++;
			else
				enc_cop_pa_and_call_tx_rtpseq = 1;

			enc_cop_pa_and_call_tx_rtpTS += 2160; // 64kbps
			//enc_cop_pa_and_call_tx_rtpTS += 2351; // 128kbps

			cop_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(enc_cop_pa_and_call_tx_rtpseq);
			cop_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(enc_cop_pa_and_call_tx_rtpTS);
		}

        rctr--;
    }

    return cop_pa_and_call_tx_pkt_len;
}

static int __call_audio_discard(int enc_fd, int passive_on, int mon_on, int cantalk)
{
    int rctr = 1;

    if (enc_fd < 0) return 0;

    if (cop_pa_mic_WaitWord < __COP_PA_AND_CALL_ENCODING_LIMIT) {
        cop_pa_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
//    printf("CALL SENDING, waitWord: %d.\n", occ_pa_and_occ_call_mic_WaitWord);
        return 0;
     }

    if (cop_pa_mic_WaitWord >= (__COP_PA_AND_CALL_ENCODING_LIMIT*2)) {
//printf("%s, CHKKK1, %d\n", __func__, cop_pa_mic_WaitWord);
        rctr = 2;
    }
    cop_pa_sciburstreg.size = __COP_PA_AND_CALL_ENCODING_LIMIT;
    cop_pa_sciburstreg.data = cop_pa_and_call_enc_tx_packet.Palyload;

    while (rctr > 0) {
        cop_pa_mic_WaitWord -= __COP_PA_AND_CALL_ENCODING_LIMIT;
        ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &cop_pa_sciburstreg);

		if (cantalk || passive_on || mon_on) {
			if (enc_cop_pa_and_call_tx_rtpseq < 65535)
				enc_cop_pa_and_call_tx_rtpseq++;
			else
				enc_cop_pa_and_call_tx_rtpseq = 1;

			enc_cop_pa_and_call_tx_rtpTS += 2160; // 64kbps
			//enc_cop_pa_and_call_tx_rtpTS += 2351; // 128kbps

			cop_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(enc_cop_pa_and_call_tx_rtpseq);
			cop_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(enc_cop_pa_and_call_tx_rtpTS);
		}

        rctr--;
    }

    return cop_pa_and_call_tx_pkt_len;
}

static int __voip_call_audio_sending(int voip_fd)
{
    frame_size = get_mic_enc_wav_frame();//G711_buf);
    if(frame_size>0) {
        send_call_multi_tx_data((char *)&pei_voip_call_enc_tx_packet, pei_voip_call_tx_pkt_len);

        //printf("pei_voip_call_enc_tx_rtpseq = %d\n", pei_voip_call_enc_tx_rtpseq);

        if (pei_voip_call_enc_tx_rtpseq < (unsigned short)0xFFFF)
            pei_voip_call_enc_tx_rtpseq++;
        else
            pei_voip_call_enc_tx_rtpseq = 1;

		pei_voip_call_enc_tx_rtpTS += 160; //8kbps

        pei_voip_call_enc_tx_packet.rtpHeader.seq = htons(pei_voip_call_enc_tx_rtpseq);
        pei_voip_call_enc_tx_packet.rtpHeader.ts = htonl(pei_voip_call_enc_tx_rtpTS);
    }

#if 0
    // Decoding the G.711 audio date from RTP again on here
    {
        char *tmpbuf;
        int len = 0;

        rxbuf = try_read_call_rx_data(&len, 0, 1);
        
        printf("len = %d\n", len);

        while(len) {
            running_call_recv_and_dec(0, tmpbuf, len, 1, 0);
            
            len = 0;

            tmpbuf = try_read_call_rx_data(&len, 0, 1);
        }
    }
#endif

    return frame_size;
}

#if 0
static int __call_and_occ_pa_audio_sending(int enc_fd, int passive_on, int mon_on, int cantalk)
{
    int rctr = 1;

    if (occ_pa_and_occ_call_mic_WaitWord < __CALL_ENCODING_LIMIT) {
        occ_pa_and_occ_call_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
        return 0;
     }

    if (occ_pa_and_occ_call_mic_WaitWord >= (__CALL_ENCODING_LIMIT*2)) {
//printf("%s, CHKKK1, %d\n", __func__, occ_pa_and_occ_call_mic_WaitWord);
        rctr = 2;
    }
    occ_pa_and_call_sciburstreg.size = __CALL_ENCODING_LIMIT;
    occ_pa_and_call_sciburstreg.data = occ_pa_and_call_enc_tx_packet.Palyload;

    while (rctr > 0) {
        occ_pa_and_occ_call_mic_WaitWord -= __CALL_ENCODING_LIMIT;
        ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &occ_pa_and_call_sciburstreg);

        if (cantalk)
            send_occ_pa_multi_tx_data((char *)&occ_pa_and_call_enc_tx_packet, occ_pa_and_call_tx_pkt_len);
        if (passive_on)
            send_cop_multi_tx_data_passive_on_call((char *)&occ_pa_and_call_enc_tx_packet, occ_pa_and_call_tx_pkt_len);
        if (mon_on)
            send_cop_multi_mon_data((char *)&occ_pa_and_call_enc_tx_packet, occ_pa_and_call_tx_pkt_len);

        if (occ_pa_and_call_tx_rtpseq < 65535)
            occ_pa_and_call_tx_rtpseq++;
        else
            occ_pa_and_call_tx_rtpseq = 1;

        occ_pa_and_call_tx_rtpTS += 2160; // 64kbps
        //enc_tx_occ_pa_rtpTS += 2351; // 128kbps

        occ_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(occ_pa_and_call_tx_rtpseq);
        occ_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(occ_pa_and_call_tx_rtpTS);

        rctr--;
    }

    return occ_pa_and_call_tx_pkt_len;
}
#endif

static int __occ_pa_and_occ_call_audio_sending(int enc_fd, int cantalk, int is_occ_pa, int is_occ_call)
{
    int rctr = 1;

    if (occ_pa_and_occ_call_mic_WaitWord < __CALL_ENCODING_LIMIT) {
        occ_pa_and_occ_call_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
        return 0;
     }

    if (occ_pa_and_occ_call_mic_WaitWord >= (__CALL_ENCODING_LIMIT*2)) {
//printf("%s, CHKKK1, %d\n", __func__, occ_pa_and_occ_call_mic_WaitWord);
        rctr = 2;
    }
    occ_pa_and_call_sciburstreg.size = __CALL_ENCODING_LIMIT;
    occ_pa_and_call_sciburstreg.data = occ_pa_and_call_enc_tx_packet.Palyload;

    while (rctr > 0) {
        occ_pa_and_occ_call_mic_WaitWord -= __CALL_ENCODING_LIMIT;
        ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &occ_pa_and_call_sciburstreg);

		if (cantalk) {
			if (is_occ_pa)
				send_occ_pa_multi_tx_data((char *)&occ_pa_and_call_enc_tx_packet, occ_pa_and_call_tx_pkt_len);
			else if (is_occ_call)
				send_call_multi_tx_data((char *)&occ_pa_and_call_enc_tx_packet, occ_pa_and_call_tx_pkt_len);

			if (occ_pa_and_call_tx_rtpseq < 65535)
				occ_pa_and_call_tx_rtpseq++;
			else
				occ_pa_and_call_tx_rtpseq = 1;

			occ_pa_and_call_tx_rtpTS += 2160; // 64kbps
			//enc_tx_occ_pa_rtpTS += 2351; // 128kbps

			occ_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(occ_pa_and_call_tx_rtpseq);
			occ_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(occ_pa_and_call_tx_rtpTS);
		}

        rctr--;
    }

    return occ_pa_and_call_tx_pkt_len;
}

static int __monitoring_audio_sending(int enc_fd, int file_fd, int send_en)
{
    if (is_open_multi_mon_sock() < 0)
        return -1;

    if (monitor_WaitWord < __MON_ENCODING_LIMIT) {
        monitor_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
//printf("%s, monitor_WaitWord: %d.\n", __func__, monitor_WaitWord);
        return 0;
     }

    //printf("%s, waitWord: %d.\n", __func__, waitWord);

    monitor_WaitWord -= __MON_ENCODING_LIMIT;

    mon_sciburstreg.size = __MON_ENCODING_LIMIT;
    mon_sciburstreg.data = mon_enc_tx_packet.Palyload;
    ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &mon_sciburstreg);
    //memcpy((unsigned char *)mon_enc_tx_packet.Palyload, (unsigned char *)mon_sciburstreg.data, __MON_ENCODING_LIMIT*2);

    if (send_en)
        send_cop_multi_mon_data((char *)&mon_enc_tx_packet, mon_tx_pkt_len);

    if (enc_mon_rtpseq < 65535)
        enc_mon_rtpseq++;
    else
        enc_mon_rtpseq = 1;

    enc_mon_rtpTS += 2160; // 64kbps
    //enc_mon_rtpTS += 2351; // 128kbps

    mon_enc_tx_packet.rtpHeader.seq = htons(enc_mon_rtpseq);
    mon_enc_tx_packet.rtpHeader.ts = htonl(enc_mon_rtpTS);

#ifdef PEI_MONITORING_SAVE
    if (file_fd > 0) {
        len = write(file_fd, (unsigned char *)mon_enc_tx_packet.Palyload, __MON_ENCODING_LIMIT*2);
        if (len != __MON_ENCODING_LIMIT*2) {
            printf("< SAVE FILE ERROR: %d >\n", file_fd);
        }
    }
#endif

    return __MON_ENCODING_LIMIT;
}

#if 0
static int __cop_pa_audio_sending_while_peiocc_call(int enc_fd)
{
    if (cop_pa_mic_WaitWord < __COP_PA_AND_CALL_ENCODING_LIMIT) {
        cop_pa_mic_WaitWord = vs10xx_get_reg(enc_fd, dVS10xx_SCI_HDAT1, 0);
//printf("%s, waitWord: %d.\n", __func__, cop_pa_mic_WaitWord);
        return 0;
     }


    cop_pa_mic_WaitWord -= __COP_PA_AND_CALL_ENCODING_LIMIT;

    ioctl(enc_fd, VS1053_CTL_GETSCIBURSTREG, &cop_pa_sciburstreg);

    memcpy((unsigned char *)cop_pa_and_call_enc_tx_packet.Palyload, (unsigned char *)cop_pa_sciburstreg.data, __COP_PA_AND_CALL_ENCODING_LIMIT*2);
    //memcpy((unsigned char *)cop_pa_and_call_enc_tx_packet.Palyload, (unsigned char *)sciburstreg.data, waitWord << 1);

    ///*len =*/ send_cop_multi_tx_data((char *)&cop_pa_and_call_enc_tx_packet, cop_pa_and_call_tx_pkt_len);
    /*len =*/ send_cop_multi_tx_data_passive_on_call((char *)&cop_pa_and_call_enc_tx_packet, cop_pa_and_call_tx_pkt_len);

    if (enc_cop_pa_and_call_tx_rtpseq < 65535)
        enc_cop_pa_and_call_tx_rtpseq++;
    else
        enc_cop_pa_and_call_tx_rtpseq = 0;

    enc_cop_pa_and_call_tx_rtpTS += 2160; // 64kbps
    //enc_cop_pa_and_call_tx_rtpTS += 2351; // 128kbps

    cop_pa_and_call_enc_tx_packet.rtpHeader.seq = htons(enc_cop_pa_and_call_tx_rtpseq);
    cop_pa_and_call_enc_tx_packet.rtpHeader.ts = htonl(enc_cop_pa_and_call_tx_rtpTS);

    return __COP_PA_AND_CALL_ENCODING_LIMIT;
}
#endif

int running_cop_pa_and_call_mic_audio_sending(void)
{
    int len;

#ifdef __DEBUG_DISCARD_STATUS__
printf("running pa mic, enc0\n");
#endif
    if (fd_mic_enc < 0) return 0;

    len = __cop_pa_and_call_audio_sending();

    return len;
}

int running_cop_pa_and_call_mic_audio_discard(void)
{
    int len;

#ifdef __DEBUG_DISCARD_STATUS__
printf("running pa mic, enc0\n");
#endif
    if (fd_mic_enc < 0) return 0;

    len = __cop_pa_and_call_audio_discard();

    return len;
}

int running_call_audio_sending_to_occ_out(int stop)
{
    unsigned int len;
#ifdef __DEBUG_DISCARD_STATUS__
printf("running call pa, enc1\n");
#endif

    if (fd_mic_enc < 0) return 0;

    len = __call_audio_getting_to_send_occ_out(fd_mic_enc);

	if (len > 0) {
		if (stop)
			running_bcast_recv_and_dec(0, (char *)&cop_pa_and_call_enc_tx_packet, len, 0, 1, 0);
		else
			running_bcast_recv_and_dec(0, (char *)&cop_pa_and_call_enc_tx_packet, len, 1, 0, 0);

		increase_call_tx_rtpseq();
	}

    return len;
}

int running_call_audio_sending(int passive_also_on, int mon_also_on, int cantalk)
{
    unsigned int len;
#ifdef __DEBUG_DISCARD_STATUS__
printf("running call pa, enc1\n");
#endif

    if (fd_mic_enc < 0) return 0;

    len = __call_audio_sending(fd_mic_enc, passive_also_on, mon_also_on, cantalk);

    return len;
}

int running_voip_call_audio_sending(void)
{
	unsigned int len;

	if (fd_voip_call < 0) return 0;

	len = __voip_call_audio_sending(fd_voip_call);

	return len;
}

int running_call_audio_discard(int passive_also_on, int mon_also_on, int cantalk)
{
	unsigned int len;
#ifdef __DEBUG_DISCARD_STATUS__
	printf("running call pa, enc1\n");
#endif

	if (fd_mic_enc < 0) return 0;

	len = __call_audio_discard(fd_mic_enc, passive_also_on, mon_also_on, cantalk);

	return len;
}

int running_occ_pa_and_occ_call_audio_sending(int cantalk, int is_occ_pa, int is_occ_call)
{
    unsigned int len;
#ifdef __DEBUG_DISCARD_STATUS__
printf("running call pa, enc1\n");
#endif

    if (fd_mic_enc < 0) return 0;

    len = __occ_pa_and_occ_call_audio_sending(fd_mic_enc, cantalk, is_occ_pa, is_occ_call);

    return len;
}

#if 0
int running_call_and_occ_pa_audio_sending(int passive_also_on, int mon_also_on, int cantalk)
{
    unsigned int len;
#ifdef __DEBUG_DISCARD_STATUS__
printf("running call pa, enc1\n");
#endif

    if (fd_mic_enc < 0) return 0;

    len = __call_and_occ_pa_audio_sending(fd_mic_enc, passive_also_on, mon_also_on, cantalk);

    return len;
}
#endif

void occ_pa_packet_self_play(int len, int stop)
{
    if (stop)
        running_bcast_recv_and_dec(0, (char *)&occ_pa_and_call_enc_tx_packet, len, 0, 1, 0);
    else
        running_bcast_recv_and_dec(0, (char *)&occ_pa_and_call_enc_tx_packet, len, 1, 0, 0);
}

int running_monitoring_audio_sending(int file_fd, int send_en)
{
    unsigned int waitWord;

    if (fd_mon_enc < 0) return 0;

    waitWord = __monitoring_audio_sending(fd_mon_enc, file_fd, send_en);

    return waitWord;
}

extern int occ_pa_running;
int __discard_cop_pa_and_call_mic_encoding_data(void)
{
    int ret = 0;
    //int rctr = 1;
    int rsize;// = __COP_PA_AND_CALL_ENCODING_LIMIT;
    static int internal_ctr = 0;

	if (occ_pa_running) return 0;

    internal_ctr++;
    if (internal_ctr < 2) return 0;

    internal_ctr = 0;

    if (fd_mic_enc < 0) return 0;

	/* <<< VOIP */
	//discard_voip_call_mic_encoding_data();
	/* VOIP >>> */

    if (cop_pa_mic_WaitWord < (__COP_PA_AND_CALL_ENCODING_LIMIT)) {
        cop_pa_mic_WaitWord = vs10xx_get_reg(fd_mic_enc, dVS10xx_SCI_HDAT1, 0);
//printf("1, discard pa and call mic:%d\n", cop_pa_mic_WaitWord);
        return 1;
    }
//printf("2, discard cop_pa:%d\n", cop_pa_mic_WaitWord);

    if (cop_pa_mic_WaitWord >= (__COP_PA_AND_CALL_ENCODING_LIMIT << 1) + __COP_PA_AND_CALL_ENCODING_LIMIT) {
        rsize = (__COP_PA_AND_CALL_ENCODING_LIMIT << 1) + __COP_PA_AND_CALL_ENCODING_LIMIT;
        //rctr = 3;
    }
    else if (cop_pa_mic_WaitWord >= (__COP_PA_AND_CALL_ENCODING_LIMIT << 1)) {
        rsize = (__COP_PA_AND_CALL_ENCODING_LIMIT << 1);
        //rctr = 2;
    }
    else
        rsize = __COP_PA_AND_CALL_ENCODING_LIMIT;

    cop_pa_sciburstreg.size = rsize;
    cop_pa_sciburstreg.data = cop_pa_enc_tmp_buf;
    //while (rctr > 0)
    {
        cop_pa_mic_WaitWord -= rsize;
        ioctl(fd_mic_enc, VS1053_CTL_GETSCIBURSTREG, &cop_pa_sciburstreg);
        //rctr--;
    }

    return ret;
}

int __discard_occ_pa_and_call_mic_encoding_data(void)
{
    int ret = 0;
    //int rctr = 1;
    int rsize;// = __CALL_ENCODING_LIMIT;

#if 0
#ifdef __DEBUG_DISCARD_STATUS__
printf("discard, occ mic\n");
#endif

    if (fd_mic_enc< 0) return 0;

    if (occ_pa_and_occ_call_mic_WaitWord < __CALL_ENCODING_LIMIT) {
        occ_pa_and_occ_call_mic_WaitWord = vs10xx_get_reg(fd_mic_enc, dVS10xx_SCI_HDAT1, 0);
//printf("1,discard occ mic:%d\n", occ_pa_and_occ_call_mic_WaitWord);
        return 1;
    }
//printf("2,dscard call:%d\n", occ_pa_and_occ_call_mic_WaitWord);

    if (occ_pa_and_occ_call_mic_WaitWord >= (__CALL_ENCODING_LIMIT<<1) + __CALL_ENCODING_LIMIT) {
        rsize = (__CALL_ENCODING_LIMIT<<1) + __CALL_ENCODING_LIMIT;
        //rctr = 3;
    }
    else if (occ_pa_and_occ_call_mic_WaitWord >= (__CALL_ENCODING_LIMIT<<1)) {
        rsize = __CALL_ENCODING_LIMIT<<1;
        //rctr = 2;
    } else
        rsize = __CALL_ENCODING_LIMIT;

    occ_pa_and_call_sciburstreg.size = rsize;
    occ_pa_and_call_sciburstreg.data = occ_call_enc_tmp_buf;

    //while (rctr > 0)
    {
        occ_pa_and_occ_call_mic_WaitWord -= rsize;
        ioctl(fd_mic_enc, VS1053_CTL_GETSCIBURSTREG, &occ_pa_and_call_sciburstreg);
        //rctr--;
    }

#endif
    return ret;
}

int __discard_occ_pa_mic_encoding_data_for_test(void)
{
    int ret = 0;
    //int rctr = 1;
    int rsize;// = __CALL_ENCODING_LIMIT;

#ifdef __DEBUG_DISCARD_STATUS__
printf("discard, occ mic\n");
#endif

    if (fd_mic_enc< 0) return 0;

    if (occ_pa_and_occ_call_mic_WaitWord < __CALL_ENCODING_LIMIT) {
        occ_pa_and_occ_call_mic_WaitWord = vs10xx_get_reg(fd_mic_enc, dVS10xx_SCI_HDAT1, 0);
//printf("1,discard occ mic:%d\n", occ_pa_and_occ_call_mic_WaitWord);
        return 1;
    }
//printf("2,dscard call:%d\n", occ_pa_and_occ_call_mic_WaitWord);

    if (occ_pa_and_occ_call_mic_WaitWord >= (__CALL_ENCODING_LIMIT<<1) + __CALL_ENCODING_LIMIT) {
        rsize = (__CALL_ENCODING_LIMIT<<1) + __CALL_ENCODING_LIMIT;
        //rctr = 3;
    }
    else if (occ_pa_and_occ_call_mic_WaitWord >= (__CALL_ENCODING_LIMIT<<1)) {
        rsize = __CALL_ENCODING_LIMIT<<1;
        //rctr = 2;
    } else
        rsize = __CALL_ENCODING_LIMIT;

    occ_pa_and_call_sciburstreg.size = rsize;
    occ_pa_and_call_sciburstreg.data = occ_call_enc_tmp_buf;

    //while (rctr > 0)
    {
        occ_pa_and_occ_call_mic_WaitWord -= rsize;
        ioctl(fd_mic_enc, VS1053_CTL_GETSCIBURSTREG, &occ_pa_and_call_sciburstreg);
        //rctr--;
    }

    return ret;
}

#ifdef USE_CONTINUE_MON_ENCORDING
int __discard_monitoring_encoding_data(void)
{
    int ret = 0;

    if (fd_mon_enc< 0) return 0;

    if (monitor_WaitWord < __MON_ENCODING_LIMIT) {
        monitor_WaitWord = vs10xx_get_reg(fd_mon_enc, dVS10xx_SCI_HDAT1, 0);
        return 1;
    }
    monitor_WaitWord -= __MON_ENCODING_LIMIT;

    mon_sciburstreg.size = __MON_ENCODING_LIMIT;
    mon_sciburstreg.data = mon_enc_tmp_buf;
    ioctl(fd_mon_enc, VS1053_CTL_GETSCIBURSTREG, &mon_sciburstreg);

    if (mon_enc_tmp_buf[0] == 0xFBFF) // MP3 Header
        ret = __MON_ENCODING_LIMIT;

    return ret;
}
#endif

void set_check_different_ssrc_valid(int set)
{
//printf("%s(), set:%d\n", __func__, set);
    enable_check_different_ssrc = set;
    enable_check_same_ssrc = set ^ 1;
}

void clear_old_rtp_seq(void)
{
    Seqold = 0;
    one_flame_play_time = 0;
//printf("===%s()\n", __func__);
}

void clear_old_ssrc(void)
{
    SaveStartSSRC = 0;
//printf("===%s()\n", __func__);
}

unsigned int get_save_start_ssrc(void)
{
    return SaveStartSSRC;
}

int get_one_flame_play_time(void)
{
#if 1
    int t;

    t = one_flame_play_time >> 2;
    return t;
#else
    int t, t2;

    t = one_flame_play_time >> 1;
    //t2 = t >> 1;
    //t2 = t2 >> 1;
    //t += t2;
    //t -= t2;
    return t;
#endif
}

int RTPPacketParse(int fd, char *buff, int *StartPos,int size, int type, int check_header)
{
    RTP_Header_t* pRtpData = (RTP_Header_t *)buff;
    unsigned short seq;
    int pos, mp3_header = 0, i, j;
    int MPADataSize;
	int packet_loss = 0;
	static int enable_pa_rtp_reconnect = 0;

    //unsigned int *CSRC = (unsigned int *)&buff[12];
    //char *Payload = &buff[12 + (4*pRtpData->h_info1.CC)];
    //MPEG_AudioSpec_Header_t* MPAHeader = (MPEG_AudioSpec_Header_t*) Payload;
    //char *MPAData = (char *)(Payload + 4);
    pos = (12 + (4*pRtpData->h_info1.CC) + 4); 
    MPADataSize = size - pos;
    *StartPos = pos;

    if(pRtpData->h_info2.PT != 0x0e || pRtpData->h_info1.V != 0x02) {
        printf("--> V  : 0x%02x, ", pRtpData->h_info1.V);
        printf("PT : 0x%02x\n", pRtpData->h_info2.PT);
        return -1;
    }

    mp3_header  = buff[pos+3];
    mp3_header |= buff[pos+2] << 8;
    mp3_header |= buff[pos+1] << 16;
    mp3_header |= buff[pos+0] << 24;

    seq = B2L_SWAP16(pRtpData->seq);

    /* 2014/01/22 *******************************************************************************/
    if (check_header) {
        i = mp3_header & 0xFFFE0000;
        if (i != 0xFFFA0000) {
            printf("{ NOT SUPPORT(%d): %08X, %d, %08X }\n", type, B2L_SWAP32(pRtpData->SSRC), seq, mp3_header);
            return -1;
        }
    }

    if (enable_check_different_ssrc) {
        if (SaveStartSSRC == pRtpData->SSRC) {
            printf("{ SKIP SAME: %08X, %d, %08X }\n", B2L_SWAP32(pRtpData->SSRC), seq, mp3_header);
            enable_pa_rtp_reconnect = 1;
            return -2;
        }
        else {
            enable_check_different_ssrc = 0;
            enable_check_same_ssrc = 1;

            if (enable_pa_rtp_reconnect) {
                enable_pa_rtp_reconnect = 0;
                //close_auto_fire_func_monitoring_bcast();
                //init_auto_fire_func_monitoring_bcast();

                //reset_auto_multi_rx_data_ptr();
                //reset_fire_multi_rx_data_ptr();
                //reset_func_multi_rx_data_ptr();
                //reset_cop_pa_multi_rx_data_ptr();
                //reset_occ_pa_multi_rx_data_ptr();

                Seqold = 0;
            }
        }
    }
#if 0 //2014/02/11
    else if (enable_check_same_ssrc) {
        if (!SaveStartSSRC)
            SaveStartSSRC = pRtpData->SSRC;

        if (SaveStartSSRC != pRtpData->SSRC) {
            printf("{ SKIP DIFF: %08x, %d, %08X }\n", B2L_SWAP32(pRtpData->SSRC), seq, mp3_header);
            return -1;
        }
    }
#endif
    /********************************************************************************************/

#if 0	
    printf(" size  : %d->%d",size, MPADataSize);
    printf(" V  : 0x%02x", pRtpData->h_info1.V);
    printf(" P  : 0x%02x", pRtpData->h_info1.P);
    printf(" X  : 0x%02x", pRtpData->h_info1.X);
    printf(" CC : 0x%02x", pRtpData->h_info1.CC);	
    printf(" M : 0x%02x", pRtpData->h_info2.M); 	
    printf(" PT : 0x%02x\n", pRtpData->h_info2.PT);

    printf(" SEQ  : %d", B2L_SWAP16(pRtpData->seq));
    printf(" ts  : %u", B2L_SWAP32(pRtpData->ts));
    printf(" SSRC  : 0x%08x\n", B2L_SWAP32(pRtpData->SSRC));
#endif
    if (SSRCold != pRtpData->SSRC) {
        SSRCold = pRtpData->SSRC;
        SaveStartSSRC = pRtpData->SSRC;

        //decoding_stop();

        i = (mp3_header >> 10) & 0x3;
        j = (mp3_header >> 12) & 0xF;
        RxDecFrameLen = __mpeg_l3_frame_length[i][j];
        if (mp3_header & (1<<9))
            RxDecFrameLen += 1;

        one_flame_play_time = __mpeg_l3_frame_play_time_nsec[i];

        printf("[ T:%d, %08X, %d, %08X, (%d,%d) ]\n",
                type, B2L_SWAP32(pRtpData->SSRC), seq, mp3_header, RxDecFrameLen, MPADataSize);

        if ((seq - Seqold) != 1 && !(Seqold == 0xFFFF && seq == 0)) {
            printf("[[ RTP SEQ %d -> %d, %08X ]]\n", Seqold, seq, mp3_header);
            Seqold = seq;
        }
    }
    else {
        if ((seq - Seqold) != 1 && !(Seqold == 0xFFFF && seq == 0)) {
            printf("{{ RTP SEQ %d -> %d, %08X }}\n", Seqold, seq, mp3_header);

            i = (mp3_header >> 10) & 0x3;
            one_flame_play_time = __mpeg_l3_frame_play_time_nsec[i];

            if (Seqold) {
                //close_auto_fire_func_monitoring_bcast();
                //init_auto_fire_func_monitoring_bcast();
                //reset_auto_multi_rx_data_ptr();
                //reset_fire_multi_rx_data_ptr();
                //reset_func_multi_rx_data_ptr();
                //reset_cop_pa_multi_rx_data_ptr();
                //reset_occ_pa_multi_rx_data_ptr();
                Seqold = 0;
                packet_loss = 1;
            }
        }

        if (!packet_loss)
            Seqold = seq;
    }

    return MPADataSize;
}

/* Added by Michael RYU(To use a G.711 Encoding & Decoding */
int running_call_recv_and_dec(int patype, char *buf, int len, int run, int stop)
{
    int startpos;
    int MPADataSize = 0; 
    unsigned short rWord, rWord2;
    int i, j, l, ret = 0, cl = 0; 
    int wlen = 0; 
    int seq, ssrc;
    volatile int delay;
    int ctr = 0; 

    if (fd_voip_call < 0) { 
        printf("< %s(), WARNING, DEC is not opened(%d) >\n", __func__, fd_voip_call);
        return -1;
    }    

    if (stop) {
        printf("< PLAY STOP ");
        return 0;
#if 0   // MP3 Only
        if (len > 0) {
            MPADataSize = RTPPacketParse(buf, &startpos, len);
            l = MPADataSize;
        
            seq = (buf[2] << 8) | buf[3];
        
            ssrc  = buf[8] << 24;
            ssrc |= buf[9] << 16;
            ssrc |= buf[10] << 8;
            ssrc |= buf[11];
        
            printf("%08X %d", ssrc, seq);
        } else {
            l = 0;
        }
        
        if (l <= 0) { 
            printf("\n");
            return 0;
        }    
        
        cl = l >> 5;
        if ((l - cl) > 0) cl++;

        rWord = vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_MODE, 0);
        rWord |= dVS10xx_SM_CANCEL;
        vs10xx_set_reg(fd_voip_call, dVS10xx_SCI_MODE, rWord);
        j = 0; 
        for (i = 0; i < cl; ) {
            if (l >= VS10XX_WRITE_SIZE)
                wlen = VS10XX_WRITE_SIZE;
            else
                wlen = l; 
        
            ret = write(fd_voip_call, &buf[startpos+j], wlen);
        
            if (ret == wlen) {
                rWord = vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_MODE, 0);
                if ((rWord & dVS10xx_SM_CANCEL) == 0)
                    break;
        
                l -= wlen;
                j += wlen;
                i++; 
            } else {  
                for (delay = 0; delay < SIMPLE_DELAY_CTR/4; delay++);
            }
        }    
        
        rWord = vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_HDAT0, 0);
        rWord2 = vs10xx_get_reg(fd_voip_call, dVS10xx_SCI_HDAT1, 0);
        
        if (!rWord && !rWord2)
            printf(", %d/%d, K(%d), CTR:%d >\n", i, cl, len, ctr);
        else 
            printf(", %d/%d, F(%X,%X)(%d), CTR:%d >\n", i, cl, rWord, rWord2, len, ctr);
#endif
    }

    if (!run) return 0;

    if (len <= 0) return 0;

    startpos = 12;
    //startpos = 0x36;
    wlen = len-startpos;

    write_vs1063_decoder(buf+startpos, wlen);

	return ret;
}

void write_vs1063_decoder(char *dec_buf, unsigned int dec_size)
{
    int wlen, rlen;
    unsigned int size = dec_size;
    int call_idx = 0;
	int write_timeout_occured = 0;
	static int continuous_write_error_count = 0;
	//volatile int delay;

    //printf("wlen = %d, size = %d\n", wlen, size);

#if 0
	wlen = VS1063_DECODER_LIMIT;

	if(size > 0) {
        rlen = write(fd_voip_call, &dec_buf[0], wlen);
	}
	//DBG_LOG("Size = %d, rlen = %d, wlen = %d\n", dec_size, rlen, wlen);
#else
    while(size > 0) {
    	if(size > VS1063_DECODER_LIMIT)
    	    wlen = VS1063_DECODER_LIMIT;
    	else
    	    wlen = size;

		//while(!vs10xx_is_ready(fd_voip_call)) {
		//	for (delay = 0; delay < SIMPLE_DELAY_CTR/4; delay++);
		//}

        rlen = write(fd_voip_call, &dec_buf[call_idx], wlen);
		//DBG_LOG("Size = %d, rlen = %d, wlen = %d\n", dec_size, rlen, wlen);
        if(rlen == wlen){
            call_idx += wlen;
            size -= wlen;

            if(size < VS1063_DECODER_LIMIT)
                wlen = size;

			continuous_write_error_count = 0;

            if (dec_size <= call_idx) {
                call_idx = 0;
                break;
            }
        } else {
			write_timeout_occured++;
			//for (delay = 0; delay < SIMPLE_DELAY_CTR/4; delay++);
			DBG_LOG("Size = %d, rlen = %d, wlen = %d\n", dec_size, rlen, wlen);

			continuous_write_error_count++;
			if(continuous_write_error_count > 10)
			{
				continuous_write_error_count = 0;
				printf("!!!! error : decording fail loop over 10 times. reset voip codec !!!!\n");
				cop_voip_call_codec_init();
				cop_voip_call_codec_start();
				usleep(10*1000);
			}
			return 0;
		}
    }
#endif
	check_avc_cycle_data();

	if (write_timeout_occured)
		set_indicate_codec_timeout_occured(1);
	else
		set_indicate_codec_timeout_occured(0);
}

int running_bcast_recv_and_dec(int patype, char *buf, int len, int run, int stop, int check_header)
{
    int startpos;
    int MPADataSize = 0;
    unsigned short rWord, rWord2;
    int i = 0, j, l, ret = 0, cl = 0;
    int wlen = 0;
    int header, seq, ssrc;
    int write_timeout_occured = 0;
    volatile int delay;
    int ctr = 0;

    int auto_pa_use = 0;
    int fire_pa_use = 0;
    int func_pa_use = 0;
    int cop_pa_use = 0;
    int occ_pa_use = 0;

//struct timespec time_old, time_new;
//unsigned long elapsed_time;

    if (fd_spk_dec < 0)
        return -1;

    switch (patype) {
        case MON_BCAST_TYPE_AUTO:
            auto_pa_use = 1;
			fire_pa_use = 1;
            func_pa_use = 1;
            cop_pa_use = 1;
            occ_pa_use = 1;
            break;

		case MON_BCAST_TYPE_FIRE:
			//auto_pa_use = 0;
			fire_pa_use = 1;
            func_pa_use = 1;
            cop_pa_use = 1;
            occ_pa_use = 1;
            break;

        case MON_BCAST_TYPE_FUNC:
            //auto_pa_use = 0;
			//fire_pa_use = 0;
            func_pa_use = 1;
            cop_pa_use = 1;
            occ_pa_use = 1;
            break;

        case MON_BCAST_TYPE_COP_PASSIVE:
            //auto_pa_use = 0;
			//fire_pa_use = 0;
            //func_pa_use = 0;
            cop_pa_use = 1;
            occ_pa_use = 1;
            break;

        case MON_BCAST_TYPE_OCC_PASSIVE:
            //auto_pa_use = 0;
			//fire_pa_use = 0;
            //func_pa_use = 0;
            //cop_pa_use = 0;
            occ_pa_use = 1;
            break;
		default:
			//printf("Unknown PA Type = %d\n", patype);
			break;
    }

	if (stop) {
	    printf("< PLAY STOP(%d), ", patype);
    	j = 0;
    	if (len > 0) {
    	    MPADataSize = RTPPacketParse(fd_spk_dec, buf, &startpos, len, patype, check_header);
        	if (MPADataSize > 0) {
	            l = MPADataSize;
    	        /*
        	    header  = buf[3];
            	header |= buf[2] << 8;
	            header |= buf[1] << 16;
    	        header |= buf[0] << 24;
        	    printf("%08X, ", header);
            	*/
	            header  = buf[startpos+3];
    	        header |= buf[startpos+2] << 8;
        	    header |= buf[startpos+1] << 16;
            	header |= buf[startpos+0] << 24;

	            seq = (buf[2] << 8) | buf[3];
	            ssrc  = buf[8] << 24;
	            ssrc |= buf[9] << 16;
	            ssrc |= buf[10] << 8;
	            ssrc |= buf[11];

	            printf("%08X %d, %08X ", ssrc, seq, header);
    	    } else l = 0;
    	} else l = 0;

#if 0
		if (!DeadmanAlarm_running && !FireAlarm_running) {
#else
		if (!DeadmanAlarm_running) {
#endif
			if (l > 0) {
			    cl = l >> 5;
			    if ((l - cl) > 0) cl++;

			    rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
			    rWord |= dVS10xx_SM_CANCEL;
			    vs10xx_set_reg(fd_spk_dec, dVS10xx_SCI_MODE, rWord);

			    for (i = 0; i < cl; ) {
			        if (l >= VS10XX_WRITE_SIZE) wlen = VS10XX_WRITE_SIZE;
			        else wlen = l;
		
			        ret = write(fd_spk_dec, &buf[startpos+j], wlen);

			        if (ret == wlen) {
			            rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_MODE, 0);
			            if ((rWord & dVS10xx_SM_CANCEL) == 0)
			                break;

			            l -= wlen;
			            j += wlen;
			            i++;
			        } else
			            for (delay = 0; delay < SIMPLE_DELAY_CTR/4; delay++);

		 	        try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, occ_pa_use, 0);
			        try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, cop_pa_use, 0);
    			    try_read_fire_monitoring_bcast_rx_data(&len, 0, fire_pa_use, 0);
   	 			    try_read_func_monitoring_bcast_rx_data(&len, 0, func_pa_use, 0);
    			    try_read_auto_monitoring_bcast_rx_data(&len, 0, auto_pa_use, 0);

			        ctr++;
			        if (ctr >= 200) {
			            printf("TIMEOUT, ");
			            break;
    			    }
			        //watchdog_gpio_toggle();
			        check_avc_cycle_data();
			    }
			    rWord = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT0, 0);
			    rWord2 = vs10xx_get_reg(fd_spk_dec, dVS10xx_SCI_HDAT1, 0);

			    if (!rWord && !rWord2)
			        printf(", %d/%d, K(%d), CTR:%d >\n", i, cl, len, ctr);
			    else
			        printf(", F(%X,%X)(%d), CTR:%d >\n", rWord, rWord2, len, ctr);
			} else
			    printf("\n");
		}

		return 0;
	}

	if (!run) return 0;

    if (len <= 0) return 0;

    MPADataSize = RTPPacketParse(fd_spk_dec, buf, &startpos, len, patype, check_header);

	//printf(" -- MPADATEsize: %d\n", MPADataSize);
#if 0
	{
		int i;
		printf("\n");
		for (i = 0; i < MPADataSize; i++) {
		    if ((i%16) == 0)
		        printf("\n");
		    printf("0x%02X, ", buf[i]);
		}
		printf("\n");
	}
#endif

	//printf("%02X %02X %02X %02X \n", buf[startpos+0], buf[startpos+1], buf[startpos+2], buf[startpos+3]);

	//printf("%02X %02X %02X %02X ", buf[4], buf[5], buf[6], buf[7]);
	//printf("%02X %02X %02X %02X\n", buf[8], buf[9], buf[10], buf[11]);

    //if (MPADataSize > RxDecFrameLen) {
		//printf("<<< DECORDING PAD:%d >>\n", MPADataSize - RxDecFrameLen);
        //MPADataSize = RxDecFrameLen;
    //}

#if 0
	if (!DeadmanAlarm_running && !FireAlarm_running) {
#else
	if (!DeadmanAlarm_running) {
#endif
	    if ((MPADataSize > 0)  && (MPADataSize <= (4096)))
    	{
			//printf(" == [PLAYDATA] SEQ: %d\n", buf[2]<<8 | buf[3]);

	        while (MPADataSize) {
	            if (MPADataSize > VS10XX_WRITE_SIZE)
    	            wlen = VS10XX_WRITE_SIZE;
        	    else
            	    wlen = MPADataSize;

	            ret = write(fd_spk_dec, buf+startpos, wlen);
    	        if (ret == wlen) {
        	        MPADataSize -= wlen;
					//printf("%s(), -- MPADataSize:%d\n", __func__, MPADataSize);
	                startpos += wlen;
    	        } else {
      	          write_timeout_occured++;
        	        for (delay = 0; delay < SIMPLE_DELAY_CTR/4; delay++);
            	}

            	try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, occ_pa_use, 0);
            	try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, cop_pa_use, 0);
            	try_read_fire_monitoring_bcast_rx_data(&len, 0, fire_pa_use, 0);
            	try_read_func_monitoring_bcast_rx_data(&len, 0, func_pa_use, 0);
            	try_read_auto_monitoring_bcast_rx_data(&len, 0, auto_pa_use, 0);
        	}
        	check_avc_cycle_data();
    	}
	} else {
        try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, occ_pa_use, 0);
        try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, cop_pa_use, 0);
        try_read_fire_monitoring_bcast_rx_data(&len, 0, fire_pa_use, 0);
        try_read_func_monitoring_bcast_rx_data(&len, 0, func_pa_use, 0);
        try_read_auto_monitoring_bcast_rx_data(&len, 0, auto_pa_use, 0);
	}

    if (write_timeout_occured) set_indicate_codec_timeout_occured(1);
    else set_indicate_codec_timeout_occured(0);

    return MPADataSize;
}

static int read_codec_setup(void)
{
    int fd, i, rlen;
    char buf[64], ch;
    int val;
    char *s;

    vs1063_enc_AEC_enable = 1;
    vs1063_enc_ADC_MODE = 2; /* 0 ~ 4 */
    vs1063_enc_LINE1_enable = 1;
    vs1063_cop_pa_enc_GAIN_val = 1024;
    vs1063_call_enc_GAIN_val = 1024;
    vs1063_mon_enc_GAIN_val = 1024;

    vs1063_cop_pa_enc_freq = 48000;
    vs1063_call_enc_freq = 48000;
    vs1063_mon_enc_freq = 48000;

    vs1063_cop_pa_enc_bitrate = 32;
    vs1063_call_enc_bitrate = 32;
    vs1063_mon_enc_bitrate = 32;

    __COP_PA_AND_CALL_ENCODING_LIMIT = 48;
    __CALL_ENCODING_LIMIT = 48;
    __MON_ENCODING_LIMIT = 48;

/* <<<< VOIP */
    VS1063_ENCODER_LIMIT = 160; 
    VS1063_DECODER_LIMIT = 160; 
	VOIP_ENCODING_LIMIT = VS1063_ENCODER_LIMIT / 2;

    VS1063_SCI_AEC_ENABLE = 1;           //AICTRL3 14bit
    VS1063_SCI_CLOCKF = 0xc800;
    VS1063_SCI_VOL = 0; 
    VS1063_SCI_AICTRL0 = 8000;           //samplerate
    VS1063_SCI_AICTRL1 = 0x400;          //encoding gain
    VS1063_SCI_AICTRL2 = 0x400;          //maximum autogain
    VS1063_SCI_AICTRL3 = 0xC032;         //type 0xc032(Alaw)

    VS1063_SCI_MODE = 0x5800;
    VS1063_SCI_ADAPT_ADDR = 0x1e2d;      //encoding AECAdaptMultiplier
    VS1063_SCI_ADAPT_VAL = 0x2; 
    VS1063_SCI_PATCH = 0x50;             //patched package
    VS1063_SCI_MIC_GAIN_ADDR = 0x2022;   //AEC mic gain 
    VS1063_SCI_MIC_GAIN_VAL = 0x400;
    VS1063_SCI_SPK_GAIN_ADDR = 0x2023;   //AEC spk gain
    VS1063_SCI_SPK_GAIN_VAL = 0x400;
    VS1063_SCI_AEC_PASS_ADDR = 0x2024;   //AEC pass through
    VS1063_SCI_AEC_PASS_VAL = 0;         //pass use:0

    VS1063_SCI_COEFFICIENTS = 0x1000;    //Adaptive Coefficients for AEC 
/* VOIP >>>> */

    fd = open(CODEC_CONFIG_SETUP_FILE, O_RDONLY);
    if (fd <= 0) {
        printf("CODEC CTRL SETUP FILE OPEN ERROR: %s\n", CODEC_CONFIG_SETUP_FILE);

    }
    else {
        i = 0;
        while (1) {
            rlen = read(fd, &ch, 1);
            //printf("len: %d\n", rlen);
            if (rlen <= 0)
                break;
            //printf("%02X ", ch);
            if (ch == 0x0A || ch == 0x0D) {
                buf[i] = 0;
                i = 0;


                if (strncmp(buf, "ENC_AEC_ENABLE", 14) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val == 0 || val == 1)
                            vs1063_enc_AEC_enable = val;
                    }
                }
                else if (strncmp(buf, "ENC_ADC_MODE", 12) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val >= 0 && val <= 4)
                            vs1063_enc_ADC_MODE = val; /* 0 ~ 4 */
                    }
                }
                else if (strncmp(buf, "ENC_LINE1_ENABLE", 16) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val == 0 || val == 1)
                            vs1063_enc_LINE1_enable = val;
                    }
                }
                else if (strncmp(buf, "COP_PA_AND_CALL_ENC_GAIN_VALUE", 30) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_cop_pa_enc_GAIN_val = val;
                    }
                }
                else if (strncmp(buf, "CALL_ENC_GAIN_VALUE", 19) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_call_enc_GAIN_val = val;
                    }
                }
                else if (strncmp(buf, "MON_ENC_GAIN_VALUE", 18) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_mon_enc_GAIN_val = val;
                    }
                }
                else if (strncmp(buf, "COP_PA_AND_CALL_ENC_FREQ", 24) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_cop_pa_enc_freq = val;
                    }
                }
                else if (strncmp(buf, "CALL_ENC_FREQ", 13) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_call_enc_freq = val;
                    }
                }
                else if (strncmp(buf, "MON_ENC_FREQ", 12) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_mon_enc_freq = val;
                    }
                }
                else if (strncmp(buf, "COP_PA_AND_CALL_ENC_BITRATE", 27) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_cop_pa_enc_bitrate = val;
                    }
                }
                else if (strncmp(buf, "CALL_ENC_BITRATE", 16) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_call_enc_bitrate = val;
                    }
                }
                else if (strncmp(buf, "MON_ENC_BITRATE", 15) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        vs1063_mon_enc_bitrate = val;
                    }
                }
                else if (strncmp(buf, "COP_PA_AND_CALL_ENC_LIMIT_CTR", 29) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        __COP_PA_AND_CALL_ENCODING_LIMIT = val;
                    }
                }
                else if (strncmp(buf, "CALL_ENC_LIMIT_CTR", 18) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        __CALL_ENCODING_LIMIT = val;
                    }
                }
                else if (strncmp(buf, "MON_ENC_LIMIT_CTR", 17) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        __MON_ENCODING_LIMIT = val;
                    }
                }
/* <<<< VOIP */
				else if (strncmp(buf, "VS1063_ENCODER_LIMIT", 20) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_ENCODER_LIMIT = val;
						VOIP_ENCODING_LIMIT = VS1063_ENCODER_LIMIT / 2;
					}
				}
				else if (strncmp(buf, "VS1063_DECODER_LIMIT", 20) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_DECODER_LIMIT = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_AEC_ENABLE", 21) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_AEC_ENABLE = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_CLOCKF", 17) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_CLOCKF = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_VOL", 14) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_VOL = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_AICTRL0", 18) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_AICTRL0 = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_AICTRL1", 18) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_AICTRL1 = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_AICTRL2", 18) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_AICTRL2 = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_AICTRL3", 18) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_AICTRL3 = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_MODE", 15) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_MODE = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_ADAPT_ADDR", 21) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_ADAPT_ADDR = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_ADAPT_VAL", 20) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_ADAPT_VAL = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_PATCH", 16) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_PATCH = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_MIC_GAIN_ADDR", 24) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_MIC_GAIN_ADDR = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_MIC_GAIN_VAL", 23) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_MIC_GAIN_VAL= val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_SPK_GAIN_ADDR", 24) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_SPK_GAIN_ADDR = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_SPK_GAIN_VAL", 23) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_SPK_GAIN_VAL= val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_AEC_PASS_ADDR", 24) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_AEC_PASS_ADDR = val;
					}
				}
				else if (strncmp(buf, "VS1063_SCI_AEC_PASS_VAL", 23) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						VS1063_SCI_AEC_PASS_VAL= val;
					}
				}
/* VOIP >>>> */
            }
            else {
                buf[i++] = ch;
            }
        }
        close (fd);
    }

    return 0;
}

