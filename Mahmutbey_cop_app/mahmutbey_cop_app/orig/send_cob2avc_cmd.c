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

#include "avc_tcp.h"
#include "crc16.h"
#include "gpio_key_dev.h"
#include "cob_process.h"
#include "audio_multicast.h"

static int inout_key_pushed_status = 0;
int passive_in_button;
int passive_out_button;

static void print_cop2avc_packet(packet_COP2AVC_t *packet);

int send_cop2avc_packet_init(void)
{
    inout_key_pushed_status = INOUT_BUTTON_STATUS_ALLOFF; // 1
    return 0;
}

#define BUTTON_ON       1
#define BUTTON_OFF      0

void update_inout_key_status(int whaton, char *where)
{
    inout_key_pushed_status = whaton;

//printf("%s, %d\n", __func__, whaton);

    switch (whaton) {
        case INOUT_BUTTON_STATUS_ALLOFF: //1
            printf("< LED IN/OUT BUTTON OFF(%s)>\n", where);
            clear_key_led(enum_COP_KEY_PA_IN);
            clear_key_led(enum_COP_KEY_PA_OUT);
            passive_in_button = BUTTON_OFF;
            passive_out_button = BUTTON_OFF;
            break;

        case INOUT_BUTTON_STATUS_INOUT_ON: //2
            printf("< LED IN/OUT BUTTON ON(%s) >\n", where);
            set_key_led(enum_COP_KEY_PA_IN);
            set_key_led(enum_COP_KEY_PA_OUT);
            passive_in_button = BUTTON_ON;
            passive_out_button = BUTTON_ON;
            break;

        case INOUT_BUTTON_STATUS_IN_ON: //3
            printf("< LED IN BUTTON ON(%s) >\n", where);
            set_key_led(enum_COP_KEY_PA_IN);
            clear_key_led(enum_COP_KEY_PA_OUT);
            passive_in_button = BUTTON_ON;
            passive_out_button = BUTTON_OFF;
            break;

        case INOUT_BUTTON_STATUS_OUT_ON: //4
            printf("< LED OUT BUTTON ON(%s) >\n", where);
            clear_key_led(enum_COP_KEY_PA_IN);
            set_key_led(enum_COP_KEY_PA_OUT);
            passive_in_button = BUTTON_OFF;
            passive_out_button = BUTTON_ON;
            break;
    }
}

static void cop2avc_packet_last_fill(packet_COP2AVC_t *packet, unsigned short status)
{
    unsigned short crc16;
    unsigned char *buf;

    packet->length = COP2AVC_PACKET_SIZE;
    packet->id = COP2AVC_PACKET_ID;

    buf = (unsigned char *)packet;

    crc16 = make_crc16(buf, COP2AVC_PACKET_SIZE-2);
    packet->crc16 = crc16;
}

/******* INDOOR START /STOP *************************************************/
int send_cop2avc_cmd_in_door_start_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.in_out_flag = inout_key_pushed_status;
    cop2avc_packet.BCastReq.IndoorBCast.start = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_in_door_stop_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.in_out_flag = inout_key_pushed_status;
    cop2avc_packet.BCastReq.IndoorBCast.stop = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}
/*****************************************************************************/


/******* OUTDOOR START /STOP *************************************************/
int send_cop2avc_cmd_out_door_start_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.in_out_flag = inout_key_pushed_status;
    cop2avc_packet.BCastReq.OutdoorBCast.start = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_out_door_stop_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.in_out_flag = inout_key_pushed_status;
    cop2avc_packet.BCastReq.OutdoorBCast.stop = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}
/*****************************************************************************/


/******* OCC PA START /STOP *************************************************/
int send_cop2avc_cmd_occ_pa_start_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.OCCPABroadCast.start = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_occ_pa_stop_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.OCCPABroadCast.stop = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}
/*****************************************************************************/


static void rx_tx_mon_multi_addr_fill(packet_COP2AVC_t *packet)
{
#if 0
    unsigned int rx_addr;
    unsigned int tx_addr;
    unsigned int mon_addr;

    rx_addr = __get_rx_multi_addr();
    tx_addr = __get_tx_multi_addr();
    mon_addr = __get_mon_multi_addr();

    if (rx_addr || tx_addr || mon_addr) {
        packet->multi_addr_valid = COP2AVC_MULTICAST_ADDR_VALID;
//printf("%s, CHK, rx_addr: 0x%X\n", __func__, rx_addr);
        packet->RxMultiAddr = rx_addr;
        packet->TxMultiAddr = tx_addr;
        packet->MonMultiAddr = mon_addr;
    }
#endif
}

