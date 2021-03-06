#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <QTimer>
#include <QSize>
#include <QStyleOptionViewItem>
#include <QItemDelegate>
#include <QThread>
#include <QtNetwork>
#include <QtNetwork/QUdpSocket>

#include <QButtonGroup>
#include <QMouseEvent>

#include "cop_main.h"
#include "calibration.h"
#include "scribblewidget.h"
#include "sub_device.h"
#include "file_upload.h"

#include "updatemainframe.h"

#include "progressdlg.h"
#include "ftpclient.h"

#define DISP_STAT_NO_ACTIVE_NO_SELECT           1
#define DISP_STAT_NO_ACTIVE_SELECT              2
#define DISP_STAT_ACTIVE_SELECT                 3
#define DISP_STAT_ACTIVE_NO_SELECT              4
#define DISP_STAT_ACTIVE_SELECT_MON             5
#define DISP_STAT_ACTIVE_SELECT_MON_EMER        6
#define DISP_STAT_ACTIVE_SELECT_MON_WAIT        7
#define DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT   8
#define DISP_STAT_ACTIVE_NO_SELECT_EMER         9
#define DISP_STAT_BLINK                         10

#define LOG_MESSAGE_DISPLAY_TIME		2000			// Miliseconds

#define SCREEN_UPDATE_TIME              500             // Miliseconds
#define TCP_CONNECT_UPDATE_TIME         500             // Miliseconds
#define PEI_CALL_UPDATE_TIME            500             // Miliseconds
#define CAB_CALL_UPDATE_TIME            500             // Miliseconds
#define BTN_BLINK_TIME                  500             // Miliseconds
#define OCC_BTN_UPDATE_TIME             500             // Miliseconds
#define INOUT_BTN_UPDATE_TIME           100             // Miliseconds
#define CAB_BTN_ENABLE_TIME             500             // Miliseconds
#define CTM_STATUS_UPDATE_TIME          100             // Miliseconds

#define TOP_MENU		0
#define CONFIG_MENU		1
#define CALL_MENU		2
#define ROUTE_S_MENU	3
#define DI_MENU			4
//-----------------------
#define STATION_MENU	5
#define STATION_PA_MENU 6
#define ROUTE_MENU		7
//-----------------------
#define STATUS_MENU		8
#define MANAGER_MENU	9
//-----------------------
#define SWUPDATE_MENU	10
#define LOGDOWN_MENU	12

#define LANG_TURKISH            0
#define LANG_ENGLISH            1

#define BTN_ENABLE              1
#define BTN_DISABLE             0

#define CALIBRATION_FILE        "/etc/pointercal"

enum TOP_BTN_ID
{
	TOP_BTN_ID_NONE,

	TOP_BTN_ID_1,
	TOP_BTN_ID_2,
	TOP_BTN_ID_3,
	TOP_BTN_ID_4,
	TOP_BTN_ID_5,
	TOP_BTN_ID_6,
	TOP_BTN_ID_7,
	TOP_BTN_ID_8,
	TOP_BTN_ID_9,
	TOP_BTN_ID_IN,
	TOP_BTN_ID_OUT,
	TOP_BTN_ID_LRM,
	TOP_BTN_ID_PIB,
	TOP_BTN_ID_UP,
	TOP_BTN_ID_DOWN,
	TOP_BTN_ID_STATION,

	TOP_BTN_ID_MAX,
};

enum DI_BTN_ID
{
	DI_BTN_ID_NONE,

	DI_BTN_ID_1,
	DI_BTN_ID_2,
	DI_BTN_ID_3,
	DI_BTN_ID_4,
	DI_BTN_ID_5,
	DI_BTN_ID_6,
	DI_BTN_ID_7,
	DI_BTN_ID_8,
	DI_BTN_ID_9,
	DI_BTN_ID_10,
	DI_BTN_ID_OK,
	DI_BTN_ID_STOP,

	DI_BTN_ID_MAX,
};

enum ROUTE_BTN_ID
{
	ROUTE_BTN_ID_NONE,

	ROUTE_BTN_ID_01,
	ROUTE_BTN_ID_02,
	ROUTE_BTN_ID_03,
	ROUTE_BTN_ID_04,
	ROUTE_BTN_ID_05,
	ROUTE_BTN_ID_06,
	ROUTE_BTN_ID_07,
	ROUTE_BTN_ID_08,
	ROUTE_BTN_ID_09,
	ROUTE_BTN_ID_10,
	ROUTE_BTN_ID_11,
	ROUTE_BTN_ID_12,
	ROUTE_BTN_ID_13,
	ROUTE_BTN_ID_14,
	ROUTE_BTN_ID_15,
	ROUTE_BTN_ID_16,
	ROUTE_BTN_ID_DEPARTURE,
	ROUTE_BTN_ID_DESTINATION,
	ROUTE_BTN_ID_OK,

