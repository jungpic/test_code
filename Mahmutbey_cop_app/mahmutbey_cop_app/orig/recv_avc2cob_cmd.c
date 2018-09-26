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

int check_avc2cop_cmd(char *buf, int len, COP_Cur_Status_t __broadcast_status, int data)
{
    int ret = -1;
    packet_AVC2COP_t *avc2cop_packet;

    if (len == 0)
        return -1;

    if (len > sizeof(packet_AVC2COP_t)) {
        printf("%s(), size error got %d.\n", __func__, len);
        return -1;
    }

    //memcpy((char *)&avc2cop_packet, buf, len);
    avc2cop_packet = (packet_AVC2COP_t *)buf;

    if (avc2cop_packet->length != AVC2COP_PACKET_SIZE) {
        printf("check_avc2cop_cmd(), size error.\n");
        return -1;
    }

    if (avc2cop_packet->id != AVC2COP_PACKET_ID) {
        printf("check_avc2cop_cmd(), id error.\n");
        return -1;
    }

    switch(__broadcast_status) {
      case AUTO_BCAST_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.Auto_Func_BCast.auto_start == 1
            && (!buf[7] && !buf[8] && !buf[9])
           )
           ret = 0;
        break;

      case AUTO_BCAST_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.Auto_Func_BCast.auto_stop == 1
            && (!buf[7] && !buf[8] && !buf[8])
           )
           ret = 0;
        break;

      case FUNC_BCAST_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.Auto_Func_BCast.func_start == 1
            && (!buf[7] && !buf[8] && !buf[9])
           )
           ret = 0;
        break;

      case FUNC_BCAST_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.Auto_Func_BCast.func_stop == 1
            && (!buf[7] && !buf[8] && !buf[9])
           )
           ret = 0;
        break;

      case FUNC_BCAST_REJECT:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.Auto_Func_BCast.func_reject == 1
            && (!buf[7] && !buf[8] && !buf[9])
           )
           ret = 0;
        break;

      case __PASSIVE_INDOOR_BCAST_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.IndoorBCast.start == 1
            && (!buf[5] && !buf[9])
           )
        {
             cab_pa_in_set_for_key();
             ret = 0;
        }
        break;

      case __PASSIVE_INDOOR_BCAST_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.IndoorBCast.stop == 1
            && (!buf[5] && !buf[9])
           )
        {
            cab_pa_in_clear_for_key();
            ret = 0;
        }
        break;

      case __PASSIVE_OUTDOOR_BCAST_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.OutdoorBCast.start == 1
            && (!buf[5] && !buf[9])
           )
        {
            cab_pa_out_set_for_key();
            ret = 0;
        }
        break;

      case __PASSIVE_OUTDOOR_BCAST_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.OutdoorBCast.stop == 1
            && (!buf[5] && !buf[9])
           )
        {
            cab_pa_out_clear_for_key();
            ret = 0;
        }
        break;

      case __PASSIVE_INOUTDOOR_BCAST_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.IndoorBCast.start == 1
            && avc2cop_packet->BCastReq.OutdoorBCast.start == 1
            && (!buf[5] && !buf[9])
           )
        {
            cab_pa_in_set_for_key();
            cab_pa_out_set_for_key();
            ret = 0;
        }
        break;

      case PASSIVE_INOUTDOOR_BCAST_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.IndoorBCast.stop == 1
            && avc2cop_packet->BCastReq.OutdoorBCast.stop == 1
            && (!buf[5] && !buf[9])
           )
        {
            cab_pa_in_clear_for_key();
            cab_pa_out_clear_for_key();
            ret = 0;
        }
        break;

      case OCC_PA_BCAST_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.OCCPABCast.start == 1
            && (!buf[5] && !buf[7] && !buf[8])
           )
           ret = 0;
        break;

      case OCC_PA_BCAST_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.OCCPABCast.stop == 1
            && (!buf[5] && !buf[7] && !buf[8])
           )
           ret = 0;
        break;

      case OCCPA_BCAST_STATUS_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.Auto_Func_BCast.occpa_start == 1
            && (!buf[6] && !buf[7] && !buf[8] && !buf[9])
           )
           ret = 0;
        break;

      case OCCPA_BCAST_STATUS_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 1
            && avc2cop_packet->CallReq.active == 0
            && avc2cop_packet->BCastReq.Auto_Func_BCast.occpa_stop == 1
            && (buf[6] == 0 && buf[7] == 0 && buf[8] == 0 && buf[9] == 0)
           )
               ret = 0;
        break;

      case __PEI2CAB_CALL_WAKEUP:
        if ((avc2cop_packet->BCastReq.active & 1) == 0 && avc2cop_packet->CallReq.active == 1) {
	    if (   (avc2cop_packet->CallReq.pei_call.wake_up == 1)
	        /*&& (avc2cop_packet->CallReq.pei_call.emergency_en == 0) */ //delete, Ver0.78
	        && (avc2cop_packet->CallReq.cab_call.wake_up == 0)
               )
	        ret = 0;
        }
        break;

      case __PEI2CAB_CALL_OCC:
        if ((avc2cop_packet->BCastReq.active & 1) == 0 && avc2cop_packet->CallReq.active == 1) {
	    	if (avc2cop_packet->CallReq.pei_call.standby == 1)
	        	ret = 0;
        }
        break;

      case __PEI2CAB_CALL_START:
        if ((avc2cop_packet->BCastReq.active & 1) == 0 && avc2cop_packet->CallReq.active == 1) {
	    	if (   (avc2cop_packet->CallReq.pei_call.start == 1)
	    	    /* && (avc2cop_packet->CallReq.pei_call.emergency_en == 0) */ // delete, Ver0.78
               )
            {
                cab_call_pei_set_for_key();
	    	    ret = 0;
            } else if (avc2cop_packet->CallReq.pei_call.reject == 1) {
	        	ret = -1;
			}
        }
        break;

