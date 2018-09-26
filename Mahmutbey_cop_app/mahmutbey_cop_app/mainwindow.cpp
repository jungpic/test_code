#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QWSServer>

#include "orig/cob_process_extern.h"

enum __cop_key_enum {
    enum_COP_KEY_NONE = 0,
    enum_COP_KEY_UP,
    enum_COP_KEY_DOWN,
    enum_COP_KEY_LEFT,
    enum_COP_KEY_RIGHT,
    enum_COP_KEY_MENU,	/* 5 */
    enum_COP_KEY_SELECT,
    enum_COP_KEY_OCC, /* 7 */
    enum_COP_KEY_PA_IN,
    enum_COP_KEY_PA_OUT,
    enum_COP_KEY_CALL_CAB,
    enum_COP_KEY_CALL_PEI,
    enum_COP_KEY_FIRE,
    enum_COP_KEY_PA_INOUT,
    enum_COP_KEY_LONG_KEY,
    enum_COP_KEY_MAX	/* 14 */
};

enum line_color_enum {
	enum_LINE_COLOR_DEFAULT = 0,
	enum_LINE_COLOR_GREEN,
	enum_LINE_COLOR_YELLOW,
};

enum line_color_enum line_button_color[2];

extern COP_Cur_Status_t __broadcast_status;

extern int Log_Status;

extern int mic_vol_level;
extern int spk_vol_level;
extern int inspk_vol_level;
extern int outspk_vol_level;

extern int done_avc_tcp_connected;
extern int change_to_call_menu;
extern int change_to_top_menu;
extern int change_to_config_menu;

extern int special_msg_lrm_on;
extern int special_msg_pib_on;

extern int occ_pa_enable_level;
extern int occ_pa_running;
extern int occ_pa_en_status;

extern int passive_in_button;
extern int passive_out_button;

extern enum __cop_key_enum key_val;

extern char AVC_Master_IPName[16];
extern char AVC_ver[10];
extern char cop_version_txt[10];
extern unsigned char my_ip_value[4];

extern int ui_update_di_and_route;
extern unsigned char avc_di_display_id;
extern unsigned char avc_departure_id;
extern unsigned char avc_destination_id;
extern int avc_special_route_pattern;

#define LINE_COUNT_MAX			2
#define STATION_COUNT_MAX		255
#define STATION_NAME_LEN_MAX	255
extern unsigned int line_count;
extern char line_name[LINE_COUNT_MAX][STATION_NAME_LEN_MAX];

struct station_data {
	unsigned int	code;
	char			name[STATION_NAME_LEN_MAX];
};
extern unsigned int station_count[LINE_COUNT_MAX];
extern struct station_data station_info[LINE_COUNT_MAX][STATION_COUNT_MAX];

#define DI_MANUAL_DISPLAY_MAX	8
extern unsigned int di_manual_display_count;
extern struct station_data di_manual_display_info[DI_MANUAL_DISPLAY_MAX];

#define FUNC_BROADCAST_COUNT_MAX	255
#define FUNC_BROADCAST_NAME_LEN_MAX	255
#define FUNC_BROADCAST_1PAGE_ITEMS	9
struct func_broadcast_data {
	unsigned int    code;
	char    		name[FUNC_BROADCAST_NAME_LEN_MAX];
	unsigned char	color;
};
extern unsigned int func_broadcast_count;
extern struct func_broadcast_data func_broadcast_info[FUNC_BROADCAST_COUNT_MAX];

struct func_broadcast_ui_data {
	unsigned int total_page;
	unsigned int current_page;
	int selected_page;
	int selected_idx;
};
struct func_broadcast_ui_data ui_func_broadcast_status;

extern unsigned int station_fpa_count;
extern struct func_broadcast_data station_fpa_info[FUNC_BROADCAST_COUNT_MAX];

unsigned int a_station_fpa_count;
struct func_broadcast_data a_station_fpa_info[FUNC_BROADCAST_COUNT_MAX];

struct func_broadcast_ui_data ui_station_fpa_status;

#define SPECIAL_ROUTE_COUNT_MAX		20
#define SPECIAL_ROUTE_NAME_LEN_MAX	255
#define SPECIAL_ROUTE_1PAGE_ITEMS	4
struct func_broadcast_ui_data ui_special_route_status;
extern int special_route_count;
extern struct station_data special_route_info[SPECIAL_ROUTE_COUNT_MAX];

#define MAX_STATION_NAME        2048

/* Color code definitions */
#define COLOR_SKY				"background-color: #03A1E8; color: black;	\
					   selection-background-color: #03A1E8;"
#define COLOR_PINK				"background-color: #FF0066; color: white;	\
					   selection-background-color: #FF0066;"
#define COLOR_NAVY				"background-color: #0C1441; color: white;	\
					   selection-background-color: #0C1441;"
#define COLOR_LIGHT_NAVY		"background-color: #B4B9D3; color: black;	\
					   selection-background-color: #B4B9D3;"
#define COLOR_BLUE              "background-color: blue; color: white;	\
                       selection-background-color: blue;"
#define COLOR_YELLOW			"background-color: #BF9000; color: black;	\
					   selection-background-color: #BF9000;"
#define COLOR_GREEN				"background-color: #00C600; color: black;	\
					   selection-background-color: #00C600;"
#define COLOR_RED				"background-color: #DA262F; color: white;	\
					   selection-background-color: #DA262F;"
//								border-color:red; gridline-color:white; selection-color:blue; selection-background-color:yellow;"
#define COLOR_RED_B				"background-color: #DA262F; color: black;	\
					   selection-background-color: #DA262F;"
#define COLOR_FUNC_RED			"background-color: red; color: white;	\
					   selection-background-color: #DA262F;"
#define COLOR_WHITE				"background-color: white; color: black;		\
					   selection-background-color: white;"
#define COLOR_GRAY				"background-color: white; color: gray;		\
					   selection-background-color: white;"
#define COLOR_BLACK				"background-color: black; color: orange;	\
					   selection-background-color: black;"

#define COLOR_PROGRESS_NOT_READY	"QProgressBar{text-align:center;} \
										QProgressBar::chunk{background-color:gray;}"
