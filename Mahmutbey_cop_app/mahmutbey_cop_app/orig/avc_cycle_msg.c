/*
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "avc_tcp.h"
#include "avc_cycle_msg.h"
#include "bcd.h"
#include "crc16.h"

#include "debug_multicast.h"

#define AVC_CYCLE_MULTICAST_IP "234.1.1.1"
#define AVC_CYCLE_PORT_NUMBER 5500

#define CYCLE_RX_MAXBUF	1024
static char CycleRxBuf[CYCLE_RX_MAXBUF];

static int avc_cycle_sock;
static struct sockaddr_in avc_cycle_srv, avc_cycle_cli;
static socklen_t avc_cycle_addrSize;

static unsigned int buf_avc_srv_ip_addr[5];
static unsigned int cur_avc_srv_ip_addr;
static int avc_srv_ip_ctr;

static unsigned int buf_avc_srv_ip_addr2[2];
static int avc_srv_ip_ctr2;

static unsigned int old_avc_srv_ip_addr;

static unsigned int changed_avc_srv_ip_addr;
static int AvcCycleIsValid;

static int elapsed_1min;
static time_t old_time, cur_time;

/* Heart Beat Counter of AVC */
static unsigned int CurAvcHBC;
static unsigned int OldAvcHBC;

extern int internal_test_use;                  /* Only used for Env. test case */

static int check_avc_cycle_data_valid(char *buf, int len);

void init_avc_cycle_data(void)
{
    int i;

    avc_cycle_sock = -1;
    cur_avc_srv_ip_addr = 0;

    for (i = 0; i < 5; i++)
        buf_avc_srv_ip_addr[i] = 0;
    avc_srv_ip_ctr = 0;
    avc_srv_ip_ctr2 = 0;

    old_avc_srv_ip_addr = -1;
    changed_avc_srv_ip_addr = 0;
    AvcCycleIsValid = 0;

    time(&old_time);

    CurAvcHBC = 0;
    OldAvcHBC = 0;
}

void reset_avc_cycle_hbc(void)
{
    CurAvcHBC = 0;
    OldAvcHBC = 0;
}

int connect_avc_cycle_multicast(void)
{
    struct ip_mreq avc_cycle_mreq;

    bzero(&avc_cycle_srv, sizeof(avc_cycle_srv));
    avc_cycle_srv.sin_family = AF_INET;
    avc_cycle_srv.sin_port = htons(AVC_CYCLE_PORT_NUMBER);
    if (inet_aton(AVC_CYCLE_MULTICAST_IP, &avc_cycle_srv.sin_addr) < 0) {
        perror("AVC CYCLE inet_aton");
        return -1;
    }

    if ((avc_cycle_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("AVC CYCLE socket");
        return -1;
    }

    if (bind(avc_cycle_sock, (struct sockaddr *)&avc_cycle_srv, sizeof(avc_cycle_srv)) < 0) {
        perror("AVC CYCLE bind");
        close(avc_cycle_sock);
        avc_cycle_sock = -1;
        return -1;
    }

    if (inet_aton(AVC_CYCLE_MULTICAST_IP, &avc_cycle_mreq.imr_multiaddr) < 0) {
        perror("AVC CYCLE inet_aton");
        close(avc_cycle_sock);
        avc_cycle_sock = -1;
        return -1;
    }

    avc_cycle_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(avc_cycle_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                &avc_cycle_mreq,sizeof(avc_cycle_mreq)) < 0)
    {
        perror("AVC CYCLE setsockopt");
        close(avc_cycle_sock);
        avc_cycle_sock = -1;
        return -1;
    }

    avc_cycle_addrSize = sizeof(avc_cycle_cli);

    printf("< CONNECT AVC CYCLE MULTICAST (sock:%d) >\n", avc_cycle_sock);

    return 0;
}

int avc_cycle_close(void)
{
    if (avc_cycle_sock < 0)
        return -1;

    printf("< CLOSE CYCLE MULTICAST FROM AVC >\n");

    close(avc_cycle_sock);

    avc_cycle_sock = -1;

    return 0;
}

static int is_all_ip_is_same(unsigned int *buf_ip_addr, int n)
{
    int i, tmp;
    int ret = 0, compare = 0;

    tmp = buf_ip_addr[0];
    for (i = 1; i < n; i++) {
        if (tmp == buf_ip_addr[i])
            compare++;
    }

    if (compare == (n - 1)) ret = 1;

    return ret;
}

