
#include <QTextCharFormat>
#include <QDate>
#include <QList>
#include <QDebug>
#include "updatemainframe.h"
#include "ui_updatemainframe.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "orig/usb_update.h"
#include "orig/avc_tcp.h"
#include "sub_device.h"

extern packet_AVC2CTM_EVENT_t  avc2ctm_log_event_packet; /*ctm sw log event packet */
extern char AVC_ScheduleTime[19];
#define COLOR_LIGHT_NAVY		"background-color: #B4B9D3; color: black;	\
                       selection-background-color: #B4B9D3;"

extern int iDetectFlag; /* USB insert flag*/

updateMainFrame::updateMainFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::updateMainFrame)
{
    ui->setupUi(this);

    m_parent = (MainWindow *)parent;

    // set color to black for sunday and saturday
    QTextCharFormat weekendFormat;
    weekendFormat.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->calendar->setWeekdayTextFormat(Qt::Saturday, weekendFormat);
    ui->calendar->setWeekdayTextFormat(Qt::Sunday, weekendFormat);

    m_uiYear  = QDate::currentDate().year();
    m_uiMonth = QDate::currentDate().month();

    ui->calendar->setCurrentPage( m_uiYear, m_uiMonth );



    // connecting between signal and slot
    connect( ui->pb_log,      SIGNAL( clicked( void ) ),
             this,            SLOT( logDownloadClicked( void ) ));

    connect( ui->pb_schedule, SIGNAL( clicked( void ) ),
             this,            SLOT( scheduleUploadClicked( void ) ));
    connect( ui->pb_SwUpdate, SIGNAL( clicked() ),
             this,            SLOT( SwUpdateClicked() ));

    connect( ui->calendar   , SIGNAL( currentPageChanged(int, int) ),
             this,            SLOT( pageChanged(int, int) ));

    connect( ui->pb_prev,     SIGNAL( clicked( void ) ),
             this,            SLOT( prevMonthClicked( void ) ));

    connect( ui->pb_next,     SIGNAL( clicked( void ) ),
             this,            SLOT( nextMonthClicked( void ) ));
/*
    connect( ui->cb_year,     SIGNAL( clicked( void ) ),
             this,            SLOT( yearSelecterClicked( void ) ));

    connect( ui->cb_month,    SIGNAL( clicked( void ) ),
             this,            SLOT( yearSelecterClicked( void ) ));
*/

    changeDayColor( QDate::currentDate().year(), QDate::currentDate().month(),
                    QDate::currentDate().day(),  Qt::red );

    QString strDate;
    strDate.sprintf("%d. %02d", m_uiYear, m_uiMonth);
    ui->lb_year->setText(strDate);

    m_pFtpclient = new ftpClient;
    m_pFtpclient->connectToServer( "10.128.9.33", "wbpa", "dowon1222" );

    m_pFtpclient->changeDirectory("/log");
    m_pFtpclient->refreshList();
    connect( m_pFtpclient,    SIGNAL( updateFileList() ),
             this,            SLOT  ( finishedUpdateFileList()));

    m_pDlgProgress = new progressDlg;

    connect( m_pFtpclient   , SIGNAL( prograssing(qint32,qint32,qint32,qint32) ),
             m_pDlgProgress , SLOT  ( prograssing(qint32,qint32,qint32,qint32) ));

    connect( m_pFtpclient   , SIGNAL( finished( qint32,qint32 ) ),
             m_pDlgProgress , SLOT  ( finished( qint32,qint32 ) ));

    connect( m_pFtpclient   , SIGNAL( finished( qint32,qint32 ) ),
             this , SLOT  ( ftpClient_finished( qint32,qint32 ) ));

    m_thread = new Sub_device(this);
    connect(m_thread,SIGNAL(finish_request()),this, SLOT(on_thread_finish()));

    StatusUpdateTimer = new QTimer(this);
    connect(StatusUpdateTimer, SIGNAL(timeout()), this, SLOT(StatusUpdate()));
    StatusUpdateTimer->start(CTM_STATUS_UPDATE_TIME);

    ui->STATUS_LABEL_BG->setStyleSheet(COLOR_LIGHT_NAVY);
}

