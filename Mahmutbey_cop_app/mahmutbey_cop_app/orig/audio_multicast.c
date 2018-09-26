/*
 */
#include <stdio.h>
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

#include "audio_multicast.h"
#include "gpio_key_dev.h"
#include "cob_process.h"
#include "bcd.h"
#include "crc16.h"

#define DEBUG_LINE                      1
#if (DEBUG_LINE == 1)
#define DBG_LOG(fmt, args...)   \
    do {                    \
            fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);        \
    } while(0)
#else
#define DBG_LOG(fmt, args...)
#endif

extern int pei_call_count;

extern unsigned int get_rx_mult_addr(char *buf, int len);
extern unsigned int get_tx_mult_addr(char *buf, int len);
extern unsigned int get_mon_mult_addr(char *buf, int len);

#define MULTI_RX_BUF_LEN	1600
#if 0
#define MAX_RTP_BUF_N		256
#define PLAY_FIRE_FUNC_LIMIT_RTP_BUF_N	20 // Ver 0.78, 2014/08/26, 20->5
#define PLAY_AUTO_FUNC_LIMIT_RTP_BUF_N	20 // Ver 0.78, 2014/08/26, 20->5
#define PLAY_PA_LIMIT_RTP_BUF_N		2 // Ver 0.78, 2014/08/26, 5->3
#else
#define MAX_RTP_BUF_N		256	// Ver 0.82, 2014/09/12
#define PLAY_FIRE_FUNC_LIMIT_RTP_BUF_N	20 // Ver 0.82, 2014/09/12
#define PLAY_AUTO_FUNC_LIMIT_RTP_BUF_N	20 // Ver 0.82, 2014/09/12
#define PLAY_PA_LIMIT_RTP_BUF_N		3 // Ver 0.82, 2014/09/12
#endif

#define RTP_SEQ_LIMIT_LOW	1
#define RTP_SEQ_LIMIT_HIGH	2000

static char *__monitoring_multi_addr_str[] = {
    "0.0.0.0",
    "234.1.3.1", // MON_BCAST_TYPE_AUTO
    "234.1.3.2", // MON_BCAST_TYPE_FUNC
    "234.1.3.3", // MON_BCAST_TYPE_COP_PASSIVE
    "234.1.3.4", // MON_BCAST_TYPE_OCC_PASSIVE
    "234.1.3.5", // MON_BCAST_TYPE_FIRE
};

/*
static char *__monitoring_multi_comment[] = {
    "None",
    "OCC Passive Broadcasting",
    "COP Passive Broadcasting",
    "Function Broadcating",
    "Auto Broadcasting",
    "Fire Broadcasting",
};
*/


#define MULTICAST_SEND_TTL 64


static int bcast_mon_sock;
static struct sockaddr_in bcast_mon_srv;


static int cop_pa_multi_tx_sock;
static struct sockaddr_in cop_pa_multi_tx_srv;

static int multi_occ_pa_tx_sock;

static int multi_call_tx_sock;
static int multi_call_tx_sock_2nd;
static struct sockaddr_in cop_call_multi_tx_srv;
static struct sockaddr_in cop_call_multi_tx_srv_2nd;

static int multi_passive_tx_sock;

static int multi_rx_sock;
static int multi_rx_sock_2nd;

static struct sockaddr_in multi_rx_srv;
static struct sockaddr_in multi_rx_srv_2nd;
static struct sockaddr_in cop_occ_pa_multi_tx_srv;

static struct sockaddr_in cop_multi_passive_tx_srv;

static int unlock_fire_rtp_recv;
static int unlock_auto_rtp_recv;
static int unlock_func_rtp_recv;

static int occ_pa_mon_sock;
static int cop_pa_mon_sock;
static int func_mon_sock;
static int fire_mon_sock;
static int auto_mon_sock;

static int multi_mon_sock;
static struct sockaddr_in cop_multi_mon_srv;

static char cop_tx_addr_str[32];
static int cop_tx_addr_int;
static char cop_tx_addr_str_2nd[32];
static int cop_tx_addr_int_2nd;

static char cop_rx_addr_str[32];
static int cop_rx_addr_int;

static char cop_mon_addr_str[32];
static int cop_mon_addr_int;

void init_cop_multicast(void)
{
    cop_pa_multi_tx_sock = -1;
    multi_occ_pa_tx_sock = -1;
    multi_passive_tx_sock = -1;
    multi_call_tx_sock = -1;
	multi_call_tx_sock_2nd = -1;
    multi_rx_sock = -1;
	multi_rx_sock_2nd = -1;
    multi_mon_sock = -1;

    memset(cop_tx_addr_str, 0, 32);
    memset(cop_tx_addr_str_2nd, 0, 32);
    memset(cop_rx_addr_str, 0, 32);
    memset(cop_mon_addr_str, 0, 32);
    cop_tx_addr_int = 0;
	cop_tx_addr_int_2nd = 0;
    cop_rx_addr_int = 0;
    cop_mon_addr_int = 0;
}

void init_auto_fire_func_monitoring_bcast(void)
{
    auto_mon_sock = connect_monitoring_bcast_multicast(MON_BCAST_TYPE_AUTO);
    fire_mon_sock = connect_monitoring_bcast_multicast(MON_BCAST_TYPE_FIRE);
    func_mon_sock = connect_monitoring_bcast_multicast(MON_BCAST_TYPE_FUNC);
    cop_pa_mon_sock = connect_monitoring_bcast_multicast(MON_BCAST_TYPE_COP_PASSIVE);
    occ_pa_mon_sock = connect_monitoring_bcast_multicast(MON_BCAST_TYPE_OCC_PASSIVE);

    unlock_auto_rtp_recv = 0;
    unlock_fire_rtp_recv = 0;
    unlock_func_rtp_recv = 0;
}

void temporary_mon_enc_connect_to_passive_pa_tx(int onoff, unsigned int last_mon_addr)
{
	static struct sockaddr_in mon_addr;
	static actived=0;

	if(onoff == 1 && actived == 0)
	{
		cop_monitoring_encoding_stop();
		close_cop_multi_monitoring();
		if (cop_pa_mon_sock > 0)
		{
			printf("cop_pa_mon_sock(0x%x) CLOSE\n", cop_pa_mon_sock);
			close(cop_pa_mon_sock);
		}
		cop_monitoring_encoding_start();
		connect_cop_multi_monitoring(0, 0, 0, inet_addr(COP_PASSIVE_BCAST_TX_MULTI_ADDR));

		mon_addr.sin_addr.s_addr = last_mon_addr;
		printf(">> START Passive PA through mon_enc!! keep mon_addr = %s\n", inet_ntoa(mon_addr.sin_addr));
		//mon_addr = last_mon_addr;

		actived = 1;
	}
	else if (onoff == 0 && actived == 1)//recovery
	{
		cop_monitoring_encoding_stop();
		close_cop_multi_monitoring();
		if(mon_addr.sin_addr.s_addr != 0)
		{
			cop_pa_mon_sock = connect_monitoring_bcast_multicast(MON_BCAST_TYPE_COP_PASSIVE);
			printf(">> FINISH Passive PA through mon_enc!! recovery mon_addr = %s\n", inet_ntoa(mon_addr.sin_addr));
			cop_monitoring_encoding_start();
			connect_cop_multi_monitoring(0, 0, 0, mon_addr.sin_addr.s_addr);
			mon_addr.sin_addr.s_addr = 0;
		}
		actived = 0;
	}
}

void close_auto_fire_func_monitoring_bcast(void)
{
    if (occ_pa_mon_sock > 0) {
        //printf("< CLOSE %s MULTI MON(%d) >\n",
		//									__monitoring_multi_comment[MON_BCAST_TYPE_OCC_PASSIVE], occ_pa_mon_sock);
        close(occ_pa_mon_sock);
    }

    if (cop_pa_mon_sock > 0) {
        //printf("< CLOSE %s MULTI MON(%d) >\n",
		//									__monitoring_multi_comment[MON_BCAST_TYPE_COP_PASSIVE], cop_pa_mon_sock);
        close(cop_pa_mon_sock);
    }

    if (func_mon_sock > 0) {
        //printf("< CLOSE %s MULTI MON(%d) >\n",
		//									__monitoring_multi_comment[MON_BCAST_TYPE_FUNC], func_mon_sock);
        close(func_mon_sock);
    }

	if (fire_mon_sock > 0) {
		//printf("< CLOSE %s MULTI MON(%d) >\n",
		//									__monitoring_multi_comment[MON_BCAST_TYPE_FIRE], fire_mon_sock);
		close(fire_mon_sock);
	}

    if (auto_mon_sock > 0) {
        //printf("< CLOSE %s MULTI MON(%d) >\n",
		//									__monitoring_multi_comment[MON_BCAST_TYPE_AUTO], auto_mon_sock);
        close(auto_mon_sock);
    }
}

void clear_rx_tx_mon_multi_addr(void)
{
    cop_tx_addr_int = 0;
	cop_tx_addr_int_2nd = 0;
    cop_rx_addr_int = 0;
    cop_mon_addr_int = 0;
}

unsigned int  __get_rx_multi_addr(void)
{
    return cop_rx_addr_int;
}

unsigned int  __get_tx_multi_addr(void)
{
    return cop_tx_addr_int;
}

unsigned int  __get_mon_multi_addr(void)
{
    return cop_mon_addr_int;
}