/******* FIRE ALARAM STATUS *******************************************************/
int send_cop2avc_tractor_start_flag(COP_Cur_Status_t __broadcast_status, unsigned short status)
{
	int ret = 0;
	packet_COP2AVC_t cop2avc_packet;

	memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	cop2avc_packet.CallReq.active = 1;
	cop2avc_packet.CallReq.cab_call.tractor_start = 1;
	cop2avc_packet.CallReq.cab_call.tractor_stop = 0;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

	ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

	return ret;
}

int send_cop2avc_tractor_stop_flag(COP_Cur_Status_t __broadcast_status, unsigned short status)
{
	int ret = 0;
	packet_COP2AVC_t cop2avc_packet;

	memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	cop2avc_packet.CallReq.active = 1;
	cop2avc_packet.CallReq.cab_call.tractor_start = 0;
	cop2avc_packet.CallReq.cab_call.tractor_stop = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

	ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

	return ret;
}

/******* FIRE ALARAM STATUS *******************************************************/
int send_cop2avc_fire_flag(COP_Cur_Status_t __broadcast_status, unsigned short status)
{
	int ret = 0;
	packet_COP2AVC_t cop2avc_packet;

	memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

	ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

	return ret;
}

/******* FIRE ALARAM STATUS *******************************************************/
int send_cop2avc_vol_info(COP_Cur_Status_t __broadcast_status, char in_vol, char out_vol)
{
	int ret = 0;
	packet_COP2AVC_t cop2avc_packet;

	memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	cop2avc_packet.vol.in_vol_chnage = 1;
	cop2avc_packet.vol.out_vol_change = 1;
	cop2avc_packet.indoor_vol = in_vol;
	cop2avc_packet.outdoor_vol = out_vol;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);
	ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);
	return ret;
}

/******* CAB2CAB CALL START /STOP *************************************************/
int send_cop2avc_cmd_cab2cab_call_start_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;

    packet_COP2AVC_t cop2avc_packet;
    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.cab_call.start = 1;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);
	
    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

    return ret;
}

int send_cop2avc_cmd_cab2cab_call_accept_request(COP_Cur_Status_t __broadcast_status,
                                        unsigned char car_num, unsigned char dev_num)
{
    int ret = 0;

    packet_COP2AVC_t cop2avc_packet;

    if ((car_num < 0) || (car_num > MAX_CAR_NUM))
        car_num = 0;
    if ((dev_num < 0) || (dev_num > MAX_CAB_ID_NUM))
        dev_num = 0;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.cab_call.accept = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_cab2cab_call_reject_request(COP_Cur_Status_t __broadcast_status,
                                       unsigned char car_num, unsigned char dev_num)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    if ((car_num < 0) || (car_num > MAX_CAR_NUM))
        car_num = 0;
    if ((dev_num < 0) || (dev_num > MAX_CAB_ID_NUM))
        dev_num = 0;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.cab_call.reject = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_cab2cab_call_stop_request(COP_Cur_Status_t __broadcast_status,
                                         unsigned char car_num, unsigned char dev_num)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    if ((car_num < 0) || (car_num > MAX_CAR_NUM))
        car_num = 0;
    if ((dev_num < 0) || (dev_num > MAX_CAB_ID_NUM))
        dev_num = 0;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.cab_call.stop = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

    return ret;
}
/*****************************************************************************/

/******* Every Broadcast STOP ************************************************/
int send_cop2avc_cmd_broadcast_stop_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.FuncBCast.broadcast_stop = 1;

	if(get_func_is_now_start() > 0)
	{
		cop2avc_packet.BCastReq.funcNum = get_func_is_now_start();
	}

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

    return ret;

}


/*****************************************************************************/

