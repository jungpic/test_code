/*
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gpio_key_dev.h"
#include "cob_process.h"
#include "avc_cycle_msg.h"
#include "avc_tcp.h"
#include "send_cob2avc_cmd.h"
#include "recv_avc2cob_cmd.h"
#include "audio_multicast.h"
#include "lcd.h"
#include "audio_ctrl.h"
#include "watchdog_trigger.h"
#include "version.h"
#include "cob_system_error.h"
#include "software_update.h"

#include "ftpget.h"

#include "debug_multicast.h"

#define LOG_AVC_NOT_CONNECT 0
#define LOG_READY           1
#define LOG_IP_SETUP_ERR    2

#define DEBUG_LINE                      0
#if (DEBUG_LINE == 1)
#define DBG_LOG(fmt, args...)   \
	do {                    \
		fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);        \
	} while(0)
#else
#define DBG_LOG(fmt, args...)
#endif


#define COP_VERSION_INTEGER			0	
#define COP_VERSION_DECIMAL_POINT	26	// OCC Enable  v0.69
//#define COP_VERSION_DECIMAL_POINT	24	// OCC Disable v0.67

#define WATCHDOG_TIMEOUT_NS			150000000		// 150ms

#if (FIRE_ALARM_TIMER == 1)
extern int enable_alaram_transfer;
extern int fire_alarm_is_use;
extern int enable_fire_alarm;
extern int fire_alarm_stime;
extern int fire_alarm_etime;
#endif
extern int internal_test_use;                  /* Only used for Env. test case */
extern int debug_output_enable;

int Log_Status = LOG_AVC_NOT_CONNECT;
enum __cop_key_enum key_val;

extern int fire_alarm_key_status;

time_t first_cop_debug_time, second_cop_debug_time;

void force_reboot(void);

static struct timespec w_old_time, w_cur_time;
static unsigned long watchdog_elapsed_time;
extern long cal_clock_delay(struct timespec *old_t, struct timespec *cur_t);

static void exit_program(int sig);
static void app_init(void);
static void app_exit(void);
static void print_program_start_message(void);

int check_avc_cycle_data(void);

static int GPIO_Driver_OK = 0;

static time_t wait_old_time, wait_cur_time;
static time_t tcp_check_wait_old_time, tcp_check_wait_cur_time;

static time_t old_wait_dhcp_renew_time, cur_wait_dhcp_renew_time;
static int dhcp_renew_killed_dhcp;

extern COP_Cur_Status_t __broadcast_status;
extern unsigned char my_ip_value[4];

static int avc_cycle_rx_ctr;
static int avc_cycle_rx_timeout;
static unsigned int avc_srv_ip_addr;
int done_avc_tcp_connected;
int b_received_first_cmd_from_avc;
static int enable_reconnect_avc_tcp;
static int rx_len;

char AVC_Master_IPName[16];

#define ETH_RX_MAXBUF	1024
#define DUMMY_ADD_BUFLEN	2048
#define MAX_BUF_LEN (ETH_RX_MAXBUF + DUMMY_ADD_BUFLEN)
static char RxBuf[MAX_BUF_LEN];

static char TmpBuf[128];

static int _occured_ID_PRODUCT_ID_ERR;
static int _occured_CODEC_HW_1_ERR;
static int _occured_CODEC_HW_2_ERR;
static int _occured_CODEC_HW_3_ERR;
static int _occured_CODEC_HW_4_ERR;

static unsigned short UBoot_Version;
static unsigned short Kernel_Version;
static unsigned short App_Version;
static unsigned short Entire_Version;
char cop_version_txt[10];

static int now_reboot;
static int need_renew;
static int Software_Update_Start;
static int Try_Software_Update;
static char FTP_Remote_FilePath[128];
static enum sw_up_status Status_Update;
static int watchdog_onoff_enable;

extern int LcdLogIndex;

#define LINUX_CARRIER_PROC_FILE	"/sys/class/net/eth0/carrier"
#define LINK_DETECT_START	1
#define LINK_UP_FIRST		2
#define LINK_DOWN_FIRST		3
#define LINK_UP_SECOND		4

static int dhcp_use_enable;
static int dhcp_live_and_link_up;

static int udhcpc_pid;
int get_pid_udhcpc(void);

static int cur_link_status, old_cur_link_status;

static int avc_ip_fixed;

void gpio_watchdog_disable_start_watchdog_by_kernel_trigger(void)
{
    if (GPIO_Driver_OK && watchdog_onoff_enable)
        watchdog_pwm_on();
    set_watchdog_trigger_heartbeat(); // start trigger by kernel
}

void gpio_watchdog_enable(void)
{
    if (GPIO_Driver_OK && watchdog_onoff_enable)
        watchdog_pwm_off();
    set_watchdog_trigger_none(); // stop trigger in kernel
}

static void to_do_reset_ethernet_link(void)
{
    dhcp_live_and_link_up = 0;
    enable_reconnect_avc_tcp = 0;
    avc_cycle_rx_ctr = 0;
    done_avc_tcp_connected = 0;
	b_received_first_cmd_from_avc = 0;
    cop_status_reset();
    reset_avc_cycle_hbc();

    avc_cycle_close();
    close_auto_fire_func_monitoring_bcast();
    avc_tcp_close("LINK DOWN");

    //log_msg_draw(LOG_MSG_STR_NOT_CON_AVC);
    Log_Status = LOG_AVC_NOT_CONNECT;

    close_cop_multi_tx();

	if(internal_test_use) {
		close_cop_debug_multicast();
		close_qc_test_multicast_recv();
		close_qc_test_multicast_send();
	}

    close_call_and_occ_pa_multi_tx();
    close_cop_multi_passive_tx();
    close_cop_multi_rx_sock();
    close_cop_multi_monitoring();

    phy_reset_low();
    usleep(10000);
    phy_reset_high();

    reset_call_multi_rx_data_ptr();
    reset_mon_multi_rx_data_ptr();
    reset_occ_pa_multi_rx_data_ptr();
    reset_cop_pa_multi_rx_data_ptr();
    reset_auto_multi_rx_data_ptr();
    reset_func_multi_rx_data_ptr();

    if (dhcp_use_enable) {
        udhcpc_pid = get_pid_udhcpc();
        if (udhcpc_pid > 0) {
	    sprintf(TmpBuf, "/bin/kill %d", udhcpc_pid);
	    system(TmpBuf);
        }
    }
}

