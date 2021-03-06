/*
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "gpio_key_dev.h"
#include "avc_tcp.h"
#include "avc_cycle_msg.h"
#include "crc16.h"
#include "cob_process.h"

#include "debug_multicast.h"

extern void gpio_watchdog_disable_start_watchdog_by_kernel_trigger(void);
extern void gpio_watchdog_enable(void);

extern COP_Cur_Status_t __broadcast_status;
extern int internal_test_use;                  /* Only used for Env. test case */


static unsigned short Entire_Ver;
static int avc_tcp_sock;

static unsigned int AllToAVC_HBC;
static unsigned int UpdateToAVC_HBC;

static unsigned short SetDIErrCode;
static unsigned short ClearDIErrCode;

static unsigned int AvcFrameErrCounter;

static char time_set_done;
static char TestActive;
static char TestResult;

static int AllToTcpSendEnabled;

static char TcpTxBuf[1024];
static int TcpSend_ReadyLen;

static int make_all_to_avc_tcp_data(char *buf);
static int make_update_result_tcp_data(char *buf, char login_stat, char down_stat, char update_stat);

int send_tcp_data_to_avc(char *buf, int len);

unsigned char my_ip_value[4];
static unsigned char my_mask_value[4];

packet_AVC2CTM_t avc2ctm_packet; //add ctm function
packet_AVC2CTM_SW_UPDATE_t avc2ctm_sw_up_packet; /*ctm sw update packet */
packet_AVC2CTM_EVENT_t  avc2ctm_log_event_packet; /*ctm sw log event packet */
SubDevice_t g_SubDevice[150];  //mc1, m ,t, mc2 중 mc1,m 차량 subdeivce 만 업데이트한다. 
char AVC2CTM_msg[53];
char avc_status;
char AVC_ScheduleTime[19];


/*
#define DI_ERROR_DRIVER_SW_ERR		1
#define DI_ERROR_DRIVER_HW_ERR		2
#define DI_ERROR_FONT_ERROR		3
#define DI_ERROR_DI_MSG_STRING_TYPE	4
*/

static char *cop_system_error_name[] = {
    "DI_NO_ERROR",
    "DI_DRIVER_SW_ERROR",		/* 1 */
    "DI_DRIVER_HW_ERROR",		/* 2 */
    "DI_ERROR_FONT_ERROR",		/* 3 */
    "DI_ERROR_DI_MSG_STRING_TYPE",	/* 4 */
    "TEST ERROR 5",	/* 5 */
    "TEST ERROR 6",	/* 5 */
    "TEST ERROR 7",	/* 5 */

#if 0
    "DI_ERROR_TRAIN_NUMBER_FILE_SAVE",	/* 3 */
    "DI_ERROR_LINE_NUMBER_FILE_SAVE",	/* 4 */
    "DI_ERROR_DEP_NAME_FILE_SAVE",	/* 5 */
    "DI_ERROR_DEST_NAME_FILE_SAVE",	/* 6 */
    "DI_ERROR_CURR_NAME_FILE_SAVE",	/* 7 */
    "DI_ERROR_NEXT_NAME_FILE_SAVE",	/* 8 */
#endif
};

char read_link_carrier(void)
{
#if 0
    int fd;
    char buf[4];

    fd = open(LINUX_CARRIER_PROC_FILE, O_RDONLY);
    if (fd < 0) {
        //printf("Cannot open: %s\n", LINUX_CARRIER_PROC_FILE);
        return -1;
    }

    read(fd, buf, 4);
    close(fd);

    return buf[0];
#endif

    int ret;
    int sockfd;
    struct ifreq ifr;
    char ret_ch = '0';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "%s(),ERROR, creating socket (%d %s)\n", __func__, errno, strerror(errno));
        return ret_ch;
    }

    strcpy(ifr.ifr_name, "eth0");

    ret = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
    if (ret < 0) {
        return ret_ch;
    }

    if (((ifr.ifr_flags & IFF_UP) == IFF_UP) && ((ifr.ifr_flags & IFF_RUNNING) == IFF_RUNNING))
        ret_ch = '1';

    close (sockfd);

    return ret_ch;
}

int init_ip_addr(void)
{
    struct sockaddr_in *my_sin;
    struct ifreq ifrq;
    int sockfd;
    int j, ret = 0;
    unsigned int tmp_address;

    memset(my_ip_value, 0, 4);
    memset(my_mask_value, 0, 4);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("TCP SOCKR");
        return -1;
    }

    //for (i = 1; ; i++)
    while (1)
    {
#if 0
        ifrq.ifr_name[0] = 0;
        ifrq.ifr_ifindex = i;
        if (ioctl(sockfd, SIOCGIFNAME, &ifrq) < 0) {
            //printf("IOCTL IFNAME ERROR (%d)\n", i);
            break;
        }
#else
        strcpy(ifrq.ifr_name, "eth0");
#endif

        printf("\nIF NAME:%s\n", ifrq.ifr_name);

        if (ioctl(sockfd, SIOCGIFHWADDR, &ifrq) < 0) {
            printf("IOCTL HWADDR ERROR\n");
            ret = -1;
            break;
        }

        printf("###################################\n");
        printf("  MAC: ");
        for (j = 0; j < 6; j++) {
            printf("%02X", ifrq.ifr_hwaddr.sa_data[j]);
            if (j >= 0 && j < 5)
                printf(":");
        }
        printf("\n###################################\n");

        if (ioctl(sockfd, SIOCGIFADDR, &ifrq) < 0) {
			printf("IOCTL IFADDR ERROR errno[%d]\n",errno);
            ret = -1;
            break;
        }
        my_sin = (struct sockaddr_in *)&ifrq.ifr_addr;

        my_ip_value[0] = my_sin->sin_addr.s_addr & 0xff;;
        my_ip_value[1] = (my_sin->sin_addr.s_addr >> 8) & 0xff;;
        my_ip_value[2] = (my_sin->sin_addr.s_addr >> 16) & 0xff;;
        my_ip_value[3] = (my_sin->sin_addr.s_addr >> 24) & 0xff;;

        printf("IP : %d.%d.%d.%d\n",
                my_ip_value[0], my_ip_value[1], my_ip_value[2], my_ip_value[3]);

        tmp_address = (my_ip_value[0] << 8) | my_ip_value[1];
        if (tmp_address != IP_NETWORK_CLASS) {
	    //printf("< ERROR, IP NETWORK ADDRESS > \n");
            //ret = -1;
            //break;
	    printf("< WARNING, IP NETWORK ADDRESS > \n");
        }

        if (ioctl(sockfd, SIOCGIFNETMASK, &ifrq) < 0) {
            printf("IOCTL IFNETMASK ERROR\n");
            ret = -1;
            break;
        }
        my_sin = (struct sockaddr_in *)&ifrq.ifr_addr;
        my_mask_value[0] = my_sin->sin_addr.s_addr & 0xff;;
        my_mask_value[1] = (my_sin->sin_addr.s_addr >> 8) & 0xff;;
        my_mask_value[2] = (my_sin->sin_addr.s_addr >> 16) & 0xff;;
        my_mask_value[3] = (my_sin->sin_addr.s_addr >> 24) & 0xff;;
        printf("MASK: %d.%d.%d.%d\n",
                my_mask_value[0], my_mask_value[1], my_mask_value[2], my_mask_value[3]);

        printf("\n");


        tmp_address = (my_mask_value[0] << 8) | my_mask_value[1];
        if ((unsigned int)(tmp_address & 0xFFFF) != (unsigned int)(IP_MASK_CLASS & 0xFFFF)) {
	    //printf("< ERROR, IP MASK ADDRESS (0x%X-0x%X) > \n", tmp_address, IP_NETWORK_CLASS & 0xFFFF);
            //ret = -1;
            //break;
	    printf("< WARNING, IP MASK ADDRESS (0x%X-0x%X) > \n", tmp_address, IP_NETWORK_CLASS & 0xFFFF);
        }
        break;
    }

    close(sockfd);

    return ret;
}

