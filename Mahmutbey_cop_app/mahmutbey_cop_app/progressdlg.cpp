#include "progressdlg.h"
#include "ui_progressdlg.h"
#include <QDebug>

progressDlg::progressDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::progressDlg)
{
    ui->setupUi(this);
    setWindowFlags( Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint );
    m_pCloseTimer = new QTimer( this );
    connect( m_pCloseTimer, SIGNAL( timeout() ),
             this,          SLOT( closeDlg()));
    m_pCloseTimer->setSingleShot( true );
}

progressDlg::~progressDlg()
{
    delete ui;
}

void progressDlg::prograssing( qint32 uiTotalFiles, qint32 uiRemaindFile,qint32 uiTotalPrograssSize, qint32 uiPrograssSize )
{
    QString label_comp;
    qint32 perFile = ( (double)uiRemaindFile  / (double)uiTotalFiles ) * 100;
    qint32 perRead = ( (double)uiPrograssSize / (double)uiTotalPrograssSize ) * 100;

    if( this->isHidden() )
        this->show();

    ui->progressBar->setValue( 100 - perFile );
    ui->progressBar_2->setValue( perRead );

    label_comp.sprintf("(%d/%d)", uiTotalFiles - uiRemaindFile, uiTotalFiles);
    ui->label_3->setText(label_comp);

    qDebug() << perFile << perRead;
}

void progressDlg::finished( qint32 uiTotalFiles, qint32 uiRemaindFile )
{
    QString label_comp;
    qint32 perFile = ( (double)uiRemaindFile  / (double)uiTotalFiles ) * 100;
    ui->progressBar->setValue( 100 - perFile );
    label_comp.sprintf("(%d/%d)", uiTotalFiles - uiRemaindFile, uiTotalFiles);
    ui->label_3->setText(label_comp);

    m_pCloseTimer->start( 1000 );
}

void progressDlg::closeDlg( void )
{
    if( !this->isHidden() )
        this->hide();
}
