#ifndef __KEY_H__
#define __KEY_H__

enum __cop_key_enum {
    enum_COP_KEY_NONE = 0,
    enum_COP_KEY_PA_IN,		// 1
    enum_COP_KEY_PA_OUT,	// 2
    enum_COP_KEY_CALL_CAB,	// 3
    enum_COP_KEY_CALL_PEI,	// 4
	enum_COP_KEY_SPARE_1,	// 5
	enum_COP_KEY_SPARE_2,	// 6
	enum_COP_KEY_SPARE_3,	// 7
	enum_COP_KEY_SPARE_4,	// 8
    enum_COP_KEY_PA_INOUT,	// 9
    enum_COP_KEY_LONG_KEY,	// 10
    enum_COP_KEY_MAX		// 11
};

#define KEY_PRESSED_MASK	(1<<15)


#define KEY_IS_OFF	(1<<0)
#define KEY_IS_ON	(1<<1)
#define KEY_IS_BLINK	(1<<2)

typedef struct {
    unsigned char up;
    unsigned char down;
    unsigned char left;
    unsigned char right;
    unsigned char menu;
    unsigned char select;
    unsigned char pa_occ;
    unsigned char pa_in;
    unsigned char pa_out;
    unsigned char call_cab;
    unsigned char call_pei;
    unsigned char fire;
} KeyStatus_List_t;

int key_dev_open(void);
int key_init(void);
int key_exit(void);
void key_test(void);
int clear_key_led(enum __cop_key_enum keyval);
int set_key_led(enum __cop_key_enum keyval);
int get_key_value(void);
int set_key_led_blink(enum __cop_key_enum, int blinkonoff);
int get_status_key_blink(enum __cop_key_enum keyval);

int set_cop_pa_mic_audio_switch_on(void);
int set_cop_spk_audio_switch_on(void);
int set_cop_spk_audio_switch_off(void);
int set_cop_voip_call_spk_audio_switch_on(void);
int set_cop_voip_mon_mic_audio_switch_on(void);
int set_occ_spk_audio_switch_on(void);

int set_tractor_spk_audio_switch_on(void);

int set_occ_mic_to_recode_audio_switch_on(void);
int set_occ_mic_to_recode_audio_switch_off(void);

int set_occ_mic_to_spk_audio_switch_on(void);
int set_occ_mic_to_spk_audio_switch_off(void);

int set_occ_mic_audio_switch_on(void);
int set_occ_mic_audio_switch_off(void);


int set_cop_spk_mute_onoff(int onoff);

int watchdog_pwm_on(void);
int watchdog_pwm_off(void);
int watchdog_gpio_low(void);
int watchdog_gpio_high(void);
int watchdog_gpio_toggle(void);

int phy_reset_low(void);
int phy_reset_high(void);

int occ_pa_en_ext_is_active(void);
int occ_pa_en_get_level(void);
int occ_tx_rx_en_ext_is_active(void);
int occ_pa_ack_set_high(void);
int occ_pa_ack_set_low(void);

int tractor_in_1_get_level(void);
int tractor_in_2_get_level(void);

int occ_pei_call_ack_set_low(void);
int occ_pei_call_ack_set_high(void);

int deadman_set_for_key(void);
int deadman_clear_for_key(void);

int cab_pa_in_set_for_key(void);
int cab_pa_in_clear_for_key(void);
int cab_pa_out_set_for_key(void);
int cab_pa_out_clear_for_key(void);
int cab_call_pei_set_for_key(void);
int cab_call_pei_clear_for_key(void);
int cab_call_cab_set_for_key(void);
int cab_call_cab_clear_for_key(void);

#endif // __KEY_H__