updateMainFrame::~updateMainFrame()
{
    delete ui;
    delete m_pFtpclient;
    delete m_pDlgProgress;
}

void updateMainFrame::showEvent( QShowEvent* event ) {
    QWidget::showEvent( event );
    //your code here
    m_uiYear  = QDate::currentDate().year();
    m_uiMonth = QDate::currentDate().month();

    ui->calendar->setCurrentPage( m_uiYear, m_uiMonth );

    QString strDate;
    strDate.sprintf("%d. %02d", m_uiYear, m_uiMonth);
    ui->lb_year->setText(strDate);
}

void updateMainFrame::logDownloadClicked( void )
{
    if( iDetectFlag==0){
        qDebug()<<"iDetectFlag return";
        return;
    }
    if(iDetectFlag == 1) {
        QString logPath = "/media/usb0/log";
        QDir dir(logPath);
        dir.mkpath(logPath);

        m_pFtpclient->changeDirectory("/log");
        m_pFtpclient->refreshList();

        //m_pFtpclient->gets("20180911.log.bz2");
        m_pFtpclient->connectToServer( "10.128.9.33", "wbpa", "dowon1222" );
        QString filePrefix = ui->calendar->selectedDate().toString("yyyyMMdd");

        qDebug() << filePrefix << "logDownloadClicked jhlee";

       // m_pFtpclient->gets( filePrefix, "/home/sangwon/Src/log_test" );
         //m_pFtpclient->gets( filePrefix, "/home/jhjo/jhl/log" );

        m_pFtpclient->gets( filePrefix, "/media/usb0/log" );
    }
}

void updateMainFrame::nextMonthClicked( void )
{
    QString strDate;
    if ( m_uiMonth < 12 ) {
        m_uiMonth++;
    } else {
        m_uiMonth = 1;
        m_uiYear = ( m_uiYear < QDate::currentDate().year() ) ? m_uiYear + 1 : m_uiYear;
    }

    ui->calendar->setCurrentPage( m_uiYear, m_uiMonth );

    strDate.sprintf("%d. %02d", m_uiYear, m_uiMonth);
    ui->lb_year->setText(strDate);

    qDebug() << ui->calendar->yearShown() << ui->calendar->monthShown();
}

void updateMainFrame::prevMonthClicked( void )
{
    QString strDate;
    if ( m_uiMonth > 1 ) {
        m_uiMonth--;
    } else {
        m_uiMonth = 12;
        m_uiYear = ( m_uiYear > 1970 ) ? m_uiYear - 1 : m_uiYear;
    }

    ui->calendar->setCurrentPage( m_uiYear, m_uiMonth );

    strDate.sprintf("%d. %02d", m_uiYear, m_uiMonth);
    ui->lb_year->setText(strDate);

    qDebug() << ui->calendar->yearShown() << ui->calendar->monthShown();
}
/*
void updateMainFrame::yearSelecterClicked( void )
{
    qint32 uiCurrYear =  QDate::currentDate().year();
    qint32 uiYear = ui->cb_month->currentText().toInt();

    if ( uiCurrYear <= uiYear ||
         uiYear >= 1970 ) {

        ui->calendar->setCurrentPage( uiYear, m_uiMonth );
    }
}

void updateMainFrame::monthSelecterClicked( void )
{
    qint32 uiMonth = ui->cb_month->currentText().toInt();

    if ( uiMonth <= 12 ||
         uiMonth >= 1 ) {

        ui->calendar->setCurrentPage( m_uiYear, uiMonth );
    }
}
*/
void updateMainFrame::scheduleUploadClicked( void )
{
#if 0
    QList<QString> list;
    QString strDir = "/media/usb0/sw_update/";

    QDirIterator iterDir(strDir, QDir::Files, QDirIterator::NoIteratorFlags);

    while (iterDir.hasNext())
    {
        list.push_back(iterDir.next());
    }
    m_pFtpclient->puts( &list );

#endif
    if( iDetectFlag==0){
        qDebug()<<"iDetectFlag return";
        return;
    }

    QList<QString> list;
    m_pFtpclient->connectToServer( "10.128.9.33", "wbpa", "dowon1222" );

    QString strDir = "/media/usb0/sw_update/";
    QDirIterator iterDir(strDir, QDir::Files, QDirIterator::NoIteratorFlags);
    //static int clicked_flag =0; clicked..:<


    if(iDetectFlag ==1){

        StatusUpdateTimer->stop();
        while (iterDir.hasNext())
        {
            list.push_back(iterDir.next());
        }
        m_pFtpclient->puts( &list );

    }


}
void updateMainFrame::SwUpdateClicked( void )
{
    qDebug() << "SwUpdateClicked";
    m_parent->ui->MENU->setCurrentIndex(SWUPDATE_MENU);
    m_thread->start();

}



