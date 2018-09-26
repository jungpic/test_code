#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "gpio_key_dev.h"

#define COP_GPIO_DEV_NAME	"/dev/cop_gpio"

#define DEBUG_LINE                      0
#if (DEBUG_LINE == 1)
#define DBG_LOG(fmt, args...)   \
        do {                    \
                fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);        \
        } while(0)
#else
#define DBG_LOG(fmt, args...)
#endif

#define IOC_GPIODEV_MAGIC  'C'
#define GET_KEY_VALUE					_IOWR(IOC_GPIODEV_MAGIC, 1, unsigned int)
#define SET_KEY_LED						_IOWR(IOC_GPIODEV_MAGIC, 2, unsigned int)
#define CLEAR_KEY_LED					_IOWR(IOC_GPIODEV_MAGIC, 3, unsigned int)
#define BLINK_KEY_LED_SET				_IOWR(IOC_GPIODEV_MAGIC, 4, unsigned int)
#define BLINK_KEY_LED_CLEAR				_IOWR(IOC_GPIODEV_MAGIC, 5, unsigned int)
#define GET_STATUS_BLINK_KEY_LED		_IOWR(IOC_GPIODEV_MAGIC, 6, unsigned int)

#define COP_PA_MIC_SWITCH_ON			_IOWR(IOC_GPIODEV_MAGIC, 7, unsigned int)
#define COP_CALL_MIC_SWITCH_ON			_IOWR(IOC_GPIODEV_MAGIC, 8, unsigned int)
#define COP_SPK_SWITCH_ON				_IOWR(IOC_GPIODEV_MAGIC, 9, unsigned int)
#define OCC_SPK_SWITCH_ON				_IOWR(IOC_GPIODEV_MAGIC, 10, unsigned int)

#define OCC_MIC_SWITCH_ON				_IOWR(IOC_GPIODEV_MAGIC, 11, unsigned int)
#define OCC_MIC_SWITCH_OFF				_IOWR(IOC_GPIODEV_MAGIC, 12, unsigned int)

#define OCC_MIC_TO_RECODE_SWITCH_ON		_IOWR(IOC_GPIODEV_MAGIC, 13, unsigned int)
#define OCC_MIC_TO_RECODE_SWITCH_OFF	_IOWR(IOC_GPIODEV_MAGIC, 14, unsigned int)

#define OCC_MIC_TO_SPK_SWITCH_ON		_IOWR(IOC_GPIODEV_MAGIC, 15, unsigned int)
#define OCC_MIC_TO_SPK_SWITCH_OFF		_IOWR(IOC_GPIODEV_MAGIC, 16, unsigned int)

#define COP_SPK_MUTE_CTRL				_IOWR(IOC_GPIODEV_MAGIC, 17, unsigned int)
	
#define WATCHDOG_PWM_ON					_IOWR(IOC_GPIODEV_MAGIC, 18, unsigned int)
#define WATCHDOG_PWM_OFF				_IOWR(IOC_GPIODEV_MAGIC, 19, unsigned int)
#define WATCHDOG_GPIO_LOW				_IOWR(IOC_GPIODEV_MAGIC, 20, unsigned int)
#define WATCHDOG_GPIO_HIGH				_IOWR(IOC_GPIODEV_MAGIC, 21, unsigned int)
#define WATCHDOG_GPIO_TOGGLE			_IOWR(IOC_GPIODEV_MAGIC, 22, unsigned int)

#define PHY_RESET_HIGH					_IOWR(IOC_GPIODEV_MAGIC, 23, unsigned int)
#define PHY_RESET_LOW					_IOWR(IOC_GPIODEV_MAGIC, 24, unsigned int)

#define OCC_PA_EN_GET_LEVEL				_IOWR(IOC_GPIODEV_MAGIC, 25, unsigned int)
#define OCC_PA_ACK_SET_HIGH				_IOWR(IOC_GPIODEV_MAGIC, 26, unsigned int)
#define OCC_PA_ACK_SET_LOW				_IOWR(IOC_GPIODEV_MAGIC, 27, unsigned int)