/******* Function Bcast START/STOP *************************************************/
int send_cop2avc_cmd_func_bcast_start_request (COP_Cur_Status_t __broadcast_status, int func_option, int funcnumber, unsigned char is_station_fpa)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.FuncBCast.start = 1;
	cop2avc_packet.BCastReq.FuncBCast.indoor = ((func_option >> FUNC_IN_OFFSET) & 0x1);
	cop2avc_packet.BCastReq.FuncBCast.outdoor = ((func_option >> FUNC_OUT_OFFSET) & 0x1);
	cop2avc_packet.BCastReq.FuncBCast.lrm = ((func_option >> FUNC_LRM_OFFSET) & 0x1);
	cop2avc_packet.BCastReq.FuncBCast.pib = ((func_option >> FUNC_PIB_OFFSET) & 0x1);
	cop2avc_packet.BCastReq.FuncBCast.is_station_fpa = is_station_fpa;
	//cop2avc_packet.BCastReq.FuncBCast.door = (func_option & 0x01);
	//cop2avc_packet.BCastReq.FuncBCast.arrive = ((func_option & 0x02)>>1);
	//cop2avc_packet.BCastReq.FuncBCast.arrive_broadcast = ((func_option & 0x04)>>2);
	//cop2avc_packet.BCastReq.FuncBCast.departure_broadcast = ((func_option & 0x08)>>3);
    cop2avc_packet.BCastReq.funcNum = funcnumber;
    cop2avc_packet.BCastReq.in_out_flag = 0;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_func_bcast_stop_request (COP_Cur_Status_t __broadcast_status, int func_option, int funcnumber, unsigned char is_station_fpa)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.BCastReq.active = 1;
    cop2avc_packet.BCastReq.FuncBCast.stop = 1;
	cop2avc_packet.BCastReq.FuncBCast.indoor = ((func_option >> FUNC_IN_OFFSET) & 0x1);
	cop2avc_packet.BCastReq.FuncBCast.outdoor = ((func_option >> FUNC_OUT_OFFSET) & 0x1);
	cop2avc_packet.BCastReq.FuncBCast.is_station_fpa = is_station_fpa;
	//cop2avc_packet.BCastReq.FuncBCast.door = (func_option & 0x01);
	//cop2avc_packet.BCastReq.FuncBCast.arrive = ((func_option & 0x02)>>1);
	//cop2avc_packet.BCastReq.FuncBCast.arrive_broadcast = ((func_option & 0x04)>>2);
	//cop2avc_packet.BCastReq.FuncBCast.departure_broadcast = ((func_option & 0x08)>>3);
    cop2avc_packet.BCastReq.funcNum = funcnumber;
    cop2avc_packet.BCastReq.in_out_flag = 0;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

    return ret;
}
/***********************************************************************/

/******* PEI START /STOP *************************************************/

int send_cop2avc_cmd_pei2cab_call_start_request(COP_Cur_Status_t __broadcast_status,
                               unsigned char car_num, unsigned char dev_num, int is_emergency, int is_join)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.pei_call.start = 1;
    if (is_emergency)
        cop2avc_packet.CallReq.pei_call.emergency_en = 1;
	if (is_join)
		cop2avc_packet.CallReq.pei_call.join = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_pei2cab_call_stop_request(COP_Cur_Status_t __broadcast_status,
                       unsigned char car_num, unsigned char dev_num, int is_emergency, int same_id)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.pei_call.stop = 1;
    if (is_emergency)
        cop2avc_packet.CallReq.pei_call.emergency_en = 1;
    if (same_id)
        cop2avc_packet.CallReq.pei_call.same_id = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_pei2cab_can_talk_request(COP_Cur_Status_t __broadcast_status,
                                       unsigned char car_num, unsigned char dev_num)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.occ_call.can_talk = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

	print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_pei2cab_cannot_talk_request(COP_Cur_Status_t __broadcast_status,
                                       unsigned char car_num, unsigned char dev_num)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.occ_call.cannot_talk = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_pei2cab_can_talk_and_receive_request(COP_Cur_Status_t __broadcast_status,
                                       unsigned char car_num, unsigned char dev_num)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

    cop2avc_packet.CallReq.active = 1;
    cop2avc_packet.CallReq.occ_call.can_talk_and_receive = 1;
    cop2avc_packet.CallReq.car_number = car_num;
    cop2avc_packet.CallReq.dev_number = dev_num;

    rx_tx_mon_multi_addr_fill(&cop2avc_packet);

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);

print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

/*****************************************************************************/

/******* DI MANUAL DISPLAY / SET ROUTE ***************************************/
int send_cop2avc_cmd_di_display_start_request(COP_Cur_Status_t __broadcast_status, unsigned int id)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	cop2avc_packet.UiFlag = 0x2;
	cop2avc_packet.di_display_id = id;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);
    print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_di_display_stop_request(COP_Cur_Status_t __broadcast_status)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	cop2avc_packet.UiFlag = 0x4;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);
    print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_set_route_request(COP_Cur_Status_t __broadcast_status,
                            unsigned int departure_id, unsigned int destination_id)//, unsigned int via)
{
    int ret = 0;
    packet_COP2AVC_t cop2avc_packet;

    memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	cop2avc_packet.UiFlag = 0x1;

	//if (via) cop2avc_packet.UiFlag |= 0x10;

	cop2avc_packet.departure_id = departure_id;
	cop2avc_packet.destination_id = destination_id;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);
    print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

