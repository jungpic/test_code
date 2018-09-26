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
#include "send_cob2avc_cmd.h"
#include "recv_avc2cob_cmd.h"
#include "audio_ctrl.h"
#include "audio_multicast.h"
#include "lcd.h"
#include "watchdog_trigger.h"
#include "cob_system_error.h"
#include "lm1971_ctrl.h"

#define NOT_USE_CAB_CALL_WAKEUP // don't use cab wakeup to communicate by pushing talk for CAB-CAB CALL

#define DEBUG_LINE                      1

#if (DEBUG_LINE == 1)
#define DBG_LOG(fmt, args...)   \
        do {                    \
                fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);        \
        } while(0)
#else
#define DBG_LOG(fmt, args...)
#endif


#define CAB_MONITORING_ON_PASSIVE_INDOOR
#define CAB_MONITORING_ON_PASSIVE_OUTDOOR

#define IZMIR_TRAM_COP		1
#define BUSAN_COB			0

//#define PEI_BYPASS_TO_OCC	0
//#define PEI_TO_OCC			1
#define HIGH_LOW_CHANGE		1

#define PEI_OCC_HALF_DUPLEX	1
#define PEI_OCC_FULL_DUPLEX	0
#define PEI_OCC_DUPLEX_MODE	PEI_OCC_HALF_DUPLEX

#define MENUSCREEN_FREEZE_ON_FIRE_ALARM		0
#define MENUSCREEN_UNFREEZE_ON_FIRE_ALARM	1
#define MENUSCREEN_ON_FIRE_ALARM			MENUSCREEN_FREEZE_ON_FIRE_ALARM

#define COP_SELF_VOL				0
#define PAMP_AVC_VOL				1
#define MONITORING_VOL				PAMP_AVC_VOL

/**** For Audio Mixer ****/
#define MAXIMUM_RTP_SOCKET                      16
#define MIXER_RX_BUF_LEN                        (172*2)
char call_mixer_rxbuf1[2][MIXER_RX_BUF_LEN];
int  call_mixer_rxlen1[2];
int  call_mixer_outptr1;
char call_mixer_rxbuf2[2][MIXER_RX_BUF_LEN];
int  call_mixer_rxlen2[2];
int  call_mixer_outptr2;

int occ_pa_enable_level;
int occ_pei_call_out_level;
int occ_pa_pei_enabled;
int old_occ_pa_enable_level;
int old_occ_pei_call_out_level;
int old_occ_pa_pei_enabled;

int change_to_call_menu;
int change_to_config_menu;
int occ_key_lock;
int cab_key_lock;
int pei_key_lock;
int pa_in_key_lock;
int pa_out_key_lock;
int func_key_lock;
int special_msg_lrm_on;
int special_msg_pib_on;
int pei_join_button_on;
char train_num;

extern int passive_in_button;
extern int passive_out_button;

static int occ_status;

static int tractor_possible;
static int tractor_broadcast_status;

static int pamp_in_vol; // Ver 0.73, 2014/03/06
static int pamp_out_vol; // Ver 0.73, 2014/03/06

#define MULTI_RX_BUF_LEN	1500
/*static char MultiRxBuf[MULTI_RX_BUF_LEN];
static int Last_Get_Multi_Len;*/

extern int check_avc_cycle_data(void);
extern int b_received_first_cmd_from_avc;

static char TmpBuf[32];

static KeyStatus_List_t __key_list_status;
COP_Cur_Status_t __broadcast_status;
COP_Cur_Status_t __2nd_broadcast_status;
static COP_Cur_Status_t __old_broadcast_status;
static COP_Cur_Status_t __2nd_old_broadcast_status;

#define STOP_LIMIT_PACKET_N	5

long cal_clock_delay(struct timespec *old_t, struct timespec *cur_t);

int (*Func_Cop_Key_Process_List[14])(enum __cop_key_enum keyval, int tcp_connected);

#define MENU_UPDOWN_INDEX_LOW	1
#define MENU_UPDOWN_INDEX_MAX	3 //4

static int pei2cab_call_start(int is_emergency);
static int pei2cab_join_call_start(void);
void update_pei_join_active_status(void);
int update_pei_join_call_list_status(void);

//static void key_off_process(void);
static void display_lcd_log_msg_process(void);
static time_t log_old_time, log_cur_time;

extern int Log_Status;

// mhryu : 05202015
static int pa_in_Key_pressed_on_fire_status;
static int pa_out_Key_pressed_on_fire_status;
static int call_cab_Key_pressed_on_fire_status;

static int pei2cab_monitoring_on;
static int auto_monitoring_on;
static int occ_monitoring_on;

int fire_alarm_key_status;

static int occ_key_status;
static int occ_tx_rx_en_ext_flag;
static int old_occ_tx_rx_en_ext_flag;

int occ_pa_en_status;
static int occ_pa_en_status_old;

static int pei_half_way_com_flag;

static int ptt_key_on_pei_cab_pa_open_multi_tx;

static int InOut_LedStatus;
static int Fire_alarm_status;

static int inout_bcast_processing;

static void menu_call_screen_reset(void);
static void menu_bcast_screen_reset(void);

static int occ_pa_bcast_monitor_start;
static int self_occ_pa_bcast_monitor_start;
static int remote_occ_pa_monitor_start;
static int remote_occ_pa_monitor_flag;

static int cab_pa_bcast_monitor_start;
static int func_bcast_monitor_start;
static int fire_bcast_monitor_start;
static int auto_bcast_monitor_start;
static int global_auto_func_monitor_start;

static int occ_pa_bcast_rx_running;
static int cop_pa_bcast_rx_running;
static int func_bcast_rx_running;
static int fire_bcast_rx_running;
static int auto_bcast_rx_running;

static int avc_cycle_sync;

static int global_menu_active;
static int bmenu_active;
static int bmenu_func_active;
static int bmenu_start_active;
static int bmenu_station_active;
static int bmenu_station_num;
static int bmenu_station_num_start;
static int bmenu_station_num_end;

static int bcast_updown_index;
static int bcast_func_10;
static int bcast_func_1;
static int bcast_func_index;
static int bcast_start_index;
static int bcast_start_num;
static int bcast_station_index;
static int bcast_leftright_index;
static int FuncNum;
static int Func_Is_Now_Start, FuncNum;

static int cop_cantalk_on_pei2cab_call;

static int tractor_cab2cab_start;

static int emergency_pei2cab_start;
static int pei2cab_is_emergency;

static int emergency_pei2cab_waiting;

static int waiting_cab_stop_cmd_on_waiting_cab2cab_on_emer_pei;

static int run_force_pei_call_wakeup;
static int run_force_cab_call_wakeup;
static int run_force_pei2cab_mon_on_passive;

static int pending_indoor_on_pei_wakeup;
static int pending_outdoor_on_pei_wakeup;
static int pending_inoutdoor_on_pei_wakeup;

static int enable_in_pa_simultaneous_on_pei2cab;
static int enable_out_pa_simultaneous_on_pei2cab;

static int enable_pei_wakeup_on_in_passive;
static int enable_pei_wakeup_on_out_passive;

/*static int enable_pei2cab_simultaneous_on_occ_pa;*/

static int enable_occ_pa_on_cab_pa_mon; // Ver 0.71

static int enable_occ_pa_on_in_passive;
static int enable_occ_pa_on_out_passive;

static int call_menu_active;
static int cmenu_active;

static int iovmenu_active; // In/out volume ctrl menu
static int iovol_updown_index;

static int vmenu_active; // volume ctrl menu
static int vol_updown_index;

static int pei_joined_call_idx = -1;
char conv_addr_str[32];
char* addr_to_str(int addr);
void print_call_list(int where);

void debug_print_cob_status_change(void);

static int is_call_menu_selected(void);
static int is_bcast_menu_selected(void);

static int add_call_list(int where, short status, int is_start_requested, int emergency, char car_n, char dev_n,
				int mon_multi_addr, int tx_multi_addr, int train_num);
static int del_call_list(int where, int emergency, int check_status, char car_n, char dev_n);

static int move_only_1th_wakeup_to_1th_index(int where, int is_emergnency);
static int move_only_2nd_live_call_to_2nd_index(int where, int is_emergency);
/* static int move_only_2nd_live_call_to_1st_index(int where, int is_emergency, int car_num, int dev_num); */
/*static int move_wakeup_to_1th_index(int where);*/
/*static int move_emergency_pei_wakeup_to_1th_index(void);*/

static int get_call_waiting_ctr(int where);
static int is_waiting_call(int where);

static int is_waiting_normal_call(int where, int car_num, int dev_num);
static int is_waiting_normal_or_emer_call(int where, int car_num, int dev_num);

static int is_waiting_normal_call_any(int where, int car_num, int dev_num);
static int is_waiting_normal_or_emer_call_any(int where, int car_num, int dev_num);

static int is_waiting_emergency_pei_call(void);
static int is_waiting_emergency_pei_call_any(void);
static int is_waiting_emergency_pei_call_any_2(int car_num, int dev_num);
static int is_monitoring_call(int where, int car_num, int idx_num, int any_id);
static int is_waiting_monitoring_call(int where, char *get_car_num, char *get_dev_num,
			int *mon_addr, int *tx_addr, int ayn_id);

static int is_emergency_pei_live(void);
//static int get_call_live_num(int where);
static int get_call_any_ctr(int where);

static int is_live_call(int where, int car_num, int idx_num);

static char get_call_car_num(int where);
static char get_call_id_num(int where);
static char get_call_train_num(int where);
static int get_call_mon_addr(int where);
static int get_call_tx_addr(int where);
static int get_waiting_call_pei_index(int *get_car_num, int *get_dev_num, int *get_emergency);

static int get_call_is_emergency(void);
/*static int change_emer_to_normal_pei_call(void);*/

static void passive_func_auto_fire_bcast_monitoring(COP_Cur_Status_t bcast_status);

#define BCAST_PENDING_NONE		0
#define BCAST_PENDING_REQUEST_SENDING	1
#define BCAST_PENDING_REQUEST_SEND_DONE	2
#define BCAST_PENDING_REQUEST_WAITING	3
#define BCAST_PENDING_REQUEST_DONE	4
static int func_pending;

static int indoor_pending;
static int outdoor_pending;
static int inoutdoor_pending;
static int backup_keyval;

static int indoor_stop_pending;
static int outdoor_stop_pending;

static int waiting_outstart_on_wait_in_stop;
static int waiting_outstop_on_wait_in_start;

static int cop_pa_tx_running;

static int occ_pa_pending;
int occ_pa_running;

static int indoor_waiting;
static int indoor_stop_waiting;

static int outdoor_waiting;
static int outdoor_stop_waiting;

static int now_occ_pa_start_wait;
static int now_occ_pa_start;
static int now_occ_pa_stop_wait;
static int now_occ_pa_stop;

struct multi_call_status __cab_call_list;
struct multi_call_status __pei_call_list;
struct multi_call_status __tmp_call_list;

static volatile int cab2cab_call_pending;
static int cab2cab_call_waiting;
static int cab2cab_call_running;
static int cab2cab_reject_waiting;

int pei_call_count;
static int call_mon_sending;
static int cab2cab_call_monitoring;

static int pei2cab_call_monitoring;

static volatile int pei2cab_call_pending;
static int pei2cab_call_waiting;
static int pei2cab_call_running;

#ifdef PEI_MONITORING_SAVE
static int save_file_fd;
#endif

//static int occ2cab_call_pending;
//static int cab2occ_call_pending;

static int is_occ_cmd;

static char __car_num;
static char __dev_num;
static char __train_num;
static int __mon_addr;
static int __tx_addr;
static unsigned int __rx_addr_for_mon_for_push_to_talk;
static int CurCab_ListNum;

/***************************/

#define COB_CONFIG_SETUP_FILE	"/mnt/data/cop_config.txt"

int selected_language;
static int pei_to_occ_no_mon_can_cop_pa;
//static int occ_pa_no_mon_can_pei_call;
static int occ_spk_vol_atten;
static int occ_mic_vol_atten;
static int pei_comm_half_duplex;
static int deadman_alarm_is_use;
int internal_test_use;					/* Only used for Env. test case */
int fire_alarm_is_use;
int menu_force_redraw;
int ui_update_di_and_route;
unsigned char avc_di_display_id;
unsigned char avc_departure_id;
unsigned char avc_destination_id;
int  avc_special_route_pattern;

static int enable_deadman_alarm;
int enable_fire_alarm;
#if (FIRE_ALARM_TIMER == 1)
int enable_alaram_transfer;
int fire_alarm_stime;
int fire_alarm_etime;
#endif

static int check_ssrc_set_enable;

static int pa_mon_enable_wait_play_time;
static int pa_mon_flame_play_time;
static int dummy_delay_time;

static unsigned int pa_mon_cur_ssrc;
//static struct timespec old_time, cur_time;

static void play_stop_occ_pa_mon_rx_data(int mute, int instant);
static void play_stop_cop_pa_mon_rx_data(int mute, int instant);
static void play_stop_fire_mon_rx_data(int mute, int instant);
static void play_stop_auto_mon_rx_data(int mute, int instant);
static void play_stop_func_mon_rx_data(int mute, int instant);

static void discard_cop_pa_and_call_encoding_data(COP_Cur_Status_t status);
static void discard_occ_pa_and_call_encoding_data(COP_Cur_Status_t status);
static void discard_cop_voip_call_encoding_data(COP_Cur_Status_t status);

static void if_call_selected_call_menu_draw(void);
static void force_call_menu_draw(void);
static void menu_bcast_screen_reset(void);

void fire_status_reset(int status)
{
	DBG_LOG("\n");
	menu_force_redraw = 1;
	draw_lines();

	switch(status) {
		case 1:			/* Call */
			global_menu_active = COP_MENU_BROADCAST | COP_MENU_HEAD_SELECT;
			menu_call_screen_reset();
			call_draw_menu(&__cab_call_list, &__pei_call_list, 1, 0, 0);
    		menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, 
					       cmenu_active, vmenu_active, iovmenu_active);
			break;

		case 2:			/* Broadcast */
			menu_bcast_screen_reset();
    		InOut_LedStatus = INOUT_BUTTON_STATUS_ALLOFF;
    		broadcast_draw_menu(InOut_LedStatus, 0, 0, FuncNum, 1, bmenu_active, 0);
    		menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, 
					       cmenu_active, vmenu_active, iovmenu_active);
			break;

		default:		/* IDle */
			break;
	}
	menu_force_redraw = 0;
}


void cop_status_reset(void)
{
	//DBG_LOG("\n");
    enable_deadman_alarm = 0;
    enable_fire_alarm = 0;
#if (FIRE_ALARM_TIMER == 1)
	enable_alaram_transfer = 0;
#endif

    key_init();
#if (BUSAN_COB == 1)
	if(fire_alarm_key_status)
		fire_set_for_key();
#endif
    __broadcast_status = COP_STATUS_IDLE;
    __2nd_broadcast_status = COP_STATUS_IDLE;
    __old_broadcast_status = COP_STATUS_IDLE;
    __2nd_old_broadcast_status = COP_STATUS_IDLE;

    inout_bcast_processing = 0;

    cop_pa_mic_volume_mute_set();  // LOAD 4
    occ_mic_volume_mute_set();
    cop_speaker_volume_mute_set();
    occ_speaker_volume_mute_set();

	/* For OCC PA & PEI */
	occ_pei_call_ack_set_high();
    occ_pa_ack_set_low();

    set_cop_spk_mute_onoff(0);

    clear_rx_tx_mon_multi_addr();

    close_cop_multi_rx_sock();
    close_cop_multi_rx_sock_2nd();
	close_call_multi_tx(); // 2014/12/26
	close_call_multi_tx_2nd(); // 2014/12/26
    close_cop_multi_tx();
    close_call_and_occ_pa_multi_tx();
    close_cop_multi_monitoring();
    close_cop_multi_passive_tx();

    cop_pa_mic_codec_stop();
    cop_monitoring_encoding_stop();
    decoding_stop();

    cab2cab_call_pending = 0;
    cab2cab_call_waiting = 0;
    cab2cab_call_running = 0;
    cab2cab_reject_waiting = 0;

	pei_call_count = 0;

	pei2cab_call_waiting = 0;
	pei2cab_call_running = 0;

    call_mon_sending = 0;
    cab2cab_call_monitoring = 0;
    pei2cab_call_monitoring = 0;

    is_occ_cmd = 0;

    call_list_init();

    enable_in_pa_simultaneous_on_pei2cab = 0;
    enable_out_pa_simultaneous_on_pei2cab = 0;

    enable_pei_wakeup_on_in_passive = 0;
    enable_pei_wakeup_on_out_passive = 0;

    /*enable_pei2cab_simultaneous_on_occ_pa = 0;*/

    enable_occ_pa_on_cab_pa_mon = 0;

    enable_occ_pa_on_in_passive = 0;
    enable_occ_pa_on_out_passive = 0;

	tractor_cab2cab_start = 0;

    emergency_pei2cab_start = 0;
    pei2cab_is_emergency = 0;
    emergency_pei2cab_waiting = 0;

    waiting_cab_stop_cmd_on_waiting_cab2cab_on_emer_pei = 0;

    run_force_pei_call_wakeup = 0;
    run_force_cab_call_wakeup = 0;
    run_force_pei2cab_mon_on_passive = 0;

	occ_key_lock = 0;
	cab_key_lock = 0;
	pei_key_lock = 0;
	pa_in_key_lock = 0;
	pa_out_key_lock = 0;
	func_key_lock = 0;

    global_menu_active = COP_MENU_BROADCAST | COP_MENU_HEAD_SELECT;
    bmenu_active = 1;
    bmenu_func_active = 0;
    bmenu_start_active = 0;
    bmenu_station_active = 0;
	bmenu_station_num = 1;
	bmenu_station_num_start = 1;
	bmenu_station_num_end = 2;
    FuncNum = 1;
    Func_Is_Now_Start = -1;

    cmenu_active = 0;
    call_menu_active = 0;
    bcast_updown_index = 1;
    bcast_func_10 = 0;
    bcast_func_1 = 0;
    bcast_func_index = 0;
    bcast_start_index = 0;
    bcast_start_num = 1;
    bcast_station_index = 0;
    bcast_leftright_index = 0;

    vmenu_active = 0;
    vol_updown_index = 0;

	iovmenu_active = 0;
    iovol_updown_index = 0;

	//DBG_LOG("\n");
    occ_pa_bcast_monitor_start = 0;
    self_occ_pa_bcast_monitor_start = 0;
    remote_occ_pa_monitor_start = 0;
    remote_occ_pa_monitor_flag = 0;

	occ_status = 0;

	tractor_possible = 0;
	tractor_broadcast_status = 0;

	occ_pa_enable_level = 0;
	occ_pei_call_out_level = 0;
	occ_pa_pei_enabled = 0;
	old_occ_pa_enable_level = 0;
	old_occ_pei_call_out_level = 0;
	old_occ_pa_pei_enabled = 0;

    cab_pa_bcast_monitor_start = 0;
    func_bcast_monitor_start = 0;
    fire_bcast_monitor_start = 0;
    auto_bcast_monitor_start = 0;
    global_auto_func_monitor_start = 0;

    occ_pa_bcast_rx_running = 0;
    cop_pa_bcast_rx_running = 0;
    fire_bcast_rx_running = 0;
    func_bcast_rx_running = 0;
    auto_bcast_rx_running = 0;

    avc_cycle_sync = 0;

	// mhryu : 0502015
	pa_in_Key_pressed_on_fire_status = 0;
	pa_out_Key_pressed_on_fire_status = 0;
	call_cab_Key_pressed_on_fire_status = 0;

	// mhryu : 0502015
	pei2cab_monitoring_on = 0;
	auto_monitoring_on = 0;
	occ_monitoring_on = 0;
	menu_force_redraw = 1;

    passive_in_button = 0;
    passive_out_button = 0;

	change_to_call_menu = 0;
	change_to_config_menu = 0;

	special_msg_lrm_on = 0;
	special_msg_pib_on = 0;
	pei_join_button_on = 0;

    clear_menu_screen();
    menu_bcast_screen_reset();

    menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, cmenu_active, vmenu_active, iovmenu_active);
    InOut_LedStatus = INOUT_BUTTON_STATUS_ALLOFF;
    broadcast_draw_menu(InOut_LedStatus, 0, 0, FuncNum, 1, bmenu_active, 0);

    broadcast_draw_menu(InOut_LedStatus, 0, 0, FuncNum, 1, bmenu_active, 1);
	global_menu_active = COP_MENU_BROADCAST | COP_MENU_HEAD_SELECT;
    bmenu_active = 0;	/* Changed by Michael RYU : May 15, 2015 */
    menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, cmenu_active, vmenu_active, iovmenu_active);
    broadcast_draw_menu(InOut_LedStatus, 0, 0, FuncNum, 1, bmenu_active, 1);
	menu_force_redraw = 0;

    occ_key_status = 0;
    occ_tx_rx_en_ext_flag = 0;
    old_occ_tx_rx_en_ext_flag = -1;

    occ_pa_en_status = 0;
    occ_pa_en_status_old = 0;

    ptt_key_on_pei_cab_pa_open_multi_tx = 0;

    if (pei_comm_half_duplex) {
        pei_half_way_com_flag = 1;
        cop_cantalk_on_pei2cab_call = 0;
    }
    else {
        pei_half_way_com_flag = 0;
        cop_cantalk_on_pei2cab_call = 1; // always can talk on full duplex
    }

    func_pending = 0;
    indoor_pending = 0;
    outdoor_pending = 0;
	inoutdoor_pending = 0;

    indoor_stop_pending = 0;
    outdoor_stop_pending = 0;

    waiting_outstart_on_wait_in_stop = 0;

    cop_pa_tx_running = 0;
    occ_pa_pending = 0;
    occ_pa_running = 0;

    indoor_waiting = 0;
    indoor_stop_waiting = 0;

    outdoor_waiting = 0;
    outdoor_stop_waiting = 0;

    check_ssrc_set_enable = 0;

    set_check_different_ssrc_valid(0);
    clear_old_rtp_seq();

    pa_mon_enable_wait_play_time = 0;
    pa_mon_flame_play_time = 0;
    pa_mon_cur_ssrc = 0;

    dummy_delay_time = 0;
}

void set_indicate_avc_cycle_data(void)
{
    avc_cycle_sync++;
}

void set_indicate_codec_timeout_occured(int v)
{
    if (v) {
        //dummy_delay_time = pa_mon_flame_play_time;
        //dummy_delay_time += pa_mon_flame_play_time >> 1;
        dummy_delay_time = pa_mon_flame_play_time >> 2;
//printf("add dummy timeout\n");
    } else {
        dummy_delay_time = 0;
//printf("zero dummy timeout\n");
    }
}

static void clear_pa_mon_play_time(void)
{
    pa_mon_enable_wait_play_time = 0;
    pa_mon_flame_play_time = 0;
    pa_mon_cur_ssrc = 0;
}

static void force_call_menu_draw(void)
{
	if(enable_fire_alarm || fire_alarm_key_status)
		fire_status_reset(1);

	change_to_call_menu = 1;

    if (is_call_menu_selected()) {
        call_draw_menu(&__cab_call_list, &__pei_call_list, 0, 0, 0);
	} else {
        menu_call_screen_reset();
    	menu_head_draw(global_menu_active, 0, 0, 0, 0, cmenu_active, 0, 0);
        call_draw_menu(&__cab_call_list, &__pei_call_list, 1, 0, 0);
    }
}

static void if_call_selected_call_menu_draw(void)
{
    if (is_call_menu_selected())
        call_draw_menu(&__cab_call_list, &__pei_call_list, 0, 0, 0);
}

static void menu_call_screen_reset(void)
{
    if ((global_menu_active & COP_MENU_MASK) != COP_MENU_CALL)
        clear_menu_screen();

    call_menu_active = 1;

    global_menu_active = COP_MENU_CALL | COP_MENU_HEAD_SELECT;
    bmenu_active = 0;
	bmenu_func_active = 0;
	bmenu_start_active = 0;
	bmenu_station_active = 0;
	bcast_func_index = 0;
	bcast_start_index = 0;
	bcast_station_index = 0;
    cmenu_active = 1;
    vmenu_active = 0;
    iovmenu_active = 0;
    menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, cmenu_active, vmenu_active, iovmenu_active);
}

static void menu_bcast_screen_reset(void)
{
    if ((global_menu_active & COP_MENU_MASK) != COP_MENU_BROADCAST)
        clear_menu_screen();

    call_menu_active = 0;

    global_menu_active = COP_MENU_BROADCAST | COP_MENU_HEAD_SELECT;
    bmenu_active = 0;
	bmenu_func_active = 0;
	bmenu_start_active = 0;
	bmenu_station_active = 0;
	bcast_func_index = 0;
	bcast_start_index = 0;
	bcast_station_index = 0;
    cmenu_active = 0;
    vmenu_active = 0;
    iovmenu_active = 0;
    menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, cmenu_active, vmenu_active, iovmenu_active);
}

void call_list_init(void)
{
    memset(&__cab_call_list, 0, sizeof(struct multi_call_status));
    memset(&__pei_call_list, 0, sizeof(struct multi_call_status));
    force_call_menu_draw();
}

static void tractor_call_start_prepare(void)
{
	set_occ_spk_audio_switch_on();
	set_cop_pa_mic_audio_switch_on();
	set_occ_mic_audio_switch_on();

	cop_speaker_volume_mute_set();
	set_occ_mic_to_spk_audio_switch_on();
	set_cop_spk_mute_onoff(0);

    cop_pa_and_call_mic_volume_set();
	occ_speaker_volume_set(occ_spk_vol_atten);
	occ_mic_volume_set(occ_mic_vol_atten);

    //set_cop_pa_mic_audio_switch_on();
    //set_cop_call_mic_audio_switch_on();
    //set_cop_spk_audio_switch_on();

    //set_occ_mic_audio_switch_off();

    //cop_pa_and_call_mic_volume_set();
    //cop_speaker_volume_set();
}

static void call_start_prepare(void)
{
    set_cop_pa_mic_audio_switch_on();
    set_cop_call_mic_audio_switch_on();
    set_cop_spk_audio_switch_on();

    set_occ_mic_audio_switch_off();

    cop_pa_and_call_mic_volume_set();
    cop_speaker_volume_set();
	//occ_speaker_volume_mute_set();
}

static void voip_call_start_prepare(void)
{
    set_cop_pa_mic_audio_switch_on();
    set_cop_call_mic_audio_switch_on();
    //set_cop_spk_audio_switch_on();
    set_occ_mic_audio_switch_off();
	set_cop_voip_call_spk_audio_switch_on();
	set_cop_voip_mon_mic_audio_switch_on();

    cop_pa_and_call_mic_volume_set();
    cop_speaker_volume_set();
	//occ_speaker_volume_mute_set();
}

#if 0
static void call_start_prepare_by_cop_pa_codec(void)
{
    set_cop_pa_mic_audio_switch_on(); /* PE9:0, PE10:0 */
    set_cop_spk_audio_switch_on();

    cop_pa_and_call_mic_volume_set();
    cop_speaker_volume_set();
}
#endif

static void play_stop_call_mon_rx_data(void)
{
    int k, len;
    char *rxbuf;
    //int delay;

    k = get_mon_multi_rx_ctr();
    while (k > 1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

        rxbuf = try_read_call_monitoring_data(&len, 1, 1);
        running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0);
        k--;
    }

    if (k > 0) {
        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

        rxbuf = try_read_call_monitoring_data(&len, 1, 1);
        running_bcast_recv_and_dec(0, rxbuf, len, 0, 1, 0);
    }

    reset_mon_multi_rx_data_ptr();
    pa_mon_flame_play_time = 0;
    cop_speaker_volume_mute_set();
}

static void tractor_cab2cab_call_rx_tx_running(void)
{
    int len;
    char *rxbuf;

	running_call_audio_sending_to_occ_out(0); // stop = 0
}

static void cab2cab_call_rx_tx_running(int passive_on)
{
    int len;
    char *rxbuf;

#if 1	/* Changed by Michael RYU */
	if (call_mon_sending) {
		running_call_audio_sending(passive_on, 1, 1);
	} else {
		running_call_audio_sending(passive_on, 0, 1);
	}

    rxbuf = try_read_call_rx_data(&len, 0, 1);
    running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0);
#if 0
    if (call_mon_sending)
        running_monitoring_audio_sending(0, 1);
#endif
#else
    if (CurCab_ListNum == 1) {
        running_call_audio_sending(passive_on, 1, 0);

        try_read_call_rx_data(&len, 0, 0);
        if (call_mon_sending)
            running_monitoring_audio_sending(0, 0);

    } else {
        running_call_audio_sending(passive_on, 0, 1);

        rxbuf = try_read_call_rx_data(&len, 0, 1);
        running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0);
        if (call_mon_sending)
            running_monitoring_audio_sending(0, 1);
    }
#endif
}

static void play_stop_call_rx_data(void)
{
    int k, len;
    char *rxbuf;
    //int delay;

    k = get_call_multi_rx_ctr();

    while (k > 1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
		discard_voip_call_mic_encoding_data();

        rxbuf = try_read_call_rx_data(&len, 1, 1);
		if(len > 0)
        	running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0);
        k--;
    }

    if (k > 0) {
		discard_voip_call_mic_encoding_data();
        rxbuf = try_read_call_rx_data(&len, 1, 1);
		if(len > 0)
        	running_bcast_recv_and_dec(0, rxbuf, len, 0, 1, 0);
    }

    reset_call_multi_rx_data_ptr();
    pa_mon_flame_play_time = 0;
    cop_speaker_volume_mute_set();
}

static void play_stop_call_rx_data_2nd(void)
{
    int k, len;
    char *rxbuf;
    //int delay;

    k = get_call_multi_rx_ctr_2nd();

    while (k > 1) {
		discard_voip_call_mic_encoding_data();
        rxbuf = try_read_call_rx_data_2nd(&len, 1, 1);
		if(len > 0)
        	running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0);
        k--;
    }

    if (k > 0) {
		discard_voip_call_mic_encoding_data();
        rxbuf = try_read_call_rx_data_2nd(&len, 1, 1);
		if(len > 0)
        	running_bcast_recv_and_dec(0, rxbuf, len, 0, 1, 0);
    }

    reset_call_multi_rx_data_ptr_2nd();
}


static void try_read_other_pa_monitoring_bcast_rx_data(int etype)
{
    int len;

    switch(etype) {
        case MON_BCAST_TYPE_AUTO:
            try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, 1, 0);
            try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, 1, 0);
			try_read_fire_monitoring_bcast_rx_data(&len, 0, 1, 0);
            try_read_func_monitoring_bcast_rx_data(&len, 0, 1, 0);
            break;

        case MON_BCAST_TYPE_FUNC:
            try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, 1, 0);
            try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, 1, 0);
			try_read_fire_monitoring_bcast_rx_data(&len, 0, 1, 0);
            try_read_auto_monitoring_bcast_rx_data(&len, 0, 0, 0);
            break;

		case MON_BCAST_TYPE_FIRE:
			try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, 1, 0);
			try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, 1, 0);
			try_read_func_monitoring_bcast_rx_data(&len, 0, 0, 0);
			try_read_auto_monitoring_bcast_rx_data(&len, 0, 0, 0);
			break;

        case MON_BCAST_TYPE_COP_PASSIVE:
            try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, 1, 0);
			try_read_fire_monitoring_bcast_rx_data(&len, 0, 0, 0);
            try_read_func_monitoring_bcast_rx_data(&len, 0, 0, 0);
            try_read_auto_monitoring_bcast_rx_data(&len, 0, 0, 0);
            break;

        case MON_BCAST_TYPE_OCC_PASSIVE:
            try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, 0, 0);
			try_read_fire_monitoring_bcast_rx_data(&len, 0, 0, 0);
            try_read_func_monitoring_bcast_rx_data(&len, 0, 0, 0);
            try_read_auto_monitoring_bcast_rx_data(&len, 0, 0, 0);
            break;
    }
}

static void occ_pa_monitoring_bcast_rx_dec_running(int run, int use)
{
    int len;
    char *rxbuf;

    if (run) {
        rxbuf = try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_OCC_PASSIVE, rxbuf, len, 1, 0, 0);
    } else {
        rxbuf = try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, use, 0);
    }
}

static void play_stop_occ_pa_mon_rx_data(int mute, int instant)
{
    int k, len;
    char *rxbuf;
    //int t = 0;
    int cur_ssrc, next_ssrc;
    unsigned short cur_seq, next_seq;
    volatile int delay;

    if (instant) {
        k = get_occ_pa_multi_rx_ctr();
        if (k > 2) k = 2; // to stop instantly

    if (k > 1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_occ_pa_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_OCC_PASSIVE, rxbuf, len, 1, 0, 0);
        k--;

    }

    if (k > 0) {
        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_OCC_PASSIVE);

        for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        rxbuf = try_read_occ_pa_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_OCC_PASSIVE, rxbuf, len, 0, 1, 0);
    }
    } else {

    while (1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_occ_pa_monitoring_bcast_rx_data(&len, 1, 1, 1);
        if (!len) break;

        cur_ssrc  = rxbuf[8] << 24;
        cur_ssrc |= rxbuf[9] << 16;
        cur_ssrc |= rxbuf[10]  << 8;
        cur_ssrc |= rxbuf[11];
        cur_seq = rxbuf[2] << 8 | rxbuf[3];
        next_ssrc = get_next_occ_pa_monitoring_data_ssrc();
        next_seq =  get_next_occ_pa_monitoring_data_seq();

        if (cur_ssrc == next_ssrc && (next_seq - cur_seq) == 1)
            running_bcast_recv_and_dec(MON_BCAST_TYPE_OCC_PASSIVE, rxbuf, len, 1, 0, 0);
        else {
            printf("< EXIT OCC-PA, CUR:%08X-%d, NEXT:%08X-%d >\n", cur_ssrc, cur_seq, next_ssrc, next_seq);
            running_bcast_recv_and_dec(MON_BCAST_TYPE_OCC_PASSIVE, rxbuf, len, 0, 1, 0);
            break;
        }
        if (!check_avc_cycle_data()) break;

        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_OCC_PASSIVE);
    }
    }

    if (mute) cop_speaker_volume_mute_set();

    try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_OCC_PASSIVE);
}

static void cop_pa_monitoring_bcast_rx_dec_running(int run, int use)
{
    int len;
    char *rxbuf;

    if (run) {
        rxbuf = try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_COP_PASSIVE, rxbuf, len, 1, 0, 0);
    } else {
        try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, use, 0);
    }
}

static void play_stop_cop_pa_mon_rx_data(int mute, int instant)
{
    int k, len;
    char *rxbuf;
    //int t = 0;
    int cur_ssrc, next_ssrc;
    unsigned short cur_seq, next_seq;
    volatile int delay;

    if (instant) {
        k = get_cop_pa_multi_rx_ctr();
        if (k > 2) k = 2; // to stop instantly

    if (k > 1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_cop_pa_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_COP_PASSIVE, rxbuf, len, 1, 0, 0);
        k--;
    }

    if (k > 0) {
        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_COP_PASSIVE);

        for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);

        rxbuf = try_read_cop_pa_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_COP_PASSIVE, rxbuf, len, 0, 1, 0);
    }

    } else {

    while (1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_cop_pa_monitoring_bcast_rx_data(&len, 1, 1, 1);

        if (!len) break;

        cur_ssrc  = rxbuf[8] << 24;
        cur_ssrc |= rxbuf[9] << 16;
        cur_ssrc |= rxbuf[10]  << 8;
        cur_ssrc |= rxbuf[11];
        cur_seq = rxbuf[2] << 8 | rxbuf[3];

        next_ssrc = get_next_cop_pa_monitoring_data_ssrc();
        next_seq =  get_next_cop_pa_monitoring_data_seq();

        if (cur_ssrc == next_ssrc && (next_seq - cur_seq) == 1)
            running_bcast_recv_and_dec(MON_BCAST_TYPE_COP_PASSIVE, rxbuf, len, 1, 0, 0);
        else {
            printf("< EXIT CAB-PA, CUR:%08X-%d, NEXT:%08X-%d >\n", cur_ssrc, cur_seq, next_ssrc, next_seq);
            running_bcast_recv_and_dec(MON_BCAST_TYPE_COP_PASSIVE, rxbuf, len, 0, 1, 0);
            break;
        }

        if (!check_avc_cycle_data()) break;

        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_COP_PASSIVE);
    }
    }

    if (mute) cop_speaker_volume_mute_set();

    try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_COP_PASSIVE);
}

static void play_stop_fire_mon_rx_data(int mute, int instant)
{
	int k, len;
	char *rxbuf;
	int cur_ssrc, next_ssrc;
	unsigned short cur_seq, next_seq;
	volatile int delay;

	if (instant) {
		k = get_fire_multi_rx_ctr();
		if (k > 2) k = 2; // to stop instantly

		if (k > 1) {
			discard_cop_pa_and_call_encoding_data(__broadcast_status);
			discard_occ_pa_and_call_encoding_data(__broadcast_status);

			rxbuf = try_read_fire_monitoring_bcast_rx_data(&len, 1, 1, 1); 
			running_bcast_recv_and_dec(MON_BCAST_TYPE_FIRE, rxbuf, len, 1, 0, 1);
			k--;
		}

		if (k > 0) {
			try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_FIRE);

			for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
			rxbuf = try_read_fire_monitoring_bcast_rx_data(&len, 1, 1, 1);
			running_bcast_recv_and_dec(MON_BCAST_TYPE_FIRE, rxbuf, len, 0, 1, 1);
		}
	} else {
		while(1) {
			//for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
			discard_cop_pa_and_call_encoding_data(__broadcast_status);
			discard_occ_pa_and_call_encoding_data(__broadcast_status);

			rxbuf = try_read_fire_monitoring_bcast_rx_data(&len, 1, 1, 1);
			if (!len) break;

			cur_ssrc  = rxbuf[8] << 24;
			cur_ssrc |= rxbuf[9] << 16;
			cur_ssrc |= rxbuf[10]  << 8;
			cur_ssrc |= rxbuf[11];
			cur_seq = rxbuf[2] << 8 | rxbuf[3];

			next_ssrc = get_next_fire_monitoring_data_ssrc();
			next_seq =  get_next_fire_monitoring_data_seq();

			if (cur_ssrc == next_ssrc && (next_seq - cur_seq) == 1)
				running_bcast_recv_and_dec(MON_BCAST_TYPE_FIRE, rxbuf, len, 1, 0, 1);
			else {
				printf("< EXIT FIRE, CUR:%08X-%d, NEXT:%08X-%d >\n", cur_ssrc, cur_seq, next_ssrc, next_seq);
				running_bcast_recv_and_dec(MON_BCAST_TYPE_FIRE, rxbuf, len, 0, 1, 1);
				break;
			}
			if (!check_avc_cycle_data()) break;
			try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_FIRE);
		}
	}
	if (mute) 
		cop_speaker_volume_mute_set();

	try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_FIRE);
}

static void play_stop_auto_mon_rx_data(int mute, int instant)
{
    int k, len;
    char *rxbuf;
    //int t = 0;
    int cur_ssrc, next_ssrc;
    unsigned short cur_seq, next_seq;
    volatile int delay;

    if (instant) {
        k = get_auto_multi_rx_ctr();
        if (k > 2) k = 2; // to stop instantly

    if (k > 1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_auto_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_AUTO, rxbuf, len, 1, 0, 1);
        k--;
    }

    if (k > 0) {
        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_AUTO);

        for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        rxbuf = try_read_auto_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_AUTO, rxbuf, len, 0, 1, 1);
    }

    } else {

    while (1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_auto_monitoring_bcast_rx_data(&len, 1, 1, 1);
        if (!len) break;

        cur_ssrc  = rxbuf[8] << 24;
        cur_ssrc |= rxbuf[9] << 16;
        cur_ssrc |= rxbuf[10]  << 8;
        cur_ssrc |= rxbuf[11];
        cur_seq = rxbuf[2] << 8 | rxbuf[3];

        next_ssrc = get_next_auto_monitoring_data_ssrc();
        next_seq =  get_next_auto_monitoring_data_seq();

        if (cur_ssrc == next_ssrc && (next_seq - cur_seq) == 1)
            running_bcast_recv_and_dec(MON_BCAST_TYPE_AUTO, rxbuf, len, 1, 0, 1);
        else {
            printf("< EXIT AUTO, CUR:%08X-%d, NEXT:%08X-%d >\n", cur_ssrc, cur_seq, next_ssrc, next_seq);
            running_bcast_recv_and_dec(MON_BCAST_TYPE_AUTO, rxbuf, len, 0, 1, 1);
            break;
        }

        if (!check_avc_cycle_data()) break;

        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_AUTO);
    }
    }

    if (mute) cop_speaker_volume_mute_set();

    try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_AUTO);
}

static void auto_monitoring_bcast_rx_dec_running(int run, int use)
{
    int len;
    char *rxbuf;

    if (run) {
        rxbuf = try_read_auto_monitoring_bcast_rx_data(&len, 0, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_AUTO, rxbuf, len, 1, 0, 1);
    } else {
        try_read_auto_monitoring_bcast_rx_data(&len, 0, use, 0);
    }
}

static void fire_monitoring_bcast_rx_dec_running(int run, int use)
{
	int len;
	char *rxbuf;

	if (run) {
		rxbuf = try_read_fire_monitoring_bcast_rx_data(&len, 0, 1, 1);
		running_bcast_recv_and_dec(MON_BCAST_TYPE_FIRE, rxbuf, len, 1, 0, 1);
	} else {
		try_read_fire_monitoring_bcast_rx_data(&len, 0, use, 0);
	}
}

static void func_monitoring_bcast_rx_dec_running(int run, int use)
{
    int len;
    char *rxbuf;

    if (run) {
        rxbuf = try_read_func_monitoring_bcast_rx_data(&len, 0, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_FUNC, rxbuf, len, 1, 0, 1);
    } else {
        try_read_func_monitoring_bcast_rx_data(&len, 0, use, 0);
    }
}

static void play_stop_func_mon_rx_data(int mute, int instant)
{
    int k, len;
    char *rxbuf;
    //int t = 0;
    int cur_ssrc, next_ssrc;
    unsigned short cur_seq, next_seq;
    volatile int delay;

    if (instant) {
        k = get_func_multi_rx_ctr();
        if (k > 2) k = 2; // to stop instantly

    if (k > 1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_func_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_FUNC, rxbuf, len, 1, 0, 1);
        k--;
    }

    if (k > 0) {
        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_FUNC);

        for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        rxbuf = try_read_func_monitoring_bcast_rx_data(&len, 1, 1, 1);
        running_bcast_recv_and_dec(MON_BCAST_TYPE_FUNC, rxbuf, len, 0, 1, 1);
    }

    } else {

    while (1) {
        //for (delay = 0; delay < SIMPLE_DELAY_CTR; delay++);
        discard_cop_pa_and_call_encoding_data(__broadcast_status);
        discard_occ_pa_and_call_encoding_data(__broadcast_status);

        rxbuf = try_read_func_monitoring_bcast_rx_data(&len, 1, 1, 1);

        if (!len) break;

        cur_ssrc  = rxbuf[8] << 24;
        cur_ssrc |= rxbuf[9] << 16;
        cur_ssrc |= rxbuf[10]  << 8;
        cur_ssrc |= rxbuf[11];
        cur_seq = rxbuf[2] << 8 | rxbuf[3];

        next_ssrc = get_next_func_monitoring_data_ssrc();
        next_seq =  get_next_func_monitoring_data_seq();

        if (cur_ssrc == next_ssrc && (next_seq - cur_seq) == 1)
            running_bcast_recv_and_dec(MON_BCAST_TYPE_FUNC, rxbuf, len, 1, 0, 1);
        else {
            printf("< EXIT FUNC, CUR:%08X-%d, NEXT:%08X-%d >\n", cur_ssrc, cur_seq, next_ssrc, next_seq);
            running_bcast_recv_and_dec(MON_BCAST_TYPE_FUNC, rxbuf, len, 0, 1, 1);
            break;
        }

        if (!check_avc_cycle_data()) break;

        try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_FUNC);
    }
    }

    if (mute) cop_speaker_volume_mute_set();

    try_read_other_pa_monitoring_bcast_rx_data(MON_BCAST_TYPE_FUNC);
}

//####################################################

static void discard_cop_pa_and_call_encoding_data(COP_Cur_Status_t status)
{
    if (!inout_bcast_processing && !cab2cab_call_running && !pei2cab_call_running) {
        __discard_cop_pa_and_call_mic_encoding_data();
    }
}

static void discard_occ_pa_and_call_encoding_data(COP_Cur_Status_t status)
{
    int ret = 0;
    if (/*!pei2occ_call_running &&*/ !occ_pa_running) {
        __discard_occ_pa_and_call_mic_encoding_data();
    }
}

static void discard_cop_voip_call_encoding_data(COP_Cur_Status_t status)
{
	int ret = 0;
	if(!pei2cab_call_running) {
		discard_voip_call_mic_encoding_data();
	}
}

/*******************************************************************/
static void pei_led_control_on_pei2cab_call(void)
{
    int stat;

    stat = is_live_call(PEI_LIST, 0, 0);
    if (   !is_waiting_normal_call_any(PEI_LIST, 0, 0)
        && !is_waiting_emergency_pei_call_any_2(0, 0) && !stat)
    {
        clear_key_led(enum_COP_KEY_CALL_PEI);
        set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
    }
    else if (   !is_waiting_normal_call_any(PEI_LIST, 0, 0)
        && !is_waiting_emergency_pei_call_any_2(0, 0) && stat)
    {
        set_key_led(enum_COP_KEY_CALL_PEI);
        set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
    }
}

static void pei_led_off_if_no_wait_pei_call(void)
{
    if (!is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
        clear_key_led(enum_COP_KEY_CALL_PEI);
        set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
    }
}

static void pei_led_on_if_no_wait_pei_call(void)
{
    if (!is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
        set_key_led(enum_COP_KEY_CALL_PEI);
        set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
    }
}

static void pei_led_off_if_no_wait_normal_pei_and_emer_pei_call(void)
{
    if (!is_waiting_normal_call_any(PEI_LIST, 0, 0) && !is_waiting_emergency_pei_call_any()) {
        clear_key_led(enum_COP_KEY_CALL_PEI);
        set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
    }
}
/*******************************************************************/

/*******************************************************************/
static void cab_led_control_on_cab2cab_call(void)
{
    int stat;

    stat = is_live_call(CAB_LIST, 0, 0);
    if (!is_waiting_normal_call_any(CAB_LIST, 0, 0) && !stat) {
        clear_key_led(enum_COP_KEY_CALL_CAB);
        set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);
    }
    else if (!is_waiting_normal_call_any(CAB_LIST, 0, 0) && stat) {
        set_key_led(enum_COP_KEY_CALL_CAB);
        set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);
    }
}

static void cab_led_off_if_no_wait_cab_call(void)
{
    if (!is_waiting_normal_call_any(CAB_LIST, 0, 0)) {
        clear_key_led(enum_COP_KEY_CALL_CAB);
        set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);
    }
}

static void cab_led_on_if_no_wait_cab_call(void)
{
    if (!is_waiting_normal_call_any(CAB_LIST, 0, 0)) {
        set_key_led(enum_COP_KEY_CALL_CAB);
        set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);
    }
}

static void cab_led_off_if_no_wait_cab_call_or_lef_blink_if_wait(void)
{
    if (!is_waiting_normal_call_any(CAB_LIST, 0, 0)) {
        clear_key_led(enum_COP_KEY_CALL_CAB);
        set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);
    } else {
		set_key_led(enum_COP_KEY_CALL_CAB);
        set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);
	}
}
/*******************************************************************/

int cop_bcast_status_update (enum __cop_key_enum keyval, char *buf, int rxlen,
            int *is_reboot, int tcp_connected, int ip_id_nok)
{
    int idx, ret = 0, i, tmp;
    int stat = 0, stat2 = 0, stat3 = 0, is_emer_call;
    int used_len = 0;
    int len = 0, len_2nd = 0;
    char *rxbuf, *rxbuf_2nd, *mixed_buf;
#if 0
    int key_is_long;
	key_is_long = keyval >> 16;
if (key_is_long)
    printf("%s, LONG KEY:%d, KEY: 0x%x\n", __func__, key_is_long, keyval);
#endif

#if 0 // 2013/11/26
    if (ip_id_nok)
        return 0;
#endif

#if 0	//OCC Enable
	if(b_received_first_cmd_from_avc)
	{
		if (occ_pa_en_get_level()) {
			occ_pa_enable_level = 1;
			occ_pa_pei_enabled = 1;
		} else {
			occ_pa_enable_level = 0;
			occ_pa_pei_enabled = 0;
		}

		if((old_occ_pa_enable_level==0) && (occ_pa_enable_level == 1) ) {
			if(occ_pa_pei_enabled == 1) {
				/* OCC PA */
				DBG_LOG("OCC PA ENABLE BY SIGNAL\n");
				//ret = Func_Cop_Key_Process_List[7](0, tcp_connected);
				ret = __occ_key_process(1, 0);
				if (ret != 0)
					return -1;
			} else {
				printf("-ERR- : Unknown occ_pa_pei_enabled(%d)\n", occ_pa_pei_enabled);
			}
		} else if ( (old_occ_pa_enable_level==1) && (occ_pa_enable_level ==0) ) {
			if(old_occ_pa_pei_enabled == 1) {
				/* OCC PA Disabled */
				DBG_LOG("OCC PA DISABLE BY SIGNAL\n");
				//ret = Func_Cop_Key_Process_List[7](0, tcp_connected);
				ret = __occ_key_process(1, 0);
				if (ret != 0)
					return -1;
			} else {
				printf("-ERR- : Unknown old_occ_pa_pei_enabled(%d)\n", old_occ_pa_pei_enabled);
			}
		}

		old_occ_pa_enable_level = occ_pa_enable_level;
		old_occ_pa_pei_enabled = occ_pa_pei_enabled;
	}
#endif

	idx = (int)keyval & 0x1f;

	if (idx > 0 && (idx < enum_COP_KEY_MAX)) {
		//DBG_LOG("Key ID = %d\n", idx);
		ret = Func_Cop_Key_Process_List[idx](keyval, tcp_connected);
		if (ret != 0)
			return -1;
	}
	else if (idx < 0) {
		//printf("%s(), Invalid key: %d\n", __func__, idx);
		return -1;
	}

	while(occ_key_lock || cab_key_lock || pei_key_lock || pa_in_key_lock || pa_out_key_lock || func_key_lock) {
		usleep(1000);
	}

    if (rxlen && buf && rxlen >= (int)buf[0]) {
#if (BUSAN_COB == 1)
        if (deadman_alarm_is_use) {
            ret = check_avc2cop_cmd(buf, rxlen, IS_DEADMAN_CMD, 0);
            if (ret == 0) {
                ret = check_avc2cop_cmd(buf, rxlen, GET_DEADMAN_CMD, 0);
                if (ret == 0x01) {
                    enable_deadman_alarm = 1;
                    start_play_deadman_alarm();
                    set_cop_spk_audio_switch_on();

                    cop_pa_mic_volume_mute_set();
                    occ_mic_volume_mute_set();
                    deadman_cop_speaker_volume_set();

                    occ_speaker_volume_mute_set(); // <===
                    printf("< DEADMAN SET >\n");

                    deadman_set_for_key();
                } else if (ret == 0x02) {
                    enable_deadman_alarm = 0;
                    printf("< DEADMAN CLEAR >\n");
                    stop_play_deadman_alarm();
                    deadman_cop_speaker_volume_mute_set();

                    if (cab2cab_call_running || pei2cab_call_running) {
                        for (i = 0; i < 150; i++) {
                            rxbuf = try_read_call_rx_data(&len, 0, 1);
                            if (len == 0) break;
                        }
                    }
                    reset_call_multi_rx_data_ptr();

                    if (occ_pa_bcast_rx_running) {
                        for (i = 0; i < 150; i++) {
                            try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, 0, 0);
                            if (len == 0) break;
                        }
                    }
                    reset_occ_pa_multi_rx_data_ptr();

                    if (cop_pa_bcast_rx_running) {
                        for (i = 0; i < 150; i++) {
                            try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, 0, 0);
                            if (len == 0) break;
                        }
                    }
                    reset_cop_pa_multi_rx_data_ptr();

					if (fire_bcast_rx_running) {
						for (i=0; i<150; i++) {
							try_read_fire_monitoring_bcast_rx_data(&len, 0, 0, 0);
							if (len == 0) break;
						}
					}
                    reset_fire_multi_rx_data_ptr();

                    if (func_bcast_rx_running) {
                        for (i = 0; i < 150; i++) {
                            try_read_func_monitoring_bcast_rx_data(&len, 0, 0, 0);
                            if (len == 0) break;
                        }
                    }
                    reset_func_multi_rx_data_ptr();

                    if (auto_bcast_rx_running) {
                        for (i = 0; i < 150; i++) {
                            try_read_auto_monitoring_bcast_rx_data(&len, 0, 0, 0);
                            if (len == 0) break;
                        }
                    }
                    reset_auto_multi_rx_data_ptr();

                    deadman_clear_for_key();
                }
            }
        }
#endif
#if 0
        // Ver 0.75, 2014/07/30
        if (fire_alarm_is_use) {
			ret = check_avc2cop_cmd(buf, rxlen, IS_FIRE_ALARM_CMD, 0);
			if(ret == 0) {
				ret = check_avc2cop_cmd(buf, rxlen, GET_FIRE_ALARM_CMD, 0);
				if(ret == 0x01) {
 	                fire_bcast_monitor_start = 1;
                    enable_fire_alarm = 1;
#if (FIRE_ALARM_TIMER == 1)
					enable_alaram_transfer = 1;
					time(&fire_alarm_stime);
#endif
					//set_cop_spk_audio_switch_on();
					//deadman_cop_speaker_volume_set();
					//start_play_fire_alarm();

					//cop_pa_mic_volume_mute_set();
					//occ_mic_volume_mute_set();

					//occ_speaker_volume_mute_set();
					printf("< FIRE ALRAM SET >\n");
					fire_set_for_key();
					lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
				} else if(ret == 0x02) {
 	                fire_bcast_monitor_start = 0;
                    enable_fire_alarm = 0;
#if (FIRE_ALARM_TIMER == 1)
					enable_alaram_transfer = 0;
#endif
					printf("< FIRE ALARM CLEAR >\n");

					//deadman_cop_speaker_volume_mute_set();
					//stop_play_fire_alarm();

	                if (cab2cab_call_running || pei2cab_call_running) {
    	                for (i = 0; i < 150; i++) {
        	                rxbuf = try_read_call_rx_data(&len, 0, 1);
            	            if (len == 0) break;
                	    }
	                }
    	            reset_call_multi_rx_data_ptr();
	
    	            if (occ_pa_bcast_rx_running) {
        	            for (i = 0; i < 150; i++) {
            	            try_read_occ_pa_monitoring_bcast_rx_data(&len, 0, 0, 0);
                	        if (len == 0) break;
	                    }
    	            }
        	        reset_occ_pa_multi_rx_data_ptr();
	
    	            if (cop_pa_bcast_rx_running) {
        	            for (i = 0; i < 150; i++) {
            	            try_read_cop_pa_monitoring_bcast_rx_data(&len, 0, 0, 0);
                	        if (len == 0) break;
	                    }
    	            }
        	        reset_cop_pa_multi_rx_data_ptr();

					if (fire_bcast_rx_running) {
						for (i = 0; i < 150; i++) {
							try_read_fire_monitoring_bcast_rx_data(&len, 0, 0, 0);
							if (len == 0) break;
						}
					}
					reset_fire_multi_rx_data_ptr();

	                if (func_bcast_rx_running) {
    	                for (i = 0; i < 150; i++) {
        	                try_read_func_monitoring_bcast_rx_data(&len, 0, 0, 0);
            	            if (len == 0) break;
                	    }
	                }
	                reset_func_multi_rx_data_ptr();
	
	                if (auto_bcast_rx_running) {
	     	            for (i = 0; i < 150; i++) {
            	            try_read_auto_monitoring_bcast_rx_data(&len, 0, 0, 0);
                	        if (len == 0) break;
	                    }
    	            }
        	        reset_auto_multi_rx_data_ptr();

            	    fire_clear_for_key();
					clear_alarm_screen();
					cop_status_reset();
				}
			}
		}
#endif

#if 0
        ret = check_avc2cop_cmd(buf, rxlen, COP_REBOOT_CMD, 0);
        if (ret == 0) {
            used_len = rxlen;

            printf("< GET REBOOT CMD\n");
            *is_reboot = 1;

            cop_pa_mic_volume_mute_set();
            cop_speaker_volume_mute_set();

            close_cop_multi_rx_sock();
            close_cop_multi_rx_sock_2nd();
            close_cop_multi_tx();
            close_call_and_occ_pa_multi_tx();
            close_cop_multi_monitoring();

            close_auto_fire_func_monitoring_bcast();

            decoding_stop();

            printf("<< **** REBOOT CMD **** >>\n");
            sleep(1);
            set_watchdog_trigger_none();
            system("reboot");

            return used_len;
        }
#endif

		ret = check_avc2cop_cmd(buf, rxlen, GET_TRAIN_NUM, 0);
		if (ret == 0)
		{
			if(buf[36] != train_num)
			{
				train_num = buf[36];
				printf("< GET_TRAIN_NUM - changed train num : %d\n", train_num);
			}
		}

        ret = check_avc2cop_cmd(buf, rxlen, UI_DI_DISPLAY_CMD, 0);
        if (ret == 0) {
            used_len = rxlen;

			if(buf[39] & 0x02)
			{
				avc_di_display_id = buf[42];
				ui_update_di_and_route = 1;
            	printf("< GET UI_DI_DISPLAY_CMD - START, id=%d\n",buf[42]);
			}
			else if(buf[39] & 0x04)
			{
				avc_di_display_id = 0;
				ui_update_di_and_route = 1;
            	printf("< GET UI_DI_DISPLAY_CMD - STOP\n");
			}

            return used_len;
        }

        ret = check_avc2cop_cmd(buf, rxlen, UI_SET_ROUTE_CMD, 0);
        if (ret == 0) {
			char str[5]={0,};
            used_len = rxlen;

			avc_departure_id = buf[40];
			avc_destination_id = buf[41];

			if(buf[39] & 0x10)	//OK
			{
				ui_update_di_and_route = 1;
				strcpy(str, "OK");
			}
			else if(buf[39] & 0x20)	//NG
			{
				ui_update_di_and_route = -1;
				strcpy(str, "NG");
			}
			else
			{
				ui_update_di_and_route = 0;
				printf("UI_SET_ROUTE_CMD OK? NG? need to check\n");
				strcpy(str, "00?");
			}

            printf("< GET UI_SET_ROUTE_CMD %d -> %d (%s)\n",buf[40], buf[41], str);

            return used_len;
        }

        ret = check_avc2cop_cmd(buf, rxlen, UI_SET_SPECIAL_ROUTE_CMD, 0);
        if (ret == 0) {
			char str[5]={0,};
            used_len = rxlen;

			avc_special_route_pattern = ((buf[44]<<8)|buf[43]);

			//printf("pattern(%d), buf[45]=(%d)\n", avc_special_route_pattern, buf[45]);
			if(buf[45] & 0x41)	//OK
			{
				ui_update_di_and_route = 1;
				strcpy(str, "OK");
			}
			else if(buf[45] & 0x02) //Clear
			{
				ui_update_di_and_route = 1;
				avc_special_route_pattern = -1;
				strcpy(str, "Clear");
			}
			else if(buf[45] & 0x81)	//NG
			{
				ui_update_di_and_route = -1;
				avc_special_route_pattern = -1;
				strcpy(str, "NG");
			}
			else
			{
				ui_update_di_and_route = 0;
				printf("UI_SET_SPECIAL_ROUTE_CMD OK? NG? need to check\n");
				strcpy(str, "00?");
			}

            printf("< GET UI_SET_SPECIAL_ROUTE_CMD pattern= %d (%s)\n", avc_special_route_pattern ,str);

            return used_len;
        }
    }

    ret = check_avc2cop_cmd(buf, rxlen, IS_ONLY_LED_FLAG, 0);
    if (ret > 0) {
        used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
        if (!inout_bcast_processing) {
            InOut_LedStatus = ret;
            update_inout_key_status(ret, "HEAD");
        }
#elif defined(INOUT_LED_ONOFF_BY_AVC)
        if (!inout_bcast_processing) {
            if (ret != INOUT_BUTTON_STATUS_ALLOFF) {
                if (!occ_pa_bcast_rx_running) {
                    cab_pa_bcast_monitor_start = 1;
					if(buf[37] & 0x80)	
						pamp_in_vol = (int)(buf[37] & 0x7f);

                    printf("< CAB-PA START in HEAD >\n");
                } else
                    printf("< IGNORE CAB-PA START in HEAD >\n");
            } else {
                if (!cab_pa_bcast_monitor_start) {
                    reset_cop_pa_multi_rx_data_ptr();
                    reset_func_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_auto_multi_rx_data_ptr(); //
                }
    
                printf("< CAB-PA STOP in HEAD >\n");
                cab_pa_bcast_monitor_start = 0;
            }
            InOut_LedStatus = ret;
        }
        update_inout_key_status(ret, "HEAD");
#endif

        if (is_bcast_menu_selected()) {
			if ( (fire_alarm_is_use && enable_fire_alarm) || (fire_alarm_key_status) ) {
				if ( (InOut_LedStatus != INOUT_BUTTON_STATUS_ALLOFF) 
					 || pa_in_Key_pressed_on_fire_status 
					 || pa_out_Key_pressed_on_fire_status 
					 || call_cab_Key_pressed_on_fire_status) 
				{
					menu_force_redraw = 1;
					clear_alarm_screen();
    				//menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, cmenu_active, vmenu_active, iovmenu_active);

            		menu_bcast_screen_reset();
            		//menu_head_draw(global_menu_active, bmenu_active, 0, 0, 0);
					broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0, FuncNum, 1, bmenu_active, Func_Is_Now_Start);
					menu_force_redraw = 0;
				} else {
					lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
				}
			} else {
				menu_force_redraw = 0;
            	broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0, FuncNum, 0, bmenu_active, Func_Is_Now_Start);
			}
		} else if (!get_call_any_ctr(PEI_LIST) && !get_call_any_ctr(CAB_LIST)) {
            menu_bcast_screen_reset();
    		menu_head_draw(global_menu_active, bmenu_active, 0, 0, 0, 0, 0, 0);
            broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0, FuncNum, 1, bmenu_active, Func_Is_Now_Start);
        }
        rxlen = 0;
    }

    if (rxlen && buf && rxlen >= (int)buf[0]) {
        ret = check_avc2cop_cmd(buf, rxlen, ONLY_PAMP_VOLUME, 0);
        if (ret == 0) {
            used_len = rxlen;

			if(buf[37] & 0x80)
				pamp_in_vol = (int)(buf[37] & 0x7f);

			if (buf[38] & 0x80)
				pamp_out_vol = (int)(buf[38] & 0x7f);
			
			inout_spk_volume_setup(&global_menu_active, &iovmenu_active, &iovol_updown_index, pamp_in_vol, pamp_out_vol);
            printf("< SET IN/OUT VOL in PAMP: IN(%d), OUT(%d) >\n", pamp_in_vol, pamp_out_vol);
            rxlen = 0;
        }

        ret = check_avc2cop_cmd(buf, rxlen, ENABLE_GLOBAL_START_FUNC_AUTO, 0);
        if (ret == 0) {
            printf("< GLOBAL FUNC,AUTO START ENABLE >\n");
            global_auto_func_monitor_start = 1;
            //used_len = rxlen;
            //rxlen = 0;
        }

        ret = check_avc2cop_cmd(buf, rxlen, DISABLE_GLOBAL_START_FUNC_AUTO, 0);
        if (ret == 0) {
            printf("< GLOBAL FUNC,AUTO START DISABLE >\n");
            global_auto_func_monitor_start = 0;
            //used_len = rxlen;
            //rxlen = 0;
        }

        ret = check_avc2cop_cmd(buf, rxlen, FUNC_BCAST_START, 0);
        if (ret == 0)
        {
          if (global_auto_func_monitor_start) { //Ver0.83, 2014/09/13
            used_len = rxlen;

			if(buf[37] & 0x80)
				pamp_in_vol = (int)(buf[37] & 0x7f); // Ver 0.73, 2014/03/06

            if (!occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running && !fire_bcast_rx_running) {
                func_bcast_monitor_start = 1;
                printf("< FUNC START in HEAD, IN-VOL:%d >\n", pamp_in_vol);

                Func_Is_Now_Start = ((buf[4]<<8)|buf[3]);
#if 0
                if (is_bcast_menu_selected()) {
                    broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0,
										FuncNum, 0, bmenu_active, Func_Is_Now_Start);
					DBG_LOG("\n");
				} else if (!get_call_any_ctr(PEI_LIST) && !get_call_any_ctr(CAB_LIST)) {
					menu_bcast_screen_reset();
    				menu_head_draw(global_menu_active, bmenu_active, 0, 0, 0, 0, 0, 0);
                    broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0,
										FuncNum, 1, bmenu_active, Func_Is_Now_Start);
                }
#endif
            } else
                printf("< IGNORE FUNC START in HEAD >\n");

          } else {
            printf("< IGNORE FUNC START by Disabled GLOBAL START >\n");
            used_len = rxlen;
          }

          rxlen = 0;
        }

        if (ret == -1) {
 	       ret = check_avc2cop_cmd(buf, rxlen, FUNC_BCAST_STOP, 0);
 	       if (ret == 0) {
 	           used_len = rxlen;

 	           if (!func_bcast_monitor_start) {
 	               reset_func_multi_rx_data_ptr();
 	               reset_fire_multi_rx_data_ptr();
 	               reset_auto_multi_rx_data_ptr();
 	           }

 	           printf("< FUNC STOP in HEAD >\n");
 	           func_bcast_monitor_start = 0;
 	           Func_Is_Now_Start = -1;

#if 0
 	           if (is_bcast_menu_selected()) {
 	               broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0, FuncNum, 0, bmenu_active, Func_Is_Now_Start);
				   DBG_LOG("\n");
		  	   } else if (!get_call_any_ctr(PEI_LIST) && !get_call_any_ctr(CAB_LIST)) {
 	               menu_bcast_screen_reset();
 	               menu_head_draw(global_menu_active, bmenu_active, 0, 0, 0, 0, 0, 0);
 	               broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0, FuncNum, 1, bmenu_active, Func_Is_Now_Start);
				   DBG_LOG("\n");
 	           }
#endif

 	           rxlen = 0;
 	       }
        }

		if (ret == -1) {
			ret = check_avc2cop_cmd(buf, rxlen, FUNC_BCAST_REJECT, 0);
			if (ret == 0) {
				used_len = rxlen;
				printf("<FUNC REJECT in HEAD >\n");
				Log_Status = LOG_FUNC_REJECT;
				rxlen = 0;
			}
		}

#if 0
		if (fire_alarm_is_use) {
			if (ret == -1) {
				ret = check_avc2cop_cmd(buf, rxlen, FIRE_BCAST_START, 0);
				if (ret == 0) {
					if (global_auto_func_monitor_start) { // Ver0.83, 2014/09/13
						enable_fire_alarm = 1;
						used_len = rxlen;
						if(buf[37] & 0x80)
							pamp_in_vol = (int)(buf[37] & 0x7f); // Ver 0.73, 2014/03/06

						if (!occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running) {
							fire_bcast_monitor_start = 1;
							printf("< FIRE START in HEAD, IN-VOL:%d >\n", pamp_in_vol);
							fire_alarm_key_status = 1;
#if (BUSAN_COB == 1)
							fire_set_for_key();
#endif
							lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
							auto_monitoring_on = 1;
						} else {
							printf("< IGNORE FIRE START in HEAD  >\n");
						}
					} else {
						printf("< IGNORE FIRE START by Disabled GLOBAL START >\n");
						used_len = rxlen;
					}
					rxlen = 0;
				}
			}

			if (ret == -1) {
				ret = check_avc2cop_cmd(buf, rxlen, FIRE_BCAST_STOP, 0);
				if (ret == 0) {
					enable_fire_alarm = 0;
					if (!fire_bcast_monitor_start)
						reset_fire_multi_rx_data_ptr();

					used_len = rxlen;

					printf("< FIRE STOP in HEAD >\n");
					auto_monitoring_on = 0;
					fire_bcast_monitor_start = 0;

            	    fire_clear_for_key();			// mhryu : May 29, 2015
					// Should be added the code that is not reset the screen when using PEI / Tractor / CAB
					clear_alarm_screen();
					fire_alarm_key_status = 0;
#if (BUSAN_COB == 1)	
					switch(__broadcast_status) {
						case COP_STATUS_IDLE:		
							fire_status_reset(2);
							break;
						case OCC_PA_BCAST_START:
						case FUNC_BCAST_START:
						case __PASSIVE_INDOOR_BCAST_START:
						case __PASSIVE_OUTDOOR_BCAST_START:
						case __PASSIVE_INOUTDOOR_BCAST_START:
							fire_status_reset(2);
							break;
						case __PEI2CAB_CALL_START:
						case __CAB2CAB_CALL_START:
						case OCC2PEI_CALL_START:
						case __CAB2CAB_CALL_MONITORING_START:
						case __PEI2CAB_CALL_MONITORING_START:
							fire_status_reset(1);
							break;
					}
#endif
					rxlen = 0;
				}
			}
		}
#endif

        if (ret == -1) {
			ret = check_avc2cop_cmd(buf, rxlen, AUTO_BCAST_START, 0);
			if (ret == 0) {
				if (global_auto_func_monitor_start) { // Ver0.83, 2014/09/13
					used_len = rxlen;
	
					if(buf[37] & 0x80)
						pamp_in_vol = (int)(buf[37] & 0x7f); // Ver 0.73, 2014/03/06
	
					if (!occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running && !fire_bcast_rx_running && !func_bcast_rx_running) {
						auto_monitoring_on = 1;
						auto_bcast_monitor_start = 1;
						printf("< AUTO START in HEAD, IN-VOL:%d >\n", pamp_in_vol);
#if (BUSAN_COB == 1)	
						if(check_avc2cop_cmd(buf, rxlen, AUTO_FIRE_BCAST_START, 0) == 0) {
							printf("< AUTO START in HEAD, IN-VOL:%d >\n", pamp_in_vol);
							fire_alarm_key_status = 1;
							fire_set_for_key();
							if(__broadcast_status != __PEI2CAB_CALL_WAKEUP)
								lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
							else
								fire_status_reset(1);		// Added by Michael RYU(July 3, 2015)
						}
#endif
					} else {
						printf("< IGNORE AUTO START in HEAD  >\n");
					}
				} else {
					printf("< IGNORE AUTO START by Disabled GLOBAL START >\n");
					used_len = rxlen;
				}
				rxlen = 0;
			}
		}

		if (ret == -1) {
			ret = check_avc2cop_cmd(buf, rxlen, AUTO_BCAST_STOP, 0);
			if (ret == 0) {
				if (!auto_bcast_monitor_start)
					reset_auto_multi_rx_data_ptr();

				used_len = rxlen;

				auto_monitoring_on = 1;
				printf("< AUTO STOP in HEAD >\n");
				auto_bcast_monitor_start = 0;

#if (BUSAN_COB == 1)	
				if(check_avc2cop_cmd(buf, rxlen, AUTO_FIRE_BCAST_STOP, 0) == 0) {
					printf("< AUTO(FIRE) STOP in HEAD >\n");
					fire_alarm_key_status = 0;
					fire_clear_for_key();
					clear_alarm_screen();
					switch(__broadcast_status) {
						//case COP_STATUS_IDLE:		cop_status_reset();			break;
						case COP_STATUS_IDLE:
							fire_status_reset(2);
							break;
						case OCC_PA_BCAST_START:
						case FUNC_BCAST_START:
						case __PASSIVE_INDOOR_BCAST_START:
						case __PASSIVE_OUTDOOR_BCAST_START:
						case __PASSIVE_INOUTDOOR_BCAST_START:
							fire_status_reset(2);
							break;
						case __PEI2CAB_CALL_START:
						case __CAB2CAB_CALL_START:
						case OCC2PEI_CALL_START:
						case __CAB2CAB_CALL_MONITORING_START:
						case __PEI2CAB_CALL_MONITORING_START:
							fire_status_reset(1);
							break;
					}
				}
#endif
				rxlen = 0;
			}
		}
#if 1   // __PEI2CAB_CALL_STOP delete emergency waiting call
        if(ret == -1)
        {     
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_STOP, 0);
            if(ret == 0) 
            {     
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);

                /*    
                   stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                   if (stat3 == 0) is_emer_call = 1;
                   else {
                   is_emer_call = 0;
                   printf("\n< ON PEI WAKEUP, CANNOT SUPPORT NORMAL PEI-CALL STOP(%d-%d) >\n",
                   __car_num, __dev_num);
                   break;
                   }
                 */

                if (is_waiting_emergency_pei_call_any_2(__car_num, __dev_num)) {
                    stat = del_call_list(PEI_LIST, 1, 0, __car_num, __dev_num);
                    if (!stat) {
                        used_len = rxlen;

                        printf("\n< DEL WAIT EMER PEI(%d-%d) in HEAD >\n", __car_num, __dev_num);

						if(is_waiting_normal_or_emer_call_any(PEI_LIST, 0, 0))
						{
							set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
						}
						else if (is_live_call(PEI_LIST, 0, 0))
						{
							set_key_led(enum_COP_KEY_CALL_PEI);
						}
						else if (occ_pa_bcast_rx_running || cop_pa_bcast_rx_running || func_bcast_rx_running ||
									pei2cab_call_monitoring || cab2cab_call_monitoring)
						{
							clear_key_led(enum_COP_KEY_CALL_PEI);
							set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
						}
						else
						{
							clear_key_led(enum_COP_KEY_CALL_PEI);
							set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
							cop_speaker_volume_mute_set();
							stop_play_beep();
							decoding_stop();
						}
						
                        force_call_menu_draw();

						if( !inout_bcast_processing && !cab2cab_call_running && !pei2cab_call_running
							&& !is_waiting_normal_or_emer_call_any(PEI_LIST, 0, 0) 
							&& !occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running 
							&& !func_bcast_rx_running && !auto_bcast_rx_running
						  	&& !pei2cab_call_monitoring
						  )
						{
							printf("go COP_STATUS_IDLE\n");
                        	__broadcast_status = COP_STATUS_IDLE;
						}
						else
						{
							printf("Don't go IDLE : %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
								inout_bcast_processing, cab2cab_call_running, pei2cab_call_running,
								is_waiting_normal_or_emer_call_any(PEI_LIST, 0, 0),
								occ_pa_bcast_rx_running, cop_pa_bcast_rx_running,
								func_bcast_rx_running, auto_bcast_rx_running, pei2cab_call_monitoring);

						}
                    }     
                }     
                //reset_func_multi_rx_data_ptr();
                //reset_fire_multi_rx_data_ptr(); //
                //reset_auto_multi_rx_data_ptr(); 
            }
        }
#endif
	}

    if (   __broadcast_status != OCC_PA_BCAST_START
        && __broadcast_status != WAITING_OCC_PA_BCAST_STOP
        && __broadcast_status != OCC_PA_BCAST_STOP
        /*** Ver 0.71, 2014/01/08 *******************************/
        && __broadcast_status != __PASSIVE_INDOOR_BCAST_START
        && __broadcast_status != __PASSIVE_OUTDOOR_BCAST_START
        && __broadcast_status != __PASSIVE_INOUTDOOR_BCAST_START
       /*********************************************************/
       )
    {
        ret = check_avc2cop_cmd(buf, rxlen, OCCPA_BCAST_STATUS_START, 0);
        if (ret == 0) {
            used_len = rxlen;

			if(buf[37] & 0x80)
				pamp_in_vol = (int)(buf[37] & 0x7f); // 2014/12/24

            printf("< OCC-PA START in HEAD >\n");

            occ_pa_bcast_monitor_start = 1;

            //if (!occ_key_status)
                //set_key_led(enum_COP_KEY_OCC);

            rxlen = 0;
        }
        if (ret == -1) {
        ret = check_avc2cop_cmd(buf, rxlen, OCCPA_BCAST_STATUS_STOP, 0);
        if (ret == 0) {
            printf("< OCC-PA STOP in HEAD >\n");

            if (!occ_pa_bcast_monitor_start) {
                reset_occ_pa_multi_rx_data_ptr();
                reset_cop_pa_multi_rx_data_ptr();
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr();
            }

			//DBG_LOG("\n");
            used_len = rxlen;
            occ_pa_bcast_monitor_start = 0;
            //if (!occ_key_status)
                //clear_key_led(enum_COP_KEY_OCC);
            rxlen = 0;
        }
        }
     }

    if (tcp_connected) {
        discard_cop_pa_and_call_encoding_data(__broadcast_status);	// MIC
        discard_occ_pa_and_call_encoding_data(__broadcast_status);	// OCC
		discard_cop_voip_call_encoding_data(__broadcast_status);	// VoIP
	}

	debug_print_cob_status_change();

    switch(__broadcast_status)
    {
      case COP_STATUS_IDLE:

        if (is_waiting_normal_or_emer_call(PEI_LIST, -1, -1))
            run_force_pei_call_wakeup = 1;
        else if (is_waiting_normal_call(CAB_LIST, -1, -1))
            run_force_cab_call_wakeup = 1;

		//printf(" run_force_cab_call_wakeup: %d, run_force_pei_call_wakeup:%d\n", run_force_cab_call_wakeup, run_force_pei_call_wakeup);

        if (   run_force_pei_call_wakeup || run_force_cab_call_wakeup
           || (rxlen && buf && rxlen >= (int)buf[0]))
        {
			ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_STOP, 0);
			if(ret == 0)
			{
				printf("IDLE ==> Can't support PEI_STOP on here..\n");
				used_len = rxlen;
				break;
			}

            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (    emergency_pei2cab_start == 0
                 && (ret == 0 || run_force_pei_call_wakeup == 1))
            {
                if (ret == 0)
                    used_len = rxlen;

                if (run_force_pei_call_wakeup) {
                    __car_num = get_call_car_num(PEI_LIST);
                    __dev_num = get_call_id_num(PEI_LIST);
					__train_num = get_call_train_num(PEI_LIST);
                    pei2cab_call_pending = 1;
                }
                else {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
                }

                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

//				if(!tractor_cab2cab_start) {
                	__broadcast_status = __PEI2CAB_CALL_WAKEUP;
                	pei2cab_call_waiting = 1;

                	printf("\n< IDLE -> PEI-CALL WAKE-UP (%d-%d) >\n", __car_num, __dev_num);
//				}

                if (!run_force_pei_call_wakeup) {
                   // Ver 0.78
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;
                    add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);
                }

				if (fire_alarm_key_status)
					fire_status_reset(1);
				else
                	force_call_menu_draw();

//				if(!tractor_cab2cab_start) {
                	decoding_start();
                	start_play_beep();
                	set_cop_spk_audio_switch_on();
                	cop_speaker_volume_set();
                	occ_speaker_volume_mute_set(); // <===

                	run_force_pei_call_wakeup = 0;
//				}

                break;
            }

            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< IDLE, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
                break;
            }
#else
            if (ret == 0 || run_force_cab_call_wakeup == 1) {
                if (ret == 0)
                    used_len = rxlen;

                if (run_force_cab_call_wakeup) {
                    __car_num = get_call_car_num(CAB_LIST);
                    __dev_num = get_call_id_num(CAB_LIST);
					__train_num = get_call_train_num(CAB_LIST);
                    printf("< IDLE, Get WAITING CAB (%d-%d) >\n", __car_num, __dev_num);
                    cab2cab_call_pending = 1;
                }
                else {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
                }

				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink

                decoding_start();
                start_play_beep();
                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
                occ_speaker_volume_mute_set(); // <===

                ret = 0;
                if (!run_force_cab_call_wakeup) {
                    ret = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
                }

                call_mon_sending = 0;

                if ((run_force_cab_call_wakeup) && __car_num == (char)-1 && __dev_num == (char)-1) {
                    __broadcast_status = __WAITING_CAB2CAB_CALL_START;
                    cab2cab_call_pending = 1;
                    call_mon_sending = 1;

                    printf("< IDLE, PENDING CAB-CALL REQEUST -> WAITING CAB-START\n");
                }
                else {
                    __broadcast_status = __CAB2CAB_CALL_WAKEUP;
                    cab2cab_call_waiting = 1;
                    printf("< IDLE -> CAB CALL WAKEUP(%d-%d) >\n", __car_num, __dev_num);
                }

                force_call_menu_draw();

                if (run_force_cab_call_wakeup)
                    run_force_cab_call_wakeup = 0;

                break;
            }
#endif

            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                //used_len = rxlen;
                //printf("< IGNOR CAB MON START in IDLE ??? >\n");

#if 1 // Ver0.76, to get cab mon directly for push to talk CAB-CAB CALL and no alarm

                used_len = 0; // to run again CAB-CAB MON START command

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);

                occ_speaker_volume_mute_set(); // <===

                ret = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                call_mon_sending = 0;

                __broadcast_status = __CAB2CAB_CALL_WAKEUP;
                cab2cab_call_waiting = 1;
                printf("< IDLE -> CAB CALL WAKEUP(%d-%d) >\n", __car_num, __dev_num);

				force_call_menu_draw();
#endif
                break;
            }

            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                printf("< IGNORE PEI MON START in IDLE ??? >\n");
                break;
            }

            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                printf("< IGNOR PEI MON STOP in IDLE >\n");
                break;
            }

            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
#if 0
                used_len = rxlen;
                printf("< IGNOR CAB MON STOP in IDEL >\n");
                break;
#endif
				__car_num = get_car_number(buf, rxlen);
				__dev_num = get_device_number(buf, rxlen);
				if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
					printf("\n< ON IDLE, DEL MON WAIT-CAB %d-%d >\n", __car_num, __dev_num);
					stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (stat == 0) {
						used_len = rxlen;
						force_call_menu_draw();
					}
				}
				else if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) {
					printf("\n< ON IDLE, DEL MON CAB %d-%d >\n", __car_num, __dev_num);
					stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (stat == 0) {
						used_len = rxlen;
						play_stop_call_mon_rx_data();

						cab2cab_call_monitoring = 0;

						occ_mic_volume_mute_set();
						close_cop_multi_monitoring();
						clear_rx_tx_mon_multi_addr();
						decoding_stop();
					}
				}
				else
				{
					used_len = rxlen;
					printf("\n< ON WAITING CAB-CALL STOP, CAB ID ERROR, DEL MON CAB(%d-%d) >\n", __car_num, __dev_num);
				}
            }
        }

        if_call_selected_call_menu_draw();
        break;

/*
WAITING_PASSIVE_INDOOR_BCAST_START*/
      case WAITING_PASSIVE_INDOOR_BCAST_START:
        if (rxlen && buf && rxlen >= (int)buf[0]) {
            if (indoor_pending) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_waiting = 0;

                if (!inout_bcast_processing)
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif

                inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;

                __broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                printf("< WAITING IN -> IN START(STAT:0x%X)\n",inout_bcast_processing);

				change_to_config_menu = 1;

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19

                break;
            }
            }

            if (outdoor_pending) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_waiting = 0;

                if (!inout_bcast_processing)
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;

                __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                printf("< WAITING IN -> OUT START(STAT:0x%X)\n",inout_bcast_processing);

				change_to_config_menu = 1;

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                break;
            }
            }
        }

        if_call_selected_call_menu_draw();
        break;

/*
__PASSIVE_INDOOR_BCAST_START*/
      case __PASSIVE_INDOOR_BCAST_START:
        if (indoor_pending == 1) {
            cop_pa_mic_codec_start();
            set_cop_pa_mic_audio_switch_on();
            cop_pa_and_call_mic_volume_set();

            indoor_pending = 0;
        }

        if (inout_bcast_processing) {
            if (!remote_occ_pa_monitor_start) {
				running_cop_pa_and_call_mic_audio_sending();
            }
            else {
                __discard_cop_pa_and_call_mic_encoding_data();
                if (remote_occ_pa_monitor_flag)
                    occ_pa_monitoring_bcast_rx_dec_running(1, 1);
            }
#ifdef CAB_MONITORING_ON_PASSIVE_INDOOR
        	if (cab2cab_call_monitoring == 1) {
        	    rxbuf = try_read_call_monitoring_data(&len, 0, 1);
        	    running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0); // run
        	}
#endif
        }
        else {
            __discard_cop_pa_and_call_mic_encoding_data();
        }
        /*****************************************************/
        if(inoutdoor_pending == 1) {
            Func_Cop_Key_Process_List[enum_COP_KEY_PA_OUT](backup_keyval, tcp_connected);
            //DBG_LOG("backup_keyval = 0x%X:%d\n", backup_keyval, backup_keyval);
            backup_keyval = 0;
            inoutdoor_pending = 0;
        }
        /*****************************************************/

        if (rxlen && buf && rxlen >= (int)buf[0]) {
            ret = -1; // 2014/02/20 <<<=========

            if (outdoor_pending == 1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;
                //outdoor_pending = 0;
                outdoor_waiting = 0;
                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;

                printf("< IN START -> INOUT START(STAT:0x%X) >\n",inout_bcast_processing);
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON IN, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;
				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                printf("\n< ON IN, ADD WAIT CAB %d-%d >\n", __car_num, __dev_num);
                ret = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON IN, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1
                set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);
                printf("\n< ON IN, ADD WAIT CAB %d-%d by CAB MON >\n", __car_num, __dev_num);
                ret = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("\n< ON IN, ADD WAIT-MON CAB %d-%d >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
#ifdef CAB_MONITORING_ON_PASSIVE_INDOOR
                    if (!is_waiting_normal_call(CAB_LIST, __car_num, __dev_num)) {
                        stop_play_beep();
                        cop_speaker_volume_mute_set();
                        decoding_stop();

						set_cop_spk_audio_switch_on(); // <<===

                        decoding_start();
                        cop_speaker_volume_set();

                        connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

                        cab2cab_call_monitoring = 1;
					}
#else
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
#endif
                    /*move_wakeup_to_1th_index(CAB_LIST);*/

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                } else
                    printf("\n< ON IN, CAB ID ERROR, CAB MON START(%d-%d) >\n", __car_num, __dev_num);
                used_len = rxlen;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    printf("\n< ON IN, DEL MON WAIT-CAB %d-%d >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
#ifdef CAB_MONITORING_ON_PASSIVE_INDOOR
				else if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) {
					printf("\n< ON IN, DEL MON CAB %d-%d >\n", __car_num, __dev_num);
					stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (stat == 0) {
						used_len = rxlen;
						play_stop_call_mon_rx_data();

						cab2cab_call_monitoring = 0;

						occ_mic_volume_mute_set();
						close_cop_multi_monitoring();
						clear_rx_tx_mon_multi_addr();
						decoding_stop();
					}
				}
#endif
                else
                    printf("\n< ON IN, CAB ID ERROR, DEL MON CAB(%d-%d) >\n", __car_num, __dev_num);
            }
            }

            if (ret == -1) {
            	ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            	if (ret == 0) {
                	__car_num = get_car_number(buf, rxlen);
                	__dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
                	printf("\n< ON IN, ADD PEI %d-%d >\n", __car_num, __dev_num);

               		// Ver 0.78
                	stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                	if (stat3 == 0) is_emer_call = 1;
                	else is_emer_call = 0;
                	stat = add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

                	if (stat == 0) {
                	    used_len = rxlen;
                    	set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

						force_call_menu_draw();
                	}

                	enable_pei_wakeup_on_in_passive = 1;
            	}
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON IN, ADD WAIT-MON %s PEI %d-%d >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);

                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    /*move_wakeup_to_1th_index(PEI_LIST);*/

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                } else
                    printf("\n< ON IN, PEI ID ERROR, PEI MON START(%d-%d) >\n", __car_num, __dev_num);
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON IN, DEL %s WAIT-PEI %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                } else
                    printf("\n< ON IN, PEI ID ERROR, PEI MON STOP(%d-%d) >\n", __car_num, __dev_num);
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, OCCPA_BCAST_STATUS_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                remote_occ_pa_monitor_start = 1;
                remote_occ_pa_monitor_flag = 1;

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
                decoding_start();

                //set_key_led(enum_COP_KEY_OCC);

                printf("< ON IN, START OCC-PA MON >\n");
            }
            }
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, OCCPA_BCAST_STATUS_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                remote_occ_pa_monitor_flag = 0;

                play_stop_occ_pa_mon_rx_data(1, 0);
                reset_occ_pa_multi_rx_data_ptr();

               	decoding_stop();

                printf("< ON IN, STOP OCC-PA MON >\n");
            }
            }

            if (ret == -1) {
            if (remote_occ_pa_monitor_start && !remote_occ_pa_monitor_flag) {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;
                    remote_occ_pa_monitor_start = 0;
                    //clear_key_led(enum_COP_KEY_OCC);
                }
            }
            }
            /************************************************************************/
        }

        if_call_selected_call_menu_draw();

        break;

      case WAITING_PASSIVE_INDOOR_BCAST_STOP:
        if (rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif

                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

	        	enable_pei_wakeup_on_in_passive = 0;
                enable_in_pa_simultaneous_on_pei2cab = 0;

                if (!waiting_outstart_on_wait_in_stop) {
                    __broadcast_status = __PASSIVE_INDOOR_BCAST_STOP;
                    indoor_pending = 1;
                    printf("< WAITING IN STOP -> IN STOP >\n");
                }
                else {
                    waiting_outstart_on_wait_in_stop = 0;
                    outdoor_pending = 1;
                    outdoor_waiting = 1;

					if(!tractor_cab2cab_start) {		// mhryu 20150526
                   		cop_pa_mic_volume_mute_set();
                   		cop_pa_mic_codec_stop();
					}
                   	close_cop_multi_tx();
                    __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;
                    printf("< WAITING IN STOP -> WAITING OUT START >\n");
                }

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr();

                break;
            }

            if (waiting_outstart_on_wait_in_stop) {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;
                    waiting_outstart_on_wait_in_stop = 0;

                    outdoor_waiting = 0;
#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_OUT);
                    set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                    inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    __broadcast_status = _WAITING_INSTOP_PASSIVE_INOUTDOOR_BCAST_START;
                    printf("< WAITING IN STOP, GET OUT START  -> WAITING IN STOP ON INOUT >\n");

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19

                    break;
                }
            }
        }

        if_call_selected_call_menu_draw();

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        break;

      case __PASSIVE_INDOOR_BCAST_STOP:
        if (indoor_pending == 1) {
            indoor_pending = 0;
        }

		if(!tractor_cab2cab_start) {		// mhryu 20150526
        	cop_pa_mic_volume_mute_set();
        	cop_pa_mic_codec_stop();
		} else {
			/* Tractor input to Speaker */
			set_tractor_mic_audio_switch_on();
			set_occ_mic_to_spk_audio_switch_on();
			occ_mic_volume_set(occ_mic_vol_atten);
		}
        close_cop_multi_tx();

        if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, &__mon_addr, NULL, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                pei2cab_call_monitoring = 1;
                __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                ret = get_call_is_emergency();

                add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);
                printf("< IN STOP -> PEI MON START(%d-%d) >\n", __car_num, __dev_num);

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }

            break;
        }

        if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, &__mon_addr, &__tx_addr, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                cab2cab_call_monitoring = 1;
                __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                printf("< IN STOP -> CAB MON START(%d-%d) >\n", __car_num, __dev_num);

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }
            break;
        }

        if (outdoor_waiting == 1) {
            __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;
            printf("< IN STOP -> WAITING OUT START >\n");
            break;
        }

		if (cab2cab_call_monitoring)
		{
			__broadcast_status = __CAB2CAB_CALL_MONITORING_START;
			printf("< IN STOP -> CAB MON START continue\n");
			break;
		}

        __broadcast_status = COP_STATUS_IDLE;
        printf("< IN STOP -> IDLE >\n");

        break;

      case WAITING_PASSIVE_OUTDOOR_BCAST_START:
        if (outdoor_pending && rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;

                printf("< WAITING OUT START -> OUT START(STAT:0x%X)>\n", inout_bcast_processing);

                if (!inout_bcast_processing)
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                outdoor_waiting = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;

				change_to_config_menu = 1;

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
        }

        if (indoor_pending && rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;

                printf("<  WAITING OUT START -> IN START(STAT:0x%X)>\n", inout_bcast_processing);

                if (!inout_bcast_processing)
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                indoor_waiting = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif

                __broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;

				change_to_config_menu = 1;

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
        }

        if_call_selected_call_menu_draw();

        break;

      case __PASSIVE_OUTDOOR_BCAST_START:
        if (outdoor_pending == 1) {
//printf("__cop_bcast_status_update() _PASSIVE_OUTDOOR_BCAST_START\n");
            //ret = get_status_key_blink(enum_COP_KEY_PA_OUT);
            //if (ret == 0) {
                cop_pa_mic_codec_start();
                set_cop_pa_mic_audio_switch_on();
                cop_pa_and_call_mic_volume_set();

                outdoor_pending = 0;
                //set_key_led(enum_COP_KEY_PA_OUT);
                //printf("[ ON OUT, LED OUT ON ]\n");
            //}
        }

/*
        if (indoor_stop_waiting == 1) {
            ret = get_status_key_blink(enum_COP_KEY_PA_IN);
            if (ret == 0) {
                indoor_stop_waiting = 0;
                clear_key_led(enum_COP_KEY_PA_IN);
                printf("[ ON OUT, LED IN OFF ]\n");
            }
        }
*/

        //if (!outdoor_pending)
        {
            if (inout_bcast_processing) {
				running_cop_pa_and_call_mic_audio_sending();
#ifdef CAB_MONITORING_ON_PASSIVE_OUTDOOR
				if (cab2cab_call_monitoring == 1) {
					rxbuf = try_read_call_monitoring_data(&len, 0, 1);
					running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0); // run
				}
#endif
			}

            if (rxlen && buf && rxlen >= (int)buf[0]) {

                ret = -1; // 2014/02/20 

                if (indoor_pending == 1) {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_IN);
                    set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif

                    __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;
                    //indoor_pending = 0;
                    indoor_waiting = 0;
                    inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;

                    printf("< OUT START -> INOUT START(STAT:0x%X) >\n",inout_bcast_processing);
                }
                }

                if (ret == -1) {
                ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
                if (ret == 0) {
                    printf("< ON OUT, NOT USE CAB WAKEUP >\n");
                    used_len = rxlen;
                }
#else
                if (ret == 0) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
                    printf("\n< ON OUT, ADD WAKEUP-CAB %d-%d >\n", __car_num, __dev_num);
                    stat = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
                    if (stat == 0) {
                        used_len = rxlen;
						set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        				set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
                        force_call_menu_draw();
                    }
                }
#endif
                }

                if (ret == -1) {
                ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
                if (ret == 0) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
                    if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                        printf("< ON OUT, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                        ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                        if (ret == 0) {
                            used_len = rxlen;

                            cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                            force_call_menu_draw();
                        }
                    }
                }
                }

                if (ret == -1) {
                ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
                if (ret == 0) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
#if 1
                    set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);
                    printf("\n< ON OUT, ADD WAKEUP-CAB %d-%d by CAB MON >\n", __car_num, __dev_num);
                    stat = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                    if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                        used_len = rxlen;
                        __mon_addr = get_mon_mult_addr(buf, rxlen);
                        __tx_addr = get_tx_mult_addr(buf, rxlen);
						__train_num  = get_train_number(buf, rxlen);

                        printf("\n< ON OUT, ADD WAIT-MON CAB %d-%d >\n", __car_num, __dev_num);
                        add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
#ifdef CAB_MONITORING_ON_PASSIVE_OUTDOOR
						if (!is_waiting_normal_call(CAB_LIST, __car_num, __dev_num)) {
							stop_play_beep();
							cop_speaker_volume_mute_set();
							decoding_stop();

							set_cop_spk_audio_switch_on(); // <<===

							decoding_start();
							cop_speaker_volume_set();

							connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

							cab2cab_call_monitoring = 1;
						}
#else
                        add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
#endif
                        /*move_wakeup_to_1th_index(CAB_LIST);*/

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();
                    }
                }
                }

                if (ret == -1) {
                ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
                if (ret == 0) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
                    if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                        printf("< ON OUT, DEL CAB WAIT-MON %d-%d >\n", __car_num, __dev_num);
                        stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);

                        if (stat == 0) {
                            used_len = rxlen;
                            force_call_menu_draw();
                        }
                    }
#ifdef CAB_MONITORING_ON_PASSIVE_OUTDOOR
					else if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) {
						printf("\n< ON OUT, DEL MON CAB %d-%d >\n", __car_num, __dev_num);
						stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
						if (stat == 0) {
							used_len = rxlen;
							play_stop_call_mon_rx_data();

							cab2cab_call_monitoring = 0;

							occ_mic_volume_mute_set();
							close_cop_multi_monitoring();
							clear_rx_tx_mon_multi_addr();
							decoding_stop();
						}
					}
#endif
                }
                }

                if (ret == -1) {
                ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
                if (ret == 0) {
                    used_len = rxlen;
                    set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON OUT, ADD %s PEI %d-%d >\n", is_emer_call ? "EMER": "NORMAL", __car_num, __dev_num);

                    stat = add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

					force_call_menu_draw();

                    enable_pei_wakeup_on_out_passive = 1;
                }
                }

                if (ret == -1) {
                ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
                if (ret == 0) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
                    /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                    if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                    {
                        used_len = rxlen;
                        __mon_addr = get_mon_mult_addr(buf, rxlen);

                        stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                        if (stat3 == 0) is_emer_call = 1;
                        else is_emer_call = 0;

                        printf("\n< ON OUT, ADD WAIT-MON %s PEI %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                        add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                        add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                        /*move_wakeup_to_1th_index(PEI_LIST);*/

                        pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                        force_call_menu_draw();
                    }
                }
                }

                if (ret == -1) {
                ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
                if (ret == 0) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
                    if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                        stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                        if (stat3 == 0) is_emer_call = 1;
                        else is_emer_call = 0;

                        printf("< ON OUT, DEL %s PEI WAIT-MON %d-%d >\n",
                                    is_emer_call ? "EMER": "NORMAL", __car_num, __dev_num);
                        stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);
                        if (stat == 0) {
                            used_len = rxlen;
                            force_call_menu_draw();
                        }
                    }
                }
                }
            }
        }

        /*****************************************************/
        if(inoutdoor_pending == 1) {
            Func_Cop_Key_Process_List[enum_COP_KEY_PA_OUT](backup_keyval, tcp_connected);
            //DBG_LOG("backup_keyval = 0x%X:%d\n", backup_keyval, backup_keyval);
            backup_keyval = 0;
            inoutdoor_pending = 0;
        }
        /*****************************************************/

        if_call_selected_call_menu_draw();

        break;

      case WAITING_PASSIVE_OUTDOOR_BCAST_STOP:
        if (rxlen && buf && rxlen >= (int)buf[0])
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                __broadcast_status = __PASSIVE_OUTDOOR_BCAST_STOP;
                outdoor_pending = 0;
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;

	        	enable_pei_wakeup_on_out_passive = 0;
                enable_out_pa_simultaneous_on_pei2cab = 0;

                printf("< WAITING OUT STOP -> OUT STOP(STAT:0x%X) >\n", inout_bcast_processing);

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr();

                break;
            }
        }

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        if_call_selected_call_menu_draw();

        break;

      case __PASSIVE_OUTDOOR_BCAST_STOP:
        if (outdoor_pending == 1) {
            outdoor_pending = 0;
        }

        close_cop_multi_tx();
		if(!tractor_cab2cab_start) {		// mhryu 20150526
       		cop_pa_mic_volume_mute_set();
        	cop_pa_mic_codec_stop();
		} else {
			/* Tractor input to Speaker */
			set_tractor_mic_audio_switch_on();
			set_occ_mic_to_spk_audio_switch_on();
			occ_mic_volume_set(occ_mic_vol_atten);
		}

        if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, &__mon_addr, &__tx_addr, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                pei2cab_call_monitoring = 1;
                __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                ret = get_call_is_emergency();
                add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                printf("< OUT STOP -> PEI MON START(%d-%d) >\n", __car_num, __dev_num);

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }
            break;
        }

        if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, &__mon_addr, &__tx_addr, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                cab2cab_call_monitoring = 1;
                __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                printf("< OUT STOP -> CAB MON START(%d-%d) >\n", __car_num, __dev_num);

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }
            break;
        }

        if (indoor_waiting == 1) {
            __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;
            printf("< OUT STOP -> WAITING IN START >\n");
            break;
        }

		if (cab2cab_call_monitoring)
		{
			__broadcast_status = __CAB2CAB_CALL_MONITORING_START;
			printf("< OUT STOP -> CAB MON START continue\n");
			break;
		}

        __broadcast_status = COP_STATUS_IDLE;
        printf("< OUT STOP -> IDLE >\n");

        break;

      case WAITING_OUTSTART_PASSIVE_INOUTDOOR_BCAST_START:
        if (rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;
                outdoor_pending = 1;
                outdoor_waiting = 0;
                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;

                printf("< ON IN, WAITING OUT START -> INOUT START(STAT:0x%X) >\n",inout_bcast_processing);
            }
        }

        if_call_selected_call_menu_draw();

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        break;

      case WAITING_INSTART_PASSIVE_INOUTDOOR_BCAST_START:
        if (rxlen && buf && rxlen >= (int)buf[0])
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;
                indoor_waiting = 0;

                if (!waiting_outstop_on_wait_in_start) {
                    __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;
                    indoor_pending = 1;

                    printf("< WAITING IN START for INOUT -> INOUT START(STAT:0x%X)\n", inout_bcast_processing);
                } else {
                    waiting_outstop_on_wait_in_start = 0;
#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                    __broadcast_status = _WAITING_OUTSTOP_PASSIVE_INOUTDOOR_BCAST_START;
                    printf("< WAITING IN START for INOUT -> WAIT OUT STOP on INOUT >\n");
                }
                break;
            }

            if (waiting_outstop_on_wait_in_start) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                waiting_outstop_on_wait_in_start = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                close_cop_multi_tx();
				if(!tractor_cab2cab_start) {		// mhryu 20150526
                	cop_pa_mic_volume_mute_set();
                	cop_pa_mic_codec_stop();
				}

                indoor_pending = 1;
                indoor_waiting = 1;
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;
                __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;

	        enable_pei_wakeup_on_out_passive = 0;
                enable_out_pa_simultaneous_on_pei2cab = 0;

                printf("< WAITING IN for INOUT, GET OUT STOP -> WAITING IN START(STAT:0x%X)\n", inout_bcast_processing);

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr();
                break;
            }
            }
        }

        if_call_selected_call_menu_draw();

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        break;

      case _WAITING_OUTSTOP_PASSIVE_INOUTDOOR_BCAST_START:
        if (rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;

                if (   !indoor_stop_pending
                    && inout_bcast_processing == INOUT_BUTTON_STATUS_IN_ON_MASK
                   )
                {
                    outdoor_stop_waiting = 1;
                    __broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                    printf("< ON INTOUT, WAITING OUT-STOP -> IN-START >\n");
                }
                else if (   indoor_stop_pending
                         && inout_bcast_processing == INOUT_BUTTON_STATUS_IN_ON_MASK
                        )
                {
                    printf("< ON INTOUT, WAITING OUT-STOP, GET OUT-STOP >\n");
                }
                else if (!inout_bcast_processing) {
                    __broadcast_status = __PASSIVE_OUTDOOR_BCAST_STOP;
	            enable_pei_wakeup_on_in_passive = 0;
                    enable_in_pa_simultaneous_on_pei2cab = 0;

	            enable_pei_wakeup_on_out_passive = 0;
                    enable_out_pa_simultaneous_on_pei2cab = 0;

                    printf("< ON INTOUT, WAITING OUT-STOP -> OUT STOP >\n");
                }
                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr();
            }

            if (indoor_stop_pending && ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_stop_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

                if (!inout_bcast_processing) {
                    __broadcast_status = __PASSIVE_OUTDOOR_BCAST_STOP;
	            enable_pei_wakeup_on_in_passive = 0;
                    enable_in_pa_simultaneous_on_pei2cab = 0;

	            enable_pei_wakeup_on_out_passive = 0;
                    enable_out_pa_simultaneous_on_pei2cab = 0;
                    printf("< ON INTOUT, WAITING OUT-STOP -> OUT STOP >\n");
                }
                else
                    printf("< ON INOUT, WAITING OUT-STOP, GET IN-STOP(STAT:0x%X) >\n", inout_bcast_processing);

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr();
            }
            }
        }
        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}

        if_call_selected_call_menu_draw();
        break;

      case _WAITING_INSTOP_PASSIVE_INOUTDOOR_BCAST_START:
        if (rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif

                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

                if (    !outdoor_stop_pending
                     && inout_bcast_processing == INOUT_BUTTON_STATUS_OUT_ON_MASK
                   )
                {
                    indoor_stop_waiting = 1;
                    __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                    printf("< ON INOUT, WAITING IN-STOP -> OUT-START >\n");

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
                else if (   outdoor_stop_pending
                         && inout_bcast_processing == INOUT_BUTTON_STATUS_OUT_ON_MASK
                   )
                {
                    printf("< ON INOUT, WAITING IN-STOP, GOT IN-STOP >\n");
                }
                else if (!inout_bcast_processing) {
                    __broadcast_status = __PASSIVE_INDOOR_BCAST_STOP;

	            enable_pei_wakeup_on_in_passive = 0;
                    enable_in_pa_simultaneous_on_pei2cab = 0;
	            enable_pei_wakeup_on_out_passive = 0;
                    enable_out_pa_simultaneous_on_pei2cab = 0;

                    printf("< ON INTOUT, WAITING IN-STOP -> IN STOP >\n");
                }
                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }

            if (outdoor_stop_pending && ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_stop_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;

                if (!inout_bcast_processing) {
                    __broadcast_status = __PASSIVE_INDOOR_BCAST_STOP;

	            enable_pei_wakeup_on_in_passive = 0;
                    enable_in_pa_simultaneous_on_pei2cab = 0;
	            enable_pei_wakeup_on_out_passive = 0;
                    enable_out_pa_simultaneous_on_pei2cab = 0;

                    printf("< ON INOUT, WAITING IN-STOP -> IN STOP >\n");
                }
                else
                    printf("< ON INOUT, WAITING IN-STOP, GET OUT-STOP >\n");
                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
        }
        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        if_call_selected_call_menu_draw();
        break;

/*
__PASSIVE_INOUTDOOR_BCAST_START */
      case __PASSIVE_INOUTDOOR_BCAST_START:
        //if (indoor_pending == 1 && outdoor_pending == 1) {
        if (indoor_pending == 1 || outdoor_pending == 1) {
            //ret = get_status_key_blink(enum_COP_KEY_PA_IN);
            //ret |= get_status_key_blink(enum_COP_KEY_PA_OUT);
            //if (ret == 0) {
                //set_key_led(enum_COP_KEY_PA_IN);
                //set_key_led(enum_COP_KEY_PA_OUT);

                cop_pa_mic_codec_start();
                indoor_pending = 0;
                outdoor_pending = 0;
                set_cop_pa_mic_audio_switch_on();
                cop_pa_and_call_mic_volume_set();
                //printf("[ ON IN, LED IN/OUT ON ]\n");
            //}
        }
/*
        else if (indoor_pending == 1 && outdoor_pending == 0) {
            //ret = get_status_key_blink(enum_COP_KEY_PA_IN);
            //if (ret == 0) {
                indoor_pending = 0;
                //set_key_led(enum_COP_KEY_PA_IN);
                //printf("[ ON INOUT, LED IN ON ]\n");
            //}
        }
        else if (indoor_pending == 0 && outdoor_pending == 1) {
            //ret = get_status_key_blink(enum_COP_KEY_PA_OUT);
            //if (ret == 0) {
                outdoor_pending = 0;
                //set_key_led(enum_COP_KEY_PA_OUT);
                //printf("[ ON INOUT, LED OUT ON ]\n");
            //}
        }
*/

#if defined(CAB_MONITORING_ON_PASSIVE_INDOOR) || defined (CAB_MONITORING_ON_PASSIVE_OUTDOOR)
		if (cab2cab_call_monitoring == 1) {
			rxbuf = try_read_call_monitoring_data(&len, 0, 1);
			running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0); // run
		}
#endif


        if (rxlen && buf && rxlen >= (int)buf[0]) {
            ret = -1; // 2014/02/20

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON INOUT, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;
				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                printf("\n< ON INOUT, ADD CAB %d-%d >\n", __car_num, __dev_num);
                stat = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON INOUT, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;
                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1
                set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);
                printf("\n< ON INOUT, ADD CAB %d-%d by CAB MON >\n", __car_num, __dev_num);
                stat = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);

                    printf("\n< ON INOUT, ADD WAIT-MON CAB(%d-%d) >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
#if defined(CAB_MONITORING_ON_PASSIVE_INDOOR) || defined (CAB_MONITORING_ON_PASSIVE_OUTDOOR)
					if (!is_waiting_normal_call(CAB_LIST, __car_num, __dev_num)) {
						stop_play_beep();
						cop_speaker_volume_mute_set();
						decoding_stop();

						set_cop_spk_audio_switch_on(); // <<===

						decoding_start();
						cop_speaker_volume_set();

						connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

						cab2cab_call_monitoring = 1;
					}
#else
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
#endif

                    /*move_wakeup_to_1th_index(CAB_LIST);*/

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    printf("< ON INOUT, DEL CAB WAIT-MON %d-%d >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);

                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
#if defined(CAB_MONITORING_ON_PASSIVE_INDOOR) || defined (CAB_MONITORING_ON_PASSIVE_OUTDOOR)
				else if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) { 
					printf("\n< ON INOUT, DEL MON CAB %d-%d >\n", __car_num, __dev_num);
					stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (stat == 0) {
						used_len = rxlen;
						play_stop_call_mon_rx_data();

						cab2cab_call_monitoring = 0;

						occ_mic_volume_mute_set();
						close_cop_multi_monitoring();
						clear_rx_tx_mon_multi_addr();
						decoding_stop();
					}     
				}     
#endif
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;
                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("\n< ON INOUT, ADD %s PEI %d-%d >\n", is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

				force_call_menu_draw();

                enable_pei_wakeup_on_in_passive = 1;
                enable_pei_wakeup_on_out_passive = 1;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON INOUT, ADD WAIT-MON %s PEI %d-%d >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    /*move_wakeup_to_1th_index(PEI_LIST);*/

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("< ON INOUT, DEL %s PEI WAIT-MON %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);

                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

#if 0 // delete, ver0.78
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    printf("< ON INOUT, DEL EMER-PEI WAIT-MON %d-%d >\n", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, 1, CALL_STATUS_MON_WAIT, __car_num, __dev_num);

                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }
#endif

            if (ret == -1 && (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                indoor_stop_waiting = 1;
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

                printf("< ON INOUT, GET IN-STOP -> OUT-START(STAT:0x%X) >\n", inout_bcast_processing);

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }

            if (ret == -1 && (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                __broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                outdoor_stop_waiting = 1;
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;

                printf("< ON INOUT, GET OUT-STOP -> IN-START(STAT:0x%X) >\n", inout_bcast_processing);
                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
        }

        if_call_selected_call_menu_draw();

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        break;

      case PASSIVE_INOUTDOOR_BCAST_STOP:
        if ((indoor_pending == 1) && (outdoor_pending == 1)) {
            indoor_pending = 0;
            outdoor_pending = 0;
            __broadcast_status = COP_STATUS_IDLE;
        }
        break;

/*
__PEI2CAB_CALL_WAKEUP*/
      case __PEI2CAB_CALL_WAKEUP:
        if (pei2cab_call_waiting == 1 && rxlen && buf && rxlen >= (int)buf[0])
        {
#if 1 // Ver 0.78
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else {
                    is_emer_call = 0;
                    printf("\n< ON PEI WAKEUP, CANNOT SUPPORT NORMAL PEI-CALL STOP(%d-%d) >\n",
                                __car_num, __dev_num);
                    break;
                }

                if (is_waiting_emergency_pei_call_any()) {
                    stat = del_call_list(PEI_LIST, 1, 0, __car_num, __dev_num);
                    if (!stat) {
                        used_len = rxlen;

                        cop_speaker_volume_mute_set();
                        stop_play_beep();
                        decoding_stop();

                        printf("\n< ON PEI WAKEUP, DEL WAIT EMER PEI(%d-%d) >\n", __car_num, __dev_num);

                        pei_led_off_if_no_wait_normal_pei_and_emer_pei_call(); // enum_COP_KEY_CALL_PEI

                        force_call_menu_draw();

                        __broadcast_status = COP_STATUS_IDLE;
                    }
                }
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
#endif
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                if (is_waiting_normal_or_emer_call(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    if (is_monitoring_call(PEI_LIST, __car_num, __dev_num, 0))
                    {
                        cop_speaker_volume_mute_set();
                        stop_play_beep();
                        occ_mic_volume_mute_set();
                        decoding_stop();

                        decoding_start();
                        set_cop_spk_audio_switch_on();
                        cop_speaker_volume_set();

                        connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);
						pei2cab_monitoring_on = 1;

                        pei2cab_call_waiting = 0;

                        printf("< PEI WAKEUP -> %s PEI-CAB MON >\n", is_emer_call ? "EMER" : "NORMAL");
                        pei2cab_call_monitoring = 1;
                        __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

#ifdef PEI_MONITORING_SAVE
                        file_close_mon_save(save_file_fd);
                        save_file_fd = -1;
                        sync();
#endif
                    }
                    else {
                        add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                        /*move_wakeup_to_1th_index(PEI_LIST);*/
                        __broadcast_status = COP_STATUS_IDLE;
                    }

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                }
            }
            }

            /* 2013/11/21 */
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON PEI WAKEUP, GET PEI MON STOP, DEL %s PEI-WAKEUP(%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;

                        cop_speaker_volume_mute_set();
                        stop_play_beep();
                        occ_mic_volume_mute_set();
                        decoding_stop();

                        force_call_menu_draw();

                        pei2cab_call_waiting = 0;

                        __broadcast_status = COP_STATUS_IDLE;

                        //cancel_auto_func_monitoring(); // 2014/02/19
                        // 2014/02/19
                        reset_fire_multi_rx_data_ptr();
                        reset_func_multi_rx_data_ptr();
                        reset_auto_multi_rx_data_ptr();
                    }

                    pei_led_off_if_no_wait_normal_pei_and_emer_pei_call(); // Ver 0.80, 2014/09/03
                }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1
                printf("< ON PEI WAKEUP, ADD CAB WAKEUP(%d-%d) by CAB MON >\n", __car_num, __dev_num);
                ret = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
                set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);
                /*move_wakeup_to_1th_index(CAB_LIST);*/
                call_mon_sending = 0;
                cab2cab_call_waiting = 1;
#endif
                if (is_waiting_normal_call(CAB_LIST, __car_num, __dev_num)) { // *
                    cop_speaker_volume_mute_set();
                    stop_play_beep();
                    occ_mic_volume_mute_set();
                    decoding_stop();

                    decoding_start();
                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();//DEFAULT_COP_SPK_VOLUME);

                    cab2cab_call_waiting = 0;

                    ret = connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

                    __tx_addr = get_tx_mult_addr(buf, rxlen);

                    printf("< ON PEI WAKEUP -> CAB MON(%d-%d) >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    cab2cab_call_monitoring = 1;
                    __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                    force_call_menu_draw();
                } else {
                    printf("\n< ON PEI WAKEUP, NO CAB WAIT LIST(%d-%d) >\n", __car_num, __dev_num);
                }
                used_len = rxlen;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;
                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("< ON PEI WAKEUP, ADD %s PEI(%d-%d) >\n", is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

                /*move_wakeup_to_1th_index(PEI_LIST);*/

                force_call_menu_draw();
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON PEI WAKEUP, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                printf("< ON PEI WAKEUP, ADD CAB WAKEUP(%d-%d) >\n", __car_num, __dev_num);
                ret = add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink

                /*move_wakeup_to_1th_index(CAB_LIST);*/

                call_mon_sending = 0;
                cab2cab_call_waiting = 1;

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON PEI WAKEUP, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();
                    }
                }
            }
            }
        }
        else if (pei2cab_call_waiting == 1 && !rxlen)
	    	running_play_beep();

        if_call_selected_call_menu_draw();

      /** __PEI2CAB_CALL_WAKEUP ########################################## */
        break;

      case WAITING_OCC_PA_BCAST_START:
        if (occ_pa_pending && rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, OCC_PA_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;

                if (enable_occ_pa_on_in_passive || enable_occ_pa_on_out_passive) {
                    close_cop_multi_tx();
                    cop_pa_mic_volume_mute_set();
                    cop_pa_mic_codec_stop();
                }

				if(tractor_cab2cab_start) {
            		printf("< TRACTOR MODE DISABLE by OCC >\n");
			        tractor_cab2cab_start = 0;
			        tractor_clear_for_key();
			  
				    send_cop2avc_tractor_stop_flag(__broadcast_status, 0x00);
				}

                cop_occ_pa_and_occ_call_codec_start();

                connect_occ_passive_bcast_tx();
#if 0
                cop_speaker_volume_mute_set(); // 2013/11/12
                occ_mic_volume_mute_set();

                set_occ_mic_audio_switch_off(); // on -> off, 2014/12/20
            	occ_mic_volume_set(occ_mic_vol_atten);
#else
				set_occ_mic_audio_switch_on();
				occ_mic_volume_set(occ_mic_vol_atten);
				set_cop_spk_audio_switch_on();
				cop_speaker_volume_set();
#endif
                //set_key_led(enum_COP_KEY_OCC);
                //set_key_led_blink(enum_COP_KEY_OCC, 0);

                occ_pa_en_status_old = 0;

                __broadcast_status = OCC_PA_BCAST_START;
                now_occ_pa_start_wait = 0;;
                now_occ_pa_start = 1;

	        	occ_pa_running = 1;
                printf("< WAITING OCC PA -> OCC-PA START >\n");
                break;
            }

            if (indoor_pending && indoor_waiting) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_waiting = 0;

                enable_occ_pa_on_in_passive = 1;
                //if (!inout_bcast_processing)
                //    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                //set_key_led_blink(enum_COP_KEY_PA_IN, 0);

                inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;

                //__broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                printf("< WAITING OCC PA, GET IN START >\n");

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                break;
            }
            }

            if (outdoor_pending && outdoor_waiting) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_waiting = 0;

                enable_occ_pa_on_out_passive = 1;

                //if (!inout_bcast_processing)
                //    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                //set_key_led_blink(enum_COP_KEY_PA_OUT, 0);

                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;

                //__broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                printf("< WAITING OCC PA, GET OUT START >\n");

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                break;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;
                pei2cab_call_waiting = 1;

                /*
                decoding_start();
                start_play_beep();
                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
                */

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

                printf("\n< ON WAITING OCC-PA, ADD %s PEI-CALL WAKE-UP (%d-%d) >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

				force_call_menu_draw();
            }
            }

#if 0 // delete, ver0.78
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;
                pei2cab_call_waiting = 1;

                /*
                decoding_start();
                start_play_beep();
                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
                */

                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                printf("\n< ON WAITING OCC-PA, ADD EMER PEI(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, 1, __car_num, __dev_num, 0, 0);

                force_call_menu_draw();
            }
            }
#endif

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON WAITING OCC-PA, ADD WAIT-MON %s PEI %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    /*move_wakeup_to_1th_index(PEI_LIST);*/

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON WAITING OCC-PA, DEL %s WAIT-PEI %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON WAITING OCC-PA, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;

				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                printf("\n< ON WAITING OCC-PA, ADD CAB WAKE-UP (%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();
            }
#endif
            }

			// Added by Michael RYU(June 29, 2015)
			if (ret == -1) {
				//DBG_LOG("\n");
				ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_STOP, 0);
				if (ret == 0) {
					//set_key_led(enum_COP_KEY_CALL_CAB);             // Added by Michael RYU(June. 29, 2015)
					//set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);    // Changed value from '1' to '0' to disable the blink
					__car_num = get_car_number(buf, rxlen);
					__dev_num = get_device_number(buf, rxlen);

					printf("< ON WAITING OCC-PA, GOT CAB2CAB STOP(%d-%d) >\n", __car_num, __dev_num);
					ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (ret == 0) {
						used_len = rxlen;
                    	play_stop_call_rx_data();

                    	close_cop_multi_rx_sock();
                    	close_call_and_occ_pa_multi_tx();
                    	if (call_mon_sending)
                        	close_cop_multi_monitoring();

                    	cab2cab_call_running = 0;

                    	cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    	reset_func_multi_rx_data_ptr();
                   		reset_fire_multi_rx_data_ptr(); //
                 		reset_auto_multi_rx_data_ptr(); //

                		force_call_menu_draw();
					}
				}
			}

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON WAITING OCC-PA, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1
                set_key_led_blink (enum_COP_KEY_CALL_CAB, 1);
                printf("\n< ON WAITING OCC-PA, ADD CAB WAKE-UP (%d-%d) by CAB MON >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("\n< ON WAITING OCC-PA, ADD WAIT-MON CAB %d-%d >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    /*move_wakeup_to_1th_index(CAB_LIST);*/

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                } else {
                    printf("\n< ON WAITING OCC-PA, NO CAB WAIT LIST(%d-%d) >\n", __car_num, __dev_num);
                }
                used_len = rxlen;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    printf("\n< ON WAITING OCC-PA, DEL MON WAIT-PEI %d-%d >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }
        }

        if (   inout_bcast_processing
            && (enable_occ_pa_on_in_passive || enable_occ_pa_on_out_passive)
           ) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        if_call_selected_call_menu_draw();

      /*** WAITING_OCC_PA_BCAST_START ##########################################*/
        break;

      case OCC_PA_BCAST_START:
        if (occ_pa_pending) {
	        occ_pa_pending = 0;
        }

        if (rxlen && buf && rxlen >= (int)buf[0]) {
            if (ret == -1) {
            	ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            	if (ret == 0) {
            	    used_len = rxlen;
                	pei2cab_call_waiting = 0; // no alarm if add pei wakeup

	                /*
    	            decoding_start();
        	        start_play_beep();
            	    set_cop_spk_audio_switch_on();
                	cop_speaker_volume_set();
	                */
	
    	            __car_num = get_car_number(buf, rxlen);
        	        __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);

            	    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                	if (stat3 == 0) is_emer_call = 1;
	                else is_emer_call = 0;

    	            printf("\n< ON OCC-PA, ADD %s PEI-CALL WAKE-UP (%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
        	        add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

            	    set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

					force_call_menu_draw();
	            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON OCC-PA, ADD WAIT-MON %s PEI %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    /*move_wakeup_to_1th_index(PEI_LIST);*/

                    pei_led_off_if_no_wait_normal_pei_and_emer_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON OCC-PA, DEL %s WAIT-PEI %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON OCC-PA, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;

				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                printf("\n< ON OCC-PA, ADD CAB WAKE-UP (%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON OCC-PA, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1
                set_key_led_blink (enum_COP_KEY_CALL_CAB, 1);
                printf("\n< ON OCC-PA, ADD CAB WAKE-UP (%d-%d) by CAB MON >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("\n< ON OCC-PA, ADD WAIT-MON CAB %d-%d >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
    
                    /*move_wakeup_to_1th_index(CAB_LIST);*/
    
                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                } else {
                    printf("\n< ON OCC-PA, NO CAB WAIT LIST(%d-%d) >\n", __car_num, __dev_num);
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    printf("\n< ON OCC-PA, DEL MON WAIT-PEI %d-%d >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            	ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            	if (ret == 0) {
                	used_len = rxlen;
                	indoor_waiting = 0;
                	indoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                	clear_key_led(enum_COP_KEY_PA_IN);
                	set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                	inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

                	if (!inout_bcast_processing) {
						if(!tractor_cab2cab_start) {		// mhryu 20150526
                    		cop_pa_mic_volume_mute_set();
                    		cop_pa_mic_codec_stop();
						}
                	    close_cop_multi_tx();
                	}

                	printf("< WAITING PEI-CALL, GET IN STOP(STAT:0x%X)\n",inout_bcast_processing);
                	// 2014/02/12
                	reset_func_multi_rx_data_ptr();
                	reset_fire_multi_rx_data_ptr(); //
                	reset_auto_multi_rx_data_ptr(); //
				}
			}

            if (ret == -1) {
            	ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            	if (ret == 0) {
                	used_len = rxlen;
                	outdoor_waiting = 0;
                	outdoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                	clear_key_led(enum_COP_KEY_PA_OUT);
                	set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                	inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;

                	if (!inout_bcast_processing) {
						if(!tractor_cab2cab_start) {		// mhryu 20150526
                    		cop_pa_mic_volume_mute_set();
                    		cop_pa_mic_codec_stop();
						}
                    	close_cop_multi_tx();
                	}

                	printf("< WAITING PEI-CALL, GET OUT STOP(STAT:0x%X)\n",inout_bcast_processing);
                	// 2014/02/12
                	reset_func_multi_rx_data_ptr();
                	reset_fire_multi_rx_data_ptr(); //
                	reset_auto_multi_rx_data_ptr(); //
				}
			}

            if (ret == -1) {
            	ret = check_avc2cop_cmd(buf, rxlen, PASSIVE_INOUTDOOR_BCAST_STOP, 0);
            	if (ret == 0) {
                	used_len = rxlen;
            		indoor_pending = 0;
            		outdoor_pending = 0;
                	indoor_waiting = 0;
                	outdoor_waiting = 0;
				}
			}
        }

        if (get_call_waiting_ctr(PEI_LIST) || get_call_waiting_ctr(CAB_LIST))
            if_call_selected_call_menu_draw();

#ifdef OCC_PA_PEI_REVERSE
		if(occ_pa_en_get_level())
			occ_pa_en_status = 0;
		else
			occ_pa_en_status = 1;
#else
#if 0
        occ_pa_en_status = occ_pa_en_get_level();
#else
		if(occ_pa_en_get_level())		/* BREAK signal is high on here */
		{
			occ_pa_enable_level = 1;
			//DBG_LOG("occ_pa_enable_level = 1\n");
		} else {							/* BREAK signal is low on here */
			occ_pa_enable_level = 0;
			//DBG_LOG("occ_pa_enable_level = 0\n");
		}
#endif
#endif
        occ_pa_en_status = occ_pa_en_get_level();
        if (occ_pa_en_status_old != occ_pa_en_status) {
			DBG_LOG("occ_pa_en_status_old(%d), occ_pa_en_status(%d)\n", occ_pa_en_status_old, occ_pa_en_status);
			occ_pa_en_status_old = occ_pa_en_status;
			if(occ_pa_en_status) {
				set_occ_mic_audio_switch_on();
				occ_mic_volume_set(occ_mic_vol_atten);
			} else {
				set_occ_mic_audio_switch_off();
				occ_mic_volume_mute_set();
			}
		}

		// Chnaged by Michael RYU (For activating) */
	    if (occ_pa_running && occ_pa_enable_level) {
			//DBG_LOG("running_occ_pa_and_occ_call_audio_sending(1, 1, 0)\n");
            running_occ_pa_and_occ_call_audio_sending(1, 1, 0);
			occ_pa_monitoring_bcast_rx_dec_running(1,1);
        } else {
			//DBG_LOG("__discard_occ_pa_and_call_mic_encoding_data(), occ_pa_running(%d), occ_pa_en_ext_is_active()=%d\n", occ_pa_running, occ_pa_en_ext_is_active());
            __discard_occ_pa_and_call_mic_encoding_data();
		}

        if (pei2cab_call_waiting == 1)
	        running_play_beep();

        if (inout_bcast_processing)
            __discard_cop_pa_and_call_mic_encoding_data();

        /***  OCC_PA_BCAST_START #######################################################*/
        break;

      case WAITING_OCC_PA_BCAST_STOP:
        if (occ_pa_pending) {
            ret = check_avc2cop_cmd(buf, rxlen, OCC_PA_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                occ_mic_volume_mute_set();
                cop_occ_pa_and_occ_call_codec_stop();
                set_occ_mic_audio_switch_off();
                close_call_and_occ_pa_multi_tx();
	        	occ_pa_running = 0;

                //clear_key_led(enum_COP_KEY_OCC);
                //set_key_led_blink(enum_COP_KEY_OCC, 0);

                __broadcast_status = OCC_PA_BCAST_STOP;
                now_occ_pa_stop = 1;
                now_occ_pa_stop_wait = 0;

                self_occ_pa_bcast_monitor_start = 0;

                printf("< WAITING OCC-PA STOP -> OCC-PA STOP >\n");

                // 2014/02/13
                reset_cop_pa_multi_rx_data_ptr();
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
        }

	    if (occ_pa_running) {
            running_occ_pa_and_occ_call_audio_sending(1, 1, 0);
        }

        if (pei2cab_call_waiting == 1)
			running_play_beep();

        if_call_selected_call_menu_draw();

        if (inout_bcast_processing)
            __discard_cop_pa_and_call_mic_encoding_data();
      /*** WAITING_OCC_PA_BCAST_STOP ##################################### */
        break;

      case OCC_PA_BCAST_STOP:
        if (inout_bcast_processing)
            __discard_cop_pa_and_call_mic_encoding_data();

        occ_pa_ack_set_low();

        if (get_call_waiting_ctr(PEI_LIST) || get_call_waiting_ctr(CAB_LIST))
            if_call_selected_call_menu_draw();

        if (enable_occ_pa_on_in_passive || enable_occ_pa_on_out_passive) {
            if (enable_occ_pa_on_in_passive && !enable_occ_pa_on_out_passive) {

                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
                if (ret != 0)
                    break;

                used_len = rxlen;
                occ_pa_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_IN);
                //set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                printf("\n< OCC-PA STOP -> GO TO IN >\n");
                __broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                indoor_pending = 1;
                enable_occ_pa_on_in_passive = 0;

#if 0 //delete, Ver0.78
                if (is_waiting_emergency_pei_call_any()) {
                    emergency_pei2cab_start = 1;
                    ret = pei2cab_call_start(1);
                }
#endif

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                break;
            }
            else if (!enable_occ_pa_on_in_passive && enable_occ_pa_on_out_passive) {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
                if (ret != 0)
                    break;

                used_len = rxlen;
                occ_pa_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                //set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                printf("\n< OCC-PA STOP -> GO TO OUT >\n");
                __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                outdoor_pending = 1;
                enable_occ_pa_on_out_passive = 0;

#if 0 //delete, Ver0.78
                if (is_waiting_emergency_pei_call_any()) {
                    emergency_pei2cab_start = 1;
                    ret = pei2cab_call_start(1);
                }
#endif

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19

                break;
            }
            else if (enable_occ_pa_on_in_passive && enable_occ_pa_on_out_passive) {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INOUTDOOR_BCAST_START, 0);
                if (ret != 0)
                    break;

                used_len = rxlen;
                occ_pa_pending = 0;

                connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                printf("\n< OCC-PA STOP -> GO TO INOUT >\n");
                __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;
                indoor_pending = 1;
                outdoor_pending = 1;
                enable_occ_pa_on_in_passive = 0;
                enable_occ_pa_on_out_passive = 0;

#if 0 //delete, Ver0.78
                if (is_waiting_emergency_pei_call_any()) {
                    emergency_pei2cab_start = 1;
                    ret = pei2cab_call_start(1);
                }
#endif

                break;
            }
        }

        occ_pa_pending = 0;

        now_occ_pa_stop = 0;

        if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, &__mon_addr, NULL, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                pei2cab_call_monitoring = 1;
                __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                ret = get_call_is_emergency();

                add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);
                printf("< OCC-PA STOP -> PEI MON START(%d-%d) >\n", __car_num, __dev_num);

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }
            break;
        }

        if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, &__mon_addr, &__tx_addr, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                cab2cab_call_monitoring = 1;
                __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                printf("< OCC-PA STOP -> CAB MON START(%d-%d) >\n", __car_num, __dev_num);

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }
            break;
        }

        if (indoor_waiting == 1) {
            indoor_pending = 1;
            __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;
            printf("< OCC-PA STOP -> WAITING IN START >\n");
            break;
        }

        if (outdoor_waiting == 1) {
            outdoor_pending = 1;
            __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;
            printf("< OCC-PA STOP -> WAITING OUT START >\n");
            break;
        }

        __broadcast_status = COP_STATUS_IDLE;
        printf("< OCC-PA STOP -> IDLE >\n");
        break;

/*
__WAITING_PEI2CAB_CALL_START*/
      case __WAITING_PEI2CAB_CALL_START:
        if (pei2cab_call_waiting == 1) {
            if (cab2cab_reject_waiting == 1) {
                ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
                if (ret == 0) {
                    used_len = rxlen;
                    cab2cab_reject_waiting = 0;

                    stat = del_call_list(CAB_LIST, 0, 0, -1, -1);

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB
                }
            }
            else {
                pei2cab_call_pending = 1;
                pei2cab_call_waiting = 0;

                ret = pei2cab_call_start(0);
                printf("< REJECT CAB & PEI WAKEUP -> WAITING PEI START >\n");
            }
        }

        if (pei2cab_call_pending && rxlen && buf && rxlen >= (int)buf[0])
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
#if 0
                if (!is_waiting_normal_call(PEI_LIST, __car_num, __dev_num))
                    break;
#else
                if (is_waiting_normal_call(PEI_LIST, __car_num, __dev_num))
                    is_emer_call = 0;
                else
                    is_emer_call = 1;
#endif

				pei_call_count++;
				printf(" < pei_call_count = %d\n", pei_call_count);

                used_len = rxlen;

                //cop_occ_pa_and_occ_call_codec_start();
                connect_cop_cab2cab_call_multicast(buf, rxlen);		// mhryu : 05202015
               	connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);
                cop_monitoring_encoding_start();					// mhryu : 20150526

                if (!is_emer_call) {
                    printf("< WAITING PEI-CALL -> PEI START%s >\n",
                        (enable_pei_wakeup_on_in_passive | enable_pei_wakeup_on_out_passive) ? "(STANDBY CAB-PA)" : "(NORMAL)");
                } else {
                    printf("< WAITING PEI-CALL -> PEI START%s >\n",
                        (enable_pei_wakeup_on_in_passive | enable_pei_wakeup_on_out_passive) ? "(STANDBY CAB-PA)" : "(EMER)");
                }

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                add_call_list(PEI_LIST, CALL_STATUS_LIVE, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

                pei_led_on_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                force_call_menu_draw();

	        	pei2cab_call_running = 1;
				update_pei_join_active_status();
                __broadcast_status = __PEI2CAB_CALL_START;

#ifdef PEI_MONITORING_SAVE
                save_file_fd = file_open_mon_save();
#endif
                //decoding_start(); //2013/07/02

				/* <<<< VOIP */ // WAITING_PEI2CAB_CALL_START -> __PEI2CAB_CALL_START
				//cop_voip_call_codec_init();
				voip_call_start_prepare();
				//cop_voip_call_codec_start();
				reset_voip_frame_info();
				/* VOIP >>>> */

				if(occ_pa_running)
				{
					set_occ_mic_audio_switch_on();
					occ_mic_volume_set(occ_mic_vol_atten);
					set_occ_mic_to_recode_audio_switch_on();
				}

				if(inout_bcast_processing)
				{
					set_cop_spk_audio_switch_off();
					temporary_mon_enc_connect_to_passive_pa_tx(1, __mon_addr);
				}

                break;
            }
			/* __CAB2CAB_CALL_STOP */
			// Added by Michael RYU(July 1, 2015)
			if (ret == -1) {
				DBG_LOG("\n");
				ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_STOP, 0);
				if (ret == 0) {
					//set_key_led(enum_COP_KEY_CALL_CAB);             // Added by Michael RYU(June. 29, 2015)
					//set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);    // Changed value from '1' to '0' to disable the blink
					__car_num = get_car_number(buf, rxlen);
					__dev_num = get_device_number(buf, rxlen);

					printf("< ON WAITING OCC-PEI, GOT CAB2CAB STOP(%d-%d) >\n", __car_num, __dev_num);
					ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (ret == 0) {
						used_len = rxlen;
                   		play_stop_call_rx_data();

            			cop_speaker_volume_mute_set();
            			occ_mic_volume_mute_set();
            			decoding_stop();

                   		close_cop_multi_rx_sock();
                   		close_call_and_occ_pa_multi_tx();
                   		if (call_mon_sending)
                       		close_cop_multi_monitoring();

                   		cab2cab_call_running = 0;

                   		cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                   		reset_func_multi_rx_data_ptr();
               			reset_fire_multi_rx_data_ptr(); //
               			reset_auto_multi_rx_data_ptr(); //

               			force_call_menu_draw();
					}
            		cop_occ_pa_and_occ_call_codec_stop();
            		__discard_cop_pa_and_call_mic_encoding_data();
					break;
				}
			}


            if (indoor_pending) {
            if (ret == -1 && !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_waiting = 0;
                indoor_pending = 0;

                if (!inout_bcast_processing) {
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                    cop_pa_mic_codec_start();
                    set_cop_pa_mic_audio_switch_on();
                    cop_pa_and_call_mic_volume_set();
                }

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;

                printf("< WAITING PEI-CALL, GET IN START(STAT:0x%X)\n",inout_bcast_processing);

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
            }
            else if (ret == -1 && (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_waiting = 0;
                indoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

                if (!inout_bcast_processing) {
					if(!tractor_cab2cab_start) {		// mhryu 20150526
                    	cop_pa_mic_volume_mute_set();
                    	cop_pa_mic_codec_stop();
					}
                    close_cop_multi_tx();
                }

                printf("< WAITING PEI-CALL, GET IN STOP(STAT:0x%X)\n",inout_bcast_processing);
                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
            }

            if (outdoor_pending) {
            if (ret == -1 && !(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_waiting = 0;
                outdoor_pending = 0;

                if (!inout_bcast_processing) {
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                    cop_pa_mic_codec_start();
                    set_cop_pa_mic_audio_switch_on();
                    cop_pa_and_call_mic_volume_set();
                }

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;

                printf("< WAITING PEI-CALL, GET OUT START(STAT:0x%X)\n",inout_bcast_processing);

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
            }
            else if (ret == -1 && (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_waiting = 0;
                outdoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;

                if (!inout_bcast_processing) {
					if(!tractor_cab2cab_start) {		// mhryu 20150526
                    	cop_pa_mic_volume_mute_set();
                    	cop_pa_mic_codec_stop();
					}
                    close_cop_multi_tx();
                }

                printf("< WAITING PEI-CALL, GET OUT STOP(STAT:0x%X)\n",inout_bcast_processing);
                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
            }
        }

        if_call_selected_call_menu_draw();

        /* CAB-PA -> CALL simultaneous */
        if (inout_bcast_processing) { // <<=============
			running_cop_pa_and_call_mic_audio_sending();
		} else {
            __discard_cop_pa_and_call_mic_encoding_data();
		}

      /*** __WAITING_PEI2CAB_CALL_START ##############################################*/
        break;

/*
__PEI2CAB_CALL_START*/
      case __PEI2CAB_CALL_START:
        if (pei2cab_call_pending == 1) {
            pei2cab_call_pending = 0;
            if (pei_comm_half_duplex) {
				Log_Status = LOG_UNKNOWN_ERR;
                pei_half_way_com_flag = 1;
            }
            else
                pei_half_way_com_flag = 0;
        }

        if (rxlen && buf && rxlen >= (int)buf[0]) {
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_STOP, 0);
            stat = get_call_is_emergency();
            //if (ret == 0 && !stat)
            if (ret == 0 /*&& !stat*/) // Ver0.78
            {
                used_len = rxlen;

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                printf("< ON PEI-CALL, GOT %s PEI STOP(%d-%d) >\n",
                            stat ? "EMER" : "NORMAL", __car_num, __dev_num);
                stat2 = is_live_call(PEI_LIST, __car_num, __dev_num);
                if (stat2) {
                    stat = del_call_list(PEI_LIST, stat, 0, __car_num, __dev_num);
				}
                if (/*stat == 0 &&*/ stat2 == 1) {
                    if (pei_comm_half_duplex)
						Log_Status = LOG_READY;

                    play_stop_call_rx_data();

                    close_cop_multi_rx_sock();
					if(pei_call_count == 2)
                    	close_cop_multi_rx_sock_2nd();
                    close_call_and_occ_pa_multi_tx();
                    close_cop_multi_monitoring();

                    force_call_menu_draw();

                    printf("\n< PEI-CALL(NORMAL) -> WAITING STOP > \n");
                    pei2cab_call_pending = 0; // <<==========
                    __broadcast_status = WAITING_PEI2CAB_CALL_STOP;
                }

				if (is_waiting_emergency_pei_call_any_2(__car_num, __dev_num)) {
					stat = del_call_list(PEI_LIST, 1, 0, __car_num, __dev_num);
					if (!stat) {
						used_len = rxlen;

						//cop_speaker_volume_mute_set();
						//stop_play_beep();
						//decoding_stop();

						printf("\n< ON PEI-CALL(NORMAL), DEL WAIT EMER PEI(%d-%d) >\n", __car_num, __dev_num);

						//pei_led_off_if_no_wait_normal_pei_and_emer_pei_call(); // enum_COP_KEY_CALL_PEI
						force_call_menu_draw();
						//__broadcast_status = COP_STATUS_IDLE;
					}
				}

                if (occ_key_status) {
                    occ_key_status = 0;
                    occ_speaker_volume_mute_set();
                   	set_cop_spk_mute_onoff(0);
                    //cop_speaker_volume_set();
                }

                //clear_key_led(enum_COP_KEY_OCC);
                //set_key_led_blink(enum_COP_KEY_OCC, 0);

                pei_led_control_on_pei2cab_call(); // <- enum_COP_KEY_CALL_PEI

                // 2014/02/13
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }

			if(ret == -1) {
				// __PEI2CAB_CALL_START on __PEI2CAB_CALL_START // Call Join Case
				ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_START, 0);
				if (ret == 0)
				{
                	__car_num = get_car_number(buf, rxlen);
                	__dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);

                	if (is_waiting_normal_call(PEI_LIST, __car_num, __dev_num))
                    	is_emer_call = 0;
                	else
                	    is_emer_call = 1;

                	add_call_list(PEI_LIST, CALL_STATUS_LIVE, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

					ret = update_pei_join_call_list_status();
					pei_led_on_if_no_wait_pei_call();

					//if(pei_call_count < MAX_PEI_CALL_JOIN)
					if(pei_call_count < MAX_PEI_CALL_JOIN)	// Changed by MH RYU
					{
						pei_call_count++;
						if(pei_call_count == 2) {
							mixer_buffer_init();
							connect_cop_cab2cab_call_multicast_2nd(buf, rxlen);
						}
					}
					else
						printf("MAXIMUM PEI Call joined.....(Current Call join = %d calls)\n", pei_call_count);
					used_len = rxlen;

					return used_len;
				}
			}

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("< ON PEI-CALL, ADD MON %s PEI(%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    /*move_wakeup_to_1th_index(PEI_LIST);*/

                    force_call_menu_draw();

                    pei_led_control_on_pei2cab_call(); // <- enum_COP_KEY_CALL_PEI
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) { // *
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON PEI START, DEL WAIT-MON %s PEI %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;
                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("\n< ON PEI CALL, ADD %s PEI(%d-%d) >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

				force_call_menu_draw();
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON PEI-CALL, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;
				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                printf("< ON PEI-CALL, ADD WAKEUP-CAB(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON PEI-CALL, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
	            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_STOP, 0);
    	        if (ret == 0) {
                	__car_num = get_car_number(buf, rxlen);
            	    __dev_num = get_device_number(buf, rxlen);
        	        if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
    	                printf("< ON PEI-CALL, GOT CAB STOP(%d-%d) >\n", __car_num, __dev_num);
	                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    	if (ret == 0) {
                 	        used_len = rxlen;

                        	cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    	    force_call_menu_draw();
                	    }
            	    }
        	    }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1
                set_key_led_blink (enum_COP_KEY_CALL_CAB, 1);
                printf("< ON PEI-CALL, ADD WAKEUP-CAB(%d-%d) by CAB MON >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) { // *
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("\n< ON PEI-CALL, ADD WAIT-MON CAB(%d-%d) >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    /*move_wakeup_to_1th_index(CAB_LIST);*/

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                } else {
                    printf("\n< ON PEI-CALL START, NO CAB WAIT LIST(%d-%d) >\n", __car_num, __dev_num);
                }
                used_len = rxlen;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) { // *
                    printf("\n< ON PEI-CALL, DEL WAIT-MON CAB(%d-%d) >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1 && occ_pa_pending) {
                ret = check_avc2cop_cmd(buf, rxlen, OCC_PA_BCAST_STOP, 0);
                if (ret == 0) {
                    used_len = rxlen;
                    printf("< ON PEI-CALL, GET OCC-PA STOP >\n");
                    cop_occ_pa_and_occ_call_codec_stop();
                    close_call_and_occ_pa_multi_tx();
	            	occ_pa_running = 0;
	            	occ_pa_pending = 0;
                    //clear_key_led(enum_COP_KEY_OCC);

                    // 2014/02/13
                    reset_cop_pa_multi_rx_data_ptr();
                    reset_func_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_auto_multi_rx_data_ptr(); //
                }
            }

            /* CAB-PA on __PEI2CAB_CALL_START *******************************************/
            if (indoor_pending) {
            	if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK))
            	{
					ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
					if (ret == 0) {
                    	used_len = rxlen;
#if defined(INOUT_LED_ONOFF_BY_COP)
                    	set_key_led(enum_COP_KEY_PA_IN);
                    	set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                    	indoor_pending = 0;
                    	indoor_waiting = 0;

                    	if (    !inout_bcast_processing
                        	 && (enable_in_pa_simultaneous_on_pei2cab == 1)
                       	)
                    	{
							#if 0
                        	if (occ_key_status)
                           		connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                        	else
                           		connect_cop_passive_bcast_tx_while_othercall(); // CALL + CAB-PA: Simutaneous
							#else
							set_cop_spk_audio_switch_off();
							temporary_mon_enc_connect_to_passive_pa_tx(1, __mon_addr);
							#endif
                    	}

                    	inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;
                    	printf("< ON PEI-CALL, GET IN START(STAT:0x%X) >\n", inout_bcast_processing);

                    	reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                	}
            	}
            	else if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
            	{
                	ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
                	if (ret == 0) {
                    	used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    	clear_key_led(enum_COP_KEY_PA_IN);
                    	set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                    	indoor_pending = 0;

                    	inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;
                    	if (    !inout_bcast_processing
                         	/*&& (enable_in_pa_simultaneous_on_pei2cab == 1)*/
                       	) {
							#if 0
                       		if (occ_key_status)
                          		close_cop_multi_tx();
                       		else
                          		close_cop_multi_passive_tx();
							#else
							set_cop_voip_call_spk_audio_switch_on();
							temporary_mon_enc_connect_to_passive_pa_tx(0, 0);
							#endif
                    	}
                    	printf("< ON PEI-CALL, GET IN STOP(STAT:0x%X)\n", inout_bcast_processing);
                    	enable_pei_wakeup_on_in_passive = 0;
                    	enable_in_pa_simultaneous_on_pei2cab = 0;

                    	// 2014/02/12
                    	reset_func_multi_rx_data_ptr();
                    	reset_fire_multi_rx_data_ptr(); //
                    	reset_auto_multi_rx_data_ptr(); //
                	}
            	}
            }

            if (outdoor_pending) {
            	if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK))
            	{
                	ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
                	if (ret == 0) {
                    	used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    	set_key_led(enum_COP_KEY_PA_OUT);
                    	set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                    	outdoor_pending = 0;
                    	outdoor_waiting = 0;

                    	if (    !inout_bcast_processing
                    	     /*&& (enable_out_pa_simultaneous_on_pei2cab == 1)*/
                       	) {
							#if 0
                       		if (occ_key_status)
                         		connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                       		else
                         		connect_cop_passive_bcast_tx_while_othercall(); // CALL + CAB-PA
							#else
							set_cop_spk_audio_switch_off();
							temporary_mon_enc_connect_to_passive_pa_tx(1, __mon_addr);
							#endif
                    	}
                    	inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    	printf("< ON PEI CALL, GET OUT START(STAT:0x%X) >\n", inout_bcast_processing);

                    	reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                	}
            	}
            	else if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
            	{
                	ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
                	if (ret == 0) {
                    	used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    	clear_key_led(enum_COP_KEY_PA_OUT);
                    	set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                    	outdoor_pending = 0;
                    	inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    	if (    !inout_bcast_processing
                       		  && (enable_out_pa_simultaneous_on_pei2cab == 1)
                       	) {
							#if 0
                       		if (occ_key_status)
                        		close_cop_multi_tx();
                       		else
                          		close_cop_multi_passive_tx();
							#else
							set_cop_voip_call_spk_audio_switch_on();
							temporary_mon_enc_connect_to_passive_pa_tx(0, 0);
							#endif
                    	}

                    	printf("< ON PEI CALL, GET OUT STOP(STAT:0x%X-%d) >\n",
                            	inout_bcast_processing, enable_out_pa_simultaneous_on_pei2cab);

                    	enable_pei_wakeup_on_out_passive = 0;
                    	enable_out_pa_simultaneous_on_pei2cab = 0;
                    	// 2014/02/12
                    	reset_func_multi_rx_data_ptr();
                    	reset_fire_multi_rx_data_ptr(); //
                    	reset_auto_multi_rx_data_ptr(); //
                	}
            	}
            }
            /* CAB-PA on __PEI2CAB_CALL_START *******************************************/
        }

        if (pei2cab_call_running) {
            if (occ_key_status) {

#if (PEI_OCC_DUPLEX_MODE == PEI_OCC_HALF_DUPLEX)
                pei_half_way_com_flag = 1;
#else
                pei_half_way_com_flag = 0;
#endif

#if (PEI_OCC_DUPLEX_MODE == PEI_OCC_HALF_DUPLEX)
				if(occ_pa_en_get_level())       /* BREAK signal is high on here */
				{
					//DBG_LOG("occ_pa_enable_level = 0\n");
					occ_tx_rx_en_ext_flag = 0;		/* PTT OFF */
					occ_pei_call_ack_set_low();
				} else {                            /* BREAK signal is low on here */
					//DBG_LOG("occ_pa_enable_level = 1\n");
					occ_tx_rx_en_ext_flag = 1;		/* PTT ON */
					occ_pei_call_ack_set_high();
				}

                if (occ_tx_rx_en_ext_flag != old_occ_tx_rx_en_ext_flag){
                    if (occ_tx_rx_en_ext_flag) {
                        __car_num = get_call_car_num(PEI_LIST);
                        __dev_num = get_call_id_num(PEI_LIST);
						DBG_LOG("cannot_talk(occ_tx_rx_en_ext_flag = 1 ==> BREAK = LOW)\n");
                        send_cop2avc_cmd_pei2cab_cannot_talk_request(0, __car_num, __dev_num);
                    } else {
                        __car_num = get_call_car_num(PEI_LIST);
                        __dev_num = get_call_id_num(PEI_LIST);
						DBG_LOG("can_talk(occ_tx_rx_en_ext_flag = 0 ==> BREAK = HIGH)\n");
                        send_cop2avc_cmd_pei2cab_can_talk_request(0, __car_num, __dev_num);
                    }
                    old_occ_tx_rx_en_ext_flag = occ_tx_rx_en_ext_flag;
                }
#endif
            } else {
                if (pei_half_way_com_flag == 1) {
					DBG_LOG("\n");
                    pei_half_way_com_flag = 0;

                    if (pei_comm_half_duplex == 0)
                        send_cop2avc_cmd_pei2cab_can_talk_and_receive_request(0, __car_num, __dev_num);
                    else if (pei_comm_half_duplex && cop_cantalk_on_pei2cab_call == 0) {
                        __car_num = get_call_car_num(PEI_LIST);
                        __dev_num = get_call_id_num(PEI_LIST);
                        send_cop2avc_cmd_pei2cab_can_talk_request(0, __car_num, __dev_num);
                    }
                }
            }

            if (    inout_bcast_processing
                && (   enable_in_pa_simultaneous_on_pei2cab == 1
                    || enable_out_pa_simultaneous_on_pei2cab == 1)
               )
            {
                if (occ_key_status)
				{
					running_voip_call_audio_sending();
                }
                else 
				{
					running_voip_call_audio_sending();
                	__discard_occ_pa_and_call_mic_encoding_data();
                }
            } else {
                if (occ_pa_en_get_level()) {
#if (PEI_OCC_DUPLEX_MODE == PEI_OCC_HALF_DUPLEX)
					running_occ_pa_and_occ_call_audio_sending(1, 1, 0);
#else
					running_occ_pa_and_occ_call_audio_sending(0, 0, 1);
#endif
                }
				running_voip_call_audio_sending();
            }

			if(pei_call_count == 1)
			{
				rxbuf = try_read_call_rx_data(&len, 0, 1);
				if(len > 0) {
					running_call_recv_and_dec(0, rxbuf, len, 1, 0);
				}
				/* if (occ_key_status) {
				   running_cop_pa_and_call_mic_audio_discard();
				} */
			}
			else if(pei_call_count == 2)
			{
				rxbuf = try_read_call_rx_data(&len, 0, 1);
				if(len) {
					if(call_mixer_rxlen1[0] > 0) {	// First Buffer is not empty
						memcpy(&call_mixer_rxbuf1[1][0], rxbuf, len);	// Data copy to Second buffer
						call_mixer_rxlen1[1] = len;						// Second Buffer size
						call_mixer_outptr1 = 0;
					} else {						// First Buffer is empty
						memcpy(&call_mixer_rxbuf1[0][0], rxbuf, len);	// Data copy to First buffer
						call_mixer_rxlen1[0] = len;						// First Buffer size
						call_mixer_outptr1 = 1;
					}
				}

				rxbuf = try_read_call_rx_data_2nd(&len, 0, 1);
				if(len) {
					if(call_mixer_rxlen2[0] > 0) {	// First Buffer is not empty
						memcpy(&call_mixer_rxbuf2[1][0], rxbuf, len);	// Data copy to Second buffer
						call_mixer_rxlen2[1] = len;						// Second Buffer size
						call_mixer_outptr2 = 0;
					} else {
						memcpy(&call_mixer_rxbuf2[0][0], rxbuf, len);
						call_mixer_rxlen2[0] = len;
						call_mixer_outptr2 = 1;
					}
				}

				if( (call_mixer_rxlen1[0] > 0) && (call_mixer_rxlen2[0] > 0) ) {
					/* Mixing two RTP packet to COP */
					mixed_buf = mixing_two_call(&call_mixer_rxbuf1[0][0], 172, 
												&call_mixer_rxbuf2[0][0], 172, 1);

					/* Send RTP Packet to COP : Mixed Packet */
					running_call_recv_and_dec(0, mixed_buf, 172, 1, 0);

					if(call_mixer_rxlen1[1] > 0) {
						memcpy(&call_mixer_rxbuf1[0][0], &call_mixer_rxbuf1[1][0], 172);
						call_mixer_rxlen1[0] = call_mixer_rxlen1[1];
						call_mixer_rxlen1[1] = 0;
					} else {
						call_mixer_rxlen1[0] = 0;
					}
					if(call_mixer_rxlen2[1] > 0) {
						memcpy(&call_mixer_rxbuf2[0][0], &call_mixer_rxbuf2[1][0], 172);
						call_mixer_rxlen2[0] = call_mixer_rxlen2[1];
						call_mixer_rxlen2[1] = 0;
					} else {
						call_mixer_rxlen2[0] = 0;
					}
               }
			}

           	running_monitoring_audio_sending(0, 1);		// Changed by Michael RYU(20150817) : For sending the monitoring
		}

        if_call_selected_call_menu_draw();
      	/*** __PEI2CAB_CALL_START #####################################################*/
        break;

      case WAITING_PEI2CAB_CALL_STOP:
        if (pei2cab_call_pending && rxlen && buf && rxlen >= (int)buf[0])
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_STOP, 0);
            stat3 = get_call_is_emergency();

#if 0
            if (ret == 0 && stat3)
                break;
            else if (ret == -1)
                stat = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_STOP, 0);
#else
            stat = -1; // Ver0.78
#endif

            if (ret == 0 /*|| (ret == -1 && stat == 0 && stat3)*/) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);

                stat2 = is_live_call(PEI_LIST, __car_num, __dev_num);
                if (stat2)
				{
                    stat = del_call_list(PEI_LIST, stat3, 0, __car_num, __dev_num);
					//DBG_LOG("Delete Call List = %d-%d\n", __car_num, __dev_num);
					//print_call_list(PEI_LIST);
				} else {
					//DBG_LOG("stat2 = %d(%d-%d)\n", stat2, __car_num, __dev_num);
					//print_call_list(PEI_LIST);
				}

                if (stat2 /*&& !stat*/) {
                    used_len = rxlen;

                    printf("< WAITING %s PEI STOP -> PEI STOP\n", stat3 ? "EMER" : "NORMAL");

					if(pei_call_count == 2) {
						close_call_multi_tx_2nd();
						close_cop_multi_rx_sock_2nd();
					}

					close_call_multi_tx();	// mhryu : 05202015
                   	close_cop_multi_rx_sock();
                   	close_call_and_occ_pa_multi_tx();
                   	close_cop_multi_monitoring();
                    occ_speaker_volume_mute_set();
					temporary_mon_enc_connect_to_passive_pa_tx(0, 0);

                    pei2cab_call_pending = 0;
                    pei2cab_call_running = 0;

                    stat = is_live_call(PEI_LIST, 0, 0);
					//DBG_LOG("is_live_call(PEI_LIST, 0, 0) = %d\n", stat);
                    if (   !is_waiting_normal_call_any(PEI_LIST, 0, 0)
                        && !is_waiting_emergency_pei_call_any_2(0, 0) && !stat)
                    {
                        clear_key_led(enum_COP_KEY_CALL_PEI);
                        set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
                    }

					if( (pei_call_count == 3) || (pei_call_count == 1) )
						pei_call_count = 0;
                }

                // 2014/02/13
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //

            }

            /* CAB-PA on WAITING_PEI2CAB_CALL_STOP *******************************************/
            if (indoor_pending) {
            if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK))
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_IN);
                    set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                    indoor_pending = 0;
                    indoor_waiting = 0;

                    if (    !inout_bcast_processing
                         && (enable_in_pa_simultaneous_on_pei2cab == 1)
                       )
                    {
                       if (occ_key_status)
                         connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                       else
                         connect_cop_passive_bcast_tx_while_othercall(); // CAB-PA + CALL: Simutaneous
                    }
                    inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;
                    printf("< ON WAITING PEI STOP, GET IN START(STAT:0x%X) >\n", inout_bcast_processing);

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
            }
            else if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    clear_key_led(enum_COP_KEY_PA_IN);
                    set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                    indoor_pending = 0;

                    inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;
                    if (    !inout_bcast_processing
                         && (enable_in_pa_simultaneous_on_pei2cab == 1)
                       )
                        close_cop_multi_passive_tx();
                    printf("< ON WAITING PEI STOP, IN STOP(STAT:0x%X)\n", inout_bcast_processing);

                    enable_pei_wakeup_on_in_passive = 0;
                    enable_in_pa_simultaneous_on_pei2cab = 0;

                    // 2014/02/12
                    reset_func_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_auto_multi_rx_data_ptr(); //
                }
            }
            }

            if (outdoor_pending) {
            if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK))
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_OUT);
                    set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                    outdoor_pending = 0;
                    outdoor_waiting = 0;

                    if (    !inout_bcast_processing
                         && (enable_out_pa_simultaneous_on_pei2cab == 1)
                       ) {
                       if (occ_key_status)
                         connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                       else
                         connect_cop_passive_bcast_tx_while_othercall(); // CAB-PA + CALL: Simutaneous
                    }
                    inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    printf("< ON WAITING PEI STOP, GET OUT START(STAT:0x%X) >\n", inout_bcast_processing);

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
            }
            else if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    clear_key_led(enum_COP_KEY_PA_OUT);
                    set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                    outdoor_pending = 0;
                    inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    if (    !inout_bcast_processing
                         && (enable_out_pa_simultaneous_on_pei2cab == 1)
                       )
                        close_cop_multi_passive_tx();

                    enable_pei_wakeup_on_out_passive = 0;
                    enable_out_pa_simultaneous_on_pei2cab = 0;

                    printf("< ON WAITING PEI STOP, OUT STOP(STAT:0x%X) >\n", inout_bcast_processing);

                    // 2014/02/12
                    reset_func_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_auto_multi_rx_data_ptr(); //
                }
            }
            }
            /* CAB-PA on WAITING_PEI2CAB_CALL_STOP *******************************************/
        }
        else if (pei2cab_call_pending == 0) {
            //ret = get_status_key_blink(enum_COP_KEY_CALL_PEI);
            //if (ret == 0)
            {

				if (occ_pa_running)
				{
					//__broadcast_status = OCC_PA_BCAST_START;
					send_cop2avc_cmd_occ_pa_start_request(0);
					__broadcast_status = WAITING_OCC_PA_BCAST_START;
					now_occ_pa_start_wait = 1;
					occ_pa_pending = 1;
					printf("\n < WAITING PEI-CALL STOP -> WAITING_OCC_PA_BCAST_START >\n");
					break;
				}

                //clear_key_led(enum_COP_KEY_CALL_PEI);
	        	pei2cab_call_running = 0;
                occ_speaker_volume_mute_set();

                if (is_waiting_emergency_pei_call()) {
                    printf("\n< WAITING PEI-CALL STOP, WAITING EMER PEI-CALL -> IDLE >\n");
                    //emergency_pei2cab_start = 1;
                    //ret = pei2cab_call_start(1);
                    __broadcast_status = COP_STATUS_IDLE;
                    break;
                }

                if (inout_bcast_processing) {
                    cop_speaker_volume_mute_set();
                    occ_mic_volume_mute_set();
                    decoding_stop();

                    cop_occ_pa_and_occ_call_codec_stop();
                    cop_monitoring_encoding_stop();
                    clear_rx_tx_mon_multi_addr();

                    //close_cop_multi_passive_tx();
                }

                if (enable_in_pa_simultaneous_on_pei2cab || enable_out_pa_simultaneous_on_pei2cab) {
                    close_cop_multi_passive_tx();
                }

	            if (   (enable_pei_wakeup_on_in_passive == 1)
                    || (enable_pei_wakeup_on_out_passive == 1)
                    || (enable_in_pa_simultaneous_on_pei2cab == 1) 
                    || (enable_out_pa_simultaneous_on_pei2cab == 1) 
                   )
                {
                    if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
                        menu_call_screen_reset();
                        menu_head_draw(global_menu_active, bmenu_active, 0, 0, 0, 0, 0, 0);
                        call_draw_menu(&__cab_call_list, &__pei_call_list, 1, 0, 0);
                    }
    
                    if (    (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
                         && (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                       )
                    {
                        connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                        indoor_pending = 1;
                        outdoor_pending = 1;

                        __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;

                        enable_in_pa_simultaneous_on_pei2cab = 0; // 2013/11/15
                        enable_out_pa_simultaneous_on_pei2cab = 0; // 2013/11/15

                        if (!is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
                            enable_pei_wakeup_on_in_passive = 0;
                            enable_pei_wakeup_on_out_passive = 0;
                            printf("\n< WAITING PEI-CALL STOP -> GO TO INOUT >\n");
                        }
                        else {
                            enable_pei_wakeup_on_in_passive = 1;
                            enable_pei_wakeup_on_out_passive = 1;
                            printf("\n< WAITING PEI-CALL STOP -> GO TO INOUT(WAITING PEI) >\n");
                        }
                        break;
                    }

                    if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) {
                        connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                        enable_in_pa_simultaneous_on_pei2cab = 0; // 2013/11/15

                        if (!is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
                            enable_pei_wakeup_on_in_passive = 0;
                            printf("\n< WAITING PEI-CALL STOP -> GO TO IN >\n");
                        }
                        else {
                            enable_pei_wakeup_on_in_passive = 1;
                            printf("\n< WAITING PEI-CALL STOP -> GO TO IN(WAITING PEI) >\n");
                        }

                        indoor_pending = 1;
                        __broadcast_status = __PASSIVE_INDOOR_BCAST_START;

                        reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19

                        break;
                    }

                    if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK) {
                        connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                        enable_out_pa_simultaneous_on_pei2cab = 0; // 2013/11/15

                        if (!is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
                            enable_pei_wakeup_on_out_passive = 0;
                            printf("\n< WAITING PEI-CALL STOP -> GO TO OUT >\n");
                        }
                        else {
                            enable_pei_wakeup_on_out_passive = 1;
                            printf("\n< WAITING PEI-CALL STOP -> GO TO OUT(WAITING PEI) >\n");
                        }

                        outdoor_pending = 1;
                        __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;

                        reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                        break;
                    }
                }

				if(pei_call_count == 2) {
					char TempBuf[30];
					send_all2avc_cycle_data(TempBuf, 8, 0, 0, 0);

					pei2cab_call_pending = 1;
					//DBG_LOG("Second call STOP ==> __broadcast_status = __PEI2CAB_CALL_START(Call PEI Button)\n");
					//DBG_LOG("pei_call_count = %d\n", pei_call_count);
            		__car_num = get_call_car_num(PEI_LIST);
            		__dev_num = get_call_id_num(PEI_LIST);
            		stat = get_call_is_emergency();

					//DBG_LOG("Join Call stop request : %d-%d\n", __car_num, __dev_num);
            		ret = send_cop2avc_cmd_pei2cab_call_stop_request(0, __car_num, __dev_num, stat, 0);
            		if (stat)
                		printf("\n< PEI KEY, EMER PEI START -> WAITING STOP(%d-%d) > \n", __car_num, __dev_num);
            		else
                		printf("\n< PEI KEY, NORMAL PEI START -> WAITING STOP(%d-%d) > \n", __car_num, __dev_num);

            		__broadcast_status = WAITING_PEI2CAB_CALL_STOP;
					pei_call_count = 3;
					break;
					//return used_len;
				} else {
                	__broadcast_status = __PEI2CAB_CALL_STOP;
					pei_call_count = 0;
				}
                break;
            }
        }

        if_call_selected_call_menu_draw();

        //__discard_cop_pa_and_call_mic_encoding_data(); //Need not, BUSAN COB TYPE BOARD
      /***** WAITING_PEI2CAB_CALL_STOP #######################################*/
        break;

/*
__PEI2CAB_CALL_STOP*/
      case __PEI2CAB_CALL_STOP:
        cop_speaker_volume_mute_set();
        occ_mic_volume_mute_set();

		if(tractor_cab2cab_start) {
			/* Tractor input to Speaker */
			set_tractor_mic_audio_switch_on();
			set_occ_mic_to_spk_audio_switch_on();
			occ_mic_volume_set(occ_mic_vol_atten);
		}

        decoding_stop();

        cop_occ_pa_and_occ_call_codec_stop();
        cop_monitoring_encoding_stop();

        clear_rx_tx_mon_multi_addr();

        if (is_waiting_normal_call(PEI_LIST, -1, -1) == 0) {
            if (   (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
                && (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                && indoor_pending == 0 && outdoor_pending == 0
               )
            {
                printf("< PEI STOP on PEI2CAB+INOUT START -> INOUT START\n");

                cop_pa_mic_volume_mute_set();
                close_cop_multi_tx();
                cop_pa_mic_codec_stop();

                connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;
                indoor_pending = 1;
                outdoor_pending = 1;
                break;
            }
            else if (   (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
                 && indoor_pending == 0
               )
            {
                printf("< PEI STOP on PEI2CAB+IN START -> IN START\n");
                cop_pa_mic_volume_mute_set();
                close_cop_multi_tx();
                cop_pa_mic_codec_stop();

                connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                //set_key_led_blink(enum_COP_KEY_PA_IN, 0);
                __broadcast_status = __PASSIVE_INDOOR_BCAST_START;

                indoor_pending = 1;

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                break;
            }
            else if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                 && outdoor_pending == 0
               )
            {
                printf("< PEI STOP on PEI2CAB+OUT START -> OUT START\n");
                close_cop_multi_passive_tx();

                connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                //set_key_led_blink(enum_COP_KEY_PA_IN, 0);
                __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                outdoor_pending = 1;

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                break;
           }
        }

        /*
        if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
            __broadcast_status = COP_STATUS_IDLE;
            printf("< PEI-CALL STOP -> IDLE(#1) >\n");
            break;
        }
        if (is_waiting_normal_call(CAB_LIST, -1, -1)) {
            __broadcast_status = COP_STATUS_IDLE;
            printf("< PEI-CALL STOP -> IDLE(#2) >\n");
            break;
        } */

        if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, &__mon_addr, NULL, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                pei2cab_call_monitoring = 1;
                __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                ret = get_call_is_emergency();

                printf("\n< PEI-CALL STOP -> PEI MON START(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);

                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }
            break;
        }

        if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, &__mon_addr, &__tx_addr, 0)) {
            ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
            if (ret == 0) {
                decoding_start();
                cab2cab_call_monitoring = 1;
                __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                printf("< PEI-CALL STOP -> CAB MON START(%d-%d) >\n", __car_num, __dev_num);

                    set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
            }
            break;
        }

        __discard_occ_pa_and_call_mic_encoding_data();

        __broadcast_status = COP_STATUS_IDLE;
        occ_tx_rx_en_ext_flag = 0;
        old_occ_tx_rx_en_ext_flag = -1;
        printf("< PEI-CALL STOP -> IDLE(#3) >\n");
        pei2cab_is_emergency = 0;

        call_mon_sending = 0;
		if (fire_alarm_key_status)
			lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */

        break;

/*
__WAITING_CAB2CAB_CALL_START */
      case __WAITING_CAB2CAB_CALL_START:
        if (cab2cab_call_pending && rxlen && buf && rxlen >= (int)buf[0]) {

            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_START, 0);
            if (ret == 0) {
                if (is_waiting_normal_call(PEI_LIST, -1, -1)) {

                    cab2cab_call_waiting = 0; // <=====
                    pei2cab_call_waiting = 0; // <=====
                    cop_speaker_volume_mute_set();
                    stop_play_beep();
                    decoding_stop();
                }
            }

            if (ret == 0) {
#if 1
				if(tractor_cab2cab_start) {
					printf("< PEI MON, TRACTOR CALL EN >\n");
				}
#endif
#if 0
                stat3 = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) {
					tractor_cab2cab_start = 1;

					printf("< PEI MON, TRACTOR CALL EN >\n");
				}
#endif
                __car_num = get_call_car_num(CAB_LIST);
                __dev_num = get_call_id_num(CAB_LIST);
				__train_num = get_call_train_num(CAB_LIST);
                if (__car_num == (char)-1 && __dev_num == (char)-1) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
                }
                else if (__car_num != (char)-1 && __dev_num != (char)-1) {
                    __car_num = get_car_number(buf, rxlen);
                    __dev_num = get_device_number(buf, rxlen);
					__train_num = get_train_number(buf, rxlen);
                    if (is_waiting_normal_call(CAB_LIST, __car_num, __dev_num) == 0) {
                        printf("\n< WAITING CAB-CALL START, CAB ID ERROR, CAB CALL START(%d-%d) >\n",
                            __car_num, __dev_num);
                        break;
                    }
                }

                used_len = rxlen;

                printf("\n< WAITING CAB-CALL -> CAB-CALL START (%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_LIVE, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

#if 1 // Ver 0.76
                stat2 = get_call_waiting_ctr(CAB_LIST);
                if (stat2 == 2) {
                    __tx_addr = get_rx_mult_addr(buf, rxlen);
                    put_rx_mult_addr(buf, __rx_addr_for_mon_for_push_to_talk);

                    printf("\n< WAITING CAB START, CHANGE RX ADDR (%X --> %X) >\n", __tx_addr, __rx_addr_for_mon_for_push_to_talk);
                }
#endif

                connect_cop_cab2cab_call_multicast(buf, rxlen);
                if (call_mon_sending) {
                    cop_monitoring_encoding_start();
                    connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);
                }

                cab_led_on_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                force_call_menu_draw();

	        //cab2cab_call_pending = 0;

                cab2cab_call_waiting = 0; // <=====
                pei2cab_call_waiting = 0; // <=====

	        	cab2cab_call_running = 1;
                __broadcast_status = __CAB2CAB_CALL_START;

                CurCab_ListNum = get_call_waiting_ctr(CAB_LIST);

//				if (!help_cab2cab_start) {
					call_start_prepare();
					//set_tractor_spk_audio_switch_on();		// Disable by mhryu : 20150526Audio Output to Tractor

//				}

                if (inout_bcast_processing) {
                    //cop_pa_mic_volume_mute_set();
                    //close_cop_multi_tx();
                    //cop_pa_mic_codec_stop();
                    connect_cop_passive_bcast_tx_while_othercall(); // CAB-PA + CALL: Simutaneous
                }

                //__discard_cop_pa_and_call_mic_encoding_data(); // <--DON'T USE AT BUSAN COB BOARD

                break;
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON WAITING CAB-CALL, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;

                cab2cab_call_waiting = 1;
				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                printf("\n< ON WAITING CAB-CALL, ADD CAB(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);;
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("\n< ON WAITING CAB-CALL, ADD WAIT-MON CAB %d-%d >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    /*move_wakeup_to_1th_index(CAB_LIST);*/

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    printf("\n< ON WAITING CAB-CALL, DEL CAB-MON %d-%d >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {

                used_len = rxlen;

                decoding_start();
                start_play_beep();
                set_cop_spk_audio_switch_on();
                cop_speaker_volume_set();
                pei2cab_call_waiting = 1;

                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);;

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("\n< ON WAITING CAB-CALL, ADD %s PEI(%d-%d) >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

				force_call_menu_draw();
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;
                    printf("< WAITING CAB-CALL -> %s PEI-CAB MON(%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);

                    cop_speaker_volume_mute_set();
                    occ_mic_volume_mute_set();
                    stop_play_beep();
                    decoding_stop();

                    decoding_start();
                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();

                    pei2cab_call_waiting = 0;

                    connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

#ifdef PEI_MONITORING_SAVE
                    file_close_mon_save(save_file_fd);
                    save_file_fd = -1;
                    sync();
#endif

                    pei2cab_call_monitoring = 1;
                    __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                    force_call_menu_draw();

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI
                }
            }
            }

            if (ret == -1 && !cab2cab_reject_waiting) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON WAITING CAB-CALL, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1 && cab2cab_reject_waiting) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                used_len = rxlen;
                cab2cab_reject_waiting = 0;

                // if (!is_waiting_emergency_pei_call_any()) //delete ver0.78
                {
                    printf("\n< WAITING CAB-CALL START, GOT REJECT CAB -> CAB STOP >\n");
                    stat = del_call_list(CAB_LIST, 0, 0, -1, -1);

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();

                    __broadcast_status = __CAB2CAB_CALL_STOP;

                    break;
                }
            }
            }

            if (indoor_pending) {
            if (ret == -1 && !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_waiting = 0;
                indoor_pending = 0;

                if (!inout_bcast_processing) {
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                    cop_pa_mic_codec_start();
                    set_cop_pa_mic_audio_switch_on();
                    cop_pa_and_call_mic_volume_set();
                }

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;

                printf("< WAITING CAB-CALL, GET IN START(STAT:0x%X)\n",inout_bcast_processing);

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
            }
            else if (ret == -1 && (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_waiting = 0;
                indoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

                if (!inout_bcast_processing) {
                    cop_pa_mic_volume_mute_set();
                    close_cop_multi_tx();
                    cop_pa_mic_codec_stop();
                }

                printf("< WAITING CAB-CALL, GET IN STOP(STAT:0x%X)\n",inout_bcast_processing);
                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
            }

            if (outdoor_pending) {
            if (ret == -1 && !(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_waiting = 0;
                outdoor_pending = 0;

                if (!inout_bcast_processing) {
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

                    cop_pa_mic_codec_start();
                    set_cop_pa_mic_audio_switch_on();
                    cop_pa_and_call_mic_volume_set();
                }

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;

                printf("< WAITING CAB-CALL, GET OUT START(STAT:0x%X)\n",inout_bcast_processing);

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
            }
            else if (ret == -1 && (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_waiting = 0;
                outdoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;

                if (!inout_bcast_processing) {
                    cop_pa_mic_volume_mute_set();
                    close_cop_multi_tx();
                    cop_pa_mic_codec_stop();
                }

                printf("< WAITING CAB-CALL, GET OUT STOP(STAT:0x%X)\n",inout_bcast_processing);

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
            }
        }

        if (!inout_bcast_processing)
            __discard_cop_pa_and_call_mic_encoding_data();

        if_call_selected_call_menu_draw();

        if (cab2cab_call_waiting || pei2cab_call_waiting)
			running_play_beep();

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		} else {
            __discard_cop_pa_and_call_mic_encoding_data();
		}
      /***** __WAITING_CAB2CAB_CALL_START ##################################### */
        break;

/*
__CAB2CAB_CALL_START*/
      case __CAB2CAB_CALL_START:
		if (cab2cab_call_pending == 1) {
			cab2cab_call_pending = 0;
        }

        if (rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);

                if (stat == 0) {
                    used_len = rxlen;

                    printf("< CAB-CALL START -> CAB-CALL STOP (%d-%d)\n", __car_num, __dev_num);

                    play_stop_call_rx_data();

                    close_cop_multi_rx_sock();
                    close_call_and_occ_pa_multi_tx();
                    if (call_mon_sending)
                        close_cop_multi_monitoring();

                    __broadcast_status = __CAB2CAB_CALL_STOP;
                    cab2cab_call_running = 0;

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    // 2014/02/13
                    reset_func_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_auto_multi_rx_data_ptr(); //

                    break;
                }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON CAB-CALL START, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;
				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                printf("< ON CAB-CALL START, ADD CAB WAKEUP(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON CAB-CALL, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;

                        cab_led_control_on_cab2cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1 // VER 0.76
                set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);
                printf("< ON CAB-CALL START, ADD CAB WAKEUP(%d-%d) by CAB MON >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) { // *
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("< ON CAB-CALL START, ADD CAB MON(%d-%d) >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    /*move_wakeup_to_1th_index(PEI_LIST);*/

                    cab_led_control_on_cab2cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
#if 1 // VER 0.76
                    stat2 = get_call_waiting_ctr(CAB_LIST);
                    if (stat2 == 2) {
                        __rx_addr_for_mon_for_push_to_talk = __tx_addr; // Ver 0.76
                        printf("\n< ON CAB START, NOW GET CALL RX ADDR (--> %X) >\n",
				__rx_addr_for_mon_for_push_to_talk);

                        close_cop_multi_rx_sock();
                        put_rx_mult_addr(buf, __rx_addr_for_mon_for_push_to_talk);
                        connect_cop_cab2cab_call_rx_multicast(buf, rxlen);
                        CurCab_ListNum = get_call_waiting_ctr(CAB_LIST);
                    }
#endif
                }
                else {
                    printf("\n< ON CAB-CALL START, NO CAB WAIT LIST(%d-%d) >\n", __car_num, __dev_num);
                }
                used_len = rxlen;

            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0); // *
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    printf("\n< ON CAB-CALL, DEL WAIT-MON CAB(%d-%d) >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, CALL_STATUS_MON_WAIT, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }

				} 
				else if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) { 
					printf("\n< ON CAB-CALL, DEL MON CAB %d-%d >\n", __car_num, __dev_num);
					stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (stat == 0) {
						used_len = rxlen;
						play_stop_call_mon_rx_data();

						cab2cab_call_monitoring = 0;

						occ_mic_volume_mute_set();
						close_cop_multi_monitoring();
						clear_rx_tx_mon_multi_addr();
						decoding_stop();
					}     
				}
				else
				{
					used_len = rxlen;
					printf("\n< ON CAB-CALL, CAB ID ERROR, DEL MON CAB(%d-%d) >\n", __car_num, __dev_num);
				}
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;
                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("\n< ON CAB-CALL START, ADD %s PEI(%d-%d) >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

				force_call_menu_draw();
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num)) { // *
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("< ON CAB-CALL START, ADD %s PEI MON(%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    /*move_wakeup_to_1th_index(PEI_LIST);*/

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0); // *
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("\n< ON CAB-CALL, DEL WAIT-MON %s PEI(%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, CALL_STATUS_MON_WAIT, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }
        }

        /** on __CAB2CAB_CALL_START **********************************************************/
        if (indoor_pending == 1) {
        if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK))
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_pending = 0;
                indoor_waiting = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif

                if (!inout_bcast_processing)
                    connect_cop_passive_bcast_tx_while_othercall(); // CAB-CALL with CAB-PA
                inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;
                printf("< ON CAB-CALL, GET IN START(STAT:0x%X) >\n", inout_bcast_processing);

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
        }
        else if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                indoor_pending = 0;
                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_IN);
                set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif

                if (!inout_bcast_processing)
                    close_cop_multi_passive_tx();
                printf("< ON CAB-CALL, GET IN STOP(STAT:0x%X) >\n", inout_bcast_processing);

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
        }
        }

        if (outdoor_pending == 1) {
        if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK))
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_pending = 0;
                outdoor_waiting = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                if (!inout_bcast_processing)
                    ret = connect_cop_passive_bcast_tx_while_othercall(); // CAB-CALL with CAB-PA
                inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;
                printf("< ON CAB-CALL, GET OUT START(STAT:0x%X) >\n", inout_bcast_processing);

                reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
            }
        }
        else if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
        {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
            if (ret == 0) {
                used_len = rxlen;
                outdoor_pending = 0;

#if defined(INOUT_LED_ONOFF_BY_COP)
                clear_key_led(enum_COP_KEY_PA_OUT);
                set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;
                if (!inout_bcast_processing)
                    close_cop_multi_passive_tx();
                printf("< ON CAB-CALL, GET OUT STOP(STAT:0x%X) >\n", inout_bcast_processing);

                // 2014/02/12
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
        }
        }
        /** on __CAB2CAB_CALL_START ################################################# */
#if 0
		if (tractor_cab2cab_start) {
			tractor_cab2cab_call_rx_tx_running();
		} else {
#endif
			if (cab2cab_call_running) {
				if (inout_bcast_processing)
					cab2cab_call_rx_tx_running(1);
				else
					cab2cab_call_rx_tx_running(0);
			}
#if 0
		}
#endif

        if_call_selected_call_menu_draw();

        /* Donot use this code on busan cob type board
         if (inout_bcast_processing)
            __discard_cop_pa_and_call_mic_encoding_data();
         */

      /*** __CAB2CAB_CALL_START ################################################ */
        break;

/*
WAITING_CAB2CAB_CALL_STOP*/
      case WAITING_CAB2CAB_CALL_STOP:
        if (cab2cab_call_pending && rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);

                if (    __car_num == get_call_car_num(CAB_LIST)
                     && __dev_num == get_call_id_num(CAB_LIST)
                     && is_live_call(CAB_LIST, __car_num, __dev_num)
                   )
                {
                    used_len = rxlen;
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);

                    force_call_menu_draw();

                    if (stat == 0) {
                        printf("< WAITING CAB-CALL STOP -> CAB-CALL STOP (%d-%d) >\n", __car_num, __dev_num);
						close_call_multi_tx();	// mhryu : 05202015
                        close_cop_multi_rx_sock();
                        close_call_and_occ_pa_multi_tx();
                        if (call_mon_sending)
                            close_cop_multi_monitoring();

                        cab2cab_call_pending = 0;
	             	    cab2cab_call_running = 0;

                        __broadcast_status = __CAB2CAB_CALL_STOP;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB
                        
                    }
                }

                // 2014/02/13
                reset_func_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr(); //
                reset_fire_multi_rx_data_ptr(); //
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
#if 0
                printf("< IGNORE CAB MON STOP on WAITING CAB-CALL STOP (%d-%d) >\n", __car_num, __dev_num);
                used_len = rxlen;
#endif
				if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
					printf("\n< ON WAITING CAB-CALL STOP, DEL MON WAIT-CAB %d-%d >\n", __car_num, __dev_num);
					stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (stat == 0) {
						used_len = rxlen;
						force_call_menu_draw();
					}
				}
				else if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) {
					printf("\n< ON WAITING CAB-CALL STOP, DEL MON CAB %d-%d >\n", __car_num, __dev_num);
					stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
					if (stat == 0) {
						used_len = rxlen;
						play_stop_call_mon_rx_data();

						cab2cab_call_monitoring = 0;

						occ_mic_volume_mute_set();
						close_cop_multi_monitoring();
						clear_rx_tx_mon_multi_addr();
						decoding_stop();
					}
				}
				else
				{
					used_len = rxlen;
					printf("\n< ON WAITING CAB-CALL STOP, CAB ID ERROR, DEL MON CAB(%d-%d) >\n", __car_num, __dev_num);
				}
			}
			}

            /* CAB-PA on __PEI2CAB_CALL_START *******************************************/
            if (indoor_pending) {
            if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK))
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_IN);
                    set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                    indoor_pending = 0;
                    indoor_waiting = 0;

                    if (    !inout_bcast_processing
                         /*&& (enable_in_pa_simultaneous_on_pei2cab == 1)*/
                       )
                    {
                       if (occ_key_status)
                         connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                       //else
                         //connect_cop_passive_bcast_tx_while_othercall();
                    }
                    inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;
                    printf("< ON WAITING CAB-CALL STOP, GET IN START(STAT:0x%X) >\n", inout_bcast_processing);

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
            }
            else if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_STOP, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    clear_key_led(enum_COP_KEY_PA_IN);
                    set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                    indoor_pending = 0;

                    inout_bcast_processing &= ~INOUT_BUTTON_STATUS_IN_ON_MASK;
                    if (    !inout_bcast_processing
                         && (enable_in_pa_simultaneous_on_pei2cab == 1)
                       )
                        close_cop_multi_passive_tx();
                    printf("< ON WAITING CAB-CALL STOP, IN STOP(STAT:0x%X)\n", inout_bcast_processing);
                    enable_pei_wakeup_on_in_passive = 0;
                    enable_in_pa_simultaneous_on_pei2cab = 0;

                    // 2014/02/12
                    reset_func_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_auto_multi_rx_data_ptr(); //
                }
            }
            }

            if (outdoor_pending) {
            if (!(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK))
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_OUT);
                    set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif
                    outdoor_pending = 0;
                    outdoor_waiting = 0;

                    if (    !inout_bcast_processing
                         /*&& (enable_out_pa_simultaneous_on_pei2cab == 1)*/
                       ) {
                       if (occ_key_status)
                         connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                       //else
                         //connect_cop_passive_bcast_tx_while_othercall();
                    }
                    inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    printf("< ON WAITING CAB-CALL STOP, GET OUT START(STAT:0x%X) >\n", inout_bcast_processing);

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
            }
            else if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_STOP, 0);
                if (ret == 0) {
                    used_len = rxlen;

#if defined(INOUT_LED_ONOFF_BY_COP)
                    clear_key_led(enum_COP_KEY_PA_OUT);
                    set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                    outdoor_pending = 0;
                    inout_bcast_processing &= ~INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    if (    !inout_bcast_processing
                         && (enable_out_pa_simultaneous_on_pei2cab == 1)
                       ) {
                        close_cop_multi_passive_tx();
                    }

                    enable_pei_wakeup_on_out_passive = 0;
                    enable_out_pa_simultaneous_on_pei2cab = 0;

                    printf("< ON WAITING CAB-CALL STOP, OUT STOP(STAT:0x%X) >\n", inout_bcast_processing);

                    // 2014/02/12
                    reset_func_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_auto_multi_rx_data_ptr(); //
                }
            }
            }
            /* CAB-PA on __PEI2CAB_CALL_START *******************************************/
        }

#if 0 /* 2013/11/07 */
        if (cab2cab_call_running) {
            if (inout_bcast_processing)
                cab2cab_call_rx_tx_running(1);
            else
                cab2cab_call_rx_tx_running(0);
        }
#endif

        force_call_menu_draw();

        if (inout_bcast_processing)
            __discard_cop_pa_and_call_mic_encoding_data();

      /**** WAITING_CAB2CAB_CALL_STOP ###################################### */
        break;

/*
__CAB2CAB_CALL_STOP*/
      case __CAB2CAB_CALL_STOP:
#if 0
	if (cab2cab_call_running == 1) {
            //ret = get_status_key_blink(enum_COP_KEY_CALL_CAB);
            //if (ret == 0) {
                //clear_key_led(enum_COP_KEY_CALL_CAB);
	        cab2cab_call_pending = 0;
	        cab2cab_call_running = 0;
                clear_rx_tx_mon_multi_addr();
            //}
        }
        else
#endif
        {
            cop_occ_pa_and_occ_call_codec_stop();
            if (call_mon_sending) {
                call_mon_sending = 0;
                cop_monitoring_encoding_stop();
            }

            cop_speaker_volume_mute_set();
            occ_mic_volume_mute_set();
            decoding_stop();

			if(tractor_cab2cab_start) {
				/* Tractor input to Speaker */
				set_tractor_mic_audio_switch_on();
				set_occ_mic_to_spk_audio_switch_on();
				occ_mic_volume_set(occ_mic_vol_atten);
			}

            force_call_menu_draw();

            if (inout_bcast_processing) {
                if (   (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
                    && (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                    && indoor_pending == 0 && outdoor_pending == 0
                   )
                {
                    close_cop_multi_passive_tx();
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                    __broadcast_status = __PASSIVE_INOUTDOOR_BCAST_START;
                    indoor_pending = 1;
                    outdoor_pending = 1;
                    printf("\n< CAB-CALL+INOUT, CAB STOP -> INOUT START >\n");
                }
                else if (   (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
                     && indoor_pending == 0
                   )
                {
                    printf("\n< CAB-CALL+IN, CAB STOP -> IN START >\n");
                    close_cop_multi_passive_tx();
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                    //set_key_led_blink(enum_COP_KEY_PA_IN, 0);
                    __broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                    indoor_pending = 1;

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
                else if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                     && outdoor_pending == 0
                   )
                {
                    close_cop_multi_passive_tx();
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
                    printf("\n< CAB-CALL+OUT, CAB STOP -> OUT START >\n");
                    //set_key_led_blink(enum_COP_KEY_PA_IN, 0);
                    __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                    outdoor_pending = 1;

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
                break;
            }

            __discard_cop_pa_and_call_mic_encoding_data();
            __discard_occ_pa_and_call_mic_encoding_data();

            /*
            if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
                __broadcast_status = COP_STATUS_IDLE;
                printf("< CAB-CALL STOP -> IDLE(#1) >\n");
            }
            else if (is_waiting_normal_call(CAB_LIST, -1, -1)) {
                __broadcast_status = COP_STATUS_IDLE;
                printf("< CAB-CALL STOP -> IDLE(#2) >\n");
            }
            else*/ if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, &__mon_addr, NULL, 0)) {
                ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
                if (ret == 0) {
                    decoding_start();
                    pei2cab_call_monitoring = 1;
                    __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                    printf("< CAB-CALL STOP -> PEI MON START(%d-%d) >\n", __car_num, __dev_num);
                    ret = get_call_is_emergency();
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();
                }
            }
            else if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, &__mon_addr, &__tx_addr, 0)) {
                ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
                if (ret == 0) {
                    decoding_start();
                    cab2cab_call_monitoring = 1;
                    __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                    printf("< CAB-CALL STOP -> CAB MON START(%d-%d) >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();
                }
            }
            else {
                __broadcast_status = COP_STATUS_IDLE;
                printf("< CAB-CALL STOP -> IDLE(#3) >\n");
				if (fire_alarm_key_status)
					lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
            }
        }
      /*** __CAB2CAB_CALL_STOP ########################################### */
        break;

/*
__CAB2CAB_CALL_WAKEUP*/
      case __CAB2CAB_CALL_WAKEUP:
#if 0 //#ifdef NOT_USE_CAB_CALL_WAKEUP
        printf("< BUG, NOT USE CAB WAKEUP >\n");
#else
        if (cab2cab_call_waiting == 1 && rxlen && buf && rxlen >= (int)buf[0]) {
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        stop_play_beep();
                        cop_speaker_volume_mute_set();
                        occ_mic_volume_mute_set();
                        decoding_stop();

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        cab2cab_call_monitoring = 0;
                        close_cop_multi_monitoring();
                        clear_rx_tx_mon_multi_addr();

                        force_call_menu_draw();

                        cab2cab_call_waiting = 0;
						
						if(tractor_cab2cab_start) {
							/* Tractor input to Speaker */
							set_tractor_mic_audio_switch_on();
							set_occ_mic_to_spk_audio_switch_on();
							occ_mic_volume_set(occ_mic_vol_atten);
						}
                    }
                }
                break;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;

                    if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
                        __mon_addr = get_mon_mult_addr(buf, rxlen);
                        __tx_addr = get_tx_mult_addr(buf, rxlen);
                        add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                        add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                        __broadcast_status = COP_STATUS_IDLE;
                        printf("< CAB WAKEUP -> IDLE, BY PEI WAKEUP >\n");
                    }
                    else {
                        __mon_addr = get_mon_mult_addr(buf, rxlen);
                        __tx_addr = get_tx_mult_addr(buf, rxlen);
                        add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                        if (!is_waiting_normal_call(CAB_LIST, __car_num, __dev_num)) {

                            stop_play_beep();
                            cop_speaker_volume_mute_set();
                            decoding_stop();

							set_cop_spk_audio_switch_on(); // <<===

                            decoding_start();
                            cop_speaker_volume_set();

                            connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

                            cab2cab_call_monitoring = 1;
                            __broadcast_status = __CAB2CAB_CALL_MONITORING_START;
                            printf("< ON CAB WAKEUP, ADD CAB MON -> CAB MON START(%d-%d) >\n", __car_num, __dev_num);
                        }
                        else {
                            add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                            printf("< ON CAB WAKEUP, ADD CAB WAIT-MON(%d-%d) >\n", __car_num, __dev_num);
                            /*move_wakeup_to_1th_index(CAB_LIST);*/
                            __broadcast_status = COP_STATUS_IDLE;
                        }
                    }

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                }
                break;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON CAB WAKEUP, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                printf("< ON CAB WAKEUP, ADD CAB WAKEUP(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                /*move_wakeup_to_1th_index(CAB_LIST);*/

                force_call_menu_draw();
                break;
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_STOP, 0);
            if (ret == -1)
                 ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;

                    printf("< CAB WAKEUP -> GOT CAB STOP(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {

                        cop_speaker_volume_mute_set();
                        stop_play_beep();

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();

                        cab2cab_call_waiting = 0;
                        decoding_stop();
                    }
                }

                // 2014/02/13
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //

                break;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;

                stop_play_beep();
                cop_speaker_volume_mute_set();
                decoding_stop();

                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("< CAB WAKEUP, ADD %s PEI WAKEUP(%d-%d) -> IDLE BY PEI WAKEUP >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

                /* move_wakeup_to_1th_index(PEI_LIST); */

				force_call_menu_draw();
                __broadcast_status = COP_STATUS_IDLE;

                break;
            }
            }

#if 0 // delete, Ver0.78
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;

                cop_speaker_volume_mute_set();
                stop_play_beep();
                decoding_stop();

                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                printf("\n< ON CAB-CALL WAKEUP, ADD EMER PEI %d-%d >\n", __car_num, __dev_num);
                ret = add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 1, 1, __car_num, __dev_num, 0, 0);

                move_only_1th_wakeup_to_1th_index(PEI_LIST, 1);

                emergency_pei2cab_start = 1;
                ret = pei2cab_call_start(1);

                break;
            }
            }

            if (ret == -1 && emergency_pei2cab_start == 1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_START, 0);
            if (ret == 0) {

                used_len = rxlen;
                emergency_pei2cab_start = 0;
                cop_occ_pa_and_occ_call_codec_start();

                waiting_cab_stop_cmd_on_waiting_cab2cab_on_emer_pei = 0;

                cop_monitoring_encoding_start();
                connect_cop_cab2cab_call_multicast(buf, rxlen);
                ret = connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);

                printf("\n< CAB-CALL WAKEUP -> EMER PEI START(EMER) (%d-%d) >\n", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_LIVE, 0, 1, __car_num, __dev_num, __mon_addr, 0);

                pei2cab_call_pending = 1;
                pei2cab_call_running = 1;
                pei2cab_is_emergency = 1;
                __broadcast_status = __PEI2CAB_CALL_START;

                enable_pei_wakeup_on_in_passive = 1;

                force_call_menu_draw();

                call_start_prepare();

                pei_led_on_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                break;
            }
            }
#endif

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                /*if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num))*/
                if (is_waiting_normal_or_emer_call_any(PEI_LIST, __car_num, __dev_num))
                {
                    used_len = rxlen;

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    cop_speaker_volume_mute_set();
                    stop_play_beep();
                    occ_mic_volume_mute_set();
                    decoding_stop();

                    decoding_start();
                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();

                    pei2cab_call_waiting = 0;

                    ret = connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);

                    printf("< ON CAB WAKEUP & PEI WAKEUP -> %s PEI MON  %d-%d >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    ret = get_call_is_emergency();
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    pei2cab_call_monitoring = 1;
                    __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                    force_call_menu_draw();
                }
                break;
            }
            }

            if (   !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
                 && indoor_pending == 1
               )
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;
                    cop_speaker_volume_mute_set();
                    stop_play_beep();
                    occ_mic_volume_mute_set();

                    decoding_stop();

                    indoor_pending = 1;
                    indoor_waiting = 0;

                    inout_bcast_processing |= INOUT_BUTTON_STATUS_IN_ON_MASK;
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_IN);
                    set_key_led_blink(enum_COP_KEY_PA_IN, 0);
#endif
                    __broadcast_status = __PASSIVE_INDOOR_BCAST_START;
                    printf("< CAB WAKEUP -> IN START(STAT:0x%X) >\n", inout_bcast_processing);

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
            }

            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                 && outdoor_pending == 1
               )
            {
                ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
                if (ret == 0) {
                    used_len = rxlen;
                    cop_speaker_volume_mute_set();
                    stop_play_beep();
                    occ_mic_volume_mute_set();

                    outdoor_pending = 1;
                    outdoor_waiting = 0;
                    inout_bcast_processing |= INOUT_BUTTON_STATUS_OUT_ON_MASK;
                    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);

#if defined(INOUT_LED_ONOFF_BY_COP)
                    set_key_led(enum_COP_KEY_PA_OUT);
                    set_key_led_blink(enum_COP_KEY_PA_OUT, 0);
#endif

                    __broadcast_status = __PASSIVE_OUTDOOR_BCAST_START;
                    printf("< CAB WAKEUP -> OUT START(STAT:0x%X) >\n", inout_bcast_processing);

                    reset_cop_pa_multi_rx_data_ptr(); // 2014/02/19
                }
            }
        }
        else if (cab2cab_call_waiting == 0) {
            //ret = get_status_key_blink(enum_COP_KEY_CALL_CAB);
            //if (ret == 0) {
                //clear_key_led(enum_COP_KEY_CALL_CAB);
                __broadcast_status = COP_STATUS_IDLE;
                printf("< CAB WAKEUP -> IDLE >\n");
            //}
        }
        else if (cab2cab_call_waiting == 1 && rxlen == 0) {
            if (emergency_pei2cab_start == 0)
	        running_play_beep();

            if_call_selected_call_menu_draw();
        }
#endif

        break;
      /** __CAB2CAB_CALL_WAKEUP ############################################## */
/*
__CAB2CAB_CALL_MONITORING_START */
      case __CAB2CAB_CALL_MONITORING_START:
        if (cab2cab_call_monitoring == 1 && rxlen && buf && rxlen >= (int)buf[0]) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                stat = is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0); // *
                stat2 = is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1); // *
            }

            if (stat || stat2) {
                used_len = rxlen;
                printf("\n< ON CAB MON, DEL MON-CAB(%d-%d) >\n", __car_num, __dev_num);
                stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                if (stat != 0)
                    break;
                used_len = rxlen;

                play_stop_call_mon_rx_data();

                cab2cab_call_monitoring = 0;

                occ_mic_volume_mute_set();
                close_cop_multi_monitoring();
                clear_rx_tx_mon_multi_addr();
                decoding_stop();

                force_call_menu_draw();

                stat2 = 0;
                __car_num = get_call_car_num(PEI_LIST);
                __dev_num = get_call_id_num(PEI_LIST);
				__train_num = get_call_train_num(PEI_LIST);

                stat = is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, &__mon_addr, NULL, 0);
                if (stat) {
                    __mon_addr = get_call_mon_addr(PEI_LIST);

                    printf("< CAB MON -> PEI MON(%d-%d) >\n\n", __car_num, __dev_num);

                    connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
                    __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                    ret = get_call_is_emergency();
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    cab2cab_call_monitoring = 0;
                    pei2cab_call_monitoring = 1;

                    decoding_start();
                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();
                    break;
                }

                stat2 = 0;
                __car_num = get_call_car_num(CAB_LIST);
                __dev_num = get_call_id_num(CAB_LIST);
				__train_num = get_call_train_num(CAB_LIST);
                stat = is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0);
                if (stat == 0)
                    stat2 = is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, &__mon_addr, NULL, 0);

                if (stat || stat2) {
                    if (stat)
                        __mon_addr = get_call_mon_addr(CAB_LIST);

                    printf("\n< ON CAB MON, RECONNECT CAB MON(%d-%d) >\n", __car_num, __dev_num);

                    ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);
                    __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    cab2cab_call_monitoring = 1;

                    decoding_start();
                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();

                    break;
                }

                __broadcast_status = COP_STATUS_IDLE;
				if(fire_alarm_key_status)
					lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
                printf("< CAB MON -> IDLE >\n");
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1 // Ver0.76, to get cab mon directly for push to talk CAB-CAB CALL and no alarm
                set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);
                printf("< ON CAB MON, FORCE ADD CAB(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) { // *
                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("< ON CAB MON, ADD MON-CAB(%d-%d) >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    /*move_wakeup_to_1th_index(CAB_LIST);*/

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                } else {
                    printf("\n< ON CAB MON, NO CAB WAIT LIST(%d-%d) >\n", __car_num, __dev_num);
                }
                used_len = rxlen;
            }
            }

            __discard_cop_pa_and_call_mic_encoding_data();
            __discard_occ_pa_and_call_mic_encoding_data();

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0);
            if (ret == 0) {
                used_len = rxlen;

                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("< ON CAB MON, ADD %s PEI(%d-%d) -> IDLE >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

				force_call_menu_draw();
            }
            }


            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num)) {
                    used_len = rxlen;

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    printf("< ON CAB MON, ADD %s PEI-MON(%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) {
                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("< ON CAB MON, DEL %s PEI MON(%d-%d) >\n",
                                is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0);
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON CAB MON, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;

				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                printf("< ON CAB MON, ADD CAB(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                /*move_wakeup_to_1th_index(CAB_LIST);*/

                force_call_menu_draw();
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON CAB-MON, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;

                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB

                        force_call_menu_draw();
                    }
                }
            }
            }
        }

        if (cab2cab_call_monitoring == 1) {
            rxbuf = try_read_call_monitoring_data(&len, 0, 1);
            running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0); // run
        }


        if_call_selected_call_menu_draw();

        __discard_cop_pa_and_call_mic_encoding_data();
        __discard_occ_pa_and_call_mic_encoding_data();

      /** __CAB2CAB_CALL_MONITORING_START ################################## */
        break;

/*
__PEI2CAB_CALL_MONITORING_START*/
      case __PEI2CAB_CALL_MONITORING_START:
        if (pei2cab_call_monitoring == 1 && rxlen && buf && rxlen >= (int)buf[0])
        {
#if 1 // Ver 0.78
            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);

				/*
                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else {
                    is_emer_call = 0;
                    printf("\n< ON PEI WAKEUP, CANNOT SUPPORT NORMAL PEI-CALL STOP(%d-%d) >\n",
                                __car_num, __dev_num);
                    break;
                }
				*/

                if (is_waiting_emergency_pei_call_any_2(__car_num, __dev_num)) {
                    stat = del_call_list(PEI_LIST, 1, 0, __car_num, __dev_num);
                    if (!stat) {
                        used_len = rxlen;

                        //cop_speaker_volume_mute_set();
                        //stop_play_beep();
                        //decoding_stop();

                        printf("\n< ON PEI MON, DEL WAIT EMER PEI(%d-%d) >\n", __car_num, __dev_num);

                        pei_led_off_if_no_wait_normal_pei_and_emer_pei_call(); // enum_COP_KEY_CALL_PEI

                        force_call_menu_draw();

                        //__broadcast_status = COP_STATUS_IDLE;
                    }
                }
                reset_func_multi_rx_data_ptr();
                reset_fire_multi_rx_data_ptr(); //
                reset_auto_multi_rx_data_ptr(); //
            }
            }
#endif
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_STOP, 0); // *

            if (ret == 0) {
                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                stat = is_monitoring_call(PEI_LIST, __car_num, __dev_num, 0); // *
                stat2 = is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, NULL, NULL, 1); // *
            }

            if (ret == 0 && (stat || stat2)) {
                printf("\n< ON PEI MON, DEL %s PEI-MON(%d-%d) >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);

                stat = del_call_list(PEI_LIST, is_emer_call, 0, __car_num, __dev_num);

                if (stat != 0)
                    break;

                play_stop_call_mon_rx_data();

                close_cop_multi_monitoring();
                clear_rx_tx_mon_multi_addr();
                decoding_stop();
		
				if(tractor_cab2cab_start) {
					/* Tractor input to Speaker */
					set_tractor_mic_audio_switch_on();
					set_occ_mic_to_spk_audio_switch_on();
					occ_mic_volume_set(occ_mic_vol_atten);
				}

                used_len = rxlen;
                pei2cab_call_monitoring = 0;

                force_call_menu_draw();

                stat2 = 0;
                __car_num = get_call_car_num(PEI_LIST);
                __dev_num = get_call_id_num(PEI_LIST);
				__train_num = get_call_train_num(PEI_LIST);

                stat = is_monitoring_call(PEI_LIST, __car_num, __dev_num, 0);
                if (stat == 0)
                    stat2= is_waiting_monitoring_call(PEI_LIST, &__car_num, &__dev_num, &__mon_addr, NULL, 0);

                if (stat || stat2) {
                    if (stat)
                        __mon_addr = get_call_mon_addr(PEI_LIST);

                    ret = get_call_is_emergency();
                    if (ret)
                        printf("\n< ON PEI MON, RECONNECT EMER-PEI MON(%d-%d) >\n", __car_num, __dev_num);
                    else
                        printf("\n< ON PEI MON, RECONNECT PEI MON(%d-%d) >\n", __car_num, __dev_num);

                    connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);

                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    pei2cab_call_monitoring = 1;

                    decoding_start();
                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();

                    __broadcast_status = __PEI2CAB_CALL_MONITORING_START;

                    break;
                }

                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, &__mon_addr, &__tx_addr, 0)) {
                    printf("\n< PEI MON -> CAB MON START(%d-%d) >\n", __car_num, __dev_num);
                    ret = connect_cop_multi_monitoring(NULL, 0, NULL, __mon_addr);

                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    pei2cab_call_monitoring = 0;
                    cab2cab_call_monitoring = 1;

                    decoding_start();
                    set_cop_spk_audio_switch_on();
                    cop_speaker_volume_set();

                    __broadcast_status = __CAB2CAB_CALL_MONITORING_START;

                    break;
                }

                __broadcast_status = COP_STATUS_IDLE;
                printf("< PEI MON -> IDLE >\n");

                //cancel_auto_func_monitoring(); // 2014/02/19
                // 2014/02/19
                reset_fire_multi_rx_data_ptr();
                reset_func_multi_rx_data_ptr();
                reset_auto_multi_rx_data_ptr();
            }

            if (ret == -1 && cab2cab_reject_waiting) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                printf("\n< ON PEI MON, GOT REJECT CAB >\n");
                stat = del_call_list(CAB_LIST, 0, 0, -1, -1);
                if (stat == 0) {
                    used_len = rxlen;
                    cab2cab_reject_waiting = 0;

                    cab_led_off_if_no_wait_cab_call_or_lef_blink_if_wait(); // enum_COP_KEY_CALL_CAB

                    force_call_menu_draw();
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_REJECT, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) {
                    printf("< ON PEI MON, GOT CAB REJECT(%d-%d) >\n", __car_num, __dev_num);
                    ret = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (ret == 0) {
                        used_len = rxlen;
                        cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                if (is_waiting_normal_call_any(PEI_LIST, __car_num, __dev_num)) { // *
                    used_len = rxlen;
                    __mon_addr = get_mon_mult_addr(buf, rxlen);

                    stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                    if (stat3 == 0) is_emer_call = 1;
                    else is_emer_call = 0;

                    printf("< ON PEI MON, ADD %s PEI MON(%d-%d) >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, is_emer_call, __car_num, __dev_num, __mon_addr, 0, __train_num);

                    /* move_wakeup_to_1th_index(PEI_LIST); */

                    pei_led_off_if_no_wait_pei_call(); // enum_COP_KEY_CALL_PEI

                    force_call_menu_draw();
                }
            }
            }

#if 0 // delete, Ver 0.78
#endif

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_START, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
#if 1
                set_key_led_blink(enum_COP_KEY_CALL_CAB, 1);
                printf("< ON PEI MON, ADD CAB(%d-%d) by CAB MON >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
                    play_stop_call_mon_rx_data();
                    pei2cab_call_monitoring = 0; // <===
                    close_cop_multi_monitoring();
                    clear_rx_tx_mon_multi_addr();
                    decoding_stop();

                    printf("< ON PEI MON, NO PEI-WAKEUP -> IDLE by CAB-MON >\n");
                    __broadcast_status = COP_STATUS_IDLE;
                }
#endif
                if (is_waiting_normal_call_any(CAB_LIST, __car_num, __dev_num)) { // *

                    __mon_addr = get_mon_mult_addr(buf, rxlen);
                    __tx_addr = get_tx_mult_addr(buf, rxlen);
                    printf("< ON PEI MON, ADD MON-CAB(%d-%d) >\n", __car_num, __dev_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

                    /*move_wakeup_to_1th_index(CAB_LIST);*/

                    force_call_menu_draw();

                    cab_led_off_if_no_wait_cab_call(); // enum_COP_KEY_CALL_CAB
                } else {
                    printf("\n< ON PEI MON, NO CAB WAIT LIST(%d-%d) >\n", __car_num, __dev_num);
                }
                used_len = rxlen;
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_MONITORING_STOP, 0);
            if (ret == 0) {
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
                if (is_waiting_monitoring_call(CAB_LIST, &__car_num, &__dev_num, NULL, NULL, 1)) { // *
                    printf("\n< ON PEI MON, DEL MON WAIT-CAB %d-%d >\n", __car_num, __dev_num);
                    stat = del_call_list(CAB_LIST, 0, 0, __car_num, __dev_num);
                    if (stat == 0) {
                        used_len = rxlen;
                        force_call_menu_draw();
                    }
                }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_WAKEUP, 0); // *
            if (ret == 0) {
                used_len = rxlen;
                set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);

                stat3 = check_avc2cop_cmd(buf, rxlen, __PEI2CAB_CALL_EMER_EN, 0);
                if (stat3 == 0) is_emer_call = 1;
                else is_emer_call = 0;

                printf("\n< ON PEI MON, ADD %s PEI WAKEUP(%d-%d) >\n",
                            is_emer_call ? "EMER" : "NORMAL", __car_num, __dev_num);
                add_call_list(PEI_LIST, CALL_STATUS_WAKEUP, 0, is_emer_call, __car_num, __dev_num, 0, 0, __train_num);

                /*move_wakeup_to_1th_index(PEI_LIST);*/

				force_call_menu_draw();
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_WAKEUP, 0); // *
#ifdef NOT_USE_CAB_CALL_WAKEUP
            if (ret == 0) {
                printf("< ON PEI MON, NOT USE CAB WAKEUP >\n");
                used_len = rxlen;
            }
#else
            if (ret == 0) {
                used_len = rxlen;
				set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
        		set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                printf("< ON PEI MON, ADD CAB(%d-%d) >\n", __car_num, __dev_num);
                add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                force_call_menu_draw();

                if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {

                    play_stop_call_mon_rx_data();

                    pei2cab_call_monitoring = 0; // <===

                    close_cop_multi_monitoring();
                    clear_rx_tx_mon_multi_addr();
                    decoding_stop();

                    printf("< ON PEI MON, NO PEI-WAKEUP -> IDLE >\n");
                    __broadcast_status = COP_STATUS_IDLE;
                }
            }
#endif
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_START, 0);
            if (ret == 0) {
#if 1
			  if(tractor_cab2cab_start) {
				  printf("< PEI MON, TRACTOR CALL EN >\n");
			  }
#endif
#if 0
              stat3 = check_avc2cop_cmd(buf, rxlen, __CAB2CAB_CALL_EMER_EN, 0);
              if (stat3 == 0) {
                  printf("< PEI MON, TRACTOR CALL EN >\n");
			  }
#endif

              __car_num = get_call_car_num(CAB_LIST);
              __dev_num = get_call_id_num(CAB_LIST);
              if (__car_num == (char)-1 && __dev_num == (char)-1) // *
              {
                used_len = rxlen;

                printf("\n< PEI MON -> CAB-CALL START (%d-%d) >\n", __car_num, __dev_num);

                play_stop_call_mon_rx_data();

                pei2cab_call_monitoring = 0;

                close_cop_multi_monitoring();
                clear_rx_tx_mon_multi_addr();
                decoding_stop();

                __car_num = get_call_car_num(PEI_LIST);
                __dev_num = get_call_id_num(PEI_LIST);
                __mon_addr = get_call_mon_addr(PEI_LIST);
				__train_num = get_call_train_num(PEI_LIST);

                ret = get_call_is_emergency();
                add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);

                cop_occ_pa_and_occ_call_codec_start();
                decoding_start();

                connect_cop_cab2cab_call_multicast(buf, rxlen);

                if (call_mon_sending) {
                    cop_monitoring_encoding_start();
                    connect_cop_multi_monitoring(buf, rxlen, &__mon_addr, 0);
                }

                __car_num = get_car_number(buf, rxlen);
                __dev_num = get_device_number(buf, rxlen);
				__train_num = get_train_number(buf, rxlen);
                add_call_list(CAB_LIST, CALL_STATUS_LIVE, 0, 0, __car_num, __dev_num, 0, 0, __train_num);

                cab_led_control_on_cab2cab_call(); // enum_COP_KEY_CALL_CAB

                force_call_menu_draw();

	        	cab2cab_call_pending = 1;
	        	cab2cab_call_running = 1;
                __broadcast_status = __CAB2CAB_CALL_START;

                CurCab_ListNum = get_call_waiting_ctr(CAB_LIST);

                call_start_prepare();
				set_tractor_spk_audio_switch_on();		// Audio Output to Tractor

                break;
              }
            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_INDOOR_BCAST_START, 0);
            if (ret == 0) used_len = rxlen;
            if (indoor_pending && ret == 0) {
                printf("< PEI MON -> GET IN START(STAT:0x%X) >\n", inout_bcast_processing);
                indoor_waiting = 0;

            }
            }

            if (ret == -1) {
            ret = check_avc2cop_cmd(buf, rxlen, __PASSIVE_OUTDOOR_BCAST_START, 0);
            if (ret == 0) used_len = rxlen;
            if (outdoor_pending && ret == 0) {
                outdoor_waiting = 0;
                printf("< PEI MON -> GET OUT START(STAT:0x%X) >\n", inout_bcast_processing);
            }
            }
        }

        if (pei2cab_call_monitoring == 1) {
            rxbuf = try_read_call_monitoring_data(&len, 0, 1);
            running_bcast_recv_and_dec(0, rxbuf, len, 1, 0, 0); // run
        }

        if_call_selected_call_menu_draw();

        if (inout_bcast_processing)
            __discard_cop_pa_and_call_mic_encoding_data();
      /*** __PEI2CAB_CALL_MONITORING_START ##################################*/
        break;

      default:
        printf("%s: Not defined Index: %d.\n", __func__, (int)__broadcast_status);
        break;
    }

    //key_off_process();

#if 0
    if (tcp_connected) {
        display_lcd_log_msg_process();
		if( (enable_fire_alarm) && (__broadcast_status == COP_STATUS_IDLE) ) {	// Added by Michael RYU(For supporting the Fire Alarm)
			fire_set_for_key();
			lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
			auto_monitoring_on = 1;
		}
	}
#endif

    if (tcp_connected) {
			passive_func_auto_fire_bcast_monitoring(__broadcast_status);
	}


#if 0
    if (used_len != rxlen)
        send_call_protocol_err_to_avc(buf, __broadcast_status);

    if (fire_alarm_is_use && enable_fire_alarm)
        running_play_fire_alarm();
#endif

    if (deadman_alarm_is_use && enable_deadman_alarm)
        running_play_deadman_alarm();

    return used_len;
}

static void passive_func_auto_fire_bcast_monitoring(COP_Cur_Status_t bcast_status)
{
    int occ_pa_use;
    int cop_pa_use;
    int func_pa_use;
    int fire_pa_use;
    int auto_pa_use;

    if (    (bcast_status == COP_STATUS_IDLE
        && !is_waiting_normal_call(PEI_LIST, -1, -1)
        && !is_waiting_emergency_pei_call()
        && !is_waiting_normal_call(CAB_LIST, -1, -1)
        && !emergency_pei2cab_start
        && !inout_bcast_processing)
       )
    {
        /* OCC PASSIVE Broadcast */

       	/*if (check_ssrc_set_enable) {
            check_ssrc_set_enable = 0;
            set_check_different_ssrc_valid(1);
        }
		*/

        if (occ_pa_bcast_monitor_start && !occ_pa_bcast_rx_running) {
            if (cop_pa_bcast_rx_running || fire_bcast_rx_running || func_bcast_rx_running || auto_bcast_rx_running) {
                if (cop_pa_bcast_rx_running) {
                    cop_pa_bcast_rx_running = 0;
                    cab_pa_bcast_monitor_start = 0; // <===
                    //InOut_LedStatus = INOUT_BUTTON_STATUS_ALLOFF;

                    printf("\n< CAB-PA MON STOP BY OCC-PA >\n");
                    play_stop_cop_pa_mon_rx_data(1, 1);

					clear_pa_mon_play_time();
                    reset_cop_pa_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr();
                    reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr();

                    clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
                } else if (fire_bcast_rx_running) {
					fire_bcast_rx_running = 0;
					fire_bcast_monitor_start = 0;

					printf("< FIRE MON STOP BY OCC-PA >\n");
					play_stop_fire_mon_rx_data(1, 1);

					clear_pa_mon_play_time();
					reset_fire_multi_rx_data_ptr();
					reset_func_multi_rx_data_ptr();
					reset_auto_multi_rx_data_ptr();

					clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
                } else if (func_bcast_rx_running) {
                    func_bcast_rx_running = 0;
                    func_bcast_monitor_start = 0;

                    printf("< FUNC MON STOP BY OCC-PA >\n");
                    play_stop_func_mon_rx_data(1, 1);

                    clear_pa_mon_play_time();
                    reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr();

                    clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
                } else if (auto_bcast_rx_running) {
                    auto_bcast_rx_running = 0;
                    auto_bcast_monitor_start  = 0;

                    printf("< AUTO MON STOP BY OCC-PA >\n");
                    play_stop_auto_mon_rx_data(1, 1);

                    clear_pa_mon_play_time();
                    reset_auto_multi_rx_data_ptr();

                    clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
				}

                decoding_stop();
            }

            printf("\n< OCC-PA MON START in IDLE >\n");
            occ_pa_bcast_rx_running = 1;

            set_cop_spk_audio_switch_on();
#if (MONITORING_VOL == COP_SELF_VOL)
            cop_speaker_volume_set();
#else
            /* Ver 0.73, 2014/03/06 */
            if (pamp_in_vol)
                cop_speaker_volume_set();
            else
                cop_speaker_volume_mute_set();
#endif

            decoding_start();

            reset_occ_pa_multi_rx_data_ptr(); // 2014/02/19
        }
        else if (!occ_pa_bcast_monitor_start && occ_pa_bcast_rx_running) {
            occ_pa_bcast_rx_running = 0;

            printf("\n< OCC-PA MON STOP in IDLE >\n");
            play_stop_occ_pa_mon_rx_data(1, 0);

            clear_pa_mon_play_time();

            reset_occ_pa_multi_rx_data_ptr();
            reset_cop_pa_multi_rx_data_ptr();
            reset_fire_multi_rx_data_ptr();
            reset_func_multi_rx_data_ptr();
            reset_auto_multi_rx_data_ptr();

            decoding_stop();
            clear_old_rtp_seq();
            //check_ssrc_set_enable = 1;
        }

        /* CAB PASSIVE Broadcast */
        if (    cab_pa_bcast_monitor_start &&
               !occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running
             /*&& (InOut_LedStatus != INOUT_BUTTON_STATUS_ALLOFF)*/
           )
        {
            if (fire_bcast_rx_running || func_bcast_rx_running || auto_bcast_rx_running) {
				if (fire_bcast_rx_running) {
					fire_bcast_rx_running = 0;
					fire_bcast_monitor_start = 0;
					printf("< FIRE MON STOP BY CAB-PA >\n");

					play_stop_fire_mon_rx_data(1, 1);

					clear_pa_mon_play_time();
					reset_fire_multi_rx_data_ptr();
					reset_func_multi_rx_data_ptr();
					reset_auto_multi_rx_data_ptr();

					clear_old_rtp_seq();
					//set_check_different_ssrc_valid(1);
				} else if (func_bcast_rx_running) {
                    func_bcast_rx_running = 0;
                    func_bcast_monitor_start = 0;
                    printf("< FUNC MON STOP BY CAB-PA >\n");

                    play_stop_func_mon_rx_data(1, 1);

                    clear_pa_mon_play_time();
					reset_fire_multi_rx_data_ptr();
                    reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr();

                    clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
				} else if (auto_bcast_rx_running) {
                    auto_bcast_rx_running = 0;
                    auto_bcast_monitor_start  = 0;
                    printf("< AUTO MON STOP BY CAB-PA >\n");

                    play_stop_auto_mon_rx_data(1, 1);

                    clear_pa_mon_play_time();
					reset_fire_multi_rx_data_ptr();
                    reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr();

                    clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
                }

                decoding_stop();
            }

            /* Ver 0.71, 2014/01/27 */
            /*if (check_ssrc_set_enable) {
                check_ssrc_set_enable = 0;
                //set_check_different_ssrc_valid(1);
            }*/

            printf("< CAB-PA MON START in IDLE >\n");
            cop_pa_bcast_rx_running = 1;

            set_cop_spk_audio_switch_on();

#if (MONITORING_VOL == COP_SELF_VOL)
            cop_speaker_volume_set();
#else
            /* Ver 0.73, 2014/03/06 */
            if (pamp_in_vol)
                cop_speaker_volume_set();
            else
                cop_speaker_volume_mute_set();
#endif

            decoding_start();

            reset_cop_pa_multi_rx_data_ptr();
        }
        else if (    (!cab_pa_bcast_monitor_start && cop_pa_bcast_rx_running)
             /*&& (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF)*/
           )
        {
            cop_pa_bcast_rx_running = 0;

            printf("\n< CAB-PA MON STOP in IDLE >\n");
            play_stop_cop_pa_mon_rx_data(1, 0);

            clear_pa_mon_play_time();
            reset_cop_pa_multi_rx_data_ptr();
            reset_fire_multi_rx_data_ptr();
            reset_func_multi_rx_data_ptr();
            reset_auto_multi_rx_data_ptr();

            decoding_stop();
            //close_monitoring_bcast_multicast(MON_BCAST_TYPE_COP_PASSIVE);

            clear_old_rtp_seq();
            //check_ssrc_set_enable = 1;
        }


        if (!occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running) {
            if (fire_bcast_monitor_start && !fire_bcast_rx_running) {
				if (func_bcast_rx_running) {
					func_bcast_rx_running = 0;

					printf("< FUNC MON STOP BY FIRE >\n");
					play_stop_func_mon_rx_data(1, 1);

					clear_pa_mon_play_time();
					reset_func_multi_rx_data_ptr();
					reset_auto_multi_rx_data_ptr();

					decoding_stop();
					//close_monitoring_bcast_multicast(MON_BCAST_TYPE_FUNC);

					func_bcast_monitor_start  = 0;

					clear_old_rtp_seq();
					//set_check_different_ssrc_valid(1);
				} else if (auto_bcast_rx_running) {
                    auto_bcast_rx_running = 0;

                    printf("< AUTO MON STOP BY FIRE >\n");
                    play_stop_auto_mon_rx_data(1, 1);

                    clear_pa_mon_play_time();
                    reset_auto_multi_rx_data_ptr();

                    decoding_stop();
                    //close_monitoring_bcast_multicast(MON_BCAST_TYPE_AUTO);

                    auto_bcast_monitor_start  = 0;

                    clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
                }

                fire_bcast_rx_running = 1;
                avc_cycle_sync = 0;

                printf("\n< FIRE MON START in IDLE >\n");
                set_cop_spk_audio_switch_on();

#if 1
                fire_cop_speaker_volume_set();
#else
                /* Ver 0.73, 2014/03/06 */
                if (pamp_in_vol)
                    cop_speaker_volume_set();
                else
                    cop_speaker_volume_mute_set();
#endif

                decoding_start();
            }
            else if (!fire_bcast_monitor_start && fire_bcast_rx_running) {
                fire_bcast_rx_running = 0;

                printf("\n< FIRE MON STOP in IDLE >\n");
                play_stop_fire_mon_rx_data(1, 0);

                clear_pa_mon_play_time();
                //reset_fire_multi_rx_data_ptr();
                reset_func_multi_rx_start_get_buf();//<---
                reset_auto_multi_rx_data_ptr();

                //close_monitoring_bcast_multicast(MON_BCAST_TYPE_FIRE);
                decoding_stop();
                clear_old_rtp_seq();
                //check_ssrc_set_enable = 1;
            }
        }

        if (!occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running && !fire_bcast_monitor_start)
        {
			if (func_bcast_monitor_start && !func_bcast_rx_running) {
				if (auto_bcast_rx_running) {
                    auto_bcast_rx_running = 0;

                    printf("< AUTO MON STOP BY FUNC >\n");
                    play_stop_auto_mon_rx_data(1, 1);

                    clear_pa_mon_play_time();
                    reset_auto_multi_rx_data_ptr();

                    decoding_stop();
                    //close_monitoring_bcast_multicast(MON_BCAST_TYPE_AUTO);

                    auto_bcast_monitor_start  = 0;

                    clear_old_rtp_seq();
                    //set_check_different_ssrc_valid(1);
				}
				func_bcast_rx_running = 1;
				avc_cycle_sync = 0;

				printf("\n<FUNC MON START in IDLE >\n");
				set_cop_spk_audio_switch_on();

#if (MONITORING_VOL == COP_SELF_VOL)
				cop_speaker_volume_set();
#else
				/* Ver 0.73, 2014/03/06 */
				if (pamp_in_vol)
					cop_speaker_volume_set();
				else
					cop_speaker_volume_mute_set();
#endif
				decoding_start();
			} else if (!func_bcast_monitor_start && func_bcast_rx_running) {
				func_bcast_rx_running = 0;

				printf("\n< FUNC MON STOP in IDLE >\n");
				play_stop_func_mon_rx_data(1, 0);

				clear_pa_mon_play_time();
				//reset_fire_multi_rx_data_ptr();
				reset_func_multi_rx_start_get_buf();//<---
				reset_auto_multi_rx_data_ptr();

				//close_monitoring_bcast_multicast(MON_BCAST_TYPE_FUNC);
				decoding_stop();
				clear_old_rtp_seq();
				//check_ssrc_set_enable = 1;
			}
		}

        if (!occ_pa_bcast_rx_running && !cop_pa_bcast_rx_running && !fire_bcast_monitor_start && !func_bcast_monitor_start) {
            if (auto_bcast_monitor_start && !auto_bcast_rx_running) {

                /*if (check_ssrc_set_enable) {
                    check_ssrc_set_enable = 0;
                    set_check_different_ssrc_valid(1);
                }*/

                auto_bcast_rx_running = 1;
                avc_cycle_sync = 0;

                set_cop_spk_audio_switch_on();

                if (pamp_in_vol)
                    cop_speaker_volume_set();
                else
                    cop_speaker_volume_mute_set();

                printf("< AUTO MON START in IDLE >\n");

                decoding_start();

                //if (check_ssrc_set_enable) {
                //    check_ssrc_set_enable = 0;
                //    set_check_different_ssrc_valid(1);
                //}
            }
            else if (!auto_bcast_monitor_start && auto_bcast_rx_running) {
                auto_bcast_rx_running = 0;

                printf("< AUTO MON STOP in IDLE >\n");
                play_stop_auto_mon_rx_data(1, 0);

                clear_pa_mon_play_time();
                //reset_auto_multi_rx_data_ptr();
                reset_auto_multi_rx_start_get_buf();//<<---

                decoding_stop();
                //close_monitoring_bcast_multicast(MON_BCAST_TYPE_AUTO);

                clear_old_rtp_seq();
                //check_ssrc_set_enable = 1;
            }
        }

    } else {
        if (   occ_pa_bcast_rx_running || cop_pa_bcast_rx_running
            || fire_bcast_rx_running || func_bcast_rx_running || auto_bcast_rx_running)
        {
            /*if (   !is_waiting_normal_call(PEI_LIST, -1, -1)
                && !is_waiting_normal_call(CAB_LIST, -1, -1)
                && !is_waiting_emergency_pei_call()
                && !get_call_live_num(PEI_LIST)
                && !get_call_live_num(CAB_LIST)
                && !is_monitoring_call(PEI_LIST, 0, 0, 1)
                && !is_monitoring_call(CAB_LIST, 0, 0, 1)
               )*/
            {
                //cop_speaker_volume_mute_set(); // <=== // delete 2014/02/12

                if (occ_pa_bcast_rx_running) {
                    play_stop_occ_pa_mon_rx_data(0, 1);
                    clear_pa_mon_play_time(); //
                    reset_occ_pa_multi_rx_data_ptr();
                    reset_cop_pa_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr(); //
                    occ_pa_bcast_rx_running = 0;
					DBG_LOG("\n");
					//if(!occ_pa_bcast_monitor_start)
                    //	occ_pa_bcast_monitor_start = 0; // 2014/02/11
                    clear_old_rtp_seq();//
                }
                else if (cop_pa_bcast_rx_running) {
                    play_stop_cop_pa_mon_rx_data(0, 1);
                    clear_pa_mon_play_time(); //
                    reset_cop_pa_multi_rx_data_ptr();
                    reset_fire_multi_rx_data_ptr(); //
                    reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr(); //
                    cop_pa_bcast_rx_running = 0;
					DBG_LOG("\n");
                   	//occ_pa_bcast_monitor_start = 0; // 2014/02/11
                    clear_old_rtp_seq();//
                }
                else if (fire_bcast_rx_running) {
					play_stop_fire_mon_rx_data(0, 1);
					clear_pa_mon_play_time();
					reset_fire_multi_rx_data_ptr();
					reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr();
					fire_bcast_rx_running = 0;
					fire_bcast_monitor_start = 0;
					clear_old_rtp_seq();
				}
                else if (func_bcast_rx_running) {
                    play_stop_func_mon_rx_data(0, 1);
                    clear_pa_mon_play_time(); //
                    reset_func_multi_rx_data_ptr();
                    reset_auto_multi_rx_data_ptr(); //
                    func_bcast_rx_running = 0;
                    func_bcast_monitor_start = 0; // 2014/02/11
                    clear_old_rtp_seq();//
                }
                else if (auto_bcast_rx_running) {
                    play_stop_auto_mon_rx_data(0, 1);
                    clear_pa_mon_play_time(); //
                    reset_auto_multi_rx_data_ptr();
                    auto_bcast_rx_running = 0;
                    auto_bcast_monitor_start = 0; // 2014/02/11
                    clear_old_rtp_seq();//
                }
                //cop_speaker_volume_set(); // <=== // delete 2014/02/12

                printf("< STOP PA MON(%d) >\n", __broadcast_status);
                decoding_stop();

                //check_ssrc_set_enable = 1;
                set_check_different_ssrc_valid(0);

            }
        }
    }

	//DBG_LOG("occ_pa_bcast_rx_running(%d), cop_pa_bcast_rx_running(%d), fire_bcast_rx_running(%d), func_bcast_rx_running(%d), auto_bcast_rx_running(%d)\n",
	//		 occ_pa_bcast_rx_running,	  cop_pa_bcast_rx_running,     fire_bcast_rx_running,     func_bcast_rx_running,     auto_bcast_rx_running);

    if (occ_pa_bcast_rx_running) {
		occ_pa_use  = 1;
        cop_pa_use  = 0;
		fire_pa_use = 0;
        func_pa_use = 0;
        auto_pa_use = 0;
    } else if (cop_pa_bcast_rx_running) {
		occ_pa_use  = 1;
		cop_pa_use  = 1;
		fire_pa_use = 0;
        func_pa_use = 0;
        auto_pa_use = 0;
    } else if (fire_bcast_rx_running) {
		occ_pa_use  = 1;
		cop_pa_use  = 1;
		fire_pa_use = 1;
		func_pa_use = 0;
		auto_pa_use = 0;
    } else if (func_bcast_rx_running) {
		occ_pa_use  = 1;
		cop_pa_use  = 1;
		fire_pa_use = 1;
		func_pa_use = 1;
        auto_pa_use = 0;
	} else if (auto_bcast_rx_running) {
		occ_pa_use  = 1;
		cop_pa_use  = 1;
		fire_pa_use = 1;
		func_pa_use = 1;
        auto_pa_use = 1;
    }

    if (pei2cab_call_running || cab2cab_call_running) {
        occ_pa_use = 0;
        cop_pa_use = 0;
		fire_pa_use = 0;
        func_pa_use = 0;
        auto_pa_use = 0;
    }

    if (!remote_occ_pa_monitor_start) /* Ver 0.71, 2014/01/08 */
        occ_pa_monitoring_bcast_rx_dec_running(occ_pa_bcast_rx_running, occ_pa_use);
    cop_pa_monitoring_bcast_rx_dec_running(cop_pa_bcast_rx_running, cop_pa_use);
    fire_monitoring_bcast_rx_dec_running(fire_bcast_rx_running, fire_pa_use);
    func_monitoring_bcast_rx_dec_running(func_bcast_rx_running, func_pa_use);
    auto_monitoring_bcast_rx_dec_running(auto_bcast_rx_running, auto_pa_use);
}

static void display_lcd_log_msg_process(void)
{
#if 0
    static int busy_delay_start = 0;

    if  (   busy_delay_start == 0
         && (LcdLogIndex >= BUSY_LOG_START_IDX)
         && (LcdLogIndex <= BUSY_LOG_END_IDX)
        )
    {
		//DBG_LOG("LcdLogIndex = %d\n", LcdLogIndex);
        busy_delay_start = 1;
        time(&log_old_time);
    }
    else if  (   busy_delay_start == 1
         && (LcdLogIndex >= BUSY_LOG_START_IDX)
         && (LcdLogIndex <= BUSY_LOG_END_IDX)
        )
    {
		//DBG_LOG("LcdLogIndex = %d\n", LcdLogIndex);
        time(&log_cur_time);
        if ((log_cur_time - log_old_time) >=  LOG_MESSAGE_DISPLAY_TIME) {
           	busy_delay_start = 0;
           	LcdLogIndex = LOG_MSG_STR_READY;
        }
    }
    log_msg_draw(LcdLogIndex);
#endif
}

static int __none_key_process (enum __cop_key_enum keyval, int tcp_connected)
{
    int ret = 0;
	printf("__none_key_process()\n");

    return ret;
}

int __pa_function_force_stop (int func_num, unsigned char selected, unsigned char is_station_fpa)
{
    int ret;

	if(!func_key_lock)
		func_key_lock = 1;

	if(Func_Is_Now_Start != -1) {
		ret = send_cop2avc_cmd_func_bcast_stop_request(0, selected, func_num, is_station_fpa);
	}

	func_key_lock = 0;

    return ret;
}

int __pa_function_process (int func_num, unsigned char selected, unsigned char is_station_fpa)
{
    int ret;

	if(!func_key_lock)
		func_key_lock = 1;

    if (Func_Is_Now_Start != -1) {
		//ret = send_cop2avc_cmd_func_bcast_stop_request(0, selected, Func_Is_Now_Start, is_station_fpa);
		printf("Func bcast start request again in func pa bcasting\n");
    }
	ret = send_cop2avc_cmd_func_bcast_start_request(0, selected, func_num, is_station_fpa);
	// Not implemented yet. But, need to this at future.
	//else {
	//	Func_Is_Now_Start = 0;
	//	ret = send_cop2avc_cmd_func_bcast_stop_request(0, 0, func_num);
	//}

    if (is_bcast_menu_selected()) {
        broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0, FuncNum, 0, bmenu_active, Func_Is_Now_Start);
	    //DBG_LOG("\n");
	} else if (!get_call_any_ctr(PEI_LIST) && !get_call_any_ctr(CAB_LIST)) {
        menu_bcast_screen_reset();
        menu_head_draw(global_menu_active, bmenu_active, 0, 0, 0, 0, 0, 0);
        broadcast_draw_menu(InOut_LedStatus, inout_bcast_processing, 0, FuncNum, 1, bmenu_active, Func_Is_Now_Start);
	    // DBG_LOG("\n");
    }

	func_key_lock = 0;
    return ret;
}

int send_broadcast_stop_request(void)
{
	int ret;

	ret = send_cop2avc_cmd_broadcast_stop_request(0);

	return ret;
}

static int is_call_menu_selected(void)
{
    int ret = (global_menu_active & COP_MENU_MASK) == COP_MENU_CALL;
    if (ret)
        call_menu_active = 1;
     else
        call_menu_active = 0;

    return ret;
}

static int is_bcast_menu_selected(void)
{
    int ret = (global_menu_active & COP_MENU_MASK) == COP_MENU_BROADCAST;

    if (ret == 0)
        call_menu_active = 0;

    return ret;
}

static int __pa_inout_key_process (enum __cop_key_enum keyval, int tcp_connected)
{
	/*
    if( trial_bcast_start ) {
        printf("PA IN+OUT Rejected by COP(Trial Broadcast)\n");
        return 0;
    }
	*/

    if( tractor_cab2cab_start ) {
        printf("PA IN+OUT Rejected by COP(TRACTOR)\n");
        return 0;
    }

    Func_Cop_Key_Process_List[enum_COP_KEY_PA_IN](keyval, tcp_connected);
    inoutdoor_pending = 1;
    backup_keyval = keyval;

    //DBG_LOG("backup_keyval = 0x%X:%d\n", backup_keyval, backup_keyval);

    return 0;
}

int __pa_in_key_process (int key_pressed_status)
{
    int ret = 0;
    char __msg_str[32];
    int is_pushed_key = key_pressed_status & KEY_PRESSED_MASK;

	pa_in_key_lock = 1;

	if(pa_in_Key_pressed_on_fire_status)
		pa_in_Key_pressed_on_fire_status = 0;
	else
		pa_in_Key_pressed_on_fire_status = 1;

    if (remote_occ_pa_monitor_start) { /* Ver 0.71, 2014/01/08 */
		Log_Status = LOG_OCC_PA_BUSY;
		DBG_LOG("\n");
		pa_in_key_lock = 0;
        return 0;
    }

    __msg_str[0] = 0;

	DBG_LOG("%s(), __broadcast_status: %d, is_pushed_key(%d)\n", __func__, __broadcast_status, is_pushed_key);
    switch(__broadcast_status) {
      case COP_STATUS_IDLE:
      case WAITING_PASSIVE_INDOOR_BCAST_START:
      case WAITING_PASSIVE_OUTDOOR_BCAST_START:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED IN KEY on IDLE >\n");
            break;
        }

        if (indoor_waiting == 1) {
            printf("< IN KEY, ON WAITING IN START >\n");
            break;
        }

        if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) {
            printf("<< IN KEY, Already RUN INDOOR > >\n");
            break;
        }

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF && !occ_pa_bcast_monitor_start)
        {
#if defined(INOUT_LED_ONOFF_BY_COP)
            set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif

            ret = send_cop2avc_cmd_in_door_start_request(0);
            if (ret > 0) {
                printf("< IN KEY -> WAITING IN START >\n");
                indoor_pending = 1;
                indoor_waiting = 1;
                __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;
            }
        }
        else if (InOut_LedStatus != INOUT_BUTTON_STATUS_ALLOFF && !occ_pa_bcast_monitor_start)
			Log_Status = LOG_CAB_PA_BUSY;
        else if (occ_pa_bcast_monitor_start)
			Log_Status = LOG_OCC_PA_BUSY;
        break;

      case __PASSIVE_INDOOR_BCAST_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED IN KEY on INDOOR >\n");
            break;
        }

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
            if(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
            {
#if defined(INOUT_LED_ONOFF_BY_COP)
               set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
               ret = send_cop2avc_cmd_in_door_stop_request(__PASSIVE_INDOOR_BCAST_START);
               if (ret > 0) {
                   printf("< IN KEY, IN START -> WAITING STOP(STAT:0x%X) >\n", inout_bcast_processing);
                   __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_STOP;
               }
            }
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case _WAITING_OUTSTOP_PASSIVE_INOUTDOOR_BCAST_START:
        if (indoor_stop_pending == 1) {
            printf("< IN KEY, ON INOUT BCAST, WAITING IN-STOP >\n");
            break;
        }

        if (is_pushed_key) {
            printf("< IGNORE PUSHED IN KEY on WAIT OUTSTOP in INOUT >\n");
            break;
        }

#if defined(INOUT_LED_ONOFF_BY_COP)
        set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
        send_cop2avc_cmd_in_door_stop_request(__PASSIVE_INOUTDOOR_BCAST_START);
        printf("< IN KEY, ON WAIT OUTSTOP in INOUT START, SEND IN-STOP REQ > \n");

        indoor_stop_pending = 1;
        break;

      case WAITING_PASSIVE_INDOOR_BCAST_STOP:
        printf("< IN KEY, ON WAITING IN-STOP >\n");
        break;

      case _WAITING_INSTOP_PASSIVE_INOUTDOOR_BCAST_START:
        printf("< IN KEY, ON INOUT START, WAITING IN-STOP > \n");
        break;

      case WAITING_INSTART_PASSIVE_INOUTDOOR_BCAST_START:
        printf("< IN KEY, ON OUT START, WAITING IN-START >\n");
        break;

      case WAITING_OUTSTART_PASSIVE_INOUTDOOR_BCAST_START:
        printf("< IN KEY, ON IN START, WAITING OUT-START >\n");
        break;

      case __PASSIVE_OUTDOOR_BCAST_START:
        if (indoor_waiting == 1) {
            printf("< IN KEY, ON WAITING IN-START(%d) >\n", __broadcast_status);
            break;
        }

        if (!is_pushed_key) {
            printf("< IGNORE RELEASED IN KEY on OUTDOOR >\n");
            break;
        }

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
            if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
               )
            {
#if defined(INOUT_LED_ONOFF_BY_COP)
               set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
               send_cop2avc_cmd_in_door_start_request(__PASSIVE_OUTDOOR_BCAST_START);
               indoor_waiting = 1;
               printf("< IN KEY, OUT START -> WAITING IN-START ON INOUT(STAT:0x%X) >\n", inout_bcast_processing);
               __broadcast_status = WAITING_INSTART_PASSIVE_INOUTDOOR_BCAST_START;
            }
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        break;

      case __PASSIVE_INOUTDOOR_BCAST_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED IN KEY on INOUT >\n");
            break;
        }

#if defined(INOUT_LED_ONOFF_BY_COP)
        set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
        send_cop2avc_cmd_in_door_stop_request(__PASSIVE_INOUTDOOR_BCAST_START);
        printf("< IN KEY, INOUT START -> WAITING IN-STOP > \n");
        __broadcast_status = _WAITING_INSTOP_PASSIVE_INOUTDOOR_BCAST_START;
        break;

      case __PEI2CAB_CALL_WAKEUP:
      case __WAITING_PEI2CAB_CALL_START:
        if (__broadcast_status == __PEI2CAB_CALL_WAKEUP)
            strcpy(__msg_str, "PEI WAKEUP");
        if (__broadcast_status == __WAITING_PEI2CAB_CALL_START)
            strcpy(__msg_str, "PEI-CALL START");

        if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) {
            if (is_pushed_key) {
                printf("< IGNORE PUSHED IN KEY on %s >\n", __msg_str);
                break;
            }

#if defined(INOUT_LED_ONOFF_BY_COP)
            set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
            send_cop2avc_cmd_in_door_stop_request(__PASSIVE_INDOOR_BCAST_STOP);
            printf("< IN KEY, %s -> WAITING IN-STOP >\n", __msg_str);
            __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_STOP;
        }
        else {
            if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
              if (!is_pushed_key) {
                printf("< IGNORE RELEASED IN KEY on %s >\n", __msg_str);
                break;
              }

              if (!occ_pa_bcast_monitor_start) {
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                send_cop2avc_cmd_in_door_start_request(__PASSIVE_INDOOR_BCAST_START);
                printf("< IN KEY, %s -> WAITING IN-START >\n", __msg_str);
                indoor_pending = 1;
                indoor_waiting = 1;
                pei2cab_call_waiting = 0; // <===
                __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;
                enable_pei_wakeup_on_in_passive = 1;
    
                cop_speaker_volume_mute_set();
                occ_mic_volume_mute_set();
                stop_play_beep();
            }
            else
				Log_Status = LOG_OCC_PA_BUSY;
          } else
			Log_Status = LOG_CAB_PA_BUSY;
        }
        break;

      case __PEI2CAB_CALL_START:
        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {

            if (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) {
               if (is_pushed_key) {
                 printf("< IGNORE PUSHED IN KEY on PEI CALL >\n");
                 break;
               }
#if defined(INOUT_LED_ONOFF_BY_COP)
               set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
               send_cop2avc_cmd_in_door_stop_request(__PASSIVE_INDOOR_BCAST_STOP);
               printf("< IN KEY, ON PEI CALL, WAITING IN STOP >\n");
               indoor_pending = 1;
            }
            else if ( !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK))
            {
               if (!is_pushed_key) {
                 printf("< IGNORE RELEASED IN KEY on PEI CALL >\n");
                 break;
               }
               if (!occ_key_status) {
#if defined(INOUT_LED_ONOFF_BY_COP)
                   set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                   send_cop2avc_cmd_in_door_start_request(__PASSIVE_INDOOR_BCAST_START);
                   printf("< IN KEY, ON PEI CALL, WAITING IN-START(#1) >\n");
                   indoor_pending = 1;
                   indoor_waiting = 1;
                   enable_in_pa_simultaneous_on_pei2cab = 1;

                   /*if (!enable_out_pa_simultaneous_on_pei2cab) {
                       cop_pa_mic_codec_start();
                       set_cop_pa_mic_audio_switch_on();
                       cop_pa_and_call_mic_volume_set();
                   }*/
               }
               else if (occ_key_status) {
#if 0
                   if (pei_to_occ_no_mon_can_cop_pa) {
                       printf("< IN KEY, OPTION: MON, CAN CAB-PA >\n");
#if defined(INOUT_LED_ONOFF_BY_COP)
                       set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                       send_cop2avc_cmd_in_door_start_request(0);
                       printf("< IN KEY, ON PEI CALL, WAITING IN-START(#2) >\n");
                       indoor_pending = 1;
                       indoor_waiting = 1;
                       enable_in_pa_simultaneous_on_pei2cab = 1;

                       if (!inout_bcast_processing) {
                           cop_pa_mic_codec_start();
                           set_cop_pa_mic_audio_switch_on();
                           cop_pa_and_call_mic_volume_set();
                       }
                   }
                   else {
                       printf("< IN KEY, OPTION: PEI to OCC BUSY >\n");
				   	   Log_Status = LOG_PEI_TO_OCC_BUSY;
                   }
#else
					Log_Status = LOG_OCC_PA_BUSY;
#endif
               }
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case __CAB2CAB_CALL_WAKEUP:
#ifdef NOT_USE_CAB_CALL_WAKEUP
        printf("< BUG, IN KEY, NOT USE CAB WAKEUP >\n");
#else
//printf("%s, status:%d, inout_bcast_processing:0x%X, indoor_pending:%d,InOut_LED:%d\n", __func__,
//__broadcast_status, inout_bcast_processing, indoor_pending, InOut_LedStatus);

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
                if (!is_pushed_key) {
                  printf("< IGNORE RELEASED IN KEY, ON CAB WAKEUP >\n");
                  break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                send_cop2avc_cmd_in_door_start_request(0);

                printf("< IN KEY, ON CAB WAKEUP -> WAITING IN START >\n");
                indoor_pending = 1;
                indoor_waiting = 1;
                __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;
            }
            else if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
                if (is_pushed_key) {
                  printf("< IGNORE PUSHED IN KEY, INDOOR on CAB WAKEUP >\n");
                  break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                ret = send_cop2avc_cmd_in_door_stop_request(0);
                if (ret > 0) {
                    printf("< IN KEY, INDOOR on CAB WAKEUP -> WAITING IN-STOP >\n");
                    indoor_pending = 1;
                }
            }
          } else
			{
				Log_Status = LOG_OCC_PA_BUSY;
			}
        } else
			{
				Log_Status = LOG_CAB_PA_BUSY;
			}
#endif
        break;

      case __CAB2CAB_CALL_START:
      case WAITING_PEI2CAB_CALL_STOP:
        if (__broadcast_status == __CAB2CAB_CALL_START)
            strcpy(__msg_str, "CAB START");
        else if (__broadcast_status == WAITING_CAB2CAB_CALL_STOP)
            strcpy(__msg_str, "WAITING CAB STOP");

		DBG_LOG("__broadcast_status:%d, inout_bcast_processing:0x%X, indoor_pending:%d,InOut_LED:%d\n", 
				__broadcast_status, inout_bcast_processing, indoor_pending, InOut_LedStatus);

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if ( !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) && !indoor_pending ) {
		  		DBG_LOG("is_pushed_key = %d\n", is_pushed_key);
                if (!is_pushed_key) {
                  printf("< IGNORE RELEASED IN KEY ON %s >\n", __msg_str);
                  break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                send_cop2avc_cmd_in_door_start_request(0);
                printf("< IN KEY, ON %s, IN BCAST REQ >\n", __msg_str);
                indoor_pending = 1;
                indoor_waiting = 1;
            } else if ( (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) && !indoor_pending ) {
		  		DBG_LOG("is_pushed_key = %d\n", is_pushed_key);
                if (is_pushed_key) {
                  printf("< IGNORE PUSHED IN KEY ON %s >\n", __msg_str);
                  break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                send_cop2avc_cmd_in_door_stop_request(0);
                printf("< IN KEY, ON %s, WAITING IN STOP >\n", __msg_str);
                indoor_pending = 1;
            }
          } else {
			//DBG_LOG("\n");
			Log_Status = LOG_OCC_PA_BUSY;
		  }
        } else {
			//DBG_LOG("\n");
			Log_Status = LOG_CAB_PA_BUSY;
		}
        break;

      case __WAITING_CAB2CAB_CALL_START:
      case WAITING_CAB2CAB_CALL_STOP:
        if (__broadcast_status == __WAITING_CAB2CAB_CALL_START)
            strcpy(__msg_str, "WAITING CAB-CALL START");
        else if (__broadcast_status == WAITING_CAB2CAB_CALL_STOP)
            strcpy(__msg_str, "WAITING CAB-CALL STOP");

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
                if (!is_pushed_key) {
                  printf("< IGNORE RELEASED IN KEY ON %s >\n", __msg_str);
                  break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif

                send_cop2avc_cmd_in_door_start_request(0);
                indoor_pending = 1;
                indoor_waiting = 1;

                if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
                    cop_speaker_volume_mute_set();
                    occ_mic_volume_mute_set();
                    stop_play_beep();
                    printf("< IN KEY, %s + WAKEUP-PEI, IN START REQ >\n", __msg_str);
                }
                else 
                    printf("< IN KEY, %s, IN START REQ >\n", __msg_str);

                //__broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;
            }
            else if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
                if (is_pushed_key) {
                  printf("< IGNORE PUSHED IN KEY ON %s >\n", __msg_str);
                  break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                send_cop2avc_cmd_in_door_stop_request(0);
                printf("< IN KEY, %s, IN STOP REQ >\n", __msg_str);
                indoor_pending = 1;
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        } else
			Log_Status = LOG_CAB_PA_BUSY;
        break;


      case __PEI2CAB_CALL_MONITORING_START:
//printf("%s, status:%d, inout_bcast_processing:0x%X, indoor_pending:%d,InOut_LED:%d\n", __func__,
//__broadcast_status, inout_bcast_processing, indoor_pending, InOut_LedStatus);

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
                if (!is_pushed_key) {
                  printf("< IGNORE RELEASED IN KEY ON PEI MON >\n");
                  break;
                }
                play_stop_call_mon_rx_data();

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                send_cop2avc_cmd_in_door_start_request(0);
                printf("< IN KEY, ON PEI MON, IN START REQ >\n");
                indoor_pending = 1;
                indoor_pending = 1;
                indoor_waiting = 1;
                __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;

                pei2cab_call_monitoring = 0;
                close_cop_multi_monitoring();
                clear_rx_tx_mon_multi_addr();
                decoding_stop();

                __car_num = get_call_car_num(PEI_LIST);
                __dev_num = get_call_id_num(PEI_LIST);
				__train_num = get_call_train_num(PEI_LIST);

                if (is_monitoring_call(PEI_LIST, __car_num, __dev_num, 0)) {
                    __mon_addr = get_call_mon_addr(PEI_LIST);
                    ret = get_call_is_emergency();
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);
                }
            }
            else if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
               if (is_pushed_key) {
                 printf("< IGNORE PUSHED IN KEY ON PEI MON >\n");
                 break;
               }
               send_cop2avc_cmd_in_door_stop_request(0);
               printf("< IN KEY, ON PEI MON, IN STOP REQ >\n");
               indoor_pending = 1;
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case __CAB2CAB_CALL_MONITORING_START:
        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
                if (!is_pushed_key) {
                  printf("< IGNORE RELEASED IN KEY ON CAB MON >\n");
                  break;
                }

                play_stop_call_mon_rx_data();

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
                send_cop2avc_cmd_in_door_start_request(0);
                printf("< IN KEY, ON CAB MON, IN REQ >\n");
                indoor_pending = 1;
                indoor_pending = 1;
                indoor_waiting = 1;
                __broadcast_status = WAITING_PASSIVE_INDOOR_BCAST_START;

                pei2cab_call_monitoring = 0;
                close_cop_multi_monitoring();
                clear_rx_tx_mon_multi_addr();
                decoding_stop();

                __car_num = get_call_car_num(CAB_LIST);
                __dev_num = get_call_id_num(CAB_LIST);
				__train_num = get_call_train_num(CAB_LIST);

                if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) {
                    __mon_addr = get_call_mon_addr(CAB_LIST);
                    __tx_addr = get_call_tx_addr(CAB_LIST);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                }
            }
            else if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               && !indoor_pending)
            {
                if (is_pushed_key) {
                  printf("< IGNORE PUSHED IN KEY ON CAB MON >\n");
                  break;
                }
                send_cop2avc_cmd_in_door_stop_request(0);
                printf("< IN KEY, ON CAB MON, IN STOP REQ >\n");
                indoor_pending = 1;
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else
		   Log_Status = LOG_CAB_PA_BUSY;
        break;

      case OCC_PA_BCAST_START:
      case WAITING_OCC_PA_BCAST_START:
      case WAITING_OCC_PA_BCAST_STOP:
		if(!is_pushed_key) {
        	if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
        	    //if(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) {
				if(enable_occ_pa_on_in_passive) {
#if defined(INOUT_LED_ONOFF_BY_COP)
        	    	set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
        	    	ret = send_cop2avc_cmd_in_door_stop_request(__PASSIVE_INDOOR_BCAST_START);
            		if (ret > 0) {
            	    	printf("< IN KEY, IN START -> WAITING STOP(STAT:0x%X) by OCC-PA >\n", inout_bcast_processing);
            		}
					enable_occ_pa_on_in_passive = 0;
            	} else if (inout_bcast_processing & INOUT_BUTTON_STATUS_INOUT_ON_MASK) {
#if defined(INOUT_LED_ONOFF_BY_COP)
        	    	set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
        			send_cop2avc_cmd_in_door_stop_request(__PASSIVE_INOUTDOOR_BCAST_START);
       	 			printf("< IN KEY, INOUT START -> WAITING IN-STOP by OCC-PA > \n");
				}
        	}
		} else {
			Log_Status = LOG_OCC_PA_BUSY;
        	printf("< IN KEY, ON OCC-PA, BUSY >\n");
		}
        break;

      default:
        printf("%s: Not defined Index: %d.\n", __func__, (int)__broadcast_status);
        break;
    }

	if (fire_alarm_is_use && enable_fire_alarm) {
		if(is_pushed_key)
			fire_status_reset(2);
		else
			lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
	}


	pa_in_key_lock = 0;
    return ret;
}

int __pa_out_key_process (int key_pressed_status)
{
    int ret = 0;
    char __msg_str[32];
    int is_pushed_key = key_pressed_status & KEY_PRESSED_MASK;

	pa_out_key_lock = 1;

	if(pa_out_Key_pressed_on_fire_status)
		pa_out_Key_pressed_on_fire_status = 0;
	else
		pa_out_Key_pressed_on_fire_status = 1;

    if (remote_occ_pa_monitor_start) { /* Ver 0.71, 2014/01/08 */
		Log_Status = LOG_OCC_PA_BUSY;
		pa_out_key_lock = 0;
        return 0;
    }

    __msg_str[0] = 0;

    switch(__broadcast_status) {
      case COP_STATUS_IDLE:
      case WAITING_PASSIVE_INDOOR_BCAST_START:
      case WAITING_PASSIVE_OUTDOOR_BCAST_START:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED OUT KEY on IDLE >\n");
            break;
        }

        if (outdoor_waiting == 1) {
            printf("< OUT KEY, ON WAITING OUT START(%d) >\n", __broadcast_status);
            break;
        }

        if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK) {
            printf("<< Already RUN OUTDOOR > >\n");
            break;
        }

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF && !occ_pa_bcast_monitor_start)
        {
#if defined(INOUT_LED_ONOFF_BY_COP)
            set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif

            send_cop2avc_cmd_out_door_start_request(0);
            printf("< OUT KEY -> WAITING OUTDOOR >\n");
            __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;
            outdoor_pending = 1;
            outdoor_waiting = 1;
        }
        else if (InOut_LedStatus != INOUT_BUTTON_STATUS_ALLOFF && !occ_pa_bcast_monitor_start)
			Log_Status = LOG_CAB_PA_BUSY;
        else if (occ_pa_bcast_monitor_start)
			Log_Status = LOG_OCC_PA_BUSY;
        break;

      case __PASSIVE_INDOOR_BCAST_START:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED OUT KEY on INDOOR >\n");
            break;
        }

        if (outdoor_waiting == 1) {
            printf("< OUT KEY, ON IN START, WAITING OUT START >\n");
            break;
        }

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
            if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK)
               )
            {
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                send_cop2avc_cmd_out_door_start_request(0);
                outdoor_waiting = 1;
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                printf("< IN START -> WAITING OUTSTART >\n");
                __broadcast_status = WAITING_OUTSTART_PASSIVE_INOUTDOOR_BCAST_START;
            }
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        break;

      case __PASSIVE_OUTDOOR_BCAST_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED OUT KEY on OUTDOOR >\n");
            break;
        }

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
            if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
               )
            {
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                send_cop2avc_cmd_out_door_stop_request(0);
                printf("< OUT KEY, OUT START -> WAITING STOP >\n");
                __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_STOP;
            }
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case WAITING_PASSIVE_INDOOR_BCAST_STOP:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED OUT KEY on WAIT IN-STOP >\n");
            break;
        }

        if (InOut_LedStatus != INOUT_BUTTON_STATUS_ALLOFF) {
			Log_Status = LOG_CAB_PA_BUSY;
            break;
        }

        send_cop2avc_cmd_out_door_start_request(0);
        waiting_outstart_on_wait_in_stop = 1;
#if defined(INOUT_LED_ONOFF_BY_COP)
        set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
        printf("< OUT KEY, ON WAITING IN STOP, SEND OUT START REQ >\n");
        break;

      case __PASSIVE_INOUTDOOR_BCAST_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED OUT KEY on INOUT >\n");
            break;
        }

        send_cop2avc_cmd_out_door_stop_request(0);
#if defined(INOUT_LED_ONOFF_BY_COP)
        set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
        printf("< OUT KEY, INOUT START -> WAITING OUT STOP >\n");
        __broadcast_status = _WAITING_OUTSTOP_PASSIVE_INOUTDOOR_BCAST_START;

        if (inout_bcast_processing) {
			running_cop_pa_and_call_mic_audio_sending();
		}
        break;

      case _WAITING_INSTOP_PASSIVE_INOUTDOOR_BCAST_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED OUT KEY on WAIT INSTOP in INOUT >\n");
            break;
        }

#if defined(INOUT_LED_ONOFF_BY_COP)
        set_key_led_blink(enum_COP_KEY_PA_IN, 1);
#endif
        send_cop2avc_cmd_out_door_stop_request(0);
        printf("< OUT KEY, ON WAIT INSTOP in INOUT START, SEND OUT-STOP REQ > \n");
        outdoor_stop_pending = 1;
        break;

      case _WAITING_OUTSTOP_PASSIVE_INOUTDOOR_BCAST_START:
        printf("< OUT KEY, ON INOUT START, WAITING OUT-STOP >\n");
        break;

      case WAITING_OUTSTART_PASSIVE_INOUTDOOR_BCAST_START:
        printf("< OUT KEY, ON IN START, WAITING OUT-START >\n");
        break;

      case WAITING_INSTART_PASSIVE_INOUTDOOR_BCAST_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED OUT KEY on WAIT INSTART in INOUT >\n");
            break;
        }
        send_cop2avc_cmd_out_door_stop_request(0);
        printf("< OUT KEY, ON WAIT INSTART in INOUT START, SEND OUT-STOP REQ > \n");
        waiting_outstop_on_wait_in_start = 1;
        break;

      case WAITING_PASSIVE_OUTDOOR_BCAST_STOP:
        printf("< OUT KEY, ON WAITING OUT-STOP >\n");
        break;

      case __PEI2CAB_CALL_WAKEUP:
//printf("%s, __PEI2CAB_CALL_WAKEUP, inout_bcast_processing:0x%X\n", __func__, inout_bcast_processing);
        if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK) {
            if (is_pushed_key) {
                printf("< IGNORE PUSHED OUT KEY on PEI WAKEUP >\n");
                break;
            }
#if defined(INOUT_LED_ONOFF_BY_COP)
            set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
            send_cop2avc_cmd_out_door_stop_request(0);
            printf("< PEI WAKEUP -> WAITING OUT STOP >\n");
            __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_STOP;
            pending_outdoor_on_pei_wakeup = 0;
        }
        else {
            if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
              if (!is_pushed_key) {
                  printf("< IGNORE RELEASED OUT KEY on PEI WAKEUP >\n");
                  break;
              }
              if (!occ_pa_bcast_monitor_start) {
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                ret = send_cop2avc_cmd_out_door_start_request(0);
                if (ret > 0) {
                    printf("< OUT KEY, PEI WAKEUP -> WAITING OUT START >\n");
                    outdoor_pending = 1;
                    outdoor_waiting = 1;
                    pei2cab_call_waiting = 0; // <===
                    __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;
                    enable_pei_wakeup_on_out_passive = 1;
    
                    cop_speaker_volume_mute_set();
                    occ_mic_volume_mute_set();
                    stop_play_beep();
                }
              } else
				Log_Status = LOG_OCC_PA_BUSY;
            }
            else
				Log_Status = LOG_CAB_PA_BUSY;
        }
        break;

      case __PEI2CAB_CALL_START:
        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
            {
                if (is_pushed_key) {
                    printf("< IGNORE PUSHED OUT KEY on PEI-CALL >\n");
                    break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                send_cop2avc_cmd_out_door_stop_request(0);
                printf("< OUT KEY, ON PEI CALL, WAITING OUT STOP >\n");
                outdoor_pending = 1;
            }
            else {
               if (!is_pushed_key) {
                   printf("< IGNORE RELEASED OUT KEY on PEI-CALL >\n");
                   break;
               }

               if (!occ_key_status) {
#if defined(INOUT_LED_ONOFF_BY_COP)
                   set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                   send_cop2avc_cmd_out_door_start_request(0);
                   printf("< OUT KEY, ON PEI CALL, WAITING OUT START(#1) >\n");
                   outdoor_pending = 1;
                   outdoor_waiting = 1;
                   enable_out_pa_simultaneous_on_pei2cab = 1;

                   //if (!enable_in_pa_simultaneous_on_pei2cab)
                   /*if (!inout_bcast_processing)
                   {
                       cop_pa_mic_codec_start();
                       set_cop_pa_mic_audio_switch_on();
                       cop_pa_and_call_mic_volume_set();
                   }*/
               }
               else if (occ_key_status) {
#if 0
                   if (pei_to_occ_no_mon_can_cop_pa) {
                       printf("< OUT KEY, OPTION: MON, CAN CAB-PA >\n");
#if defined(INOUT_LED_ONOFF_BY_COP)
                       set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                       send_cop2avc_cmd_out_door_start_request(0);
                       printf("< OUT KEY, ON PEI CALL, WAITING OUT START(#2) >\n");
                       outdoor_pending = 1;
                       outdoor_waiting = 1;
                       enable_out_pa_simultaneous_on_pei2cab = 1;

                       if (!inout_bcast_processing) {
                           cop_pa_mic_codec_start();
                           set_cop_pa_mic_audio_switch_on();
                           cop_pa_and_call_mic_volume_set();
                       }
                   }
                   else {
                       printf("< OUT KEY, OPTION: PEI to OCC BUSY >\n");
					   Log_Status = LOG_PEI_TO_OCC_BUSY;
                   }
#else
					Log_Status = LOG_OCC_PA_BUSY;
#endif
               }
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

#ifdef NOT_USE_CAB_CALL_WAKEUP
      case __CAB2CAB_CALL_WAKEUP:
        printf("< BUG, OUT KEY, NOT USE CAB WAKEUP >\n");
        break;
#else

      case __CAB2CAB_CALL_WAKEUP:
#endif
      case __CAB2CAB_CALL_START:
#ifdef NOT_USE_CAB_CALL_WAKEUP
            strcpy(__msg_str, "CAB CALL");
#else
        if (__broadcast_status == __CAB2CAB_CALL_WAKEUP)
            strcpy(__msg_str, "CAB WAKEUP");
        else if (__broadcast_status == __CAB2CAB_CALL_START)
            strcpy(__msg_str, "CAB CALL");
#endif

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                && !outdoor_pending
               )
            {
                 if (!is_pushed_key) {
                     printf("< IGNORE RELEASED OUT KEY on %s >\n", __msg_str);
                     break;
                 }
#if defined(INOUT_LED_ONOFF_BY_COP)
                 set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                 ret = send_cop2avc_cmd_out_door_start_request(0);
                 outdoor_pending = 1;
                 outdoor_waiting = 1;

#ifdef NOT_USE_CAB_CALL_WAKEUP
                     printf("< OUT KEY, ON %s, OUT BCAST REQ >\n", __msg_str);
#else
                 if (__broadcast_status == __CAB2CAB_CALL_WAKEUP) {
                     __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;
                     printf("< OUT KEY, ON %s -> WAITING OUT START  >\n", __msg_str);
                 } else
                     printf("< OUT KEY, ON %s, OUT BCAST REQ >\n", __msg_str);
#endif
            }
            else if (   (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                     && !outdoor_pending
                    )
            {
                 if (is_pushed_key) {
                     printf("< IGNORE PUSHED OUT KEY on %s >\n", __msg_str);
                     break;
                 }
#if defined(INOUT_LED_ONOFF_BY_COP)
                 set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                 send_cop2avc_cmd_out_door_stop_request(0);
                 printf("< OUT KEY, ON %s, WAITING OUT STOP >\n", __msg_str);
                 outdoor_pending = 1;
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case __PEI2CAB_CALL_MONITORING_START:
        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                && !outdoor_pending
               )
            {
                if (!is_pushed_key) {
                    printf("< IGNORE RELEASED OUT KEY on PEI MON>\n");
                    break;
                }

                play_stop_call_mon_rx_data();

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                send_cop2avc_cmd_out_door_start_request(0);
                printf("< OUT KEY, ON PEI MON, OUT START REQ >\n");
                outdoor_pending = 1;
                outdoor_waiting = 1;
                __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;

                pei2cab_call_monitoring = 0;
                close_cop_multi_monitoring();
                clear_rx_tx_mon_multi_addr();
                decoding_stop();

                __car_num = get_call_car_num(PEI_LIST);
                __dev_num = get_call_id_num(PEI_LIST);
				__train_num = get_call_train_num(PEI_LIST);

                if (is_monitoring_call(PEI_LIST, __car_num, __dev_num, 0)) {
                    __mon_addr = get_call_mon_addr(PEI_LIST);
                    ret = get_call_is_emergency();
                    add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);
                }
            }
            else if ( (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                    && !outdoor_pending
               )
            {
                if (is_pushed_key) {
                    printf("< IGNORE PUSHED OUT KEY on PEI MON>\n");
                    break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                send_cop2avc_cmd_out_door_stop_request(0);
                printf("< OUT KEY, ON PEI MON, OUT STOP REQ >\n");
                outdoor_pending = 1;
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case __CAB2CAB_CALL_MONITORING_START:
        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (occ_pa_bcast_monitor_start == 0) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                && !outdoor_pending
               )
            {
                if (!is_pushed_key) {
                    printf("< IGNORE RELEASED OUT KEY on CAB MON>\n");
                    break;
                }

                play_stop_call_mon_rx_data();

#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                send_cop2avc_cmd_out_door_start_request(0);
                printf("< OUT KEY, ON CAB MON, OUT START REQ >\n");
                outdoor_pending = 1;
                outdoor_waiting = 1;
                __broadcast_status = WAITING_PASSIVE_OUTDOOR_BCAST_START;

                pei2cab_call_monitoring = 0;
                close_cop_multi_monitoring();
                clear_rx_tx_mon_multi_addr();
                decoding_stop();

                __car_num = get_call_car_num(CAB_LIST);
                __dev_num = get_call_id_num(CAB_LIST);
				__train_num = get_call_train_num(CAB_LIST);

                if (is_monitoring_call(CAB_LIST, __car_num, __dev_num, 0)) {
                    __mon_addr = get_call_mon_addr(CAB_LIST);
                    __tx_addr = get_call_tx_addr(CAB_LIST);
                    add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
                }
            }
            else if ( (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
                    && !outdoor_pending
               )
            {
                if (is_pushed_key) {
                    printf("< IGNORE PUSHED OUT KEY on CAB MON>\n");
                    break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                ret = send_cop2avc_cmd_out_door_stop_request(0);
                if (ret > 0) {
                    printf("< OUT KEY, ON CAB MON, OUT STOP REQ >\n");
                    outdoor_pending = 1;
                }
            }
         } else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case __WAITING_CAB2CAB_CALL_START:
        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (occ_pa_bcast_monitor_start == 0) {
            if (  !(inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
               && !outdoor_pending)
            {
                if (!is_pushed_key) {
                    printf("< IGNORE RELEASED OUT KEY on WAIT CAB >\n");
                    break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                ret = send_cop2avc_cmd_out_door_start_request(0);
                if (ret > 0) {
                    outdoor_pending = 1;
                    outdoor_waiting = 1;

                    if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
                        cop_speaker_volume_mute_set();
                        occ_mic_volume_mute_set();
                        stop_play_beep();
                        printf("< OUT KEY, ON WAITING-CAB + WAKEUP-PEI, OUT START REQ >\n");
                    }
                    else 
                        printf("< OUT KEY, ON WAITING CAB-START, OUT START REQ >\n");

                }
            }
            else if (  (inout_bcast_processing & INOUT_BUTTON_STATUS_OUT_ON_MASK)
               && !outdoor_pending)
            {
                if (is_pushed_key) {
                    printf("< IGNORE PUSHED OUT KEY on WAIT CAB >\n");
                    break;
                }
#if defined(INOUT_LED_ONOFF_BY_COP)
                set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
                ret = send_cop2avc_cmd_out_door_stop_request(0);
                if (ret > 0) {
                    printf("< OUT KEY, ON WAITING CAB-START, OUT STOP REQ >\n");
                    outdoor_pending = 1;
                }
            }
          } else
			Log_Status = LOG_OCC_PA_BUSY;
        } else
			Log_Status = LOG_CAB_PA_BUSY;
        break;

      case OCC_PA_BCAST_START:
      case WAITING_OCC_PA_BCAST_START:
      case WAITING_OCC_PA_BCAST_STOP:
		if(!is_pushed_key) {
        	if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
        	    //if(inout_bcast_processing & INOUT_BUTTON_STATUS_IN_ON_MASK) {
				if(enable_occ_pa_on_out_passive) {
#if defined(INOUT_LED_ONOFF_BY_COP)
        	    	set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
        	    	ret = send_cop2avc_cmd_out_door_stop_request(__PASSIVE_OUTDOOR_BCAST_START);
            		if (ret > 0) {
            	    	printf("< OUT KEY, OUT START -> WAITING STOP(STAT:0x%X) by OCC-PA >\n", inout_bcast_processing);
            		}
					enable_occ_pa_on_out_passive = 0;
            	} else if (inout_bcast_processing & INOUT_BUTTON_STATUS_INOUT_ON_MASK) {
#if defined(INOUT_LED_ONOFF_BY_COP)
        	    	set_key_led_blink(enum_COP_KEY_PA_OUT, 1);
#endif
        			send_cop2avc_cmd_out_door_stop_request(__PASSIVE_INOUTDOOR_BCAST_START);
       	 			printf("< OUT KEY, INOUT START -> WAITING OUT-STOP by OCC-PA > \n");
				}
        	}
		} else {
			Log_Status = LOG_OCC_PA_BUSY;
        	printf("< OUT KEY, ON OCC-PA, BUSY >\n");
		}
        break;

      default:
        printf("%s: Not defined Index: %d.\n", __func__, (int)__broadcast_status);
        break;
    }

	if (fire_alarm_is_use && enable_fire_alarm) {
		if(is_pushed_key)
			fire_status_reset(2);
		else
			lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
	}

	pa_out_key_lock = 0;
    return ret;
}

int __occ_key_process (int key_pressed_status, int idx)
{
    int ret = 0, stat = 0;
    int is_pei_mon = 0;

	occ_key_lock = 1;
	if(tractor_cab2cab_start) {
        //Func_Cop_Key_Process_List[5](0, tcp_connected);
        __tractor_key_process(1);
		printf("< TRACTOR DISABLE by OCC >\n");
		//Log_Status = LOG_UNKNOWN_ERR;
		//return 0;
	}

#if 0
	if (fire_alarm_is_use && enable_fire_alarm) {
    	int is_pushed_key = keyval & KEY_PRESSED_MASK;
		if(is_pushed_key)
			fire_status_reset(1);
		else
			lcd_draw_image(2, 2, 316, 203, 2);		/* Imgae of Call 119 */
	}
#endif

Retry:
    switch(__broadcast_status) {
      case COP_STATUS_IDLE:
      case __PASSIVE_INDOOR_BCAST_START:
      case __PASSIVE_OUTDOOR_BCAST_START:
      case __PASSIVE_INOUTDOOR_BCAST_START:
      case WAITING_PASSIVE_INDOOR_BCAST_START:
      case WAITING_PASSIVE_OUTDOOR_BCAST_START:
		//if (idx == enum_COP_KEY_OCC) {
		//	printf("< [ERR] OCC KEY -> COP_STATUS_IDLE, not action >\n");
		//	occ_key_lock = 0;
		//	return ret;
		//}

        if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) {
          if (!occ_pa_bcast_monitor_start) {
              if (__broadcast_status == __PASSIVE_INDOOR_BCAST_START) {
                  printf("< OCC KEY, ON IN DOOR >\n");
                  enable_occ_pa_on_in_passive = 1;
              }
              else if (__broadcast_status == __PASSIVE_OUTDOOR_BCAST_START) {
                  printf("< OCC KEY, ON OUT DOOR >\n");
                  enable_occ_pa_on_out_passive = 1;
              }
              else if (__broadcast_status == __PASSIVE_INOUTDOOR_BCAST_START) {
                  printf("< OCC KEY, ON INOUT DOOR >\n");
                  enable_occ_pa_on_in_passive = 1;
                  enable_occ_pa_on_out_passive = 1;
              }
              else {
                  enable_occ_pa_on_in_passive = 0;
                  enable_occ_pa_on_out_passive = 0;
              }

              if (!occ_pa_pending) {
                  send_cop2avc_cmd_occ_pa_start_request(0);
                  printf("< 1, OCC KEY -> WAITING OCC PA >\n");
                  __broadcast_status = WAITING_OCC_PA_BCAST_START;
                  now_occ_pa_start_wait = 1;;
                  //set_key_led_blink(enum_COP_KEY_OCC, 1);
                  occ_pa_pending = 1;
              }

              if (!occ_pa_pending) {
                  enable_occ_pa_on_in_passive = 0;
                  enable_occ_pa_on_out_passive = 0;
              }
          }
          else
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else {
          if (!occ_pa_bcast_monitor_start) // add only this line, 2014/02/20
          {
              enable_occ_pa_on_cab_pa_mon = 1;

              send_cop2avc_cmd_occ_pa_start_request(0);
              printf("< 2, OCC KEY -> WAITING OCC PA >\n");
              __broadcast_status = WAITING_OCC_PA_BCAST_START;
              now_occ_pa_start_wait = 1;;
              //set_key_led_blink(enum_COP_KEY_OCC, 1);
              occ_pa_pending = 1;
          }
          else
              Log_Status = LOG_OCC_PA_BUSY; // add only this line, 2014/02/20
        }
        break;

      case WAITING_OCC_PA_BCAST_START:
        if (occ_pa_pending) {
            printf("< OCC KEY, ON WAITING OCC PA START >\n");
        }
        break;

      case OCC_PA_BCAST_START:
		//if (idx == enum_COP_KEY_OCC) {
		//	printf("< [ERR] OCC KEY -> OCC_PA_BCAST_START, not action >\n");
		//	occ_key_lock = 0;
		//	return ret;
		//}

		if (occ_pa_running) {
			occ_pa_ack_set_low();		// Added by Michael RYU(Arp 17, 2015)
            ret = send_cop2avc_cmd_occ_pa_stop_request(0);
            if (ret > 0) {
                //set_key_led_blink(enum_COP_KEY_OCC, 1);
                occ_pa_pending = 1;
                __broadcast_status = WAITING_OCC_PA_BCAST_STOP;
                now_occ_pa_stop_wait = 1;
                now_occ_pa_start = 0;
                printf("< OCC KEY, OCC PA -> WAITING OCC-PA STOP >\n");
            }
        }
        break;

      case WAITING_OCC_PA_BCAST_STOP:
        printf("< OCC KEY, ON WAITING OCC PA STOP >\n");
        break;
      case __PEI2CAB_CALL_WAKEUP:
        if (occ_pa_bcast_monitor_start) {
        	printf("< OCC KEY, ON PEI WAKEUP, OCC PA BUSY >\n");
			Log_Status = LOG_OCC_PA_BUSY;
        } else {
           	/* 2013/12/06, Ver 0.70 */
           	pei2cab_call_waiting = 0; // <===
           	cop_speaker_volume_mute_set();
           	stop_play_beep();
           	decoding_stop();

           	send_cop2avc_cmd_occ_pa_start_request(0);
           	printf("< OCC KEY -> WAITING OCC PA >\n");
           	__broadcast_status = WAITING_OCC_PA_BCAST_START;
           	now_occ_pa_start_wait = 1;;
           	//set_key_led_blink(enum_COP_KEY_OCC, 1);
           	occ_pa_pending = 1;
       	}
        break;

      case __PEI2CAB_CALL_START:
       	//if (occ_key_status == 0) {
		if( occ_pa_en_get_level() ) {
#if 0
           	//if (!occ_pa_bcast_monitor_start)
           	//    set_key_led(enum_COP_KEY_OCC);

			occ_speaker_volume_mute_set();
			cop_speaker_volume_mute_set();

			set_occ_spk_audio_switch_on(); // PC10:0, PC11:0, PA5:0

			set_cop_spk_mute_onoff(0);

			DBG_LOG("occ_spk_vol_atten = %d\n", occ_spk_vol_atten);
           	occ_speaker_volume_set(occ_spk_vol_atten);
			cop_speaker_volume_set();

			occ_mic_volume_mute_set();

			set_occ_mic_audio_switch_on(); 			// PC7:0, PC9:0, PA3:1
			set_occ_mic_to_spk_audio_switch_on();
			//set_occ_mic_to_recode_audio_switch_on();// PA4:1, PA3:0, PA2:0, PA1:0

			occ_mic_volume_set(occ_mic_vol_atten);

           	occ_key_status = 1;
           	occ_pa_en_status_old = 0;

			/* To disable the COP MIC Input */
			cop_pa_mic_volume_mute_set();
			cop_pa_mic_codec_stop();
#else
           	occ_key_status = 1;
			occ_pa_running = 1;
           	occ_pa_en_status_old = 0;

			cop_occ_pa_and_occ_call_codec_start();
			connect_occ_passive_bcast_tx();

			set_cop_call_mic_audio_switch_on();
			set_occ_mic_audio_switch_on();
			occ_mic_volume_set(occ_mic_vol_atten);
			send_cop2avc_cmd_occ_pa_start_request(0);
#endif
           	printf("< OCC KEY, ON at PEI-CAB CALL>\n");
		} else {
#if 0
           	//if (!occ_pa_bcast_monitor_start)
           	//    clear_key_led(enum_COP_KEY_OCC);

			occ_speaker_volume_mute_set();
			cop_speaker_volume_mute_set();

			set_occ_mic_to_spk_audio_switch_off();	// PA3:1
			set_cop_call_mic_audio_switch_on();		// PA2:1, PA1:0

			set_cop_spk_mute_onoff(0); // mute off

			cop_speaker_volume_set();

  	        occ_key_status = 0;

			reset_cop_pa_multi_rx_data_ptr();

			cop_pa_mic_codec_start();
			set_cop_pa_mic_audio_switch_on();
			cop_pa_and_call_mic_volume_set();
#else
  	        occ_key_status = 0;
			occ_pa_running = 0;
			//set_occ_mic_to_recode_audio_switch_off();

			send_cop2avc_cmd_occ_pa_stop_request(0);
#endif
       		DBG_LOG("< OCC KEY, OFF at PEI-CAB CALL>\n");
		}

       	if (inout_bcast_processing && occ_key_status)
       	{
           	printf("< OCC KEY, CONN MULTI to CAB-PA ONLY >\n");

           	if (!ptt_key_on_pei_cab_pa_open_multi_tx) {
           	    ptt_key_on_pei_cab_pa_open_multi_tx = 1;
           	    close_cop_multi_passive_tx();
           	    connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
           	}

           	cop_pa_mic_codec_start();
           	set_cop_pa_mic_audio_switch_on();
           	cop_pa_and_call_mic_volume_set();
       	} else if (inout_bcast_processing && !occ_key_status) {
           	printf("< OCC KEY, CONN MULTI to CAB-PA with CALL >\n");

           	ptt_key_on_pei_cab_pa_open_multi_tx = 0;
           	close_cop_multi_tx();
           	close_cop_multi_passive_tx();
           	connect_cop_passive_bcast_tx_while_othercall();

           	cop_pa_mic_volume_mute_set();
           	cop_pa_mic_codec_stop();
       	}

       	force_call_menu_draw();

       	if (!occ_key_status)
       	    old_occ_tx_rx_en_ext_flag = -1;

       	if (pei_comm_half_duplex) {
       	    if (!occ_key_status)
				Log_Status = LOG_UNKNOWN_ERR;
   		    else
				Log_Status = LOG_READY;
       	}
        break;

      case __CAB2CAB_CALL_WAKEUP:
#ifdef NOT_USE_CAB_CALL_WAKEUP
        printf("< BUG, OCC KEY, NOT USE CAB WAKEUP >\n");
#else
#if 0
        cop_speaker_volume_mute_set();
        stop_play_beep();
        decoding_stop();
        if (occ_pa_bcast_monitor_start == 0) {
            if (!occ_pa_pending) {
                send_cop2avc_cmd_occ_pa_start_request(0);
                printf("< OCC KEY, ON CAB WAKEUP -> WAITING OCC PA >\n");
                __broadcast_status = WAITING_OCC_PA_BCAST_START;
                now_occ_pa_start_wait = 1;;
                //set_key_led_blink(enum_COP_KEY_OCC, 1);
                occ_pa_pending = 1;
            }
        }
        else
			Log_Status = LOG_OCC_PA_BUSY;
#else
        /* Ver 0.71, 2014/01/10 */
        if (occ_pa_bcast_monitor_start == 0) {
            if (!occ_pa_pending) {
                cop_speaker_volume_mute_set();
                stop_play_beep();
                decoding_stop();

                send_cop2avc_cmd_occ_pa_start_request(0);
                printf("< OCC KEY, ON CAB WAKEUP -> WAITING OCC PA >\n");
                __broadcast_status = WAITING_OCC_PA_BCAST_START;
                now_occ_pa_start_wait = 1;;
                //set_key_led_blink(enum_COP_KEY_OCC, 1);
                occ_pa_pending = 1;
            }
        }
        else
			Log_Status = LOG_OCC_PA_BUSY;
#endif
#endif
        break;

      case __CAB2CAB_CALL_START:
      case __WAITING_CAB2CAB_CALL_START:
		/* Need to change : Cab2cab Exit & start the occ-pa first */
		DBG_LOG("occ_pa_pei_enabled=%d\n", occ_pa_pei_enabled);
#if 0
		if ( (occ_status == OCC2PEI_CALL_START) || (occ_status == OCC2PEI_CALL_STOP) ) {
			cab2cab_call_running = 0; /* 2013/11/07 */
			play_stop_call_rx_data();
			cab2cab_call_pending = 0;
			send_cop2avc_cmd_cab2cab_call_stop_request(0, __car_num, __dev_num);

			printf("< 1, OCC KEY IN CAB2CAB START & WAITING-> WAITING OCC-PEI >\n");
			//set_key_led_blink(enum_COP_KEY_OCC, 1);
		} else if (occ_status == OCC_PA_BCAST_START) {
#endif
        	if (!occ_pa_pending) {
        	   	cab2cab_call_running = 0; /* 2013/11/07 */
           		play_stop_call_rx_data();
				cab2cab_call_pending = 0;
            	send_cop2avc_cmd_cab2cab_call_stop_request(0, __car_num, __dev_num);

            	send_cop2avc_cmd_occ_pa_start_request(0);
            	printf("< 1, OCC KEY IN CAB2CAB START & WAITING-> WAITING OCC PA >\n");
            	__broadcast_status = WAITING_OCC_PA_BCAST_START;
            	now_occ_pa_start_wait = 1;;
            	//set_key_led_blink(enum_COP_KEY_OCC, 1);
            	occ_pa_pending = 1;
			}
#if 0
        }
#endif
        break;

      case __CAB2CAB_CALL_MONITORING_START:
      case __PEI2CAB_CALL_MONITORING_START:
        if (__broadcast_status == __CAB2CAB_CALL_MONITORING_START)
            is_pei_mon = 0;
        else
            is_pei_mon = 1;

        /* if (InOut_LedStatus == INOUT_BUTTON_STATUS_ALLOFF) Ver 0.71, delete */
        {
          if (occ_pa_bcast_monitor_start == 0) {

            if (is_pei_mon)
                printf("< OCC KEY, ON PEI-MON -> WAITING OCC PA >\n");
            else
                printf("< OCC KEY, ON CAB-MON -> WAITING OCC PA >\n");

            play_stop_call_mon_rx_data();

            cab2cab_call_monitoring = 0;

            close_cop_multi_monitoring();
            clear_rx_tx_mon_multi_addr();
            decoding_stop();

            if (is_pei_mon) {
                __car_num = get_call_car_num(PEI_LIST);
                __dev_num = get_call_id_num(PEI_LIST);
                __mon_addr = get_call_mon_addr(PEI_LIST);
				__train_num = get_call_train_num(PEI_LIST);

                stat = get_call_is_emergency();
                add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, stat, __car_num, __dev_num, __mon_addr, 0, __train_num);
            } else {
                __car_num = get_call_car_num(CAB_LIST);
                __dev_num = get_call_id_num(CAB_LIST);
                __mon_addr = get_call_mon_addr(CAB_LIST);
                __tx_addr = get_call_tx_addr(CAB_LIST);
				__train_num = get_call_train_num(CAB_LIST);
                add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);
            }

            send_cop2avc_cmd_occ_pa_start_request(0);
            __broadcast_status = WAITING_OCC_PA_BCAST_START;
            now_occ_pa_start_wait = 1;;
            //set_key_led_blink(enum_COP_KEY_OCC, 1);
            occ_pa_pending = 1;
          }
          else
			  Log_Status = LOG_OCC_PA_BUSY;
        }
        /*else
          Log_Status = LOG_CAB_PA_BUSY; Ver 0.71, delete */

        break;

      default:
        printf("%s: Not defined Index: %d.\n", __func__, (int)__broadcast_status);
        break;
    }

	occ_key_lock = 0;
    return ret;
}

#if 0
static int __occ_ptt_key_process (enum __cop_key_enum keyval, int tcp_connected)
{
    int ret = 0;

    switch(__broadcast_status) {
      case COP_STATUS_IDLE:
      case __PEI2CAB_CALL_MONITORING_START:
        break;

      case __PEI2CAB_CALL_START:
        /*if (   enable_in_pa_simultaneous_on_pei2cab
            || enable_out_pa_simultaneous_on_pei2cab)
*/
/*
        if (inout_bcast_processing)
        {
            printf("< OCC KEY, ON PEI START, PEI & CAB PA BUSY >\n");
		    Log_Status = LOG_CAB_PA_BUSY;
            break;
        }
*/

        if (occ_key_status & OCC_PTT_ON) {
            occ_speaker_volume_mute_set();
            set_cop_spk_mute_onoff(0);
            cop_speaker_volume_set();

            occ_key_status &= ~OCC_PTT_ON;
            printf("< PTT KEY, OFF at PEI CALL>\n");

            occ_pa_ack_set_high();
        }
        else {
            cop_speaker_volume_mute_set();
            set_cop_spk_mute_onoff(1);
            cop_speaker_volume_set(); /* LOAD 2 */

            occ_speaker_volume_set(occ_spk_vol_atten);

            occ_key_status |= OCC_PTT_ON;
            printf("< PTT KEY, ON at PEI CALL>\n");

            occ_pa_ack_set_high();
        }

        if (   inout_bcast_processing
            && (occ_key_status)
           )
        {
            printf("< PTT KEY, CONN MUTI to CAB-PA ONLY >\n");
            if (!ptt_key_on_pei_cab_pa_open_multi_tx) {
                ptt_key_on_pei_cab_pa_open_multi_tx = 1;
                close_cop_multi_passive_tx();
                connect_cop_passive_bcast_tx(COP_PASSIVE_BCAST_TX_MULTI_ADDR);
            }

            cop_pa_mic_codec_start();
            set_cop_pa_mic_audio_switch_on();
            cop_pa_and_call_mic_volume_set();
        }
        else if (inout_bcast_processing && !occ_key_status) {
            printf("< PTT KEY, CONN MUTI to CAB-PA with CALL >\n");

            ptt_key_on_pei_cab_pa_open_multi_tx = 0;
            close_cop_multi_tx();
            close_cop_multi_passive_tx();
            connect_cop_passive_bcast_tx_while_othercall();

            cop_pa_mic_volume_mute_set();
            cop_pa_mic_codec_stop();
        }

        force_call_menu_draw(); // 2013/11/15

        if (!occ_key_status) {
            old_occ_tx_rx_en_ext_flag = -1;
            occ_speaker_volume_mute_set();
        }

        if (pei_comm_half_duplex) {
            if (!occ_key_status)
				Log_Status = LOG_UNKNOWN_ERR;
            else
				Log_Status = LOG_READY;
        }

        break;

      default:
        printf("%s: Not defined Index: %d.\n", __func__, (int)__broadcast_status);
        break;
    }

    return ret;
}
#endif

int __tractor_key_process (int key_pressed_status)
{
	int ret = 0;

	if ( (!tractor_possible) && (tractor_cab2cab_start == 0) ) {
		printf("< TRACTOR IS NOT READY --> BY AVC(Tractor Signal)>\n");
		return 0;
	}

	if (occ_pa_pei_enabled != 0) {	/* OCC PA */
		printf("< TRACTOR IS NOT READY --> BY OCC(PA or PEI) >\n");
		//if(occ_pa_pei_enabled == 1)
		if (__broadcast_status == __PEI2CAB_CALL_START)
			Log_Status = LOG_PEI_TO_OCC_BUSY;
		else
		    Log_Status = LOG_OCC_PA_BUSY;
		return 0;
	} else {
		if (occ_pa_bcast_monitor_start) {
		    Log_Status = LOG_OCC_PA_BUSY;
			return 0;
		}
	}

#if 0	
	if (occ_pa_bcast_monitor_start) {	/* OCC PA Monitoring */
		printf("< TRACTOR IS NOT READY --> BY OCC(PA or PEI) >\n");
		Log_Status = LOG_OCC_PA_BUSY;
		return 0;
	}
#endif

    switch(__broadcast_status) {
      case COP_STATUS_IDLE:
      case __PASSIVE_INDOOR_BCAST_START:
      case __PASSIVE_OUTDOOR_BCAST_START:
      case __PASSIVE_INOUTDOOR_BCAST_START:
      case WAITING_PASSIVE_INDOOR_BCAST_START:
      case WAITING_PASSIVE_OUTDOOR_BCAST_START:
      case __PEI2CAB_CALL_WAKEUP:
      case __PEI2CAB_CALL_START:
      case __CAB2CAB_CALL_WAKEUP:
      case __CAB2CAB_CALL_START:
      case __WAITING_CAB2CAB_CALL_START:
      case __CAB2CAB_CALL_MONITORING_START:
      case __PEI2CAB_CALL_MONITORING_START:
		if (tractor_cab2cab_start) {
			printf("< TRACTOR MODE DISABLE >\n");
			tractor_cab2cab_start = 0;
			tractor_clear_for_key();

			set_tractor_mic_off();				/* Close the MIC to Safety out */
			set_occ_mic_audio_switch_off();		/* Disable the Safety Input */

			if ( (!cab_pa_bcast_monitor_start) && 
				 (!func_bcast_monitor_start) &&
				 (!fire_bcast_monitor_start) &&
				 (!auto_bcast_monitor_start) &&
				 (!pei2cab_call_running) && (!pei2cab_call_waiting) &&
				 (!cab2cab_call_running) && (!cab2cab_call_waiting) &&
				 (!cab2cab_call_monitoring) &&
				 (!pei2cab_call_monitoring) 
				) {
				set_cop_spk_mute_onoff(1);  /* 0:Speaker ON, 1:Speaker off */
			}

			send_cop2avc_tractor_stop_flag(__broadcast_status, 0x00);
		} else {
			tractor_cab2cab_start = 1;
			tractor_set_for_key();
			printf("< TRACTOR MODE ENABLE >\n");

			/* Safety Input to Speaker */
			set_tractor_mic_audio_switch_on();
			set_occ_mic_to_spk_audio_switch_on();
			occ_mic_volume_set(occ_mic_vol_atten);

			/* Speaker ON */
			set_cop_spk_mute_onoff(0);	/* 0:Speaker ON, 1:Speaker off */

			/* COB Mic to CAB is used by CAB button */
			set_tractor_mic_base_mic();	/* COB Mic to Safty Out */
			cop_pa_and_call_mic_volume_set();

			send_cop2avc_tractor_start_flag(__broadcast_status, 0x00);
		}
		break;

      case WAITING_OCC_PA_BCAST_START:
      case OCC_PA_BCAST_START:
      case WAITING_OCC_PA_BCAST_STOP:
			Log_Status = LOG_OCC_PA_BUSY;
			break;
    }
	return ret;
}

static int cab2cab_call_request(void)
{
    int ret;

	set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
   	set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink
    ret = send_cop2avc_cmd_cab2cab_call_start_request(0);

    return ret;
}

static int cab2cab_call_accept(void)
{
    int ret;

	set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
   	set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink

    __car_num = get_call_car_num(CAB_LIST);
    __dev_num = get_call_id_num(CAB_LIST);

    ret = send_cop2avc_cmd_cab2cab_call_accept_request(0, __car_num, __dev_num);

    return ret;
}
 
int __call_cab_key_process (int key_pressed_status)
{
    int ret = 0;//, stat = 0;
    //char __msg_str[32];
    //int pressed_ctr = keyval >> 16;
    int is_pushed_key = key_pressed_status & KEY_PRESSED_MASK;//key_pressed_status;
    int __num;

	if(!cab_key_lock)
		cab_key_lock = 1;

	if(call_cab_Key_pressed_on_fire_status)
		call_cab_Key_pressed_on_fire_status = 0;
	else
		call_cab_Key_pressed_on_fire_status = 1;
	
	if (fire_alarm_key_status) {
		if(is_pushed_key)
			fire_status_reset(1);
	}

	//DBG_LOG("occ_pa_bcast_monitor_start = %d, __broadcast_status = %d\n", occ_pa_bcast_monitor_start, __broadcast_status);
	if (occ_pa_bcast_monitor_start) {
		printf("< CAB KEY, OCC-PA BUSY >\n");
		Log_Status = LOG_OCC_PA_BUSY;
		if(cab_key_lock)
			cab_key_lock = 0;
		return ret;
	}

    switch (__broadcast_status) {
      case COP_STATUS_IDLE:
		if (occ_pa_bcast_monitor_start) {
        	printf("< CAB KEY, OCC-PA BUSY >\n");
			Log_Status = LOG_OCC_PA_BUSY;
        	break;
		}
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED CAB KEY ON IDLE >\n");
            break;
        }

        if (is_waiting_emergency_pei_call_any()) {
            printf("< CAB KEY, ON IDLE, EMER PEI BUSY >\n");
			Log_Status = LOG_EMER_PEI_BUSY;
            break;
        }
        else if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
            printf("< CAB KEY, ON IDLE, PEI BUSY by PEI WAKEUP >\n");
			Log_Status = LOG_PEI_BUSY;
            break;
        }

#if 0
        if (occ_pa_bcast_rx_running) {
            printf("< CAB KEY, ON IDLE, OCC-PA BUSY >\n");
			Log_Status = LOG_OCC_PA_BUSY;
            break;
        }

        if (cop_pa_bcast_rx_running) {
            printf("< CAB KEY, ON IDLE, CAB-PA BUSY >\n");
			Log_Status = LOG_CAB_PA_BUSY;
            break;
        }
#endif

        cab2cab_call_request();

        __broadcast_status = __WAITING_CAB2CAB_CALL_START;
        cab2cab_call_pending = 1;
        cab2cab_call_waiting = 0;
        call_mon_sending = 1;

        printf("< CAB KEY, IDLE -> WAITING CAB START >\n");

        add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, -1, -1, 0, 0, 0);

		if (fire_alarm_key_status)
			fire_status_reset(1);
        force_call_menu_draw();
        break;

      case __PASSIVE_INDOOR_BCAST_START:
      case __PASSIVE_OUTDOOR_BCAST_START:
      case __PASSIVE_INOUTDOOR_BCAST_START:
      case WAITING_PASSIVE_OUTDOOR_BCAST_STOP:
      case WAITING_PASSIVE_INDOOR_BCAST_STOP:
		{
			cab2cab_call_request();

			__broadcast_status = __WAITING_CAB2CAB_CALL_START;
			cab2cab_call_pending = 1;
			cab2cab_call_waiting = 0;
			call_mon_sending = 1;

			printf("< CAB KEY, PASSIVE_INorOUT? -> WAITING CAB START >\n");
			add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, -1, -1, 0, 0, 0);

            /* move_wakeup_to_1th_index(CAB_LIST); */
            move_only_1th_wakeup_to_1th_index(CAB_LIST, 0);
		}
        break;

      case __PEI2CAB_CALL_MONITORING_START:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED CAB KEY ON PEI MON >\n");
            break;
        }

        if (cab2cab_reject_waiting) {
            printf("[ ON PEI MON, WAITING CAB REJECT ]\n");
			Log_Status = LOG_CAB_REJECT_WAIT;
            break;
        }

        if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
            printf("< CAB KEY, ON PEI-MON, PEI BUSY by PEI WAKEUP >\n");
			Log_Status = LOG_PEI_BUSY;
            break;
        }

        if (is_waiting_normal_call_any(CAB_LIST, 0, 0) == 0) {
#if 0		/* Chnaged by Michael RYU : May 15, 2015 */
            ret = cab2cab_call_request();
            call_mon_sending = 1;
            cab2cab_call_waiting = 0;

            printf("< CAB KEY, ON PEI MON, SEND CAB START REQ >\n");
            add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, -1, -1, 0, 0);

            if (is_waiting_monitoring_call(CAB_LIST, NULL, NULL, NULL, NULL, 1))
                move_only_1th_wakeup_to_1th_index(CAB_LIST, 0);

            force_call_menu_draw();
#else
			printf("< CAB KEY, ON PEI-MON, PEI BUSY by PEI WAKEUP >\n");
			Log_Status = LOG_PEI_TO_CAB_BUSY;
#endif
            break;
        }

        __car_num = get_call_car_num(CAB_LIST);
        __dev_num = get_call_id_num(CAB_LIST);
		__train_num = get_call_train_num(CAB_LIST);
        if (__car_num == (char)-1 && __dev_num == (char)-1) { // *
            if (!cab2cab_reject_waiting) {
                printf("[ CAB KEY, ON PEI MON, WAITING CAB START ]\n");
				Log_Status = LOG_CAB_TO_CAB_WAIT;
            } else {
                printf("[ ON PEI MON, WAITING CAB REJECT ]\n");
				Log_Status = LOG_CAB_REJECT_WAIT;
            }

        } else {
            cab2cab_call_monitoring = 0;
            call_mon_sending = 0;

            cop_speaker_volume_mute_set();
            close_cop_multi_monitoring();
            clear_rx_tx_mon_multi_addr();
            decoding_stop();

            __car_num = get_call_car_num(PEI_LIST);
            __dev_num = get_call_id_num(PEI_LIST);
			__train_num = get_call_train_num(PEI_LIST);

            if (is_monitoring_call(PEI_LIST, __car_num, __dev_num, 0)) { // *
                __mon_addr = get_call_mon_addr(PEI_LIST);
                ret = get_call_is_emergency();
                add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);
            }

            ret = cab2cab_call_accept();
            if (ret > 0) {
                __broadcast_status = __WAITING_CAB2CAB_CALL_START;
                cab2cab_call_pending = 1;

                cop_speaker_volume_mute_set();

                cab2cab_call_waiting = 0;

                printf("< CAB KEY, PEI-MON -> WAITING START >\n");
            }
        }

        break;

      case __WAITING_CAB2CAB_CALL_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED CAB KEY ON WAIT CAB START >\n");
            break;
        }

        if (!cab2cab_reject_waiting) {
            printf("[ ON WAITING CAB START ]\n");
			Log_Status = LOG_CAB_TO_CAB_WAIT;
        } else {
            printf("[ ON WAITING CAB REJECT ]\n");
			Log_Status = LOG_CAB_REJECT_WAIT;
        }
        break;

      case __CAB2CAB_CALL_START:
        if (is_pushed_key) {
            printf("< IGNORE PUSHED CAB KEY ON CAB START >\n");
            break;
        }

        if (cab2cab_call_running) {
			set_key_led(enum_COP_KEY_CALL_CAB);				// Added by Michael RYU(Apr. 10, 2015)
   			set_key_led_blink(enum_COP_KEY_CALL_CAB, 0);	// Changed value from '1' to '0' to disable the blink

            __car_num = get_call_car_num(CAB_LIST);
            __dev_num = get_call_id_num(CAB_LIST);

            send_cop2avc_cmd_cab2cab_call_stop_request(0, __car_num, __dev_num);

            printf("< CAB KEY, CAB-CALL START -> WAITING STOP >\n");

            cab2cab_call_running = 0; /* 2013/11/07 */

            play_stop_call_rx_data();

            __broadcast_status = WAITING_CAB2CAB_CALL_STOP;			// Changed location by MHRYU(11_02_2015)
            cab2cab_call_pending = 1;
        }
        break;

      case __CAB2CAB_CALL_WAKEUP:
#ifdef NOT_USE_CAB_CALL_WAKEUP
        printf("< BUG, CAB KEY, NOT USE CAB WAKEUP >\n");
#else
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED CAB KEY ON CAB WAKEUP >\n");
            break;
        }

        if (inout_bcast_processing) {
            printf("< CAB KEY, CAB-PA BUSY on CAB-CALL WAKEUP >\n");
			Log_Status = LOG_CAB_PA_BUSY;
            break;
        }

        if (is_waiting_normal_call(CAB_LIST, -1, -1)) // *
        {

            if (is_waiting_emergency_pei_call_any()) {
                printf("< CAB KEY, ON CAB-CALL WAKEUP, EMER PEI BUSY by EMER PEI WAKEUP >\n");
				Log_Status = LOG_EMER_PEI_BUSY;
            }
            else if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
                printf("< CAB KEY, ON CAB-CALL WAKEUP, PEI BUSY by PEI WAKEUP >\n");
				Log_Status = LOG_PEI_BUSY;
            } else {
                ret = cab2cab_call_accept();
                if (ret > 0) {
                    cop_speaker_volume_mute_set();
                    stop_play_beep();
                    decoding_stop();

                    __broadcast_status = __WAITING_CAB2CAB_CALL_START;
                    cab2cab_call_pending = 1;

                    cab2cab_call_waiting = 0;
                    printf("< CAB KEY, CAB-CALL WAKEUP -> WAITING START >\n");
                }
            }
        }
#endif
        break;

      case __CAB2CAB_CALL_MONITORING_START:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED CAB KEY ON CAB MON >\n");
            break;
        }

        if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
            printf("< CAB KEY, ON CAB MON, PEI BUSY by PEI WAKEUP >\n");
			Log_Status = LOG_PEI_BUSY;
            break;
        }

#if 1 // Ver0.76, to limit one couple CAB-CAB CALL
        __num = get_call_waiting_ctr(CAB_LIST);
        if (__num >= 2) {
            printf("< CAB KEY, CAB MON NUM LIMIT(%d) >\n", __num);
			Log_Status = LOG_CAB_TO_CAB_BUSY;
            break;
        }
#endif

        cab2cab_call_monitoring = 0;
        call_mon_sending = 0;

        play_stop_call_mon_rx_data();

        decoding_stop();

        close_cop_multi_monitoring();
        clear_rx_tx_mon_multi_addr();

        __car_num = get_call_car_num(CAB_LIST);
        __dev_num = get_call_id_num(CAB_LIST);
		__train_num = get_call_train_num(CAB_LIST);
        __mon_addr = get_call_mon_addr(CAB_LIST);
        __tx_addr = get_call_tx_addr(CAB_LIST);
#if 1
        __rx_addr_for_mon_for_push_to_talk = __tx_addr; // Ver 0.76
#endif
        add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

        if (is_waiting_normal_call_any(CAB_LIST, 0, 0)) { // *
            /*move_wakeup_to_1th_index(CAB_LIST);*/
            move_only_1th_wakeup_to_1th_index(CAB_LIST, 0);

            ret = cab2cab_call_accept();
            if (ret > 0) {
                cab2cab_call_pending = 1;
                cab2cab_call_waiting = 0;
                __broadcast_status = __WAITING_CAB2CAB_CALL_START;

                __car_num = get_call_car_num(CAB_LIST);
                __dev_num = get_call_id_num(CAB_LIST);
                printf("< CAB KEY, CAB MON -> WAITING CAB START(ACCEPT) >\n");
            }
        }
        else {
            cab2cab_call_request();
            __broadcast_status = __WAITING_CAB2CAB_CALL_START;
            cab2cab_call_pending = 1;

            printf("< CAB KEY, CAB MON -> WAITING CAB START(REQ) >\n");
            call_mon_sending = 1;
            cab2cab_call_waiting = 0;

            add_call_list(CAB_LIST, CALL_STATUS_WAKEUP, 0, 0, -1, -1, 0, 0, 0);

            /* move_wakeup_to_1th_index(CAB_LIST); */
            move_only_1th_wakeup_to_1th_index(CAB_LIST, 0);

        }

        force_call_menu_draw();

        break;

      case __PEI2CAB_CALL_WAKEUP:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED CAB KEY ON PEI WAKEUP >\n");
            break;
        }
        printf("< CAB KEY, ON PEI WAKEUP, PEI BUSY by PEI WAKEUP >\n");
		Log_Status = LOG_PEI_BUSY;

        break;

      case __PEI2CAB_CALL_START:
      case WAITING_PEI2CAB_CALL_STOP:
        if (!is_pushed_key) {
            printf("< IGNORE RELEASED CAB KEY ON PEI START or WAIT PEI STOP >\n");
            break;
        }
        if (occ_key_status == 0) {
            if (is_emergency_pei_live()) {
                printf("< CAB KEY, ON PEI-CALL, EMER PEI BUSY >\n");
				Log_Status = LOG_EMER_PEI_BUSY;
            }
            else if (is_live_call(PEI_LIST, 0, 0)) {
                printf("< CAB KEY, ON PEI-CALL, PEI CAB BUSY >\n");
				Log_Status = LOG_PEI_TO_CAB_BUSY;
            }
        }
        else if (occ_key_status)
		{
			Log_Status = LOG_PEI_TO_OCC_BUSY;
		}
        break;

      case WAITING_OCC_PA_BCAST_START:
      case OCC_PA_BCAST_START:
      case WAITING_OCC_PA_BCAST_STOP:
        printf("< CAB KEY, OCC-PA BUSY >\n");
		Log_Status = LOG_OCC_PA_BUSY;
        break;

      case WAITING_CAB2CAB_CALL_STOP:
        printf("< CAB KEY, ON WAITING CAB STOP >\n");
        break;

      default:
        printf("%s: Not defined Index: %d.\n", __func__, (int)__broadcast_status);
        break;
    }

	cab_key_lock = 0;
    return ret;
}

static int pei2cab_call_start(int is_emergency)
{
    int ret;

	__car_num = get_call_car_num(PEI_LIST);
	__dev_num = get_call_id_num(PEI_LIST);

	DBG_LOG("call start request : %d-%d, pei_call_count = %d\n", __car_num, __dev_num, pei_call_count);
	ret = send_cop2avc_cmd_pei2cab_call_start_request(0, __car_num, __dev_num, is_emergency, 0);
	return ret;
}

int __call_join_key_process(int key_pressed_status)
{
	int ret;

	usleep(10*1000);	/* 10ms waiting */
	if(pei_join_button_on) {
		ret = pei2cab_join_call_start();
		printf("< process_join_start >\n");
	}

	return ret;
}

int __call_pei_key_process (int key_pressed_status)
{
    int ret = 0, stat = 0, stat2 = 0;
    char car = 0, id = 0;
    char __msg_str[32];

	int is_pushed_key = key_pressed_status;

	pei_key_lock = 1;

	//DBG_LOG("__broadcast_status(%d), tractor_cab2cab_start(%d)\n", __broadcast_status, tractor_cab2cab_start);
	if (fire_alarm_is_use && enable_fire_alarm) {
		if(is_pushed_key)
			fire_status_reset(1);
	}

    switch(__broadcast_status) {
      case COP_STATUS_IDLE:
          printf("< PEI KEY, N/A(IDLE) >\n");
          break;

      case __PASSIVE_INDOOR_BCAST_START:
      case __PASSIVE_OUTDOOR_BCAST_START:
      case __PASSIVE_INOUTDOOR_BCAST_START:
      case WAITING_PASSIVE_INDOOR_BCAST_START:
      case WAITING_PASSIVE_OUTDOOR_BCAST_START:
          if (__broadcast_status == __PASSIVE_INDOOR_BCAST_START)
              strcpy(__msg_str, "IN");
          else if (__broadcast_status == __PASSIVE_OUTDOOR_BCAST_START)
              strcpy(__msg_str, "OUT");
          else if (__broadcast_status == __PASSIVE_INOUTDOOR_BCAST_START)
              strcpy(__msg_str, "INOUT");
          else if (__broadcast_status == WAITING_PASSIVE_INDOOR_BCAST_START)
              strcpy(__msg_str, "WAITING IN");
          else if (__broadcast_status == WAITING_PASSIVE_OUTDOOR_BCAST_START)
              strcpy(__msg_str, "WAITING OUT");

          if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
              move_only_1th_wakeup_to_1th_index(PEI_LIST, 0);
			  /* >>>> VOIP */
			  //cop_voip_call_codec_init();
			  voip_call_start_prepare();
			  //cop_voip_call_codec_start();
			  reset_voip_frame_info();
			  //SIP_PEICall();
			  //usleep(100000);	// invite from COP (before sending call_start_cmd to AVC)
			  /* VOIP <<<< */
              ret = pei2cab_call_start(0);
              if (ret > 0) {
                  __broadcast_status = __WAITING_PEI2CAB_CALL_START;
                  pei2cab_call_pending = 1;
                  printf("< PEI KEY, ON %s -> WAITING PEI START(%s) >\n", __msg_str,
                      (enable_pei_wakeup_on_in_passive | enable_pei_wakeup_on_out_passive) ? "STANDBY CAB-PA": "NORMAL");
              }
          }
          break;

      case __PEI2CAB_CALL_WAKEUP:
		if(pei_call_count != 0) {
            printf("PEI call count still waiting.....(%d)\n", pei_call_count);
			break;
		}

        stat = 0;
        if (is_waiting_normal_call(CAB_LIST, -1, -1)) {
            car = get_call_car_num(CAB_LIST);
            id = get_call_id_num(CAB_LIST);
            if (car == (char)-1 && id == (char)-1)
                stat = 1;
        }

        if (stat == 0) {
            pei2cab_call_waiting = 0;

            stat2 = is_waiting_emergency_pei_call_any();
            ret = pei2cab_call_start(stat2);

            cop_speaker_volume_mute_set();
            stop_play_beep();
            decoding_stop();

            pei2cab_call_pending = 1;
            cab2cab_reject_waiting = 0;
            __broadcast_status = __WAITING_PEI2CAB_CALL_START;

            printf("< PEI KEY, %s PEI WAKEUP -> WAITING START >\n", stat2 ? "EMER" : "NORMAL");
        }
        else if (stat == 1) {
            cop_speaker_volume_mute_set();
            decoding_stop();
            stop_play_beep();

            __broadcast_status = __WAITING_PEI2CAB_CALL_START;
            ret = send_cop2avc_cmd_cab2cab_call_reject_request(0, 0, 0);
            if (ret > 0) {
                pei2cab_call_waiting = 1;
                pei2cab_call_pending = 0;
                cab2cab_reject_waiting = 1;
                printf("< PEI KEY, PEI_WAKEUP & WAITING CAB START -> WAITING CAB REJECT >\n");
            }
        }

        break;

#if 0
      case __WAITING_PEI2CAB_CALL_START:
	stat = del_call_list(PEI_LIST, 0, 0, -1, -1);
        if (stat != 0)
            break;

        set_key_led_blink(enum_COP_KEY_CALL_PEI, 0);
        __broadcast_status = WAITING_PEI2CAB_CALL_STOP;
        pei2cab_call_pending = 0;

        break;
#endif

      case __PEI2CAB_CALL_START:
        if (pei2cab_call_running == 1 /*&& occ_key_status == 0*/)
        {
            play_stop_call_rx_data();

			if(pei_call_count >= 2) {
				//DBG_LOG("Calling play_stop_call_rx_data_2nd() \n");
				play_stop_call_rx_data_2nd();
			}

            if (pei_comm_half_duplex)
				Log_Status = LOG_READY;

            set_key_led_blink(enum_COP_KEY_CALL_PEI, 1);

            __car_num = get_call_car_num(PEI_LIST);
            __dev_num = get_call_id_num(PEI_LIST);
            stat = get_call_is_emergency();


			//DBG_LOG("Call stop request : %d-%d\n", __car_num, __dev_num);
            ret = send_cop2avc_cmd_pei2cab_call_stop_request(0, __car_num, __dev_num, stat, 0);
            if (stat)
                printf("\n< PEI KEY, EMER PEI START -> WAITING STOP(%d-%d) > \n", __car_num, __dev_num);
            else
                printf("\n< PEI KEY, NORMAL PEI START -> WAITING STOP(%d-%d) > \n", __car_num, __dev_num);

            __broadcast_status = WAITING_PEI2CAB_CALL_STOP;
            pei2cab_call_pending = 1;

            /* 2013/11/07 */
            //pei2cab_call_running = 0;

			/* >>>> VOIP */
			//cop_voip_call_codec_stop();
			//reset_voip_frame_info();
			/* VOIP <<<< */

        }
		/*
        else if (pei2cab_call_running == 1 && occ_key_status) {
			Log_Status = LOG_PEI_TO_OCC_BUSY;
        }
		*/
        break;

      case __WAITING_PEI2CAB_CALL_START:
        printf("< PEI KEY, ON WAITING PEI START >\n");
        break;

      case WAITING_PEI2CAB_CALL_STOP:
        printf("< PEI KEY, ON WAITING PEI STOP >\n");
        break;

      case __PEI2CAB_CALL_MONITORING_START:
        if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
            printf("\n< PEI KEY, ON PEI-MON -> WAITING PEI START >\n");

            play_stop_call_mon_rx_data();

            decoding_stop();

            pei2cab_call_monitoring = 0;
            call_mon_sending = 0;

			// mhryu : 05202015
			if (pei2cab_monitoring_on) {
				close_call_multi_tx();
				pei2cab_monitoring_on = 0;
			}

            close_cop_multi_monitoring();
            clear_rx_tx_mon_multi_addr();

            __car_num = get_call_car_num(PEI_LIST);
            __dev_num = get_call_id_num(PEI_LIST);
            __mon_addr = get_call_mon_addr(PEI_LIST);
			__train_num = get_call_train_num(PEI_LIST);

            ret = get_call_is_emergency();
            add_call_list(PEI_LIST, CALL_STATUS_MON_WAIT, 0, ret, __car_num, __dev_num, __mon_addr, 0, __train_num);

            /*move_wakeup_to_1th_index(PEI_LIST);*/
            move_only_1th_wakeup_to_1th_index(PEI_LIST, 0);

            force_call_menu_draw();

			/* >>>> VOIP */
			//cop_voip_call_codec_init();
			voip_call_start_prepare();
			//cop_voip_call_codec_start();
			reset_voip_frame_info();
			/* VOIP <<<< */

            ret = pei2cab_call_start(0);
            __broadcast_status = __WAITING_PEI2CAB_CALL_START;
            pei2cab_call_pending = 1;
            pei2cab_call_waiting = 0;
        }
        break;

      case __CAB2CAB_CALL_MONITORING_START:
        if (is_waiting_normal_call_any(PEI_LIST, 0, 0))
        {
            pei2cab_call_monitoring = 0;
            call_mon_sending = 0;

            cop_speaker_volume_mute_set();
            close_cop_multi_monitoring();
            clear_rx_tx_mon_multi_addr();
            decoding_stop();

            __car_num = get_call_car_num(CAB_LIST);
            __dev_num = get_call_id_num(CAB_LIST);
			__train_num = get_call_train_num(CAB_LIST);
            __mon_addr = get_call_mon_addr(CAB_LIST);
            __tx_addr = get_call_tx_addr(CAB_LIST);
            add_call_list(CAB_LIST, CALL_STATUS_MON_WAIT, 0, 0, __car_num, __dev_num, __mon_addr, __tx_addr, __train_num);

            force_call_menu_draw();

            ret = pei2cab_call_start(0);
            if (ret > 0) {
                __broadcast_status = __WAITING_PEI2CAB_CALL_START;
                pei2cab_call_pending = 1;
                pei2cab_call_waiting = 0;
                printf("< PEI KEY, ON CAB-MON -> WAITING PEI START >\n");
            }
        }
        break;

      case __CAB2CAB_CALL_START:
        if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
            printf("< PEI KEY, CAB to CAB BUSY >\n");
			Log_Status = LOG_CAB_TO_CAB_BUSY;
        }
        break;

      case __CAB2CAB_CALL_WAKEUP:
#ifdef NOT_USE_CAB_CALL_WAKEUP
        printf("< BUG, PEI KEY, NOT USE CAB WAKEUP >\n");
#else
        if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
            pei2cab_call_waiting = 0;
            ret = pei2cab_call_start(0);
            if (ret > 0) {
                __broadcast_status = __WAITING_PEI2CAB_CALL_START;
                pei2cab_call_pending = 1;
                printf("< PEI KEY, CALL_WAKEUP & PEI WAKEUP -> WAITING PEI START >\n");
            }
        }
#endif
        break;

      case __WAITING_CAB2CAB_CALL_START:
        if (is_waiting_normal_call(PEI_LIST, -1, -1)) {
            cop_speaker_volume_mute_set();
            stop_play_beep();

            ret = send_cop2avc_cmd_cab2cab_call_reject_request(0, 0, 0);
            if (ret > 0) {
                pei2cab_call_waiting = 1;
                pei2cab_call_pending = 0;
                cab2cab_reject_waiting = 1;
                __broadcast_status = __WAITING_PEI2CAB_CALL_START;
                printf("< PEI KEY, ON WAITING CAB & PEI WAKEUP, SEND CAB REJECT -> WAITING PEI-CALL START >\n");
            }
        }
        break;

      case WAITING_CAB2CAB_CALL_STOP:
        printf("< PEI KEY, N/A >\n");
        break;

      case WAITING_OCC_PA_BCAST_START:
      case OCC_PA_BCAST_START:
#if 0
#if 0
          printf("< PEI KEY, WAITING PEI START on OCC-PA >\n");
          enable_pei2cab_simultaneous_on_occ_pa = 1;
#endif

        if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) {
            printf("< PEI KEY, OCC-PA BUSY >\n");
			Log_Status = LOG_OCC_PA_BUSY;
        }
        else if (is_waiting_emergency_pei_call() && occ_pa_running) {
            cop_speaker_volume_mute_set();
            decoding_stop();

            ret = send_cop2avc_cmd_occ_pa_stop_request(0);
            if (ret > 0) {
                //set_key_led_blink(enum_COP_KEY_OCC, 1);
                occ_pa_pending = 1;
                __broadcast_status = WAITING_OCC_PA_BCAST_STOP;
                now_occ_pa_stop_wait = 1;
                now_occ_pa_start = 0;
                printf("< PEI KEY, OCC-PA+EMER PEI WAKEUP -> WAITING OCC-PA STOP >\n");
            }
        }
#else
		if (is_waiting_normal_call_any(PEI_LIST, 0, 0)) { 
			move_only_1th_wakeup_to_1th_index(PEI_LIST, 0);
			/* >>>> VOIP */
			//cop_voip_call_codec_init();
			voip_call_start_prepare();
			//cop_voip_call_codec_start();
			reset_voip_frame_info();
			//SIP_PEICall();
			//usleep(100000); // invite from COP (before sending call_start_cmd to AVC)
			/* VOIP <<<< */
			ret = pei2cab_call_start(0);
			if (ret > 0) {
				__broadcast_status = __WAITING_PEI2CAB_CALL_START;
				pei2cab_call_pending = 1;
                printf("< PEI KEY, OCC-PA+EMER PEI WAKEUP -> WAITING PEI2CAB_CALL_START >\n");
			}     

			set_occ_mic_audio_switch_on();
			set_occ_mic_to_recode_audio_switch_on();
		}
#endif
		break;

      case WAITING_OCC_PA_BCAST_STOP:
        printf("< PEI KEY, OCC-PA BUSY >\n");
		Log_Status = LOG_OCC_PA_BUSY;
        break;

      default:
        printf("%s: Not defined Index: %d.\n", __func__, (int)__broadcast_status);
        break;
    }

	pei_key_lock = 0;
    return ret;
}

static int add_call_list(int where, short status, int is_start_requested, int emergency, char car_n, char dev_n,
                            int mon_multi_addr, int tx_multi_addr, int train_n)
{
    struct multi_call_status *list;
    int i, idx, max, ctr, n = 0;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");

        /*if (dev_n != (char)-1 && dev_n > MAX_CAB_ID_NUM)
            dev_n = 0;*/
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");

        /*if (dev_n != (char)-1 && dev_n > MAX_PEI_ID_NUM)
            dev_n = 0;*/
    }
    else
        return -1;

    /*if (car_n != (char)-1 && car_n > MAX_CAR_NUM)
        car_n = 0;*/

    /*if (car_n == 0 || dev_n == 0) {
        printf("< ADD %s ERR (%d-%d) >\n", base_name, car_n, dev_n);
		Log_Status = LOG_CAB_ID_ERR;
        return -1;
    }*/

    //Log_Status = LOG_READY; // ???

    if (status == CALL_STATUS_WAKEUP) {
        if (list->ctr < max) {
            list->ctr++;
            list->info[list->in_ptr].car_id = car_n;
            list->info[list->in_ptr].idx_id = dev_n;
			list->info[list->in_ptr].train_num = train_n;
            list->info[list->in_ptr].status = status;
            list->info[list->in_ptr].is_start_requested = is_start_requested;
            list->info[list->in_ptr].emergency = emergency;
            list->in_ptr++;
            if (list->in_ptr == max)
                list->in_ptr = 0;

            //DBG_LOG("< ADD %s %d-%d(ctr:%d,in:%d,out:%d), WAKE-UP EMER:%d >\n",
            //        base_name, car_n, dev_n, list->ctr, list->in_ptr, list->out_ptr, emergency);
            printf("< ADD(%d) %s %d-%d(TS#%d), WAKE-UP EMER:%d >\n", list->in_ptr, base_name, car_n, dev_n, train_n, emergency);
        }
    }
    else if (status == CALL_STATUS_LIVE) {
        idx = list->out_ptr;
        for (i = 0; i < list->ctr; i++) {
            if (  (   (list->info[idx].car_id == car_n)
                   && (list->info[idx].idx_id == dev_n)
                   && (list->info[idx].status == CALL_STATUS_WAKEUP)
                   && (list->info[idx].emergency == emergency)
                  )
               || (   (list->info[idx].car_id == (char)-1)
                   && (list->info[idx].idx_id == (char)-1)
                   && (list->info[idx].status == CALL_STATUS_WAKEUP)
                   && (list->info[idx].emergency == emergency)
                  )
               )
            {
				DBG_LOG("idx = %d\n", idx);
                list->info[idx].car_id = car_n;
                list->info[idx].idx_id = dev_n;
				list->info[idx].train_num = train_n;
                list->info[idx].status = status;
                list->info[idx].is_start_requested = 0;
                //DBG_LOG("< ADD %s %d-%d(ctr:%d,in:%d,out:%d), LIVE >\n",
                //    base_name, car_n, dev_n, list->ctr, list->in_ptr, list->out_ptr);
                printf("< ADD(%d) %s %d-%d(TS#%d), LIVE, EMER:%d >\n", idx, base_name, car_n, dev_n, train_n, list->info[idx].emergency);
                break;
            }

            idx++;
            if (idx == max) idx = 0;
        }
    }
    else if (status == CALL_STATUS_MON) {
        idx = list->out_ptr;

        for (i = 0; i < list->ctr; i++) {
            if (  (   (list->info[idx].car_id == car_n)
                   && (list->info[idx].idx_id == dev_n)
                   && (   (list->info[idx].status == CALL_STATUS_WAKEUP)
                       || (list->info[idx].status == CALL_STATUS_MON_WAIT)
                      )
                   && (list->info[idx].emergency == emergency)
                  )
               )
            {
                list->info[idx].status = status;
                list->info[idx].is_start_requested = 0;
                list->info[idx].mon_addr = mon_multi_addr;
                list->info[idx].tx_addr = tx_multi_addr;
                //printf("< ADD %s %d-%d(ctr:%d,in:%d,out:%d,M:%X), MON >\n",
                //    base_name, car_n, dev_n, list->ctr, list->in_ptr, list->out_ptr, list->info[idx].mon_addr);
                printf("< ADD(%d) %s %d-%d(TS#%d), MON >\n", idx, base_name, car_n, dev_n, train_n);
                break;
            }

            idx++;
            if (idx == max) idx = 0;
        }
    }
    else if (status == CALL_STATUS_MON_WAIT) {
        idx = list->out_ptr;

        for (i = 0; i < list->ctr; i++) {
            if (  (   (list->info[idx].car_id == car_n)
                   && (list->info[idx].idx_id == dev_n)
                   && (/*  (list->info[idx].status == CALL_STATUS_WAKEUP)
                       ||*/(list->info[idx].status == CALL_STATUS_MON)
                      )
                   && (list->info[idx].emergency == emergency)
                  )
               )
            {
                list->info[idx].status = status;
                list->info[idx].is_start_requested = 0;
                list->info[idx].mon_addr = mon_multi_addr;
                list->info[idx].tx_addr = tx_multi_addr;
                //printf("< ADD %s %d-%d(ctr:%d,in:%d,out:%d,M:0x%X), WAITING MON >\n",
                //    base_name, car_n, dev_n, list->ctr, list->in_ptr, list->out_ptr, list->info[idx].mon_addr);
                printf("< ADD(%d) %s %d-%d, WAITING MON >\n", idx, base_name, car_n, dev_n);
                break;
            }

            idx++;
            if (idx == max) idx = 0;
        }
    }

    if (where == PEI_LIST) {
        n = is_waiting_call(PEI_LIST);
		//DBG_LOG("n = %d(is_waiting_call(PEI_LIST))\n", n);
        if (n >= 2) {
            ctr = list->ctr;
            idx = list->out_ptr + ctr;
			//DBG_LOG("ctr = %d, idx=%d\n", ctr, idx);
//printf(" == idx: %d\n", idx);
//printf("== list->info[idx-1].status:%d\n", list->info[idx-1].status);
//printf("== list->info[idx-2].status:%d\n", list->info[idx-2].status);
//printf("== list->info[idx-1].emergency:%d\n", list->info[idx-1].emergency);
//printf("== list->info[idx-2].emergency:%d\n", list->info[idx-2].emergency);

            if (   (list->info[idx-1].status == CALL_STATUS_WAKEUP)
                && (list->info[idx-1].emergency == 1)
               )
            {
                idx = list->out_ptr;

                for (i = 0; i < ctr; i++) {
                    if (   (list->info[idx].status == CALL_STATUS_WAKEUP)
                        && (list->info[idx].emergency == 0))
                        break;
                    idx++;
                    if (idx == max) idx = 0;
                }

                n = idx;

                idx = list->out_ptr + ctr;

                /* MAX_PEI_CALL_NUM-1: Termperary */
                list->info[MAX_PEI_CALL_NUM-1].car_id = list->info[idx-1].car_id;
                list->info[MAX_PEI_CALL_NUM-1].idx_id = list->info[idx-1].idx_id;
				list->info[MAX_PEI_CALL_NUM-1].train_num = list->info[idx-1].train_num;
                list->info[MAX_PEI_CALL_NUM-1].status = list->info[idx-1].status;
                list->info[MAX_PEI_CALL_NUM-1].is_start_requested = list->info[idx-1].is_start_requested;
                list->info[MAX_PEI_CALL_NUM-1].emergency = list->info[idx-1].emergency;
                list->info[MAX_PEI_CALL_NUM-1].mon_addr = list->info[idx-1].mon_addr;
                list->info[MAX_PEI_CALL_NUM-1].tx_addr = list->info[idx-1].tx_addr;

                for (i = ctr-1; i > n ; i--) {
					DBG_LOG("i = %d, %d-%d(%d), TS#%d\n", i, list->info[i-1].car_id, list->info[i-1].idx_id, list->info[i-1].status, list->info[i-1].train_num);
                    list->info[i].car_id = list->info[i-1].car_id;
                    list->info[i].idx_id = list->info[i-1].idx_id;
					list->info[i].train_num = list->info[i-1].train_num;
                    list->info[i].status = list->info[i-1].status;
                    list->info[i].is_start_requested = list->info[i-1].is_start_requested;
                    list->info[i].emergency = list->info[i-1].emergency;
                    list->info[i].mon_addr = list->info[i-1].mon_addr;
                    list->info[i].tx_addr = list->info[i-1].tx_addr;
                }

                idx = list->out_ptr;

                list->info[n].car_id = list->info[MAX_PEI_CALL_NUM-1].car_id;
                list->info[n].idx_id = list->info[MAX_PEI_CALL_NUM-1].idx_id;
				list->info[n].train_num = list->info[MAX_PEI_CALL_NUM-1].train_num;
                list->info[n].status = list->info[MAX_PEI_CALL_NUM-1].status;
                list->info[n].is_start_requested = list->info[MAX_PEI_CALL_NUM-1].is_start_requested;
                list->info[n].emergency = list->info[MAX_PEI_CALL_NUM-1].emergency;
                list->info[n].mon_addr = list->info[MAX_PEI_CALL_NUM-1].mon_addr;
                list->info[n].tx_addr = list->info[MAX_PEI_CALL_NUM-1].tx_addr;
            }
        }
    }

#if 0
{
char c, d;
short s, e;
int idx, m;
idx = list->out_ptr;
for (i = 0; i < list->ctr; i++) {
    c = list->info[idx].car_id;
    d = list->info[idx].idx_id;
    s = list->info[idx].status;
    e = list->info[idx].emergency;
    m = list->info[idx].mon_addr;
    printf("< After ADD, %s %d-%d s:%02d e:%d i:%d M:%X >\n", base_name, c, d, s, e, idx, m);
    idx++;
}
}
#endif

	if (where == PEI_LIST) {
		move_only_2nd_live_call_to_2nd_index(PEI_LIST, 0);
		update_pei_join_active_status();
	}

	printf("** add_call_list **\n");
	print_call_list(where);

    return 0;
}

static int del_call_list(int where, int emergency, int check_status, char car_n, char dev_n)
{
    struct multi_call_status *list = NULL;
    int i, j, idx, ctr, max = 0;
    char base_name[8];
    int ret = -1;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else return -1;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (   (car_n == (char)-1 && dev_n == (char)-1)
        /*|| (car_n == 0 && dev_n == 0)*/
       )
    {
        if (ctr > 0) {
            car_n = list->info[idx].car_id;
            dev_n = list->info[idx].idx_id;
        }
    }

    for (i = 0; i < ctr; i++) {
        if (   (list->info[idx].car_id == car_n)
            && (list->info[idx].idx_id == dev_n)
            && (list->info[idx].emergency == emergency)
           )
        {
            if (check_status && (list->info[idx].status != check_status)) {
                idx++;
                if (idx == max) idx = 0;
                continue;
            }

            list->info[idx].car_id = 0;
            list->info[idx].idx_id = 0;
			list->info[idx].train_num = 0;
            list->info[idx].status = 0;
            list->info[idx].emergency = 0;
            list->info[idx].mon_addr = 0;
            list->info[idx].tx_addr = 0;

            list->ctr--;

            if (i == 0) {
                list->out_ptr++;
                if (list->out_ptr == max) list->out_ptr = 0;
            }
            else {
                for (j = i; j < list->ctr; j++) {
                    list->info[idx].car_id = list->info[idx+1].car_id;
                    list->info[idx].idx_id = list->info[idx+1].idx_id;
                    list->info[idx].train_num = list->info[idx+1].train_num;
                    list->info[idx].status = list->info[idx+1].status;
                    list->info[idx].is_start_requested = list->info[idx+1].is_start_requested;
                    list->info[idx].emergency = list->info[idx+1].emergency;
                    list->info[idx].mon_addr = list->info[idx+1].mon_addr;
                    list->info[idx].tx_addr = list->info[idx+1].tx_addr;

                    idx++; if (idx == max) idx = 0;
                }

                if (list->in_ptr == 0)
                    list->in_ptr = max - 1;
                else
                    list->in_ptr--;
            }

            //printf("< DEL %s %d-%d(i:%d,ctr:%d,in:%d,out:%d,S:%d) >\n",
            //        base_name, car_n, dev_n, i,list->ctr,list->in_ptr,list->out_ptr, list->info[idx].status);
            printf("< DEL %s %d-%d >\n", base_name, car_n, dev_n);


            ret = 0;
            break;
        }
        idx++;
        if (idx == max) idx = 0;
    }

#if 0
{
char c, d;
short s;
int idx;
idx = list->out_ptr;
for (i = 0; i < list->ctr; i++) {
    c = list->info[idx].car_id;
    d = list->info[idx].idx_id;
    s = list->info[idx].status;
    printf("< After DEL, LIST %s %d-%d %d >\n", base_name, c, d, s);
    idx++;
}
}
#endif

    if (list->ctr == 0) {
        list->in_ptr = 0;
        list->out_ptr = 0;
    }

    if (ret != 0)
        printf("< DEL IGNOR, LIST %s NO ID %d-%d >\n", base_name, car_n, dev_n);

	if (where == PEI_LIST) {
		move_only_2nd_live_call_to_2nd_index(PEI_LIST, 0);
		update_pei_join_active_status();
	}

	printf("** del_call_list **\n");
	print_call_list(where);

    return ret;
}

#if 0
static int del_list_1th_item(int where)
{
    struct multi_call_status *list = NULL;
    int idx;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
    }
    else return -1;

    idx = list->out_ptr;

    list->info[idx].car_id = 0;
    list->info[idx].idx_id = 0;
    list->info[idx].status = 0;
    list->info[idx].emergency = 0;
    list->info[idx].mon_addr = 0;
    list->info[idx].tx_addr = 0;

    list->ctr--;

    if (list->ctr == 0) {
        list->in_ptr = 0;
        list->out_ptr = 0;
    }

    return 1;
}
#endif

#if 0
static int move_wakeup_to_1th_index(int where)
{
    struct multi_call_status *list = NULL;
    struct multi_call_status *tmp_list = NULL;
    int i, m, idx, ctr;
    char base_name[8];
    int ret = -1;
    //struct call_info tmp_callinfo;


    if (where == CAB_LIST) {
        list = &__cab_call_list;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        strcpy(base_name, "PEI");
    }
    else return -1;

    tmp_list = &__tmp_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;
    m = idx;

    if (ctr >= 2 && (is_waiting_normal_call_any(where, 0, 0) < ctr)) {
        for (i = idx; i < (idx+ctr); i++) {
            tmp_list->info[i].car_id = list->info[i].car_id;
            tmp_list->info[i].idx_id = list->info[i].idx_id;
            tmp_list->info[i].status = list->info[i].status;
            tmp_list->info[i].emergency = list->info[i].emergency;
            tmp_list->info[i].is_start_requested = list->info[i].is_start_requested;
            tmp_list->info[i].mon_addr = list->info[i].mon_addr;
            tmp_list->info[i].tx_addr = list->info[i].tx_addr;
        }

        for (i = idx; i < (idx+ctr); i++) {
            if (tmp_list->info[i].status == CALL_STATUS_WAKEUP) {
printf("%s, 1, m:%d, i:%d, car:%d-%d, S:%d\n", __func__, m, i, list->info[i].car_id, list->info[i].idx_id, list->info[i].status);
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

                tmp_list->info[i].status = 0;
                m++;
            }
        }

        for (i = idx; i < (idx+ctr); i++) {
            if (tmp_list->info[i].status) {
printf("%s, 2, m:%d, i:%d, car:%d-%d, S:%d\n", __func__, m, i, list->info[i].car_id, list->info[i].idx_id, list->info[i].status);
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

                tmp_list->info[i].status = 0;
                m++;
            }
        }
    }

#if 1
{
char c, d;
short s;
int idx;
idx = list->out_ptr;
for (i = 0; i < list->ctr; i++) {
    c = list->info[idx].car_id;
    d = list->info[idx].idx_id;
    s = list->info[idx].status;
    printf("< After MOVE, %s %d-%d S:%d >\n", base_name, c, d, s);
    idx++;
}
}
#endif

    return ret;
}
#endif

/*
static int move_only_2nd_live_call_to_1st_index(int where, int is_emergency, int car_num, int dev_num)
{
	struct multi_call_status *list = NULL;
	struct multi_call_status *tmp_list = NULL;
    int i, m, idx, ctr;
    char base_name[8];
    int ret = -1;
    //struct call_info tmp_callinfo;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        strcpy(base_name, "PEI");
    }
    else return -1;

    tmp_list = &__tmp_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;
    m = idx;

    if (ctr >= 2)
    {
        for (i = idx; i < (idx+ctr); i++) {
            tmp_list->info[i].car_id = list->info[i].car_id;
            tmp_list->info[i].idx_id = list->info[i].idx_id;
            tmp_list->info[i].status = list->info[i].status;
            tmp_list->info[i].emergency = list->info[i].emergency;
            tmp_list->info[i].is_start_requested = list->info[i].is_start_requested;
            tmp_list->info[i].mon_addr = list->info[i].mon_addr;
            tmp_list->info[i].tx_addr = list->info[i].tx_addr;
        }

        for (i = idx; i < (idx+ctr); i++) {
            if (   (tmp_list->info[i].status == CALL_STATUS_LIVE) 
				&& (tmp_list->info[i].car_id == car_num) 
				&& (tmp_list->info[i].idx_id == dev_num) )
			{
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

				tmp_list->info[i].status = 0;
				m++;
			}
		}

        for (i = idx; i < (idx+ctr); i++) {
            if (tmp_list->info[i].status == CALL_STATUS_LIVE) {
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

                tmp_list->info[i].status = 0;
				m++;
            }
        }

        for (i = idx; i < (idx+ctr); i++) {
            if (tmp_list->info[i].status) {
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

                tmp_list->info[i].status = 0;
                m++;
            }
        }
    }

    return ret;
}
*/

static int move_only_2nd_live_call_to_2nd_index(int where, int is_emergency)
{
	struct multi_call_status *list = NULL;
	struct multi_call_status *tmp_list = NULL;
    int i, m, idx, ctr;
    char base_name[8];
    int ret = -1;
    //struct call_info tmp_callinfo;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        strcpy(base_name, "PEI");
    }
    else return -1;

    tmp_list = &__tmp_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;
    m = idx;

    if (ctr >= 2)
    {
        for (i = idx; i < (idx+ctr); i++) {
            tmp_list->info[i].car_id = list->info[i].car_id;
            tmp_list->info[i].idx_id = list->info[i].idx_id;
            tmp_list->info[i].status = list->info[i].status;
            tmp_list->info[i].emergency = list->info[i].emergency;
            tmp_list->info[i].is_start_requested = list->info[i].is_start_requested;
            tmp_list->info[i].mon_addr = list->info[i].mon_addr;
            tmp_list->info[i].tx_addr = list->info[i].tx_addr;
        }

        for (i = idx; i < (idx+ctr); i++) {
            if (tmp_list->info[i].status == CALL_STATUS_LIVE)
            {
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

                tmp_list->info[i].status = 0;
				m++;
            }
        }

#if 0
        for (i = idx; i < (idx+ctr); i++) {
            if (   tmp_list->info[i].status == CALL_STATUS_WAKEUP
                && tmp_list->info[i].emergency == is_emergency
               )
            {
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

                tmp_list->info[i].status = 0;
                m++;
            }
        }
#endif

        for (i = idx; i < (idx+ctr); i++) {
            if (tmp_list->info[i].status) {
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;

                tmp_list->info[i].status = 0;
                m++;
            }
        }
    }

    return ret;
}

static int move_only_1th_wakeup_to_1th_index(int where, int is_emergency)
{
    struct multi_call_status *list = NULL;
    struct multi_call_status *tmp_list = NULL;
    int i, m, idx, ctr, max;
    char base_name[8];
    int ret = -1;
    //struct call_info tmp_callinfo;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
		max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
		max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else return -1;

    tmp_list = &__tmp_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;
    m = idx;


    //if (ctr >= 2 && (is_waiting_normal_call_any(where, 0, 0) < ctr))
    if (ctr >= 2)
    {
		memset(tmp_list, 0, sizeof(struct multi_call_status));

        for (i = 0; i < ctr; i++) {
            tmp_list->info[i].car_id = list->info[idx].car_id;
            tmp_list->info[i].idx_id = list->info[idx].idx_id;
            tmp_list->info[i].status = list->info[idx].status;
            tmp_list->info[i].emergency = list->info[idx].emergency;
            tmp_list->info[i].is_start_requested = list->info[idx].is_start_requested;
            tmp_list->info[i].mon_addr = list->info[idx].mon_addr;
            tmp_list->info[i].tx_addr = list->info[idx].tx_addr;
			tmp_list->info[i].train_num = list->info[idx].train_num;

			idx++;
			if(idx == max) idx = 0;
        }

		memset(list, 0, sizeof(struct multi_call_status));
		m = 0; 
		idx = 0;
		list->in_ptr = 0;
		list->out_ptr = 0;
		list->ctr = ctr;

        for (i = 0; i < ctr; i++) {
            if (tmp_list->info[i].status == CALL_STATUS_LIVE)
            {
//printf("%s, 1, m:%d, i:%d, car:%d-%d, S:%d\n", __func__, m, i, list->info[i].car_id, list->info[i].idx_id, list->info[i].status);
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;
				list->info[m].train_num = tmp_list->info[i].train_num;

                tmp_list->info[i].status = 0;
                m++;
                break; // <<===
            }
        }

        for (i = 0; i < ctr; i++) {
            if (   tmp_list->info[i].status == CALL_STATUS_WAKEUP
                && tmp_list->info[i].emergency == is_emergency
               )
            {
//printf("%s, 2, m:%d, i:%d, car:%d-%d, S:%d\n", __func__, m, i, list->info[i].car_id, list->info[i].idx_id, list->info[i].status);
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;
				list->info[m].train_num = tmp_list->info[i].train_num;

                tmp_list->info[i].status = 0;
                m++;
                //break; // <<=== // delete break, 2013/11/20, to move over one emergency
            }
        }

        for (i = 0; i < ctr; i++) {
            if (tmp_list->info[i].status) {
//printf("%s, 3, m:%d, i:%d, car:%d-%d, S:%d\n", __func__, m, i, list->info[i].car_id, list->info[i].idx_id, list->info[i].status);
                list->info[m].car_id = tmp_list->info[i].car_id;
                list->info[m].idx_id = tmp_list->info[i].idx_id;
                list->info[m].status = tmp_list->info[i].status;
                list->info[m].emergency = tmp_list->info[i].emergency;
                list->info[m].is_start_requested = tmp_list->info[i].is_start_requested;
                list->info[m].mon_addr = tmp_list->info[i].mon_addr;
                list->info[m].tx_addr = tmp_list->info[i].tx_addr;
				list->info[m].train_num = tmp_list->info[i].train_num;

                tmp_list->info[i].status = 0;
                m++;
            }
        }
		list->in_ptr = m;
    }

#if 0
{
char c, d;
short s;
int idx;
idx = list->out_ptr;
for (i = 0; i < list->ctr; i++) {
    c = list->info[idx].car_id;
    d = list->info[idx].idx_id;
    s = list->info[idx].status;
    printf("< After 1TH WAKE MOVE, %s %d-%d S:%d >\n", base_name, c, d, s);
    idx++;
}
}
#endif
	printf("** %s **\n", __func__);
	print_call_list(where);

    return ret;
}

#if 0
static int move_emergency_pei_wakeup_to_1th_index(void)
{
    struct multi_call_status *list;
    int i, idx, ctr, max;
    int pos = 0;
    int ret = -1;
    int waiting = 0;
    struct call_info tmp_callinfo;

    list = &__pei_call_list;
    max = MAX_PEI_CALL_NUM;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr >= 2) {
        for (i = 0; i < ctr; i++) {
            if (   list->info[idx].status == CALL_STATUS_WAKEUP
                && list->info[idx].emergency == 1
               )
            {
                waiting = 1;
                pos = idx;
                break;
            }
            idx++;
            if (idx == max) idx = 0;
        }

        if (waiting == 1) {
            tmp_callinfo.car_id = list->info[pos].car_id;
            tmp_callinfo.idx_id = list->info[pos].idx_id;
            tmp_callinfo.status = list->info[pos].status;
            tmp_callinfo.emergency = list->info[pos].emergency;
            tmp_callinfo.is_start_requested = list->info[pos].is_start_requested;
            tmp_callinfo.mon_addr = list->info[pos].mon_addr;

            idx = list->out_ptr;
            for (i = pos; i > idx; i--) {
                list->info[i].car_id = list->info[i-1].car_id;
                list->info[i].idx_id = list->info[i-1].idx_id;
                list->info[i].status = list->info[i-1].status;
                list->info[i].emergency = list->info[i-1].emergency;
                list->info[i].is_start_requested = list->info[i-1].is_start_requested;
                list->info[i].mon_addr = list->info[i-1].mon_addr;
            }

            idx = list->out_ptr;
            list->info[idx].car_id = tmp_callinfo.car_id;
            list->info[idx].idx_id = tmp_callinfo.idx_id;
            list->info[idx].status = tmp_callinfo.status;
            list->info[idx].emergency = tmp_callinfo.emergency;
            list->info[idx].is_start_requested = tmp_callinfo.is_start_requested;
            list->info[idx].mon_addr = tmp_callinfo.mon_addr;
        }
    }

#if 1
{
char c, d;
short s;
int idx;
idx = list->out_ptr;
for (i = 0; i < list->ctr; i++) {
    c = list->info[idx].car_id;
    d = list->info[idx].idx_id;
    s = list->info[idx].status;
    printf("< After MOVE EMER-PEI %d-%d S:%d >\n", c, d, s);
    idx++;
}
}
#endif

    return ret;
}
#endif


static int get_call_waiting_ctr(int where)
{
    struct multi_call_status *list;
    int ctr;
    //int i, max;
    //char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
        //strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
        //strcpy(base_name, "PEI");
    }
    else
        return 0;

    ctr = list->ctr;

    return ctr;
}

static int is_waiting_call(int where)
{
    struct multi_call_status *list;
    int idx, ctr;
    int waiting = 0;
    //int i, max;
    //char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
        //strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
        //strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0 && list->info[idx].status == CALL_STATUS_WAKEUP) {
        waiting++;
        //DBG_LOG("< IS WAITING %s %d-%d >\n", base_name, list->info[idx].car_id, list->info[idx].idx_id);
    } else {
		//DBG_LOG("idx(%d) : %d-%d \n", idx, list->info[idx].car_id, list->info[idx].idx_id);
	}

    return waiting;
}

static int is_waiting_normal_call(int where, int car_num, int dev_num)
{
    struct multi_call_status *list;
    int idx, ctr;
    int waiting = 0;
    //int i, max;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (   ctr > 0
        && (list->info[idx].status == CALL_STATUS_WAKEUP)
        && (list->info[idx].emergency == 0)
       )
    {
        if (car_num == -1 && dev_num == -1) {
            waiting++;
            /*printf("< IS NORMAL WAITING %s %d-%d >\n", base_name,
                    list->info[idx].car_id, list->info[idx].idx_id);*/
        } else {
            if (  list->info[idx].car_id == car_num
                && list->info[idx].idx_id == dev_num
               )
            {
                waiting++;
                /*printf("< IS NORMAL WAITING %s BY %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
            }
        }
    }

    return waiting;
}

static int is_waiting_normal_or_emer_call(int where, int car_num, int dev_num)
{
    struct multi_call_status *list;
    int idx, ctr;
    int waiting = 0;
    //int i, max;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (   ctr > 0
        && (list->info[idx].status == CALL_STATUS_WAKEUP)
        /*&& (list->info[idx].emergency == 0)*/
       )
    {
        if (car_num == -1 && dev_num == -1) {
            waiting++;
            /*printf("< IS NORMAL WAITING %s %d-%d >\n", base_name,
                    list->info[idx].car_id, list->info[idx].idx_id);*/
        } else {
            if (  list->info[idx].car_id == car_num
                && list->info[idx].idx_id == dev_num
               )
            {
                waiting++;
                /*printf("< IS NORMAL WAITING %s BY %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
            }
        }
    }

    return waiting;
}

static int is_waiting_normal_call_any(int where, int car_num, int dev_num)
{
    struct multi_call_status *list;
    int idx;
    int waiting = 0;
    int i, ctr, max;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    for (i = 0; i < ctr; i++) {
        if (   (list->info[idx].status == CALL_STATUS_WAKEUP)
            && (list->info[idx].emergency == 0))
        {
            if (car_num && dev_num) {
                if (   list->info[idx].car_id == car_num
                    && list->info[idx].idx_id == dev_num
                   )
                {
                    waiting++;
                    /*printf("< IS ANY NORMAL WAITING %s BY %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
                    break;;
                }
            } else {
                waiting++;
                /*printf("< IS ANY NORMAL WAITING %s %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
                //break;
            }
        }
        idx++;
        if (idx == max) idx = 0;
    }

    return waiting;
}

static int is_waiting_normal_or_emer_call_any(int where, int car_num, int dev_num)
{
    struct multi_call_status *list;
    int idx;
    int waiting = 0;
    int i, ctr, max;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    for (i = 0; i < ctr; i++) {
        if (   (list->info[idx].status == CALL_STATUS_WAKEUP)
            /*&& (list->info[idx].emergency == 0)*/)
        {
            if (car_num && dev_num) {
                if (   list->info[idx].car_id == car_num
                    && list->info[idx].idx_id == dev_num
                   )
                {
                    waiting++;
                    /*printf("< IS ANY NORMAL WAITING %s BY %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
                    break;;
                }
            } else {
                waiting++;
                /*printf("< IS ANY NORMAL WAITING %s %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
                //break;
            }
        }
        idx++;
        if (idx == max) idx = 0;
    }

    return waiting;
}

static int is_waiting_emergency_pei_call(void)
{
    struct multi_call_status *list;
    int idx, ctr;
    int waiting = 0;

    list = &__pei_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (   ctr > 0
        && (list->info[idx].status == CALL_STATUS_WAKEUP)
        && (list->info[idx].is_start_requested == 0)
        && (list->info[idx].emergency == 1)) {
        waiting++;
        //printf("< IS WAITING EMER %s %d-%d >\n", base_name,
        //        list->info[idx].car_id, list->info[idx].idx_id);
    }

    return waiting;
}

static int is_waiting_emergency_pei_call_any(void)
{
    struct multi_call_status *list;
    int idx, ctr;
    int waiting = 0;

    list = &__pei_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (   ctr > 0
        && (list->info[idx].status == CALL_STATUS_WAKEUP)
        /*&& (list->info[idx].is_start_requested == 0)*/
        && (list->info[idx].emergency == 1)) {
        waiting = 1;
        //printf("< IS WAITING EMER %s %d-%d >\n", base_name,
        //        list->info[idx].car_id, list->info[idx].idx_id);
    }

    return waiting;
}

static int is_waiting_emergency_pei_call_any_2(int car_num, int dev_num)
{
    struct multi_call_status *list;
    int i, idx, ctr, max;
    int waiting = 0;

    list = &__pei_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;
    max = MAX_PEI_CALL_NUM;

    for (i = 0; i < ctr; i++) {
        if (   (list->info[idx].status == CALL_STATUS_WAKEUP)
            && (list->info[idx].emergency == 1))
        {
            if (car_num && dev_num) {
                if (   list->info[idx].car_id == car_num
                    && list->info[idx].idx_id == dev_num
                   )
                {
                    waiting++;
                    /*printf("< IS ANY EMER WAITING %s BY %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
                    break;
                }
            } else {
                waiting++;
                /*printf("< IS ANY EMER WAITING %s %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);*/
                //break; // delete, 2013/11/27
            }
        }
        idx++;
        if (idx == max) idx = 0;
    }

    return waiting;
}

static int is_monitoring_call(int where, int car_num, int dev_num, int any_id)
{
    struct multi_call_status *list;
    int idx;
    int waiting = 0;
    int i, ctr, max;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (any_id) {
        for (i = 0; i < ctr; i++) {
            if (   list->info[idx].status == CALL_STATUS_MON
                /*|| list->info[idx].status == CALL_STATUS_MON_WAIT*/
               )
            {
                waiting = 1;
                //printf("< ON ANY MONITORING %s %d-%d >\n", base_name,
                //        list->info[idx].car_id, list->info[idx].idx_id);
                break;
            }
            idx++;
            if (idx == max) idx = 0;
        }
    }
    else {
            if (   ctr > 0
                && list->info[idx].status == CALL_STATUS_MON
                && list->info[idx].car_id == car_num
                && list->info[idx].idx_id == dev_num
               )
            {
                waiting = 1;
#if 0
                printf("< ON MONITORING %s %d-%d >\n", base_name,
                        list->info[idx].car_id, list->info[idx].idx_id);
#endif
            }
    }

    return waiting;
}

static int is_waiting_monitoring_call(int where, char *get_car_num, char *get_dev_num, int *mon_addr, int *tx_addr, int any_id)
{
    struct multi_call_status *list;
    int idx;
    int waiting = 0;
    int i, ctr, max;
    char base_name[8];
    int car = 0, dev = 0;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (any_id) {
        if (get_car_num && get_dev_num) {
            car = *get_car_num;
            dev = *get_dev_num;
        }

//printf("%s, car:%d, dev:%d\n", __func__, car, dev);
        for (i = 0; i < ctr; i++) {
            if (list->info[idx].status == CALL_STATUS_MON_WAIT)
            {
                if (   car && dev
                    && list->info[idx].car_id == car
                    && list->info[idx].idx_id == dev
                   )
                {
                    waiting = 1;
                    //printf("< ON ANY WAIT-MONITORING BY ID %s %d-%d >\n", base_name,
                    //        list->info[idx].car_id, list->info[idx].idx_id);
                    break;
                }
                else if (car == 0 && dev == 0) {
                    waiting = 1;
                    //printf("< ON ANY WAIT-MONITORING %s %d-%d >\n", base_name,
                    //        list->info[idx].car_id, list->info[idx].idx_id);
                    break;
                }
            }
            idx++;
            if (idx == max) idx = 0;
        }
    }
    else {
        if (   ctr > 0
            && list->info[idx].status == CALL_STATUS_MON_WAIT
            && list->info[idx].car_id
            && list->info[idx].idx_id
           )
        {
            if (get_car_num) *get_car_num = list->info[idx].car_id;
            if (get_dev_num) *get_dev_num = list->info[idx].idx_id;
            if (mon_addr) *mon_addr = list->info[idx].mon_addr;
            if (tx_addr) *tx_addr = list->info[idx].tx_addr;
            waiting = 1;
#if 0
            printf("< ON WAIT-MONITORING %s %d-%d >\n", base_name,
                    list->info[idx].car_id, list->info[idx].idx_id);
#endif
        }
    }

    return waiting;
}

static int is_emergency_pei_live(void)
{
    struct multi_call_status *list;
    int ctr, idx;
    int living = 0;

    list = &__pei_call_list;
    idx = list->out_ptr;
    ctr = list->ctr;

    if (   ctr
        && list->info[idx].status == CALL_STATUS_LIVE
        && list->info[idx].emergency == 1
       )
    {
        living++;
        //printf("< EMER PEI LIVE %d-%d >\n", 
        //        list->info[idx].car_id, list->info[idx].idx_id);
    }

    return living;
}

#if 0
static int get_call_live_num(int where)
{
    struct multi_call_status *list;
    int i, idx, ctr, max;
    int living = 0;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    }
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    for (i = 0; i < ctr; i++) {
        if (list->info[idx].status == CALL_STATUS_LIVE) {
            living++;
            printf("< LIVING %s %d-%d >\n", base_name,
                    list->info[idx].car_id, list->info[idx].idx_id);
        }
        idx++;
        if (idx == max) idx = 0;
    }
//printf("[[ Live Num:%d ]]\n", living);
    return living;
}
#endif

static int is_live_call(int where, int car_num, int idx_num)
{
    struct multi_call_status *list;
    int idx; //, ctr, max;
    int living = 0;
    char base_name[8];

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
        strcpy(base_name, "CAB");
    } else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
        strcpy(base_name, "PEI");
    } else {
		//DBG_LOG("\n");
        return 0;
	}

    idx = list->out_ptr;
    //ctr = list->ctr;

//printf("== live, idx:%d, ctr:%d\n", idx, ctr);
//printf("== status:%d, car_id:%d, idx_id:%d\n",
//list->info[idx].status,
//list->info[idx].car_id,
//list->info[idx].idx_id);
    if (list->info[idx].status == CALL_STATUS_LIVE) {
        if (   car_num && idx_num
            && list->info[idx].car_id == car_num
            && list->info[idx].idx_id == idx_num
           )
        {
            living = 1;
            //printf("< IS LIVE WITH ID %s %d-%d >\n", base_name,
            //        list->info[idx].car_id, list->info[idx].idx_id);
        }
        else if (car_num == 0 && idx_num == 0) {
            living = 1;
            //printf("< IS LIVE ID %s %d-%d >\n", base_name,
            //        list->info[idx].car_id, list->info[idx].idx_id);
        }
    } else {
		DBG_LOG("< IS live call ID %s %d-%d >\n", base_name, list->info[idx].car_id, list->info[idx].idx_id);
	}
	//DBG_LOG("living = %d\n", living);
    return living;
}

static char get_call_car_num(int where)
{
    char n = 0;
    struct multi_call_status *list;
    int idx, ctr;//i, max;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
    }
    else
        return n;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0)
        n = list->info[idx].car_id;
    return n;
}

static char get_call_id_num(int where)
{
    char n = 0;
    struct multi_call_status *list;
    int idx, ctr;//i, max;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
    }
    else
        return n;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0)
        n = list->info[idx].idx_id;
    return n;
}

static char get_call_train_num(int where)
{
    char n = 0;
    struct multi_call_status *list;
    int idx, ctr;//i, max;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
        //max = MAX_CAB_CALL_NUM;
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
        //max = MAX_PEI_CALL_NUM;
    }
    else
        return n;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0)
        n = list->info[idx].train_num;
    return n;
}

static int get_call_mon_addr(int where)
{
    int addr = 0;
    struct multi_call_status *list;
    int idx, ctr;

    if (where == CAB_LIST)
        list = &__cab_call_list;
    else if (where == PEI_LIST)
        list = &__pei_call_list;
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0)
        addr = list->info[idx].mon_addr;

    return addr;
}

static int get_call_tx_addr(int where)
{
    int addr = 0;
    struct multi_call_status *list;
    int idx, ctr;

    if (where == CAB_LIST)
        list = &__cab_call_list;
    else if (where == PEI_LIST)
        list = &__pei_call_list;
    else
        return 0;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0)
        addr = list->info[idx].tx_addr;

    return addr;
}

static int get_call_is_emergency(void)
{
    int n = 0;
    struct multi_call_status *list;
    int idx, ctr;//i, max;

    list = &__pei_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0)
        n = list->info[idx].emergency;

    return n;
}

static int get_waiting_call_pei_index(int *get_car_num, int *get_dev_num, int *get_emergency)
{
    struct multi_call_status *list = NULL;
    int i, m, idx, ctr;
    char base_name[8];
    int ret = -1;

	list = &__pei_call_list;
	strcpy(base_name, "PEI");

    idx = list->out_ptr;
    ctr = list->ctr;
    m = idx;


    //if (ctr >= 2 && (is_waiting_normal_call_any(where, 0, 0) < ctr))
    if (ctr >= 2)
    {
        for (i = idx; i < (idx+ctr); i++) {
            if (   list->info[i].status == CALL_STATUS_WAKEUP
                //&& tmp_list->info[i].emergency == is_emergency
               )
            {
				*get_car_num = list->info[i].car_id;
				*get_dev_num = list->info[i].idx_id;
				*get_emergency = list->info[i].emergency;
                ret = i;
                break; // <<=== // delete break, 2013/11/20, to move over one emergency
            }
        }
	}

    return ret;
}

#if 0
static int change_emer_to_normal_pei_call(void)
{
    int n = 0;
    struct multi_call_status *list;
    int idx, ctr;//i, max;

    list = &__pei_call_list;

    idx = list->out_ptr;
    ctr = list->ctr;

    if (ctr > 0)
        list->info[idx].emergency = 0;

    return n;
}
#endif

static int get_call_any_ctr(int where)
{
    struct multi_call_status *list;
    int ctr;

    if (where == CAB_LIST) {
        list = &__cab_call_list;
    }
    else if (where == PEI_LIST) {
        list = &__pei_call_list;
    }
    else
        return 0;

    ctr = list->ctr;

    return ctr;
}

static int read_cop_config_setup(void)
{
    int fd, i, rlen;
    char buf[64], ch;
    int val;
    char *s;

    pei_to_occ_no_mon_can_cop_pa = 1;
    //occ_pa_no_mon_can_pei_call = 0;
    occ_spk_vol_atten = LM1971_VOLUME_MIN_VALUE;
    occ_mic_vol_atten = LM1971_VOLUME_MIN_VALUE;
    pei_comm_half_duplex = 0;
    selected_language = 1;
    deadman_alarm_is_use = 1;
    fire_alarm_is_use = 1;
    internal_test_use = 0;

    fd = open(COB_CONFIG_SETUP_FILE, O_RDONLY);
    if (fd <= 0) {
        printf("COP CONFIG SETUP FILE OPEN ERROR: %s\n", COB_CONFIG_SETUP_FILE);

    }
    else {
        i = 0;
        while (1) {
            rlen = read(fd, &ch, 1);
            //printf("len: %d\n", rlen);
            if (rlen <= 0)
                break;
            //printf("%02X ", ch);
            if (ch == 0x0A || ch == 0x0D) {
                buf[i] = 0;
                i = 0;

                if (strncmp(buf, "PEI_TO_OCC_NO_MON_CAN_COP_PA", 28) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val == 0 || val == 1)
                            pei_to_occ_no_mon_can_cop_pa = val;
                        else
                            pei_to_occ_no_mon_can_cop_pa = 0;
                    }
                }
                /*else if (strncmp(buf, "OCC_PA_NO_MON_CAN_PEI_CALL", 26) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val == 0 || val == 1)
                            occ_pa_no_mon_can_pei_call = val;
                    }
                }*/
                else if (strncmp(buf, "OCC_SPK_VOL_ATTEN", 17) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val < 0 || val > 96)
                            occ_spk_vol_atten = 96;
                        occ_spk_vol_atten = val;
                    }
                }
                else if (strncmp(buf, "OCC_MIC_VOL_ATTEN", 17) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val < 0 || val > 96)
                            occ_mic_vol_atten = 96;
                        occ_mic_vol_atten = val;
                    }
                }
#if 0 // delete, Ver0.75, 2014/07/31
                else if (strncmp(buf, "PEI_COMM_HALF_DUPLEX", 20) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val == 0 || val == 1)
                            pei_comm_half_duplex = val;
                    }
                }
#endif
                else if (strncmp(buf, "SELECT_LANGUAGE", 15) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val == 0 || val == 1 || val == 2)
                            selected_language = val;
                    }
                }
                else if (strncmp(buf, "DEADMAN_IS_USING", 16) == 0) {
                    s = strchr(buf, ':');
                    if (s) {
                        s++;
                        val = strtol(s, NULL, 0);
                        if (val == 0 || val == 1)
                            deadman_alarm_is_use = val;
                    }
                }
				else if (strncmp(buf, "FIRE_IS_USING", 13) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						if (val == 0 || val == 1)
							fire_alarm_is_use = val;
					}
				}
				else if (strncmp(buf, "INTERNAL_TEST", 13) == 0) {
					s = strchr(buf, ':');
					if (s) {
						s++;
						val = strtol(s, NULL, 0);
						if (val == 0 || val == 1)
							internal_test_use = val;
					}
				}
            }
            else {
                buf[i++] = ch;
            }
        }
        close (fd);
    }

    set_language_type(selected_language);

    printf("\n[ COP CONFIG: PEI-OCC NO MON CAN COP-PA:%d ]\n", pei_to_occ_no_mon_can_cop_pa);
    /*printf("[ COP CONFIG: OCC-PA  NO MON CAN PEI-CALL:%d ]\n", occ_pa_no_mon_can_pei_call);*/
    printf("[ COP CONFIG: OCC SPK VOL ATTEN:%d ]\n", occ_spk_vol_atten);
    printf("[ COP CONFIG: PEI HALF DUPLEX:%d ]\n", pei_comm_half_duplex);
    printf("[ COP CONFIG: LANGUAGE:%d ]\n", selected_language);
    printf("[ COP CONFIG: TEST:%d ]\n", internal_test_use);

    return 0;
}

void send_call_protocol_err_to_avc(char *cmdbuf)
{
    packet_AVC2COP_t *avc2cop_packet;
    COP_Cur_Status_t status = __broadcast_status;

    avc2cop_packet = (packet_AVC2COP_t *)cmdbuf;

    if (avc2cop_packet->CallReq.active == 1)
    {
        if (   (status >= __PEI2CAB_CALL_START && status <= __CAB2CAB_CALL_STOP)
            || (status >= OCC2PEI_CALL_START && status <= OCC2PEI_CALL_STOP)
            || (status >= __WAITING_CAB2CAB_CALL_START && status <= WAITING_PEI2CAB_CALL_STOP)
           )
            send_all2avc_cycle_data(TmpBuf, 8, ERROR_AVC_CALL_PROTOCOL_ERR, 1, 0);
    }
}

int cop_process_init(void)
{
    enable_deadman_alarm = 0;
    enable_fire_alarm = 0;

    memset(&__key_list_status, 0, sizeof(KeyStatus_List_t));
    __broadcast_status = COP_STATUS_IDLE;

    Func_Cop_Key_Process_List[0] = __none_key_process;
    Func_Cop_Key_Process_List[1] = __pa_in_key_process;
    Func_Cop_Key_Process_List[2] = __pa_out_key_process;
    Func_Cop_Key_Process_List[3] = __call_cab_key_process;
    Func_Cop_Key_Process_List[4] = __call_pei_key_process;
    Func_Cop_Key_Process_List[5] = __none_key_process;	//S1
	//Func_Cop_Key_Process_List[5] = __call_join_key_process;	//S1
    Func_Cop_Key_Process_List[6] = __none_key_process;	//S2
    Func_Cop_Key_Process_List[7] = __none_key_process;	//S3
    Func_Cop_Key_Process_List[8] = __none_key_process;	//S4
    Func_Cop_Key_Process_List[9] = __pa_inout_key_process;
    Func_Cop_Key_Process_List[10] = __none_key_process;
    Func_Cop_Key_Process_List[11] = __none_key_process;
    Func_Cop_Key_Process_List[12] = __none_key_process;
    Func_Cop_Key_Process_List[13] = __none_key_process;

    func_pending = 0;

    indoor_pending = 0;
    inout_bcast_processing = 0;
    outdoor_pending = 0;
    occ_pa_pending = 0;
    occ_pa_running = 0;

    __car_num = 0;
    __dev_num = 0;

#ifdef PEI_MONITORING_SAVE
    save_file_fd = -1;
#endif

    pei2cab_call_pending = 0;
    pei2cab_call_waiting = 0;
    pei2cab_call_running = 0;

	occ_key_lock = 0;
	cab_key_lock = 0;
	pei_key_lock = 0;
	pa_in_key_lock = 0;
	pa_out_key_lock = 0;
	func_key_lock = 0;

    global_menu_active = COP_MENU_BROADCAST | COP_MENU_HEAD_SELECT;
    bmenu_active = 1;
    bcast_updown_index = 0;

    bmenu_func_active = 0;
    bmenu_start_active = 0;
    bmenu_station_active = 0;

    bcast_func_10 = 0;
    bcast_func_1 = 0;
    bcast_func_index = 0;
    bcast_start_index = 0;
    bcast_start_num = 1;
    bcast_station_index = 0;
    bcast_leftright_index = 0;
	bmenu_station_num = 1;
	bmenu_station_num_start = 1;
	bmenu_station_num_end = 2;

    bcast_updown_index = 1;
    bcast_leftright_index = 0;
    FuncNum = 1;
    Func_Is_Now_Start = -1;
    run_force_cab_call_wakeup = 0;

    cmenu_active = 0;

    vmenu_active = 0;
    vol_updown_index = 0;

    pending_indoor_on_pei_wakeup = 0;
    pending_outdoor_on_pei_wakeup = 0;
    pending_inoutdoor_on_pei_wakeup = 0;
    run_force_pei2cab_mon_on_passive = 0;

    emergency_pei2cab_start = 0;

	Log_Status = LOG_AVC_NOT_CONNECT;

    enable_in_pa_simultaneous_on_pei2cab = 0;
    enable_out_pa_simultaneous_on_pei2cab = 0;
    enable_pei_wakeup_on_in_passive = 0;
    enable_pei_wakeup_on_out_passive = 0;

    /*enable_pei2cab_simultaneous_on_occ_pa = 0;*/

    enable_occ_pa_on_in_passive = 0;
    enable_occ_pa_on_out_passive = 0;

    cab_pa_bcast_monitor_start = 0;
    func_bcast_monitor_start = 0;
    fire_bcast_monitor_start = 0;
    auto_bcast_monitor_start = 0;
    global_auto_func_monitor_start = 0;

    cop_pa_bcast_rx_running = 0;
    func_bcast_rx_running = 0;
    fire_bcast_rx_running = 0;
    auto_bcast_rx_running = 0;

    occ_key_status = 0;
    occ_tx_rx_en_ext_flag = 0;
    old_occ_tx_rx_en_ext_flag = -1;
    pei_half_way_com_flag = 0;

    pei_to_occ_no_mon_can_cop_pa = 0;
    read_cop_config_setup();

    menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, cmenu_active, 0, 0);
    InOut_LedStatus = INOUT_BUTTON_STATUS_ALLOFF;
    broadcast_draw_menu(InOut_LedStatus, 0, 0, FuncNum, 1, bmenu_active, 0);

    broadcast_draw_menu(InOut_LedStatus, 0, 0, FuncNum, 1, bmenu_active, 1);
    global_menu_active = COP_MENU_BROADCAST | COP_MENU_HEAD_SELECT;
    bmenu_active = 0;
    menu_head_draw(global_menu_active, bmenu_active, bmenu_func_active, bmenu_start_active, bmenu_station_active, cmenu_active, 0, 0);
    broadcast_draw_menu(InOut_LedStatus, 0, 0, FuncNum, 1, bmenu_active, 1);

    draw_lines();

    return 0;
}

void mixer_buffer_init(void)
{
	int loop;
	for(loop=0; loop<2; loop++) {
		call_mixer_rxlen1[loop] = 0;
		call_mixer_rxlen2[loop] = 0;
	}
}

long cal_clock_delay(struct timespec *old_t, struct timespec *cur_t)
{
    long timedelay, temp, temp_n;

    if (cur_t->tv_nsec >= old_t->tv_nsec) {
        temp   = cur_t->tv_sec  - old_t->tv_sec;
        temp_n = cur_t->tv_nsec - old_t->tv_nsec;
        timedelay = 1000000000 * temp + temp_n;
    } else {
        temp   = cur_t->tv_sec  - old_t->tv_sec - 1;
        temp_n = 1000000000 + cur_t->tv_nsec - old_t->tv_nsec;
        timedelay = 1000000000 * temp + temp_n;
    }

    return timedelay;
}

#define ENUM_CASE(name) case name: return #name;
char* get_string_cob_status(COP_Cur_Status_t status)
{
	switch(status)
	{
		ENUM_CASE(COP_STATUS_IDLE)
		ENUM_CASE(FUNC_BCAST_START)
		ENUM_CASE(FUNC_BCAST_STOP)
		ENUM_CASE(__PASSIVE_INDOOR_BCAST_START)
		ENUM_CASE(__PASSIVE_INDOOR_BCAST_STOP)
		ENUM_CASE(__PASSIVE_OUTDOOR_BCAST_START)
		ENUM_CASE(__PASSIVE_OUTDOOR_BCAST_STOP)
		ENUM_CASE(__PASSIVE_INOUTDOOR_BCAST_START)
		ENUM_CASE(PASSIVE_INOUTDOOR_BCAST_STOP)
		ENUM_CASE(__PEI2CAB_CALL_START)
		ENUM_CASE(__PEI2CAB_CALL_STOP)
		ENUM_CASE(__CAB2CAB_CALL_START)
		ENUM_CASE(__CAB2CAB_CALL_STOP)
		ENUM_CASE(OCC_PA_BCAST_START)
		ENUM_CASE(OCC_PA_BCAST_STOP)
		ENUM_CASE(OCC2PEI_CALL_START)
		ENUM_CASE(OCC2PEI_CALL_STOP)
		ENUM_CASE(OCCPA_BCAST_STATUS_START)
		ENUM_CASE(OCCPA_BCAST_STATUS_STOP)
		ENUM_CASE(AUTO_BCAST_START)
		ENUM_CASE(AUTO_BCAST_STOP)
		ENUM_CASE(AUTO_FIRE_BCAST_START)
		ENUM_CASE(AUTO_FIRE_BCAST_STOP)
		ENUM_CASE(FIRE_BCAST_START)
		ENUM_CASE(FIRE_BCAST_STOP)
		ENUM_CASE(WAITING_FUNC_BCAST_START)
		ENUM_CASE(WAITING_PASSIVE_INDOOR_BCAST_START)
		ENUM_CASE(WAITING_PASSIVE_INDOOR_BCAST_STOP)
		ENUM_CASE(WAITING_PASSIVE_OUTDOOR_BCAST_START)
		ENUM_CASE(WAITING_PASSIVE_OUTDOOR_BCAST_STOP)
		ENUM_CASE(WAITING_OUTSTART_PASSIVE_INOUTDOOR_BCAST_START)
		ENUM_CASE(WAITING_INSTART_PASSIVE_INOUTDOOR_BCAST_START)
		ENUM_CASE(_WAITING_OUTSTOP_PASSIVE_INOUTDOOR_BCAST_START)
		ENUM_CASE(_WAITING_INSTOP_PASSIVE_INOUTDOOR_BCAST_START)
		ENUM_CASE(__WAITING_CAB2CAB_CALL_START)
		ENUM_CASE(__CAB2CAB_CALL_REJECT)
		ENUM_CASE(__CAB2CAB_CALL_WAKEUP)
		ENUM_CASE(__CAB2CAB_CALL_MONITORING_START)
		ENUM_CASE(__CAB2CAB_CALL_MONITORING_STOP)
		ENUM_CASE(WAITING_CAB2CAB_CALL_STOP)
		ENUM_CASE(__PEI2CAB_CALL_WAKEUP)
		ENUM_CASE(__PEI2CAB_CALL_MONITORING_START)
		ENUM_CASE(__PEI2CAB_CALL_MONITORING_STOP)
		ENUM_CASE(PEI_CALL_REJECT)
		ENUM_CASE(__PEI2CAB_CALL_EMER_EN)//__PEI2CAB_CALL_EMER_WAKEUP,    /* 127 */
		ENUM_CASE(__NOT_USED_1__)//__PEI2CAB_CALL_EMER_START,            /* 128 */
		ENUM_CASE(__NOT_USED_2__)//__PEI2CAB_CALL_EMER_STOP,             /* 129 */
		ENUM_CASE(__NOT_USED_3__)// __PEI2CAB_CALL_EMER_MONITORING_START,/* 130 */
		ENUM_CASE(__NOT_USED_4__)// __PEI2CAB_CALL_EMER_MONITORING_STOP, // 131
		ENUM_CASE(__WAITING_PEI2CAB_CALL_START)
		ENUM_CASE(WAITING_PEI2CAB_CALL_STOP)
		ENUM_CASE(WAITING_OCC_PA_BCAST_START)
		ENUM_CASE(WAITING_OCC_PA_BCAST_STOP)
#if 0
		ENUM_CASE(__CAB2CAB_CALL_EMER_EN)
#endif
		ENUM_CASE(IS_OCC_CMD)
		ENUM_CASE(GET_TRAIN_NUM)
		ENUM_CASE(IS_DEADMAN_CMD)
		ENUM_CASE(GET_DEADMAN_CMD)
		ENUM_CASE(IS_ONLY_LED_FLAG)
		ENUM_CASE(ONLY_PAMP_VOLUME)
		ENUM_CASE(ENABLE_GLOBAL_START_FUNC_AUTO)
		ENUM_CASE(DISABLE_GLOBAL_START_FUNC_AUTO)
#if 0
		ENUM_CASE(IS_FIRE_ALARM_CMD)
		ENUM_CASE(GET_FIRE_ALARM_CMD)
#endif
		ENUM_CASE(TRACTOR_POSSIBLE_CMD)
		ENUM_CASE(TRACTOR_UNPOSSIBLE_CMD)
		ENUM_CASE(HELP_CAB2CAB_START)
		ENUM_CASE(__PEI2CAB_CALL_OCC)
		ENUM_CASE(FUNC_BCAST_REJECT)
		ENUM_CASE(UI_SET_ROUTE_CMD)
		ENUM_CASE(UI_SET_SPECIAL_ROUTE_CMD)
		ENUM_CASE(UI_DI_DISPLAY_CMD)

		defaut:
			return "COB_STATUS_UNKNOWN";
	}	
}

void debug_print_cob_status_change(void)
{
	static COP_Cur_Status_t last_broadcast_status = COP_STATUS_IDLE;
	if(last_broadcast_status != __broadcast_status)
	{
		printf("  --{{ %s (%d)  =>  %s (%d) }}--\n",
			get_string_cob_status(last_broadcast_status), last_broadcast_status,
			get_string_cob_status(__broadcast_status), __broadcast_status);
		last_broadcast_status = __broadcast_status;
	}
}

int get_func_is_now_start(void)
{
	return Func_Is_Now_Start;
}

char* addr_to_str(int addr)
{
    unsigned int temp_addr[4];

    memset(conv_addr_str, 0, sizeof(conv_addr_str));

    if(addr == 0)
	{
		sprintf(conv_addr_str,"%s", "0.0.0.0");
        return conv_addr_str;
	}

    temp_addr[0] = (addr) & 0xff;
    temp_addr[1] = (addr >> 8) & 0xff;
    temp_addr[2] = (addr >> 16) & 0xff;
    temp_addr[3] = (addr >> 24) & 0xff;

    sprintf(conv_addr_str, "%d.%d.%d.%d", temp_addr[0], temp_addr[1], temp_addr[2], temp_addr[3]);

    return conv_addr_str;
}

void print_call_list(int where)
{
	struct multi_call_status *list;
	struct call_info *ptr;
	int i, idx, ctr, in_ptr, max;
	char addr_str[2][32];
	char base_name[8]={0,};

	if (where == CAB_LIST)
	{
		list = &__cab_call_list;
		max = MAX_CAB_CALL_NUM;
		strcpy(base_name, "CAB");
	}
	else if (where == PEI_LIST)
	{
		list = &__pei_call_list;
		max = MAX_PEI_CALL_NUM;
		strcpy(base_name, "PEI");
	}

	idx = list->out_ptr;
	in_ptr = list->in_ptr;
	ctr = list->ctr;

	printf("=========== %s_LIST ctr(%d) in_ptr(%d) out_ptr(%d) ============\n", base_name, ctr, in_ptr, idx);
	printf("  Status: 1:WAKEUP, 2:EMER, 4:LIVE, 8:MON, 16:MON_WAIT\n");
	for(i = 0; i <ctr; i++)
	{
		memset(addr_str, 0, sizeof(addr_str));

		ptr = &(list->info[idx]);

		memcpy(addr_str[0], addr_to_str(ptr->mon_addr), 32);
		memcpy(addr_str[1], addr_to_str(ptr->tx_addr), 32);


		printf("  [%d] %2d-%2d (TS#%2d), %02d, /%d %d /mon:%s, tx:%s\n",
				idx, ptr->car_id, ptr->idx_id, ptr->train_num, ptr->status, 
				ptr->emergency, ptr->is_start_requested,
				addr_str[0], addr_str[1]);
		
		idx++;
		if (idx == max) idx = 0;
	}
	printf("=============================================================\n");
}

void update_pei_join_active_status(void)
{
    struct multi_call_status *list = NULL;
	static last_ctr=0;
    int i, idx, ctr;
    //int ret = -1;
	int live_ctr=0, wait_ctr=0;

	list = &__pei_call_list;
	//strcpy(base_name, "PEI");

    idx = list->out_ptr;
    ctr = list->ctr;

	//printf("-----update_pei_join_active_status------\n");
	//print_call_list(PEI_LIST);
	//printf("----------------------------------------\n");

	last_ctr = ctr;

    //if (ctr >= 2 && (is_waiting_normal_call_any(where, 0, 0) < ctr))
    if (ctr >= 2)
    {
        for (i = idx; i < (idx+ctr); i++)
		{
			if( list->info[i].status == CALL_STATUS_LIVE ) {
				live_ctr++;
			}

			if( list->info[i].status == CALL_STATUS_WAKEUP ) {
				wait_ctr++;
            }
        }

		if( live_ctr == 1 && wait_ctr > 0)
			pei_join_button_on = 1;
		else
			pei_join_button_on = 0;
	}
	else
	{
		pei_join_button_on = 0;
	}

	return;
    //return ret;
}

int update_pei_join_call_list_status(void)
{
    struct multi_call_status *list = NULL;
	//int idx = -1;
	int ctr, ret = -1;

	list = &__pei_call_list;

    //idx = list->out_ptr;
    ctr = list->ctr;

	//print_call_list(PEI_LIST);
	if(	ctr >= 2 &&
		pei_join_button_on &&
		pei_joined_call_idx != -1
	  )
	{
		if(list->info[pei_joined_call_idx].status == CALL_STATUS_WAKEUP)
		{
			//printf("\n>>>>update_pei_join_call_list_status\n");
			list->info[pei_joined_call_idx].status = CALL_STATUS_LIVE;
			//printf("****PEI JOIN idx(%d) CALL_STATUS_WAKEUP -> CALL_STATUS_LIVE\n", pei_joined_call_idx);
			//pei_joined_call_idx = -1;
			update_pei_join_active_status();
			//printf("<<<<update_pei_join_call_list_status\n\n");
			return 0;
		}
	}

	return ret;
}

static int pei2cab_join_call_start(void)
{
    int ret;
	int join_car_num = -1, join_dev_num = -1, join_emergency_en = -1;

	print_call_list(PEI_LIST);
	pei_joined_call_idx = get_waiting_call_pei_index(&join_car_num, &join_dev_num, &join_emergency_en);
	//__car_num = join_car_num;
	//__dev_num = join_dev_num;
	printf("pei2cab_join_call_start - send join call start - idx(%d), %d-%d\n", 
			pei_joined_call_idx, join_car_num, join_dev_num);

	//DBG_LOG("Join call start request : %d-%d, pei_call_count = %d\n", join_car_num, join_dev_num, pei_call_count);
	ret = send_cop2avc_cmd_pei2cab_call_start_request(0, join_car_num, join_dev_num, join_emergency_en, 0);

	return ret;
}

int process_join_start(void)
{
	int ret;

	usleep(10*1000);	/* 10ms waiting */
	ret = pei2cab_join_call_start();
	printf("< process_join_start >\n");

	return ret;
}