	ROUTE_BTN_ID_MAX,
};

enum ROUTE_S_BTN_ID
{
	ROUTE_S_BTN_ID_NONE,

	ROUTE_S_BTN_ID_1,
	ROUTE_S_BTN_ID_2,
	ROUTE_S_BTN_ID_3,
	ROUTE_S_BTN_ID_4,
	ROUTE_S_BTN_ID_UP,
	ROUTE_S_BTN_ID_DOWN,
	ROUTE_S_BTN_ID_LINE_SELECT_1,
	ROUTE_S_BTN_ID_LINE_SELECT_2,

	ROUTE_S_BTN_ID_MAX,
};

enum STATION_BTN_ID
{
	STATION_BTN_ID_NONE,

	STATION_BTN_ID_01,
	STATION_BTN_ID_02,
	STATION_BTN_ID_03,
	STATION_BTN_ID_04,
	STATION_BTN_ID_05,
	STATION_BTN_ID_06,
	STATION_BTN_ID_07,
	STATION_BTN_ID_08,
	STATION_BTN_ID_09,
	STATION_BTN_ID_10,
	STATION_BTN_ID_11,
	STATION_BTN_ID_12,
	STATION_BTN_ID_13,
	STATION_BTN_ID_14,
	STATION_BTN_ID_15,
	STATION_BTN_ID_16,
	STATION_BTN_ID_LINE_SELECT_1,
	STATION_BTN_ID_LINE_SELECT_2,
	STATION_BTN_ID_BACK,

	STATION_BTN_ID_MAX,
};

enum STATION_PA_BTN_ID
{
	STATION_PA_BTN_ID_NONE,

	STATION_PA_BTN_ID_1,
	STATION_PA_BTN_ID_2,
	STATION_PA_BTN_ID_3,
	STATION_PA_BTN_ID_4,
	STATION_PA_BTN_ID_5,
	STATION_PA_BTN_ID_6,
	STATION_PA_BTN_ID_7,
	STATION_PA_BTN_ID_8,
	STATION_PA_BTN_ID_9,
	STATION_PA_BTN_ID_UP,
	STATION_PA_BTN_ID_DOWN,
	STATION_PA_BTN_ID_BACK,

	STATION_PA_BTN_ID_MAX,
};

enum STATUS_BTN_ID
{
	STATUS_BTN_ID_NONE,

	STATUS_BTN_UP,
	STATUS_BTN_DOWN,

    STATUS_BTN_ID_MAX,
};

enum SUB_DEV_ID
{
    ID_NONE,
    PIB,
    SCAM,
    FCAM,
    COP,
    PAMP,
    PEI,
    LRM,
    PID,
    DIF,
    NVR
};

/*device index*/
#define PIB_INDEX   0
#define SCAM_INDEX  8
#define FCAM_INDEX  24
#define COP_INDEX   26
#define PAMP_INDEX  28
#define PEI_INDEX   36
#define LRM_INDEX   52
#define PID_INDEX   84
#define DIF_INDEX   132
#define NVR_INDEX   142



#define MAX_DEVICE_LEN 255
#define DEVICE_CNT 40
#define SUB_DEVICE_CNT 136
 //char SUB_DEVICE_NAME[DEVICE_CNT][MAX_DEVICE_LEN];
/*
typedef struct _st_ctm_device{
    char SUB_DEVICE_NAME[MAX_DEVICE_LEN];
}STRUCT_CTM_DEVICE;

static STRUCT_CTM_DEVICE stCTMdevice[DEVICE_CNT] = {
    {"AVC"},{"COP"},{"FDI"},{"PIB_1"},{"PIB_2"},{"SDI_1"},{"SDI_2"},
};*/

