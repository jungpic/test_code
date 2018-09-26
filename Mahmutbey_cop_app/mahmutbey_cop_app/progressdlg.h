#ifndef PROGRESSDLG_H
#define PROGRESSDLG_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class progressDlg;
}

class progressDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit progressDlg(QWidget *parent = 0);
    ~progressDlg();
public slots:
    void prograssing( qint32 uiTotalFiles, qint32 uiRemaindFile,qint32 uiTotalPrograssSize, qint32 uiPrograssSize );
    void finished   ( qint32 uiTotalFiles, qint32 uiRemaindFile );
    void closeDlg   ( void );
private:
    Ui::progressDlg *ui;
    QTimer*         m_pCloseTimer;
};

#endif // PROGRESSDLG_H