int try_read_avc_cycle_data(char *avc_srv_ip, int is_fixed)
{
    fd_set rd_set;
    int rx_len;
    int numFds;
    struct timeval timeout;
    int ret = -1, valid = 0, compare;
    unsigned int srv_ip = 0;
    char tmp_buf[32];

    if (avc_cycle_sock < 0)
        return -1;

    AvcCycleIsValid = 0;

    rx_len = 0;
    FD_ZERO(&rd_set);
    FD_SET(avc_cycle_sock, &rd_set);
    numFds = avc_cycle_sock + 1;
    timeout.tv_sec = 0;
    timeout.tv_usec = 30;
    ret = select(numFds, &rd_set, NULL, NULL, &timeout);
    if (ret > 0) {
        rx_len = recvfrom(avc_cycle_sock, CycleRxBuf, AVC_CYCLE_FRAME_LEN,
            0, (struct sockaddr *) &avc_cycle_cli, &avc_cycle_addrSize);
        if (rx_len > 0) {
            srv_ip = avc_cycle_cli.sin_addr.s_addr;

            if (is_fixed && srv_ip != inet_addr(avc_srv_ip))
                valid = 0;
            else
                valid = check_can_connect_srv_ip(srv_ip);

            if (valid == 1) {
                buf_avc_srv_ip_addr[avc_srv_ip_ctr++] = srv_ip;
                if (avc_srv_ip_ctr == 5) {
                    compare = is_all_ip_is_same(buf_avc_srv_ip_addr, avc_srv_ip_ctr);
                    avc_srv_ip_ctr = 0;

                    if (compare)
                        cur_avc_srv_ip_addr = srv_ip;
                }

                if (avc_srv_ip_ctr2 >= 2) avc_srv_ip_ctr2 = 0;
                buf_avc_srv_ip_addr2[avc_srv_ip_ctr2++] = srv_ip;
                if ((buf_avc_srv_ip_addr2[0] > 0) && (buf_avc_srv_ip_addr2[0] == buf_avc_srv_ip_addr2[1]))
                    AvcCycleIsValid = check_avc_cycle_data_valid(&CycleRxBuf[0], rx_len);
                else
                    AvcCycleIsValid = 0;
            }
            else {
                rx_len = 0;
                AvcCycleIsValid = 0;
            }

            if ((cur_avc_srv_ip_addr) && (old_avc_srv_ip_addr != cur_avc_srv_ip_addr)) {
                old_avc_srv_ip_addr = cur_avc_srv_ip_addr;
                changed_avc_srv_ip_addr = 1;
                if (is_fixed == 0) {
                    sprintf(avc_srv_ip, "%d.%d.%d.%d", cur_avc_srv_ip_addr & 0xff,
                                   (cur_avc_srv_ip_addr>>8) & 0xff,
                                   (cur_avc_srv_ip_addr>>16) & 0xff,
                                   (cur_avc_srv_ip_addr>>24) & 0xff);
                    printf("[ DETECT AVC IP: %s ]\n", avc_srv_ip);
                }
            }
            else
                changed_avc_srv_ip_addr = 0;
        }
    }

    if (rx_len && AvcCycleIsValid)
        ret = rx_len;
    else
        ret = -1;

	if( internal_test_use && (ret > 0) ) {
		make_qc_test_send_buf(CycleRxBuf, ret, DEBUG_CYCLE);
	}

    return ret;
}

int setup_local_time(void)
{
    struct tm tm_val;
    time_t m_time;
    int sec, min, hour, mday, mon, year;
    char *buf = &CycleRxBuf[AVC_CYCLE_TIME_BUF_OFF];
    int ret;

    year = bcd2bin(buf[0]) + bcd2bin(buf[1]) * 100;
    mon = bcd2bin(buf[2]);
    mday = bcd2bin(buf[3]);
    hour = bcd2bin(buf[4]);
    min = bcd2bin(buf[5]);
    sec = bcd2bin(buf[6]);

#if 1
    printf("< SET TIME: %d-%d-%d %d:%d:%d >\n", year, mon, mday, hour, min, sec);
#endif

    tm_val.tm_year = year - 1900;
    tm_val.tm_mon = mon-1;
    tm_val.tm_mday = mday;
    tm_val.tm_hour = hour;
    tm_val.tm_min = min;
    tm_val.tm_sec = sec;

    m_time = mktime(&tm_val);
    ret = stime(&m_time);

    if (!elapsed_1min) {
        time(&cur_time);
        if ((cur_time - old_time) < (60*60))
            return ret;
    }

    old_time = m_time;

    if (elapsed_1min)
        elapsed_1min = 0;

    return ret;
}