extern "C" {
void force_reboot(void);

void mic_volume_step_up(int add_val, int active);
void mic_volume_step_down(int sub_val, int active);

void spk_volume_step_up(int add_val, int active);
void spk_volume_step_down(int sub_val, int active);

void inspk_volume_step_up(int add_val, int active);
void inspk_volume_step_down(int sub_val, int active);
void outspk_volume_step_up(int sub_val, int active);
void outspk_volume_step_down(int sub_val, int active);
int __pa_in_key_process (int key_pressed_status);
int __pa_out_key_process (int key_pressed_status);
int __call_cab_key_process (int key_pressed_status);
int __call_pei_key_process (int key_pressed_status);
int __occ_key_process (int key_pressed_status, int idx);
int occ_pa_en_get_level(void);

int __pa_function_process (int func_num, unsigned char selected, unsigned char is_station_fpa);
int __pa_function_force_stop (int func_num, unsigned char selected, unsigned char is_station_fpa);
void update_pamp_volume_level(void);
int send_broadcast_stop_request(void);

int get_func_is_now_start(void);
int process_join_start(void);

int cmd_di_display_start(unsigned int id);
int cmd_di_display_stop(void);
int cmd_set_route(unsigned int departure, unsigned int destination);
int cmd_set_special_route_start(unsigned short code);
int cmd_set_special_route_stop(unsigned short code);
int send_cop2avc_cmd_sw_version_request(char SWversion[], char add4,char add3,char add2,char add1);
int send_cop2avc_cmd_sw_update_start(short SWversion, char add4,char add3,char add2,char add1,int DeviceId);
int copy_usb2tmp(void);
//int run_ftp_upload_execlp(void);
//int run_ftp_all_upload_execlp(void);
int run_ftp_upload_execlp(char* AVC_IP, char* Update_filename);
int run_ftp_all_mc1_upload_execlp(void);
int run_ftp_all_mc2_upload_execlp(void);
}

class updateMainFrame;

namespace Ui {
class MainWindow;
}

class RowHeightDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {        
        return QSize(1, 40);
    }
};


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    COP_main            *pThread;
    Calibration         *TouchCal;
    ScribbleWidget      *Scribble;
    Sub_device          *m_thread;
    File_Upload         *file_thread;
  //  progressDlg         *m_progressDlg;
   // ftpClient           *m_ftpClient;
    ftpClient*      m_pFtpclient;
    progressDlg*    m_pDlgProgress;

    Ui::MainWindow *ui;
    Ui::MainWindow* getui() { return ui; }
private slots:
    void ConnectUpdate(void);
    void ScreenUpdate(void);
    void PEI_CallUpdate(void);
    void CAB_CallUpdate(void);
    void StatusUpdate(void);
    void SoftwareUpdateStatus(void);
    void PEI_BTN_1_BLINK_Update(void);
    void PEI_BTN_2_BLINK_Update(void);
    void CAB_BTN_1_BLINK_Update(void);
    void CAB_BTN_2_BLINK_Update(void);
    void ReadCalibrationData(QString filename);
	void mousePressEvent(QMouseEvent* event);

	void menu_button_clicked(int);
	void top_button_clicked(int);
	void di_button_clicked(int);
	void route_button_clicked(int);
	void special_route_button_clicked(int);
	void station_button_clicked(int);
	void station_PA_button_clicked(int);
	void status_button_clicked(int);

	void on_BUTTON_CANCEL_clicked();

    void on_CAB_MIC_UP_clicked();
    void on_CAB_MIC_DOWN_clicked();
    void on_CAB_SPK_UP_clicked();
    void on_CAB_SPK_DOWN_clicked();

    void on_PAMP_IN_VOL_UP_clicked();
    void on_PAMP_IN_VOL_DOWN_clicked();
    void on_PAMP_OUT_VOL_UP_clicked();
    void on_PAMP_OUT_VOL_DOWN_clicked();

	void on_CALL_MIC_UP_clicked();
	void on_CALL_MIC_DOWN_clicked();
	void on_CALL_SPK_UP_clicked();
	void on_CALL_SPK_DOWN_clicked();

    //void on_PASSIVE_IN_clicked();
    //void on_PASSIVE_OUT_clicked();

	void on_PEI_JOIN_clicked();
    //void on_CAB_BUTTON_clicked();
    //void on_PEI_BUTTON_clicked();
    //void on_OCC_BUTTON_clicked();

    void on_LANG_COMBO_currentIndexChanged(int index);

    void on_TOUCH_CAL_clicked();

    void on_NUMBER_1_clicked();
    void on_NUMBER_2_clicked();
    void on_NUMBER_3_clicked();
    void on_NUMBER_4_clicked();
    void on_NUMBER_5_clicked();
    void on_NUMBER_6_clicked();
    void on_NUMBER_7_clicked();
    void on_NUMBER_8_clicked();
    void on_NUMBER_9_clicked();
    void on_NUMBER_0_clicked();

    void on_NUMBER_OK_clicked();

    void on_NUMBER_RESET_clicked();

  //  void on_SUBDEVICE_1_clicked();
    void on_SUBDEVICE_PIB_clicked();
    void on_SUBDEVICE_SCAM_clicked();
    void on_SUBDEVICE_FCAM_clicked();
    void on_SUBDEVICE_COP_clicked();
    void on_SUBDEVICE_PAMP_clicked();
    void on_SUBDEVICE_PEI_clicked();
    void on_SUBDEVICE_LRM_clicked();
    void on_SUBDEVICE_PID_clicked();
    void on_SUBDEVICE_DIF_clicked();
    void on_SUBDEVICE_NVR_clicked();
    void on_SUBDEVICE_FILE_UP_clicked();

    //void on_thread_finish(const int value);
    //void on_file_thread_finish(const int value);
    void SwUpdateCellSelected(int, int);    
    void finishedUpdateFileList_1(void);
    void on_bt_LIST_DOWN_clicked();
    void on_bt_LIST_UP_clicked();
    void ftpClient_finished( qint32,qint32 );

