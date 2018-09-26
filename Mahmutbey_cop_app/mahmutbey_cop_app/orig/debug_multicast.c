/*
 */
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug_multicast.h"

#define COP_MULTI_PORT_NUMBER			55000
#define QC_TEST_RECV_ADDRESS			"234.100.100.100"
#define QC_TEST_SEND_ADDRESS			"234.100.100.101"
#define COP_DEBUG_ADDRESS				"234.100.100.102"
#define MULTICAST_SEND_TTL				1

#define ETH_RX_MAXBUF					1024
#define DUMMY_ADD_BUFLEN				2048
#define MAX_BUF_LEN						(ETH_RX_MAXBUF + DUMMY_ADD_BUFLEN)

static char 							qc_test_recv_buf[MAX_BUF_LEN];
static char 							qc_test_send_buf[MAX_BUF_LEN];


static int					cop_debug_sock;
static int					qc_test_recv_sock;
static int					qc_test_send_sock;
static int					qc_test_send_dbg_idx;

static struct sockaddr_in	cop_debug_srv;
static struct sockaddr_in	qc_test_recv_srv;
static struct sockaddr_in	qc_test_send_srv;
static struct sockaddr_in	qc_test_recv_cli;
static socklen_t			qc_test_recv_addrSize;

static int					enable_print_multi_qc_test;
static unsigned int			cop_debug_hbc;

static unsigned char		occ_pa_ack;
static unsigned char		occ_pei_call_ack;
static unsigned char		tractor_out_1;
static unsigned char		tractor_out_2;
static unsigned char		DO_Receive_Cmd;
static unsigned char		DO_100V_Receive;
static unsigned char		DO_12V_Receive;

int							debug_output_enable;

extern unsigned char my_ip_value[4];
extern char AVC_Master_IPName[16];

void init_cop_debug_multicast(void)
{
	if(cop_debug_sock >= 0)
		close(cop_debug_sock);

	debug_output_enable = 0;
	occ_pa_ack=0;
	occ_pei_call_ack=0;
	tractor_out_1=0;
	tractor_out_2=0;

    cop_debug_sock = -1;
	cop_debug_hbc = 1;
	enable_print_multi_qc_test = 0;
}

void init_qc_test_multicast_recv(void)
{
	if(qc_test_recv_sock >= 0)
		close(qc_test_recv_sock);

    qc_test_recv_sock = -1;
}

void init_qc_test_multicast_send(void)
{
	if(qc_test_send_sock >= 0)
		close(qc_test_send_sock);

    qc_test_send_sock = -1;
	enable_print_multi_qc_test = 0;
}

void start_make_qc_test_send_buf(void)
{
	qc_test_send_dbg_idx = 0;
}

void end_make_qc_test_send_buf(void)
{
	if (qc_test_send_dbg_idx <= 0)
		return;

	qc_test_send_buf[qc_test_send_dbg_idx++] = 0;
	send_qc_test_multi_tx_data(qc_test_send_buf, qc_test_send_dbg_idx);
}

void multiDbg_printf(const char *fmt, ...)
{
	int len;
	va_list arg;

	va_start(arg, fmt);
	len = vsprintf(&qc_test_send_buf[qc_test_send_dbg_idx], fmt, arg);
	va_end(arg);

	qc_test_send_dbg_idx += len;
}

void make_qc_test_send_buf(char *buf, int len, int type)
{
	int i;

	start_make_qc_test_send_buf();

	switch(type) {
		case DEBUG_TCP_RECEIVE:
			multiDbg_printf("\n----------- AVC(%s) -> COB(%d.%d.%d.%d) ---------------------\n",
							AVC_Master_IPName, my_ip_value[0], my_ip_value[1], my_ip_value[2], my_ip_value[3]);
			break;

		case DEBUG_TCP_SEND:
			multiDbg_printf("\n----------- COB(%d.%d.%d.%d) -> AVC(%s) ---------------------\n",
							my_ip_value[0], my_ip_value[1], my_ip_value[2], my_ip_value[3], AVC_Master_IPName);
			break;


		case DEBUG_CYCLE:
			multiDbg_printf("\n----------- AVC(Cycle) -> COB(%d.%d.%d.%d) --------------\n",
							my_ip_value[0], my_ip_value[1], my_ip_value[2], my_ip_value[3]);
			break;
		default:
			multiDbg_printf("\n$$$$$$$$$$$$$$$$$$ Unknown Packet $$$$$$$$$$$$$$$$$$$$$$$$\n");
			break;
	}
	if ( len > 0) {
		for (i = 1; i < len+1; i++) {
			if ((i % 16) == 0)
				multiDbg_printf("\n");
			multiDbg_printf("%02X ", buf[i-1]);
		}
		multiDbg_printf("\n---------------------------------------------------------\n");
	}
	end_make_qc_test_send_buf();
}

