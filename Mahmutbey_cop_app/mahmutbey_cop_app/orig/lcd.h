#ifndef __LCD_H__
#define __LCD_H__

#define DISP_STAT_NO_ACTIVE_NO_SELECT		1
#define DISP_STAT_NO_ACTIVE_SELECT		2
#define DISP_STAT_ACTIVE_SELECT			3
#define DISP_STAT_ACTIVE_NO_SELECT		4
#define DISP_STAT_ACTIVE_SELECT_MON		5
#define DISP_STAT_ACTIVE_SELECT_MON_EMER	6
#define DISP_STAT_ACTIVE_SELECT_MON_WAIT	7
#define DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT	8
#define DISP_STAT_ACTIVE_NO_SELECT_EMER		9
#define DISP_STAT_BLINK				10

#define CMD_STAT_SELECT		10
#define CMD_STAT_NO_SELECT	11
#define CMD_STAT_BLINK		12

#define LOG_MSG_COLOR_BLACK	1
#define LOG_MSG_COLOR_RED	2

#define LOG_MSG_STR_READY		1
#define LOG_MSG_STR_IP_SETUP_ERR	2
#define LOG_MSG_STR_NOT_CON_AVC		3
#define LOG_MSG_STR_PEI_HALF		4
#define LOG_MSG_STR_CAB_ID_ERROR	5
#define LOG_MSG_STR_CABCAB_WAITING	6
#define LOG_MSG_STR_CAB_REJECT_WAITING	7
#define LOG_MSG_STR_BUSY		8
#define LOG_MSG_STR_OCC_PA_BUSY		9
#define LOG_MSG_STR_CAB_PA_BUSY		10
#define LOG_MSG_STR_CABCAB_BUSY		11
#define LOG_MSG_STR_PEI_BUSY		12
#define LOG_MSG_STR_PEICAB_BUSY		13
#define LOG_MSG_STR_EMER_PEICAB_BUSY	14
#define LOG_MSG_STR_PEIOCC_BUSY		15
#define LOG_MSG_STR_FUNC_BUSY		16
#define LOG_MSG_STR_HELP_BUSY		17
#define LOG_MSG_STR_MAX			18

#define BUSY_LOG_START_IDX LOG_MSG_STR_CABCAB_WAITING
#define BUSY_LOG_END_IDX   LOG_MSG_STR_HELP_BUSY

void set_language_type(int set);
int lcd_init(void);
int font_init(void);

void setcolor(unsigned int colidx, unsigned value);
void pixel (int x, int y, unsigned int colidx);
void lcd_fillrect (int x1, int y1, int x2, int y2, unsigned int colidx);
void lcd_put_string(int x, int y, char *s, unsigned int colidx);

void __lcd_put_string(int x, int y, char *s, unsigned int colidx, int font_level);
void write_font_pa_area(void);

void menu_head_draw(int menu_active, int bcast_status, int func_status, int start_status, int station_status, int call_status, int vol_status, int iovol_status);
void draw_menu(int bcast_active, int call_active, int updown_idx, int leftright_idx);
void broadcast_draw_menu(int inout_stat1, int inout_stat2, int occ_stat, int func, int force_active, int active, int func_start);

void clear_menu_screen(void);
void draw_lines(void);
void call_draw_menu(struct multi_call_status *cab_list, struct multi_call_status *pei_list, int re_new, int is_pei_half, int is_talk);
void vol_ctrl_draw_menu(int mic_vol_up, int mic_active, int spk_vol_up, int spk_active, int re_new);
void iovol_ctrl_draw_menu(int inspk_vol_up, int inspk_active, int outspk_vol_up, int outspk_active, int re_new);

void display_cop_version(void);

void log_msg_draw(int str_idx);

#endif // __LCD_H__