unsigned int get_avc_srv_ip(void)
{
    return cur_avc_srv_ip_addr;
}

void reset_avc_srv_ip(void)
{
    int i;

    cur_avc_srv_ip_addr = 0;
    old_avc_srv_ip_addr = -1;

    for (i = 0; i < 5; i++)
        buf_avc_srv_ip_addr[i] = 0;
    avc_srv_ip_ctr = 0;
    avc_srv_ip_ctr2 = 0;
}

int is_changed_avc_cycle_ip(void)
{
    int ret = changed_avc_srv_ip_addr;
    changed_avc_srv_ip_addr = 0;
    return ret;
}

static int check_avc_cycle_data_valid(char *buf, int len)
{
    //unsigned short crc16_cal, crc16_get;
    //unsigned short l, h;

    if (buf[0] != len) {
        printf("< LEN ERROR OF AVC CYCLE, %d >\n", buf[0]);
        return 0;
    }

    if (buf[1] != AVC_CYCLE_FRAME_ID) {
        printf("< ID ERROR OF AVC CYCLE, %d >\n", buf[1]);
        return 0;
    }

#if 0
    l = (unsigned short)buf[len-2];
    h = (unsigned short)buf[len-1];
    crc16_get = l | (h<<8);

    crc16_cal = make_crc16((unsigned char *)buf, len-2);

    if (crc16_cal != crc16_get) {
        printf("< CRC ERROR OF AVC CYCLE, R:0x%04X, C:0x%04X >\n",
            crc16_get, crc16_cal);
        return 0;
    }
#endif

    return 1;
}
 
int check_avc_hbc(void)
{
    unsigned int c1, c2, c3, c4;
    int ret = 0;

    char *buf = &CycleRxBuf[0];

    c1 = (unsigned int)buf[AVC_CYCLE_HBC_BUF_OFF];
    c2 = (unsigned int)buf[AVC_CYCLE_HBC_BUF_OFF+1];
    c3 = (unsigned int)buf[AVC_CYCLE_HBC_BUF_OFF+2];
    c4 = (unsigned int)buf[AVC_CYCLE_HBC_BUF_OFF+3];

    CurAvcHBC = c1 + (c2<<8) + (c3<<16) + (c4<<24);

    //printf("[ AVC HBC: %d ]\n", CurAvcHBC);

    if (OldAvcHBC) {
        if (CurAvcHBC != 0xFFFFFFFF) {
            if ((CurAvcHBC - OldAvcHBC) != 1) {
                printf("[ HBC ERROR of AVC CYCLE, CUR HBC: %d, OLD HBC: %d]\n",
                	CurAvcHBC, OldAvcHBC);
                ret = -1;
            }
        }
    }

    OldAvcHBC = CurAvcHBC;

    return ret;
}

int cop_set_time(void)
{
    struct tm tm_val;
    time_t m_time;
    int sec, min, hour, mday, mon, year;
    char *buf = &CycleRxBuf[AVC_CYCLE_TIME_BUF_OFF];
    int ret;

    year = bcd2bin(buf[0]) + bcd2bin(buf[1]) * 100;
    mon = bcd2bin(buf[2]);
    mday = bcd2bin(buf[3]);
    hour = bcd2bin(buf[4]);
    min = bcd2bin(buf[5]);
    sec = bcd2bin(buf[6]);

#if 1
    printf("< SET TIME: %d-%d-%d  %d-%d-%d >\n", year, mon, mday, hour, min, sec);
#endif

    tm_val.tm_year = year - 1900;
    tm_val.tm_mon = mon;
    tm_val.tm_mday = mday;
    tm_val.tm_hour = hour;
    tm_val.tm_min = min;
    tm_val.tm_sec = sec;

    m_time = mktime(&tm_val);
    ret = stime(&m_time);

    if (!elapsed_1min) {
        time(&cur_time);
        if ((cur_time - old_time) < (60*60))
            return ret;
    }

    old_time = m_time;

    if (elapsed_1min)
        elapsed_1min = 0;

    return ret;
}