static int connect_cop_pa_multi_tx(char *multitxaddr)
{
    struct ip_mreq cop_tx_mreq;
    int ttl = MULTICAST_SEND_TTL;
    int ret, flags;

    if (cop_pa_multi_tx_sock > 0) {
        printf("< Already USE MULTICAST TX SOCK >\n");
        return 0;
    }

    bzero(&cop_pa_multi_tx_srv, sizeof(cop_pa_multi_tx_srv));
    cop_pa_multi_tx_srv.sin_family = AF_INET;
    cop_pa_multi_tx_srv.sin_addr.s_addr = inet_addr(multitxaddr);
    cop_pa_multi_tx_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

    if ((cop_pa_multi_tx_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("COP MULTI TX socket");
        return -1;
    }

    flags = fcntl(cop_pa_multi_tx_sock, F_GETFL);
    ret = fcntl(cop_pa_multi_tx_sock, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< tx_sock FCNT ERROR >\n");
        close(cop_pa_multi_tx_sock);
        return -1;
    }

    if (inet_aton(multitxaddr, &cop_tx_mreq.imr_multiaddr) < 0) {
        perror("COP MULTI TX inet_aton");
        close(cop_pa_multi_tx_sock);
        cop_pa_multi_tx_sock = -1;
        return -1;
    }

    if (setsockopt(cop_pa_multi_tx_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
    {
        perror("COP MULTI TX setsockopt");
        close(cop_pa_multi_tx_sock);
        cop_pa_multi_tx_sock = -1;
        return -1;
    }

    printf("< TX MULTI CONNECTED:%s(SOCK:%d) > \n", multitxaddr, cop_pa_multi_tx_sock);

    return 0;
}

static int connect_cop_multi_tx_while_othercall(char *multitxaddr)
{
    struct ip_mreq cop_tx_mreq;
    int ttl = MULTICAST_SEND_TTL;
    int ret, flags;

    if (multi_passive_tx_sock > 0) {
        printf("< Already USE MULTICAST PASSIVE TX SOCK >\n");
        return 0;
    }

    bzero(&cop_multi_passive_tx_srv, sizeof(cop_multi_passive_tx_srv));
    cop_multi_passive_tx_srv.sin_family = AF_INET;
    cop_multi_passive_tx_srv.sin_addr.s_addr = inet_addr(multitxaddr);
    cop_multi_passive_tx_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

    if ((multi_passive_tx_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("COP MULTI TX socket");
        return -1;
    }

    flags = fcntl(multi_passive_tx_sock, F_GETFL);
    ret = fcntl(multi_passive_tx_sock, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< tx_sock FCNT ERROR >\n");
        return -1;
    }

    if (inet_aton(multitxaddr, &cop_tx_mreq.imr_multiaddr) < 0) {
        perror("COP MULTI TX inet_aton");
        close(multi_passive_tx_sock);
        multi_passive_tx_sock = -1;
        return -1;
    }

    if (setsockopt(multi_passive_tx_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
    {
        perror("COP MULTI TX setsockopt");
        close(multi_passive_tx_sock);
        multi_passive_tx_sock = -1;
        return -1;
    }

    printf("< TX(CAB-PA ON CALL) MULTI CONNECTED:%s(SOCK:%d) > \n", multitxaddr, multi_passive_tx_sock);

    return 0;
}

static int connect_call_multi_tx(char *multitxaddr)
{
    struct ip_mreq cop_tx_mreq;
    int ttl = MULTICAST_SEND_TTL;
    int ret, flags;

    if (multi_call_tx_sock > 0) {
        printf("< Already USING CALL MULTICAST TX SOCK >\n");
        return 0;
    }

    bzero(&cop_call_multi_tx_srv, sizeof(cop_call_multi_tx_srv));
    cop_call_multi_tx_srv.sin_family = AF_INET;
    cop_call_multi_tx_srv.sin_addr.s_addr = inet_addr(multitxaddr);
    cop_call_multi_tx_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

    if ((multi_call_tx_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("CALL MULTI TX socket");
        return -1;
    }

    flags = fcntl(multi_call_tx_sock, F_GETFL);
    ret = fcntl(multi_call_tx_sock, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< CALL tx_sock FCNT ERROR >\n");
        return -1;
    }

    if (inet_aton(multitxaddr, &cop_tx_mreq.imr_multiaddr) < 0) {
        perror("OCC PA MULTI TX inet_aton");
        close(multi_call_tx_sock);
        multi_call_tx_sock = -1;
        return -1;
    }

    if (setsockopt(multi_call_tx_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
    {
        perror("CALL MULTI TX setsockopt");
        close(multi_call_tx_sock);
        multi_call_tx_sock = -1;
        return -1;
    }

    printf("< CALL TX MULTI CONNECTED:%s(SOCK:%d) > \n", multitxaddr, multi_call_tx_sock);

    return 0;
}

static int connect_call_multi_tx_2nd(char *multitxaddr)
{
    struct ip_mreq cop_tx_mreq;
    int ttl = MULTICAST_SEND_TTL;
    int ret, flags;

    if (multi_call_tx_sock_2nd > 0) {
        printf("< Already USING CALL 2nd MULTICAST TX SOCK >\n");
        return 0;
    }

    bzero(&cop_call_multi_tx_srv_2nd, sizeof(cop_call_multi_tx_srv_2nd));
    cop_call_multi_tx_srv_2nd.sin_family = AF_INET;
    cop_call_multi_tx_srv_2nd.sin_addr.s_addr = inet_addr(multitxaddr);
    cop_call_multi_tx_srv_2nd.sin_port = htons(COP_MULTI_PORT_NUMBER);

    if ((multi_call_tx_sock_2nd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("CALL 2nd MULTI TX socket");
        return -1;
    }

    flags = fcntl(multi_call_tx_sock_2nd, F_GETFL);
    ret = fcntl(multi_call_tx_sock_2nd, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< CALL 2nd tx_sock FCNT ERROR >\n");
        return -1;
    }

    if (inet_aton(multitxaddr, &cop_tx_mreq.imr_multiaddr) < 0) {
        perror("OCC PA MULTI TX inet_aton");
        close(multi_call_tx_sock_2nd);
        multi_call_tx_sock_2nd = -1;
        return -1;
    }

    if (setsockopt(multi_call_tx_sock_2nd, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
    {
        perror("CALL MULTI TX setsockopt");
        close(multi_call_tx_sock_2nd);
        multi_call_tx_sock_2nd = -1;
        return -1;
    }

    printf("< CALL 2nd TX MULTI CONNECTED:%s(SOCK:%d) > \n", multitxaddr, multi_call_tx_sock_2nd);

    return 0;
}

static int connect_occ_pa_multi_tx(char *multitxaddr)
{
    struct ip_mreq cop_tx_mreq;
    int ttl = MULTICAST_SEND_TTL;
    int ret, flags;

    if (multi_occ_pa_tx_sock > 0) {
        printf("< Already USE OCC PA MULTICAST TX SOCK >\n");
        return 0;
    }

    bzero(&cop_occ_pa_multi_tx_srv, sizeof(cop_occ_pa_multi_tx_srv));
    cop_occ_pa_multi_tx_srv.sin_family = AF_INET;
    cop_occ_pa_multi_tx_srv.sin_addr.s_addr = inet_addr(multitxaddr);
    cop_occ_pa_multi_tx_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

    if ((multi_occ_pa_tx_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("OCC PA MULTI TX socket");
        return -1;
    }

    flags = fcntl(multi_occ_pa_tx_sock, F_GETFL);
    ret = fcntl(multi_occ_pa_tx_sock, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< OCC PA tx_sock FCNT ERROR >\n");
        return -1;
    }

    if (inet_aton(multitxaddr, &cop_tx_mreq.imr_multiaddr) < 0) {
        perror("OCC PA MULTI TX inet_aton");
        close(multi_occ_pa_tx_sock);
        multi_occ_pa_tx_sock = -1;
        return -1;
    }

    if (setsockopt(multi_occ_pa_tx_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
    {
        perror("OCC PA MULTI TX setsockopt");
        close(multi_occ_pa_tx_sock);
        multi_occ_pa_tx_sock = -1;
        return -1;
    }

    printf("< OCC-PA TX MULTI CONNECTED:%s(SOCK:%d) > \n", multitxaddr, multi_occ_pa_tx_sock);

    return 0;
}

int connect_cop_passive_bcast_tx(char *multitxaddr)
{
    return connect_cop_pa_multi_tx(multitxaddr);
}

int connect_occ_passive_bcast_tx(void)
{
    return connect_occ_pa_multi_tx(OCC_PASSIVE_BCAST_TX_MULTI_ADDR);
}

int connect_cop_passive_bcast_tx_while_othercall(void)
{
    return connect_cop_multi_tx_while_othercall(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
}

int close_cop_multi_tx(void)
{
//printf("< CLOSE TX SOCK(%d) >\n", cop_pa_multi_tx_sock);
    if (cop_pa_multi_tx_sock < 0)
        return -1;

    cop_tx_addr_int = 0;

    printf("< CLOSE COP MULTICAST TX(%d) >\n", cop_pa_multi_tx_sock);

    close(cop_pa_multi_tx_sock);

    cop_pa_multi_tx_sock = -1;

    return 0;
}

int close_call_multi_tx(void)
{
    if (multi_call_tx_sock < 0)
        return -1;

    printf("< CLOSE CALL MULTICAST TX(%d) >\n", multi_call_tx_sock);

    close(multi_call_tx_sock);

    multi_call_tx_sock = -1;

    return 0;
}

int close_call_multi_tx_2nd(void)
{
    if (multi_call_tx_sock_2nd < 0)
        return -1;

    printf("< CLOSE CALL 2nd MULTICAST TX(%d) >\n", multi_call_tx_sock_2nd);

    close(multi_call_tx_sock_2nd);

    multi_call_tx_sock_2nd = -1;

    return 0;
}

int close_call_and_occ_pa_multi_tx(void)
{
    if (multi_occ_pa_tx_sock < 0)
        return -1;

    printf("< CLOSE OCC PA MULTICAST TX(%d) >\n", multi_occ_pa_tx_sock);

    close(multi_occ_pa_tx_sock);

    multi_occ_pa_tx_sock = -1;

    return 0;
}

int close_cop_multi_passive_tx(void)
{
    if (multi_passive_tx_sock < 0)
        return -1;

    printf("< CLOSE PASSIVE TX ON CALL(%d) >\n", multi_passive_tx_sock);

    close(multi_passive_tx_sock);

    multi_passive_tx_sock = -1;

    return 0;
}

int send_cop_multi_tx_data(char *buf, int len, int dd)
{
    int ret;
    ret = sendto(cop_pa_multi_tx_sock, buf, len, 0,
            (struct sockaddr *)&cop_pa_multi_tx_srv, sizeof(cop_pa_multi_tx_srv));

    return ret;
}

int send_cop_multi_tx_data_passive_on_call(char *buf, int len)
{
    int ret;
    ret = sendto(multi_passive_tx_sock, buf, len, 0,
            (struct sockaddr *)&cop_multi_passive_tx_srv, sizeof(cop_multi_passive_tx_srv));

    return ret;
}

int send_call_multi_tx_data(char *buf, int len)
{
    int ret;
    ret = sendto(multi_call_tx_sock, buf, len, 0,
            (struct sockaddr *)&cop_call_multi_tx_srv, sizeof(cop_call_multi_tx_srv));

	if(pei_call_count == 2)
	{
		ret = sendto(multi_call_tx_sock_2nd, buf, len, 0,
				(struct sockaddr *)&cop_call_multi_tx_srv_2nd, sizeof(cop_call_multi_tx_srv_2nd));
	}

    return ret;
}

int send_occ_pa_multi_tx_data(char *buf, int len)
{
    int ret;
    ret = sendto(multi_occ_pa_tx_sock, buf, len, 0,
            (struct sockaddr *)&cop_occ_pa_multi_tx_srv, sizeof(cop_occ_pa_multi_tx_srv));

    return ret;
}

int is_open_multi_mon_sock(void)
{
   return multi_mon_sock;
}

int send_cop_multi_mon_data(char *buf, int len)
{
    int ret;

    if (multi_mon_sock < 0)
       return -1;
    ret = sendto(multi_mon_sock, buf, len, 0,
            (struct sockaddr *)&cop_multi_mon_srv, sizeof(cop_multi_mon_srv));

    return ret;
}

/********** COP CAB2CBA CALL/Monitoring *******************************************/
int connect_cop_cab2cab_call_rx_multicast(char *buf, int len)
{
    int ret;
    struct ip_mreq rx_mreq;
    unsigned int rx_multi_addr;
    int flags;

    rx_multi_addr = get_rx_mult_addr(buf, len);
    if ((rx_multi_addr & 0xff) != 234) {
        printf("%s(), RX Multi addr error: 0x%X\n", __func__, rx_multi_addr);
        return -1;
    }

    cop_rx_addr_int = rx_multi_addr;

    if (rx_multi_addr) {
        bzero(&multi_rx_srv, sizeof(multi_rx_srv));
        multi_rx_srv.sin_addr.s_addr = rx_multi_addr;
        multi_rx_srv.sin_family = AF_INET;
        multi_rx_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

        strcpy(cop_rx_addr_str, inet_ntoa(multi_rx_srv.sin_addr));

        if ((multi_rx_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("COP MULTI RX, socket");
            return -1;
        }

        flags = fcntl(multi_rx_sock, F_GETFL);
        ret = fcntl(multi_rx_sock, F_SETFL, flags | O_NONBLOCK);
        if (ret == -1) {
            printf("< rx_sock FCNT ERROR >\n");
            return -1;
        }

        if (bind(multi_rx_sock, (struct sockaddr *)&multi_rx_srv, sizeof(multi_rx_srv)) < 0)
        {
            perror("COP MULTI RX, bind");
            close(multi_rx_sock);
            multi_rx_sock = -1;
            return -1;
        }

        if (inet_aton(cop_rx_addr_str, &rx_mreq.imr_multiaddr) < 0) {
            perror("COP MULTI MON, inet_aton");
            close(multi_rx_sock);
            multi_rx_sock = -1;
            return -1;
        }

        rx_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(multi_rx_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &rx_mreq, sizeof(rx_mreq)) < 0)
        {
            perror("COP MULTI RX setsockopt");
            close(multi_rx_sock);
            multi_rx_sock = -1;
            return -1;
        }

        printf("< 2, RX MULTI CONNECTED:%s(SOCK:%d) > \n", cop_rx_addr_str, multi_rx_sock);
    } else {
        printf("< 2, CANNOT RX MULTI CONNECTED(ADDR:%d) > \n", rx_multi_addr);
        close_cop_multi_rx_sock();
    }

    if (ret == -1)
        return -1;

    return 0;
}

int connect_cop_cab2cab_call_multicast(char *buf, int len)
{
    int ret;
    struct ip_mreq rx_mreq;
    unsigned int rx_multi_addr;
    unsigned int tx_multi_addr;
    unsigned int mon_multi_addr;
    unsigned int tx_addr1, tx_addr2, tx_addr3, tx_addr4;
    int flags;

    rx_multi_addr = get_rx_mult_addr(buf, len);
    if ((rx_multi_addr & 0xff) != 234) {
        printf("%s(), RX Multi addr error: 0x%X\n", __func__, rx_multi_addr);
        return -1;
    }

    tx_multi_addr = get_tx_mult_addr(buf, len);
    if ((tx_multi_addr & 0xff) != 234) {
        printf("%s(), TX Multi addr error: 0x%X\n", __func__, tx_multi_addr);
        return -1;
    }

    mon_multi_addr = get_mon_mult_addr(buf, len);

    cop_rx_addr_int = rx_multi_addr;
    cop_tx_addr_int = tx_multi_addr;
    cop_mon_addr_int = mon_multi_addr;

    if (rx_multi_addr) {
        bzero(&multi_rx_srv, sizeof(multi_rx_srv));
        multi_rx_srv.sin_addr.s_addr = rx_multi_addr;
        multi_rx_srv.sin_family = AF_INET;
        multi_rx_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

        strcpy(cop_rx_addr_str, inet_ntoa(multi_rx_srv.sin_addr));

        if ((multi_rx_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("COP MULTI RX, socket");
            return -1;
        }

        flags = fcntl(multi_rx_sock, F_GETFL);
        ret = fcntl(multi_rx_sock, F_SETFL, flags | O_NONBLOCK);
        if (ret == -1) {
            printf("< rx_sock FCNT ERROR >\n");
            return -1;
        }

        if (bind(multi_rx_sock, (struct sockaddr *)&multi_rx_srv, sizeof(multi_rx_srv)) < 0)
        {
            perror("COP MULTI RX, bind");
            close(multi_rx_sock);
            multi_rx_sock = -1;
            return -1;
        }

        if (inet_aton(cop_rx_addr_str, &rx_mreq.imr_multiaddr) < 0) {
            perror("COP MULTI MON, inet_aton");
            close(multi_rx_sock);
            multi_rx_sock = -1;
            return -1;
        }

        rx_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(multi_rx_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &rx_mreq, sizeof(rx_mreq)) < 0)
        {
            perror("COP MULTI RX setsockopt");
            close(multi_rx_sock);
            multi_rx_sock = -1;
            return -1;
        }

        printf("< 1, RX MULTI CONNECTED:%s(SOCK:%d) > \n", cop_rx_addr_str, multi_rx_sock);
    } else {
        printf("< 1, CANNOT RX MULTI CONNECTED(ADDR:%d) > \n", rx_multi_addr);
        close_cop_multi_rx_sock();
    }

    tx_addr1 = (tx_multi_addr) & 0xff;
    tx_addr2 = (tx_multi_addr >> 8) & 0xff;
    tx_addr3 = (tx_multi_addr >> 16) & 0xff;
    tx_addr4 = (tx_multi_addr >> 24) & 0xff;

    sprintf(cop_tx_addr_str, "%d.%d.%d.%d", tx_addr1, tx_addr2, tx_addr3, tx_addr4); 

    ret = connect_call_multi_tx(cop_tx_addr_str);

    if (ret != 0) {
        perror("COP CAB2CAB CALL TX");
        close(multi_rx_sock);
        multi_rx_sock = -1;
    }

    if (ret == -1)
        return -1;

    return 0;
}

int connect_cop_cab2cab_call_multicast_2nd(char *buf, int len)
{
    int ret;
    struct ip_mreq rx_mreq;
    unsigned int rx_multi_addr;
    unsigned int tx_multi_addr;
    unsigned int mon_multi_addr;
    unsigned int tx_addr1, tx_addr2, tx_addr3, tx_addr4;
    int flags;

    rx_multi_addr = get_rx_mult_addr(buf, len);
    if ((rx_multi_addr & 0xff) != 234) {
        printf("%s(), RX Multi addr error: 0x%X\n", __func__, rx_multi_addr);
        return -1;
    }

    tx_multi_addr = get_tx_mult_addr(buf, len);
    if ((tx_multi_addr & 0xff) != 234) {
        printf("%s(), TX Multi addr error: 0x%X\n", __func__, tx_multi_addr);
        return -1;
    }

    mon_multi_addr = get_mon_mult_addr(buf, len);

    cop_rx_addr_int = rx_multi_addr;
    cop_tx_addr_int_2nd = tx_multi_addr;
    cop_mon_addr_int = mon_multi_addr;

    if (rx_multi_addr) {
        bzero(&multi_rx_srv_2nd, sizeof(multi_rx_srv_2nd));
        multi_rx_srv_2nd.sin_addr.s_addr = rx_multi_addr;
        multi_rx_srv_2nd.sin_family = AF_INET;
        multi_rx_srv_2nd.sin_port = htons(COP_MULTI_PORT_NUMBER);

        strcpy(cop_rx_addr_str, inet_ntoa(multi_rx_srv_2nd.sin_addr));

        if ((multi_rx_sock_2nd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("COP MULTI RX, socket");
            return -1;
        }

        flags = fcntl(multi_rx_sock_2nd, F_GETFL);
        ret = fcntl(multi_rx_sock_2nd, F_SETFL, flags | O_NONBLOCK);
        if (ret == -1) {
            printf("< rx_sock FCNT ERROR >\n");
            return -1;
        }

        if (bind(multi_rx_sock_2nd, (struct sockaddr *)&multi_rx_srv_2nd, sizeof(multi_rx_srv_2nd)) < 0)
        {
            perror("COP MULTI RX, bind");
            close(multi_rx_sock_2nd);
            multi_rx_sock_2nd = -1;
            return -1;
        }
        if (inet_aton(cop_rx_addr_str, &rx_mreq.imr_multiaddr) < 0) {
            perror("COP MULTI MON, inet_aton");
            close(multi_rx_sock_2nd);
            multi_rx_sock_2nd = -1;
            return -1;
        }

        rx_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(multi_rx_sock_2nd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &rx_mreq, sizeof(rx_mreq)) < 0)
        {
            perror("COP MULTI RX setsockopt");
            close(multi_rx_sock_2nd);
            multi_rx_sock_2nd = -1;
            return -1;
        }

        printf("< 1, RX MULTI CONNECTED:%s(SOCK:%d) > \n", cop_rx_addr_str, multi_rx_sock_2nd);
    } else {
        printf("< 1, CANNOT RX MULTI CONNECTED(ADDR:%d) > \n", rx_multi_addr);
        close_cop_multi_rx_sock_2nd();
    }

    tx_addr1 = (tx_multi_addr) & 0xff;
    tx_addr2 = (tx_multi_addr >> 8) & 0xff;
    tx_addr3 = (tx_multi_addr >> 16) & 0xff;
    tx_addr4 = (tx_multi_addr >> 24) & 0xff;

    sprintf(cop_tx_addr_str_2nd, "%d.%d.%d.%d", tx_addr1, tx_addr2, tx_addr3, tx_addr4);

    ret = connect_call_multi_tx_2nd(cop_tx_addr_str_2nd);

    if (ret != 0) {
        perror("COP CAB2CAB CALL TX");
        close(multi_rx_sock_2nd);
        multi_rx_sock_2nd = -1;
    }

    if (ret == -1)
        return -1;

    return 0;
}

int connect_cop_multi_monitoring(char *buf, int len, int *get_mon_addr, int force_set_addr)
{
    int ret;
    struct ip_mreq mon_mreq;
    unsigned int rx_multi_addr = 0;
    unsigned int tx_multi_addr = 0;
    unsigned int mon_multi_addr;
	int	ttl = MULTICAST_SEND_TTL;
    int flags;

    if (!force_set_addr && buf && len) {
        rx_multi_addr = get_rx_mult_addr(buf, len);
        tx_multi_addr = get_tx_mult_addr(buf, len);
        mon_multi_addr = get_mon_mult_addr(buf, len);
        if ((mon_multi_addr & 0xff) != 234) {
            printf("%s(), MON Multi addr error: 0x%X\n", __func__, mon_multi_addr);
            multi_mon_sock = -1;
            return -1;
        }
    }
    else
        mon_multi_addr = force_set_addr;

    if (!force_set_addr && get_mon_addr != NULL) {
        *get_mon_addr = mon_multi_addr;
    }

    if (!force_set_addr) {
        cop_rx_addr_int = rx_multi_addr;
        cop_tx_addr_int = tx_multi_addr;
    }
    cop_mon_addr_int = mon_multi_addr;

    bzero(&cop_multi_mon_srv, sizeof(cop_multi_mon_srv));
    cop_multi_mon_srv.sin_addr.s_addr = mon_multi_addr;
    cop_multi_mon_srv.sin_family = AF_INET;
    cop_multi_mon_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

    strcpy(cop_mon_addr_str, inet_ntoa(cop_multi_mon_srv.sin_addr));


    if ((multi_mon_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("COP MULTI MON, socket");
        return -1;
    }

    flags = fcntl(multi_mon_sock, F_GETFL);
    ret = fcntl(multi_mon_sock, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< mon_sock FCNT ERROR >\n");
        return -1;
    }

    if (bind(multi_mon_sock, (struct sockaddr *)&cop_multi_mon_srv, sizeof(cop_multi_mon_srv)) < 0)
    {
        printf("< BIND ERROR ADDR:0x%X >\n", mon_multi_addr);
        perror("COP MULTI MON, bind");
        close(multi_mon_sock);
        multi_mon_sock = -1;
        return -1;
    }

    if (inet_aton(cop_mon_addr_str, &mon_mreq.imr_multiaddr) < 0) {
        perror("COP MULTI MON, inet_aton");
        close(multi_mon_sock);
        multi_mon_sock = -1;
        return -1;
    }

	if (setsockopt(multi_mon_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
	{
        perror("COP MULTI MON setsockopt IP_MULTICAST_TTL");
        close(multi_mon_sock);
        multi_mon_sock = -1;
        return -1;
	}

    mon_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    ret = setsockopt(multi_mon_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mon_mreq, sizeof(mon_mreq));
    if (ret < 0) {
        perror("COP MULTI MON setsockopt IP_ADD_MEMBERSHIP");
        close(multi_mon_sock);
        multi_mon_sock = -1;
        return -1;
    }

    printf("< MON MULTI CONNECTED:%s(SOCK:%d) > \n", cop_mon_addr_str, multi_mon_sock);

    return 0;
}

int connect_monitoring_bcast_multicast(int mon_type)
{
    struct ip_mreq bcast_mon_mreq;
    int flags, ret;
    char *ipaddr;

    if ((mon_type < 0) || (mon_type >  MON_BCAST_TYPE_MAX)) {
        return -1;
    }

    ipaddr = __monitoring_multi_addr_str[mon_type];

    bzero(&bcast_mon_srv, sizeof(bcast_mon_srv));
    bcast_mon_srv.sin_addr.s_addr = inet_addr(ipaddr);
    bcast_mon_srv.sin_family = AF_INET;
    bcast_mon_srv.sin_port = htons(COP_MULTI_PORT_NUMBER);

    if ((bcast_mon_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("PASSIVE/FUNC/AUTO MON BCAST socket");
        return -1;
    }

    flags = fcntl(bcast_mon_sock, F_GETFL);
    ret = fcntl(bcast_mon_sock, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        printf("< PASSIVE/FUNC/AUTO BCAST FCNTL ERROR >\n");
        return -1;
    }

    if (bind(bcast_mon_sock, (struct sockaddr *)&bcast_mon_srv, sizeof(bcast_mon_srv)) < 0) {
        perror("PASSIVE/FUNC/AUTO BCAST bind");
        bcast_mon_sock = -1;
        close(bcast_mon_sock);
        return -1;
    }

    if (inet_aton(ipaddr, &bcast_mon_mreq.imr_multiaddr) < 0) {
        perror("PASSIVE/FUNC/AUTO BCAST inet_aton");
        bcast_mon_sock = -1;
        close(bcast_mon_sock);
        return -1;
    }

    bcast_mon_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(bcast_mon_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                &bcast_mon_mreq, sizeof(bcast_mon_mreq)) < 0)
    {
        perror("PASSIVE/FUNC/AUTO BCAST setsockopt");
        bcast_mon_sock = -1;
        close(bcast_mon_sock);
        return -1;
    }

    printf("< CONNECT %s, SOCK:%d >\n", ipaddr, bcast_mon_sock);

    return bcast_mon_sock;
}

int close_monitoring_bcast_multicast(int mon_type)
{
    if (bcast_mon_sock < 0)
        return -1;

    //printf("< CLOSE %s MON SOCK(%d) >\n",
    //        __monitoring_multi_comment[mon_type], bcast_mon_sock);
    close(bcast_mon_sock);

    bcast_mon_sock = -1;

    return 0;
}

void close_cop_multi_monitoring(void)
{
    if (multi_mon_sock < 0)
        return;

    cop_mon_addr_int = 0;

    //printf("< CLOSE MON SOCK(%d) >\n", multi_mon_sock);
    close(multi_mon_sock);

    multi_mon_sock = -1;
}

static int try_read_cop_multicast_data(int sock, char *buf, int maxlen)
{
    //fd_set rd_set;
    int rx_len = 0;
    //int numFds;
    //struct timeval timeout;
    //struct sockaddr multi_cli;
    //socklen_t multi_addrsize;
    //int ret = -1;

    if (sock < 0)
        return -1;

#if 0
    multi_addrsize = sizeof(multi_cli);

    rx_len = 0;
    FD_ZERO(&rd_set);
    FD_SET(sock, &rd_set);
    numFds = sock + 1;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    ret = select(numFds, &rd_set, NULL, NULL, &timeout);
    if (ret <= 0)
        return -1;

    rx_len = recvfrom(sock, buf, maxlen, 0, (struct sockaddr *) &multi_cli, &multi_addrsize);
    //SrverIP = multi_cli.sin_addr.s_addr;

#else
    errno = 0;
    rx_len = recv(sock, buf, maxlen, 0);
    if (rx_len == -1) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            printf("<< SOCK %d RECV ERROR, errno:%d >>\n", sock, errno);
        }
    }
#endif

    return rx_len;
}

#define MON_MAX_BUF_N	10
#define MON_LIMIT_RTP_BUF_N	3
static char __mon_rxbuf[MON_MAX_BUF_N][MULTI_RX_BUF_LEN];
static char __mon_return_buf[MULTI_RX_BUF_LEN];
static int __mon_rxlen[MON_MAX_BUF_N];
static int mon_inptr = 0;
static int mon_outptr = 0;
static int mon_rxctr = 0;
static int mon_start_get_buf = 0;

char *try_read_call_monitoring_data(int *len, int stop, int use)
{
    int l;
    char *buf, *return_buf = NULL;
    int limit_ctr;

    buf = &__mon_rxbuf[mon_inptr][0];
    l = try_read_cop_multicast_data(multi_mon_sock, buf, MULTI_RX_BUF_LEN);

    if (l > 0 && mon_rxctr >= MON_MAX_BUF_N) {
        __mon_rxlen[mon_inptr] = l;

        mon_outptr++;
        if (mon_outptr == MON_MAX_BUF_N) mon_outptr = 0;

        mon_inptr++;
        if (mon_inptr == MON_MAX_BUF_N) mon_inptr = 0;

        l = 0;
    }

    if (!use) {
        *len = 0;
        return return_buf;
    }

    if (stop) {/*l = 0;*/ limit_ctr = 1; }
    else limit_ctr = 2;

    *len = 0;

    if (mon_start_get_buf && mon_rxctr >= limit_ctr) {
        return_buf = &__mon_return_buf[0];

        *len = __mon_rxlen[mon_outptr];
        buf = &__mon_rxbuf[mon_outptr][0];
        memcpy(return_buf, buf, *len);
        memset(buf, 0, 20);

        mon_rxctr--;

        mon_outptr++;
        if (mon_outptr == MON_MAX_BUF_N) mon_outptr = 0;
    }

    if (l > 0) {
//printf("[IN DATA- %d, %d] %02X %02X, \n", mon_inptr, l, buf[2], buf[3]);

        if (mon_rxctr < MON_MAX_BUF_N) {
            __mon_rxlen[mon_inptr] = l;
            mon_rxctr++;

            mon_inptr++;
            if (mon_inptr == MON_MAX_BUF_N) mon_inptr = 0;

            if (!mon_start_get_buf && mon_rxctr >= MON_LIMIT_RTP_BUF_N) {
                mon_start_get_buf = 1;
                printf("< READY-MON >\n");
            }
        }
        else
            printf("[ FAIL INSERT, MON - %d, %d ] %d\n", mon_inptr, l, buf[2]<<8 | buf[3]);

    }

//if (*len)
//      printf("[OUTDATA - %d, %d] %02X %02X\n", mon_outptr, *len, return_buf[2], return_buf[3]);

    return return_buf;
}

int get_mon_multi_rx_ctr(void)
{
    return mon_rxctr;
}

void reset_mon_multi_rx_data_ptr(void)
{
    mon_inptr = 0;
    mon_outptr = 0;
    mon_rxctr = 0;
    mon_start_get_buf = 0;
}

#define CALL_MAX_BUF_N	10
#define CALL_LIMIT_RTP_BUF_N	3
static char __call_rxbuf[CALL_MAX_BUF_N][MULTI_RX_BUF_LEN];
static char __call_return_buf[MULTI_RX_BUF_LEN];
static int __call_rxlen[CALL_MAX_BUF_N];
static int call_inptr = 0;
static int call_outptr = 0;
static int call_rxctr = 0;
static int call_start_get_buf = 0;

char *try_read_call_rx_data(int *len, int stop, int use)
{
    int l;
    char *buf, *return_buf = NULL;
    int limit_ctr;

    buf = &__call_rxbuf[call_inptr][0];
    l = try_read_cop_multicast_data(multi_rx_sock, buf, MULTI_RX_BUF_LEN);

    if (l > 0 && call_rxctr >= CALL_MAX_BUF_N) {
        __call_rxlen[call_inptr] = l;

        call_outptr++;
        if (call_outptr == CALL_MAX_BUF_N) call_outptr = 0;

        call_inptr++;
        if (call_inptr == CALL_MAX_BUF_N) call_inptr = 0;

        l = 0;
    }

    if (!use) {
        *len = 0;
        return return_buf;
    }

    if (stop) {/*l = 0;*/ limit_ctr = 1; }
    else limit_ctr = 2;

    *len = 0;

    if (call_start_get_buf && call_rxctr >= limit_ctr) {
        return_buf = &__call_return_buf[0];

        *len = __call_rxlen[call_outptr];
        buf = &__call_rxbuf[call_outptr][0];

		return_buf = buf;
        //memcpy(return_buf, buf, *len);
        //memset(buf, 0, 20);

        call_rxctr--;

        call_outptr++;
        if (call_outptr == CALL_MAX_BUF_N) call_outptr = 0;
    }

    if (l > 0) {
//printf("[IN DATA- %d, %d] %02X %02X, \n", call_inptr, l, buf[2], buf[3]);
        if (call_rxctr < CALL_MAX_BUF_N) {
            __call_rxlen[call_inptr] = l;
            call_rxctr++;

            call_inptr++;
            if (call_inptr == CALL_MAX_BUF_N) call_inptr = 0;

            if (!call_start_get_buf && call_rxctr >= CALL_LIMIT_RTP_BUF_N) {
                call_start_get_buf = 1;
                printf("< READY-CALL >\n");
            }
        } else
            printf("[ FAIL INSERT, CALL - %d, %d ] %d\n", call_inptr, l, buf[2]<<8 | buf[3]);
    }

	//if (*len)
	//	printf("[1st OUTDATA - %d, %d] %02X %02X\n", call_outptr, *len, return_buf[2], return_buf[3]);

    return return_buf;

}

int get_call_multi_rx_ctr(void)
{
    return call_rxctr;
}

void reset_call_multi_rx_data_ptr(void)
{
    call_inptr = 0;
    call_outptr = 0;
    call_rxctr = 0;
    call_start_get_buf = 0;
}

char __call_rxbuf_2nd[CALL_MAX_BUF_N][MULTI_RX_BUF_LEN];
char __call_return_buf_2nd[MULTI_RX_BUF_LEN];
int __call_rxlen_2nd[CALL_MAX_BUF_N];
static int call_inptr_2nd = 0; 
static int call_outptr_2nd = 0; 
static int call_rxctr_2nd = 0; 
static int call_start_get_buf_2nd = 0; 

#if 0
char call_1st_g711_buf[172*2];
char call_2nd_g711_buf[172*2];


char *try_read_call_rx_data_1st(int *len, int stop, int use)
{
	int l;
	char *buf, *return_buf = NULL;

    if (!use) {
        *len = 0; 
        return return_buf;
    }

	buf = &call_1st_g711_buf[0];
	l = try_read_cop_multicast_data(multi_rx_sock, buf, MULTI_RX_BUF_LEN);
	
	if(l>0) {
		*len = l;
		return buf;
	} else {
		*len = 0;
		return return_buf;
	}
}

char *try_read_call_rx_data_2nd(int *len, int stop, int use)
{
	int l;
	char *buf, *return_buf = NULL;

    if (!use) {
        *len = 0; 
        return return_buf;
    }

	buf = &call_2nd_g711_buf[0];
	l = try_read_cop_multicast_data(multi_rx_sock_2nd, buf, MULTI_RX_BUF_LEN);
	
	if(l>0) {
		*len = l;
		return buf;
	} else {
		*len = 0;
		return return_buf;
	}
}

#else
char *try_read_call_rx_data_2nd(int *len, int stop, int use) 
{
    int l;
    char *buf, *return_buf = NULL;
    int limit_ctr;

    buf = &__call_rxbuf_2nd[call_inptr_2nd][0];
    l = try_read_cop_multicast_data(multi_rx_sock_2nd, buf, MULTI_RX_BUF_LEN);

    if (l > 0 && call_rxctr_2nd >= CALL_MAX_BUF_N) {
        __call_rxlen_2nd[call_inptr_2nd] = l; 

        call_outptr_2nd++;
        if (call_outptr_2nd == CALL_MAX_BUF_N) call_outptr_2nd = 0; 

        call_inptr_2nd++;
        if (call_inptr_2nd == CALL_MAX_BUF_N) call_inptr_2nd = 0; 

        l = 0; 
    }    

    if (!use) {
        *len = 0; 
        return return_buf;
    }    

    if (stop)
		limit_ctr = 1; 
    else
		limit_ctr = 2; 

    *len = 0; 

    if (call_start_get_buf_2nd && call_rxctr_2nd >= limit_ctr) {
        return_buf = &__call_return_buf_2nd[0];

        *len = __call_rxlen_2nd[call_outptr_2nd];
        buf = &__call_rxbuf_2nd[call_outptr_2nd][0];

		return_buf = buf;
        //memcpy(return_buf, buf, *len);
        //memset(buf, 0, 20);

        call_rxctr_2nd--;

        call_outptr_2nd++;
        if (call_outptr_2nd == CALL_MAX_BUF_N) call_outptr_2nd = 0;
    }

    if (l > 0) {
//printf("[IN DATA- %d, %d] %02X %02X, \n", call_inptr_2nd, l, buf[2], buf[3]);
        if (call_rxctr_2nd < CALL_MAX_BUF_N) {
            __call_rxlen_2nd[call_inptr_2nd] = l;
            call_rxctr_2nd++;

            call_inptr_2nd++;
            if (call_inptr_2nd == CALL_MAX_BUF_N) call_inptr_2nd = 0;

            if (!call_start_get_buf_2nd && call_rxctr_2nd >= CALL_LIMIT_RTP_BUF_N) {
                call_start_get_buf_2nd = 1;
                printf("< READY-CALL 2nd(Mixing) >\n");
            }
        } else
            printf("[ FAIL INSERT, CALL - %d, %d ] %d\n", call_inptr_2nd, l, buf[2]<<8 | buf[3]);
    }

	//if (*len)
	//	printf("[2nd OUTDATA - %d, %d] %02X %02X\n", call_outptr_2nd, *len, return_buf[2], return_buf[3]);

    return return_buf;

}
#endif

int get_call_multi_rx_ctr_2nd(void)
{
    return call_rxctr_2nd;
}

void reset_call_multi_rx_data_ptr_2nd(void)
{
    call_inptr_2nd = 0;
    call_outptr_2nd = 0;
    call_rxctr_2nd = 0;
    call_start_get_buf_2nd = 0;
}

/**** Audio Mixing on here ****/
char call_rxbuf[172*10];
char rtp_header[15];
static char AlltoAVCBuf[30];

char *mixing_two_call(char *buf1, int len, char *buf2, int len2, int id)
{
    char *return_buf = NULL;
    char *buffer_1, *buffer_2, *buffer_mix;
    short linear_1 = 0;
    short linear_2 = 0;
    short linear_mix = 0;
    int i=12;

    //memcpy(&rtp_header[0], &buf1[0], 12);

    //buffer_1 = &buf1[12];
    //buffer_2 = &buf2[12];

    //buffer_mix = &call_rxbuf[12];
#if 0
	{
		int loop;
		for(loop=0; loop<12; loop++)
			call_rxbuf[loop] = buf1[loop];
	}
#else
    //send_all2avc_cycle_data(AlltoAVCBuf, 8, 0, 0, 0);
    //memcpy(&call_rxbuf[0], &buf1[0], 12);
#endif

    while(i < len) {
        /* Decoding */
        //linear_1 = alaw2linear(buffer_1[i]);
        //linear_2 = alaw2linear(buffer_2[i]);
        linear_1 = alaw2linear(buf1[i]);
        linear_2 = alaw2linear(buf2[i]);

        /* Mixing & Encoding */
        linear_mix = (linear_1 + linear_2) / 2;
        //buffer_mix[i] = (char)linear2alaw(linear_mix);
		buf1[i] = linear2alaw(linear_mix);
		//printf("[%d] %X:%X \n", i, buffer_mix[i], (char)linear2alaw(linear_mix));

        i++;
    }
	//printf("\n");

	return buf1;
	return_buf = &call_rxbuf[0];

	//DBG_LOG("Mixing completed....\n");
    return return_buf;
}

static char __occ_pa_rxbuf[MAX_RTP_BUF_N][MULTI_RX_BUF_LEN];
static char __occ_pa_return_buf[MULTI_RX_BUF_LEN];
static int __occ_pa_rxlen[MAX_RTP_BUF_N];
static int occ_pa_inptr = 0;
static int occ_pa_outptr = 0;
static int occ_pa_rxctr = 0;
static int occ_pa_start_get_buf = 0;

char *try_read_occ_pa_monitoring_bcast_rx_data(int *len, int stop, int use, int play)
{
    int l = 0;
    char *buf = 0, *return_buf = NULL;
    int limit_ctr;

    buf = &__occ_pa_rxbuf[occ_pa_inptr][0];
    l = try_read_cop_multicast_data(occ_pa_mon_sock, buf, MULTI_RX_BUF_LEN);

    if (l > 0 && occ_pa_rxctr >= MAX_RTP_BUF_N) {
        /*sss  = buf[11] << 24;
        sss |= buf[10] << 16;
        sss |= buf[9]  << 8;
        sss |= buf[8];
        */

        __occ_pa_rxlen[occ_pa_inptr] = l;

        occ_pa_outptr++;
        if (occ_pa_outptr == MAX_RTP_BUF_N) occ_pa_outptr = 0;

        occ_pa_inptr++;
        if (occ_pa_inptr == MAX_RTP_BUF_N) occ_pa_inptr = 0;

        l = 0;
    }

    if (!use) {
        *len = 0;
        return NULL;
    }

    if (stop) { /*l = 0;*/ limit_ctr = 1; }
    else limit_ctr = 2;

    *len = 0;

    /*if (l > 0) {
        sss  = buf[11] << 24;
        sss |= buf[10] << 16;
        sss |= buf[9]  << 8;
        sss |= buf[8];
    }*/

    if (play && occ_pa_start_get_buf && occ_pa_rxctr >= limit_ctr)
    {
        return_buf = &__occ_pa_return_buf[0];

        *len = __occ_pa_rxlen[occ_pa_outptr];
        buf = &__occ_pa_rxbuf[occ_pa_outptr][0];
        memcpy(return_buf, buf, *len);
        memset(buf, 0, 20);

        occ_pa_rxctr--;
#if 0
sss  = return_buf[11] << 24;
sss |= return_buf[10] << 16;
sss |= return_buf[9]  << 8;
sss |= return_buf[8];
printf("[OUT OCC-PA DATA- outptr: %d, rxctr:%d], SSRC:%08X, SEQ:%d\n", occ_pa_outptr, occ_pa_rxctr, sss, return_buf[2]<<8|return_buf[3]);
#endif

        occ_pa_outptr++;
        if (occ_pa_outptr == MAX_RTP_BUF_N) occ_pa_outptr = 0;
    }

    if (l > 0) {
        if (occ_pa_rxctr < MAX_RTP_BUF_N) {
            __occ_pa_rxlen[occ_pa_inptr] = l;
            occ_pa_rxctr++;

//printf("[IN OCC-PA DATA- %d, %d] %02X %02X, \n", occ_pa_inptr, l, buf[2], buf[3]);
            occ_pa_inptr++;
            if (occ_pa_inptr == MAX_RTP_BUF_N) occ_pa_inptr = 0;

            if (!occ_pa_start_get_buf && occ_pa_rxctr >= PLAY_PA_LIMIT_RTP_BUF_N) {
                occ_pa_start_get_buf  = 1;
                printf("< READY-OCC-PA >\n");
            }
        }
        else
            printf("[FAIL INSERT, OCC-PA - %d, %d] %d\n", occ_pa_inptr, l, buf[2]<<8 | buf[3]);
    }

//if (*len)
//      printf("[OUTDATA - %d, %d] %02X %02X\n", occ_pa_outptr, *len, return_buf[2], return_buf[3]);

    return return_buf;
}

int get_next_occ_pa_monitoring_data_ssrc(void)
{
     int ssrc = 0;
     int outptr;
     char *buf;

     if (occ_pa_rxctr > 0) {
        outptr = occ_pa_outptr;
        buf =  &__occ_pa_rxbuf[outptr][0];

        ssrc  = buf[8] << 24;
        ssrc |= buf[9] << 16;
        ssrc |= buf[10]  << 8;
        ssrc |= buf[11];
     }

     return ssrc;
}

unsigned short get_next_occ_pa_monitoring_data_seq(void)
{
     unsigned short seq = 0;
     int outptr;
     char *buf;

     if (occ_pa_rxctr > 0) {
        outptr = occ_pa_outptr;
        buf =  &__occ_pa_rxbuf[outptr][0];
        seq = buf[2] << 8 | buf[3];
     }

     return seq;
}

int get_occ_pa_multi_rx_ctr(void)
{
    return occ_pa_rxctr;
}

void reset_occ_pa_multi_rx_data_ptr(void)
{
    int i;

    occ_pa_inptr = 0;
    occ_pa_outptr = 0;
    occ_pa_rxctr = 0;
    occ_pa_start_get_buf = 0;

    for (i = 0; i < MAX_RTP_BUF_N; i++)
        memset(&__occ_pa_rxbuf[i][0], 0, 12);

    if (!unlock_func_rtp_recv) unlock_func_rtp_recv = 1;
    if (!unlock_fire_rtp_recv) unlock_fire_rtp_recv = 1;
    if (!unlock_auto_rtp_recv) unlock_auto_rtp_recv = 1;

    printf("< RST BUF OCC-PA >\n");
}

void reset_occ_pa_multi_rx_start_get_buf(void)
{
    occ_pa_start_get_buf = 0;
//printf("== %s()==\n", __func__);
}

static char __cop_pa_rxbuf[MAX_RTP_BUF_N][MULTI_RX_BUF_LEN];
static char __cop_pa_return_buf[MULTI_RX_BUF_LEN];
static int __cop_pa_rxlen[MAX_RTP_BUF_N];
static int cop_pa_inptr = 0;
static int cop_pa_outptr = 0;
static int cop_pa_rxctr = 0;
static int cop_pa_start_get_buf = 0;

char *try_read_cop_pa_monitoring_bcast_rx_data(int *len, int stop, int use, int play)
{
    int l = 0;
    char *buf = 0, *return_buf = NULL;
    int limit_ctr;
    //unsigned int sss;

    buf = &__cop_pa_rxbuf[cop_pa_inptr][0];
    l = try_read_cop_multicast_data(cop_pa_mon_sock, buf, MULTI_RX_BUF_LEN);

    if (l > 0 && cop_pa_rxctr >= MAX_RTP_BUF_N) {
        /*sss  = buf[11] << 24;
        sss |= buf[10] << 16;
        sss |= buf[9]  << 8;
        sss |= buf[8];
        */

        __cop_pa_rxlen[cop_pa_inptr] = l;

        cop_pa_outptr++;
        if (cop_pa_outptr == MAX_RTP_BUF_N) cop_pa_outptr = 0;

        cop_pa_inptr++;
        if (cop_pa_inptr == MAX_RTP_BUF_N) cop_pa_inptr = 0;

        l = 0;
    }

    if (!use) {
        *len = 0;
        return return_buf;
    }

    if (stop) { /*l = 0;*/ limit_ctr = 1; }
    else limit_ctr = 2;

    *len = 0;

    /*if (l > 0) {
        sss  = buf[11] << 24;
        sss |= buf[10] << 16;
        sss |= buf[9]  << 8;
        sss |= buf[8];
    }*/

    /*if (l > 0 && ssrc) {
        //sss  = buf[11] << 24;
        //sss |= buf[10] << 16;
        //sss |= buf[9]  << 8;
        //sss |= buf[8];
        if (sss != ssrc) l = 0; // ignore packet
    }*/

    if (play && cop_pa_start_get_buf && cop_pa_rxctr >= limit_ctr)
    {
        return_buf = &__cop_pa_return_buf[0];

        *len = __cop_pa_rxlen[cop_pa_outptr];
        buf = &__cop_pa_rxbuf[cop_pa_outptr][0];
        memcpy(return_buf, buf, *len);
        memset(buf, 0, 20);

        cop_pa_rxctr--;

#if 0
sss  = return_buf[11] << 24;
sss |= return_buf[10] << 16;
sss |= return_buf[9]  << 8;
sss |= return_buf[8];
printf("[OUT CAB-PA DATA- outptr: %d, rxctr:%d], SSRC:%08X, SEQ:%d\n", cop_pa_outptr, cop_pa_rxctr, sss, return_buf[2]<<8|return_buf[3]);
#endif

        cop_pa_outptr++;
        if (cop_pa_outptr == MAX_RTP_BUF_N) cop_pa_outptr = 0;
    }

    if (l > 0) {
        if (cop_pa_rxctr < MAX_RTP_BUF_N) {
            __cop_pa_rxlen[cop_pa_inptr] = l;
            cop_pa_rxctr++;
//printf("[IN CAB-PA DATA- inptr: %d, rxctr:%d, l:%d] SSRC:%08X, SEQ: %d\n", cop_pa_inptr, cop_pa_rxctr, l, sss, buf[2] <<8 | buf[3]);

            cop_pa_inptr++;
            if (cop_pa_inptr == MAX_RTP_BUF_N) cop_pa_inptr = 0;

            if (!cop_pa_start_get_buf && cop_pa_rxctr >= PLAY_PA_LIMIT_RTP_BUF_N) {
                cop_pa_start_get_buf = 1;
                printf("< READY-CAB-PA >\n");
            }
        }
        else
            printf("[FAIL INSERT, COP-PA - %d, %d] %d\n", cop_pa_inptr, l, buf[2]<<8 | buf[3]);
    }

//if (*len)
//      printf("[OUTDATA - %d, %d] %02X %02X\n", cop_pa_outptr, *len, return_buf[2], return_buf[3]);

    return return_buf;

}

int get_next_cop_pa_monitoring_data_ssrc(void)
{
     int ssrc = 0;
     int outptr;
     char *buf;

     if (cop_pa_rxctr > 0) {
        outptr = cop_pa_outptr;
        buf =  &__cop_pa_rxbuf[outptr][0];

        ssrc  = buf[8] << 24;
        ssrc |= buf[9] << 16;
        ssrc |= buf[10]  << 8;
        ssrc |= buf[11];
     }
     return ssrc;
}

unsigned short get_next_cop_pa_monitoring_data_seq(void)
{
     unsigned short seq = 0;
     int outptr;
     char *buf;

     if (cop_pa_rxctr > 0) {
        outptr = cop_pa_outptr;
        buf =  &__cop_pa_rxbuf[outptr][0];
        seq = buf[2] << 8 | buf[3];
     }

     return seq;
}

int get_cop_pa_multi_rx_ctr(void)
{
    return cop_pa_rxctr;
}

void reset_cop_pa_multi_rx_data_ptr(void)
{
    int i;

    cop_pa_inptr = 0;
    cop_pa_outptr = 0;
    cop_pa_rxctr = 0;
    cop_pa_start_get_buf = 0;

    for (i = 0; i < MAX_RTP_BUF_N; i++)
        memset(&__cop_pa_rxbuf[i][0], 0, 12);

    if (!unlock_func_rtp_recv) unlock_func_rtp_recv = 1;
    if (!unlock_fire_rtp_recv) unlock_fire_rtp_recv = 1;
    if (!unlock_auto_rtp_recv) unlock_auto_rtp_recv = 1;

    printf("< RST BUF CAB-PA >\n");
}

void reset_cop_pa_multi_rx_start_get_buf(void)
{
    cop_pa_start_get_buf = 0;
//printf("== %s()==\n", __func__);
}

static char __auto_rxbuf[MAX_RTP_BUF_N][MULTI_RX_BUF_LEN];
static char __auto_return_buf[MULTI_RX_BUF_LEN];
static int __auto_rxlen[MAX_RTP_BUF_N];
static int auto_inptr = 0;
static int auto_outptr = 0;
static int auto_rxctr = 0;
static int auto_start_get_buf = 0;

char *try_read_auto_monitoring_bcast_rx_data(int *len, int stop, int use, int play)
{
    int l = 0;
    char *buf = NULL, *return_buf = NULL;
    int limit_ctr;
    unsigned short seq = 0;
//unsigned int sss = 0;

    buf = &__auto_rxbuf[auto_inptr][0];
    l = try_read_cop_multicast_data(auto_mon_sock, buf, MULTI_RX_BUF_LEN);

#ifdef CHECK_RTP_START_SEQ_ONE
    if (l > 0 && !unlock_auto_rtp_recv) {
        seq = (buf[2] << 8) + buf[3];
        if (seq <= RTP_SEQ_LIMIT_LOW || seq >= RTP_SEQ_LIMIT_HIGH) {
            unlock_auto_rtp_recv = 1;
            printf("< UNLOCK AUTO RTP(%d) >\n", seq);
        } else {
            *len = 0;
            return NULL;
        }
    }
#endif

    if (l > 0 && auto_rxctr >= MAX_RTP_BUF_N) {
        __auto_rxlen[auto_inptr] = l;

        auto_outptr++;
        if (auto_outptr == MAX_RTP_BUF_N) auto_outptr = 0;

#if 0
		sss  = buf[11] << 24;
		sss |= buf[10] << 16;
		sss |= buf[9]  << 8;
		sss |= buf[8];
		printf("[IN AUTO FULL - in: %d, out:%d, ctr:%d, l:%d] SSRC:%08X, SEQ: %d\n", auto_inptr, auto_outptr, auto_rxctr, l, sss, buf[2] <<8 | buf[3]);
#endif

        auto_inptr++;
        if (auto_inptr == MAX_RTP_BUF_N) auto_inptr = 0;

        l = 0;
    }
#if 0
    else {
		if (l > 0) {
			sss  = buf[8] << 24;
			sss |= buf[9] << 16;
			sss |= buf[10] << 8;
			sss |= buf[11];
			printf("2. IN AUTO - l:%d] SSRC:%08X, SEQ: %d\n", l, sss, buf[2] <<8 | buf[3]);
		}
    }
#endif

#if 0
	if (l > 0) {
		printf("3. IN AUTO - l:%d] SSRC:%08X, SEQ: %d\n", l, sss, buf[2] <<8 | buf[3]);
	}
#endif

    if (!use) {
        *len = 0;
        return return_buf;
    }

    if (stop) { /*l = 0;*/ limit_ctr = 1; }
    else limit_ctr = 2;

    *len = 0;

    /*if (l) {
        sss  = buf[11] << 24;
        sss |= buf[10] << 16;
        sss |= buf[9]  << 8;
        sss |= buf[8];
    }*/

/*	if (l > 0) {
		printf("4. IN AUTO - l:%d] SSRC:%08X, SEQ: %d\n", l, sss, buf[2] <<8 | buf[3]);
	}*/

    if (play && auto_start_get_buf && auto_rxctr >= limit_ctr) {
        return_buf = &__auto_return_buf[0];

        *len = __auto_rxlen[auto_outptr];
        buf = &__auto_rxbuf[auto_outptr][0];
        memcpy(return_buf, buf, *len);
        memset(buf, 0, 20);

        auto_rxctr--;

#if 0
		sss  = return_buf[8] << 24;
		sss |= return_buf[9] << 16;
		sss |= return_buf[10]  << 8;
		sss |= return_buf[11];
		printf("[OUT AUTO - out: %d, ctr:%d], SSRC:%08X, SEQ:%d\n", auto_outptr, auto_rxctr, sss, return_buf[2]<<8|return_buf[3]);
#endif
        auto_outptr++;
        if (auto_outptr == MAX_RTP_BUF_N) auto_outptr = 0;
    }

    if (l > 0) {
        if (auto_rxctr < MAX_RTP_BUF_N) {
            __auto_rxlen[auto_inptr] = l;
            auto_rxctr++;

//printf("[IN AUTO - in: %d, ctr:%d, l:%d] SSRC:%08X, SEQ: %d\n", auto_inptr, auto_rxctr, l, sss, buf[2] <<8 | buf[3]);
            auto_inptr++;
            if (auto_inptr == MAX_RTP_BUF_N) auto_inptr = 0;

            if (!auto_start_get_buf && auto_rxctr >= PLAY_AUTO_FUNC_LIMIT_RTP_BUF_N) {
                auto_start_get_buf = 1;
                printf("< READY-AUTO >\n");
            }
        }
        else
            printf("[FAIL INSERT, AUTO - %d, %d] %d, \n", auto_inptr, l, (buf[2]<<8) | buf[3]);
    }

    //if (auto_rxctr == MAX_BUF_N)
//if (*len)
//      printf("[OUTDATA - %d, %d] %02X %02X\n", auto_outptr, *len, return_buf[2], return_buf[3]);

    return return_buf;
}

int get_next_auto_monitoring_data_ssrc(void)
{
     int ssrc = 0;
     int outptr;
     char *buf;

     if (auto_rxctr > 0) {
        outptr = auto_outptr;

        buf =  &__auto_rxbuf[outptr][0];
        ssrc  = buf[8] << 24;
        ssrc |= buf[9] << 16;
        ssrc |= buf[10]  << 8;
        ssrc |= buf[11];
     }

     return ssrc;
}

unsigned short get_next_auto_monitoring_data_seq(void)
{
     unsigned short seq = 0;
     int outptr;
     char *buf;

     if (auto_rxctr > 0) {
        outptr = auto_outptr;

        buf =  &__auto_rxbuf[outptr][0];
        seq = buf[2] << 8 | buf[3];
     }

     return seq;
}

int get_auto_multi_rx_ctr(void)
{
    return auto_rxctr;
}

void reset_auto_multi_rx_data_ptr(void)
{
    int i;

    auto_inptr = 0;
    auto_outptr = 0;
    auto_rxctr = 0;
    auto_start_get_buf = 0;

    for (i = 0; i < MAX_RTP_BUF_N; i++)
        memset(&__auto_rxbuf[i][0], 0, 12);

    printf("< RST BUF AUTO >\n");
}

void reset_auto_multi_rx_start_get_buf(void)
{
    auto_start_get_buf = 0;
//printf("%s()==\n", __func__);
}


static char __fire_rxbuf[MAX_RTP_BUF_N][MULTI_RX_BUF_LEN];
static char __fire_return_buf[MULTI_RX_BUF_LEN];
static int __fire_rxlen[MAX_RTP_BUF_N];
static int fire_inptr = 0;
static int fire_outptr = 0;
static int fire_rxctr = 0;
static int fire_start_get_buf = 0;

char *try_read_fire_monitoring_bcast_rx_data(int *len, int stop, int use, int play)
{
    int l = 0;
    char *buf = NULL, *return_buf = NULL;
    int limit_ctr;
    unsigned short seq = 0;
//unsigned int sss = 0;

    buf = &__fire_rxbuf[fire_inptr][0];
    l = try_read_cop_multicast_data(fire_mon_sock, buf, MULTI_RX_BUF_LEN);

#ifdef CHECK_RTP_START_SEQ_ONE
    if (l > 0 && !unlock_fire_rtp_recv) {
        seq = (buf[2] << 8) + buf[3];
        if (seq <= RTP_SEQ_LIMIT_LOW || seq >= RTP_SEQ_LIMIT_HIGH) {
            unlock_fire_rtp_recv = 1;
            printf("< UNLOCK FIRE RTP(%d) >\n", seq);
        } else {
            *len = 0;
            return NULL;
        }
    }
#endif

    if (l > 0 && fire_rxctr >= MAX_RTP_BUF_N) {
        __fire_rxlen[fire_inptr] = l;

        fire_outptr++;
        if (fire_outptr == MAX_RTP_BUF_N) fire_outptr = 0;

#if 0
		sss  = buf[11] << 24;
		sss |= buf[10] << 16;
		sss |= buf[9]  << 8;
		sss |= buf[8];
		printf("[IN FIRE FULL - in: %d, out:%d, ctr:%d, l:%d] SSRC:%08X, SEQ: %d\n", fire_inptr, fire_outptr, fire_rxctr, l, sss, buf[2] <<8 | buf[3]);
#endif

        fire_inptr++;
        if (fire_inptr == MAX_RTP_BUF_N) fire_inptr = 0;

        l = 0;
    }
#if 0
    else {
		if (l > 0) {
			sss  = buf[8] << 24;
			sss |= buf[9] << 16;
			sss |= buf[10] << 8;
			sss |= buf[11];
			printf("2. IN AUTO - l:%d] SSRC:%08X, SEQ: %d\n", l, sss, buf[2] <<8 | buf[3]);
		}
    }
#endif

#if 0
	if (l > 0) {
		printf("3. IN AUTO - l:%d] SSRC:%08X, SEQ: %d\n", l, sss, buf[2] <<8 | buf[3]);
	}
#endif

    if (!use) {
        *len = 0;
        return return_buf;
    }

    if (stop) { /*l = 0;*/ limit_ctr = 1; }
    else limit_ctr = 2;

    *len = 0;

    /*if (l) {
        sss  = buf[11] << 24;
        sss |= buf[10] << 16;
        sss |= buf[9]  << 8;
        sss |= buf[8];
    }*/

/*	if (l > 0) {
		printf("4. IN FIRE - l:%d] SSRC:%08X, SEQ: %d\n", l, sss, buf[2] <<8 | buf[3]);
	}*/

    if (play && fire_start_get_buf && fire_rxctr >= limit_ctr) {
        return_buf = &__fire_return_buf[0];

        *len = __fire_rxlen[fire_outptr];
        buf = &__fire_rxbuf[fire_outptr][0];
        memcpy(return_buf, buf, *len);
        memset(buf, 0, 20);

        fire_rxctr--;

#if 0
		sss  = return_buf[8] << 24;
		sss |= return_buf[9] << 16;
		sss |= return_buf[10]  << 8;
		sss |= return_buf[11];
		printf("[OUT FIRE - out: %d, ctr:%d], SSRC:%08X, SEQ:%d\n", fire_outptr, fire_rxctr, sss, return_buf[2]<<8|return_buf[3]);
#endif
        fire_outptr++;
        if (fire_outptr == MAX_RTP_BUF_N) fire_outptr = 0;
    }

    if (l > 0) {
        if (fire_rxctr < MAX_RTP_BUF_N) {
            __fire_rxlen[fire_inptr] = l;
            fire_rxctr++;

//printf("[IN FIRE - in: %d, ctr:%d, l:%d] SSRC:%08X, SEQ: %d\n", fire_inptr, fire_rxctr, l, sss, buf[2] <<8 | buf[3]);
            fire_inptr++;
            if (fire_inptr == MAX_RTP_BUF_N) fire_inptr = 0;

            if (!fire_start_get_buf && fire_rxctr >= PLAY_FIRE_FUNC_LIMIT_RTP_BUF_N) {
                fire_start_get_buf = 1;
                printf("< READY-FIRE >\n");
            }
        }
        else
            printf("[FAIL INSERT, FIRE - %d, %d] %d, \n", fire_inptr, l, (buf[2]<<8) | buf[3]);
    }

    //if (fire_rxctr == MAX_BUF_N)
//if (*len)
//      printf("[OUTDATA - %d, %d] %02X %02X\n", fire_outptr, *len, return_buf[2], return_buf[3]);

    return return_buf;
}

int get_next_fire_monitoring_data_ssrc(void)
{
     int ssrc = 0;
     int outptr;
     char *buf;

     if (fire_rxctr > 0) {
        outptr = fire_outptr;

        buf =  &__fire_rxbuf[outptr][0];
        ssrc  = buf[8] << 24;
        ssrc |= buf[9] << 16;
        ssrc |= buf[10]  << 8;
        ssrc |= buf[11];
     }

     return ssrc;
}

unsigned short get_next_fire_monitoring_data_seq(void)
{
     unsigned short seq = 0;
     int outptr;
     char *buf;

     if (fire_rxctr > 0) {
        outptr = fire_outptr;

        buf =  &__fire_rxbuf[outptr][0];
        seq = buf[2] << 8 | buf[3];
     }

     return seq;
}

int get_fire_multi_rx_ctr(void)
{
    return fire_rxctr;
}

void reset_fire_multi_rx_data_ptr(void)
{
    int i;

    fire_inptr = 0;
    fire_outptr = 0;
    fire_rxctr = 0;
    fire_start_get_buf = 0;

    for (i = 0; i < MAX_RTP_BUF_N; i++)
        memset(&__fire_rxbuf[i][0], 0, 12);

    printf("< RST BUF FIRE >\n");
}

void reset_fire_multi_rx_start_get_buf(void)
{
    fire_start_get_buf = 0;
//printf("%s()==\n", __func__);
}

static char __func_rxbuf[MAX_RTP_BUF_N][MULTI_RX_BUF_LEN];
static char __func_return_buf[MULTI_RX_BUF_LEN];
static int __func_rxlen[MAX_RTP_BUF_N];
static int func_inptr = 0;
static int func_outptr = 0;
static int func_rxctr = 0;
static int func_start_get_buf = 0;

char *try_read_func_monitoring_bcast_rx_data(int *len, int stop, int use, int play)
{
    int l = 0;
    char *buf = 0, *return_buf = NULL;
    int limit_ctr;
    unsigned short seq;
//unsigned int sss;

    buf = &__func_rxbuf[func_inptr][0];
    l = try_read_cop_multicast_data(func_mon_sock, buf, MULTI_RX_BUF_LEN);


#ifdef CHECK_RTP_START_SEQ_ONE
    if (l > 0 && !unlock_func_rtp_recv) {
        seq = (buf[2] << 8) + buf[3];
        if (seq <= RTP_SEQ_LIMIT_LOW || seq >= RTP_SEQ_LIMIT_HIGH) {
            unlock_func_rtp_recv = 1;
            printf("< UNLOCK FUNC RTP(%d) >\n", seq);
        } else {
            *len = 0;
            return NULL;
        }
    }

#endif

    if (l > 0 && func_rxctr >= MAX_RTP_BUF_N) {
        __func_rxlen[func_inptr] = l;

        func_outptr++;
        if (func_outptr == MAX_RTP_BUF_N) func_outptr = 0;
#if 0
sss  = buf[8] << 24;
sss |= buf[9] << 16;
sss |= buf[10] << 8;
sss |= buf[11];
printf("[IN FUNC FULL - in: %d, out:%d, ctr:%d, l:%d] SSRC:%08X, SEQ: %d\n", func_inptr, func_outptr, func_rxctr, l, sss, buf[2] <<8 | buf[3]);
#endif

        func_inptr++;
        if (func_inptr == MAX_RTP_BUF_N) func_inptr = 0;

        l = 0;
    }
#if 0
    else {
if (l > 0) {
sss  = buf[8] << 24;
sss |= buf[9] << 16;
sss |= buf[10] << 8;
sss |= buf[11];
printf("2. IN FUNC - l:%d] SSRC:%08X, SEQ: %d\n", l, sss, buf[2] <<8 | buf[3]);
}
    }
#endif

    if (!use) {
        *len = 0;
        return NULL;
    }

    if (stop) { /*l = 0;*/ limit_ctr = 1; }
    else limit_ctr = 2;

    *len = 0;

    /*if (l > 0) {
        sss  = buf[11] << 24;
        sss |= buf[10] << 16;
        sss |= buf[9] << 8;
        sss |= buf[8];
    }*/

    if (play && func_start_get_buf && func_rxctr >= limit_ctr)
    {
        return_buf = &__func_return_buf[0];

        *len = __func_rxlen[func_outptr];
        buf = &__func_rxbuf[func_outptr][0];
        memcpy(return_buf, buf, *len);
        memset(buf, 0, 20);

        func_rxctr--;

#if 0
sss  = return_buf[8] << 24;
sss |= return_buf[9] << 16;
sss |= return_buf[10]  << 8;
sss |= return_buf[11];
printf("[OUT FUNC - out: %d, ctr:%d], SSRC:%08X, SEQ:%d, len:%d\n\n", func_outptr, func_rxctr, sss, return_buf[2]<<8|return_buf[3], *len);
#endif

        func_outptr++;
        if (func_outptr == MAX_RTP_BUF_N) func_outptr = 0;
    }

    if (l > 0) {
        if (func_rxctr < MAX_RTP_BUF_N) {
            __func_rxlen[func_inptr] = l;
            func_rxctr++;
//printf("[IN FUNC - in: %d, ctr:%d, l:%d] SSRC:%08X, SEQ: %d\n", func_inptr, func_rxctr, l, sss, buf[2] <<8 | buf[3]);

            func_inptr++;
            if (func_inptr == MAX_RTP_BUF_N) func_inptr = 0;

            if (!func_start_get_buf && func_rxctr >= PLAY_AUTO_FUNC_LIMIT_RTP_BUF_N) {
                func_start_get_buf = 1;
                printf("< READY-FUNC >\n");
             }
        }
        else
            printf("[FAIL INSERT, FUNC - %d, %d] %d\n", func_inptr, l, buf[2]<<8 | buf[3]);
    }


//if (*len)
//      printf("[OUTDATA - %d, %d] %02X %02X\n", func_outptr, *len, return_buf[2], return_buf[3]);

    return return_buf;
}

int get_next_func_monitoring_data_ssrc(void)
{
     int ssrc = 0;
     int outptr;
     char *buf;

     if (func_rxctr > 0) {
        outptr = func_outptr;
        buf =  &__func_rxbuf[outptr][0];

        ssrc  = buf[8] << 24;
        ssrc |= buf[9] << 16;
        ssrc |= buf[10]  << 8;
        ssrc |= buf[11];
     }

     return ssrc;
}

unsigned short get_next_func_monitoring_data_seq(void)
{
     unsigned short seq = 0;
     int outptr;
     char *buf;

     if (func_rxctr > 0) {
        outptr = func_outptr;
        buf =  &__func_rxbuf[outptr][0];
        seq = buf[2] << 8 | buf[3];
     }

     return seq;
}

int get_func_multi_rx_ctr(void)
{
    return func_rxctr;
}

void reset_func_multi_rx_data_ptr(void)
{
    int i;

    func_inptr = 0;
    func_outptr = 0;
    func_rxctr = 0;
    func_start_get_buf = 0;

    for (i = 0; i < MAX_RTP_BUF_N; i++)
        memset(&__func_rxbuf[i][0], 0, 12);

    if (!unlock_fire_rtp_recv) unlock_fire_rtp_recv = 1;
    if (!unlock_auto_rtp_recv) unlock_auto_rtp_recv = 1;

//printf("%s()==\n", __func__);
    printf("< RST BUF FUNC >\n");
}

void reset_func_multi_rx_start_get_buf(void)
{
    func_start_get_buf = 0;
}


void close_cop_multi_rx_sock(void)
{
    if (multi_rx_sock < 0)
        return;

    //printf("< CLOSE RX SOCK(%d) >\n", multi_rx_sock);

    close(multi_rx_sock);

    multi_rx_sock = -1;
}

void close_cop_multi_rx_sock_2nd(void)
{
    if (multi_rx_sock_2nd < 0)
        return;

    //printf("< CLOSE RX SOCK(%d) >\n", multi_rx_sock_2nd);

    close(multi_rx_sock_2nd);

    multi_rx_sock_2nd = -1;
}