static int to_do_start_ethernet_link(void)
{
    int ret = -1;
    int check_ctr = 0;
    int try = 0;

    if (GPIO_Driver_OK && watchdog_onoff_enable)
        watchdog_pwm_on();
    set_watchdog_trigger_heartbeat(); // start trigger by kernel

    if (dhcp_use_enable)
        system("/sbin/udhcpc -i eth0 -s /etc/udhcpc.script -p /tmp/udhcpc.pid -t 2 -b");

    //clock_gettime(CLOCK_REALTIME, &w_old_time);

    while (1) {
        if (GPIO_Driver_OK && watchdog_onoff_enable) {
	    //clock_gettime(CLOCK_REALTIME, &w_cur_time);
	    //watchdog_elapsed_time = cal_clock_delay(&w_old_time, &w_cur_time);
	    //if (watchdog_elapsed_time >= WATCHDOG_TIMEOUT_NS) {
	        //clock_gettime(CLOCK_REALTIME, &w_old_time);
	        watchdog_gpio_toggle();
	    //}
        }

        ret = avc_tcp_init();
        if (ret == 0)
	    break;

        sleep(1);
        check_ctr++;
        if (check_ctr >= 6) {
	    printf("< ETH CHECK CTR:%d >\n", check_ctr);
	    check_ctr = 0;
	    phy_reset_low();
	    usleep(10000);
	    phy_reset_high();
	    sleep(1);

            try++;
        }

        if (try >= 2) break;
    }

    if (GPIO_Driver_OK && watchdog_onoff_enable)
        watchdog_pwm_off();
    set_watchdog_trigger_none(); // stop trigger in kernel

    if (GPIO_Driver_OK && watchdog_onoff_enable) {
        clock_gettime(CLOCK_REALTIME, &w_old_time);
        watchdog_gpio_toggle();
    }

    if (ret != 0) return -1;

    ret = check_my_ip();
    if (ret != 0) {
        _occured_ID_PRODUCT_ID_ERR = 1;
        //log_msg_draw(LOG_MSG_STR_IP_SETUP_ERR);
        Log_Status = LOG_IP_SETUP_ERR;
    }
    else {
        if (_occured_ID_PRODUCT_ID_ERR == 1)
	    send_all2avc_cycle_data(RxBuf, 8, ERROR_IP_SETUP_ERR, 0, 1); // ERR CLEAR

        _occured_ID_PRODUCT_ID_ERR = 0;
    }

    time(&old_wait_dhcp_renew_time);
    dhcp_renew_killed_dhcp = 0;

    dhcp_live_and_link_up = 1;
    init_cop_multicast();
    init_avc_cycle_data();
    ret = connect_avc_cycle_multicast();

    init_auto_fire_func_monitoring_bcast();

	if(internal_test_use) {
		init_cop_debug_multicast();
		init_qc_test_multicast_recv();
		init_qc_test_multicast_send();

		connect_cop_debug_multicast();

		ret = connect_qc_test_multicast_recv(QC_TEST_MULTICAST_RECV_IP, QC_TEST_MULTICAST_PORT);
		if (ret != 0) {
			printf("< TEST PROGRAM MULTICAST RECV INIT ERROR >\n");
			force_reboot();
		}

		ret = connect_qc_test_multicast_send(QC_TEST_MULTICAST_SEND_IP, QC_TEST_MULTICAST_PORT);
		if (ret != 0) {
			printf("< TEST PROGRAM MULTICAST SEND INIT ERROR >\n");
			force_reboot();
		}
	}

    return 0;
}

void force_reboot(void)
{
     set_watchdog_trigger_none();
     system("reboot");
}