int avc_tcp_init(void)
{
    int ret = 0;

    TcpSend_ReadyLen = 0;
    avc_tcp_sock = -1;

    AvcFrameErrCounter = 0;
    time_set_done = 0;
    AllToAVC_HBC = 1;
    UpdateToAVC_HBC = 1;

    SetDIErrCode = 0;
    ClearDIErrCode = 0;
    AllToTcpSendEnabled = 0;

    memset(TcpTxBuf, 0, 1024);

    ret = init_ip_addr();

    return ret;
}

int check_can_connect_srv_ip(unsigned int srv_ip_addr)
{
	return 1;
	/* for Transet coupling */
#if 0
    int ret = 1, i;
    unsigned char cli_val, srv_val;
    unsigned char srv_ip[4];

    srv_ip[0] = srv_ip_addr & 0xff;
    srv_ip[1] = (srv_ip_addr >> 8) & 0xff;
    srv_ip[2] = (srv_ip_addr >> 16) & 0xff;
    srv_ip[3] = (srv_ip_addr >> 24) & 0xff;

//printf("%s, srv_ip_addr: 0x%X\n", __func__, srv_ip_addr);

    for (i = 0; i < 4; i++) {
        cli_val = my_ip_value[i] & my_mask_value[i];
        srv_val = srv_ip[i] & my_mask_value[i];
        if (cli_val != srv_val) {
            //printf("< Different Network address >\n");
            ret = 0;
            break;
        }
    }

    return ret;
#endif
}

extern void ignore_avc_server_to_wait_another_server(void);

int try_avc_tcp_connect(char *ip_addr, int port, int *need_reboot, int *need_renew)
{
    int ret;
    int sockfd;
    int flags;
    struct sockaddr_in serv_addr;
    int error = 0;
    struct timeval timeout;
    int len = 0;
    fd_set wset; //rset;
    static int tcp_conn_problem_ctr = 0;

    avc_tcp_sock = -1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR, creating socket (%d %s)\n", errno, strerror(errno));
        if (errno == EMFILE) {
            fprintf(stderr, "<<< AVC TCP SOCKET ERROR, WILL REBOOT... >>>\n");
            *need_reboot = 1;
        }
        return -1;
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    //serv_addr.sin_addr.s_addr = ip_addr;
    serv_addr.sin_addr.s_addr = inet_addr(ip_addr);

    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        fprintf(stderr, "ERROR, fcntl F_GETFL (%s)\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    ret = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if (ret < 0) {
        fprintf(stderr, "ERROR, fcntl F_SETFL (%s)\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            fprintf(stderr, "ERROR, connect to AVC (%d %s)\n", errno, strerror(errno));
            close(sockfd);
            return -1;
        }
    }

    if (ret == 0) {
        fcntl(sockfd, F_SETFL, flags);
        avc_tcp_sock = sockfd;
        printf("\n[ 1.TCP CONNECTTION TO AVC: SUCCESS(%d) ]\n", avc_tcp_sock);
        if (tcp_conn_problem_ctr) tcp_conn_problem_ctr = 0;
        return 0;
    }

    //FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(sockfd, &wset);
    //rset = wset;
    //timeout.tv_sec = 0;
    //timeout.tv_usec = 20000;
    timeout.tv_sec = 1; // Ver0.90, 1 -> 2
    timeout.tv_usec = 0;

    ret = select(sockfd+1, NULL, &wset, NULL, &timeout);
    if (ret == 0) {
        printf("< TIMTOUT CONNECT TO AVC(s:%d) ... Cancelling >\n", sockfd);
        close(sockfd);
        ignore_avc_server_to_wait_another_server();

        tcp_conn_problem_ctr++;
        if (tcp_conn_problem_ctr == 1) {
            fprintf(stderr, "<<< CONNECT TO AVC TIMEOUT, WILL RENEW... >>>\n");
            *need_renew = 1;
            *need_reboot = 0;
        } else if (tcp_conn_problem_ctr > 1) {
            fprintf(stderr, "<<< CONNECT TO AVC TIMEOUT, WILL REBOOT... >>>\n");
            *need_renew = 0;
            *need_reboot = 1;
        }

        return -1;
    } else if (ret < 0) {
        fprintf(stderr, "< ERROR, connect to AVC (%d - %s) >\n", errno, strerror(errno));
        close(sockfd);
        ignore_avc_server_to_wait_another_server();

        tcp_conn_problem_ctr++;
        if (tcp_conn_problem_ctr == 1) {
            fprintf(stderr, "<<< CONNECT TO AVC ERROR, WILL RENEW... >>>\n");
            *need_renew = 1;
            *need_reboot = 0;
        } else if (tcp_conn_problem_ctr > 1) {
            fprintf(stderr, "<<< CONNECT TO AVC ERROR, WILL REBOOT... >>>\n");
            *need_renew = 0;
            *need_reboot = 1;
        }

        return -1;
    }

    /* now ret > 0 */
    len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0) {
        fprintf(stderr, "ERROR, in getsockopt (%d - %s)\n", errno, strerror(errno));
        close(sockfd);
        return -1;
    }

    if (error) {
        close(sockfd);
        printf("< TCP CONNECTTION TO AVC: FAIL(%d - %s) >\n", error, strerror(error));
        ignore_avc_server_to_wait_another_server();

        tcp_conn_problem_ctr++;
        if (tcp_conn_problem_ctr == 1) {
            fprintf(stderr, "<<< CONNECT TO AVC FAIL, WILL RENEW... >>>\n");
            *need_renew = 1;
            *need_reboot = 0;
        } else if (tcp_conn_problem_ctr > 1) {
            fprintf(stderr, "<<< CONNECT TO AVC FAIL, WILL REBOOT... >>>\n");
            *need_renew = 0;
            *need_reboot = 1;
        }

        return -1;
    }

    fcntl(sockfd, F_SETFL, flags);

    avc_tcp_sock = sockfd;
    printf("\n[ 2.TCP CONNECTTION TO AVC: SUCCESS(%d) ]\n", avc_tcp_sock);

    if (tcp_conn_problem_ctr) tcp_conn_problem_ctr = 0;

    return 0;
}

int avc_tcp_close(char *_dbg_msg)
{
    if (avc_tcp_sock < 0)
        return -1;

    printf("\n< CLOSE TCP CONNECTION TO AVC(s:%d, %s) >\n", avc_tcp_sock, _dbg_msg);

    close(avc_tcp_sock);

    avc_tcp_sock = -1;

    return 0;
}

void set_DI_errcode(unsigned short ec)
{
    SetDIErrCode = ec;
    AllToTcpSendEnabled = 1;

    if (ec)
        printf("< DI SYSTEM ERROR SET:%s >\n", cop_system_error_name[ec]);
}