#define COLOR_FUNC_NUM_LIGHT_NAVY 	"QPushButton{background-color:#B4B9D3; color:black; \
										selection-background-color:#B4B9D3} \
										QPushButton:pressed {background-color:#0C1441; color:white;}"
#define COLOR_OK_BUTTON				"QPushButton{background-color:#0C1441; color:white; \
										selection-background-color:#0C1441} \
										QPushButton:pressed {background-color:#00C600; color:white;}"

#define COLOR_LINE_NAME_DEFAULT		COLOR_FUNC_NUM
#define COLOR_LINE_NAME_GREEN		"QPushButton{background-color:#008D3D; color:black; \
										selection-background-color:#008D3D} \
										QPushButton:pressed {background-color:#DA262F; color:white;}"
#define COLOR_LINE_NAME_YELLOW		"QPushButton{background-color:#D3B077; color:black; \
										selection-background-color:#D2B077} \
										QPushButton:pressed {background-color:#DA262F; color:white;}"

#define COLOR_LINE_NAME_DEACTIVE	"QPushButton{background-color:#B4B9D3; color:white; \
										selection-background-color:#B4B9D3} \
										QPushButton:pressed {background-color:#DA262F; color:white;}"
#define COLOR_CANCLE 				"QPushButton{background-color:red; color:white; \
										selection-background-color:#DA262F} \
										QPushButton:pressed {background-color:red; color:black;}"

#define COLOR_NOT_READY			COLOR_GRAY
#define COLOR_TITLE				COLOR_NAVY
#define COLOR_VOL_SUBTITLE		COLOR_NAVY
#define COLOR_VOL_LABEL			COLOR_LIGHT_NAVY
#define COLOR_VOL_LABEL_ACTIVE	COLOR_RED
#define COLOR_STATUS_BAR_READY	COLOR_NAVY
#define COLOR_STATUS_BAR_NOT_READY COLOR_GRAY
#define COLOR_MENU_BUTTON_ACTIVE COLOR_RED
#define COLOR_MENU_BUTTON		COLOR_NAVY

#define COLOR_PA_BUTTON			COLOR_NAVY
#define COLOR_PA_BUTTON_ACTIVE	COLOR_GREEN
#define COLOR_CALL_BUTTON		COLOR_NAVY
#define COLOR_CALL_BUTTON_ACTIVE COLOR_RED
//#define COLOR_EMER_PEI_NUM		COLOR_LIGHT_NAVY
#define COLOR_EMER_PEI_NUM		COLOR_RED
//#define COLOR_CALL_MON_NUM		COLOR_LIGHT_NAVY
#define COLOR_CALL_MON_NUM		COLOR_NAVY
#define COLOR_CALL_NUM			COLOR_LIGHT_NAVY
#define COLOR_CALL_NUM_ACTIVE 	COLOR_RED
#define COLOR_MSG_BUTTON		COLOR_LIGHT_NAVY
#define COLOR_MSG_BUTTON_ACTIVE COLOR_NAVY

#define COLOR_FUNC_DISP			COLOR_LIGHT_NAVY
#define COLOR_FUNC_STATION		COLOR_OK_BUTTON
#define COLOR_FUNC_ACTIVE		COLOR_GREEN
#define COLOR_FUNC_CANCLE		COLOR_LIGHT_NAVY
#define COLOR_FUNC_NUM			COLOR_FUNC_NUM_LIGHT_NAVY
//#define COLOR_FUNC_NUM_CANCLE 	COLOR_LIGHT_NAVY
#define COLOR_FUNC_BUTTON		COLOR_WHITE
#define COLOR_FUNC_BUTTON_ACTIVE COLOR_NAVY
#define COLOR_DI_BUTTON			COLOR_FUNC_NUM_LIGHT_NAVY
#define COLOR_DI_LABEL			COLOR_LIGHT_NAVY
#define COLOR_DI_LABEL_ACTIVE	COLOR_BLACK
#define COLOR_DI_OK				COLOR_WHITE
#define COLOR_DI_OK_ACTIVE		COLOR_OK_BUTTON
#define COLOR_ROUTE_BUTTON		COLOR_FUNC_NUM_LIGHT_NAVY
//#define COLOR_ROUTE_LABEL		COLOR_LIGHT_NAVY
#define COLOR_ROUTE_LABEL		COLOR_SKY
#define COLOR_ROUTE_LABEL_SELECT	COLOR_GREEN
#define COLOR_ROUTE_LABEL_ACTIVE	COLOR_RED_B
#define COLOR_ROUTE_OK			COLOR_WHITE
#define COLOR_ROUTE_OK_ACTIVE	COLOR_OK_BUTTON
#define COLOR_STATION_BUTTON	COLOR_FUNC_NUM_LIGHT_NAVY

extern char Departure_Name_Turkish[MAX_STATION_NAME];
extern char Departure_Name_English[MAX_STATION_NAME];
extern char Current_Name_Turkish[MAX_STATION_NAME];
extern char Current_Name_English[MAX_STATION_NAME];
extern char Next_1_Name_Turkish[MAX_STATION_NAME];
extern char Next_1_Name_English[MAX_STATION_NAME];
extern char Next_2_Name_Turkish[MAX_STATION_NAME];
extern char Next_2_Name_English[MAX_STATION_NAME];
extern char Destination_Name_Turkish[MAX_STATION_NAME];
extern char Destination_Name_English[MAX_STATION_NAME];
struct CAB_Call_Status {
     int stat;
     char car;
     char id;
	 char train_num;
};
extern struct CAB_Call_Status  cab_call_status[2];

struct PEI_Call_Status {
     int stat;
     int emergency;
     char car;
     char id;
	 char train_num;
};
extern struct PEI_Call_Status  pei_call_status[2];
//struct PEI_Call_Status  old_pei_call_status[2];
extern int pei_join_button_on;
extern int pei_call_count;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#if 1   /* Should enable on thee real target project */
    /* Remove the Window Title Bar & fixed windows size */
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);
    this->setWindowFlags(Qt::CustomizeWindowHint);
    this->setWindowFlags(Qt::FramelessWindowHint);
#endif
    /* Doesn't show the debug buttons */
    ui->LANG_TITLE->setVisible(false);
    ui->LANG_COMBO->setVisible(false);
    ui->TOUCH_CAL->setVisible(false);

    //ui->PEI_JOIN->setVisible(false);

    ui->LANG_COMBO->setItemDelegate(new RowHeightDelegate());

	pMenuBtnGroup = new QButtonGroup(this);
	pMenuBtnGroup->addButton(ui->BUTTON_TOP, TOP_MENU);
	pMenuBtnGroup->addButton(ui->BUTTON_CONFIG, CONFIG_MENU);
	pMenuBtnGroup->addButton(ui->BUTTON_CALL, CALL_MENU);
	pMenuBtnGroup->addButton(ui->BUTTON_ROUTE, ROUTE_S_MENU);
	pMenuBtnGroup->addButton(ui->BUTTON_DI, DI_MENU);
	connect(pMenuBtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(menu_button_clicked(int)));

	pTopBtnGroup = new QButtonGroup(this);
	pTopBtnGroup->addButton(ui->FUNC_1, TOP_BTN_ID_1);
    pTopBtnGroup->addButton(ui->FUNC_2, TOP_BTN_ID_2);
	pTopBtnGroup->addButton(ui->FUNC_3, TOP_BTN_ID_3);
	pTopBtnGroup->addButton(ui->FUNC_4, TOP_BTN_ID_4);
	pTopBtnGroup->addButton(ui->FUNC_5, TOP_BTN_ID_5);
	pTopBtnGroup->addButton(ui->FUNC_6, TOP_BTN_ID_6);
	pTopBtnGroup->addButton(ui->FUNC_7, TOP_BTN_ID_7);
	pTopBtnGroup->addButton(ui->FUNC_8, TOP_BTN_ID_8);
	pTopBtnGroup->addButton(ui->FUNC_9, TOP_BTN_ID_9);
	pTopBtnGroup->addButton(ui->FUNC_IN, TOP_BTN_ID_IN);
	pTopBtnGroup->addButton(ui->FUNC_OUT, TOP_BTN_ID_OUT);
	pTopBtnGroup->addButton(ui->FUNC_LRM, TOP_BTN_ID_LRM);
	pTopBtnGroup->addButton(ui->FUNC_PIB, TOP_BTN_ID_PIB);
	pTopBtnGroup->addButton(ui->FUNC_UP, TOP_BTN_ID_UP);
	pTopBtnGroup->addButton(ui->FUNC_DOWN, TOP_BTN_ID_DOWN);
	pTopBtnGroup->addButton(ui->FUNC_STATION, TOP_BTN_ID_STATION);
	connect(pTopBtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(top_button_clicked(int)));

	pDiBtnGroup = new QButtonGroup(this);
	pDiBtnGroup->addButton(ui->DI_BTN_1, DI_BTN_ID_1);
	pDiBtnGroup->addButton(ui->DI_BTN_2, DI_BTN_ID_2);
	pDiBtnGroup->addButton(ui->DI_BTN_3, DI_BTN_ID_3);
	pDiBtnGroup->addButton(ui->DI_BTN_4, DI_BTN_ID_4);
	pDiBtnGroup->addButton(ui->DI_BTN_5, DI_BTN_ID_5);
	pDiBtnGroup->addButton(ui->DI_BTN_6, DI_BTN_ID_6);
	pDiBtnGroup->addButton(ui->DI_BTN_7, DI_BTN_ID_7);
	pDiBtnGroup->addButton(ui->DI_BTN_8, DI_BTN_ID_8);
	pDiBtnGroup->addButton(ui->DI_BTN_9, DI_BTN_ID_9);
	pDiBtnGroup->addButton(ui->DI_BTN_10, DI_BTN_ID_10);
	pDiBtnGroup->addButton(ui->DI_OK, DI_BTN_ID_OK);
	pDiBtnGroup->addButton(ui->DI_STOP, DI_BTN_ID_STOP);
	connect(pDiBtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(di_button_clicked(int)));

	pRouteBtnGroup = new QButtonGroup(this);
	pRouteBtnGroup->addButton(ui->ROUTE_01, ROUTE_BTN_ID_01);
	pRouteBtnGroup->addButton(ui->ROUTE_02, ROUTE_BTN_ID_02);
	pRouteBtnGroup->addButton(ui->ROUTE_03, ROUTE_BTN_ID_03);
	pRouteBtnGroup->addButton(ui->ROUTE_04, ROUTE_BTN_ID_04);
	pRouteBtnGroup->addButton(ui->ROUTE_05, ROUTE_BTN_ID_05);
	pRouteBtnGroup->addButton(ui->ROUTE_06, ROUTE_BTN_ID_06);
	pRouteBtnGroup->addButton(ui->ROUTE_07, ROUTE_BTN_ID_07);
	pRouteBtnGroup->addButton(ui->ROUTE_08, ROUTE_BTN_ID_08);
	pRouteBtnGroup->addButton(ui->ROUTE_09, ROUTE_BTN_ID_09);
	pRouteBtnGroup->addButton(ui->ROUTE_10, ROUTE_BTN_ID_10);
	pRouteBtnGroup->addButton(ui->ROUTE_11, ROUTE_BTN_ID_11);
	pRouteBtnGroup->addButton(ui->ROUTE_12, ROUTE_BTN_ID_12);
	pRouteBtnGroup->addButton(ui->ROUTE_13, ROUTE_BTN_ID_13);
	pRouteBtnGroup->addButton(ui->ROUTE_14, ROUTE_BTN_ID_14);
	pRouteBtnGroup->addButton(ui->ROUTE_15, ROUTE_BTN_ID_15);
	pRouteBtnGroup->addButton(ui->ROUTE_16, ROUTE_BTN_ID_16);
	pRouteBtnGroup->addButton(ui->ROUTE_DEPARTURE, ROUTE_BTN_ID_DEPARTURE);
	pRouteBtnGroup->addButton(ui->ROUTE_DESTINATION, ROUTE_BTN_ID_DESTINATION);
	pRouteBtnGroup->addButton(ui->ROUTE_OK, ROUTE_BTN_ID_OK);
	connect(pRouteBtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(route_button_clicked(int)));

	pRouteSBtnGroup = new QButtonGroup(this);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_1, ROUTE_S_BTN_ID_1);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_2, ROUTE_S_BTN_ID_2);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_3, ROUTE_S_BTN_ID_3);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_4, ROUTE_S_BTN_ID_4);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_UP, ROUTE_S_BTN_ID_UP);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_DOWN, ROUTE_S_BTN_ID_DOWN);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_SELECT_1, ROUTE_S_BTN_ID_LINE_SELECT_1);
	pRouteSBtnGroup->addButton(ui->ROUTE_S_SELECT_2, ROUTE_S_BTN_ID_LINE_SELECT_2);
	connect(pRouteSBtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(special_route_button_clicked(int)));

	pStationBtnGroup = new QButtonGroup(this);
	pStationBtnGroup->addButton(ui->STATION_01, STATION_BTN_ID_01);
	pStationBtnGroup->addButton(ui->STATION_02, STATION_BTN_ID_02);
	pStationBtnGroup->addButton(ui->STATION_03, STATION_BTN_ID_03);
	pStationBtnGroup->addButton(ui->STATION_04, STATION_BTN_ID_04);
	pStationBtnGroup->addButton(ui->STATION_05, STATION_BTN_ID_05);
	pStationBtnGroup->addButton(ui->STATION_06, STATION_BTN_ID_06);
	pStationBtnGroup->addButton(ui->STATION_07, STATION_BTN_ID_07);
	pStationBtnGroup->addButton(ui->STATION_08, STATION_BTN_ID_08);
	pStationBtnGroup->addButton(ui->STATION_09, STATION_BTN_ID_09);
	pStationBtnGroup->addButton(ui->STATION_10, STATION_BTN_ID_10);
	pStationBtnGroup->addButton(ui->STATION_11, STATION_BTN_ID_11);
	pStationBtnGroup->addButton(ui->STATION_12, STATION_BTN_ID_12);
	pStationBtnGroup->addButton(ui->STATION_13, STATION_BTN_ID_13);
	pStationBtnGroup->addButton(ui->STATION_14, STATION_BTN_ID_14);
	pStationBtnGroup->addButton(ui->STATION_15, STATION_BTN_ID_15);
	pStationBtnGroup->addButton(ui->STATION_16, STATION_BTN_ID_16);
	pStationBtnGroup->addButton(ui->STATION_SELECT_1, STATION_BTN_ID_LINE_SELECT_1);
	pStationBtnGroup->addButton(ui->STATION_SELECT_2, STATION_BTN_ID_LINE_SELECT_2);
	pStationBtnGroup->addButton(ui->STATION_BACK, STATION_BTN_ID_BACK);
	connect(pStationBtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(station_button_clicked(int)));

	pStation_PA_BtnGroup = new QButtonGroup(this);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_1, STATION_PA_BTN_ID_1);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_2, STATION_PA_BTN_ID_2);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_3, STATION_PA_BTN_ID_3);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_4, STATION_PA_BTN_ID_4);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_5, STATION_PA_BTN_ID_5);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_6, STATION_PA_BTN_ID_6);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_7, STATION_PA_BTN_ID_7);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_8, STATION_PA_BTN_ID_8);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_9, STATION_PA_BTN_ID_9);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_UP, STATION_PA_BTN_ID_UP);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_DOWN, STATION_PA_BTN_ID_DOWN);
	pStation_PA_BtnGroup->addButton(ui->STATION_PA_BACK, STATION_PA_BTN_ID_BACK);
	connect(pStation_PA_BtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(station_PA_button_clicked(int)));

    ui->MENU->setCurrentIndex(TOP_MENU);
    ui->BUTTON_TOP->setDisabled(false);
    ui->BUTTON_CONFIG->setDisabled(false);
	ui->BUTTON_CALL->setDisabled(false);
	ui->BUTTON_ROUTE->setDisabled(false);
    ui->BUTTON_DI->setDisabled(false);

    language_selection = LANG_TURKISH;
    on_LANG_COMBO_currentIndexChanged(language_selection);

    join_request = 0;

    passive_in_status = 0;
    passive_out_status = 0;
	occ_status = 0;

	focus_menu_button_index = 0;
	last_done_avc_tcp_connected = -1;
	func_selected = 0xd;	// selecte default IN/OUT(X)/LRM/PIB
	func_activated_code = -1;
	route_selected = 0;
	route_line_selected = 0;
	station_pa_line_selected = 0;
	special_route_activated_code = -1;

	ui_di_display_id = 0;
	ui_departure_station_id = 0;
	ui_destination_stataion_id = 0;

	ui_func_broadcast_status.total_page 	= 0;
	ui_func_broadcast_status.current_page 	= 0;
	ui_func_broadcast_status.selected_idx 	= -1;
	ui_func_broadcast_status.selected_page 	= -1;

    ui->TITLE_LOGO_ICON->setPixmap(QPixmap(":/Logo_101_81_gray.jpg"));
    ui->TITLE_LOGO_ICON->show();
	/*
    //ui->TITLE_BACKGROUND_ICON->setPixmap(QPixmap(":/Title_Background_701_81.jpg"));
    ui->TITLE_BACKGROUND_ICON->setStyleSheet(COLOR_TITLE);
    ui->TITLE_ARROW_ICON->setPixmap(QPixmap(":/Arrow_81_41.jpg"));
    ui->TOP_PIB_ICON->setPixmap(QPixmap(":/PIB_Logo_201_311.jpg"));

    ui->TITLE_BACKGROUND_ICON->show();
    ui->TITLE_ARROW_ICON->show();
    ui->TOP_PIB_ICON->show();
	*/

    QPalette *palette = new QPalette();

    palette->setColor(QPalette::WindowText, Qt::blue);
    ui->LOG_WINDOW->setStyleSheet(COLOR_STATUS_BAR_NOT_READY);
    ui->LOG_WINDOW->setPalette(*palette);
    if(language_selection == LANG_TURKISH) {
        char temp[25] = "HAZIR DEĞİL";
        ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
    } else {
        ui->LOG_WINDOW->setText("NOT READY");
    }

    palette->setColor(QPalette::ButtonText, Qt::white);
	activate_button(done_avc_tcp_connected);

    TcpConnectedTimer = new QTimer(this);
    connect(TcpConnectedTimer, SIGNAL(timeout()), this, SLOT(ConnectUpdate()));
    TcpConnectedTimer->start(TCP_CONNECT_UPDATE_TIME);

    ScreenUpdateTimer = new QTimer(this);
    connect(ScreenUpdateTimer, SIGNAL(timeout()), this, SLOT(ScreenUpdate()));

    ScreenUpdateTimer->start(SCREEN_UPDATE_TIME);

    PEI_CallUpdateTimer = new QTimer(this);
    connect(PEI_CallUpdateTimer, SIGNAL(timeout()), this, SLOT(PEI_CallUpdate()));
    PEI_CallUpdateTimer->start(PEI_CALL_UPDATE_TIME);

    CAB_CallUpdateTimer = new QTimer(this);
    connect(CAB_CallUpdateTimer, SIGNAL(timeout()), this, SLOT(CAB_CallUpdate()));
    CAB_CallUpdateTimer->start(CAB_CALL_UPDATE_TIME);

    PEI_BTN_1_BlinkTimer_Status = 0;
    PEI_BTN_2_BlinkTimer_Status = 0;
    pei_1 = 0;
    pei_2 = 0;

    PEI_BTN_1_BlinkTimer = new QTimer(this);
    connect(PEI_BTN_1_BlinkTimer, SIGNAL(timeout()), this, SLOT(PEI_BTN_1_BLINK_Update()));

    PEI_BTN_2_BlinkTimer = new QTimer(this);
    connect(PEI_BTN_2_BlinkTimer, SIGNAL(timeout()), this, SLOT(PEI_BTN_2_BLINK_Update()));

    CAB_BTN_1_BlinkTimer_Status = 0;
    CAB_BTN_2_BlinkTimer_Status = 0;
    cab_1 = 0;
    cab_2 = 0;

    CAB_BTN_1_BlinkTimer = new QTimer(this);
    connect(CAB_BTN_1_BlinkTimer, SIGNAL(timeout()), this, SLOT(CAB_BTN_1_BLINK_Update()));

    CAB_BTN_2_BlinkTimer = new QTimer(this);
    connect(CAB_BTN_2_BlinkTimer, SIGNAL(timeout()), this, SLOT(CAB_BTN_2_BLINK_Update()));

    pThread = new COP_main(this);

    //qDebug() << "Stack size 1: " << pThread->stackSize();
    pThread->setStackSize(16*1024*1024);
    pThread->start();
    qDebug() << "Stack size 2: " << pThread->stackSize();

    ReadCalibrationData(CALIBRATION_FILE);

    delete palette;
}

MainWindow::~MainWindow()
{
    delete ui;
    force_reboot();
}

void MainWindow::ReadCalibrationData(QString filename)
{
    QFile *file = new QFile;
    file->setFileName(filename);
    if(!file->open(QIODevice::ReadOnly)) {
        on_TOUCH_CAL_clicked();
		delete file;
        return;
    }
    file->close();
	delete file;
}