/* something mismatch from here */

//#define OCC_TX_RX_EN_GET_LEVEL			_IOWR(IOC_GPIODEV_MAGIC, 0/*28*/, unsigned int)
//#define OCC_PEI_CALL_ACK_SET_HIGH		_IOWR(IOC_GPIODEV_MAGIC, 0/*29*/, unsigned int)
//#define OCC_PEI_CALL_ACK_SET_LOW		_IOWR(IOC_GPIODEV_MAGIC, 0/*30*/, unsigned int)

#define LCD_PWR_OFF						_IOWR(IOC_GPIODEV_MAGIC, 28/*31*/, unsigned int)
#define LCD_PWR_ON						_IOWR(IOC_GPIODEV_MAGIC, 29/*32*/, unsigned int)

#define DEADMAN_SET_FOR_KEY				_IOWR(IOC_GPIODEV_MAGIC, 30/*33*/, unsigned int)
#define DEADMAN_CLEAR_FOR_KEY			_IOWR(IOC_GPIODEV_MAGIC, 31/*34*/, unsigned int)

#define CAB_PA_IN_SET_FOR_KEY 			_IOWR(IOC_GPIODEV_MAGIC, 32/*35*/, unsigned int)
#define CAB_PA_IN_CLEAR_FOR_KEY 		_IOWR(IOC_GPIODEV_MAGIC, 33/*36*/, unsigned int)

#define CAB_PA_OUT_SET_FOR_KEY			_IOWR(IOC_GPIODEV_MAGIC, 34/*37*/, unsigned int)
#define CAB_PA_OUT_CLEAR_FOR_KEY		_IOWR(IOC_GPIODEV_MAGIC, 35/*38*/, unsigned int)

#define CAB_CALL_PEI_SET_FOR_KEY		_IOWR(IOC_GPIODEV_MAGIC, 36/*39*/, unsigned int)
#define CAB_CALL_PEI_CLEAR_FOR_KEY		_IOWR(IOC_GPIODEV_MAGIC, 37/*40*/, unsigned int)

#define CAB_CALL_CAB_SET_FOR_KEY		_IOWR(IOC_GPIODEV_MAGIC, 38/*41*/, unsigned int)
#define CAB_CALL_CAB_CLEAR_FOR_KEY		_IOWR(IOC_GPIODEV_MAGIC, 39/*42*/, unsigned int)

//#define FIRE_SET_FOR_KEY				_IOWR(IOC_GPIODEV_MAGIC, 43, unsigned int)
//#define FIRE_CLEAR_FOR_KEY				_IOWR(IOC_GPIODEV_MAGIC, 44, unsigned int)

//#define TRACTOR_SET_FOR_KEY				_IOWR(IOC_GPIODEV_MAGIC, 45, unsigned int)
//#define TRACTOR_CLEAR_FOR_KEY			_IOWR(IOC_GPIODEV_MAGIC, 46, unsigned int)

//#define TRACTOR_IN1_GET_LEVEL       	_IOWR(IOC_GPIODEV_MAGIC, 47, unsigned int)
//#define TRACTOR_IN2_GET_LEVEL       	_IOWR(IOC_GPIODEV_MAGIC, 48, unsigned int)

//#define TRACTOR_CALL_OUT1_SET_FOR_KEY       _IOWR(IOC_GPIODEV_MAGIC, 49, unsigned int)
//#define TRACTOR_CALL_OUT2_SET_FOR_KEY       _IOWR(IOC_GPIODEV_MAGIC, 50, unsigned int)

//#define TRACTOR_CALL_OUT1_CLEAR_FOR_KEY     _IOWR(IOC_GPIODEV_MAGIC, 51, unsigned int)
//#define TRACTOR_CALL_OUT2_CLEAR_FOR_KEY     _IOWR(IOC_GPIODEV_MAGIC, 52, unsigned int)

