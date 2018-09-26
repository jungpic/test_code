#include "ftpclient.h"

#include <QStringList>
#include <QDebug>

ftpClient::ftpClient(QObject *parent) :
    QObject(parent),
    m_pFile( NULL ),
    m_pFtpClient( NULL ),
    m_pFileList( NULL ),
    m_pPutFileList( NULL ),
    m_uiTotalFiles( 0 ),
    m_ftpStat( FTP_STAT_NONE )
{
    // init ftp client
    m_pFtpClient = new QFtp ( this );
    m_pFileList  = new QList<QString>;

    connect(m_pFtpClient, SIGNAL(commandFinished(int,bool)),
            this, SLOT(ftpCommandFinished(int,bool)));
    connect(m_pFtpClient, SIGNAL(listInfo(QUrlInfo)),
            this, SLOT(addToList(QUrlInfo)));
    connect(m_pFtpClient, SIGNAL(dataTransferProgress(qint64,qint64)),
            this, SLOT(updateDataTransferProgress(qint64,qint64)));
}

void ftpClient::connectToServer( QString strIp, QString strID, QString strPw )
{
    qDebug() <<  m_pFtpClient->errorString() << m_pFtpClient->currentCommand() << m_pFtpClient->state();

    if( m_pFtpClient->state() < QFtp::Connected ) {
        m_pFtpClient->connectToHost( strIp );
    }
    if( m_pFtpClient->state() < QFtp::LoggedIn ) {
        m_pFtpClient->login( strID, strPw );
    }
}

void ftpClient::_init()
{
    if( m_pFile != NULL )
        if( m_pFile->isOpen() )
            m_pFile->close();

    if( m_pFileList != NULL )
        m_pFileList->clear();

    m_uiTotalFiles = 0;
}

void ftpClient::close( void )
{
    _init();
    m_pFtpClient->abort();
    m_pFtpClient->close();
/*
    if( m_pFile != NULL) {
        if( m_pFile->isOpen() )
            m_pFile->close();
        delete m_pFile;
        m_pFile = NULL;
    }

    if( m_pFtpClient != NULL ) {
        delete m_pFtpClient;
        m_pFtpClient = NULL;
    }

    if( m_pFileList != NULL ) {
        delete m_pFileList;
        m_pFileList = NULL;
    }
*/
    setFtpStat( FTP_STAT_NONE );
}

ftpClient::~ftpClient()
{
    close();
}

void ftpClient::ftpCommandFinished(int commandId,bool error)
{
    eErrCode err_code = FTP_ERR_NONE;
    bool bReq = false;
    qDebug() << Q_FUNC_INFO << commandId << error << m_pFtpClient->currentCommand();

    int uiFtpStat = m_pFtpClient->currentCommand();

    switch( uiFtpStat )
    {
    case QFtp::ConnectToHost:
        if( error ) {
            err_code = FTP_ERR_CONN;
        }
        break;
    case QFtp::Login:
        if( !error ) {
           //setFtpStat( FTP_STAT_NORMAL );
        } else {
            err_code = FTP_ERR_LOGIN;
        }
        break;
    case QFtp::Cd:
        if( !error ) {
            if( m_pFtpClient != NULL ) {
                 m_pFtpClient->list();
            } else {
                err_code = FTP_ERR_UNKNOW;
            }
        } else {
           err_code = FTP_ERR_CD;
        }
        break;
     case QFtp::List:
        if( !error ) {
            if( getFtpStat() == FTP_STAT_GET ||
                getFtpStat() == FTP_STAT_GETS ) {
                if( m_pFileList != NULL ) {
                    m_uiTotalFiles = m_pFileList->size();
                } else {
                    err_code = FTP_ERR_UNKNOW;
                }
                bReq = true;
            } else {
                emit updateFileList();
            }
        } else {
            if( m_pFileList != NULL &&
                !m_pFileList->isEmpty() ) {

                m_pFileList->pop_back();
                err_code = FTP_ERR_LIST;
            } else {
                err_code = FTP_ERR_UNKNOW;
            }
        }

        break;
     case QFtp::Get:
        if( m_pFile != NULL && m_pFile->isOpen() )
            m_pFile->close();
        if( !error ) {
            if( getFtpStat() == FTP_STAT_GETS ) {
                bReq = true;
            } else {
                err_code = FTP_ERR_UNKNOW;
            }
        } else {
            if( m_pFileList != NULL ) {
                bReq = true;
                err_code = FTP_ERR_GET;
            }else {
                err_code = FTP_ERR_UNKNOW;
            }
        }
        break;
    case QFtp::Put:
        if( m_pFile != NULL && m_pFile->isOpen() )
            m_pFile->close();
        if( !error ) {
            if( getFtpStat() == FTP_STAT_PUT ) {
                bReq = true;
            } else {
                err_code = FTP_ERR_UNKNOW;
            }
        } else {
            if ( m_pFileList != NULL ) {
                bReq = true;
                err_code = FTP_ERR_PUT;
            } else {
                err_code = FTP_ERR_UNKNOW;
            }
        }
        break;
    case QFtp::Close:
        if( m_pFile != NULL && m_pFile->isOpen() ) {
            m_pFile->close();
            delete m_pFile;
            m_pFile = NULL;
    }

        if( m_pFileList != NULL ) {
            m_pFileList->clear();
        }

        setFtpStat( FTP_STAT_NONE );

        break;
    case QFtp::Mkdir:
        break;
    }    

    if( bReq ) {
        if( m_pFileList != NULL )
        {
            if( !m_pFileList->isEmpty() )
                m_pFileList->pop_back();

            if ( !m_pFileList->isEmpty() )
            {
                if( getFtpStat() == FTP_STAT_GETS ||
                    getFtpStat() == FTP_STAT_GET )
                {
                    _get( m_pFileList->back() );
                }
                else if ( m_ftpStat == FTP_STAT_PUT )
                {
                    //!, re-try to put the file. if not exist file, send the signal for the finish of the file transfer.
                    while( !_put( m_pFileList->back() ) ) {

                        m_pFileList->pop_back();

                        if ( m_pFileList->isEmpty() ) {
                            break;
                        }
                    }
                }
                else {
                    err_code = FTP_ERR_UNKNOW;
                }
            }
        } else {
            err_code = FTP_ERR_UNKNOW;
        }
    }

    //!, if not exist file, send the signal for the finish of the file transfer.
    if( m_pFileList->isEmpty() &&
        getFtpStat() != FTP_STAT_NONE ) {
        setFtpStat( FTP_STAT_NONE );
        emit finished( m_uiTotalFiles, m_pFileList->size() );
        m_pFtpClient->close();
    }

    //!, if an error code is set, send the error code.
    if( err_code != FTP_ERR_NONE )
        emit errCode( err_code );
}

