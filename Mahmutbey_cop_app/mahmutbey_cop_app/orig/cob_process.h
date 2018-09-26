#ifndef __COP_KEY_PROCESS_H__
#define __COP_KEY_PROCESS_H__

#define FIRE_ALARM_TIMER			0

#define LOG_MESSAGE_DISPLAY_TIME	2	// Second

#undef __DEBUG_DISCARD_STATUS__

#define INOUT_LED_ONOFF_BY_AVC
#undef INOUT_LED_ONOFF_BY_COP

#define MAX_PEI_CALL_JOIN			2

#define BCAST_CALL_HEAD_NEW 		1

#define BMENU_STATION_START			1
#define BMENU_STATION_END      		40

#define COP_MENU_BROADCAST			1
#define COP_MENU_AD_BROADCAST		2
#define COP_MENU_START_BROADCAST	3
#define COP_MENU_STATION_BROADCAST	4
#define COP_MENU_CALL				5
#define COP_MENU_COP_VOL			6
#define COP_MENU_PAMP_VOL			7

#define COP_MENU_MASK				0x07

#define COP_MENU_HEAD_SELECT		(1<<7)

#define FUNC_IN_OFFSET				0
#define FUNC_OUT_OFFSET				1
#define FUNC_LRM_OFFSET				2
#define FUNC_PIB_OFFSET				3

#define LOG_NOT_DEFINED         0
#define LOG_READY               1
#define LOG_IP_SETUP_ERR        2
#define LOG_NOT_READY           3
#define LOG_AVC_NOT_CONNECT     3
#define LOG_UNKNOWN_ERR         4
#define LOG_CAB_ID_ERR          5
#define LOG_CAB_TO_CAB_WAIT     6
#define LOG_CAB_REJECT_WAIT     7
#define LOG_BUSY                8
#define LOG_OCC_PA_BUSY         9
#define LOG_CAB_PA_BUSY         10
#define LOG_CAB_TO_CAB_BUSY     11
#define LOG_PEI_BUSY            12
#define LOG_PEI_TO_CAB_BUSY     13
#define LOG_EMER_PEI_BUSY       14
#define LOG_PEI_TO_OCC_BUSY     15
#define LOG_FUNC_BUSY           16
#define LOG_FUNC_REJECT			17
#define LOG_ROUTE_NG			18
#define LOG_MSG_MAX				19

#define BUSY_LOG_START_IDX		LOG_CAB_TO_CAB_WAIT
#define BUSY_LOG_END_IDX		(LOG_MSG_MAX-1)

/************* COP to AVC Packet ********************/
typedef struct {
    char start	:1;
    char stop	:1;
    char reserved :6;
} __attribute__((packed)) COP2AVC_ReqFlag_t;

typedef struct {
	char start  :1; 
	char stop   :1; 
	char indoor :1;
	char outdoor:1;
	char lrm	:1;
	char pib	:1;
	char is_station_fpa:1;
	char broadcast_stop:1;
} __attribute__((packed)) COP2AVC_FuncReqFlag_t;

typedef struct {
    char start	:1;
    char stop	:1;
	char join	:1;
    char reserved :3;
    char same_id :1;
    char emergency_en :1;
} __attribute__((packed)) COP2AVC_PEIReqFlag_t;

typedef struct {
    char start	:1;
    char stop	:1;
    char accept	:1;
    char reject	:1;
    char reserved :2;
	char tractor_start:1;
	char tractor_stop:1;
} __attribute__((packed)) COP2AVC_ReqAcceptFlag_t;

typedef struct {
/*
    char start	:1;
    char stop	:1;
    char pei_occ_started	:1;
    char reserved :5;
*/
    char can_talk :1;
    char cannot_talk :1;
    char can_talk_and_receive :1;
    char reserved :5;
} __attribute__((packed)) COP2AVC_OCCCallFlag_t;