int connect_qc_test_multicast_recv(char *ip_addr, unsigned short port)
{
    struct ip_mreq qc_test_recv_mreq;

	int flag, ret;

	bzero(&qc_test_recv_srv, sizeof(qc_test_recv_srv));
	qc_test_recv_srv.sin_family = AF_INET;
	qc_test_recv_srv.sin_port = htons(port);
	if (inet_aton(ip_addr, &qc_test_recv_srv.sin_addr) < 0) {
		printf("ERROR, QC TEST RX, inet_aton");
		return -1; 
	}   
		
	if ((qc_test_recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		qc_test_recv_sock = -1;
		printf("ERROR, QC TEST RX, socket");
		return -1; 
	}   

	flag = fcntl(qc_test_recv_sock, F_GETFL);
	ret = fcntl(qc_test_recv_sock, F_SETFL, flag | O_NONBLOCK);
	if(ret == -1){
		close(qc_test_recv_sock);
		qc_test_recv_sock = -1;
		printf("< QC TEST RX, FCNTL ERROR >\n");
		return -1; 
	}   
	
	if (bind(qc_test_recv_sock, (struct sockaddr *)&qc_test_recv_srv, sizeof(qc_test_recv_srv)) < 0) {
		close(qc_test_recv_sock);
		qc_test_recv_sock = -1;
		printf("ERROR, QC TEST RX, bind");
		return -1; 
	}   
		
	if (inet_aton(ip_addr, &qc_test_recv_mreq.imr_multiaddr) < 0) {
		close(qc_test_recv_sock);
		qc_test_recv_sock = -1;
		printf("DI MSG, inet_aton");
		return -1; 
	}   

	qc_test_recv_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(qc_test_recv_sock, IPPROTO_IP,IP_ADD_MEMBERSHIP,&qc_test_recv_mreq,sizeof(qc_test_recv_mreq)) < 0) {
		close(qc_test_recv_sock);
		qc_test_recv_sock = -1;
		printf("ERROR, QC TEST RX, setsockopt");
		return -1; 
	}   
	
	printf("< QC TEST Program RX Multi Connected: %s(sock:%d) > \n", ip_addr, qc_test_recv_sock);
		
	return 0;
}


int connect_qc_test_multicast_send(char *ip_addr, unsigned short port)
{
    struct ip_mreq qc_test_send_mreq;

	int flag, ret;

	bzero(&qc_test_send_srv, sizeof(qc_test_send_srv));
	qc_test_send_srv.sin_family = AF_INET;
	qc_test_send_srv.sin_addr.s_addr = inet_addr(ip_addr);
	qc_test_send_srv.sin_port = htons(port);

	if ((qc_test_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Diagnosis program Multi Tx, socket");
		return -1; 
	}   

	flag = fcntl(qc_test_send_sock, F_GETFL);
	ret = fcntl(qc_test_send_sock, F_SETFL, flag | O_NONBLOCK);
	if(ret == -1){
		printf("< FCNTL ERROR >\n");
		return -1; 
	}   

	if (inet_aton(ip_addr, &qc_test_send_mreq.imr_multiaddr) < 0) {
		printf("Diagnosis program Multi Tx, inet_aton");
		return -1; 
	}   

	qc_test_send_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(qc_test_send_sock, IPPROTO_IP,IP_ADD_MEMBERSHIP,&qc_test_send_mreq,sizeof(qc_test_send_mreq)) < 0) {
		printf("Diagnosis program Multi Tx, setsockopt");
		close(qc_test_send_sock);
		qc_test_send_sock = -1; 
		return -1; 
	}   

	printf("< QC TEST Program TX Multi Connected: %s sock:%d > \n", ip_addr, qc_test_send_sock);

	return 0;
}

