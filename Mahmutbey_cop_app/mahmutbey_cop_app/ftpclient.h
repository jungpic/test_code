#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QObject>
#include <QtNetwork/QFtp>
#include <QUrlInfo>
#include <QList>
#include <QFile>

class ftpClient : public QObject
{
    Q_OBJECT

private:


    enum eErrCode {
        FTP_ERR_NONE,
        FTP_ERR_CONN,
        FTP_ERR_LOGIN,
        FTP_ERR_GET,
        FTP_ERR_PUT,
        FTP_ERR_LIST,
        FTP_ERR_CD,
        FTP_ERR_NO_FILE,
        FTP_ERR_FILE_OPEN,
        FTP_ERR_UNKNOW
    };

public:
    enum eFTP_STAT {
        FTP_STAT_NONE,
        FTP_STAT_NORMAL,
        FTP_STAT_GETS,
        FTP_STAT_GET,
        FTP_STAT_PUT
    };
    explicit ftpClient(QObject *parent = 0);
    ~ftpClient();
signals:
    void prograssing    ( qint32 uiTotalFiles, qint32 uiRemaindFile,qint32 uiTotalPrograssSize, qint32 uiPrograssSize );
    void finished       ( qint32 uiTotalFiles, qint32 uiRemaindFile );
    void errCode        ( qint32 ErrCode );
    void updateFileList ( void );
public slots:
    void ftpCommandFinished(int,bool);
    void addToList      (const QUrlInfo &urlInfo);
    void updateDataTransferProgress(qint64,qint64);
public:
    void connectToServer( QString strIp, QString strID, QString strPw );
    void close          ( void );
    void setFtpStat     ( eFTP_STAT stat );
    eFTP_STAT getFtpStat( void );
    void changeDirectory( QString strDirPath );
    void put            ( QString strFilePath );
    void puts           ( QList<QString>* pFileList );
    void get            ( QString strFilePrefix, QString strDstPath );
    void gets           ( QString strFilePrefix, QString strDstPath );
    void setFileFilter  ( QString strPreFix );
    void refreshList    ( void );
    QList<QString>*     getFileList( void );
    bool                isConnect( void );
private:
    void _init          ( void );
    void _get           ( QString strFile );
    bool _put           ( QString strFile );
private:
    QFtp*            m_pFtpClient;
    QList<QString>*  m_pFileList;
    QList<QString>*  m_pPutFileList;
    QString          m_strFileFilter;
    QString          m_strCurrDir;
    eFTP_STAT        m_ftpStat;
    QFile*           m_pFile;
    QString          m_strDstDir;
    qint32           m_uiTotalFiles;
private:
};

#endif // FTPCLIENT_H