void clear_DI_errcode(unsigned short ec)
{
    ClearDIErrCode = ec;
    AllToTcpSendEnabled = 1;

    printf("< DI SYSTEM ERROR CLEAR:%s >\n", cop_system_error_name[ec]);
}

#if 1
int try_avc_tcp_read(char *buf, int maxlen)
{
    int ret = 0;
    int rx_len;
    fd_set rset, wset;
    struct timeval timeout;
    int numFds;

Retry:
    if (avc_tcp_sock < 0)
        return -1;

    rx_len = 0;
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(avc_tcp_sock, &rset);
    numFds = avc_tcp_sock + 1;
    timeout.tv_sec = 0;
	//timeout.tv_usec = 1000;
    timeout.tv_usec = 50;
    ret = select(numFds, &rset, NULL/*&wset*/, NULL, &timeout);

	if(ret < 0 && errno == EINTR)
		goto Retry;

    if (ret >= 0) {
        if (FD_ISSET(avc_tcp_sock, &rset)) {
            FD_CLR(avc_tcp_sock, &rset);
            rx_len = read(avc_tcp_sock, buf, maxlen);
            if (rx_len <= 0) { /* Connection close */
                //close(avc_tcp_sock);
                //avc_tcp_sock = -1;
                ret = -1;
                //printf("[ TCP CONNECTTION TO AVC LOST ]\n");
            }
            else {
                ret = rx_len;
                //printf("\n[ AVC->TCP READ  %d]\n", ret);
            }
        }
    }

#if 0
    if (ret > 0) {
		int i, k;
		printf("\n");
        printf("---- TCP READ (AVC --> COP) ----\n   ");
		for(i=0; i < 10; i++)
		printf("_%1X ", i);
        for (i = 0, k = 0; i < ret; i++, k++) {
            //if ((k % 16) == 0)
            if ((k % 10) == 0)
                printf("\n%1x_ ", (k==0)?0:k/10);

			if ( (i == AVC2COP_PACKET_SIZE) || (i == (AVC2COP_PACKET_SIZE*2)) ) {
                printf("\n");
        		printf("--------------------------------\n");
                printf("\n");
        		printf("---- TCP READ (AVC --> COP) ----");
                printf("\n0_ ");
				k = 0;
			}

            printf("%02X ", buf[i]);
        }
		printf("\n");
        printf("--------------------------------\n");
    }
#else
    if (ret > 0){
        print_avc_to_cop_packet(buf, ret);
	}
#endif

	if( internal_test_use && (ret > 0) ) {
		make_qc_test_send_buf(buf, ret, DEBUG_TCP_RECEIVE);
	}

    return ret;
}
#else
int try_avc_tcp_read(char *buf, int maxlen)
{
    int rx_len;

    if (avc_tcp_sock < 0)
        return -1;

    errno = 0;
    rx_len = recv(avc_tcp_sock, buf, maxlen, 0);
    if (rx_len == -1) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            printf("<< SOCK %d RECV ERROR, errno:%d >>\n", avc_tcp_sock, errno);
        }
    }
	printf("%s, rx_len:%d, errno:%d\n", __func__, rx_len, errno);

	if( internal_test_use && (rx_len > 0) )
		send_qc_test_multi_tx_data(buf, rx_len);

    return rx_len;
}
#endif

