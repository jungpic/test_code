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

#define ETH_RX_MAXBUF					1024
#define DUMMY_ADD_BUFLEN				2048
#define MAX_BUF_LEN						(ETH_RX_MAXBUF + DUMMY_ADD_BUFLEN)

#define MAX_STATION_NAME				2048

static char 							pid_recv_buf[MAX_BUF_LEN];

static int					pid_recv_sock;

static struct sockaddr_in	pid_recv_srv;
static socklen_t			pid_recv_addrSize;

char Tmp_Str[MAX_STATION_NAME];
char Departure_Name_Turkish[MAX_STATION_NAME];
char Departure_Name_English[MAX_STATION_NAME];
char Current_Name_Turkish[MAX_STATION_NAME];
char Current_Name_English[MAX_STATION_NAME];
char Next_1_Name_Turkish[MAX_STATION_NAME];
char Next_1_Name_English[MAX_STATION_NAME];
char Next_2_Name_Turkish[MAX_STATION_NAME];
char Next_2_Name_English[MAX_STATION_NAME];
char Destination_Name_Turkish[MAX_STATION_NAME];
char Destination_Name_English[MAX_STATION_NAME];

void init_pid_multicast(void)
{
	if(pid_recv_sock>= 0)
		close(pid_recv_sock);

    pid_recv_sock = -1;
}

int connect_pid_multicast_recv(char *ip_addr, unsigned short port)
{
    struct ip_mreq pid_recv_mreq;

	int flag, ret;

	bzero(&pid_recv_srv, sizeof(pid_recv_srv));
	pid_recv_srv.sin_family = AF_INET;
	pid_recv_srv.sin_port = htons(port);
	if (inet_aton(ip_addr, &pid_recv_srv.sin_addr) < 0) {
		printf("ERROR, PID RX, inet_aton");
		return -1; 
	}   
		
	if ((pid_recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		pid_recv_sock = -1;
		printf("ERROR, QC TEST RX, socket");
		return -1; 
	}   

	flag = fcntl(pid_recv_sock, F_GETFL);
	ret = fcntl(pid_recv_sock, F_SETFL, flag | O_NONBLOCK);
	if(ret == -1){
		close(pid_recv_sock);
		pid_recv_sock = -1;
		printf("< PID RX, FCNTL ERROR >\n");
		return -1; 
	}   
	
	if (bind(pid_recv_sock, (struct sockaddr *)&pid_recv_srv, sizeof(pid_recv_srv)) < 0) {
		close(pid_recv_sock);
		pid_recv_sock = -1;
		printf("ERROR, PID RX, bind");
		return -1; 
	}   
		
	if (inet_aton(ip_addr, &pid_recv_mreq.imr_multiaddr) < 0) {
		close(pid_recv_sock);
		pid_recv_sock = -1;
		printf("PID MSG, inet_aton");
		return -1; 
	}   

	pid_recv_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(pid_recv_sock, IPPROTO_IP,IP_ADD_MEMBERSHIP,&pid_recv_mreq,sizeof(pid_recv_mreq)) < 0) {
		close(pid_recv_sock);
		pid_recv_sock = -1;
		printf("ERROR, PID RX, setsockopt");
		return -1; 
	}   
	
	printf("< PID RX Multi Connected: %s(sock:%d) > \n", ip_addr, pid_recv_sock);
		
	return 0;
}


int close_pid_multicast_recv(void)
{
    if (pid_recv_sock < 0)
        return -1;

    close(pid_recv_sock);
	pid_recv_sock = -1;

    return 0;
}

int try_read_pid_msg(void)
{
	int ret = 0;
	int rx_len = 0;

	errno = 0;

	if(pid_recv_sock < 0) {
		printf("%s-%s[%d] : -ERR- : pid_recv_sock(%d) is not opened.\n", __FILE__, __FUNCTION__, __LINE__, pid_recv_sock);
		return 0;
	}

	rx_len = recv(pid_recv_sock, pid_recv_buf, ETH_RX_MAXBUF, 0);

	if(rx_len == -1){
		if((errno != EAGAIN) && (errno != EWOULDBLOCK)){
			printf("<< QC TEST SOCK %d RECV ERROR, errno: %d >>\n", pid_recv_sock, errno);
		}
	}
	if (rx_len <= 0)
		return 0;

	ret = check_pid_data_valid(pid_recv_buf, rx_len);
	if (ret != 0)
		rx_len = -1;


	return rx_len;
}

#define PID_MSG_STRING_OFF		7

