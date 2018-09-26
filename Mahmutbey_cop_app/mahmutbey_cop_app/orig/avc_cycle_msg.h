#ifndef __PROCESS_AVC_MULTICAST_H__
#define __PROCESS_AVC_MULTICAST_H__

#define AVC_CYCLE_FRAME_ID	0x01
#define AVC_CYCLE_FRAME_LEN	22

#define AVC_CYCLE_TIME_BUF_OFF	9
#define AVC_CYCLE_HBC_BUF_OFF	16

void init_avc_cycle_data(void);

int connect_avc_cycle_multicast(void);
int avc_cycle_close(void);
int try_read_avc_cycle_data(char *avc_srv_ip, int is_fixed);
int is_changed_avc_cycle_ip(void);
unsigned int get_avc_srv_ip(void);

int setup_local_time(void);
int check_avc_hbc(void);
void reset_avc_cycle_hbc(void);
void reset_avc_srv_ip(void);

int cop_set_time(void);

#endif
