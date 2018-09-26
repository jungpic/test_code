#ifndef __COP_KEY_PROCESS_EXTERN_H__
#define __COP_KEY_PROCESS_EXTERN_H__

#if 0
/************* COP to AVC Packet ********************/
typedef struct {
    char start	:1;
    char stop	:1;
    char reserved :6;
} __attribute__((packed)) COP2AVC_ReqFlag_t;

typedef struct {
    char start	:1;
    char stop	:1;
    char reserved :4;
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
    char funcNum;
    COP2AVC_ReqFlag_t FuncBCast;
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

typedef struct {
    char length;	/* 33 */
    char id;		/* 4 */
    COP2AVC_BCastReqSet_t BCastReq;
    COP2AVC_CallReqSet_t CallReq;
    char reserved;
    unsigned short status_code;						/* Fire flag */
    char multi_addr_valid; /* valid: 0x10 */
    unsigned int RxMultiAddr;
    unsigned int TxMultiAddr;
    unsigned int MonMultiAddr;
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
} COP_Cur_Status_t;

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
    char funcNum;
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

typedef struct {
    char length;	/* 33 */
    char id;		/* 5 */
    AVC2COP_BCastReqSet_t BCastReq;
    AVC2COP_CallReqSet_t CallReq;
    unsigned int RxMultiAddr;
    unsigned int TxMultiAddr;
    unsigned int MonMultiAddr;
    char reboot;
    char indoor_vol;
    char outdoor_vol;
    char fire_deadman;
    unsigned short crc16;
} __attribute__((packed)) packet_AVC2COP_t;
/******************************************************************************/
/******************************************************************************/
#endif
#if 0
/************* AVC to CTM(COP)Packet ********************/
typedef struct {
    char AVC	:1;
    char CTM	:1;
    char COP	:1;
    char FDI    :1;
    char PIB_1	:1;
    char PIB_2	:1;
    char SDI_1	:1;
    char SDI_2  :1;	
} __attribute__((packed)) SubSystem_StatusFlag1_t;
typedef struct {
    char PEI_1	 :1;
    char PEI_2	 :1;
    char PEI_3	 :1;
    char PEI_4   :1;
    char PAMP_1	 :1;
    char PAMP_2	 :1;
    char reserved:2;    	
} __attribute__((packed)) SubSystem_StatusFlag2_t;
typedef struct {
    char LRM_1	:1;
    char LRM_2	:1;
    char LRM_3	:1;
    char LRM_4  :1;
    char LRM_5	:1;
    char LRM_6	:1;
    char LRM_7	:1;
    char LRM_8  :1;	
} __attribute__((packed)) SubSystem_StatusFlag3_t;
typedef struct {
    char FCAM	 :1; 
    char OCAM_1	 :1;
    char OCAM_2	 :1;
    char SCAM_1  :1;
    char SCAM_2	 :1;
    char SCAM_3	 :1;
    char SCAM_4  :1;    	
    char reserved:1;    	
} __attribute__((packed)) SubSystem_StatusFlag4_t;

typedef struct {
    char PID_1	:1;
    char PID_2	:1;
    char PID_3	:1;
    char PID_4	:1;
    char PID_5	:1;
    char PID_6	:1;
    char PID_7	:1;
    char PID_8	:1;
} __attribute__((packed)) SubSystem_StatusFlag5_t;
typedef struct {
    char PID_9	 :1;
    char PID_10	 :1;
    char PID_11	 :1;
    char PID_12	 :1;
    char reserved:4;    	
} __attribute__((packed)) SubSystem_StatusFlag6_t;

typedef struct {
    SubSystem_StatusFlag1_t MC_status1;
    SubSystem_StatusFlag2_t MC_status2;
    SubSystem_StatusFlag3_t MC_status3;
    SubSystem_StatusFlag4_t MC_status4;
    SubSystem_StatusFlag5_t MC_status5;
    SubSystem_StatusFlag6_t MC_status6;
} __attribute__((packed)) SubSystem_Status_t;


#define AVC2CTM_PACKET_SIZE	53
#define AVC2CTM_PACKET_ID	0x11
typedef struct {
    char length;	/* 35h */
    char id;		/* 11h */
    SubSystem_Status_t Single_MC2_status;
    SubSystem_Status_t Single_M_status;
    SubSystem_Status_t Single_T_status;
    SubSystem_Status_t Single_MC1_status;
    SubSystem_Status_t Dual_MC2_status;
    SubSystem_Status_t Dual_M_status;
    SubSystem_Status_t Dual_T_status;
    SubSystem_Status_t Dual_MC1_status;
    char spare;
    unsigned short crc16;
} __attribute__((packed)) packet_AVC2CTM_t;
/******************************************************************************/
#endif 

#if 0
struct call_info {
    char car_id;
    char idx_id;
    short status;
    short emergency;
    short is_start_requested;
    int mon_addr;
    int tx_addr;
};

#define MAX_PEI_CALL_NUM    (27*3)
struct multi_call_status {
    int ctr;
    int in_ptr;
    int out_ptr;
    struct call_info info[MAX_PEI_CALL_NUM];
};
#endif 
#define FUNC_IN_OFFSET              0
#define FUNC_OUT_OFFSET             1
#define FUNC_LRM_OFFSET             2
#define FUNC_PIB_OFFSET             3

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

#define BUSY_LOG_START_IDX      LOG_CAB_TO_CAB_WAIT
#define BUSY_LOG_END_IDX        (LOG_MSG_MAX-1)

#endif // __COP_KEY_PROCESS_EXTERN_H__