typedef struct {
    char active;
    //char funcNum;
    unsigned short funcNum;
    COP2AVC_FuncReqFlag_t FuncBCast;
    char in_out_flag; /* 1: All button off */
                      /* 2: in/out button on */
                      /* 3: in button on */
                      /* 2: out button on */
    COP2AVC_ReqFlag_t IndoorBCast;
    COP2AVC_ReqFlag_t OutdoorBCast;
    COP2AVC_ReqFlag_t OCCPABroadCast;
} __attribute__((packed)) COP2AVC_BCastReqSet_t;

typedef struct {
    char active;
    COP2AVC_ReqAcceptFlag_t cab_call;
    COP2AVC_PEIReqFlag_t pei_call;
    COP2AVC_OCCCallFlag_t occ_call;
    char car_number;
    char dev_number;
} __attribute__((packed)) COP2AVC_CallReqSet_t;

typedef struct {
	char in_vol_chnage:1;
	char out_vol_change:1;
	char reserved:6;
} __attribute__((packed)) COP2AVC_ReqInOutVolChangeFlag_t;

#define COP2AVC_PACKET_SIZE	37
#define COP2AVC_PACKET_ID	0x4
#define COP2AVC_MULTICAST_ADDR_VALID	0x10
typedef struct {
    char length;	/* 37 */
    char id;		/* 4 */
    COP2AVC_BCastReqSet_t BCastReq;
    COP2AVC_CallReqSet_t CallReq;
    char reserved;
#if 0
    unsigned short status_code;						/* Fire flag */
    char multi_addr_valid; /* valid: 0x10 */
    unsigned int RxMultiAddr;
    unsigned int TxMultiAddr;
    unsigned int MonMultiAddr;
#else
	char UiFlag;
	char departure_id;
	char destination_id;
	char di_display_id;
	unsigned short route_pattern;
	char spare[9];
#endif

	COP2AVC_ReqInOutVolChangeFlag_t vol;
	char indoor_vol;
	char outdoor_vol;
    unsigned short crc16;
} __attribute__((packed)) packet_COP2AVC_t;
/****************************************************/

