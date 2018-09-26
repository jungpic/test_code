#ifndef UPDATEMAINFRAME_H
#define UPDATEMAINFRAME_H

#include <QFrame>


#include "progressdlg.h"
#include "ftpclient.h"
#include "sub_device.h"


namespace Ui {
class updateMainFrame;
class MainWindow;
}

class QUrlInfo;
class MainWindow;

class updateMainFrame : public QFrame
{
    Q_OBJECT
    
public:
    explicit updateMainFrame(QWidget *parent = 0);
    ~updateMainFrame();
    
private:
    Ui::updateMainFrame *ui;
    progressDlg*    m_pDlgProgress;
    ftpClient*      m_pFtpclient;
    ftpClient*      m_pFtpclient2;
    MainWindow*     m_parent;
    u_int32_t       m_uiYear;
    u_int32_t       m_uiMonth;
    QTimer          *StatusUpdateTimer;
    Sub_device          *m_thread;

public slots:
    void logDownloadClicked( void );
    void scheduleUploadClicked( void );
    void nextMonthClicked( void );
    void prevMonthClicked( void );
//    void yearSelecterClicked( void );
//    void monthSelecterClicked( void );
    void finishedUpdateFileList( void );
    void pageChanged(int year, int month);
    void SwUpdateClicked(void);
    void StatusUpdate(void);
    void on_thread_finish(void);


public:
    void changeDayColor( u_int32_t uiYear, u_int32_t uiMonth, u_int32_t uiDay, Qt::GlobalColor color );
protected:
    void showEvent( QShowEvent* event );
private slots:
    void ftpClient_finished( qint32,qint32 );

};

#endif // UPDATEMAINFRAME_H