void ftpClient::addToList(const QUrlInfo &urlInfo)
{
    bool bRet = false;

    if( getFtpStat() == FTP_STAT_GET )
        bRet = ( urlInfo.name().compare( m_strFileFilter ) == 0 ) ? true : false;
    else if ( getFtpStat() == FTP_STAT_GETS )
        bRet = urlInfo.name().contains( m_strFileFilter );
    else
        bRet = true;

    if( bRet ) {
        qDebug() << urlInfo.name() << m_strFileFilter;
        m_pFileList->push_back( urlInfo.name() );
    }
}

void ftpClient::updateDataTransferProgress(qint64 readBytes,qint64 totalBytes)
{
   // qDebug() << Q_FUNC_INFO << m_uiTotalFiles << m_pFileList->size() <<readBytes << totalBytes;
    emit prograssing( m_uiTotalFiles, m_pFileList->size(), totalBytes, readBytes );
}

void ftpClient::changeDirectory( QString strDirPath )
{
    m_pFileList->clear();
    m_pFtpClient->cd( strDirPath );
}

void ftpClient::setFtpStat( eFTP_STAT stat )
{
    qDebug() << Q_FUNC_INFO << stat;
    m_ftpStat = stat;
}

ftpClient::eFTP_STAT ftpClient::getFtpStat( void )
{
    return m_ftpStat;
}

QList<QString>* ftpClient::getFileList( void )
{
    return m_pFileList;
}

void ftpClient::refreshList( void )
{
    m_pFtpClient->list();
}

void ftpClient::_get( QString strFile )
{
    QString strDstPath = m_strDstDir;;

    qDebug() << strDstPath.append("/").append(strFile);

    if( m_pFile == NULL ) {
        m_pFile = new QFile();
    } else {
        m_pFile->close();
    }

    m_pFile->setFileName( strDstPath );

    if( !m_pFile->open(QIODevice::WriteOnly) ) {
        delete m_pFile;
        m_pFile = NULL;
        emit errCode( FTP_ERR_FILE_OPEN );
    } else {
    m_pFtpClient->get( strFile, m_pFile );
    }
}

void ftpClient::get( QString strFile, QString strDstPath )
{
    if( getFtpStat() == FTP_STAT_NONE ) {
        m_strFileFilter = strFile;
        m_strDstDir     = strDstPath;
        setFtpStat( FTP_STAT_GET );
        _init();
        m_pFtpClient->list();
    }
}

void ftpClient::gets( QString strFilePreFix, QString strDstPath )
{
    if( getFtpStat() == FTP_STAT_NONE ) {
        m_strFileFilter = strFilePreFix;
        m_strDstDir     = strDstPath;
        setFtpStat( FTP_STAT_GETS );
        _init();
        m_pFtpClient->list();
    }
}


bool ftpClient::_put( QString strFile )
{
    bool bRet = false;
    QStringList lst = strFile.split('/');
    QString strFileName = lst[ lst.count()-1 ];


    if( m_pFile == NULL ) {
        m_pFile = new QFile();
    } else {
        m_pFile->close();
    }

    m_pFile->setFileName( strFile );

    if( !m_pFile->open(QIODevice::ReadOnly) ) {
        delete m_pFile;
        m_pFile = NULL;
        emit errCode( FTP_ERR_FILE_OPEN );
        bRet = false;
    } else {
        setFtpStat( FTP_STAT_PUT );
    m_pFtpClient->put( m_pFile, strFileName );
        bRet = true;
}

    return bRet;
}

void ftpClient::put( QString strFile )
{
    if( getFtpStat() == FTP_STAT_NONE ) {
        m_pFileList->clear();
        m_pFileList->push_back( strFile );
        m_uiTotalFiles = m_pFileList->size();
        setFtpStat( FTP_STAT_PUT );
        _put( strFile );
    }

    qDebug() << m_pFtpClient->state() << m_pFtpClient->errorString() << m_pFtpClient->error();
}

void ftpClient::puts(  QList<QString>* pFileList )
{
    if( getFtpStat() == FTP_STAT_NONE )
    {
        if( pFileList != NULL ) {
            m_pFileList->clear();
            m_pFileList->append( *pFileList );
            m_uiTotalFiles = m_pFileList->size();
            setFtpStat( FTP_STAT_PUT );
            _put( pFileList->back() );
        }
    }

    qDebug() << m_pFtpClient->state() << m_pFtpClient->errorString() << m_pFtpClient->error();
}