typedef enum {
    COP_STATUS_IDLE = -1,
    FUNC_BCAST_START = 0x00,
    FUNC_BCAST_STOP = 0x01,
    __PASSIVE_INDOOR_BCAST_START = 0x02,
    __PASSIVE_INDOOR_BCAST_STOP = 0x03,
    __PASSIVE_OUTDOOR_BCAST_START = 0x04,
    __PASSIVE_OUTDOOR_BCAST_STOP = 0x05,
    __PASSIVE_INOUTDOOR_BCAST_START = 0x06,
    PASSIVE_INOUTDOOR_BCAST_STOP = 0x07,
    __PEI2CAB_CALL_START = 0x08,
    __PEI2CAB_CALL_STOP = 0x09,
    __CAB2CAB_CALL_START = 0x10,
    __CAB2CAB_CALL_STOP = 0x11,
    OCC_PA_BCAST_START = 0x12,
    OCC_PA_BCAST_STOP = 0x13,
    OCC2PEI_CALL_START = 0x14,
    OCC2PEI_CALL_STOP = 0x15,

    OCCPA_BCAST_STATUS_START = 100,
    OCCPA_BCAST_STATUS_STOP,			// 101

    AUTO_BCAST_START,					// 102
    AUTO_BCAST_STOP,					// 103
	
	AUTO_FIRE_BCAST_START,				// 104
	AUTO_FIRE_BCAST_STOP,				// 105

    FIRE_BCAST_START,					// 106
    FIRE_BCAST_STOP,					// 107

    WAITING_FUNC_BCAST_START,			/* 108 */

    WAITING_PASSIVE_INDOOR_BCAST_START, /* 109 */
    WAITING_PASSIVE_INDOOR_BCAST_STOP, 	/* 110 */

    WAITING_PASSIVE_OUTDOOR_BCAST_START,// 111
    WAITING_PASSIVE_OUTDOOR_BCAST_STOP, /* 112 */

    WAITING_OUTSTART_PASSIVE_INOUTDOOR_BCAST_START,	// 113
    WAITING_INSTART_PASSIVE_INOUTDOOR_BCAST_START,	// 114

    _WAITING_OUTSTOP_PASSIVE_INOUTDOOR_BCAST_START, /* 115 */
    _WAITING_INSTOP_PASSIVE_INOUTDOOR_BCAST_START,	// 116

    __WAITING_CAB2CAB_CALL_START, 					/* 117 */

    __CAB2CAB_CALL_REJECT,							/* 118 */
    __CAB2CAB_CALL_WAKEUP,							/* 119 */
    __CAB2CAB_CALL_MONITORING_START, 				/* 120 */
    __CAB2CAB_CALL_MONITORING_STOP,					// 121

    WAITING_CAB2CAB_CALL_STOP,						// 122

    __PEI2CAB_CALL_WAKEUP,							/* 123 */
    __PEI2CAB_CALL_MONITORING_START, 				/* 124 */
    __PEI2CAB_CALL_MONITORING_STOP,					// 125
    PEI_CALL_REJECT,								// 126
    __PEI2CAB_CALL_EMER_EN,//__PEI2CAB_CALL_EMER_WAKEUP,	/* 127 */
    __NOT_USED_1__, //__PEI2CAB_CALL_EMER_START,			/* 128 */
    __NOT_USED_2__, //__PEI2CAB_CALL_EMER_STOP,				/* 129 */
    __NOT_USED_3__, // __PEI2CAB_CALL_EMER_MONITORING_START,/* 130 */
    __NOT_USED_4__, // __PEI2CAB_CALL_EMER_MONITORING_STOP,	// 131

    __WAITING_PEI2CAB_CALL_START,	/* 132 */

    WAITING_PEI2CAB_CALL_STOP,		// 133

    WAITING_OCC_PA_BCAST_START,		/* 134 */
    WAITING_OCC_PA_BCAST_STOP,		/* 135 */

#if 0
    __CAB2CAB_CALL_EMER_EN,
#endif

    IS_OCC_CMD,

    GET_TRAIN_NUM,

    IS_DEADMAN_CMD,
    GET_DEADMAN_CMD,

    IS_ONLY_LED_FLAG,

    ONLY_PAMP_VOLUME,

    ENABLE_GLOBAL_START_FUNC_AUTO,
    DISABLE_GLOBAL_START_FUNC_AUTO,
#if 0
	IS_FIRE_ALARM_CMD,
	GET_FIRE_ALARM_CMD,
#endif
	TRACTOR_POSSIBLE_CMD,
	TRACTOR_UNPOSSIBLE_CMD,
	HELP_CAB2CAB_START,
    __PEI2CAB_CALL_OCC,	
	FUNC_BCAST_REJECT,
	UI_SET_ROUTE_CMD,
	UI_SET_SPECIAL_ROUTE_CMD,
	UI_DI_DISPLAY_CMD,
} COP_Cur_Status_t;

int cop_process_init(void);
int cop_bcast_status_update (enum __cop_key_enum keyval, char *buf, int rxlen,
            int *is_reboot, int tcp_connected, int ip_id_nok);
void cop_status_reset(void);
void send_call_protocol_err_to_avc(char *cmdbuf);

#define INOUT_BUTTON_STATUS_ALLOFF	1
#define INOUT_BUTTON_STATUS_INOUT_ON	2
#define INOUT_BUTTON_STATUS_IN_ON	3
#define INOUT_BUTTON_STATUS_OUT_ON	4

#define INOUT_BUTTON_STATUS_ALLOFF_MASK		(1<<1)
#define INOUT_BUTTON_STATUS_INOUT_ON_MASK	(1<<2)
#define INOUT_BUTTON_STATUS_IN_ON_MASK		(1<<3)
#define INOUT_BUTTON_STATUS_OUT_ON_MASK		(1<<4)
/******************************************************************************/

