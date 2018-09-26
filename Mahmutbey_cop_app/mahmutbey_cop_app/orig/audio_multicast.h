#ifndef __AUDIO_MULTICAST_H__
#define __AUDIO_MULTICAST_H__

#define CHECK_RTP_START_SEQ_ONE

#define USE_CONTINUE_COP_PA_ENCORDING
#define USE_CONTINUE_CALL_ENCORDING
#undef USE_CONTINUE_MON_ENCORDING

#define B2L_SWAP16(s)	((((s)&0xff)<<8)|(((s)>>8)&0xff))
#define B2L_SWAP32(i)	((((i)&0xff000000)>>24)|\
			(((i)&0x00ff0000)>>8)|\
			(((i)&0x0000ff00)<<8)|\
			(((i)&0x000000ff)<<24))

////////////////////////////// rtp packet /////////////////////////
typedef enum
{
RTP_PT_PCMU, 			//	0	PCMU	Audio	8000	1	RFC 3551
RTP_PT_1016, 			//	1	1016	Audio	8000	1	RFC 3551
RTP_PT_G721, 			//	2	G721	Audio	8000	1	RFC 3551
RTP_PT_GSM, 			//	3	GSM		Audio	8000	1	RFC 3551
RTP_PT_G723, 			//	4	G723	Audio	8000	1	 
RTP_PT_DVI4_8K,		//	5	DVI4	Audio	8000	1	RFC 3551
RTP_PT_DVI4_16K, 		//	6	DVI4	Audio	16000	1	RFC 3551
RTP_PT_LPC, 			//	7	LPC		Audio	8000	1	RFC 3551
RTP_PT_PCMA, 			//	8	PCMA	Audio	8000	1	RFC 3551
RTP_PT_G722, 			//	9	G722	Audio	8000	1	RFC 3551
RTP_PT_L16_2CH,		//	10	L16		Audio	44100	2	RFC 3551
RTP_PT_L16_1CH, 		//	11	L16		Audio	44100	1	RFC 3551
RTP_PT_QCELP, 			//	12	QCELP	Audio	8000	1	 
RTP_PT_CN, 			//	13	CN		Audio	8000	1	RFC 3389
RTP_PT_MPA, 			//	14	MPA		Audio	90000		RFC 2250, RFC 3551
RTP_PT_G728, 			//	15	G728	Audio	8000	1	RFC 3551
RTP_PT_DVI4_11025, 	//	16	DVI4	Audio	11025	1	 
RTP_PT_DVI4_22050,	//	17	DVI4	Audio	22050	1	 
RTP_PT_G729, 			//	18	G729	Audio	8000	1	 
RTP_PT_CellB = 25,		//	25	CellB	Video	90000	 	RFC 2029
RTP_PT_JPEG, 			//	26	JPEG	Video	90000	 	RFC 2435
RTP_PT_nv = 28,		//	28	nv		Video	90000	 	RFC 3551
RTP_PT_H261 = 31,		//	31	H261	Video	90000	 	RFC 2032
RTP_PT_MPV, 			//	32	MPV		Video	90000	 	RFC 2250
RTP_PT_MP2T, 			//	33	MP2T	Audio/Video	90000	 	RFC 2250
RTP_PT_H263 			//	34	H263

}enPayloadType_t;

typedef struct
{
	char CC 		:4;		// CSRC count
	char X 			:1;		// EXTENSION
	char P 			:1;		// PADDING
	char V 			:2;		// VERSION
}__attribute__((packed)) RTP_Header_info1_t;

typedef struct
{
	char PT 		:7;		// PAYLOAD TYPE
	char M 			:1;		// MARKER
}__attribute__((packed)) RTP_Header_info2_t;

typedef struct {
    RTP_Header_info1_t h_info1; // 1 byte
    RTP_Header_info2_t h_info2; // 1 byte
    unsigned short seq;		// 2 byte
    unsigned int   ts;		// 4 byte
    unsigned int   SSRC;	// 4 byte
}__attribute__((packed)) RTP_Header_t;

typedef struct {
    unsigned short MBZ;
    unsigned short FragOffset;
}__attribute__((packed)) MPEG_AudioSpec_Header_t;

typedef struct {
    RTP_Header_t rtpHeader;
    MPEG_AudioSpec_Header_t MPAHeader;
    unsigned short Palyload[512];
} __attribute__((packed)) RTP_Packet_t;

typedef struct {
	RTP_Header_t rtpHeader;
	unsigned short Palyload[512];
} __attribute__((packed)) RTP_VoIP_Packet_t;
/*************************************************************/