void MainWindow::activate_button(int enable)
{
    //QPalette *palette = new QPalette();

	if(enable)	//active
	{
		for(int i=TOP_BTN_ID_1; i<= TOP_BTN_ID_9; i++)
		{
			pTopBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
		}
		ui->FUNC_STATION->setStyleSheet(COLOR_FUNC_STATION);

		ui->FUNC_IN->setStyleSheet(COLOR_FUNC_BUTTON);
		ui->FUNC_OUT->setStyleSheet(COLOR_FUNC_BUTTON);
		ui->FUNC_LRM->setStyleSheet(COLOR_FUNC_BUTTON);
		ui->FUNC_PIB->setStyleSheet(COLOR_FUNC_BUTTON);

		ui->FUNC_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->FUNC_DOWN->setStyleSheet(COLOR_FUNC_NUM);

		ui->OCC_BUTTON->setVisible(false);
		ui->OCC_BUTTON->setStyleSheet(COLOR_CALL_BUTTON);
		ui->CAB_BUTTON->setStyleSheet(COLOR_CALL_BUTTON);
		ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_NUM);
		ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_NUM);
		ui->PEI_BUTTON->setStyleSheet(COLOR_CALL_BUTTON);
		ui->PEI_JOIN->setStyleSheet(COLOR_CALL_BUTTON);
		ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_NUM);
		ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_NUM);

		ui->CAB_VOL_TITLE->setStyleSheet(COLOR_VOL_SUBTITLE);
		ui->PASSENGER_VOL_TITLE->setStyleSheet(COLOR_VOL_SUBTITLE);
		ui->LANG_TITLE->setStyleSheet(COLOR_VOL_SUBTITLE);

		ui->CAB_MIC_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);
		ui->CAB_SPK_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);
		ui->PASSENGER_IN_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);
		ui->PASSENGER_OUT_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);

		ui->CAB_MIC_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->CAB_MIC_DOWN->setStyleSheet(COLOR_FUNC_NUM);
		ui->CAB_SPK_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->CAB_SPK_DOWN->setStyleSheet(COLOR_FUNC_NUM);
		ui->PAMP_IN_VOL_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->PAMP_IN_VOL_DOWN->setStyleSheet(COLOR_FUNC_NUM);
		ui->PAMP_OUT_VOL_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->PAMP_OUT_VOL_DOWN->setStyleSheet(COLOR_FUNC_NUM);

		ui->CAB_MIC_VOL->setTextVisible(true);
		ui->CAB_MIC_VOL->setStyleSheet(COLOR_FUNC_NUM);
		ui->CAB_SPK_VOL->setTextVisible(true);
		ui->CAB_SPK_VOL->setStyleSheet(COLOR_FUNC_NUM);
		ui->PAMP_IN_SPK_VOL->setTextVisible(true);
		ui->PAMP_IN_SPK_VOL->setStyleSheet(COLOR_FUNC_NUM);
		ui->PAMP_OUT_SPK_VOL->setTextVisible(true);
		ui->PAMP_OUT_SPK_VOL->setStyleSheet(COLOR_FUNC_NUM);

		ui->CALL_VOL_TITLE->setStyleSheet(COLOR_VOL_SUBTITLE);
		ui->CALL_MIC_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);
		ui->CALL_SPK_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);
		ui->CALL_MIC_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->CALL_MIC_DOWN->setStyleSheet(COLOR_FUNC_NUM);
		ui->CALL_SPK_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->CALL_SPK_DOWN->setStyleSheet(COLOR_FUNC_NUM);
		ui->CALL_MIC_VOL->setTextVisible(true);
		ui->CALL_MIC_VOL->setStyleSheet(COLOR_FUNC_NUM);
		ui->CALL_SPK_VOL->setTextVisible(true);
		ui->CALL_SPK_VOL->setStyleSheet(COLOR_FUNC_NUM);

		for(int i=DI_BTN_ID_1; i<DI_BTN_ID_OK; i++)
		{
			pDiBtnGroup->button(i)->setStyleSheet(COLOR_DI_BUTTON);
			pDiBtnGroup->button(i)->setText(QString::fromUtf8(di_manual_display_info[i-1].name));
		}
		pDiBtnGroup->button(DI_BTN_ID_OK)->setStyleSheet(COLOR_DI_OK);
		pDiBtnGroup->button(DI_BTN_ID_STOP)->setStyleSheet(COLOR_OK_BUTTON);
		ui->DI_LABEL->setStyleSheet(COLOR_DI_LABEL);

		for(int i=ROUTE_BTN_ID_01; i<= ROUTE_BTN_ID_16; i++)
		{
			pRouteBtnGroup->button(i)->setStyleSheet(COLOR_ROUTE_BUTTON);
			pRouteBtnGroup->button(i)->setText(QString::fromUtf8(station_info[route_line_selected][i-1].name));
		}

		ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL);
		ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL);
		ui->ROUTE_ARROW1->setStyleSheet(COLOR_WHITE);

		pRouteBtnGroup->button(ROUTE_BTN_ID_OK)->setStyleSheet(COLOR_ROUTE_OK);

		pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_LINE_NAME_DEFAULT);
		pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_1)->setText("-");
		pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_1)->setDisabled(true);

		if(line_count > 0)
		{
			QString name = QString::fromUtf8(line_name[0]);
			if(name.contains("%g"))
			{
				name.replace("%g","");
				pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_LINE_NAME_GREEN);
				line_button_color[0] = enum_LINE_COLOR_GREEN;
			}
			else if(name.contains("%y"))
			{
				name.replace("%y","");
				pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_LINE_NAME_YELLOW);
				line_button_color[0] = enum_LINE_COLOR_YELLOW;
			}
			pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_1)->setText(name);
			pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_1)->setDisabled(false);
		}

		pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_LINE_NAME_DEFAULT);
		pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_2)->setText("-");
		pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_2)->setDisabled(true);

		if(line_count > 1)
		{
			QString name = QString::fromUtf8(line_name[1]);
			if(name.contains("%g"))
			{
				name.replace("%g","");
				pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_LINE_NAME_GREEN);
				line_button_color[1] = enum_LINE_COLOR_GREEN;
			}
			else if(name.contains("%y"))
			{
				name.replace("%y","");
				pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_LINE_NAME_YELLOW);
				line_button_color[1] = enum_LINE_COLOR_YELLOW;
			}

			pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_2)->setText(name);
			pRouteSBtnGroup->button(ROUTE_S_BTN_ID_LINE_SELECT_2)->setDisabled(false);
		}

		for(int i=ROUTE_S_BTN_ID_1; i<ROUTE_S_BTN_ID_4; i++)
		{
			pRouteSBtnGroup->button(i)->setStyleSheet(COLOR_ROUTE_BUTTON);
			//pRouteSBtnGroup->button(i)->setText(QString::fromUtf8(station_info[route_line_selected][i-1].name));
		}

		ui->ROUTE_S_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->ROUTE_S_DOWN->setStyleSheet(COLOR_FUNC_NUM);

		pStationBtnGroup->button(STATION_BTN_ID_BACK)->setStyleSheet(COLOR_OK_BUTTON);

		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_OK_BUTTON);
		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setText("-");
		//pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setDisabled(true);
		if(line_count > 0)
		{
			QString name = QString::fromUtf8(line_name[0]);
			if(name.contains("%g"))
			{
				name.replace("%g","");
			}
			else if(name.contains("%y"))
			{
				name.replace("%y","");
			}

			pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setText(name);
			pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setDisabled(false);
		}

		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_OK_BUTTON);
		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setText("-");
		//pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setDisabled(true);
		if(line_count > 1)
		{
			QString name = QString::fromUtf8(line_name[1]);
			if(name.contains("%g"))
			{
				name.replace("%g","");
			}
			else if(name.contains("%y"))
			{
				name.replace("%y","");
			}

			pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setText(name);
			pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setDisabled(false);
		}

		refresh_func_broadcast_name();
		refresh_station_fpa_name();
		refresh_special_route_name();

		for(int i=STATION_PA_BTN_ID_1 ; i<= STATION_PA_BTN_ID_9; i++)
		{
			pStation_PA_BtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
		}

		ui->STATION_PA_UP->setStyleSheet(COLOR_FUNC_NUM);
		ui->STATION_PA_DOWN->setStyleSheet(COLOR_FUNC_NUM);
		ui->STATION_PA_BACK->setStyleSheet(COLOR_FUNC_STATION);

		ui->BUTTON_CANCEL->setStyleSheet(COLOR_CANCLE);
	}
	else	// deactive
	{
		for(int i=TOP_BTN_ID_1; i< TOP_BTN_ID_MAX; i++)
		{
			pTopBtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
		}

		ui->OCC_BUTTON->setStyleSheet(COLOR_NOT_READY);
		ui->CAB_BUTTON->setStyleSheet(COLOR_NOT_READY);
		ui->CAB_CALL_1->setStyleSheet(COLOR_NOT_READY);
		ui->CAB_CALL_2->setStyleSheet(COLOR_NOT_READY);
		ui->PEI_BUTTON->setStyleSheet(COLOR_NOT_READY);
		ui->PEI_JOIN->setStyleSheet(COLOR_NOT_READY);
		ui->PEI_CALL_1->setStyleSheet(COLOR_NOT_READY);
		ui->PEI_CALL_2->setStyleSheet(COLOR_NOT_READY);

		ui->CAB_VOL_TITLE->setStyleSheet(COLOR_NOT_READY);
		ui->PASSENGER_VOL_TITLE->setStyleSheet(COLOR_NOT_READY);
		ui->LANG_TITLE->setStyleSheet(COLOR_NOT_READY);

		ui->CAB_MIC_VOL_LABEL->setStyleSheet(COLOR_NOT_READY);
		ui->CAB_SPK_VOL_LABEL->setStyleSheet(COLOR_NOT_READY);
		ui->PASSENGER_IN_VOL_LABEL->setStyleSheet(COLOR_NOT_READY);
		ui->PASSENGER_OUT_VOL_LABEL->setStyleSheet(COLOR_NOT_READY);

		ui->CAB_MIC_UP->setStyleSheet(COLOR_NOT_READY);
		ui->CAB_MIC_DOWN->setStyleSheet(COLOR_NOT_READY);
		ui->CAB_SPK_UP->setStyleSheet(COLOR_NOT_READY);
		ui->CAB_SPK_DOWN->setStyleSheet(COLOR_NOT_READY);
		ui->PAMP_IN_VOL_UP->setStyleSheet(COLOR_NOT_READY);
		ui->PAMP_IN_VOL_DOWN->setStyleSheet(COLOR_NOT_READY);
		ui->PAMP_OUT_VOL_UP->setStyleSheet(COLOR_NOT_READY);
		ui->PAMP_OUT_VOL_DOWN->setStyleSheet(COLOR_NOT_READY);

		ui->CAB_MIC_VOL->setTextVisible(false);
		ui->CAB_MIC_VOL->setStyleSheet(COLOR_PROGRESS_NOT_READY);
		ui->CAB_SPK_VOL->setTextVisible(false);
		ui->CAB_SPK_VOL->setStyleSheet(COLOR_PROGRESS_NOT_READY);
		ui->PAMP_IN_SPK_VOL->setTextVisible(false);
		ui->PAMP_IN_SPK_VOL->setStyleSheet(COLOR_PROGRESS_NOT_READY);
		ui->PAMP_OUT_SPK_VOL->setTextVisible(false);
		ui->PAMP_OUT_SPK_VOL->setStyleSheet(COLOR_PROGRESS_NOT_READY);

		ui->CALL_VOL_TITLE->setStyleSheet(COLOR_NOT_READY);
		ui->CALL_MIC_VOL_LABEL->setStyleSheet(COLOR_NOT_READY);
		ui->CALL_SPK_VOL_LABEL->setStyleSheet(COLOR_NOT_READY);
		ui->CALL_MIC_UP->setStyleSheet(COLOR_NOT_READY);
		ui->CALL_MIC_DOWN->setStyleSheet(COLOR_NOT_READY);
		ui->CALL_SPK_UP->setStyleSheet(COLOR_NOT_READY);
		ui->CALL_SPK_DOWN->setStyleSheet(COLOR_NOT_READY);
		ui->CALL_MIC_VOL->setTextVisible(false);
		ui->CALL_MIC_VOL->setStyleSheet(COLOR_PROGRESS_NOT_READY);
		ui->CALL_SPK_VOL->setTextVisible(false);
		ui->CALL_SPK_VOL->setStyleSheet(COLOR_PROGRESS_NOT_READY);

		for(int i=DI_BTN_ID_1; i<DI_BTN_ID_MAX; i++)
		{
			pDiBtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
		}
		ui->DI_LABEL->setStyleSheet(COLOR_NOT_READY);

		for(int i=ROUTE_BTN_ID_01; i<ROUTE_BTN_ID_MAX; i++)
		{
			pRouteBtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
		}

		ui->ROUTE_ARROW1->setStyleSheet(COLOR_NOT_READY);

		for(int i=ROUTE_S_BTN_ID_1; i<ROUTE_S_BTN_ID_MAX; i++)
		{
			pRouteSBtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
			//pRouteSBtnGroup->button(i)->setText(QString::fromUtf8(station_info[route_line_selected][i-1].name));
		}
		//ui->ROUTE_S_LABEL_PAGE

		for(int i=STATION_BTN_ID_01; i<STATION_BTN_ID_MAX; i++)
		{
			pStationBtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
		}

		for(int i=STATION_PA_BTN_ID_1; i<STATION_PA_BTN_ID_MAX; i++)
		{
			pStation_PA_BtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
		}

		ui->BUTTON_CANCEL->setStyleSheet(COLOR_NOT_READY);
	}

	refresh_menu_buttons_focus();
	refresh_func_buttons_focus();

	//delete palette;
}

#if 0
void MainWindow::activate_button_for_route(int enable)
{
	if(enable)	//active
	{
		for(int i=ROUTE_BTN_ID_01; i<= ROUTE_BTN_ID_16; i++)
		{
			pRouteBtnGroup->button(i)->setStyleSheet(COLOR_ROUTE_BUTTON);
			if(station_info[route_line_selected][i-1].code)
			{
				pRouteBtnGroup->button(i)->setText(QString::fromUtf8(station_info[route_line_selected][i-1].name));
				pRouteBtnGroup->button(i)->setDisabled(false);
			}
			else
				pRouteBtnGroup->button(i)->setText("");
		}

		ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL);
		ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL);

		ui->ROUTE_ARROW1->setStyleSheet(COLOR_WHITE);

		pRouteBtnGroup->button(ROUTE_BTN_ID_OK)->setStyleSheet(COLOR_ROUTE_OK);

		for(int i=ROUTE_BTN_ID_16+1; i<=ROUTE_BTN_ID_OK; i++)
			pRouteBtnGroup->button(i)->setDisabled(false);

		//pRouteBtnGroup->button(ROUTE_BTN_ID_LINE_SELECT_1)->setVisible(false);
		//pRouteBtnGroup->button(ROUTE_BTN_ID_LINE_SELECT_2)->setVisible(false);
	}
	else	// deactive
	{
		//pRouteBtnGroup->button(ROUTE_BTN_ID_LINE_SELECT_1)->setVisible(true);
		//pRouteBtnGroup->button(ROUTE_BTN_ID_LINE_SELECT_2)->setVisible(true);

		for(int i=ROUTE_BTN_ID_01; i<=ROUTE_BTN_ID_OK; i++)
		{
			pRouteBtnGroup->button(i)->setDisabled(true);
			pRouteBtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
		}
		ui->ROUTE_ARROW1->setStyleSheet(COLOR_NOT_READY);
	}
}
#endif

void MainWindow::activate_button_for_station_PA(int enable)
{
	if(enable)	//active
	{
		for(int i=STATION_BTN_ID_01; i<=STATION_BTN_ID_16; i++)
		{
			if(station_info[station_pa_line_selected][i-1].code)
			{
				pStationBtnGroup->button(i)->setDisabled(false);
				pStationBtnGroup->button(i)->setText(QString::fromUtf8(station_info[station_pa_line_selected][i-1].name));
			}
			else
				pStationBtnGroup->button(i)->setText("");

			pStationBtnGroup->button(i)->setStyleSheet(COLOR_STATION_BUTTON);
		}
		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setVisible(true/*false*/);
		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setVisible(true/*false*/);

		if(station_pa_line_selected == 0)
		{
			if(line_button_color[0] == enum_LINE_COLOR_GREEN)
				pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_LINE_NAME_GREEN);
			else if (line_button_color[0] == enum_LINE_COLOR_YELLOW)
				pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_LINE_NAME_YELLOW);
			else
				pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_LINE_NAME_DEFAULT);

			pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_LINE_NAME_DEACTIVE);
		}
		else if (station_pa_line_selected == 1)
		{
			pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setStyleSheet(COLOR_LINE_NAME_DEACTIVE);

			if(line_button_color[1] == enum_LINE_COLOR_GREEN)
				pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_LINE_NAME_GREEN);
			else if (line_button_color[1] == enum_LINE_COLOR_YELLOW)
				pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_LINE_NAME_YELLOW);
			else
				pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setStyleSheet(COLOR_LINE_NAME_DEFAULT);
		}
	}
	else	// deactive
	{
		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_1)->setVisible(true);
		pStationBtnGroup->button(STATION_BTN_ID_LINE_SELECT_2)->setVisible(true);

		for(int i=STATION_BTN_ID_01; i<=STATION_BTN_ID_16; i++)
		{
			pStationBtnGroup->button(i)->setDisabled(true);
			pStationBtnGroup->button(i)->setStyleSheet(COLOR_NOT_READY);
		}
	}
}