void updateMainFrame::finishedUpdateFileList( void )
{
    qDebug() << "finish" << ui->calendar->maximumDate();

    u_int32_t uiYear = ui->calendar->yearShown();
    u_int32_t uiMonth= ui->calendar->monthShown();

    for( int i = 0 ; i < 31 ; i++ ) {
        QDate date( uiYear, uiMonth, i );

        Q_FOREACH( const QString file, *m_pFtpclient->getFileList() ) {
            if( file.contains( date.toString("yyyyMMdd") ) )
            {
                changeDayColor( uiYear, uiMonth, i, Qt::red );
            }
        }
    }
}

void updateMainFrame::changeDayColor( u_int32_t uiYear, u_int32_t uiMonth, u_int32_t uiDay, Qt::GlobalColor color )
{
    QDate date1( uiYear, uiMonth, uiDay );
    QTextCharFormat firstFridayFormat;

    // set
    firstFridayFormat.setForeground( color );
    ui->calendar->setDateTextFormat( date1, firstFridayFormat );
}

void updateMainFrame::pageChanged(int year, int month)
{
    Q_UNUSED( year );
    Q_UNUSED( month );
    m_pFtpclient->connectToServer( "10.128.9.33", "wbpa", "dowon1222" );
    m_pFtpclient->refreshList();
}

void updateMainFrame::StatusUpdate(void)
{

    ui->lb_AVC_Schedule->setText(AVC_ScheduleTime);
    /* USB status information ui */
    if( iDetectFlag==1)
    {
        ui->USB_INSERT->setPixmap(QPixmap(":/usb_on.jpg"));
        ui->USB_INSERT->show();
        //ui->pb_schedule->setStyleSheet(COLOR_FUNC_NUM);
        ui->pb_schedule->setIcon(QIcon(QPixmap(":/schedule_over.jpg")));
        ui->pb_schedule->setIconSize(QSize(180,60));
        ui->pb_schedule->show();
        ui->pb_schedule->setDisabled(false);
        //ui->pb_log->setStyleSheet(COLOR_FUNC_NUM);
        ui->pb_log->setIcon(QIcon(QPixmap(":/log_over.jpg")));
        ui->pb_log->setIconSize(QSize(180,60));
        ui->pb_log->show();
        ui->pb_log->setDisabled(false);


    }else
    {

        ui->USB_INSERT->setPixmap(QPixmap(":/usb_off.jpg"));
        ui->USB_INSERT->show();
        ui->pb_schedule->setIcon(QIcon(QPixmap(":/schedule_off.jpg")));
        ui->pb_schedule->setIconSize(QSize(180,60));
        ui->pb_schedule->show();
        ui->pb_schedule->setDisabled(true);
        ui->pb_log->setIcon(QIcon(QPixmap(":/log_off.jpg")));
        ui->pb_log->setIconSize(QSize(180,60));
        ui->pb_log->show();
        ui->pb_log->setDisabled(true);


    }
}

void updateMainFrame::on_thread_finish(void)
{
    printf("jhlee on_thread_finish ");
}


void updateMainFrame::ftpClient_finished( qint32,qint32 )
{
    qDebug()<<"StatusUpdateTimer ftpClient_finished";
    StatusUpdateTimer->start(CTM_STATUS_UPDATE_TIME);

}