//    void ftpClient_finished( qint32,qint32 ) ;

private:


    //QUdpSocket      *PIB_ReceiverSocket;
    //QHostAddress    PIB_ReceiverAddress;

    unsigned int    language_selection;

    unsigned char   passive_in_status;
    unsigned char   passive_out_status;
    unsigned char   occ_status;
    unsigned char   join_request;

    unsigned char   PEI_BTN_1_BlinkTimer_Status, pei_1;
    unsigned char   PEI_BTN_2_BlinkTimer_Status, pei_2;
    unsigned char   CAB_BTN_1_BlinkTimer_Status, cab_1;
    unsigned char   CAB_BTN_2_BlinkTimer_Status, cab_2;

	int 			last_done_avc_tcp_connected;
	unsigned char	focus_menu_button_index;
	unsigned char	func_selected;
	int				func_activated_code;
	int				special_route_activated_code;
	unsigned char	route_selected;
	int				route_line_selected;
	int				station_pa_line_selected;

	int				ui_di_display_id;
	int				ui_departure_station_id;
	int				ui_destination_stataion_id;
	int				ui_special_route_pattern_no;

	QButtonGroup	*pMenuBtnGroup;
	QButtonGroup	*pTopBtnGroup;
	QButtonGroup	*pDiBtnGroup;
	QButtonGroup	*pRouteBtnGroup;
	QButtonGroup	*pRouteSBtnGroup;
	QButtonGroup	*pStationBtnGroup;
	QButtonGroup	*pStation_PA_BtnGroup;
	QButtonGroup	*pStatus_BtnGroup;

	QTime			log_busy_display_time;

    QTimer          *ScreenUpdateTimer;
    QTimer          *TcpConnectedTimer;
    QTimer          *PEI_CallUpdateTimer;
    QTimer          *PEI_BTN_1_BlinkTimer;
    QTimer          *PEI_BTN_2_BlinkTimer;
    QTimer          *CAB_CallUpdateTimer;
    QTimer          *CAB_BTN_1_BlinkTimer;
    QTimer          *CAB_BTN_2_BlinkTimer;
	
	QTimer          *CTM_StatusUpdateTimer;
    QTimer          *CTM_SWUpdateTimer;

	void activate_button(int enable);
	void activate_button_for_route(int enable);
	void activate_button_for_station_PA(int enable);
	void refresh_menu_buttons_focus(void);
	void refresh_func_buttons_focus(void);
	void refresh_di_and_route(void);
	void refresh_func_broadcast_name(void);
	void refresh_station_fpa_name(void);
	void refresh_special_route_name(void);
	void pick_out_station_fpa(int code);
	void refresh_status_device(void);
    void status_update_single_MC2(void);
    void status_update_single_MC1(void);
    void status_update_single_M(void);
    void status_update_single_T(void);
    void status_update_dual_MC2(void);
    void status_update_dual_MC1(void);
    void status_update_dual_M(void);
    void status_update_dual_T(void);
    void setupSubDeviceItem(void);
    void initSubDeviceStatus(void);
    void initSubDevIcon(void);
    void initNumberIcon(void);

	unsigned short calc_car_num_from_train_num(char train_num, char single_car_num);

    updateMainFrame* m_pUpdateFrame;

};
#endif // MAINWINDOW_H
