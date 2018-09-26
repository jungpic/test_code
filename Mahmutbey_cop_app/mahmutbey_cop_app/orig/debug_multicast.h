#ifndef __DEBUG_MULTICAST_H__
#define __DEBUG_MULTICAST_H__

#define QC_TEST_MULTICAST_RECV_IP		"234.100.100.100"
#define QC_TEST_MULTICAST_SEND_IP		"234.100.100.101"
#define QC_TEST_MULTICAST_PORT			55000

#define DEBUG_TCP_RECEIVE				1
#define DEBUG_TCP_SEND					2
#define DEBUG_CYCLE						3


void init_cop_debug_multicast(void);
void init_qc_test_multicast_recv(void);
void init_qc_test_multicast_send(void);

void start_make_qc_test_send_buf(void);
void end_make_qc_test_send_buf(void);

void multiDbg_printf(const char *fmt, ...);
void make_qc_test_send_buf(char *buf, int len, int type);

int connect_qc_test_multicast_recv(char *ip_addr, unsigned short port);
int connect_qc_test_multicast_send(char *ip_addr, unsigned short port);
int connect_cop_debug_multicast(void);

int close_cop_debug_multicast(void);
int close_qc_test_multicast_recv(void);
int close_qc_test_multicast_send(void);

int try_read_qc_test_msg(void);
int check_qc_test_data_valid(char *buf, int len);

int send_qc_test_multi_tx_data(char *buf, int len);
int send_cop_debug_data(void);

int DO_Write(unsigned char IO_100V, unsigned char IO_12V);
int DO_Clear(unsigned char IO_100V, unsigned char IO_12V);

#endif
