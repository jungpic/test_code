#ifndef __AVC_TCP_H__
#define __AVC_TCP_H__

#define ALL_AVC_TCP_CMD_ERROR_ID	2

#define AVC_TCP_PORT	5500
#define AVC_TCP_CON_TIMEOUT	2

#define AVC_TCP_CMD_SW_UPDATE_ID	0x13
#define AVC_TCP_CMD_SW_UPDATE_VER_REQ	0x01
#define AVC_TCP_CMD_SW_UPDATE_ICS_VER_REQ 0x81
#define AVC_TCP_CMD_SW_UPDATE_START	0x02
#define AVC_TCP_CMD_SW_UPDATE_FILENAME_OFF	4
#define AVC_TCP_CMD_SW_UPDATE_MAX_FILELEN	64

#define AVC_TCP_CMD_SW_UPDATE_REPORT_ID	0x14

#define AVC_TCP_CMD_COP_CTRL_ID	0x05
#define AVC_TCP_CMD_COP_CTRL_LEN	33

#define AVC_TCP_CMD_COP_STATUS_ID	0x11  //add ctm cmd
#define AVC_TCP_CMD_SUB_SW_UPDATE_ID	0x30  //add ctm cmd

#define SW_UPDATE_CMD_START   ((AVC_TCP_CMD_SW_UPDATE_ID<<8) | (AVC_TCP_CMD_SW_UPDATE_START))
#define SW_UPDATE_CMD_VER_REQ ((AVC_TCP_CMD_SW_UPDATE_ID<<8) | (AVC_TCP_CMD_SW_UPDATE_VER_REQ))
#define SW_UPDATE_CMD_ICS_VER_REQ ((AVC_TCP_CMD_SW_UPDATE_ID<<8) | (AVC_TCP_CMD_SW_UPDATE_ICS_VER_REQ))

#define UPDATE_RESULT_LOGIN_OK		(1<<0)
#define UPDATE_RESULT_LOGIN_FAIL	(1<<1)
#define UPDATE_RESULT_DOWN_OK		(1<<2)
#define UPDATE_RESULT_DOWN_FAIL		(1<<3)

#define UPDATE_RESULT_UPDATE_OK		(1<<0)
#define UPDATE_RESULT_UPDATE_FAIL	(1<<1)

#define IP_NETWORK_CLASS	((10 << 8) | (128))
#define IP_MASK_CLASS		((255 << 8) | (255))

#define IP_PRODUCT_SHIFT_BIT	(4)
#define IP_PRODUCT_MASK	(0x0F << IP_PRODUCT_SHIFT_BIT) // 2014.05.09 Ver0.73

#define COP_PRODUCT_CLASS	0x04


/******************************************************************************/
/************* AVC to CTM(COP) SW UPDATE Packet ********************/
typedef struct {
	char FtpOK  		:1; 
	char FtpError  		:1; 
	char fileDownOK 	:1;
	char fileDownError	:1;
	char reserved 		:4;
} __attribute__((packed)) AVC2CTM_FtpFlag_t;

typedef struct {
	char UpdateOK		:1; 
	char UpdateError	:1; 
	char rebooting	 	:1;
	char rebootComplete	:1;
	char UpdateResult	:4;
} __attribute__((packed)) AVC2CTM_UpdateFlag_t;

typedef struct {
	char AckSWFileReady			:1; 
	char AckSWFileCompelte		:1; 
	char spare1	 				:2;
	char AckSWversion_reqeust	:1;
	char AckSWUpdateStart		:1;	
	char spare2					:2;
} __attribute__((packed)) AVC2CTM_ACKFlag_t;

#define AVC2CTM_SWUP_PACKET_SIZE	18  //0x12
#define AVC2CTM_SWUP_PACKET_ID	0x30
typedef struct {
    char length;	/* 18(0x12h) */
    char id;		/* 30h */
	char sw_version[2];
	AVC2CTM_FtpFlag_t 		ftp_flag;
	AVC2CTM_UpdateFlag_t 	Update_flag;
	char processClass;
	char reserved;
	//unsigned int 			DeviceAddr;
	char DeviceAddr4;
	char DeviceAddr3;
	char DeviceAddr2;
	char DeviceAddr1;
	AVC2CTM_ACKFlag_t 		Ack_flag;
    char spare[3];    
    unsigned short crc16;
} __attribute__((packed)) packet_AVC2CTM_SW_UPDATE_t;

/******************************************************************************/
/************* CTM(COP) to AVC(COP) SW UPDATE Packet ********************/
typedef struct {
	char AckUpdateOK	:1; 
	char AckUpdateError	:1; 	
	char reserve		:6;
} __attribute__((packed)) CTM2AVC_UpdateAckFlag_t;

typedef struct {
	char FtpOK  		:1; 
	char FtpError  		:1; 
	char fileDownOK 	:1;
	char fileDownError	:1;
	char reserved 		:4;
} __attribute__((packed)) CTM2AVC_FtpAckFlag_t;

typedef struct {
	char SWFileReady		:1; 
	char SWFileCompelte		:1; 
	char spare1	 			:2;
	char SWversion_reqeust	:1;
	char SWUpdateStart		:1;	
	char spare2				:2;
} __attribute__((packed)) CTM2AVC_UPDATE_Flag_t;