//#define TRACTOR_SPK_SWITCH_ON				_IOWR(IOC_GPIODEV_MAGIC, 53, unsigned int)
//#define TRACTOR_MIC_SWITCH_ON				_IOWR(IOC_GPIODEV_MAGIC, 54, unsigned int)
//#define TRACTOR_MIC_SWITCH_OFF				_IOWR(IOC_GPIODEV_MAGIC, 55, unsigned int)

//#define TRACTOR_MIC_BASE_MIC                _IOWR(IOC_GPIODEV_MAGIC, 56, unsigned int)
//#define TRACTOR_MIC_SIGOUT                  _IOWR(IOC_GPIODEV_MAGIC, 57, unsigned int)
//#define TRACTOR_MIC_OFF                     _IOWR(IOC_GPIODEV_MAGIC, 58, unsigned int)

//#define COP_PA_MIC_PTT_LEVEL				_IOWR(IOC_GPIODEV_MAGIC, 59, unsigned int)

#define COP_SPK_SWITCH_OFF					_IOWR(IOC_GPIODEV_MAGIC, 51, unsigned int)

/* <<<< VOIP */
#define COP_VOIP_CALL_SPK_SWITCH_ON			_IOWR(IOC_GPIODEV_MAGIC, 60, unsigned int)
#define COP_VOIP_MON_MIC_SWITCH_ON			_IOWR(IOC_GPIODEV_MAGIC, 61, unsigned int)
/* VOIP >>>> */

extern int occ_pei_call_out_level;

static int fd_gpio_key;

/*** AUDIO SWITCH IOCTL *****************************************/
#define COP_MIC_INPUT_IS_COP	1
#define COP_MIC_INPUT_IS_OCC	2
/****************************************************************/

int key_dev_open(void)
{
    //DBG_LOG("\n");
    fd_gpio_key = open(COP_GPIO_DEV_NAME, O_RDWR);
    if (fd_gpio_key < 0) {
        printf("Cannot open %s.\n", COP_GPIO_DEV_NAME);
        return -1;
    }

    return 0;
}

int key_init(void)
{
    enum __cop_key_enum key;

    //DBG_LOG("\n");
    for (key = enum_COP_KEY_PA_IN; key <= enum_COP_KEY_LONG_KEY; key++) {
        clear_key_led(key);
		set_key_led_blink(key, 0);
    }

    return 0;
}

int key_exit(void)
{
    if (fd_gpio_key < 0)
        return -1;

    //DBG_LOG("\n");
    close(fd_gpio_key);

    return 0;
}

int set_key_led(enum __cop_key_enum keyval)
{
    int ret, val;
    val = (int)keyval;

    DBG_LOG("keval = %d\n", keval);
    ret = ioctl(fd_gpio_key, SET_KEY_LED, &val);

    return ret;
}

int clear_key_led(enum __cop_key_enum keyval)
{
    int ret, val;
    val = (int)keyval;

    DBG_LOG("keval = %d\n", keval);
    ret = ioctl(fd_gpio_key, CLEAR_KEY_LED, &val);

    return ret;
}

int get_key_value(void)
{
    int ret, val;

    ret = ioctl(fd_gpio_key, GET_KEY_VALUE, &val);
    if (ret < 0)
        return -1;

    return val;
}

int set_key_led_blink(enum __cop_key_enum keyval, int blinkonoff)
{
    int ret, val;
    val = (int)keyval;

    //DBG_LOG("\n");
    if (blinkonoff)
        ret = ioctl(fd_gpio_key, BLINK_KEY_LED_SET, &val);
    else
        ret = ioctl(fd_gpio_key, BLINK_KEY_LED_CLEAR, &val);

    return ret;
}

int get_status_key_blink(enum __cop_key_enum keyval)
{
    int ret, val;
    val = (int)keyval;

    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, GET_STATUS_BLINK_KEY_LED, &val);

    return ret;
}

/*** AUDIO SWITCH IOCTL *********************************************/
int set_cop_pa_mic_audio_switch_on(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, COP_PA_MIC_SWITCH_ON, 0);

    return ret;
}