void print_avc_to_cop_packet(char *buf, int len)
{
    int i, n, active = 0;

    n = 0;
//    if (buf[1] != 5 ) {
//        if(buf[1] != 0x11){
//            printf("------- TCP READ ----------------------------");
//            for (i = 0; i < len; i++) {
//                if ((i % 16) == 0)
//                    printf("\n");
//                printf("%02X ", buf[i]);
//            }
//            printf("\n---------------------------------------------\n\n");
//        return;
//        }
//    }

    if (buf[1] != 5 ) {
        return;
    }

    while (n < len) {
        printf("\n----------- AVC->COP(%03d) --------------------------------", __broadcast_status);

        active = buf[n+2] & 0x1;
        if (active == 1) {
            printf("\n[BCAST] ");
            if (buf[n+2] & 0x02)
               printf("GLOBAL PA Enable\n");
            if (buf[n+2] & 0x04)
               printf("GLOBAL PA Disable\n");

            if (buf[n+3] || buf[n+4])
                printf("Func Num:%d ", ((buf[n+4]<<8)|buf[n+3]));
            switch (buf[n+5]) {
                case 0x01: printf("Func BCast:Start "); break;
                case 0x02: printf("Func BCast:Reject "); break;
                case 0x04: printf("Func BCast:Stop "); break;
                case 0x08: printf("Auto BCast:Start "); break;
                case 0x10: printf("Auto BCast:Stop "); break;
                case 0x20: printf("OCC BCast:Start "); break;
                case 0x40: printf("OCC BCast:Stop "); break;
                case 0x88: printf("Fire BCast:Start "); break;
                case 0x90: printf("Fire BCast:Stop "); break;
                default: if (buf[n+5]) printf("Bcast ?: 0x%02X ", buf[n+5]); break;
            }
            if (buf[n+6])
                printf("IN/OUT Flag:%d ", buf[n+6]);
            switch (buf[n+7]) {
                case 1: printf("IN BCast:Start "); break;
                case 2: printf("IN BCast:Reject "); break;
                case 4: printf("IN BCast:Stop "); break;
                default: if (buf[n+7]) printf("IN Bcast ?: 0x%02X ", buf[n+7]); break;
            }
            switch (buf[n+8]) {
                case 1: printf("OUT BCast:Start "); break;
                case 2: printf("OUT BCast:Reject "); break;
                case 4: printf("OUT BCast:Stop "); break;
                default: if (buf[n+8]) printf("OUT Bcast ?: 0x%02X ", buf[n+8]); break;
            }
            switch (buf[n+9]) {
                case 1: printf("OCC PA:Start "); break;
                case 4: printf("OCC PA:Stop "); break;
                default: if (buf[n+9]) printf("OCC PA ?: 0x%02X ", buf[n+9]); break;
            }
        }

        active = buf[n+10];
        if (active == 1) {
            printf("\n[CALL] ");
            switch (buf[n+11]) {
                case 0x01: printf("CAB CALL:Wake-up "); break;
                case 0x02: printf("CAB CALL:Start "); break;
                case 0x04: printf("CAB CALL:Stop "); break;
                case 0x08: printf("CAB CALL:Reject "); break;
                case 0x10: printf("CAB MON :Start "); break;
                case 0x20: printf("CAB MON :Stop "); break;
                case 0x40: printf("CAB CALL:Standby "); break;
                default: if (buf[n+11]) printf("CAB CALL: 0x%02X ", buf[n+11]); break;
            }
            switch (buf[n+12]) {
                case 0x01: printf("PEI CALL:Wake-up "); break;
                case 0x02: printf("PEI CALL:Start "); break;
                case 0x04: printf("PEI CALL:Stop "); break;
                case 0x08: printf("PEI CALL:Reject "); break;
                case 0x10: printf("PEI MON :Start "); break;
                case 0x20: printf("PEI MON :Stop "); break;
                case 0x90: printf("PEI MON:Emergency Mon start "); break;
                case 0xA0: printf("PEI MON :Emergency Stop "); break;
                case 0x40: printf("PEI PEI-OCC Enable "); break;
                case 0x81: printf("PEI CALL:Emergency wake-up "); break;
                case 0x82: printf("PEI CALL:Emergency start "); break;
                case 0x84: printf("PEI CALL:Emergency stop "); break;
                default: if (buf[n+12]) printf("PEI CALL: 0x%02X ", buf[n+12]); break;
            }
            switch (buf[n+13]) {
                case 0x02: printf("OCC CALL:Start "); break;
                case 0x04: printf("OCC CALL:Stop "); break;
                case 0x08: printf("OCC CALL:Reject "); break;
                case 0x40: printf("OCC CALL:Standby "); break;
                default: if (buf[n+13]) printf("OCC CALL: 0x%02X ", buf[n+13]); break;
            }

            if (buf[n+14] || buf[n+15])
                printf("Device: %d-%d ", buf[n+14], buf[n+15]);

			if (buf[n+16] || buf[n+17] || buf[n+18] || buf[n+19])
				printf("\nCOP IP: %d.%d.%d.%d ", buf[n+19], buf[n+18], buf[n+17], buf[n+16]);
			if (buf[n+20] || buf[n+21] || buf[n+22] || buf[n+23])
				printf("\nPEI IP: %d.%d.%d.%d ", buf[n+23], buf[n+22], buf[n+21], buf[n+20]);

            if (buf[n+24] || buf[n+25] || buf[n+26] || buf[n+27])
                printf("\nRX IP: %d.%d.%d.%d ", buf[n+24], buf[n+25], buf[n+26], buf[n+27]);
            if (buf[n+28] || buf[n+29] || buf[n+30] || buf[n+31])
                printf("TX IP: %d.%d.%d.%d ", buf[n+28], buf[n+29], buf[n+30], buf[n+31]);
            if (buf[n+32] || buf[n+33] || buf[n+34] || buf[n+35])
                printf("MON IP: %d.%d.%d.%d ", buf[n+32], buf[n+33], buf[n+34], buf[n+35]);
        }
        if (buf[n+36])
            printf("\nTrain_NUM:%d ", buf[n+36]);

        printf("\n----------------------------------------------------------\n");

        n += AVC2COP_PACKET_SIZE;
    }
}
/* system status report from AVC */
void print_avc_to_ctm_packet(char *buf, int len)
{
    int i, n, cnt = 0;
	int cnt1 = 0;
    n = 0;
	
    if (buf[1] != 0x11) {
        printf("------- TCP READ AVC2CTM SUB DEV STATUS----------------------------");
        for (i = 0; i < len; i++) {
            if ((i % 16) == 0)
                printf("\n");
            printf("%02X ", buf[i]);
        }
        printf("\n---------------------------------------------\n\n");
        return;
    }

    while (n < len) {
        printf("\n----------- AVC->CTM(%03d) --------------------------------", __broadcast_status);
			for(cnt1 =0 ;cnt1<4;cnt1++) {
				if(cnt1 == 0) {
                    printf("\r\n===Single MC1=== \r\n");
					i = 0;
				}
				if(cnt1 == 1) {
                    printf("\r\n===Single MC2=== \r\n");
					i = 18;
				}
				if(cnt1 == 2) {
                    printf("\r\n===Dual MC1=== \r\n");
					i = 24;
				}
				if(cnt1 == 3) {
                    printf("\r\n===Dual MC2=== \r\n");
					i = 42;
				}
				if (buf[2+i]) {
					if(buf[2+i] & 0x01) printf("AVC FAULT \r\n");
                    if(buf[2+i] & 0x02) printf("NVR FAULT \r\n");
					if(buf[2+i] & 0x04) printf("COP FAULT \r\n");
					if(buf[2+i] & 0x08) printf("FDI FAULT \r\n");
					if(buf[2+i] & 0x10) printf("PIB1 FAULT \r\n");
                    //if(buf[2+i] & 0x20) printf("PIB2 FAULT \r\n");
                    //if(buf[2+i] & 0x40) printf("SDI1 FAULT \r\n");
                    //if(buf[2+i] & 0x80) printf("SDI2 FAULT \r\n");
				}
				if (buf[3+i]){
					if(buf[3+i] & 0x01) printf("PEI1 FAULT \r\n");
					if(buf[3+i] & 0x02) printf("PEI2 FAULT \r\n");
					if(buf[3+i] & 0x04) printf("PEI3 FAULT \r\n");
					if(buf[3+i] & 0x08) printf("PEI4 FAULT \r\n");
					if(buf[3+i] & 0x10) printf("PAMP1 FAULT \r\n");
					if(buf[3+i] & 0x20) printf("PAMP2 FAULT \r\n");				
				}
				if (buf[4+i]){
					if(buf[4+i] & 0x01) printf("LRM1 FAULT \r\n");
					if(buf[4+i] & 0x02) printf("LRM2 FAULT \r\n");
					if(buf[4+i] & 0x04) printf("LRM3 FAULT \r\n");
					if(buf[4+i] & 0x08) printf("LRM4 FAULT \r\n");
					if(buf[4+i] & 0x10) printf("LRM5 FAULT \r\n");
					if(buf[4+i] & 0x20) printf("LRM6 FAULT \r\n");				
					if(buf[4+i] & 0x40) printf("LRM7 FAULT \r\n");
					if(buf[4+i] & 0x80) printf("LRM8 FAULT \r\n");								
				}
				if (buf[5+i]){
					if(buf[5+i] & 0x01) printf("FCAM FAULT \r\n");
                    //if(buf[5+i] & 0x02) printf("OCAM1 FAULT \r\n");
                    //if(buf[5+i] & 0x04) printf("OCAM2 FAULT \r\n");
					if(buf[5+i] & 0x08) printf("SCAM1 FAULT \r\n");
					if(buf[5+i] & 0x10) printf("SCAM2 FAULT \r\n");
					if(buf[5+i] & 0x20) printf("SCAM3 FAULT \r\n");				
					if(buf[5+i] & 0x40) printf("SCAM4 FAULT \r\n");				
				}
				if (buf[6+i]){
					if(buf[6+i] & 0x01) printf("PID1 FAULT \r\n");
					if(buf[6+i] & 0x02) printf("PID2 FAULT \r\n");
					if(buf[6+i] & 0x04) printf("PID3 FAULT \r\n");
					if(buf[6+i] & 0x08) printf("PID4 FAULT \r\n");
					if(buf[6+i] & 0x10) printf("PID5 FAULT \r\n");
					if(buf[6+i] & 0x20) printf("PID6 FAULT \r\n");				
					if(buf[6+i] & 0x40) printf("PID7 FAULT \r\n");
					if(buf[6+i] & 0x80) printf("PID8 FAULT \r\n");								
				}
				if (buf[7+i]){
					if(buf[7+i] & 0x01) printf("PID9 FAULT \r\n");
					if(buf[7+i] & 0x02) printf("PID10 FAULT \r\n");
					if(buf[7+i] & 0x04) printf("PID11 FAULT \r\n");
					if(buf[7+i] & 0x08) printf("PID12 FAULT \r\n");
				}
			}
			for(cnt =0 ;cnt<4;cnt++) {
				if(cnt ==0) {
                    printf("===Single T CAR=== \r\n");
					i = 0;
				}
				if(cnt ==1) {
                    printf("===Single M CAR=== \r\n");
					i = 6;
				}
				if(cnt ==2) {
                    printf("===Dual T CAR=== \r\n");
					i = 24;
				}
				if(cnt ==3) {
                    printf("===Dual M CAR=== \r\n");
					i = 30;
				}				
				if (buf[8+i]) {					                
				if(buf[8+i] & 0x10) printf("PIB1 FAULT \r\n");
				if(buf[8+i] & 0x20) printf("PIB2 FAULT \r\n");
                //if(buf[8+i] & 0x40) printf("SDI1 FAULT \r\n");
                //if(buf[8+i] & 0x80) printf("SDI2 FAULT \r\n");
				}
				if (buf[9+i]){
					if(buf[9+i] & 0x01) printf("PEI1 FAULT \r\n");
					if(buf[9+i] & 0x02) printf("PEI2 FAULT \r\n");
					if(buf[9+i] & 0x04) printf("PEI3 FAULT \r\n");
					if(buf[9+i] & 0x08) printf("PEI4 FAULT \r\n");
					if(buf[9+i] & 0x10) printf("PAMP1 FAULT \r\n");
					if(buf[9+i] & 0x20) printf("PAMP2 FAULT \r\n");				
				}
				if (buf[10+i]){
					if(buf[10+i] & 0x01) printf("LRM1 FAULT \r\n");
					if(buf[10+i] & 0x02) printf("LRM2 FAULT \r\n");
					if(buf[10+i] & 0x04) printf("LRM3 FAULT \r\n");
					if(buf[10+i] & 0x08) printf("LRM4 FAULT \r\n");
					if(buf[10+i] & 0x10) printf("LRM5 FAULT \r\n");
					if(buf[10+i] & 0x20) printf("LRM6 FAULT \r\n");				
					if(buf[10+i] & 0x40) printf("LRM7 FAULT \r\n");
					if(buf[10+i] & 0x80) printf("LRM8 FAULT \r\n");								
				}
				if (buf[11+i]){				
					if(buf[11+i] & 0x08) printf("SCAM1 FAULT \r\n");
					if(buf[11+i] & 0x10) printf("SCAM2 FAULT \r\n");
					if(buf[11+i] & 0x20) printf("SCAM3 FAULT \r\n");				
					if(buf[11+i] & 0x40) printf("SCAM4 FAULT \r\n");				
				}
				if (buf[12+i]){
					if(buf[12+i] & 0x01) printf("PID1 FAULT \r\n");
					if(buf[12+i] & 0x02) printf("PID2 FAULT \r\n");
					if(buf[12+i] & 0x04) printf("PID3 FAULT \r\n");
					if(buf[12+i] & 0x08) printf("PID4 FAULT \r\n");
					if(buf[12+i] & 0x10) printf("PID5 FAULT \r\n");
					if(buf[12+i] & 0x20) printf("PID6 FAULT \r\n");				
					if(buf[12+i] & 0x40) printf("PID7 FAULT \r\n");
					if(buf[12+i] & 0x80) printf("PID8 FAULT \r\n");								
				}
				if (buf[13+i]){
					if(buf[13+i] & 0x01) printf("PID9 FAULT \r\n");
					if(buf[13+i] & 0x02) printf("PID10 FAULT \r\n");
					if(buf[13+i] & 0x04) printf("PID11 FAULT \r\n");
					if(buf[13+i] & 0x08) printf("PID12 FAULT \r\n");
				}
			}
			
			
        printf("\n----------------------------------------------------------\n");

        n += AVC2CTM_PACKET_SIZE;
    }
}

