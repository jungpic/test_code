#include "file_upload.h"
#include "stdio.h"
#include "mainwindow.h"
#include "orig/avc_tcp.h"
#include "orig/usb_update.h"
#include <QDebug>

//File_Upload::File_Upload()
//{
//}

File_Upload::File_Upload(QObject *parent) :
    QThread(parent)
{
}




void File_Upload::run_upload()
{
//    int dev_count = 0;
//    char swver[2]={0};

    /*file up load device thread for progressbar */


    File_Upload::msleep(20);

    emit finish_fileUpload();
}