//#define AVC2CTM_SWUP_PACKET_SIZE	81
//#define AVC2CTM_SWUP_PACKET_ID	0x31
#define CTM2AVC_SWUP_PACKET_SIZE	81
#define CTM2AVC_SWUP_PACKET_ID	0x31
typedef struct {
    char length;	/* 81 (0x51h) */
    char id;		/* 31h */
	char sw_version[2];	/*SW update version*/	
	CTM2AVC_UPDATE_Flag_t		SwUpdate_flag;
	char processClass;
	char reserved;
	//unsigned int 				DeviceAddr;
    char DeviceAddr4; 	 /*device id */
	char DeviceAddr3; 	 /*device id */
	char DeviceAddr2; 	 /*device id */
	char DeviceAddr1; 	 /*device id */	
	char UpFilename[64]; 
	CTM2AVC_FtpAckFlag_t		ftpAck_flag;
	CTM2AVC_UpdateAckFlag_t 	UpdateAck_flag;
    char spare[2];    
    unsigned short crc16;
} __attribute__((packed)) packet_CTM2AVC_SW_UPDATE_t;

/******************************************************************************/
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


#define AVC2CTM_PACKET_SIZE	54//53->52
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
/************* AVC to COP(MMI)EVENT Packet ********************/
typedef struct {
    char SingleTS           :1;
    char DualTS             :1;
    char DirectDirection 	:1;
    char ReverseDirection	:1;
    char reserved   		:3;
    char valid          	:1;
} __attribute__((packed)) AVC2CTM_DualFlag_t;
typedef struct {
    char reserved       :2;
    char logDownStart	:1;
    char reserved2      :4;
    char logDownValid   :1;
} __attribute__((packed)) AVC2CTM_LogFlag_t;

#define AVC2CTM_EVENT_PACKET_SIZE	53// 0x35
#define AVC2CTM_EVENT_PACKET_ID	0x10
typedef struct {
    char length;	/* 2D5h */
    char id;		/* 10h */
    char spare[9];
    AVC2CTM_LogFlag_t log_flag;
    char spare2[2];
    char ScheduleTime[7];
    AVC2CTM_DualFlag_t dual_info;
    char spare3[29];
    unsigned short crc16;
} __attribute__((packed)) packet_AVC2CTM_EVENT_t;
/**************************************************************/
#if 0
typedef struct {    
    unsigned int DeviceAddr; 	 /*device id */
	unsigned short SwVerCurrent; /*receive AVC*/
    unsigned short SwVerUpdate;	 /*receive USB*/
	char status;	
} __attribute__((packed)) SubDevice_t;
#endif
#if 1
typedef struct {    
    char DeviceAddr4; 	 /*device id */
	char DeviceAddr3; 	 /*device id */
	char DeviceAddr2; 	 /*device id */
	char DeviceAddr1; 	 /*device id */
	unsigned short SwCurVer; /*receive AVC*/
    unsigned short SwUsbVer; /*receive USB*/
    AVC2CTM_FtpFlag_t       Ftpflag;
    AVC2CTM_UpdateFlag_t 	Update_flag;
	char status;	
} __attribute__((packed)) SubDevice_t;
#endif
extern SubDevice_t g_SubDevice[150]; /*MC1 , T, M, MC2 */
extern char AVC_ScheduleTime[19];
extern packet_AVC2CTM_t 			avc2ctm_packet; /*ctm function add */
extern packet_AVC2CTM_SW_UPDATE_t 	avc2ctm_sw_up_packet; /*ctm sw update packet */
extern packet_AVC2CTM_EVENT_t  avc2ctm_log_event_packet; /*ctm sw log event packet */

extern char AVC2CTM_msg[53];

char read_link_carrier(void);
int init_ip_addr(void);
int check_my_ip(void);
int avc_tcp_init(void);
int check_can_connect_srv_ip(unsigned int srv_ip_addr);
int try_avc_tcp_connect(char *ip_addr, int port, int *need_reboot, int *need_renew);
int try_avc_tcp_read(char *buf, int maxlen);
int process_avc_tcp_data(char *buf, int len, int *avc_cmd_id);

int avc_tcp_close(char *_dbg_msg);

void set_app_version(unsigned short entire_ver);

void set_time_done_bit(void);
void clear_time_done_bit(void);
void increase_avcframe_err_and_send_enable(void);

int send_all_to_tcp_data_to_avc(void);
int send_update_result_to_avc(char login_stat, char down_stat, char update_stat);
int send_ics_update_result_to_avc(unsigned short uboot_ver, unsigned short kernel_ver, unsigned short app_ver);

int get_ftp_remote_file_path(char *buf, char *get_path, int buflen);

int send_tcp_data_to_avc(char *txbuf, int txlen);

void set_DI_errcode(unsigned short ec);
void clear_DI_errcode(unsigned short ec);

void print_avc_to_cop_packet(char *buf, int len);
int sub_system_status_update (char *buf, int len);
int sub_system_sw_update (char *buf, int len);
int avc_ctm_event_msg_update (char *buf, int len);
int init_subsystem_status (void);
#endif // __AVC_TCP_H__