void MainWindow::PEI_BTN_1_BLINK_Update(void)
{
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::WindowText, Qt::white);

    if(pei_call_status[0].emergency) {
        if(pei_1)
            ui->PEI_CALL_1->setStyleSheet(COLOR_EMER_PEI_NUM);
        else
            ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_NUM);
    } else {
        if(pei_1)
            ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
        else
            ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_NUM);
    }
    ui->PEI_CALL_1->setPalette(*palette);
    pei_1 ^= 1;

	delete palette;
}

void MainWindow::PEI_BTN_2_BLINK_Update(void)
{
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::WindowText, Qt::white);

    if(pei_call_status[1].emergency) {
        if(pei_2)
            ui->PEI_CALL_2->setStyleSheet(COLOR_EMER_PEI_NUM);
        else
            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_NUM);
    } else {
        if(pei_2)
            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
        else
            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_NUM);
    }
    ui->PEI_CALL_2->setPalette(*palette);
    pei_2 ^= 1;

	delete palette;
}

void MainWindow::CAB_BTN_1_BLINK_Update(void)
{
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::WindowText, Qt::white);

    if(cab_1) {
        if(cab_call_status[0].stat == DISP_STAT_ACTIVE_SELECT_MON_WAIT)
            ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_MON_NUM);
        else
            ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
    } else
        ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_NUM);

    ui->CAB_CALL_1->setPalette(*palette);
    cab_1 ^= 1;

	delete palette;
}

void MainWindow::CAB_BTN_2_BLINK_Update(void)
{
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::WindowText, Qt::white);

    if(cab_2) {
        if(cab_call_status[1].stat == DISP_STAT_ACTIVE_SELECT_MON_WAIT)
            ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_MON_NUM);
        else
            ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
    } else
        ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_NUM);

    ui->CAB_CALL_2->setPalette(*palette);
    cab_2 ^= 1;

	delete palette;
}

unsigned short MainWindow::calc_car_num_from_train_num(char train_num, char single_car_num)
{
       unsigned short ret_car_num=0;

       switch(single_car_num)
       {
               // MC2
               case 1:
               case 5:
                       ret_car_num = 1500 + train_num * 2;
                       break;

               // M
               case 2:
               case 6:
                       ret_car_num = 1300 + train_num;
                       break;

               // T
               case 3:
               case 7:
                       ret_car_num = 1100 + train_num;
                       break;

               // MC1
               case 4:
               case 8:
                       ret_car_num = 1500 + train_num * 2 - 1;
                       break;

               default:
       				   printf(" Error Train: %d Car: %d --> %d\n", train_num, single_car_num, ret_car_num);
                       ret_car_num = 0;
                       break;
       }

       //printf("Train: %d Car: %d --> %d\n", train_num, single_car_num, ret_car_num);
       return ret_car_num;
}

void MainWindow::CAB_CallUpdate(void)
{
	if(done_avc_tcp_connected==0)
		return;

    QPalette *palette = new QPalette();
    palette->setColor(QPalette::WindowText, Qt::white);

    char label_1[8];
    char label_2[8];

    if(change_to_call_menu && done_avc_tcp_connected) {
        ui->MENU->setCurrentIndex(CALL_MENU);
        change_to_call_menu = 0;
    }
	else if(ui->MENU->currentIndex() != CALL_MENU)
	{
		return;
	}
    //printf("cab_call_status[0].stat = %d\n", cab_call_status[0].stat);

	if(cab_call_status[0].stat >= DISP_STAT_ACTIVE_SELECT &&
			cab_call_status[0].stat <= DISP_STAT_BLINK)
	{
		unsigned short car_num=0;
		car_num = calc_car_num_from_train_num(cab_call_status[0].train_num, cab_call_status[0].car);
		//sprintf(label_1, "KABİN %d - %d", car_num, cab_call_status[0].id);
		sprintf(label_1, "KABİN - %d", car_num);
		ui->CAB_CALL_1->setText(QString::fromUtf8(label_1));
#if 0
		if(language_selection == LANG_TURKISH) {
			sprintf(label_1, "KABİN %d-%d", cab_call_status[0].car, cab_call_status[0].id);
			ui->CAB_CALL_1->setText(QString::fromUtf8(label_1));
		} else {
			sprintf(label_1, "CAB %d-%d", cab_call_status[0].car, cab_call_status[0].id);
			ui->CAB_CALL_1->setText(QString::fromAscii(label_1));
		}
#endif
	}

    switch(cab_call_status[0].stat) {
        case DISP_STAT_ACTIVE_SELECT:       // active, select
            if(CAB_BTN_1_BlinkTimer_Status) {
                CAB_BTN_1_BlinkTimer->stop();
                CAB_BTN_1_BlinkTimer_Status = 0;
            }
            ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->CAB_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_NO_SELECT:
            if(CAB_BTN_1_BlinkTimer_Status) {
                CAB_BTN_1_BlinkTimer->stop();
                CAB_BTN_1_BlinkTimer_Status = 0;
            }
            ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->CAB_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_SELECT_MON:       /* PEI : Monitoring */
            if(CAB_BTN_1_BlinkTimer_Status) {
                CAB_BTN_1_BlinkTimer->stop();
                CAB_BTN_1_BlinkTimer_Status = 0;
            }
            ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_MON_NUM);
            ui->CAB_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_BLINK:
            if(!CAB_BTN_1_BlinkTimer_Status) {
                CAB_BTN_1_BlinkTimer->start(BTN_BLINK_TIME);
                CAB_BTN_1_BlinkTimer_Status = 1;
            }
            break;
        case DISP_STAT_ACTIVE_SELECT_MON_WAIT:
            if(!CAB_BTN_1_BlinkTimer_Status) {
                CAB_BTN_1_BlinkTimer->start(BTN_BLINK_TIME);
                CAB_BTN_1_BlinkTimer_Status = 1;
            }
            break;
        case DISP_STAT_NO_ACTIVE_NO_SELECT:     // no select
        default:
            if(CAB_BTN_1_BlinkTimer_Status) {
                CAB_BTN_1_BlinkTimer->stop();
                CAB_BTN_1_BlinkTimer_Status = 0;
            }
            if(language_selection == LANG_TURKISH) {
                char temp[25] = "KABİN";
                ui->CAB_CALL_1->setText(QString::fromUtf8(temp));
            } else {
                ui->CAB_CALL_1->setText("CAB");
            }
            ui->CAB_CALL_1->setStyleSheet(COLOR_CALL_NUM);
            ui->CAB_CALL_1->setPalette(*palette);
            break;
    }

    //printf("cab_call_status[1].stat = %d\n", cab_call_status[1].stat);
	if(cab_call_status[1].stat >= DISP_STAT_ACTIVE_SELECT &&
			cab_call_status[1].stat <= DISP_STAT_BLINK)
	{
		unsigned short car_num = 0;
		car_num = calc_car_num_from_train_num(cab_call_status[1].train_num, cab_call_status[1].car);
		//sprintf(label_2, "KABİN %d - %d", car_num, cab_call_status[1].id);
		sprintf(label_2, "KABİN - %d", car_num);
		ui->CAB_CALL_2->setText(QString::fromUtf8(label_2));
#if 0
		if(language_selection == LANG_TURKISH) {
			sprintf(label_2, "KABİN %d-%d", cab_call_status[1].car, cab_call_status[1].id);
			ui->CAB_CALL_2->setText(QString::fromUtf8(label_2));
		} else {
			sprintf(label_2, "CAB %d-%d", cab_call_status[1].car, cab_call_status[1].id);
			ui->CAB_CALL_2->setText(QString::fromAscii(label_2));
		}
#endif
	}
    switch(cab_call_status[1].stat) {
        case DISP_STAT_ACTIVE_SELECT:       // active, select
            if(CAB_BTN_2_BlinkTimer_Status) {
                CAB_BTN_2_BlinkTimer->stop();
                CAB_BTN_2_BlinkTimer_Status = 0;
            }
            ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->CAB_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_NO_SELECT:
            if(CAB_BTN_2_BlinkTimer_Status) {
                CAB_BTN_2_BlinkTimer->stop();
                CAB_BTN_2_BlinkTimer_Status = 0;
            }
            ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->CAB_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_SELECT_MON:       /* PEI : Monitoring */
            if(CAB_BTN_2_BlinkTimer_Status) {
                CAB_BTN_2_BlinkTimer->stop();
                CAB_BTN_2_BlinkTimer_Status = 0;
            }
            ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_MON_NUM);
            ui->CAB_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_BLINK:
            if(!CAB_BTN_2_BlinkTimer_Status) {
                CAB_BTN_2_BlinkTimer->start(BTN_BLINK_TIME);
                CAB_BTN_2_BlinkTimer_Status = 1;
            }
            break;
        case DISP_STAT_ACTIVE_SELECT_MON_WAIT:
            if(!CAB_BTN_2_BlinkTimer_Status) {
                CAB_BTN_2_BlinkTimer->start(BTN_BLINK_TIME);
                CAB_BTN_2_BlinkTimer_Status = 1;
            }
            break;
        case DISP_STAT_NO_ACTIVE_NO_SELECT:     // no select
        default:
            if(CAB_BTN_2_BlinkTimer_Status) {
                CAB_BTN_2_BlinkTimer->stop();
                CAB_BTN_2_BlinkTimer_Status = 0;
            }

            if(language_selection == LANG_TURKISH) {
                char temp[25] = "KABİN";
                ui->CAB_CALL_2->setText(QString::fromUtf8(temp));
            } else {
                ui->CAB_CALL_2->setText("CAB");
            }
            ui->CAB_CALL_2->setStyleSheet(COLOR_CALL_NUM);
            ui->CAB_CALL_2->setPalette(*palette);
            break;
    }

	delete palette;
}