/******************************************************************************/
/************* AVC to COP Packet ********************/
typedef struct {
    char start	:1;
    char reject	:1;
    char stop	:1;
    char reserved :5;
} __attribute__((packed)) AVC2COP_ReqAcceptFlag_t;

typedef struct {
    char func_start	:1;
    char func_reject	:1;
    char func_stop	:1;
    char auto_start	:1;
    char auto_stop	:1;
    char occpa_start	:1;
    char occpa_stop	:1;
    char fire_bcast :1;
} __attribute__((packed)) AVC2COP_AutoFunc_ReqAcceptFlag_t;

typedef struct {
    char active;
    unsigned short funcNum;
    AVC2COP_AutoFunc_ReqAcceptFlag_t Auto_Func_BCast;
    char reset;
    AVC2COP_ReqAcceptFlag_t IndoorBCast;
    AVC2COP_ReqAcceptFlag_t OutdoorBCast;
    AVC2COP_ReqAcceptFlag_t OCCPABCast;
} __attribute__((packed)) AVC2COP_BCastReqSet_t;

typedef struct {
    char wake_up	:1;
    char start		:1;
    char stop		:1;
    char reject		:1;
    char mon_start	:1;
    char mon_stop	:1;
    char standby	:1;
    char emergency_en	:1;
} __attribute__((packed)) AVC2COP_OCCCallFlag_t;

typedef struct {
    char active;
    AVC2COP_OCCCallFlag_t cab_call;
    AVC2COP_OCCCallFlag_t pei_call;
    AVC2COP_OCCCallFlag_t occ_call;
    unsigned char car_number;
    unsigned char dev_number;
} __attribute__((packed)) AVC2COP_CallReqSet_t;

#define AVC2COP_PACKET_SIZE	55
#define AVC2COP_PACKET_ID	0x5
typedef struct {
    char length;	/* 45 */
    char id;		/* 5 */
    AVC2COP_BCastReqSet_t BCastReq;
    AVC2COP_CallReqSet_t CallReq;
	unsigned int COPAddr;
	unsigned int PEIAddr;
    unsigned int RxMultiAddr;
    unsigned int TxMultiAddr;
    unsigned int MonMultiAddr;
    //char reboot;
	char train_num;
    char indoor_vol;
    char outdoor_vol;
	char UiFlag;
	char departure_id;
	char destinateion_id;
	char di_display_id;
	unsigned short route_pattern;
	char route_pattern_flag;
	char spare[7];
    unsigned short crc16;
} __attribute__((packed)) packet_AVC2COP_t;
/******************************************************************************/

#define CAB_LIST	(1<<4)
#define PEI_LIST	(1<<5)

#define MAX_CAR_NUM			8
#define MAX_CAB_ID_NUM		4	//1
#define MAX_PEI_ID_NUM		4	//3

#define CALL_STATUS_WAKEUP	(1<<0)
#define CALL_STATUS_EMERGENCY	(1<<1)
#define CALL_STATUS_LIVE	(1<<2)
#define CALL_STATUS_MON		(1<<3)
#define CALL_STATUS_MON_WAIT	(1<<4)

#define MAX_CAB_CALL_NUM	4
struct call_info {
    char car_id;
    char idx_id;
    short status;
    short emergency;
    short is_start_requested;
    int mon_addr;
    int tx_addr;
	int train_num;
};

#define MAX_PEI_CALL_NUM	(16*2)
struct multi_call_status {
    int ctr;
    int in_ptr;
    int out_ptr;
    struct call_info info[MAX_PEI_CALL_NUM];
};

void call_list_init(void);

void set_indicate_avc_cycle_data(void);
void set_indicate_codec_timeout_occured(int v);

/* <<<< VOIP */
//static void voip_call_start_prepare(void);
void pei2cab_call_rx_tx_running(int passive_on);
/* VOIP >>>> */

int get_func_is_now_start(void);

#endif // __COP_KEY_PROCESS_H__