int main_call(int arg1, int arg2)
{
    int i, ret, len = 0, process_len = 0, cmd_len = 0;//, ret_len = 0;
    int val;
    int rcode = -1;
    int avc_tcp_cmd = 0;
    char carrier;
    int check_ctr = 0;
    int cmd_run_retry = 0;
    //int link_done_check_ctr = 0;

    avc_ip_fixed = 0;
    need_renew = 0;

    if ( (arg1 != 1 && arg1 != 0) || (arg2 != 1 && arg2 != 0) )
    {
        printf("< Usage: cop__app(arg1=%d, arg2=%d) >\n", arg1, arg2);
        exit(1);
    }

#if 0
    if (argc >= 4) {
        strcpy(&AVC_Master_IPName[0], argv[3]);
//printf(" ARG, AVC IP: %s\n", &AVC_Master_IPName[0]);
//printf(" ARG, AVC IP: %d\n", inet_addr(&AVC_Master_IPName[0]));
        if (inet_addr(&AVC_Master_IPName[0]) != -1) {
            avc_ip_fixed = 1;
            printf("<< FIXED AVC IP: %s >>\n", &AVC_Master_IPName[0]);
        }
    }
#endif

    watchdog_onoff_enable = 0;
    dhcp_use_enable = 0;

    if (arg1)
        watchdog_onoff_enable = 1;
    else
        watchdog_onoff_enable = 0;

    if(arg2)
        dhcp_use_enable = 1;
    else
        dhcp_use_enable = 0;

    // 2014/02/25
    phy_reset_low();
    usleep(10000);
    phy_reset_high();

    udhcpc_pid = 0;
    dhcp_live_and_link_up = 0;

    UBoot_Version = get_uboot_version((unsigned char *)RxBuf, MAX_BUF_LEN);
    Kernel_Version = get_kernel_version((unsigned char *)RxBuf, MAX_BUF_LEN);
    App_Version = (COP_VERSION_INTEGER << 8) + COP_VERSION_DECIMAL_POINT;
    Entire_Version = get_entire_version(UBoot_Version, Kernel_Version, App_Version);
    set_app_version(Entire_Version);

    print_program_start_message();

    ret = key_init();

    ret = audio_init();
#if 0
    ret = lcd_init();
    ret = font_init();
#endif

    ret = cop_process_init();
    ret = send_cop2avc_packet_init();

    call_list_init();

    //log_msg_draw(LOG_MSG_STR_NOT_CON_AVC);
    Log_Status = LOG_AVC_NOT_CONNECT;

    check_ctr++;

#if 1
    while (1) {
        carrier = read_link_carrier();
        if (carrier == '1') {
            printf("\n< DETECT LINK UP >\n");
            break;
        }
        sleep(1);

        check_ctr++;
        if (check_ctr >= 5) {
            printf("< 1, ETH CHECK CTR:%d >\n", check_ctr);
            check_ctr = 0;

            phy_reset_low();
            usleep(10000);
            phy_reset_high();
            sleep(1);
        }
    }
#endif

    if (dhcp_use_enable) {
        udhcpc_pid = get_pid_udhcpc();
        if (udhcpc_pid <= 0)
            system("/sbin/udhcpc -i eth0 -s /etc/udhcpc.script -p /tmp/udhcpc.pid -t 2 -b");
    }

    check_ctr = 0;
    while (1) {
        ret = avc_tcp_init();
        if (ret == 0)
            break;
        sleep(1);
        check_ctr++;
        if (check_ctr >= 3) {
            printf("< 2, ETH CHECK CTR:%d >\n", check_ctr);
            check_ctr = 0;
            phy_reset_low();
            usleep(10000);
            phy_reset_high();
            sleep(1);

            udhcpc_pid = get_pid_udhcpc();
            if (udhcpc_pid > 0) {
                sprintf(TmpBuf, "/bin/kill %d", udhcpc_pid);
                system(TmpBuf);

                sleep(1);
                system("/sbin/udhcpc -i eth0 -s /etc/udhcpc.script -p /tmp/udhcpc.pid -t 2 -b");
            }
        }
    }

#if 1
    carrier = read_link_carrier();
    if (carrier == '1') {
        cur_link_status = 1;
        old_cur_link_status = 1;
    }
    else {
        cur_link_status = 0;
        old_cur_link_status = 0;
    }
#endif

    now_reboot = 0;
    Software_Update_Start = 0;
    _occured_ID_PRODUCT_ID_ERR = 0;
    _occured_CODEC_HW_1_ERR = 0;
    _occured_CODEC_HW_2_ERR = 0;
    _occured_CODEC_HW_3_ERR = 0;
    _occured_CODEC_HW_4_ERR = 0;

    ret = key_dev_open();
    if(ret < 0){
	GPIO_Driver_OK = 0;
    }
    else
	GPIO_Driver_OK = 1;


    init_cop_multicast();
    init_avc_cycle_data();

	if(internal_test_use) {
		init_cop_debug_multicast();
		init_qc_test_multicast_recv();
		init_qc_test_multicast_send();
	}

    ret = connect_avc_cycle_multicast();
    if (ret == -1) {
		/*app_exit(); delete, 2015.05.09 Ver0.73
		return 0; */

#if 1 // Ver0.73
        set_watchdog_trigger_none();
        watchdog_pwm_off();
        system("reboot");
#endif
    }

    (void)signal(SIGINT, exit_program);
    (void)signal(SIGTERM, exit_program);

    app_init();

#if 0
/* KEY TEST */
while (1)
    key_test();
#endif
    
    //val = get_key_value(); // remove pre-pushed key

    ret = check_my_ip();
    if (ret != 0) {
        _occured_ID_PRODUCT_ID_ERR = 1;
        //log_msg_draw(LOG_MSG_STR_IP_SETUP_ERR);
        Log_Status = LOG_IP_SETUP_ERR;
    }

    dhcp_live_and_link_up = 1;

    while (1) {
        rx_len = try_read_avc_cycle_data(&AVC_Master_IPName[0], avc_ip_fixed);
        if (rx_len > 0) {
            setup_local_time();
            break;
        }
    }

    if (dhcp_use_enable) {
        avc_cycle_close();

        udhcpc_pid = get_pid_udhcpc();
        if (udhcpc_pid > 0) {
            sprintf(TmpBuf, "/bin/kill %d", udhcpc_pid);
            system(TmpBuf);

            sleep(1);
            system("/sbin/udhcpc -i eth0 -s /etc/udhcpc.script -p /tmp/udhcpc.pid -t 2 -b");
        }

        init_cop_multicast();
        init_avc_cycle_data();
        ret = connect_avc_cycle_multicast();
		if(internal_test_use) {
			init_cop_debug_multicast();
			init_qc_test_multicast_recv();
			init_qc_test_multicast_send();
			connect_cop_debug_multicast();

			ret = connect_qc_test_multicast_recv(QC_TEST_MULTICAST_RECV_IP, QC_TEST_MULTICAST_PORT);
			if (ret != 0) {
				printf("< TEST PROGRAM MULTICAST RECV INIT ERROR >\n");
				force_reboot();
			}

			ret = connect_qc_test_multicast_send(QC_TEST_MULTICAST_SEND_IP, QC_TEST_MULTICAST_PORT);
			if (ret != 0) {
				printf("< TEST PROGRAM MULTICAST SEND INIT ERROR >\n");
				force_reboot();
			}
		}
    }
    else {
        avc_cycle_close();
        init_avc_cycle_data();
        ret = connect_avc_cycle_multicast();
		if(internal_test_use) {
			init_cop_debug_multicast();
			init_qc_test_multicast_recv();
			init_qc_test_multicast_send();
			connect_cop_debug_multicast();

			ret = connect_qc_test_multicast_recv(QC_TEST_MULTICAST_RECV_IP, QC_TEST_MULTICAST_PORT);
			if (ret != 0) {
				printf("< TEST PROGRAM MULTICAST RECV INIT ERROR >\n");
				force_reboot();
			}

			ret = connect_qc_test_multicast_send(QC_TEST_MULTICAST_SEND_IP, QC_TEST_MULTICAST_PORT);
			if (ret != 0) {
				printf("< TEST PROGRAM MULTICAST SEND INIT ERROR >\n");
				force_reboot();
			}
		}
    }

    init_auto_fire_func_monitoring_bcast();

	// Disabled by MH RYU
	//init_pid_multicast();
	//connect_pid_multicast_recv("234.1.2.2", 5500);

    time(&tcp_check_wait_old_time);

    if (check_codec_HW_error_1()) _occured_CODEC_HW_1_ERR = 1;
    if (check_codec_HW_error_2()) _occured_CODEC_HW_2_ERR = 1;
    if (check_codec_HW_error_3()) _occured_CODEC_HW_3_ERR = 1;
    if (check_codec_HW_error_4()) _occured_CODEC_HW_4_ERR = 1;

    if (GPIO_Driver_OK && watchdog_onoff_enable == 1) {
        printf("< WATCHDOG TRIGGER ON >\n");
        set_watchdog_trigger_none();
        watchdog_pwm_off();
    }
    else if(!watchdog_onoff_enable)
		printf("< WATCHDOG TRIGGER OFF >\n");

    time(&old_wait_dhcp_renew_time);
    dhcp_renew_killed_dhcp = 0;

	fire_alarm_key_status = 0;

    while (1) {
        carrier = read_link_carrier();
        if (carrier == '1')
            cur_link_status = 1;
        else if (carrier == '0')
            cur_link_status = 0;

        if (cur_link_status != old_cur_link_status) {
            old_cur_link_status = cur_link_status;

            if (!cur_link_status) {
                printf("\n< CHECK LINK -> DOWN >\n");

                to_do_reset_ethernet_link();
            }
            else {
                printf("\n< CHECK LINK -> UP >\n");

                if (GPIO_Driver_OK && watchdog_onoff_enable)
                    watchdog_pwm_on();
                set_watchdog_trigger_heartbeat(); // start trigger by kernel

                if (dhcp_use_enable)
                    system("/sbin/udhcpc -i eth0 -s /etc/udhcpc.script -p /tmp/udhcpc.pid -t 2 -b");

                if (GPIO_Driver_OK && watchdog_onoff_enable)
                    watchdog_pwm_off();
                set_watchdog_trigger_none(); // stop trigger in kernel

                clock_gettime(CLOCK_REALTIME, &w_old_time);

                while (1) {

	            if (GPIO_Driver_OK && watchdog_onoff_enable) {
                        clock_gettime(CLOCK_REALTIME, &w_cur_time);
                        watchdog_elapsed_time = cal_clock_delay(&w_old_time, &w_cur_time);
                        if (watchdog_elapsed_time >= WATCHDOG_TIMEOUT_NS) {
                            clock_gettime(CLOCK_REALTIME, &w_old_time);
                            watchdog_gpio_toggle();
                        }
                    }


                    ret = avc_tcp_init();
                    if (ret == 0)
                        break;

                    carrier = read_link_carrier();
                    if (carrier == '0')
                        break;

                    sleep(1);
                    check_ctr++;
                    if (check_ctr >= 3) {
                        printf("< ETH CHECK CTR:%d >\n", check_ctr);
                        check_ctr = 0;
                        phy_reset_low();
                        usleep(10000);
                        phy_reset_high();
                        sleep(1);
                    }
                }

                if (ret != 0)
                    break;

                ret = check_my_ip();
                if (ret != 0) {
                    _occured_ID_PRODUCT_ID_ERR = 1;
                    //log_msg_draw(LOG_MSG_STR_IP_SETUP_ERR);
                    Log_Status = LOG_IP_SETUP_ERR;
                }
                else {
                    if (_occured_ID_PRODUCT_ID_ERR == 1)
                        send_all2avc_cycle_data(RxBuf, 8, ERROR_IP_SETUP_ERR, 1, 0);

                    _occured_ID_PRODUCT_ID_ERR = 0;
                }

                time(&old_wait_dhcp_renew_time);
                dhcp_renew_killed_dhcp = 0;

                dhcp_live_and_link_up = 1;
                init_cop_multicast();
                init_avc_cycle_data();
                ret = connect_avc_cycle_multicast();

				if(internal_test_use) {
					init_cop_debug_multicast();
					init_qc_test_multicast_recv();
					init_qc_test_multicast_send();
					connect_cop_debug_multicast();

					ret = connect_qc_test_multicast_recv(QC_TEST_MULTICAST_RECV_IP, QC_TEST_MULTICAST_PORT);
					if (ret != 0) {
						printf("< TEST PROGRAM MULTICAST RECV INIT ERROR >\n");
						force_reboot();
					}

					ret = connect_qc_test_multicast_send(QC_TEST_MULTICAST_SEND_IP, QC_TEST_MULTICAST_PORT);
					if (ret != 0) {
						printf("< TEST PROGRAM MULTICAST SEND INIT ERROR >\n");
						force_reboot();
					}
				}

                init_auto_fire_func_monitoring_bcast();
            }
        }
 
        if (need_renew) {
            need_renew = 0;
            to_do_reset_ethernet_link();
            to_do_start_ethernet_link();
        }

        check_avc_cycle_data();

#if (FIRE_ALARM_TIMER == 1)
		if (fire_alarm_is_use) {
			if(enable_fire_alarm) {
				double diff_time;
				int    tm_day, tm_hour, tm_min, tm_sec;

				time(&fire_alarm_etime);
				diff_time = difftime(fire_alarm_etime, fire_alarm_stime);
				//printf("FIRE ALARM time difference : %f seconds \n", diff_time);

				if( (diff_time > 10.0) && (enable_alaram_transfer == 1) ){
					unsigned short fireflag = 0x0001;
					send_cop2avc_fire_flag(0, fireflag);
					enable_alaram_transfer = 0;
					printf("FIRE occured before 10 seconds\n");
				}
			}
		}
#endif

		if(internal_test_use) {
			int diff_time;

			second_cop_debug_time = time(NULL);
			diff_time = difftime(second_cop_debug_time, first_cop_debug_time);
			if(diff_time >= 1) {
				if(debug_output_enable) {
					ret = send_cop_debug_data();
					debug_output_enable = 0;
				}

				first_cop_debug_time = time(NULL);
			}
		}

        /* KEY PROCESS */
        val = get_key_value();
        key_val = (enum __cop_key_enum)val;

        if (done_avc_tcp_connected && key_val > 0) {
            ret = cop_bcast_status_update (key_val, NULL, 0, &now_reboot,
										   done_avc_tcp_connected, _occured_ID_PRODUCT_ID_ERR);
        }

        /* TCP Commnucation PROCESS */
		if (done_avc_tcp_connected) {
            time(&wait_cur_time);
            if ((wait_cur_time - wait_old_time) > 5) {
                avc_cycle_rx_timeout = 1;
                printf("< TIMEOUT GET AVC CYCLE >\n");
            }

            len = try_avc_tcp_read(RxBuf, ETH_RX_MAXBUF);
            if (len == -1 || avc_cycle_rx_timeout == 1) {
                //LcdLogIndex = LOG_MSG_STR_NOT_CON_AVC;
                //log_msg_draw(LOG_MSG_STR_NOT_CON_AVC);
                Log_Status = LOG_AVC_NOT_CONNECT;

                if (len == -1)
                    avc_tcp_close("SERVER CLOSED");
                else if (avc_cycle_rx_timeout == 1)
                    avc_tcp_close("MULTICAST TIMEOUT");

                done_avc_tcp_connected = 0;
				b_received_first_cmd_from_avc = 0;
                avc_cycle_rx_ctr = 0;
                avc_cycle_rx_timeout = 0;
                reset_avc_cycle_hbc();
                reset_avc_srv_ip();
                cop_status_reset();
            }
            else if (len > 0) {
                process_len = 0;
                while (process_len < len) {
                    avc_tcp_cmd = 0;
                    cmd_len = process_avc_tcp_data(&RxBuf[process_len], len, &avc_tcp_cmd);

                    if (cmd_len < 0)
                        break;

                    //ret_len = 0;

					if(b_received_first_cmd_from_avc == 0)
					{
						b_received_first_cmd_from_avc = 1;
						printf(">>>>>> received first avc cmd\n");
					}
                    switch (avc_tcp_cmd) {
                    case SW_UPDATE_CMD_VER_REQ:
                        print_avc_to_cop_packet(&RxBuf[process_len], cmd_len);
                        send_update_result_to_avc(0, 0, 0);
                        //ret_len = cmd_len;
                        break;

                    case SW_UPDATE_CMD_ICS_VER_REQ:
                        print_avc_to_cop_packet(&RxBuf[process_len], cmd_len);
                        send_ics_update_result_to_avc(UBoot_Version, Kernel_Version, App_Version);
                        //ret_len = cmd_len;
                        break;

                    case SW_UPDATE_CMD_START:
                        if (!Software_Update_Start) {
                            print_avc_to_cop_packet(&RxBuf[process_len], cmd_len);
                            ret = get_ftp_remote_file_path(&RxBuf[process_len], FTP_Remote_FilePath, 128);
                            if (ret > 0) {
                                Software_Update_Start = 1;
                                Try_Software_Update = 0;
                                Status_Update = FTP_LOGIN;
                            }
                        }
                        //ret_len = cmd_len;
                        break;

                    case AVC_TCP_CMD_COP_CTRL_ID:
                        print_avc_to_cop_packet(&RxBuf[process_len], cmd_len);
                        ret = cop_bcast_status_update (0, &RxBuf[process_len], cmd_len,
                                    				   &now_reboot, done_avc_tcp_connected,
			            			                   _occured_ID_PRODUCT_ID_ERR); /* NoKey process */
                        if (ret == cmd_len && cmd_run_retry == 0) {
                            /* OK */
                        }
                        else if (ret != cmd_len && cmd_run_retry < 3) /* Ver0.71, 2-> 3*/
                            printf("[ CMD RETRY %d ], __broadcast_status = %d \n", ++cmd_run_retry, __broadcast_status);
                        else if (ret != cmd_len && cmd_run_retry >= 3) { /* Ver0.71, 2-> 3*/
                            cmd_run_retry = 0;
                            send_call_protocol_err_to_avc(&RxBuf[process_len]);
                            printf("<<< NOT proccessed CMD >>> : ret(%d), cmd_len(%d), cmd_run_retry(%d)\n", ret, cmd_len, cmd_run_retry);
                            for (i = 0; i < cmd_len; i++) {
                                if ((i%16) == 0) printf("\n  ");
                                printf("%02X ", RxBuf[process_len+i]);
                            }
                            printf("\n");
                        }
                        /* 2013/12/29 Ver 0.71 *************************************/
                        else if (ret == cmd_len && cmd_run_retry >= 3) {
                            printf("[ CMD RETRY DONE(3) ]\n");
                            cmd_run_retry = 0;
                        }
                        else if (ret == cmd_len && cmd_run_retry >= 2) {
                            printf("[ CMD RETRY DONE(2) ]\n");
                            cmd_run_retry = 0;
                        }
                        else if (ret == cmd_len && cmd_run_retry >= 1) {
                            printf("[ CMD RETRY DONE(1) ]\n");
                            cmd_run_retry = 0;
                        }
                        else {
                            printf("[ CMD RETRY ?? ]\n");
                            cmd_run_retry = 0;
                        }
                        /***********************************************************/
                        break;

                    default:
                        printf("<< Not Defined CMD: 0x%X\n >>", avc_tcp_cmd);
                        break;
                    }

                    if (!cmd_run_retry)
                        process_len += cmd_len;

                    //process_len += ret_len;
                    if (process_len >= ETH_RX_MAXBUF)
                        break;

                    /* 2013/12/06, Ver 0.70 */
	            if (GPIO_Driver_OK && watchdog_onoff_enable && !now_reboot)
	                watchdog_gpio_toggle();
                }
            }

//jaein, 130626 FTP Update add
            if (Software_Update_Start == 1) {

				if (GPIO_Driver_OK && watchdog_onoff_enable)
					watchdog_pwm_on();
				set_watchdog_trigger_heartbeat(); // start trigger by kernel

                switch (Status_Update) {
                case FTP_LOGIN:

                    send_all2avc_cycle_data(RxBuf, 8, 0, 0, 0);
                    ret = ftpget_login(&AVC_Master_IPName[0],
                            AVC_FTP_LOGIN_NAME, AVC_FTP_LOGIN_PASSWD);
                    send_all2avc_cycle_data(RxBuf, 8, 0, 0, 0);
                    if (ret == 0) {
                        Status_Update = FTP_FILE_DOWNLOADING;
                        Try_Software_Update = 0;
                        rcode = UPDATE_RESULT_LOGIN_OK;
                    }
                    else {
                        Try_Software_Update++;
                        if (Try_Software_Update >= 1 /* 3 */) {
                            Status_Update = FTP_ERROR;
                        }
                        rcode = UPDATE_RESULT_LOGIN_FAIL;

                        done_avc_tcp_connected = 0;
						b_received_first_cmd_from_avc = 0;
                        enable_reconnect_avc_tcp = 0;
                        //LcdLogIndex = LOG_MSG_STR_NOT_CON_AVC;
                        //log_msg_draw(LOG_MSG_STR_NOT_CON_AVC);
                        Log_Status = LOG_AVC_NOT_CONNECT;

                        avc_tcp_close("FTPLOGIN FAIL");

                        avc_cycle_rx_ctr = 0;
                        avc_cycle_rx_timeout = 0;
                        reset_avc_cycle_hbc();
                        reset_avc_srv_ip();
                        cop_status_reset();
                    }
                    printf("[ FTP LOGIN %s ]\n\n",
                            rcode == UPDATE_RESULT_LOGIN_OK ? "SUCESS":"FAIL");
                    send_update_result_to_avc(rcode, 0, 0);
                    break;

                case FTP_FILE_DOWNLOADING:
                    send_all2avc_cycle_data(RxBuf, 8, 0, 0, 0);
                    ret = ftp_receive(DEFAULT_LOCAL_PATH, FTP_Remote_FilePath);
                    send_all2avc_cycle_data(RxBuf, 8, 0, 0, 0);
                    if (ret == 0) {
                        Status_Update = FTP_FILE_UPDATE;
                        Try_Software_Update = 0;
                        rcode = UPDATE_RESULT_DOWN_OK;
                    }
                    else {
                        Try_Software_Update++;
                        if (Try_Software_Update >= 1 /* 3 */) {
                            Status_Update = FTP_ERROR;
                        }
                        rcode = UPDATE_RESULT_DOWN_FAIL;
                    }
                    printf("[ FTP FILE DOWNLOAD %s ]\n\n",
                            rcode == UPDATE_RESULT_DOWN_OK ? "SUCESS":"FAIL");
                    send_update_result_to_avc(0, rcode, 0);
                    break;

                case FTP_FILE_UPDATE:
                    ret = software_update(DEFAULT_LOCAL_PATH);
                    if (ret == 0) {
                        Status_Update = FTP_UPDATE_DONE;
                        rcode = UPDATE_RESULT_UPDATE_OK;
                    }
                    else {
                        Status_Update = FTP_ERROR;
                        rcode = UPDATE_RESULT_UPDATE_FAIL;
                    }
                    printf("[ FTP FILE UPDATE %s ]\n\n",
                            rcode == UPDATE_RESULT_UPDATE_OK ? "SUCESS":"FAIL");
                    send_update_result_to_avc(0, 0, rcode);
                    break;

                case FTP_UPDATE_DONE:
                    if (GPIO_Driver_OK && watchdog_onoff_enable)
                        watchdog_pwm_off();
                    set_watchdog_trigger_none(); // stop trigger in kernel

                    //LcdLogIndex = LOG_MSG_STR_NOT_CON_AVC;
                    //log_msg_draw(LOG_MSG_STR_NOT_CON_AVC);
                    Log_Status = LOG_AVC_NOT_CONNECT;

                    avc_tcp_close("UPDATE DONE");
                    avc_cycle_close();
                    sync();
                    sleep(1);
                    now_reboot = 1;
                    //Software_Update_Start = 0;
                    break;

                case FTP_ERROR:
                    if (GPIO_Driver_OK && watchdog_onoff_enable) {
                        set_watchdog_trigger_none(); // stop trigger in kernel
                        watchdog_pwm_off();
                    }

                    now_reboot = 0;
                    Software_Update_Start = 0;
                    break;
                }
	    	}
        }


        if (!now_reboot) {
            ret = cop_bcast_status_update (0, NULL, 0, &now_reboot,
										   done_avc_tcp_connected, _occured_ID_PRODUCT_ID_ERR); /* NoKey process */
        }
        else if (now_reboot == 1) {
            done_avc_tcp_connected = 0;
			b_received_first_cmd_from_avc = 0;

            if (Software_Update_Start) {
                Software_Update_Start = 0;
                printf("<< **** REBOOT After Update **** >>\n");
            } else
                printf("<< **** REBOOT CMD **** >>\n");
            //now_reboot = 0;
            system("reboot");
            set_watchdog_trigger_none();
            sleep(1);
        }

		//try_read_pid_msg();

		if(internal_test_use)
			try_read_qc_test_msg();
#if 0
        if (__broadcast_status == COP_STATUS_IDLE && dhcp_live_and_link_up) {
            time(&cur_wait_dhcp_renew_time);
            if (   !dhcp_renew_killed_dhcp
                && (cur_wait_dhcp_renew_time - old_wait_dhcp_renew_time) >  100)
            {
                dhcp_renew_killed_dhcp = 1;
                printf("\n<<< DHCP KILL to DHCP RENEW >>>\n");

                if (dhcp_use_enable) {
                    udhcpc_pid = get_pid_udhcpc();
                    if (udhcpc_pid > 0) {
                        sprintf(TmpBuf, "/bin/kill %d", udhcpc_pid);
                        system(TmpBuf);
                    }
                }
            }
            if (   dhcp_renew_killed_dhcp
                && (cur_wait_dhcp_renew_time - old_wait_dhcp_renew_time) >  103)
            {
                dhcp_renew_killed_dhcp = 0;
                printf("\n<<< DHCP RENEW >>>\n");
                if (dhcp_use_enable)
                    system("/sbin/udhcpc -i eth0 -s /etc/udhcpc.script -p /tmp/udhcpc.pid -t 2 -b");

                time(&old_wait_dhcp_renew_time);
            }
        }
#endif
    }

    app_exit();

    (void)signal(SIGINT, SIG_DFL);
    raise(SIGSEGV);

    return 0;
}