int connect_cop_debug_multicast(void)
{
    struct ip_mreq cop_debug_mreq;
    int ttl = MULTICAST_SEND_TTL;
    int ret, flags;
	char *ipaddr = COP_DEBUG_ADDRESS;

    if (cop_debug_sock > 0) {
        printf("< Already USE COP DEBUG SOCK >\n");
        return 0;
    }

    bzero(&cop_debug_srv, sizeof(cop_debug_srv));
    cop_debug_srv.sin_family = AF_INET;
    cop_debug_srv.sin_addr.s_addr = inet_addr(ipaddr);
    cop_debug_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

    if ((cop_debug_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("COP DEBUG socket");
        return -1;
    }

    flags = fcntl(cop_debug_sock, F_GETFL);
    ret = fcntl(cop_debug_sock, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< cop_debug_sock FCNT ERROR >\n");
        close(cop_debug_sock);
        return -1;
    }

    if (inet_aton(ipaddr, &cop_debug_mreq.imr_multiaddr) < 0) {
        perror("COP DEBUG inet_aton");
        close(cop_debug_sock);
        cop_debug_sock = -1;
        return -1;
    }

    if (setsockopt(cop_debug_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
    {
        perror("COP DEBUG setsockopt");
        close(cop_debug_sock);
        cop_debug_sock = -1;
        return -1;
    }

    printf("< COP DEBUG CONNECTED:%s(SOCK:%d) > \n", ipaddr, cop_debug_sock);

    return 0;
}

int close_cop_debug_multicast(void)
{
    if (cop_debug_sock < 0)
        return -1;

    close(cop_debug_sock);
	cop_debug_sock = -1;

    return 0;
}

int close_qc_test_multicast_recv(void)
{
    if (qc_test_recv_sock < 0)
        return -1;

    close(qc_test_recv_sock);
	qc_test_recv_sock = -1;

    return 0;
}

int close_qc_test_multicast_send(void)
{
    if (qc_test_send_sock < 0)
        return -1;

    close(qc_test_send_sock);
	qc_test_send_sock = -1;

    return 0;
}

int try_read_qc_test_msg(void)
{
	int ret = 0;
	int rx_len = 0;

	errno = 0;

	if(qc_test_recv_sock < 0) {
		printf("%s-%s[%d] : -ERR- : qc_test_recv_sock(%d) is not opened.\n", __FILE__, __FUNCTION__, __LINE__, qc_test_recv_sock);
		return 0;
	}

	rx_len = recv(qc_test_recv_sock, qc_test_recv_buf, ETH_RX_MAXBUF, 0);

	if(rx_len == -1){
		if((errno != EAGAIN) && (errno != EWOULDBLOCK)){
			printf("<< QC TEST SOCK %d RECV ERROR, errno: %d >>\n", qc_test_recv_sock, errno);
		}
	}
	if (rx_len <= 0)
		return 0;

	ret = check_qc_test_data_valid(qc_test_recv_buf, rx_len);
	if (ret != 0)
		rx_len = -1;


	return rx_len;
}


extern time_t first_cop_debug_time;

int check_qc_test_data_valid(char *buf, int len)
{
	int i;
	unsigned short tmp;

	tmp = (buf[1]<<8) | buf[0];
	if (tmp != len) {
		printf("Lenth is wrong => len(%d), BUF[1:0](%d)\n", len, tmp);
		return -1;
	}

	if (buf[2] != 0x5A) return -1;

	if (buf[3] != 0x01) return -1;

	if (buf[4] == 0x01)
		enable_print_multi_qc_test = 1;
	else if (buf[4] == 0x02)
		enable_print_multi_qc_test = 0;
	else
		return -1;

//	if ( ((buf[5] & 0x04) != 0x04 ) || ((buf[5] & 0x02) != 0x02 ) || ((buf[5] & 0x01) != 0x01 ) )
//		return -1;
//	else if ( (buf[5] & 0x04) == 0x04 )
	if ( (buf[5] & 0x04) == 0x04 ) {
		DO_Init();
		first_cop_debug_time = time(NULL);
		debug_output_enable = 1;
	} else if (buf[5] & 0x01) {
		debug_output_enable = 1;
		first_cop_debug_time = time(NULL);
		DO_Write((unsigned char)buf[6], (unsigned char)buf[7]);
	} else if (buf[5] & 0x02) {
		debug_output_enable = 1;
		first_cop_debug_time = time(NULL);
		DO_Clear((unsigned char)buf[6], (unsigned char)buf[7]);
    }

	DO_Receive_Cmd = buf[5];
	DO_100V_Receive = buf[6];
	DO_12V_Receive = buf[7];

	return 0;
}

int DO_Init(void)
{
	/* 12V DO */
	occ_pa_ack_set_low();
	occ_pa_ack = 0;
	occ_pei_call_ack_set_low();
	occ_pei_call_ack = 0;

	/* 100V DO */
	tractor_out_1_set_low();
	tractor_out_1 = 0;
	tractor_out_2_set_low();
	tractor_out_2 = 0;
}

int DO_Write(unsigned char IO_100V, unsigned char IO_12V)
{
	switch(IO_12V) {
		case 0x00:	break;
		case 0x01:	occ_pa_ack_set_high();			occ_pa_ack=1;		break;
		case 0x02:	occ_pei_call_ack_set_high();		occ_pei_call_ack=1;	break;
		default:
			printf("Unknown command(Set 12V : %d).....\n", IO_12V);
			break;
	}

	switch(IO_100V) {
		case 0x00:	break;
		case 0x01:	tractor_out_1_set_high();		tractor_out_1=1;	break;
		case 0x02:	tractor_out_2_set_high();		tractor_out_2=1;	break;
		default:
			printf("Unknown command(Set 100V : %d).....\n", IO_100V);
			break;
	}
}

int DO_Clear(unsigned char IO_100V, unsigned char IO_12V)
{
	switch(IO_12V) {
		case 0x00:	break;
		case 0x01:	occ_pa_ack_set_low();			occ_pa_ack=0;		break;
		case 0x02:	occ_pei_call_ack_set_low();		occ_pei_call_ack=0;	break;
		default:
			printf("Unknown command(Clear 12V : %d).....\n", IO_12V);
			break;
	}

	switch(IO_100V) {
		case 0x00:	break;
		case 0x01:	tractor_out_1_set_low();		tractor_out_1=0;	break;
		case 0x02:	tractor_out_2_set_low();		tractor_out_2=0;	break;
		default:
			printf("Unknown command(Clear 100V : %d).....\n", IO_100V);
			break;
	}
}

int send_qc_test_multi_tx_data(char *buf, int len)
{
	int ret;

	if (!enable_print_multi_qc_test) {
		return 0;
	}

	if(qc_test_send_sock <= 0) {
		printf("%s-%s[%d] : -ERR- : qc_test_send_sock(%d) is not opened.\n", __FILE__, __FUNCTION__, __LINE__, qc_test_send_sock);
		return 0;
	}

	ret = sendto(qc_test_send_sock, buf, len, 0,
				(struct sockaddr *)&qc_test_send_srv, sizeof(qc_test_send_srv));

	return ret;
}

int send_cop_debug_data(void)
{
    int ret;
	int idx = 0;
	unsigned char buf[24];
	unsigned char DIStatus_100V = 0x00;
	unsigned char DIStatus_12V = 0x00;
	unsigned char DOStatus_100V = 0x00;
	unsigned char DOStatus_12V = 0x00;

	if( (!enable_print_multi_qc_test) || (cop_debug_sock < 0) )
		return 0;

	memset(buf, 0x00, sizeof(buf));

	buf[idx++] = 0;
	buf[idx++] = 0;

	buf[idx++] = 0xA5;	/* Magic number */
	buf[idx++] = 0x03;	/* Command code : ALL to Receiver */

	buf[idx++] = 0x02;	/* Device select : COB = 0x02 */

	if(!tractor_in_1_get_level())
		DIStatus_100V |= 1<<0;

	if(!tractor_in_2_get_level())
		DIStatus_100V |= 1<<1;

	DOStatus_100V = (tractor_out_1);
	DOStatus_100V |= (tractor_out_2 << 1);

	if(!occ_pa_en_get_level())
		DIStatus_12V |= 1<<0;

	if(!occ_tx_rx_en_get_level())
		DIStatus_12V |= 1<<1;

	DOStatus_12V = (occ_pa_ack);
	DOStatus_12V |= (occ_pei_call_ack << 1);

	buf[idx++] = DIStatus_100V;
	buf[idx++] = DIStatus_12V;
	buf[idx++] = DOStatus_100V;
	buf[idx++] = DOStatus_12V;

	buf[idx++] = DO_Receive_Cmd;
	buf[idx++] = DO_100V_Receive;
	buf[idx++] = DO_12V_Receive;

	buf[idx++] = 0;		/* RS-485 Test Status in AVC */

	/***** RS-485 Total TX Count *****/
	buf[idx++] = 0;
	buf[idx++] = 0;
	buf[idx++] = 0;
	buf[idx++] = 0;

	/***** RS-485 Total RX Count *****/
	buf[idx++] = 0;
	buf[idx++] = 0;
	buf[idx++] = 0;
	buf[idx++] = 0;

	/***** RS-485 Total ERR Count *****/
	buf[idx++] = 0;
	buf[idx++] = 0;
	buf[idx++] = 0;
	buf[idx++] = 0;

	/* Heart Beat Counter */
	buf[idx++] = (cop_debug_hbc >> 0)  & 0xFF;
	buf[idx++] = (cop_debug_hbc >> 8)  & 0xFF;
	buf[idx++] = (cop_debug_hbc >> 16) & 0xFF;
	buf[idx++] = (cop_debug_hbc >> 24) & 0xFF;

	buf[idx++] = 0;

	buf[0] = idx & 0xFF;
	buf[1] = (idx >> 8) & 0xFF;

    ret = sendto(cop_debug_sock, buf, idx, 0,
            (struct sockaddr *)&cop_debug_srv, sizeof(cop_debug_srv));

	if(ret > 0) {
		if(cop_debug_hbc > 0x7FFFFFFF)
			cop_debug_hbc = 0;
		else
			cop_debug_hbc++;
	}

    return ret;
}