#if 0 //delete, 2014,08,11
      case __PEI2CAB_CALL_EMER_WAKEUP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.pei_call.emergency_en == 1
	    && avc2cop_packet->CallReq.pei_call.wake_up == 1
	    && avc2cop_packet->CallReq.cab_call.wake_up == 0
           )
	    ret = 0;
        break;

      case __PEI2CAB_CALL_EMER_START:
        if ((avc2cop_packet->BCastReq.active & 1) == 0 && avc2cop_packet->CallReq.active == 1) {
	    if (   (avc2cop_packet->CallReq.pei_call.emergency_en == 1)
	        && (avc2cop_packet->CallReq.pei_call.start == 1)
               )
            {
                cab_call_pei_set_for_key();
	        ret = 0;
            }
        }
        break;
#else
      case __PEI2CAB_CALL_EMER_EN:
        if (avc2cop_packet->CallReq.pei_call.emergency_en == 1)
            ret = 0;
        break;
#endif

      case __PEI2CAB_CALL_STOP:
        if ((avc2cop_packet->BCastReq.active & 1) == 0 && avc2cop_packet->CallReq.active == 1) {
	    if (   (avc2cop_packet->CallReq.pei_call.stop == 1)
	        /* && (avc2cop_packet->CallReq.pei_call.emergency_en == 0) */ //delete, Ver0.78
               )
            {
                cab_call_pei_clear_for_key();
	        ret = 0;
            }
        }
        break;

#if 0
      case __PEI2CAB_CALL_EMER_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.pei_call.stop == 1
	    && avc2cop_packet->CallReq.pei_call.emergency_en == 1
           )
        {
            cab_call_pei_clear_for_key();
	    ret = 0;
        }
        break;
#endif

      case PEI_CALL_REJECT:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.pei_call.reject == 1
           )
	    ret = 0;
        break;

      case __CAB2CAB_CALL_START:
        if ((avc2cop_packet->BCastReq.active & 1) == 0 && avc2cop_packet->CallReq.active == 1) {
	    if (avc2cop_packet->CallReq.cab_call.start == 1)
            {
                cab_call_cab_set_for_key();
	        ret = 0;
            }
	    else if (avc2cop_packet->CallReq.cab_call.reject == 1)
	        ret = -1;
        }
        break;

#if 0
      case __CAB2CAB_CALL_EMER_EN: // 2014/12/24
        if (avc2cop_packet->CallReq.cab_call.emergency_en == 1)
            ret = 0;
        break;
#endif
      case __CAB2CAB_CALL_STOP:
        if (  (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.cab_call.stop == 1
           )
        {
            cab_call_cab_clear_for_key();
	    ret = 0;
        }
        break;

      case __CAB2CAB_CALL_REJECT:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.cab_call.reject == 1
           )
	    ret = 0;
        break;

      case __CAB2CAB_CALL_WAKEUP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.cab_call.wake_up == 1
	    && avc2cop_packet->CallReq.pei_call.wake_up == 0
           )
	    ret = 0;
        break;

      case __CAB2CAB_CALL_MONITORING_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.cab_call.mon_start == 1
           )
	    ret = 0;
        break;

      case __CAB2CAB_CALL_MONITORING_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.cab_call.mon_stop == 1
           )
	    ret = 0;
        break;

      case __PEI2CAB_CALL_MONITORING_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.pei_call.mon_start == 1
	    /* && avc2cop_packet->CallReq.pei_call.emergency_en == 0 */
           )
	    ret = 0;
        break;

      case __PEI2CAB_CALL_MONITORING_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.pei_call.mon_stop == 1
	    /* && (avc2cop_packet->CallReq.pei_call.emergency_en == 0) */
           )
	    ret = 0;
        break;