void MainWindow::PEI_CallUpdate(void)
{

	if(done_avc_tcp_connected==0)
		return;

    QPalette *palette = new QPalette();
    palette->setColor(QPalette::WindowText, Qt::white);
    char label_1[8];
    char label_2[8];

    //printf("pei_call_status[0].stat = %d\n", pei_call_status[0].stat);

	if(pei_call_status[0].stat >= DISP_STAT_ACTIVE_SELECT &&
			pei_call_status[0].stat <= DISP_STAT_BLINK)
	{
		unsigned short car_num = 0;
		car_num = calc_car_num_from_train_num(pei_call_status[0].train_num, pei_call_status[0].car);
		sprintf(label_1, "PEI %d - %d", car_num, pei_call_status[0].id);
		ui->PEI_CALL_1->setText(QString::fromAscii(label_1));
#if 0
		sprintf(label_1, "PEI %d-%d", pei_call_status[0].car, pei_call_status[0].id);
		ui->PEI_CALL_1->setText(QString::fromAscii(label_1));
#endif
	}

    switch(pei_call_status[0].stat) {
        case DISP_STAT_ACTIVE_SELECT:       // active, select
            if(PEI_BTN_1_BlinkTimer_Status) {
                PEI_BTN_1_BlinkTimer->stop();
                PEI_BTN_1_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->PEI_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_NO_SELECT:
            if(PEI_BTN_1_BlinkTimer_Status) {
                PEI_BTN_1_BlinkTimer->stop();
                PEI_BTN_1_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->PEI_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_SELECT_MON:       /* PEI : Monitoring */
            if(PEI_BTN_1_BlinkTimer_Status) {
                PEI_BTN_1_BlinkTimer->stop();
                PEI_BTN_1_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_1->setStyleSheet(COLOR_BLUE); //COLOR_CALL_MON_NUM);
            ui->PEI_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_SELECT_MON_EMER:
            if(PEI_BTN_1_BlinkTimer_Status) {
                PEI_BTN_1_BlinkTimer->stop();
                PEI_BTN_1_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_MON_NUM);
            ui->PEI_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_NO_SELECT_EMER:
            if(PEI_BTN_1_BlinkTimer_Status) {
                PEI_BTN_1_BlinkTimer->stop();
                PEI_BTN_1_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_1->setStyleSheet(COLOR_EMER_PEI_NUM);
            ui->PEI_CALL_1->setPalette(*palette);
            break;
        case DISP_STAT_BLINK:
            if(!PEI_BTN_1_BlinkTimer_Status) {
                PEI_BTN_1_BlinkTimer->start(BTN_BLINK_TIME);
                PEI_BTN_1_BlinkTimer_Status = 1;
            }
            break;
        case DISP_STAT_ACTIVE_SELECT_MON_WAIT:
        case DISP_STAT_NO_ACTIVE_NO_SELECT:     // no select
        case DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT:
        default:
            if(PEI_BTN_1_BlinkTimer_Status) {
                PEI_BTN_1_BlinkTimer->stop();
                PEI_BTN_1_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_1->setText("PEI");
            ui->PEI_CALL_1->setStyleSheet(COLOR_CALL_NUM);
            ui->PEI_CALL_1->setPalette(*palette);
            break;
    }

    //printf("pei_call_status[1].stat = %d\n", pei_call_status[1].stat);
	if(pei_call_status[1].stat >= DISP_STAT_ACTIVE_SELECT &&
			pei_call_status[1].stat <= DISP_STAT_BLINK)
	{
		unsigned short car_num = 0;
		car_num = calc_car_num_from_train_num(pei_call_status[1].train_num, pei_call_status[1].car);
		sprintf(label_2, "PEI %d - %d", car_num, pei_call_status[1].id);
		ui->PEI_CALL_2->setText(QString::fromAscii(label_2));
#if 0
		sprintf(label_2, "PEI %d-%d", pei_call_status[1].car, pei_call_status[1].id);
		ui->PEI_CALL_2->setText(QString::fromAscii(label_2));
#endif
	}

    switch(pei_call_status[1].stat) {
        case DISP_STAT_ACTIVE_SELECT:       // active, select
            if(PEI_BTN_2_BlinkTimer_Status) {
                PEI_BTN_2_BlinkTimer->stop();
                PEI_BTN_2_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->PEI_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_NO_SELECT:
            if(PEI_BTN_2_BlinkTimer_Status) {
                PEI_BTN_2_BlinkTimer->stop();
                PEI_BTN_2_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_NUM_ACTIVE);
            ui->PEI_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_SELECT_MON:       /* PEI : Monitoring */
            if(PEI_BTN_2_BlinkTimer_Status) {
                PEI_BTN_2_BlinkTimer->stop();
                PEI_BTN_2_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_MON_NUM);
            ui->PEI_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_SELECT_MON_EMER:
            if(PEI_BTN_2_BlinkTimer_Status) {
                PEI_BTN_2_BlinkTimer->stop();
                PEI_BTN_2_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_MON_NUM);
            ui->PEI_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_ACTIVE_NO_SELECT_EMER:
            if(PEI_BTN_2_BlinkTimer_Status) {
                PEI_BTN_2_BlinkTimer->stop();
                PEI_BTN_2_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_2->setStyleSheet(COLOR_EMER_PEI_NUM);
            ui->PEI_CALL_2->setPalette(*palette);
            break;
        case DISP_STAT_BLINK:
            if(!PEI_BTN_2_BlinkTimer_Status) {
                PEI_BTN_2_BlinkTimer->start(BTN_BLINK_TIME);
                PEI_BTN_2_BlinkTimer_Status = 1;
            }
            break;
        case DISP_STAT_ACTIVE_SELECT_MON_WAIT:
        case DISP_STAT_NO_ACTIVE_NO_SELECT:     // no select
        case DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT:
        default:
            if(PEI_BTN_2_BlinkTimer_Status) {
                PEI_BTN_2_BlinkTimer->stop();
                PEI_BTN_2_BlinkTimer_Status = 0;
            }
            ui->PEI_CALL_2->setText("PEI");

            ui->PEI_CALL_2->setStyleSheet(COLOR_CALL_NUM);
            ui->PEI_CALL_2->setPalette(*palette);
            break;
    }

    if(pei_join_button_on)
	{
		//ui->PEI_JOIN->setVisible(true);
		ui->PEI_JOIN->setStyleSheet(COLOR_CALL_BUTTON);
        ui->PEI_JOIN->setDisabled(false);
        join_request = 0;
    } else {
		ui->PEI_JOIN->setStyleSheet(COLOR_NOT_READY);
        //ui->PEI_JOIN->setVisible(false);
        ui->PEI_JOIN->setDisabled(true);
        join_request = 1;
	}

	delete palette;
}

void MainWindow::ConnectUpdate(void)
{
	int idx=0;
	if(done_avc_tcp_connected != last_done_avc_tcp_connected)
	{
		QPalette *palette = new QPalette();
		QString avcVerStr;
		QString copVerStr;
		QString copIpStr;

		last_done_avc_tcp_connected = done_avc_tcp_connected;

		if(done_avc_tcp_connected) {

			ui->TITLE_LOGO_ICON->setPixmap(QPixmap(":/Logo_101_81.jpg"));
			ui->TITLE_LOGO_ICON->show();
			ui->LOG_WINDOW->setStyleSheet(COLOR_STATUS_BAR_READY);
			ui->LOG_WINDOW->setPalette(*palette);

			ui->Master_AVC_IP->setText(AVC_Master_IPName);
			avcVerStr.sprintf("AVC v%s", AVC_ver);
			ui->Master_AVC_Label->setText(avcVerStr);
			copVerStr.sprintf("COP v%s", cop_version_txt);
			ui->COP_Ver_Label->setText(copVerStr);
			copIpStr.sprintf("%d.%d.%d.%d", my_ip_value[0], my_ip_value[1], my_ip_value[2], my_ip_value[3]);
			ui->COP_IP_Label->setText(copIpStr);
			ui->Master_AVC_IP->setVisible(false);
			ui->Master_AVC_Label->setVisible(false);
			ui->COP_Ver_Label->setVisible(false);
			ui->COP_IP_Label->setVisible(false);

			activate_button(done_avc_tcp_connected);

			ui->LANG_COMBO->setDisabled(false);

			ui->CAB_MIC_UP->setDisabled(false);         ui->CAB_MIC_DOWN->setDisabled(false);
			ui->CAB_SPK_UP->setDisabled(false);         ui->CAB_SPK_DOWN->setDisabled(false);
			ui->PAMP_IN_VOL_UP->setDisabled(false);     ui->PAMP_IN_VOL_DOWN->setDisabled(false);
			ui->PAMP_OUT_VOL_UP->setDisabled(false);    ui->PAMP_OUT_VOL_DOWN->setDisabled(false);
			ui->CALL_MIC_UP->setDisabled(false);		ui->CALL_MIC_DOWN->setDisabled(false);
			ui->CALL_SPK_UP->setDisabled(false);		ui->CALL_SPK_DOWN->setDisabled(false);

			ui->TOP->setDisabled(false);
			ui->FUNC_1->setDisabled(false);ui->FUNC_2->setDisabled(false);ui->FUNC_3->setDisabled(false);
			ui->FUNC_4->setDisabled(false);ui->FUNC_5->setDisabled(false);ui->FUNC_6->setDisabled(false);
			ui->FUNC_STATION->setDisabled(false);
			ui->FUNC_IN->setDisabled(false); ui->FUNC_OUT->setDisabled(false);
			ui->FUNC_LRM->setDisabled(false); ui->FUNC_PIB->setDisabled(false);

			for(int i=DI_BTN_ID_1; i<DI_BTN_ID_MAX; i++)
				pDiBtnGroup->button(i)->setDisabled(false);

			for(int i=ROUTE_BTN_ID_01; i<ROUTE_BTN_ID_MAX; i++)
				pRouteBtnGroup->button(i)->setDisabled(false);

			for(int i=STATION_BTN_ID_01; i<STATION_BTN_ID_MAX; i++)
				pStationBtnGroup->button(i)->setDisabled(false);

			palette->setColor(QPalette::WindowText, Qt::white);

			/*
			   if(language_selection == LANG_ENGLISH) {
			   ui->Title_Departure->setPalette(*palette);
			   ui->Title_Departure->setText(QString::fromUtf8(Departure_Name_English));
			   ui->Title_Destination->setPalette(*palette);
			   ui->Title_Destination->setText(QString::fromUtf8(Destination_Name_English));

			   ui->Departure_Label->setText(QString::fromUtf8(Departure_Name_English));
			   ui->Current_Station_Label->setText(QString::fromUtf8(Current_Name_English));

			   if(Next_1_Name_English[0] == 0x00)
			   ui->Next_Station_Label->setText("");
			   else
			   ui->Next_Station_Label->setText(QString::fromUtf8(Next_1_Name_English));

			   if(Next_2_Name_English[0] == 0x00)
			   ui->After_Next_Station_Label->setText("");
			   else
			   ui->After_Next_Station_Label->setText(QString::fromUtf8(Next_2_Name_English));

			   if( (QString::compare(Next_2_Name_English, Destination_Name_English, Qt::CaseInsensitive)) == 0 ||
			   (QString::compare(Next_1_Name_English, Destination_Name_English, Qt::CaseInsensitive)) == 0 ||
			   (QString::compare(Current_Name_English, Destination_Name_English, Qt::CaseInsensitive)) == 0)
			   {
			   ui->Destination_Label->setText("");
			   } else {
			   ui->Destination_Label->setText(QString::fromUtf8(Destination_Name_English));
			   }
			   } else {
			   ui->Title_Departure->setPalette(*palette);
			   ui->Title_Departure->setText(QString::fromUtf8(Departure_Name_Turkish));
			   ui->Title_Destination->setPalette(*palette);
			   ui->Title_Destination->setText(QString::fromUtf8(Destination_Name_Turkish));

			   ui->Departure_Label->setText(QString::fromUtf8(Departure_Name_Turkish));
			   ui->Current_Station_Label->setText(QString::fromUtf8(Current_Name_Turkish));

			   if(Next_1_Name_Turkish[0] == 0x00)
			   ui->Next_Station_Label->setText("");
			   else
			   ui->Next_Station_Label->setText(QString::fromUtf8(Next_1_Name_Turkish));

			   if(Next_2_Name_Turkish[0] == 0x00)
			   ui->After_Next_Station_Label->setText("");
			   else
			   ui->After_Next_Station_Label->setText(QString::fromUtf8(Next_2_Name_Turkish));

			   if( (QString::compare(Next_2_Name_Turkish, Destination_Name_Turkish, Qt::CaseInsensitive)) == 0 ||
			   (QString::compare(Next_1_Name_Turkish, Destination_Name_Turkish, Qt::CaseInsensitive)) == 0 ||
			   (QString::compare(Current_Name_Turkish, Destination_Name_Turkish, Qt::CaseInsensitive)) == 0)
			   {
			   ui->Destination_Label->setText("");
			   } else {
			   ui->Destination_Label->setText(QString::fromUtf8(Destination_Name_Turkish));
			   }
			   }
			 */
		} 
		else
		{
			ui->TITLE_LOGO_ICON->setPixmap(QPixmap(":/Logo_101_81_gray.jpg"));
			ui->TITLE_LOGO_ICON->show();
			activate_button(done_avc_tcp_connected);
			ui->Master_AVC_IP->setText("");
			ui->Master_AVC_Label->setText("");
			ui->COP_Ver_Label->setText("");
			ui->COP_IP_Label->setText("");
			ui->LOG_WINDOW->setStyleSheet(COLOR_STATUS_BAR_NOT_READY);
			ui->LOG_WINDOW->setPalette(*palette);

			ui->LANG_COMBO->setDisabled(true);

			ui->CAB_MIC_UP->setDisabled(true);         ui->CAB_MIC_DOWN->setDisabled(true);
			ui->CAB_SPK_UP->setDisabled(true);         ui->CAB_SPK_DOWN->setDisabled(true);
			ui->PAMP_IN_VOL_UP->setDisabled(true);     ui->PAMP_IN_VOL_DOWN->setDisabled(true);
			ui->PAMP_OUT_VOL_UP->setDisabled(true);    ui->PAMP_OUT_VOL_DOWN->setDisabled(true);
			ui->CALL_MIC_UP->setDisabled(true);        ui->CALL_MIC_DOWN->setDisabled(true);
			ui->CALL_SPK_UP->setDisabled(true);        ui->CALL_SPK_DOWN->setDisabled(true);

			ui->TOP->setDisabled(true);
			ui->FUNC_1->setDisabled(true);ui->FUNC_2->setDisabled(true);ui->FUNC_3->setDisabled(true);
			ui->FUNC_4->setDisabled(true);ui->FUNC_5->setDisabled(true);ui->FUNC_6->setDisabled(true);
			ui->FUNC_STATION->setDisabled(true);

			ui->FUNC_IN->setDisabled(true); ui->FUNC_OUT->setDisabled(true);
			ui->FUNC_LRM->setDisabled(true); ui->FUNC_PIB->setDisabled(true);

			for(int i=DI_BTN_ID_1; i<DI_BTN_ID_MAX; i++)
				pDiBtnGroup->button(i)->setDisabled(true);

			for(int i=ROUTE_BTN_ID_01; i<ROUTE_BTN_ID_MAX; i++)
				pRouteBtnGroup->button(i)->setDisabled(true);

			for(int i=STATION_BTN_ID_01; i<STATION_BTN_ID_MAX; i++)
				pStationBtnGroup->button(i)->setDisabled(true);

			ui->CAB_BUTTON->setDisabled(true);
			ui->OCC_BUTTON->setDisabled(true);
			ui->PEI_BUTTON->setDisabled(true);
			ui->PEI_JOIN->setDisabled(true);

			ui_func_broadcast_status.total_page 	= 0;
			ui_func_broadcast_status.current_page 	= 0;
			ui_func_broadcast_status.selected_idx 	= -1;
			ui_func_broadcast_status.selected_page 	= -1;
		}

		delete palette;
	}
}

void MainWindow::ScreenUpdate(void)
{
	static int busy_delay_start = 0;
	unsigned int tmp;

    QPalette *palette = new QPalette();
    QPalette *palette_black = new QPalette();
    //QPalette *palette_white = new QPalette();
    QPalette *palette_red = new QPalette();
    QPalette *palette_blue = new QPalette();

    palette_black->setColor(QPalette::WindowText, Qt::black);
    //palette_white->setColor(QPalette::WindowText, Qt::white);
    palette_red->setColor(QPalette::WindowText, Qt::red);
    palette_blue->setColor(QPalette::WindowText, Qt::blue);

    if(change_to_call_menu && done_avc_tcp_connected) {
        ui->MENU->setCurrentIndex(CALL_MENU);
        change_to_call_menu = 0;
    }

	if( func_activated_code != get_func_is_now_start())
	{
		printf("func_activated_code = %d, get_func_is_now_start()=%d\n", func_activated_code, get_func_is_now_start());
		func_activated_code = get_func_is_now_start();
		refresh_func_broadcast_name();
		refresh_station_fpa_name();
	}

	if(ui_update_di_and_route > 0)
	{
		printf(" QT ScreenUpdate ui_update_di_and_route\n");
		refresh_di_and_route();
		special_route_activated_code = avc_special_route_pattern;
		refresh_special_route_name();
		ui_update_di_and_route = 0;
	}
	else if (ui_update_di_and_route == -1)
	{
		Log_Status = LOG_ROUTE_NG;
		ui_update_di_and_route = 0;
	}

#if 1	// OCC Enable
	if(occ_status != occ_pa_running)
	{
		occ_status = occ_pa_running;
		if(occ_status)
		{
#else	// OCC Disable - Only OCC Indicator Display for USCOM Test
	tmp = occ_pa_en_get_level();
	if(occ_status != tmp)
	{
		occ_status = tmp; 
		if(occ_status)
		{
#endif
			ui->OCC_BUTTON->setVisible(true);
			ui->MENU->setCurrentIndex(CALL_MENU);
			ui->OCC_BUTTON->setStyleSheet(COLOR_CALL_BUTTON_ACTIVE);
			ui->OCC_BUTTON->setPalette(*palette);
		}
		else
		{
			ui->OCC_BUTTON->setVisible(false);
			ui->OCC_BUTTON->setStyleSheet(COLOR_CALL_BUTTON);
			ui->OCC_BUTTON->setPalette(*palette);
		}
	}

	if(focus_menu_button_index != ui->MENU->currentIndex())
		refresh_menu_buttons_focus();

	palette->setColor(QPalette::ButtonText, Qt::white);

	if (done_avc_tcp_connected)
	{
		if(change_to_config_menu) {
			ui->MENU->setCurrentIndex(CONFIG_MENU);
			change_to_config_menu = 0;
		}

		if(passive_in_button) {
			ui->PASSENGER_IN_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL_ACTIVE);
			ui->PASSENGER_IN_VOL_LABEL->setPalette(*palette);
			passive_in_status = 1;
		} else {
			ui->PASSENGER_IN_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);
			ui->PASSENGER_IN_VOL_LABEL->setPalette(*palette);
			passive_in_status = 0;
		}

		if(passive_out_button) {
			ui->PASSENGER_OUT_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL_ACTIVE);
			ui->PASSENGER_OUT_VOL_LABEL->setPalette(*palette);
			passive_out_status = 1;
		} else {
			ui->PASSENGER_OUT_VOL_LABEL->setStyleSheet(COLOR_VOL_LABEL);
			ui->PASSENGER_OUT_VOL_LABEL->setPalette(*palette);
			passive_out_status = 0;
		}
	}

	if ( ( Log_Status >= BUSY_LOG_START_IDX) && (Log_Status <= BUSY_LOG_END_IDX ) )
	{
		if (busy_delay_start == 0)
		{
			busy_delay_start  = 1;
			log_busy_display_time.start();
		}
		else if (busy_delay_start == 1)
		{
			if( log_busy_display_time.elapsed() > LOG_MESSAGE_DISPLAY_TIME )
			{
				busy_delay_start = 0;
				Log_Status = LOG_READY;
			}
		}

	}

    switch(Log_Status) {
        case LOG_AVC_NOT_CONNECT:       /* Same as LOG_NOT_READY */
            ui->LOG_WINDOW->setPalette(*palette_blue);
            if(language_selection == LANG_TURKISH) {
                char temp[25] = "HAZIR DEĞİL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("NOT READY");
            break;
        case LOG_READY:
            ui->LOG_WINDOW->setPalette(*palette_black);
            if(language_selection == LANG_TURKISH) {
                char temp[25] = "HAZIR";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("READY");
            break;
        case LOG_IP_SETUP_ERR:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[25] = "IP YÜKLEME HATASI";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("IP SETUP ERROR");
            break;
        case LOG_UNKNOWN_ERR:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[25] = "BİLİNMEYEN HATASI";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("UNKNOWN ERROR");
            break;
        case LOG_CAB_ID_ERR:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "KABİN ID MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("CAB ID ERROR");
            break;
        case LOG_CAB_TO_CAB_WAIT:
            ui->LOG_WINDOW->setPalette(*palette_blue);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "KABİNden KABİNe BEKLEME";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("CAB TO CAB WAIT");
            break;
        case LOG_CAB_REJECT_WAIT:
            ui->LOG_WINDOW->setPalette(*palette_blue);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "KABİN RED BEKLEMESİ";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("CAB REJECT");
            break;
        case LOG_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("BUSY");
            break;
        case LOG_OCC_PA_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "OCC PA MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("OCC PA BUSY");
            break;
        case LOG_CAB_PA_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "KABİN PA MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("CAB PA BUSY");
            break;
        case LOG_CAB_TO_CAB_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "KABİNden KABİNe MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("CAB TO CAB BUSY");
            break;
        case LOG_PEI_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "YOLCU İLETİŞİM MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("PEI BUSY");
            break;
        case LOG_PEI_TO_CAB_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "PEIden KABINe MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("PEI TO CAB BUSY");
            break;
        case LOG_EMER_PEI_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "ACİL YOLCU İLETİŞİM MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("EMRGENCY PEI BUSY");
            break;
        case LOG_PEI_TO_OCC_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "PEIden OCCye MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("PEI TO OCC BUSY");
            break;
        case LOG_FUNC_BUSY:
            ui->LOG_WINDOW->setPalette(*palette_red);
            ui->LOG_WINDOW->setText("FUNC BUSY");
            break;
		case LOG_FUNC_REJECT:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "FUNC ID MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("FUNC ID ERROR");
            break;
		case LOG_ROUTE_NG:
            ui->LOG_WINDOW->setPalette(*palette_red);
            if(language_selection == LANG_TURKISH) {
                char temp[50] = "GÜZERGAH ID MEŞGUL";
                ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
            } else
                ui->LOG_WINDOW->setText("ROUTE ID ERROR");
            break;
        default:
            if(!done_avc_tcp_connected) {
                ui->LOG_WINDOW->setPalette(*palette_blue);
                if(language_selection == LANG_TURKISH) {
                    char temp[25] = "HAZIR DEĞİL";
                    ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
                } else
                    ui->LOG_WINDOW->setText("NOT READY");
            } else {
                ui->LOG_WINDOW->setPalette(*palette_red);
                if(language_selection == LANG_TURKISH) {
                    char temp[25] = "BİLİNMEYEN HATASI";
                    ui->LOG_WINDOW->setText(QString::fromUtf8(temp));
                } else
                    ui->LOG_WINDOW->setText("UNKNOWN ERROR("+QString::number(Log_Status)+")");
            }
            break;
    }

    /* CAB MIC Volume */
    ui->CAB_MIC_VOL->setValue(mic_vol_level);
	ui->CALL_MIC_VOL->setValue(mic_vol_level);

    /* CAB Speaker Volume */
    ui->CAB_SPK_VOL->setValue(spk_vol_level);
	ui->CALL_SPK_VOL->setValue(spk_vol_level);

    /* Passenger Inside Speaker Volume */
    ui->PAMP_IN_SPK_VOL->setValue(inspk_vol_level);

    /* Passenger Outside Speaker Volume */
    ui->PAMP_OUT_SPK_VOL->setValue(outspk_vol_level);

	delete palette;
    delete palette_black;
    //delete palette_white;
    delete palette_red;
    delete palette_blue;
}

void MainWindow::refresh_menu_buttons_focus()
{
	focus_menu_button_index = ui->MENU->currentIndex();

	if(done_avc_tcp_connected)
	{
		ui->BUTTON_TOP->setStyleSheet(COLOR_MENU_BUTTON);
		ui->BUTTON_CONFIG->setStyleSheet(COLOR_MENU_BUTTON);
		ui->BUTTON_CALL->setStyleSheet(COLOR_MENU_BUTTON);
		ui->BUTTON_ROUTE->setStyleSheet(COLOR_MENU_BUTTON);
		ui->BUTTON_DI->setStyleSheet(COLOR_MENU_BUTTON);

		if(focus_menu_button_index >= TOP_MENU && focus_menu_button_index <= DI_MENU)
			pMenuBtnGroup->button(focus_menu_button_index)->setStyleSheet(COLOR_MENU_BUTTON_ACTIVE);
	}
	else	// not conntected to AVC
	{
		ui->BUTTON_TOP->setStyleSheet(COLOR_NOT_READY);
		ui->BUTTON_CONFIG->setStyleSheet(COLOR_NOT_READY);
		ui->BUTTON_CALL->setStyleSheet(COLOR_NOT_READY);
		ui->BUTTON_ROUTE->setStyleSheet(COLOR_NOT_READY);
		ui->BUTTON_DI->setStyleSheet(COLOR_NOT_READY);

		if(focus_menu_button_index >= TOP_MENU && focus_menu_button_index <= DI_MENU)
			pMenuBtnGroup->button(focus_menu_button_index)->setStyleSheet(COLOR_MENU_BUTTON);
	}
}

void MainWindow::refresh_func_buttons_focus()
{

	if(done_avc_tcp_connected == 0)
		return;

	unsigned char selected;

	selected = func_selected & (0x1 << FUNC_IN_OFFSET);
	ui->FUNC_IN->setStyleSheet(selected ? COLOR_FUNC_BUTTON_ACTIVE : COLOR_FUNC_BUTTON);

	selected = func_selected & (0x1 << FUNC_OUT_OFFSET);
	ui->FUNC_OUT->setStyleSheet(selected ? COLOR_FUNC_BUTTON_ACTIVE : COLOR_FUNC_BUTTON);

	selected = func_selected & (0x1 << FUNC_LRM_OFFSET);
	ui->FUNC_LRM->setStyleSheet(selected ? COLOR_FUNC_BUTTON_ACTIVE : COLOR_FUNC_BUTTON);

	selected = func_selected & (0x1 << FUNC_PIB_OFFSET);
	ui->FUNC_PIB->setStyleSheet(selected ? COLOR_FUNC_BUTTON_ACTIVE : COLOR_FUNC_BUTTON);
}

void MainWindow::refresh_func_broadcast_name(void)
{
	if(done_avc_tcp_connected == 0)
		return;

	int i=0,idx=0;
	char label[10]={0,};
	struct func_broadcast_ui_data *pdata;

	/*
	if(ui->MENU->currentIndex() != TOP_MENU)
		return;
	*/

	pdata = &ui_func_broadcast_status;
	pdata->total_page = (func_broadcast_count+FUNC_BROADCAST_1PAGE_ITEMS-1) / FUNC_BROADCAST_1PAGE_ITEMS;

	if(pdata->total_page > 0)
	{
		printf("refresh_func_broadcast_name t_count=%d, page(%d/%d)\n", func_broadcast_count, pdata->current_page+1, pdata->total_page);
		sprintf(label, "%d / %d", pdata->current_page+1, pdata->total_page);
	}
	else
	{
		printf("func_broadcast_name.. total page = 0\n");
		sprintf(label, "0 / 0");
	}
	ui->FUNC_LABEL_PAGE->setText(QString::fromUtf8(label));

	printf("\n-- FPA button list -----------------------------------\n");
	printf("  total(%d), func_activated_code = %d\n", func_broadcast_count, func_activated_code);
	printf("------------------------------------------------------\n");
	for(i=TOP_BTN_ID_1; i<=TOP_BTN_ID_9; i++)
	{
		idx = (pdata->current_page * FUNC_BROADCAST_1PAGE_ITEMS) + i-1;
		printf("%s i(%d)/ idx(%d)/ code(%d)/ color(%d)/ name(%s)\n",
					func_broadcast_info[idx].code == func_activated_code?"*":" ",
					i, idx, func_broadcast_info[idx].code, func_broadcast_info[idx].color,
					func_broadcast_info[idx].name);

		if(idx < func_broadcast_count)
		{
			if(func_broadcast_info[idx].code == func_activated_code)
			{
				pTopBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_ACTIVE);
			}
			else if(func_broadcast_info[idx].color == 1)
			{
				pTopBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_RED);
			}
			else	//normal
			{
				pTopBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
			}

			pTopBtnGroup->button(i)->setDisabled(false);
			pTopBtnGroup->button(i)->setText(QString::fromUtf8(func_broadcast_info[idx].name));
		}
		else
		{
			pTopBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
			pTopBtnGroup->button(i)->setDisabled(true);
			pTopBtnGroup->button(i)->setText("");
		}
	}
	printf("------------------------------------------------------\n");
}

void MainWindow::refresh_station_fpa_name(void)
{
	if(done_avc_tcp_connected == 0)
		return;

	int i=0,idx=0;
	char label[10]={0,};
	struct func_broadcast_ui_data *pdata;

	/*
	if(ui->MENU->currentIndex() != STATION_PA_MENU)
		return;
	*/

	pdata = &ui_station_fpa_status;
	pdata->total_page = (a_station_fpa_count+FUNC_BROADCAST_1PAGE_ITEMS-1) / FUNC_BROADCAST_1PAGE_ITEMS;

	if(pdata->total_page > 0)
	{
		printf("refresh_station_fpa_name t_count=%d, page(%d/%d)\n", a_station_fpa_count, pdata->current_page+1, pdata->total_page);
		sprintf(label, "%d / %d", pdata->current_page+1, pdata->total_page);
	}
	else
	{
		printf("station_fpa_name.. total page = 0\n");
		sprintf(label, "0 / 0");
	}
	ui->STATION_PA_LABEL_PAGE->setText(QString::fromUtf8(label));

	printf("\n-- Station FPA button list ---------------------------\n");
	printf("  total(%d), func_activated_code = %d\n", a_station_fpa_count, func_activated_code);
	printf("------------------------------------------------------\n");

	for(i=STATION_PA_BTN_ID_1; i<=STATION_PA_BTN_ID_9; i++)
	{
		idx = (pdata->current_page * FUNC_BROADCAST_1PAGE_ITEMS) + i-1;
		printf("%s i(%d)/ idx(%d)/ code(%d)/ name(%s)\n",
					a_station_fpa_info[idx].code == func_activated_code ? "*" : " ",
					i, idx, a_station_fpa_info[idx].code, a_station_fpa_info[idx].name);

		if(idx < a_station_fpa_count)
		{
			if(a_station_fpa_info[idx].code == func_activated_code)
			{
				pStation_PA_BtnGroup->button(i)->setStyleSheet(COLOR_FUNC_ACTIVE);
			}
			else	//normal
			{
				pStation_PA_BtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
			}

			pStation_PA_BtnGroup->button(i)->setDisabled(false);
			pStation_PA_BtnGroup->button(i)->setText(QString::fromUtf8(a_station_fpa_info[idx].name));
		}
		else
		{
			pStation_PA_BtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
			pStation_PA_BtnGroup->button(i)->setDisabled(true);
			pStation_PA_BtnGroup->button(i)->setText("");
		}
	}
	printf("------------------------------------------------------\n");
}

void MainWindow::refresh_special_route_name(void)
{
	if(done_avc_tcp_connected == 0)
		return;

	int i=0,idx=0;
	char label[10]={0,};
	struct func_broadcast_ui_data *pdata;

	/*
	if(ui->MENU->currentIndex() != ROUTE_S_MENU)
		return;
	*/

	pdata = &ui_special_route_status;
	pdata->total_page = (special_route_count+SPECIAL_ROUTE_1PAGE_ITEMS-1) / SPECIAL_ROUTE_1PAGE_ITEMS;

	if(pdata->total_page > 0)
	{
		printf("refresh_spcial_route_name t_count=%d, page(%d/%d)\n", special_route_count, pdata->current_page+1, pdata->total_page);
		sprintf(label, "%d / %d", pdata->current_page+1, pdata->total_page);
	}
	else
	{
		printf("refresh_spcial_route_name.. total page = 0\n");
		sprintf(label, "0 / 0");
	}
	ui->ROUTE_S_LABEL_PAGE->setText(QString::fromUtf8(label));

	printf("\n-- Special route button list --------------------------\n");
	printf("  total(%d), special_route_activated_code= %d\n", special_route_count, special_route_activated_code);
	printf("------------------------------------------------------\n");
	for(i=ROUTE_S_BTN_ID_1; i<=ROUTE_S_BTN_ID_4; i++)
	{
		idx = (pdata->current_page * SPECIAL_ROUTE_1PAGE_ITEMS) + i-1;
		printf("%s i(%d)/ idx(%d)/ code(%d)/ name(%s)\n",
					special_route_info[idx].code == special_route_activated_code?"*":" ",
					i, idx, special_route_info[idx].code, special_route_info[idx].name);

		if(idx < special_route_count)
		{
			if(special_route_info[idx].code == special_route_activated_code)
			{
				pRouteSBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_ACTIVE);
			}
			else
			{
				pRouteSBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
			}

			pRouteSBtnGroup->button(i)->setDisabled(false);
			pRouteSBtnGroup->button(i)->setText(QString::fromUtf8(special_route_info[idx].name));
		}
		else
		{
			pRouteSBtnGroup->button(i)->setStyleSheet(COLOR_FUNC_NUM);
			pRouteSBtnGroup->button(i)->setDisabled(true);
			pRouteSBtnGroup->button(i)->setText("");
		}
	}
	printf("------------------------------------------------------\n");

}

void MainWindow::pick_out_station_fpa(int code)
{

	int i=0, count=0;

	memset(a_station_fpa_info, 0, sizeof(a_station_fpa_info));
	a_station_fpa_count = 0;

	if( code < 0 )
	{
		printf("pick_out_station_fpa ERROR. code(%d)<100\n",code);
		return;
	}	

	for(i=0; i<station_fpa_count; i++)
	{
		if((station_fpa_info[i].code/100) != code) continue;

		memcpy(&a_station_fpa_info[count], &station_fpa_info[i], sizeof(struct func_broadcast_data));
		count++;
	}

	a_station_fpa_count = count;

	printf("pick_out_station_fpa count= %d  station code = %02dXX\n", 
		count, code);
}

void MainWindow::refresh_di_and_route(void)
{
	int i=0;

	if(done_avc_tcp_connected == 0)
		return;

	ui_di_display_id = avc_di_display_id;
	//ui_departure_station_id = avc_departure_id;
	//ui_destination_stataion_id = avc_destination_id;

	if(ui_di_display_id == 0)
	{
		{ char temp[25] = "ÖZEL DI"; ui->DI_LABEL->setText(QString::fromUtf8(temp)); }
		ui->DI_LABEL->setStyleSheet(COLOR_DI_LABEL);
		ui->DI_OK->setStyleSheet(COLOR_DI_OK);
		ui->DI_STOP->setStyleSheet(COLOR_DI_OK);
	}
	else
	{
		ui->DI_LABEL->setText(pDiBtnGroup->button(ui_di_display_id)->text());
		ui->DI_LABEL->setStyleSheet(COLOR_DI_LABEL_ACTIVE);
		ui->DI_OK->setStyleSheet(COLOR_DI_OK_ACTIVE);
		ui->DI_STOP->setStyleSheet(COLOR_DI_OK_ACTIVE);
	}

	if(avc_departure_id == 0)
	{
		char temp[25] = "Başlangıç";
		ui->ROUTE_DEPARTURE->setText(QString::fromUtf8(temp));
		ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL);
	}
	else
	{
		char temp[25] = "Başlangıç";
		ui->ROUTE_DEPARTURE->setText(QString::fromUtf8(temp));
		ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL);
		for(i=0; i<station_count[route_line_selected]; i++)
		{
			if(avc_departure_id == station_info[route_line_selected][i].code)
			{
				//ui_departure_station_id = i;
				ui->ROUTE_DEPARTURE->setText(QString::fromUtf8(station_info[route_line_selected][i].name));
				ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL_ACTIVE);
				break;
			}
		}

	}

	if(avc_destination_id == 0)
	{
		char temp[25] = "Bitiş";
		ui->ROUTE_DESTINATION->setText(QString::fromUtf8(temp));
		ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL);
	}
	else
	{
		char temp[25] = "Bitiş";
		ui->ROUTE_DESTINATION->setText(QString::fromUtf8(temp));
		ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL);
		for(i=0; i<station_count[route_line_selected]; i++)
		{
			if(avc_destination_id == station_info[route_line_selected][i].code)
			{
				//ui_destination_stataion_id = i;
				ui->ROUTE_DESTINATION->setText(QString::fromUtf8(station_info[route_line_selected][i].name));
				ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL_ACTIVE);
				break;
			}
		}
	}
}

