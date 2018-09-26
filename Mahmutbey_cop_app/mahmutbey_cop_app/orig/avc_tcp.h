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

#endif // __AVC_TCP_H__