void ignore_avc_server_to_wait_another_server(void)
{
    done_avc_tcp_connected = 0;
	b_received_first_cmd_from_avc = 0;
    avc_cycle_rx_ctr = 0;
    avc_cycle_rx_timeout = 0;
    reset_avc_cycle_hbc();
    reset_avc_srv_ip();
    cop_status_reset();

    enable_reconnect_avc_tcp = 0;

    time(&tcp_check_wait_old_time);
}

int check_avc_cycle_data(void)
{
    int ret;

    if (GPIO_Driver_OK && watchdog_onoff_enable && !now_reboot) {
        clock_gettime(CLOCK_REALTIME, &w_cur_time);
        watchdog_elapsed_time = cal_clock_delay(&w_old_time, &w_cur_time);
        if (watchdog_elapsed_time >= WATCHDOG_TIMEOUT_NS) {
            clock_gettime(CLOCK_REALTIME, &w_old_time);
            watchdog_gpio_toggle();
        }
    }

    if (dhcp_live_and_link_up) {
        rx_len = try_read_avc_cycle_data(&AVC_Master_IPName[0], avc_ip_fixed);
	} else {
        rx_len = 0;
	}

    if (rx_len > 0) {
        set_indicate_avc_cycle_data();

        //setup_local_time();

        if (done_avc_tcp_connected) {
            avc_cycle_rx_ctr++;
            time(&wait_old_time);
        }

//printf("--cycle data len: %d\n", rx_len);
        ret = is_changed_avc_cycle_ip();
        if (ret == 1) {
            //LcdLogIndex = LOG_MSG_STR_NOT_CON_AVC;
            //log_msg_draw(LOG_MSG_STR_NOT_CON_AVC);
            Log_Status = LOG_AVC_NOT_CONNECT;

            avc_srv_ip_addr = get_avc_srv_ip();
            reset_avc_cycle_hbc();
            avc_tcp_close("CHANGED AVC IP");
            avc_cycle_rx_ctr = 0;
            avc_cycle_rx_timeout = 0;
            enable_reconnect_avc_tcp = 1;
            cop_status_reset();
            done_avc_tcp_connected = 0; // 2013/11/05
			b_received_first_cmd_from_avc = 0;

            time(&tcp_check_wait_old_time);
		}
    }

    if (enable_reconnect_avc_tcp && !now_reboot) {

        ret = -1;

        time(&tcp_check_wait_cur_time);
        if ((tcp_check_wait_cur_time - tcp_check_wait_old_time) >=  1) {
            time(&tcp_check_wait_old_time);
            ret = try_avc_tcp_connect(&AVC_Master_IPName[0], AVC_TCP_PORT, &now_reboot, &need_renew);
        }

        if (ret == 0) {
			key_init();
			cop_voip_call_codec_swreset();
            __cop_pa_mic_codec_swreset(); // 2013/11/29
            __cop_occ_pa_and_occ_call_codec_swreset(); // 2013/11/29
            cop_pa_mic_codec_init();
			cop_voip_call_codec_init();
            //cop_occ_codec_init();

            __cop_pa_mic_codec_start();
			cop_voip_call_codec_start();
            __cop_occ_pa_and_occ_call_codec_start();

            enable_reconnect_avc_tcp = 0;
            done_avc_tcp_connected = 1;
            avc_cycle_rx_ctr = 1;
            avc_cycle_rx_timeout = 0;
            time(&wait_old_time);

            if (!_occured_ID_PRODUCT_ID_ERR) {
                //LcdLogIndex = LOG_MSG_STR_READY;
                //log_msg_draw(LOG_MSG_STR_READY);
                Log_Status = LOG_READY;
            }
            else {
                //log_msg_draw(LOG_MSG_STR_IP_SETUP_ERR);
                Log_Status = LOG_IP_SETUP_ERR;
            }

			FuncUpdateThread_Start();
			printf("Call AVC MYSQL Query thread - For station name, DIF string\n");
        }
    }

    if (avc_cycle_rx_ctr >=  1) {
        avc_cycle_rx_ctr = 0;
        if (_occured_ID_PRODUCT_ID_ERR == 1) {
            _occured_ID_PRODUCT_ID_ERR = 2;
            if (init_ip_addr() == 0) {
                if (check_my_ip() != 0)
                    send_all2avc_cycle_data(RxBuf, 8, ERROR_IP_SETUP_ERR, 1, 0);
                else {
                    _occured_ID_PRODUCT_ID_ERR = 0;
                    send_all2avc_cycle_data(RxBuf, 8, 0, 0, 0);
                }
            }
        }
        else
            send_all2avc_cycle_data(RxBuf, 8, 0, 0, 0);

        if (_occured_CODEC_HW_1_ERR) {
            send_all2avc_cycle_data(RxBuf, 8, ERROR_CODEC_HW_1_ERR, 1, 0);
            _occured_CODEC_HW_1_ERR = 0;
        }
        if (_occured_CODEC_HW_2_ERR) {
            send_all2avc_cycle_data(RxBuf, 8, ERROR_CODEC_HW_2_ERR, 1, 0);
            _occured_CODEC_HW_2_ERR = 0;
        }
        if (_occured_CODEC_HW_3_ERR) {
            send_all2avc_cycle_data(RxBuf, 8, ERROR_CODEC_HW_3_ERR, 1, 0);
            _occured_CODEC_HW_3_ERR = 0;
        }
        if (_occured_CODEC_HW_4_ERR) {
            send_all2avc_cycle_data(RxBuf, 8, ERROR_CODEC_HW_4_ERR, 1, 0);
            _occured_CODEC_HW_4_ERR = 0;
        }
    }

    return done_avc_tcp_connected;
}