void MainWindow::menu_button_clicked(int id)
{
	int next_page = id;

	if((ui->MENU->currentIndex() != DI_MENU) && (id == DI_MENU))
	{
		if(done_avc_tcp_connected)
		{
			refresh_di_and_route();	
		}
	}
	else if ( (ui->MENU->currentIndex() != TOP_MENU) && (id == TOP_MENU) )//&&
			  //(ui_func_broadcast_status.selected_page != -1) )
	{
		pick_out_station_fpa(func_activated_code/100);
		if(a_station_fpa_count > 0)
		{
			printf("<< actived_station_fpa, go to STATION PAGE >>\n");
			refresh_station_fpa_name();
			next_page = STATION_PA_MENU;
		}
		else if (ui_func_broadcast_status.selected_page != -1)
		{
			ui_func_broadcast_status.current_page = ui_func_broadcast_status.selected_page;
			refresh_func_broadcast_name();
		}
	}
	else if ( id == ROUTE_MENU )
	{
		if(done_avc_tcp_connected)
		{
			route_selected = 0;
			route_line_selected = 0;
			//ui->ROUTE_OK->setDisabled(true);
			//ui->ROUTE_OK->setStyleSheet(COLOR_ROUTE_OK);
			//refresh_di_and_route();
			//activate_button_for_route(false);
		}
	}

	ui->MENU->setCurrentIndex(next_page);
}