void print_avc_to_ctm_sw_update_packet(char *buf, int len)
{
    int i, n, cnt = 0;
	int cnt1 = 0;
    n = 0;
	
    if (buf[1] != 0x30) {
//        printf("------- TCP READ AVC2CTM_SW_UPDATE----------------------------");
//        for (i = 0; i < len; i++) {
//            if ((i % 16) == 0)
//                printf("\n");
//            printf("%02X ", buf[i]);
//        }
//        printf("\n---------------------------------------------\n\n");
        return;
    }

    while (n < len) {
        printf("------- TCP READ AVC2CTM_SW_UPDATE----------------------------");
        printf("\n30.AVC to MMI (UPDATE)--- AVC->CTM(%03d)   -------------\r\n", __broadcast_status);
		printf("SW version %d,%d \r\n",buf[n+2],buf[n+3]);
		switch (buf[n+4]) {
			case 0x1: printf("FTP Connection OK"); break;
			case 0x2: printf("FTP Connection ERROR"); break;
			case 0x4: printf("FILE DOWN OK"); break;
			case 0x8: printf("FILE DOWN ERROR"); break;			
			default: if (buf[n+4]) printf("FTP STATUS ?: 0x%02X ", buf[n+4]); break;
		}
		switch (buf[n+5]) {
			case 0x1: printf("UpDate OK"); break;
			case 0x2: printf("Update ERROR"); break;
			case 0x4: printf("RE-booting"); break;
			case 0x8: printf("RE-booting complete"); break;						
			default: 				
					if (buf[n+5]){
						if(buf[n+5] & 0x10) printf("No Error OK\r\n");
						if(buf[n+5] & 0x20) printf("Errors \r\n");
						if(buf[n+5] & 0x40) printf("Errors \r\n");
						if(buf[n+5] & 0x80) printf("Errors \r\n");
					}
					break;			
		}		
		switch (buf[n+6]) {
			case 1: printf("avc_yenikapi \r\n"); break;
			case 2: printf("avc_aengine \r\n"); break;
			case 3: printf("avc_fengine \r\n"); break;
			case 4: printf("avc_cengine \r\n"); break;
			case 5: printf("avc_rengine \r\n"); break;
			case 6: printf("avc_sengine \r\n"); break;
			case 7: printf("avc_vengine \r\n"); break;					
			case 8: printf("avc_libavc \r\n"); break;			
			default: if (buf[n+6]) printf("avc process ?: 0x%02X ", buf[n+6]); break;
		}
		if (buf[n+8] || buf[n+9] || buf[n+10] || buf[n+11])
				printf("\nDevice ID: %d.%d.%d.%d ", buf[n+8], buf[n+9], buf[n+10], buf[n+11]);
			
		switch (buf[n+12]) {
			case 0x1: printf("ACK S/W update file upload ready \r\n"); break;
			case 0x2: printf("ACK S/W update file upload complete \r\n"); break;						
			case 0x10: printf("ACK S/W Version Reqeust \r\n"); break;					
			case 0x20: printf("ACK S/W Update Start \r\n"); break;			
			default: if (buf[n+12]) printf("ACK ?: 0x%02X ", buf[n+12]); break;
		}			
			
        printf("\n----------------------------------------------------------\n");

        n += AVC2CTM_PACKET_SIZE;
    }
}
/* AVC to COP(MMI) event packet */
/* frame length 45 (0x2dh) */
void print_avc_to_ctm_event_packet(char *buf, int len)
{
    int i, n, cnt = 0;
    int cnt1 = 0;
    n = 0;

    if (buf[1] == 0x10) {
#if 1
        printf("------- TCP READ AVC2COP(MMI)_EVENT_PACKET--------");
        for (i = 0; i < len; i++) {
            if ((i % 16) == 0)
                printf("\n");
            printf("%02X ", buf[i]);
        }
        printf("\n---------------------------------------------\n\n");
#endif
     //   return;
    }

    while (n < len) {
        printf("------- TCP READ AVC2COP(MMI)_EVENT_PACKET--------");
        printf("\n----------- AVC->COP(MMI)(%03d) -----------\r\n", __broadcast_status);

        switch (buf[n+11]) {
            case 0x4: printf("LOG DOWNLOAD START"); break;
            case 0x80: printf("LOG DOWNLOAD VALID"); break;
            default: if (buf[n+11]) printf("LOG DOWNLOAD STATUS ?: 0x%02X ", buf[n+11]); break;
        }

        if (buf[n+14] || buf[n+15] || buf[n+16] || buf[n+17] || buf[n+18] || buf[n+19]){
                setup_avc_schedule_time(&buf[n+14]);
        }


        switch (buf[n+21]) {
            case 0x01: printf("Single TS"); break;
            case 0x02: printf("Dual TS"); break;
            case 0x04: printf("Direct"); break;
            case 0x08: printf("Reverse"); break;
            case 0x80: printf("Valid"); break;
            default: if (buf[n+21]) printf("Dual TS STATUS ?: 0x%02X ", buf[n+21]); break;
        }

        printf("\n----------------------------------------------------------\n");

        n += AVC2CTM_EVENT_PACKET_SIZE;
    }
}