int set_cop_call_mic_audio_switch_on(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, COP_CALL_MIC_SWITCH_ON, 0);

    return ret;
}

int set_cop_spk_audio_switch_on(void)
{
    int ret = 0;
 
    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, COP_SPK_SWITCH_ON, 0);// PE12: 0
    return ret;
}

int set_cop_spk_audio_switch_off(void)
{
	int ret = 0;

	DBG_LOG("\n");
	ret = ioctl(fd_gpio_key, COP_SPK_SWITCH_OFF, 0);
	return ret;
}

int set_cop_voip_call_spk_audio_switch_on(void)
{
    int ret = 0;
 
    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, COP_VOIP_CALL_SPK_SWITCH_ON, 0);
    return ret;
}

int set_cop_voip_mon_mic_audio_switch_on(void)
{
    int ret = 0;
 
    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, COP_VOIP_MON_MIC_SWITCH_ON, 0);
    return ret;
}

int set_occ_spk_audio_switch_on(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_SPK_SWITCH_ON, 0); // PE12: 0
    return ret;
}

int set_tractor_spk_audio_switch_on(void)
{
    int ret = 0;

    DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, TRACTOR_SPK_SWITCH_ON, 0); // PA5 : 1
    return ret;
}

int set_tractor_mic_audio_switch_on(void)
{
	int ret = 0;

	DBG_LOG("\n");
	//ret = ioctl(fd_gpio_key, TRACTOR_MIC_SWITCH_ON, 0); // PA5 : 1

	return ret;
}

int set_occ_mic_audio_switch_on(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_MIC_SWITCH_ON, 0); /* PC7:0, PC9:0 */
    return ret;
}

int set_occ_mic_audio_switch_off(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_MIC_SWITCH_OFF, 0); /* PC9: 1 */
    return ret;
}

int set_tractor_mic_base_mic(void)
{
	int ret = 0;

	DBG_LOG("\n");
	//ret = ioctl(fd_gpio_key, TRACTOR_MIC_BASE_MIC, 0); // PA5 : 1

	return ret;
}

int set_tractor_mic_sigout(void)
{
	int ret = 0;

	DBG_LOG("\n");
	//ret = ioctl(fd_gpio_key, TRACTOR_MIC_SIGOUT, 0); // PA5 : 1

	return ret;
}

int set_tractor_mic_off(void)
{
	int ret = 0;

	DBG_LOG("\n");
	//ret = ioctl(fd_gpio_key, TRACTOR_MIC_OFF, 0); // PA5 : 1

	return ret;
}

int set_occ_mic_to_recode_audio_switch_on(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_MIC_TO_RECODE_SWITCH_ON, 0);
    return ret;
}

int set_occ_mic_to_recode_audio_switch_off(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_MIC_TO_RECODE_SWITCH_OFF, 0);
    return ret;
}

int set_occ_mic_to_spk_audio_switch_on(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_MIC_TO_SPK_SWITCH_ON, 0);
    return ret;
}

int set_occ_mic_to_spk_audio_switch_off(void)
{
    int ret = 0;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_MIC_TO_SPK_SWITCH_OFF, 0);
    return ret;
}

/****************************************************************/

int set_cop_spk_mute_onoff(int onoff)
{
    int ret = 0;
    int mute_on = onoff;

    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, COP_SPK_MUTE_CTRL, &mute_on);

    return ret;
}