static void app_init(void)
{
    avc_cycle_rx_ctr = 0;
    avc_cycle_rx_timeout = 0;
    avc_srv_ip_addr = 0;
    done_avc_tcp_connected = 0;
	b_received_first_cmd_from_avc = 0;
    enable_reconnect_avc_tcp = 0;
}

static void app_exit(void)
{
    enable_reconnect_avc_tcp = 0;
    done_avc_tcp_connected = 0;
	b_received_first_cmd_from_avc = 0;

    avc_tcp_close("EXIT");
    key_init();
    key_exit();
    //cop_occ_pa_mic_codec_stop();
    audio_exit();
}

unsigned short get_version_digit(void)
{
    return Entire_Version;
}

static void print_program_start_message(void)
{
    char buf[128+64], *to;
    FILE *fp;

    to = RxBuf;
    sprintf(buf, "\n**####################################################\n");
    to = stpcpy(to, buf);

    sprintf(buf,  "  MAHMUTBEY COP Version %02d.%02d\n", (Entire_Version>>8), (Entire_Version&0xff));
    to = stpcpy(to, buf);

	sprintf(cop_version_txt, "%d.%02d", (Entire_Version>>8), (Entire_Version&0xff));

    sprintf(buf,  "    (Boot:%02d.%02d, Kernel:%02d.%02d, App:%02d.%02d)\n",
                (UBoot_Version >> 8), (UBoot_Version & 0xff),
                (Kernel_Version >> 8), (Kernel_Version & 0xff),
                (App_Version >> 8), (App_Version & 0xff));
    to = stpcpy(to, buf);
    sprintf(buf,  "**####################################################\n\n");
    to = stpcpy(to, buf);

    printf(RxBuf);

    fp = fopen("/tmp/cop_version.txt", "w");
    if (fp == NULL) {
        printf("ERROR, File open %s\n", "/tmp/cop_version.txt");
        return;
    }

    fprintf(fp, RxBuf);
    fclose(fp);

}

static void exit_program(int sig)
{
    if (sig == SIGTERM) {
        if (GPIO_Driver_OK && watchdog_onoff_enable)
            watchdog_pwm_on();
        set_watchdog_trigger_heartbeat(); // start trigger by kernel
    }

    cop_speaker_volume_mute_set();
    occ_speaker_volume_mute_set();

    printf("EXIT SIGNAL\n");

    app_exit();

    (void)signal(SIGINT, SIG_DFL);
    //raise(SIGSEGV);
}