void MainWindow::top_button_clicked(int id)
{
	QColor txtColor = QColor(Qt::black);

	switch(id)
	{
		case TOP_BTN_ID_1:
		case TOP_BTN_ID_2:
		case TOP_BTN_ID_3:
		case TOP_BTN_ID_4:
		case TOP_BTN_ID_5:
		case TOP_BTN_ID_6:
		case TOP_BTN_ID_7:
		case TOP_BTN_ID_8:
		case TOP_BTN_ID_9:
			ui_func_broadcast_status.selected_idx = 
				(ui_func_broadcast_status.current_page * FUNC_BROADCAST_1PAGE_ITEMS) + id - 1;
			ui_func_broadcast_status.selected_page = ui_func_broadcast_status.current_page;
			refresh_func_broadcast_name();

			//if(func_activated_code != -1)
			if(func_activated_code == func_broadcast_info[ui_func_broadcast_status.selected_idx].code)
			{
				printf("\nFUNC. BUTTON - STOP Req : Function idx, code = %d, %d, selected = 0x%x\n", ui_func_broadcast_status.selected_idx, 
						func_broadcast_info[ui_func_broadcast_status.selected_idx].code, func_selected);
				__pa_function_force_stop(func_broadcast_info[ui_func_broadcast_status.selected_idx].code, func_selected, false);
			}
			else if(ui_func_broadcast_status.selected_idx != -1)
			{
				printf("\nFUNC. BUTTON - START Req : Function idx, code = %d, %d, selected = 0x%x\n", ui_func_broadcast_status.selected_idx,
						func_broadcast_info[ui_func_broadcast_status.selected_idx].code, func_selected);
				__pa_function_process(func_broadcast_info[ui_func_broadcast_status.selected_idx].code,
						func_selected, false);
			}
			break;

		case TOP_BTN_ID_IN:
			func_selected ^= (1<<FUNC_IN_OFFSET);
			func_selected |= (1<<FUNC_OUT_OFFSET);
			refresh_func_buttons_focus();
			break;

		case TOP_BTN_ID_OUT:
			func_selected ^= (1<<FUNC_OUT_OFFSET);
			func_selected |= (1<<FUNC_IN_OFFSET);
			refresh_func_buttons_focus();
			break;

		case TOP_BTN_ID_LRM:
			func_selected ^= (1<<FUNC_LRM_OFFSET);
			refresh_func_buttons_focus();
			break;

		case TOP_BTN_ID_PIB:
			func_selected ^= (1<<FUNC_PIB_OFFSET);
			refresh_func_buttons_focus();
			break;

		case TOP_BTN_ID_UP:
			//ui_func_broadcast_status.selected_idx = -1;
			if(ui_func_broadcast_status.current_page > 0)
			{
				ui_func_broadcast_status.current_page--;
				refresh_func_broadcast_name();
			}
			break;

		case TOP_BTN_ID_DOWN:
			//ui_func_broadcast_status.selected_idx = -1;
			if(ui_func_broadcast_status.current_page < ui_func_broadcast_status.total_page - 1)
			{
				ui_func_broadcast_status.current_page++;
				refresh_func_broadcast_name();
			}
			break;

		case TOP_BTN_ID_STATION:
			station_pa_line_selected = 0;
			activate_button_for_station_PA(/*false*/true);
			ui->MENU->setCurrentIndex(STATION_MENU);
			break;

		default:
			printf("ERROR : TOP_BTN_ID (%d)\n", id);
			break;
	}
}

void MainWindow::route_button_clicked(int id)
{
	if(id >= ROUTE_BTN_ID_01 && id<= ROUTE_BTN_ID_16 && station_info[route_line_selected][id-1].code != 0)
	{
		//printf("id = %d, name = %s, code = %d\n",id, station_info[id-1].name, station_info[id-1].code);		
		if(route_selected)
		{
			pRouteBtnGroup->button(route_selected)->setText(pRouteBtnGroup->button(id)->text());

			if( route_selected == ROUTE_BTN_ID_DEPARTURE )
			{
				ui_departure_station_id = id;
			}
			else if ( route_selected == ROUTE_BTN_ID_DESTINATION )
			{
				ui_destination_stataion_id = id;
			}

			route_selected = 0;
			ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL);
			ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL);

			if( ui_departure_station_id != 0 && ui_destination_stataion_id !=0 && ui_departure_station_id != ui_destination_stataion_id )
			{
				ui->ROUTE_OK->setStyleSheet(COLOR_ROUTE_OK_ACTIVE);
				ui->ROUTE_OK->setDisabled(false);
			}
			else
			{
				ui->ROUTE_OK->setStyleSheet(COLOR_ROUTE_OK);
				ui->ROUTE_OK->setDisabled(true);
			}
		}
	}
	else if(id == ROUTE_BTN_ID_DEPARTURE)
	{
		ui->ROUTE_OK->setStyleSheet(COLOR_ROUTE_OK);
		ui->ROUTE_OK->setDisabled(true);
		route_selected = ROUTE_BTN_ID_DEPARTURE;
		ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL_SELECT);
		ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL);
	}
	else if (id == ROUTE_BTN_ID_DESTINATION)
	{
		ui->ROUTE_OK->setStyleSheet(COLOR_ROUTE_OK);
		ui->ROUTE_OK->setDisabled(true);
		route_selected = ROUTE_BTN_ID_DESTINATION;
		ui->ROUTE_DEPARTURE->setStyleSheet(COLOR_ROUTE_LABEL);
		ui->ROUTE_DESTINATION->setStyleSheet(COLOR_ROUTE_LABEL_SELECT);
	}
	else if(id == ROUTE_BTN_ID_OK)
	{
		if(ui_departure_station_id !=0 && ui_destination_stataion_id != 0)
		{
			printf("ROUTE_OK ID:code send %d:%d -> %d:%d\n", 
					ui_departure_station_id, station_info[route_line_selected][ui_departure_station_id-1].code,
					ui_destination_stataion_id, station_info[route_line_selected][ui_destination_stataion_id-1].code);

			cmd_set_route(station_info[route_line_selected][ui_departure_station_id-1].code, station_info[route_line_selected][ui_destination_stataion_id-1].code);
		}
	}
}

void MainWindow::special_route_button_clicked(int id)
{
	unsigned int selected_code = 0;
	int	selected_idx = 0;

	switch(id)
	{
		case ROUTE_S_BTN_ID_1:
		case ROUTE_S_BTN_ID_2:
		case ROUTE_S_BTN_ID_3:
		case ROUTE_S_BTN_ID_4:
			ui_special_route_status.selected_idx =
				(ui_special_route_status.current_page * SPECIAL_ROUTE_1PAGE_ITEMS) + id - 1;
			ui_special_route_status.selected_page = ui_special_route_status.current_page;
			refresh_special_route_name();

			selected_idx = ui_special_route_status.selected_idx;
			selected_code = special_route_info[selected_idx].code;
			if(special_route_activated_code == selected_code)
			{
				printf("\nSpecial route BUTTON - STOP Req : idx, code = %d, %d\n", selected_idx, selected_code);
				cmd_set_special_route_stop(selected_code);
			}
			else if(ui_special_route_status.selected_idx != -1)
			{
				printf("\nSpecial route BUTTON - START Req : idx, code = %d, %d\n", selected_idx, selected_code);
				cmd_set_special_route_start(selected_code);
			}
			break;

		case ROUTE_S_BTN_ID_UP:
			if(ui_special_route_status.current_page > 0)
			{
				ui_special_route_status.current_page--;
				refresh_special_route_name();
			}
			break;

		case ROUTE_S_BTN_ID_DOWN:
			if(ui_special_route_status.current_page < ui_special_route_status.total_page - 1)
			{
				ui_special_route_status.current_page++;
				refresh_special_route_name();
			}
			break;

		case ROUTE_S_BTN_ID_LINE_SELECT_1:
			route_line_selected = 0;
			//activate_button_for_route(true);
			ui->MENU->setCurrentIndex(ROUTE_MENU);
			break;

		case ROUTE_S_BTN_ID_LINE_SELECT_2:
			route_line_selected = 1;
			//activate_button_for_route(true);
			ui->MENU->setCurrentIndex(ROUTE_MENU);
			break;

		default:
			printf("ERROR : ROUTE_S_BTN_ID (%d)\n", id);
			break;
	}
}

void MainWindow::station_button_clicked(int id)
{
	if(id >= STATION_BTN_ID_01 && id <= STATION_BTN_ID_16)
	{
		if(station_info[station_pa_line_selected][id-1].code)
		{
			pick_out_station_fpa(station_info[station_pa_line_selected][id-1].code);
			ui_station_fpa_status.current_page = 0;
			ui->MENU->setCurrentIndex(STATION_PA_MENU);
			refresh_station_fpa_name();
		}
	}
	else if (id == STATION_BTN_ID_LINE_SELECT_1)
	{
		station_pa_line_selected = 0;
		activate_button_for_station_PA(true);
	}
	else if (id == STATION_BTN_ID_LINE_SELECT_2)
	{
		station_pa_line_selected = 1;
		activate_button_for_station_PA(true);
	}
	else if (id == STATION_BTN_ID_BACK)
	{
    	ui->MENU->setCurrentIndex(TOP_MENU);
	}
}