int check_pid_data_valid(char *buf, int maxlen)
{
	char	string_type, icon_num;
	unsigned short tmp;
	unsigned int len;
	int idx = 0;
	int error = 0;
	int remainlen = maxlen;
    int i;

	tmp = (buf[1]<<8) | buf[0];
	if (tmp != maxlen) {
		printf("Lenth is wrong => len(%d), BUF[1:0](%d)\n", maxlen, tmp);
		return -1;
	}

	//printf("Lenth is => len(%d), BUF[1:0](%d)\n", maxlen, tmp);

	if (buf[2] != 0x50) {
		printf("Command ID is wrong => ID[%d]\n", buf[2]);
		return -1;
	}

    memset(&Tmp_Str[0], 0x00, MAX_STATION_NAME);
    memset(&Departure_Name_Turkish[0], 0x00, MAX_STATION_NAME);
    memset(&Departure_Name_English[0], 0x00, MAX_STATION_NAME);
    memset(&Destination_Name_Turkish[0], 0x00, MAX_STATION_NAME);
    memset(&Destination_Name_English[0], 0x00, MAX_STATION_NAME);
    memset(&Current_Name_Turkish[0], 0x00, MAX_STATION_NAME);
    memset(&Current_Name_English, 0x00, MAX_STATION_NAME);
    memset(&Next_1_Name_Turkish, 0x00, MAX_STATION_NAME);
    memset(&Next_1_Name_English, 0x00, MAX_STATION_NAME);
    memset(&Next_2_Name_Turkish, 0x00, MAX_STATION_NAME);
    memset(&Next_2_Name_English, 0x00, MAX_STATION_NAME);

	idx = PID_MSG_STRING_OFF;
	remainlen -= PID_MSG_STRING_OFF + 2 + 4;		// real data length	(Frame_Length:2bytes, Cmd_code:1byte, HBC:4bytes, CRC:2 bytes)

	while(remainlen > 0) {
		string_type = buf[idx++];		// buf[7];
		remainlen--;

        if( (string_type >= 1 && string_type <= 0x14) || string_type == 0x20 || string_type == 0x30 || string_type == 0x31) {
			if(string_type >= 1 && string_type <= 0x14) {
				icon_num = buf[idx++];
				len = buf[idx++];
				len += buf[idx++] << 8;
				remainlen -= 4;
			} else if (string_type == 30) {
				len = buf[idx++];
				len += buf[idx++] << 8;
				remainlen -= 3;
			} else {
				len = buf[idx++];
				remainlen -= 2;
			}

            if( (len == 0) || (len > (MAX_STATION_NAME-1)) ) {	// more than 255 bytes
				printf("<ERROR, LEN Field of PID MSG : %d, String_Type(0x%02X) >\n", len, string_type);
				error = 1;
				break;
			}

			for (i=0; i<len; i++)
				Tmp_Str[i] = buf[idx++];
			Tmp_Str[i] = 0;

			remainlen -= len;

		} else {
			//printf("string_type is not supported..[0x%02X]\n", string_type);
			error = 1;
		}

		if(error)
			break;


		switch (string_type) {
			case 0x01:
                //printf("[ Departure Turkish Name : (%s) ]\n", Tmp_Str);
				memcpy(&Departure_Name_Turkish[0], &Tmp_Str[0], len);
				break;
			case 0x02:
                //printf("[ Departure English Name : (%s) ]\n", Tmp_Str);
				memcpy(&Departure_Name_English[0], &Tmp_Str[0], len);
				break;
			case 0x03:
                //printf("[ Destination Turkish Name : (%s) ]\n", Tmp_Str);
				memcpy(&Destination_Name_Turkish[0], &Tmp_Str[0], len);
				break;
			case 0x04:
                //printf("[ Destination English Name : (%s) ]\n", Tmp_Str);
				memcpy(&Destination_Name_English[0], &Tmp_Str[0], len);
				break;
			case 0x07:
                //printf("[ Current Turkish Name : (%s) ]\n", Tmp_Str);
				memcpy(&Current_Name_Turkish[0], &Tmp_Str[0], len);
				break;
			case 0x08:
                //printf("[ Current English Name : (%s) ]\n", Tmp_Str);
				memcpy(&Current_Name_English[0], &Tmp_Str[0], len);
				break;
			case 0x09:
                //printf("[ Next Turkish Name : (%s) ]\n", Tmp_Str);
				memcpy(&Next_1_Name_Turkish[0], &Tmp_Str[0], len);
				break;
			case 0x0A:
                //printf("[ Next  Turkish Name : (%s) ]\n", Tmp_Str);
				memcpy(&Next_1_Name_English[0], &Tmp_Str[0], len);
				break;
			case 0x0B:
                //printf("[ After Next Turkish Name : (%s) ]\n", Tmp_Str);
				memcpy(&Next_2_Name_Turkish[0], &Tmp_Str[0], len);
				break;
			case 0x0C:
                //printf("[ After Next  English Name : (%s) ]\n", Tmp_Str);
				memcpy(&Next_2_Name_English[0], &Tmp_Str[0], len);
				break;

			default:
				//printf("string_type is not supported..[0x%02X]\n", string_type);
				break;
		}
	}

	if(error)
		return -1;

	return 0;
}