#if 0
void key_test(void)
{
    int val, ret = 0;
    enum __cop_key_enum key_val;
    static int in_key_blink_on = 0;
    static int out_key_blink_on = 0;

    val = get_key_value();
    key_val = (enum __cop_key_enum)val;

    switch (key_val) {
        case enum_COP_KEY_PA_IN:
            printf("KEY is COP_KEY_PA_IN\n");
            if (!in_key_blink_on) {
                ret = ioctl(fd_gpio_key, BLINK_KEY_LED_SET, &val);
                in_key_blink_on = 1;
                printf("IN KEY blink on\n");
            }
            else {
                ret = ioctl(fd_gpio_key, BLINK_KEY_LED_CLEAR, &val);
                in_key_blink_on = 0;
                printf("IN KEY blink off\n");
            }
            break;

        case enum_COP_KEY_PA_OUT:
            printf("KEY is COP_KEY_PA_OUT\n");
            if (!out_key_blink_on) {
                ret = ioctl(fd_gpio_key, BLINK_KEY_LED_SET, &val);
                out_key_blink_on = 1;
                printf("OUT KEY blink on\n");
            }
            else {
                ret = ioctl(fd_gpio_key, BLINK_KEY_LED_CLEAR, &val);
                out_key_blink_on = 0;
                printf("OUT KEY blink off\n");
            }
            break;

        case enum_COP_KEY_CALL_CAB:
            printf("KEY is COP_KEY_CALL_CAB\n");
            break;
        case enum_COP_KEY_CALL_PEI:
            printf("KEY is COP_KEY_CALL_PEI\n");
            break;

        case enum_COP_KEY_NONE:
        case enum_COP_KEY_MAX:
            break;
    }
}
#endif

/****************************************************************/
int watchdog_pwm_on(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, WATCHDOG_PWM_ON, 0);
    return ret;
}

int watchdog_pwm_off(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, WATCHDOG_PWM_OFF, 0);
    return ret;
}

int watchdog_gpio_low(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, WATCHDOG_GPIO_LOW, 0);
    return ret;
}

int watchdog_gpio_high(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, WATCHDOG_GPIO_HIGH, 0);
    return ret;
}

int watchdog_gpio_toggle(void)
{
    int ret;
    ret = ioctl(fd_gpio_key, WATCHDOG_GPIO_TOGGLE, 0);
    return ret;
}

int phy_reset_high(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, PHY_RESET_HIGH, 0);
    return ret;
}

int phy_reset_low(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, PHY_RESET_LOW, 0);
    return ret;
}

/*int cop_pa_mic_ptt_get_level(void)
{
	int val = -1;
	DBG_LOG("\n");
	ioctl(fd_gpio_key, COP_PA_MIC_PTT_LEVEL, &val);
	return val;
}*/

int occ_pa_en_get_level(void)
{
    int val = -1;
    //DBG_LOG("\n");
    ioctl(fd_gpio_key, OCC_PA_EN_GET_LEVEL, &val);
    return val;
}

int tractor_in_1_get_level(void)
{
    int val = -1;
    //DBG_LOG("\n");
    //ioctl(fd_gpio_key, TRACTOR_IN1_GET_LEVEL, &val);
    return val;
}

int tractor_out_1_set_high(void)
{
    int val = -1;
    //DBG_LOG("\n");
    //val = ioctl(fd_gpio_key, TRACTOR_CALL_OUT1_SET_FOR_KEY, 0);
    return val;
}

int tractor_out_2_set_high(void)
{
    int val = -1;
    //DBG_LOG("\n");
    //val = ioctl(fd_gpio_key, TRACTOR_CALL_OUT2_SET_FOR_KEY, 0);
    return val;
}

int tractor_out_1_set_low(void)
{
    int val = -1;
    //DBG_LOG("\n");
    //val = ioctl(fd_gpio_key, TRACTOR_CALL_OUT1_CLEAR_FOR_KEY, 0);
    return val;
}

int tractor_out_2_set_low(void)
{
    int val = -1;
    //DBG_LOG("\n");
    //val = ioctl(fd_gpio_key, TRACTOR_CALL_OUT2_CLEAR_FOR_KEY, 0);
    return val;
}

int tractor_in_2_get_level(void)
{
    int val = -1;
    //DBG_LOG("\n");
    //ioctl(fd_gpio_key, TRACTOR_IN2_GET_LEVEL, &val);
    return val;
}

int occ_pa_en_ext_is_active(void)
{
    int val, ret = 0;
    //DBG_LOG("\n");
    ioctl(fd_gpio_key, OCC_PA_EN_GET_LEVEL, &val);

//printf("%s(), val:%d\n", __func__, val);
	if (val == 1) ret = 1;

    return ret;
}