int send_cop2avc_cmd_set_special_route_request(COP_Cur_Status_t __broadcast_status, unsigned short pattern_num, unsigned char is_start)
{
	int ret = 0;
	packet_COP2AVC_t cop2avc_packet;

	memset(&cop2avc_packet, 0, sizeof(packet_COP2AVC_t));

	//cop2avc_packet.UiFlag = 0x1;
	if(is_start)
		cop2avc_packet.UiFlag |= 0x20;
	else //stop
		cop2avc_packet.UiFlag |= 0x40;

	cop2avc_packet.route_pattern = pattern_num;

	cop2avc_packet_last_fill(&cop2avc_packet, 0);

    ret = send_tcp_data_to_avc((char *)&cop2avc_packet, COP2AVC_PACKET_SIZE);
    print_cop2avc_packet(&cop2avc_packet);

    return ret;
}

/*****************************************************************************/

int send_all2avc_cycle_data(char *buf, int maxlen, unsigned short errcode, int errset, int errclear)
{
    int ret = 0, idx = 0;
    unsigned short crc16;

    if (maxlen < 8)
        return -1;

    buf[idx++] = 8;
    buf[idx++] = 0x02;

    /* 2, 3 */
    buf[idx++] = errcode & 0xff;
    buf[idx++] = (errcode >> 8);

    /* 4 */
    if (errset && !errclear)
        buf[idx++] = 1;
    else if (!errset && errclear)
        buf[idx++] = 2;
    else
        buf[idx++] = 0;

    /* 5 */
    buf[idx++] = 0;

    crc16 = make_crc16((unsigned char *)buf, idx);
    buf[idx++] = crc16 & 0xff;
    buf[idx++] = (crc16 >> 8);

    if (errcode)
        printf("<< SEND ERRCODE: %d, set:%d, clear:%d >>\n", errcode, errset, errclear);

    ret = send_tcp_data_to_avc(buf, idx);

    return ret;
}

static void print_cop2avc_packet(packet_COP2AVC_t *packet)
{
    unsigned char *buf;
    //int i;
    int active;

	return;

    buf = (unsigned char *)packet;
    printf("\n-------------- COP->AVC [%d %d] ------------------------\n", buf[0], buf[1]);

    active = buf[2];
    if (active == 1) {
        if (buf[3] || buf[4]) {
            printf("Func Num: %d\n", ((buf[4]<<8)|buf[3]));
        	printf("Func Control : 0x%02X\n", buf[5]);
		}
        switch (buf[5] & 0x03) {
            case 1: printf("Func BCast: Start Req\n"); break;
            case 2: printf("Func BCast: Stop Req\n"); break;
        }
		/*
		switch (buf[5] & 0xF0) {
			case 0x80: printf("Departure Broadcast Req\n"); break;
			case 0x60: printf("Arrive Broadcast Req(Arrived)\n"); break;
			case 0x50: printf("Arrive Broadcast Req(Door)\n"); break;
		}
		*/
        if (buf[6])
            printf("IN/OUT Flag: %d\n", buf[6]);
        switch (buf[7]) {
            case 1: printf("IN BCast: Start Req\n"); break;
            case 2: printf("IN BCast: Stop Req\n"); break;
        }
        switch (buf[8]) {
            case 1: printf("OUT BCast: Start Req\n"); break;
            case 2: printf("OUT BCast: Stop Req\n"); break;
        }
        switch (buf[9]) {
            case 1: printf("OCC PA: Start Req\n"); break;
            case 2: printf("OCC PA: Stop Req\n"); break;
        }
    }

    active = buf[10];
    if (active == 1) {
        switch (buf[11]) {
            case 1: printf("CAB CALL: Start Req\n"); break;
            case 2: printf("CAB CALL: Stop Req\n"); break;
            case 4: printf("CAB CALL: Accept\n"); break;
            case 8: printf("CAB CALL: Reject\n"); break;
        }
        switch (buf[12]) {
            case 1: printf("PEI CALL: Start Req\n"); break;
            case 2: printf("PEI CALL: Stop Req\n"); break;
            case 0x42: printf("PEI CALL: Stop Req(SAME ID)\n"); break;
            case 0x81: printf("PEI CALL: Emergency Start Req\n"); break;
            case 0x82: printf("PEI CALL: Emergency Stop Req\n"); break;
            default: if (buf[12]) printf("PEI CALL: 0x%X\n", buf[12]); break;
        }
        switch (buf[13]) {
            case 1: printf("OCC CALL: Can talk\n"); break;
            case 2: printf("OCC CALL: Cannot talk\n"); break;
            case 4: printf("OCC CALL: Can talk and receive\n"); break;
        }
    }
    if (buf[14] || buf[15])
        printf("Device: %d-%d\n", buf[14], buf[15]);
    //printf("Status: %d\n", buf[16] + (buf[17]<<8));

    printf("-------------------------------------------------\n");
}