int setup_avc_schedule_time(char *buf)
{
    int sec, min, hour, mday, mon, year;
    int ret = 0;

    year = bcd2bin(buf[0]) + bcd2bin(buf[1]) * 100;
    mon = bcd2bin(buf[2]);
    mday = bcd2bin(buf[3]);
    hour = bcd2bin(buf[4]);
    min = bcd2bin(buf[5]);
    sec = bcd2bin(buf[6]);

#if 1
    printf("< Schedule TIME: %d-%d-%d %d:%d:%d >\n", year, mon, mday, hour, min, sec);
#endif
    if(year<0 || mon > 12 || mday>31 || hour >24 || min >60 || sec >60 )
            ret = -1;
    sprintf(&AVC_ScheduleTime[0],"%d-%d-%d %d:%d:%d",year,mon,mday,hour,min,sec);
    return ret;
}
void debug_avc2ctm_packet(void)
{
	printf("***********************\r\n");
	printf("%s()\r\n",__func__);
	printf("***********************\r\n");
		//check sub device status 
	printf("avc status %d \r\n",
	avc2ctm_packet.Single_MC2_status.MC_status1.AVC);	
	//avc2ctm_packet->Single_MC2_status.MC_status1.AVC);
	
	//avc_status = avc2ctm_packet->Single_MC2_status.MC_status1.AVC;
	printf("avc2ctm_packet->Single_MC2_status.MC_status1 %02x\r\n",avc2ctm_packet.Single_MC2_status.MC_status1);
	
}
int init_subsystem_status (void)
{
	int dev_cnt = 0,dev_num = 0;
	int c_class = 0, d_class = 0;
	
	/*PIB*/
	for(dev_cnt = 0; dev_cnt <8 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt/2;
		d_class = dev_cnt%2;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 129+d_class;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("PIB %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}
		
	/*SCAM*/	
	for(dev_cnt = 0; dev_cnt <16 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt/4;
		d_class = dev_cnt%4;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 177+d_class;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("SCAM %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}
	
	/*FCAM*/	
	for(dev_cnt = 0; dev_cnt <2 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt*3;
		//d_class = dev_cnt%2;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 145;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("FCAM %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}	
	
	/*COP*/
	for(dev_cnt = 0; dev_cnt <2 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt*3;
		//d_class = dev_cnt%2;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 65;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("COP %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}		
	/*PAMP*/
	for(dev_cnt = 0; dev_cnt <8 ;dev_cnt++) {
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt/2;
		d_class = dev_cnt%2;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 97+d_class;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("PAMP %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}
	/*PEI*/
	for(dev_cnt = 0; dev_cnt <16 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt/4;
		d_class = dev_cnt%4;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 113+d_class;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("PEI %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}	
	/*LRM*/
	for(dev_cnt = 0; dev_cnt <32 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt/8;
		d_class = dev_cnt%8;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 81+d_class;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("LRM %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;	
	}	
	/*PID*/
	for(dev_cnt = 0; dev_cnt <48 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt/12;
		d_class = dev_cnt%12;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 161+d_class;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("PID %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}	
	/*DIF*/
	for(dev_cnt = 0; dev_cnt <2 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt*3;
		//d_class = dev_cnt%2;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 209;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("DIF %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}

	/*NVR*/
	for(dev_cnt = 0; dev_cnt <2 ;dev_cnt++) {		
		g_SubDevice[dev_num].DeviceAddr1 = 10;
		g_SubDevice[dev_num].DeviceAddr2 = 128;
		c_class = dev_cnt*3;
		//d_class = dev_cnt%2;
		g_SubDevice[dev_num].DeviceAddr3 = 9+c_class;
		g_SubDevice[dev_num].DeviceAddr4 = 241;		
		g_SubDevice[dev_num].SwCurVer = 0;
		g_SubDevice[dev_num].SwUsbVer = 0;
        printf("NVR %d IP %d.%d.%d.%d Current SW %d USB SW %d \r\n",dev_num,
            g_SubDevice[dev_num].DeviceAddr1,g_SubDevice[dev_num].DeviceAddr2,
            g_SubDevice[dev_num].DeviceAddr3,g_SubDevice[dev_num].DeviceAddr4,
            g_SubDevice[dev_num].SwCurVer,g_SubDevice[dev_num].SwUsbVer);
		dev_num++;
	}	


}
int avc_ctm_event_msg_update (char *buf, int len)
{
    int ret = 0;

    if (len == 0){
        printf("%s(), size error got %d.\n", __func__, len);
        return -1;
    }
    //packet_AVC2CTM_EVENT_t  avc2ctm_log_event_packet; /*ctm sw log event packet */
    printf("len %d",len);
    memcpy(&avc2ctm_log_event_packet, buf, len);

    //setup_avc_schedule_time(buf[14]);

    return ret ;

}

int sub_system_status_update (char *buf, int len)
{
    int ret = 0;

    if (len == 0){
        printf("%s(), size error got %d.\n", __func__, len);
        return -1;
    }
    memcpy(&avc2ctm_packet, buf, len);
    memcpy(&AVC2CTM_msg[0],buf,len);
    return ret ;
	
}
			
int sub_system_sw_update (char *buf, int len)
{
    int ret = 0;
	int dev_cnt = 0;
	int dev_num = 0;

    printf("%s(), size got %d.\n", __func__, len);

    if (len == 0)
        return -1;

    if (len > sizeof(packet_AVC2CTM_SW_UPDATE_t)) {
        printf("%s(), size error got %d.\n", __func__, len);
     //   return -1;
    }

    memcpy(&avc2ctm_sw_up_packet, buf, len);
    //memcpy(&AVC2CTM_msg[0],buf,len);
    printf(" RECIVE IP  -----------> %d.%d.%d.%d \r\n",
           avc2ctm_sw_up_packet.DeviceAddr4   , avc2ctm_sw_up_packet.DeviceAddr3,
           avc2ctm_sw_up_packet.DeviceAddr2 , avc2ctm_sw_up_packet.DeviceAddr1  );
    printf(" SW version %d.%d \r\n", avc2ctm_sw_up_packet.sw_version[0],
            avc2ctm_sw_up_packet.sw_version[1] );

//    g_SubDevice[dev_num].SwCurVer = avc2ctm_sw_up_packet.sw_version[0]<<8;
//    g_SubDevice[dev_num].SwCurVer += avc2ctm_sw_up_packet.sw_version[1];
//    printf(" Compare SW version %d.%d %d.%d \r\n",
//    g_SubDevice[dev_num].SwCurVer>>8,g_SubDevice[dev_num].SwCurVer&0x00FF,
//    avc2ctm_sw_up_packet.sw_version[0], avc2ctm_sw_up_packet.sw_version[1] );
#if 1 //recieve ip valid check and sw version update
	for(dev_num = 0;dev_num< 147;dev_num++) {
		if((g_SubDevice[dev_num].DeviceAddr4 == avc2ctm_sw_up_packet.DeviceAddr4) && 
		  (g_SubDevice[dev_num].DeviceAddr3 == avc2ctm_sw_up_packet.DeviceAddr3)) {

            printf("jhlee RECIVE IP  -----------> %d.%d.%d.%d \r\n",
                   avc2ctm_sw_up_packet.DeviceAddr1   , avc2ctm_sw_up_packet.DeviceAddr2,
                   avc2ctm_sw_up_packet.DeviceAddr3 , avc2ctm_sw_up_packet.DeviceAddr4  );
            printf(" SW version %d.%d \r\n",avc2ctm_sw_up_packet.sw_version[0],
                    avc2ctm_sw_up_packet.sw_version[1] );

            g_SubDevice[dev_num].SwCurVer = avc2ctm_sw_up_packet.sw_version[0]<<8;
            g_SubDevice[dev_num].SwCurVer += avc2ctm_sw_up_packet.sw_version[1];
            printf(" Compare SW version %d.%d %d.%d \r\n",
            g_SubDevice[dev_num].SwCurVer>>8,g_SubDevice[dev_num].SwCurVer&0x00FF,
            avc2ctm_sw_up_packet.sw_version[0], avc2ctm_sw_up_packet.sw_version[1] );

            g_SubDevice[dev_num].Update_flag =avc2ctm_sw_up_packet.Update_flag;
            printf("avc2ctm_sw_up_packet.Update_flag %d \r\n",avc2ctm_sw_up_packet.Update_flag);
            g_SubDevice[dev_num].Ftpflag =avc2ctm_sw_up_packet.ftp_flag;
            printf("avc2ctm_sw_up_packet.Ftpflag %d \r\n",avc2ctm_sw_up_packet.ftp_flag);
        }

    }
#endif 	


    return ret ;
}			
			
			
int process_avc_tcp_data(char *buf, int len, int *avc_cmd_id)
{
    unsigned char cmd, req = 0;
    int frame_len;
    int cmd_id = 0;

    frame_len = buf[0];
    cmd = buf[1];

    switch (cmd) {
        case AVC_TCP_CMD_SW_UPDATE_ID: /* 0x13 */
            printf("\n< AVC CMD CODE: SW Update ");

            req = buf[2];

            if (req == AVC_TCP_CMD_SW_UPDATE_START) {
                printf("(START) >\n");
                cmd_id = SW_UPDATE_CMD_START;
            }
            else if (req == AVC_TCP_CMD_SW_UPDATE_VER_REQ) {
                printf("(VER REQ) >\n");
                cmd_id = SW_UPDATE_CMD_VER_REQ;
            }
            else if (req == AVC_TCP_CMD_SW_UPDATE_ICS_VER_REQ) {
                printf("(ICS VER REQ) >\n");
                cmd_id = SW_UPDATE_CMD_ICS_VER_REQ;
            }
            else {
                printf("(Unknown) >\n");
                cmd_id = 0;
            }

            break;

        case AVC_TCP_CMD_COP_CTRL_ID:
            if (len >= frame_len) {
                //printf("\n< AVC->COP CTRL CMD >\n");
                cmd_id = AVC_TCP_CMD_COP_CTRL_ID;
            }
            break;
        case AVC_TCP_CMD_COP_STATUS_ID:
            if (len >= frame_len) {

                cmd_id = AVC_TCP_CMD_COP_STATUS_ID;
            }
            break;
        case AVC_TCP_CMD_SUB_SW_UPDATE_ID:
            if (len >= frame_len) {

                cmd_id = AVC_TCP_CMD_SUB_SW_UPDATE_ID;
            }
            break;
        case AVC2CTM_EVENT_PACKET_ID:
            if (len >= frame_len) {

                cmd_id = AVC2CTM_EVENT_PACKET_ID;
            }
            break;
        default:
            printf("< AVC CMD CODE: unknow(%02X) >\n", cmd);
            frame_len = -1;
            break;
    }

    *avc_cmd_id = cmd_id;

    return frame_len;
}

void set_time_done_bit(void)
{
    time_set_done = 1;
}

void clear_time_done_bit(void)
{
    time_set_done = 0;
}

void increase_avcframe_err_and_send_enable(void)
{
    AvcFrameErrCounter++;
    AllToTcpSendEnabled = 1;
//printf(" AvcFrameErrCounter: %d\n", AvcFrameErrCounter);
}

int send_all_to_tcp_data_to_avc(void)
{
    int ret, len;

    if (!AllToTcpSendEnabled)
        return 0;

    len = make_all_to_avc_tcp_data(&TcpTxBuf[0]);

    ret = send_tcp_data_to_avc(&TcpTxBuf[0], len);
    if (ret > 0) {
        AllToAVC_HBC++;
        AllToTcpSendEnabled = 0;
    }

    return ret;
}

int send_tcp_data_to_avc(char *txbuf, int txlen)
{
    int ret;
    int return_len;
    fd_set rset, wset;
    struct timeval timeout;
    int numFds;
    char cmd_id = 0;

    if (avc_tcp_sock < 0) return -1;

    if (!txlen) return 0;

    return_len = 0;
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(avc_tcp_sock, &wset);
    numFds = avc_tcp_sock + 1;
    timeout.tv_sec = 0;
    //timeout.tv_usec = 1000;
    timeout.tv_usec = 50;
    ret = select(numFds, NULL/*&rset*/, &wset, NULL, &timeout);
#if 0
    //if(txlen > 8){
    if(txlen == CTM2AVC_SWUP_PACKET_SIZE){
        printf("-- send_tcp_data_to_avc, ret: %d txlen %d\n", ret,txlen);
        int i, k;
        printf("\n");
        printf("---- TCP send (COP --> AVC) ----\n   ");
        for(i=0; i < 10; i++) 
            printf("_%1X ", i);
        for (i = 0, k = 0; i < txlen; i++, k++) {
            //if ((k % 16) == 0)
            if ((k % 10) == 0)
                printf("\n%1x_ ", (k==0)?0:k/10);

            if ( (i == CTM2AVC_SWUP_PACKET_SIZE) || (i == (CTM2AVC_SWUP_PACKET_SIZE*2)) ) {
                printf("\n");
                printf("--------------------------------\n");
                printf("\n");
                printf("---- TCP SEND (COP--> AVC) ----");
                printf("\n0_ ");
                k = 0; 
            }    

            printf("%02X ",txbuf[i]);
        }    
        printf("\n");
        printf("--------------------------------\n");
    }    
#endif  
//printf("-- send_tcp_data_to_avc, ret: %d\n", ret);
    if (ret >= 0) {
        if (FD_ISSET(avc_tcp_sock, &wset)) {
            FD_CLR(avc_tcp_sock, &wset);
            //return_len = write(avc_tcp_sock, TcpTxBuf, TcpSend_ReadyLen);
            return_len = write(avc_tcp_sock, txbuf, txlen);
            if (return_len == txlen) {
                cmd_id = TcpTxBuf[1];
                if (cmd_id == 0x0E) {
                    printf("[ TCP UPDATE REPORT: ID:0x%02X, LEN:%d, STAT:0x%02X-0x%02X ]\n",
                    		TcpTxBuf[1], return_len, TcpTxBuf[2], TcpTxBuf[3]);
                }

                //printf("[ AVC TCP SEND: %d]\n", return_len);
                ret = return_len;
                //TcpSend_ReadyLen = 0;
            }
            else {
                printf("< ERROR: AVC TCP SEND: %d>\n", return_len);
                ret = -1;
            }
        }
    }

	if( internal_test_use && (txlen > 0) )
		make_qc_test_send_buf(txbuf, txlen, DEBUG_TCP_SEND);

    return ret;
}

static int make_all_to_avc_tcp_data(char *buf)
{
    int idx = 0;
    unsigned short crc16_val;

    buf[idx++] = 0;
    buf[idx++] = ALL_AVC_TCP_CMD_ERROR_ID;

    /* 2, 3 */
    if (SetDIErrCode) {
        buf[idx++] =  SetDIErrCode& 0xff;
        buf[idx++] = (SetDIErrCode >> 8) & 0xff;
    }
    else if (ClearDIErrCode) {
        buf[idx++] =  ClearDIErrCode& 0xff;
        buf[idx++] = (ClearDIErrCode >> 8) & 0xff;
    }
    else {
        buf[idx++] =  0;
        buf[idx++] = 0;
    }

    /* 4 */
    if (SetDIErrCode) {
        buf[idx++] = (1 << 0); /* ERROR OCCUR */
        SetDIErrCode = 0;
    }
    else if (ClearDIErrCode) {
        buf[idx++] = (1 << 1); /* ERROR CLEAR */
        ClearDIErrCode = 0;
    }
    else
        buf[idx++] = 0;

    /* 5 */
    buf[idx++] = (TestActive & 1) |  ((TestResult&1)<<1);

    /* 6, 7 */
    crc16_val = make_crc16((unsigned char *)buf, idx);
    buf[idx++] = crc16_val & 0xff;
    buf[idx++] = (crc16_val >> 8) & 0xff;

    buf[0] = idx;

    //TcpSend_ReadyLen = idx;
    return idx;
}

/******************************************************************************/
static char *update_ftp_status_str[] = {
    "",
    "FTP Connect OK",
    "FTP Connect ERR",
    "",
    "File Down OK",
    "",
    "",
    "",
    "File Down ERR",
};

static char *update_status_str[] = {
    "",
    "Update OK",
    "Update ERR",
};

void set_app_version(unsigned short entire_ver)
{
    Entire_Ver = entire_ver;
}

int send_update_result_to_avc(char login_stat, char down_stat, char update_stat)
{
    int ret, len, idx;
    char *str1 = NULL;
    char *str2 = NULL;

    len = make_update_result_tcp_data(&TcpTxBuf[0], login_stat, down_stat, update_stat);

    idx = TcpTxBuf[2];
    if (idx >= 0 && idx <= 8)
        str1 = update_ftp_status_str[idx];

    idx = TcpTxBuf[3];
    if (idx >= 0 && idx <= 2)
        str2 = update_status_str[idx];

    ret = send_tcp_data_to_avc(&TcpTxBuf[0], len);
    if (ret > 0)
        UpdateToAVC_HBC++;

    printf("< SEND UPDATE RESULT, FTP:%s, UPDATE:%s, Version:%d.%02d >\n",
        str1, str2, Entire_Ver >> 8, Entire_Ver & 0xff);

    return ret;
}

int send_ics_update_result_to_avc(unsigned short uboot_ver, unsigned short kernel_ver, unsigned short app_ver)
{
    int ret, len;
    int ver;

    len = make_update_result_tcp_data(&TcpTxBuf[0], 0, 0, 0);

    ver = (uboot_ver >> 8) * 100;
    ver += uboot_ver & 0xff;
    TcpTxBuf[6] = ver;

    ver = (kernel_ver >> 8) * 100;
    ver += kernel_ver & 0xff;
    TcpTxBuf[7] = ver;

    ver = (app_ver >> 8) * 100;
    ver += app_ver & 0xff;
    TcpTxBuf[8] = ver;

    ret = send_tcp_data_to_avc(&TcpTxBuf[0], len);
    if (ret > 0)
        UpdateToAVC_HBC++;

    return ret;
}

static int make_update_result_tcp_data(char *buf, char login_stat, char down_stat, char update_stat)
{
    int idx = 0;
    unsigned short crc16_val;

    buf[idx++] = 0;
    buf[idx++] = AVC_TCP_CMD_SW_UPDATE_REPORT_ID;

    buf[idx++] =  (down_stat & 0x0C) | (login_stat & 3);
    buf[idx++] = update_stat & 3;

    /* Software Version */
    buf[idx++] = Entire_Ver >> 8;
    buf[idx++] = Entire_Ver & 0xff;

#if 0
    buf[idx++] = UpdateToAVC_HBC & 0xff;
    buf[idx++] = (UpdateToAVC_HBC >> 8) & 0xff;
    buf[idx++] = (UpdateToAVC_HBC >> 16) & 0xff;
    buf[idx++] = (UpdateToAVC_HBC >> 24) & 0xff;
#else
    buf[idx++] = 0;
    buf[idx++] = 0;
    buf[idx++] = 0;
    buf[idx++] = 0;
#endif

    crc16_val = make_crc16((unsigned char *)buf, idx);

    buf[idx++] = crc16_val & 0xff;
    buf[idx++] = (crc16_val >> 8) & 0xff;

    buf[0] = idx;

    //TcpSend_ReadyLen = idx;
    return idx;
}

int get_ftp_remote_file_path(char *buf, char *get_path, int buflen)
{
    int i = AVC_TCP_CMD_SW_UPDATE_FILENAME_OFF;
    int maxl = AVC_TCP_CMD_SW_UPDATE_MAX_FILELEN;
    int len, idx = 0;

    for (; i < maxl; i++)
        get_path[idx++] = buf[i];
    get_path[idx++] = 0;

    len = strlen(get_path);

    printf("[ SW Update REMOTE PATH: (%s) ]\n", get_path);

    return len;
}

int check_my_ip(void)
{
    int err = 0;
    unsigned int my_network_address;
    unsigned int my_ip_group_id;
    unsigned int my_host_value;
    unsigned int di_group_id = 0;

    di_group_id = COP_PRODUCT_CLASS;

    my_network_address = (my_ip_value[0] << 8) | my_ip_value[1];
    my_host_value = (my_ip_value[2] << 8) | my_ip_value[3];

    my_ip_group_id = my_host_value & IP_PRODUCT_MASK;
    my_ip_group_id >>= IP_PRODUCT_SHIFT_BIT;

    if (    (my_network_address != IP_NETWORK_CLASS)
         || (my_ip_group_id != di_group_id) 
       )
    {
	printf("< ERROR, PRODUCT CLASS(0x%02X:0x%02X) > \n", my_ip_group_id, di_group_id);
        err = -1;
    }
    else {
        printf("\n[ MY PRODUCT ID: COP(0x%02X) ]\n", my_ip_group_id);
    }

    return err;
}