#define COP_PASSIVE_BCAST_TX_MULTI_ADDR	"234.1.3.3"
#define OCC_PASSIVE_BCAST_TX_MULTI_ADDR	"234.1.3.4"
#define COP_MULTI_PORT_NUMBER 5500

void init_cop_multicast(void);
void clear_rx_tx_mon_multi_addr(void);
unsigned int  __get_rx_multi_addr(void);
unsigned int  __get_tx_multi_addr(void);
unsigned int  __get_mon_multi_addr(void);

int connect_cop_passive_bcast_tx(char *multitxaddr);
int connect_occ_passive_bcast_tx(void);
int close_cop_multi_tx(void);
int send_cop_multi_tx_data(char *buf, int len, int dd);
int send_cop_multi_tx_data_passive_on_call(char *buf, int len);

int is_open_multi_mon_sock(void);
int send_cop_multi_mon_data(char *buf, int len);
int send_call_multi_tx_data(char *buf, int len);
int send_occ_pa_multi_tx_data(char *buf, int len);
int close_call_and_occ_pa_multi_tx(void);

char *try_read_call_rx_data(int *len, int stop, int use);

int connect_cop_multi_monitoring(char *buf, int len, int *get_mon_addr, int force_set_addr);
void close_cop_multi_monitoring(void);
char *try_read_call_monitoring_data(int *len, int stop, int use);

int connect_cop_cab2cab_call_rx_multicast(char *buf, int len);
int connect_cop_cab2cab_call_multicast(char *buf, int len);

void close_cop_multi_rx_sock(void);
int connect_cop_passive_bcast_tx_while_othercall(void);

int close_cop_multi_passive_tx(void);

#define MON_BCAST_TYPE_AUTO			1
#define MON_BCAST_TYPE_FUNC			2
#define MON_BCAST_TYPE_COP_PASSIVE	3
#define MON_BCAST_TYPE_OCC_PASSIVE	4
#define MON_BCAST_TYPE_FIRE			5
#define MON_BCAST_TYPE_MAX			5

//int try_read_monitoring_bcast_rx_data(char *buf, int maxlen);

char *try_read_occ_pa_monitoring_bcast_rx_data(int *len, int stop, int use, int play);
int get_next_occ_pa_monitoring_data_ssrc(void);
unsigned short get_next_occ_pa_monitoring_data_seq(void);

char *try_read_cop_pa_monitoring_bcast_rx_data(int *len, int stop, int use, int play);
int get_next_cop_pa_monitoring_data_ssrc(void);
unsigned short get_next_cop_pa_monitoring_data_seq(void);

char *try_read_func_monitoring_bcast_rx_data(int *len, int stop, int use, int play);
int get_next_func_monitoring_data_ssrc(void);
unsigned short get_next_func_monitoring_data_seq(void);

char *try_read_fire_monitoring_bcast_rx_data(int *len, int stop, int use, int play);
int get_next_fire_monitoring_data_ssrc(void);
unsigned short get_next_fire_monitoring_data_seq(void);

char *try_read_auto_monitoring_bcast_rx_data(int *len, int stop, int use, int play);
int get_next_auto_monitoring_data_ssrc(void);
unsigned short get_next_auto_monitoring_data_seq(void);

int get_mon_multi_rx_ctr(void);
int get_call_multi_rx_ctr(void);
int get_occ_pa_multi_rx_ctr(void);
int get_cop_pa_multi_rx_ctr(void);
int get_func_multi_rx_ctr(void);
int get_fire_multi_rx_ctr(void);
int get_auto_multi_rx_ctr(void);

void reset_call_multi_rx_data_ptr(void);
void reset_mon_multi_rx_data_ptr(void);
void reset_occ_pa_multi_rx_data_ptr(void);
void reset_cop_pa_multi_rx_data_ptr(void);
void reset_fire_multi_rx_data_ptr(void);
void reset_auto_multi_rx_data_ptr(void);
void reset_func_multi_rx_data_ptr(void);

void reset_auto_multi_rx_start_get_buf(void);
void reset_fire_multi_rx_start_get_buf(void);
void reset_func_multi_rx_start_get_buf(void);
void reset_cop_pa_multi_rx_start_get_buf(void);
void reset_occ_pa_multi_rx_start_get_buf(void);


void init_auto_fire_func_monitoring_bcast(void);
void close_auto_fire_func_monitoring_bcast(void);
int connect_monitoring_bcast_multicast(int mon_type);
int close_monitoring_bcast_multicast(int mon_type);
void temporary_mon_enc_connect_to_passive_pa_tx(int onoff, unsigned int last_mon_addr);

#endif // __AUDIO_MULTICAST_H__