void MainWindow::station_PA_button_clicked(int id)
{
	switch(id)
	{
		case STATION_PA_BTN_ID_1:
		case STATION_PA_BTN_ID_2:
		case STATION_PA_BTN_ID_3:
		case STATION_PA_BTN_ID_4:
		case STATION_PA_BTN_ID_5:
		case STATION_PA_BTN_ID_6:
		case STATION_PA_BTN_ID_7:
		case STATION_PA_BTN_ID_8:
		case STATION_PA_BTN_ID_9:
			ui_station_fpa_status.selected_idx = 
				(ui_station_fpa_status.current_page * FUNC_BROADCAST_1PAGE_ITEMS) + id - 1;
			ui_station_fpa_status.selected_page = ui_station_fpa_status.current_page;
			refresh_station_fpa_name();

			//if(func_activated_code != -1)
			printf("func_activated_code = %d, seletecd_code = %d\n", func_activated_code , a_station_fpa_info[ui_station_fpa_status.selected_idx].code);
			if(func_activated_code == a_station_fpa_info[ui_station_fpa_status.selected_idx].code)
			{
				printf("\nSTATION FPA BUTTON - STOP Req : Function idx, code = %d, %d, selected = 0x%x\n", ui_station_fpa_status.selected_idx,
						a_station_fpa_info[ui_station_fpa_status.selected_idx].code, func_selected);
				__pa_function_force_stop(a_station_fpa_info[ui_station_fpa_status.selected_idx].code, func_selected, true);
			}
			else if(ui_station_fpa_status.selected_idx != -1)
			{
				printf("\nSTATION FPA BUTTON - START Req : Function idx, code = %d, %d, selected = 0x%x\n", ui_station_fpa_status.selected_idx,
						a_station_fpa_info[ui_station_fpa_status.selected_idx].code, func_selected);
				__pa_function_process(a_station_fpa_info[ui_station_fpa_status.selected_idx].code,
						func_selected, true);
			}
			break;

		case STATION_PA_BTN_ID_UP:
			if(ui_station_fpa_status.current_page > 0)
			{
				ui_station_fpa_status.current_page--;
				refresh_station_fpa_name();
			}
			break;

		case STATION_PA_BTN_ID_DOWN:
			if(ui_station_fpa_status.current_page < ui_station_fpa_status.total_page - 1)
			{
				ui_station_fpa_status.current_page++;
				refresh_station_fpa_name();
			}
			break;

		case STATION_PA_BTN_ID_BACK:
			ui->MENU->setCurrentIndex(STATION_MENU);
			break;

		default:
			printf("ERROR : STATION_PA_BTN_ID(%d)\n", id);
			break;
	}
}

void MainWindow::on_BUTTON_CANCEL_clicked()
{
	send_broadcast_stop_request();
}

#if 0	// move to top_button_clicked
void MainWindow::on_FUNC_OK_clicked()
{
    QString tempString = ui->FUNC_NUMBER->toPlainText();
    int func_number = tempString.toInt();
	static int hidden_hit_counter = 0;
	static int bHidden = 0;
	
    if( (func_number > 0) && (func_number <= 9999))
	{
		if(func_ok_actived)
		{
    		printf("\nFUNC. BUTTON - STOP Req : Function Number = %d, selected = 0x%x\n", func_number, func_selected);
        	__pa_function_force_stop(func_number, func_selected);
		}
		else
		{
    		printf("\nFUNC. BUTTON - START Req : Function Number = %d, selected = 0x%x\n", func_number, func_selected);
        	__pa_function_process(func_number, func_selected);
		}
	}

	if(func_number == 9999)	/* For debugging purpose only */
	{
		hidden_hit_counter++;

		if(hidden_hit_counter > 5)
		{
			bHidden ^= 0x1;
			if(bHidden)
			{
				QWSServer::setCursorVisible(true);
				ui->LANG_TITLE->setVisible(true);
				ui->LANG_COMBO->setVisible(true);
				ui->TOUCH_CAL->setVisible(true);
			}
			else
			{
				QWSServer::setCursorVisible(false);
				ui->LANG_TITLE->setVisible(false);
				ui->LANG_COMBO->setVisible(false);
				ui->TOUCH_CAL->setVisible(false);
			}
			hidden_hit_counter = 0;
		}
	}
	else if (func_number == 4444) /* for hidden reset */
	{
		hidden_hit_counter++;

		if(hidden_hit_counter > 5)
		{
			exit(1);
		}
	}
	else
		hidden_hit_counter = 0;
}
#endif

void MainWindow::di_button_clicked(int id)
{

	if( (id >= DI_BTN_ID_1) && (id < DI_BTN_ID_OK ) && (id <= di_manual_display_count))
	{
		ui_di_display_id = id;
		ui->DI_LABEL->setStyleSheet(COLOR_DI_LABEL);
		ui->DI_LABEL->setText(pDiBtnGroup->button(id)->text());
		ui->DI_OK->setStyleSheet(COLOR_DI_OK_ACTIVE);
		ui->DI_STOP->setStyleSheet(COLOR_DI_OK_ACTIVE);
	}
	else if ((id == DI_BTN_ID_OK) && ui_di_display_id != 0)
	{
		cmd_di_display_start(ui_di_display_id);
	}
	else if ((id == DI_BTN_ID_STOP) && ui_di_display_id != 0)
	{
		cmd_di_display_stop();
	}
}

void MainWindow::on_CALL_MIC_UP_clicked() 	{ on_CAB_MIC_UP_clicked(); }
void MainWindow::on_CALL_MIC_DOWN_clicked()	{ on_CAB_MIC_DOWN_clicked(); }
void MainWindow::on_CALL_SPK_UP_clicked() 	{ on_CAB_SPK_UP_clicked(); }
void MainWindow::on_CALL_SPK_DOWN_clicked() { on_CAB_SPK_DOWN_clicked(); }

void MainWindow::on_CAB_MIC_UP_clicked()
{
    int cab_mic_vol = ui->CAB_MIC_VOL->value();

    if(cab_mic_vol == 100)
        return;

    if( (cab_mic_vol >= 0) && (cab_mic_vol < 100) )
	{
        mic_volume_step_up(10, 1);
	}
}

void MainWindow::on_CAB_MIC_DOWN_clicked()
{
    int cab_mic_vol = ui->CAB_MIC_VOL->value();

    if(cab_mic_vol == 0)
        return;

    if( (cab_mic_vol > 0) && (cab_mic_vol <= 100) )
        mic_volume_step_down(10, 1);
}

void MainWindow::on_CAB_SPK_UP_clicked()
{
    int cab_spk_vol = ui->CAB_SPK_VOL->value();

    if(cab_spk_vol == 100)
        return;

    if( (cab_spk_vol >= 0) && (cab_spk_vol < 100) )
        spk_volume_step_up(10, 1);
}

void MainWindow::on_CAB_SPK_DOWN_clicked()
{
    int cab_spk_vol = ui->CAB_SPK_VOL->value();

    if(cab_spk_vol == 0)
        return;

    if( (cab_spk_vol > 0) && (cab_spk_vol <= 100) )
        spk_volume_step_down(10, 1);
}

void MainWindow::on_PAMP_IN_VOL_UP_clicked()
{
    int pamp_in_vol = ui->PAMP_IN_SPK_VOL->value();

    if(pamp_in_vol == 100)
        return;

    if( (pamp_in_vol >= 0) && (pamp_in_vol < 100) )
        inspk_volume_step_up(10, 1);

    update_pamp_volume_level();
}

void MainWindow::on_PAMP_IN_VOL_DOWN_clicked()
{
    int pamp_in_vol = ui->PAMP_IN_SPK_VOL->value();

    if(pamp_in_vol == 0)
        return;

    if( (pamp_in_vol > 0) && (pamp_in_vol <= 100) )
        inspk_volume_step_down(10, 1);

    update_pamp_volume_level();
}

void MainWindow::on_PAMP_OUT_VOL_UP_clicked()
{
    int pamp_out_vol = ui->PAMP_OUT_SPK_VOL->value();

    if(pamp_out_vol == 100)
        return;

    if( (pamp_out_vol >= 0) && (pamp_out_vol < 100) )
        outspk_volume_step_up(10, 1);

    update_pamp_volume_level();
}

void MainWindow::on_PAMP_OUT_VOL_DOWN_clicked()
{
    int pamp_out_vol = ui->PAMP_OUT_SPK_VOL->value();

    if(pamp_out_vol == 0)
        return;

    if( (pamp_out_vol > 0) && (pamp_out_vol <= 100) )
        outspk_volume_step_down(10, 1);

    update_pamp_volume_level();
}

#if 0
void MainWindow::on_PASSIVE_IN_clicked()
{
    if(passive_in_status) {
        passive_in_status = 0;
    } else {
        passive_in_status = 1;
    }
    __pa_in_key_process (passive_in_status);
}

void MainWindow::on_PASSIVE_OUT_clicked()
{
    if(passive_out_status) {
        passive_out_status = 0;
    } else {
        passive_out_status = 1;
    }
    __pa_out_key_process (passive_out_status);
}
#endif

void MainWindow::on_PEI_JOIN_clicked()
{
    if((!join_request) && (pei_call_count == 1)) {
        join_request = 1;
        process_join_start();
    } else {
        printf("##############################################Join Button -- Not Actions\n");
    }
}

void MainWindow::on_LANG_COMBO_currentIndexChanged(int index)
{
    if(index) {         // English
        language_selection = LANG_ENGLISH;

		ui->BUTTON_TOP->setText("TOP MENU");
		ui->BUTTON_CONFIG->setText("SOUND CONTROL");
		ui->BUTTON_CALL->setText("INTERCOM");
		ui->BUTTON_ROUTE->setText("ROUTE");
		ui->BUTTON_DI->setText("SPEICIAL DI");

        ui->CAB_VOL_TITLE->setText("CAB Volume");
        ui->CAB_MIC_VOL_LABEL->setText("MIC");
        ui->CAB_SPK_VOL_LABEL->setText("SPK");
        ui->PASSENGER_VOL_TITLE->setText("Passenger Volume");
        ui->PASSENGER_IN_VOL_LABEL->setText("IN");
        ui->PASSENGER_OUT_VOL_LABEL->setText("OUT");
        ui->LANG_TITLE->setText("Language");

        ui->FUNC_STATION->setText("STATIONS");
		ui->FUNC_IN->setText("IN");
		ui->FUNC_OUT->setText("OUT");
		ui->FUNC_LRM->setText("LRM");
		ui->FUNC_PIB->setText("PIB");

        ui->TOUCH_CAL->setText("Touch\nCalibration");

        ui->CAB_BUTTON->setText("CAB");
    } else {            // Turkish
        language_selection = LANG_TURKISH;

        { char temp[25] = "ANA MENÜ";		ui->BUTTON_TOP->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "SES KONTROLÜ";	ui->BUTTON_CONFIG->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "İNTERKOM";		ui->BUTTON_CALL->setText(QString::fromUtf8(temp)); }
		{ char temp[25] = "GÜZERGAH";		ui->BUTTON_ROUTE->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "ÖZEL DI";		ui->BUTTON_DI->setText(QString::fromUtf8(temp)); }

        { char temp[25] = "Ses Kontrolü(Kabin)"; ui->CAB_VOL_TITLE->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "Mikrofon";          ui->CAB_MIC_VOL_LABEL->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "Hoparlör";          ui->CAB_SPK_VOL_LABEL->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "Ses Kontrolü(Yolcu)"; ui->PASSENGER_VOL_TITLE->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "İÇ";                ui->PASSENGER_IN_VOL_LABEL->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "DIŞ";               ui->PASSENGER_OUT_VOL_LABEL->setText(QString::fromUtf8(temp)); }

		{ char temp[25] = "Ses Kontrolü(Kabin)"; ui->CALL_VOL_TITLE->setText(QString::fromUtf8(temp)); }
		{ char temp[25] = "Mikrofon";          ui->CALL_MIC_VOL_LABEL->setText(QString::fromUtf8(temp)); }
		{ char temp[25] = "Hoparlör";          ui->CALL_SPK_VOL_LABEL->setText(QString::fromUtf8(temp)); }

        { char temp[25] = "Dil Seçimi";        ui->LANG_TITLE->setText(QString::fromUtf8(temp)); }

        { char temp[25] = "İstasyon";             ui->FUNC_STATION->setText(QString::fromUtf8(temp)); }
        //{ char temp[25] = "İptal";             ui->FUNC_CANCEL->setText(QString::fromUtf8(temp)); }
		{ char temp[25] = "İÇ";					ui->FUNC_IN->setText(QString::fromUtf8(temp)); }
		{ char temp[25] = "DIŞ";				ui->FUNC_OUT->setText(QString::fromUtf8(temp)); }
		{ char temp[25] = "LRM";				ui->FUNC_LRM->setText(QString::fromUtf8(temp)); }
		{ char temp[25] = "PIB";				ui->FUNC_PIB->setText(QString::fromUtf8(temp)); }

        { char temp[25] = "Dokunmatik\nKalibrasyon"; ui->TOUCH_CAL->setText(QString::fromUtf8(temp)); }
        { char temp[25] = "KABİN";            ui->CAB_BUTTON->setText(QString::fromUtf8(temp)); }
    }
}

#include <QMessageBox>

void MainWindow::on_TOUCH_CAL_clicked()
{
    if (!QWSServer::mouseHandler())
        qFatal("No touch handler installed");
#if 0
    {
        QMessageBox message;
        message.setText("<p>Please press once at each of the marks "
                        "shown in the next screen.</p>"
                        "<p>This messagebox will timout after 10 seconds "
                        "if you are unable to close it.</p>");
        QTimer::singleShot(10 * 1000, &message, SLOT(accept()));
        message.exec();
    }
#endif
    TouchCal = new Calibration(this);
    TouchCal->exec();
#if 0
    {
        QMessageBox message;
        message.setText("<p>The next screen will let you test the calibration "
                        "by drawing into a widget.</p><p>This program will "
                        "automatically close after 20 seconds.<p>");
        QTimer::singleShot(10 * 1000, &message, SLOT(accept()));
        message.exec();
    }
    Scribble = new ScribbleWidget(this);
    Scribble->showMaximized();
    Scribble->show();
#endif
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
	static unsigned char count=0;
	static QTime actived_time;
	int x,y;

	x = event->x();
	y = event->y();

	//printf("touch (%d,%d)\n", x, y);
	if((x > 620 && x < 650) && (y < 30))
	{
		count++;

		if(ui->Master_AVC_IP->isVisible() && count >= 8)
		{
			count = 0;
			ui->Master_AVC_IP->setVisible(false);
			ui->Master_AVC_Label->setVisible(false);
			ui->COP_Ver_Label->setVisible(false);
			ui->COP_IP_Label->setVisible(false);

			if(actived_time.elapsed() < 1500)
			{
				printf(">>>> HIDDEN MENU.. SYSTEM RESET - %d <<<<\n",actived_time.elapsed());
				exit(1);
			}
			else
			{
				printf("hidden reset fail : time %d > 1500ms \n", actived_time.elapsed());
				actived_time.restart();
			}
		}
		else if(ui->Master_AVC_IP->isVisible()==false && count >= 4)
		{
			count = 0;
			actived_time.start();
			actived_time.restart();
			ui->Master_AVC_IP->setVisible(true);
			ui->Master_AVC_Label->setVisible(true);
			ui->COP_Ver_Label->setVisible(true);
			ui->COP_IP_Label->setVisible(true);
		}
	}
	else
	{
		ui->Master_AVC_IP->setVisible(false);
		ui->Master_AVC_Label->setVisible(false);
		ui->COP_Ver_Label->setVisible(false);
		ui->COP_IP_Label->setVisible(false);
		count = 0;
	}
}