int occ_pa_ack_set_high(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_PA_ACK_SET_HIGH, 0);
    return ret;
}

int occ_pa_ack_set_low(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, OCC_PA_ACK_SET_LOW, 0);
    return ret;
}

int occ_tx_rx_en_get_level(void)
{
    int val = -1;
    //DBG_LOG("\n");
    //ioctl(fd_gpio_key, OCC_TX_RX_EN_GET_LEVEL, &val);
    return val;
}

int occ_tx_rx_en_ext_is_active(void)
{
    int val, ret = 0;
    //DBG_LOG("\n");
    //ioctl(fd_gpio_key, OCC_TX_RX_EN_GET_LEVEL, &val);

//printf("%s(), val:%d\n", __func__, val);
	if (val == 1) ret = 1;

    return ret;
}

int occ_pei_call_ack_set_low(void)
{
    int ret=-1;
    //DBG_LOG("\n");
	occ_pei_call_out_level = 0;
    //ret = ioctl(fd_gpio_key, OCC_PEI_CALL_ACK_SET_LOW, 0);
    return ret;
}

int occ_pei_call_ack_set_high(void)
{
    int ret=-1;
    //DBG_LOG("\n");
	occ_pei_call_out_level = 1;
    //ret = ioctl(fd_gpio_key, OCC_PEI_CALL_ACK_SET_HIGH, 0);
    return ret;
}

int lcd_pwer_off(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, LCD_PWR_OFF, 0);
    return ret;
}

int lcd_pwer_on(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, LCD_PWR_ON, 0);
    return ret;
}

int fire_set_for_key(void)
{
	int ret=-1;
	//DBG_LOG("\n");
	//ret = ioctl(fd_gpio_key, FIRE_SET_FOR_KEY, 0);
	return ret;
}

int tractor_set_for_key(void)
{
	int ret=-1;
	//DBG_LOG("\n");
	//ret = ioctl(fd_gpio_key, TRACTOR_SET_FOR_KEY, 0);
	return ret;
}

int deadman_set_for_key(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, DEADMAN_SET_FOR_KEY, 0);
    return ret;
}

int fire_clear_for_key(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, FIRE_CLEAR_FOR_KEY, 0);
    return ret;
}

int tractor_clear_for_key(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, TRACTOR_CLEAR_FOR_KEY, 0);
    return ret;
}

int deadman_clear_for_key(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, DEADMAN_CLEAR_FOR_KEY, 0);
    return ret;
}

int cab_pa_in_set_for_key(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, CAB_PA_IN_SET_FOR_KEY, 0);
    return ret;
}

int cab_pa_in_clear_for_key(void)
{
    int ret=-1;
    //DBG_LOG("\n");
    //ret = ioctl(fd_gpio_key, CAB_PA_IN_CLEAR_FOR_KEY, 0);
    return ret;
}

int cab_pa_out_set_for_key(void)
{
#if 0
    int ret;
    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, CAB_PA_OUT_SET_FOR_KEY, 0);
    return ret;
#else
    //DBG_LOG("\n");
    return 0; // <=== BUSAN
#endif
}

int cab_pa_out_clear_for_key(void)
{
#if 0
    int ret;
    DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, CAB_PA_OUT_CLEAR_FOR_KEY, 0);
    return ret;
#else
    //DBG_LOG("\n");
    return 0; // <=== BUSAN
#endif
}

int cab_call_pei_set_for_key(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, CAB_CALL_PEI_SET_FOR_KEY, 0);
    return ret;
}

int cab_call_pei_clear_for_key(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, CAB_CALL_PEI_CLEAR_FOR_KEY, 0);
    return ret;
}

int cab_call_cab_set_for_key(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, CAB_CALL_CAB_SET_FOR_KEY, 0);
    return ret;
}

int cab_call_cab_clear_for_key(void)
{
    int ret;
    //DBG_LOG("\n");
    ret = ioctl(fd_gpio_key, CAB_CALL_CAB_CLEAR_FOR_KEY, 0);
    return ret;
}