#if 0
      case __PEI2CAB_CALL_EMER_MONITORING_START:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.pei_call.mon_start == 1
	    && avc2cop_packet->CallReq.pei_call.emergency_en == 1
           )
	    ret = 0;
        break;

      case __PEI2CAB_CALL_EMER_MONITORING_STOP:
        if (   (avc2cop_packet->BCastReq.active & 1) == 0
            && avc2cop_packet->CallReq.active == 1
	    && avc2cop_packet->CallReq.pei_call.mon_stop == 1
	    && avc2cop_packet->CallReq.pei_call.emergency_en == 1
           )
	    ret = 0;
        break;
#endif

#if 0
      case COP_REBOOT_CMD:
        if (avc2cop_packet->reboot == 1)
            ret = 0;
        break;
#endif

      case GET_TRAIN_NUM:
		if (avc2cop_packet->train_num && avc2cop_packet->CallReq.car_number)
		{
			ret = 0;
		}
		else
			ret = -1;
		break;

      case IS_ONLY_LED_FLAG:
        if (   ((avc2cop_packet->BCastReq.active & 1) == 1)
            && (avc2cop_packet->CallReq.active == 0)
            && (buf[5] == 0 && buf[6] > 0 && buf[7] == 0 && buf[8] == 0 && buf[9] == 0)
           )
            ret = buf[6];
        else
            ret = -1;
        break;

      case ONLY_PAMP_VOLUME:
        if (   ((avc2cop_packet->BCastReq.active & 1) == 0)
            && (avc2cop_packet->CallReq.active == 0)
            && (buf[36] == 0)
           )
            ret = 0;
        else
            ret = -1;
        break;

#if 0
      case IS_DEADMAN_CMD:
        if (   ((avc2cop_packet->BCastReq.active & 1) == 0)
            && (avc2cop_packet->CallReq.active == 0)
            && ((buf[30] & 0x03))
           )
            ret = 0;
        else
            ret = -1;
        break;

      case GET_DEADMAN_CMD:
        ret = buf[30] & 0x03;
        break;

#endif
      case ENABLE_GLOBAL_START_FUNC_AUTO:
        if ((buf[2] & 0x6) == 0x02)
           ret = 0;
        break;

      case DISABLE_GLOBAL_START_FUNC_AUTO:
        if ((buf[2] & 0x6) == 0x04)
           ret = 0;
        break;

      case UI_SET_ROUTE_CMD:
        if ((buf[39] & 0x1) != 0)
            ret = 0;
        break;

      case UI_SET_SPECIAL_ROUTE_CMD:
        if ((buf[45] & 0x3) != 0)
            ret = 0;
        break;

      case UI_DI_DISPLAY_CMD:
        if ((buf[39] & 0x6) != 0)
            ret = 0;
        break;

      default:
        printf("%s(), Invalid status : %d\n", __func__, __broadcast_status);
        break;
    }

    return ret;
}

unsigned char get_car_number(char *buf, int len)
{
    packet_AVC2COP_t *avc2cop_packet;

    avc2cop_packet = (packet_AVC2COP_t *)buf;

    return avc2cop_packet->CallReq.car_number;
}

unsigned char get_device_number(char *buf, int len)
{
    packet_AVC2COP_t *avc2cop_packet;

    avc2cop_packet = (packet_AVC2COP_t *)buf;

    return avc2cop_packet->CallReq.dev_number;
}

unsigned int get_rx_mult_addr(char *buf, int len)
{
    packet_AVC2COP_t *avc2cop_packet;

    avc2cop_packet = (packet_AVC2COP_t *)buf;

    return avc2cop_packet->RxMultiAddr;
}

unsigned int put_rx_mult_addr(char *buf, unsigned int rx_addr)
{
    packet_AVC2COP_t *avc2cop_packet;

    avc2cop_packet = (packet_AVC2COP_t *)buf;

    avc2cop_packet->RxMultiAddr = rx_addr;
}

unsigned int get_tx_mult_addr(char *buf, int len)
{
    packet_AVC2COP_t *avc2cop_packet;

    avc2cop_packet = (packet_AVC2COP_t *)buf;

    return avc2cop_packet->TxMultiAddr;
}

unsigned int get_mon_mult_addr(char *buf, int len)
{
    packet_AVC2COP_t *avc2cop_packet;

    avc2cop_packet = (packet_AVC2COP_t *)buf;

    return avc2cop_packet->MonMultiAddr;
}

unsigned int get_train_number(char *buf, int len)
{
    packet_AVC2COP_t *avc2cop_packet;

    avc2cop_packet = (packet_AVC2COP_t *)buf;

    return avc2cop_packet->train_num;
}
