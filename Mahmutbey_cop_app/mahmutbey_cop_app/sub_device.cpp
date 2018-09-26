#include "sub_device.h"
#include "stdio.h"
#include "mainwindow.h"
#include "orig/avc_tcp.h"
#include "orig/usb_update.h"
#include <QDebug>

extern SubDevice_t g_SubDevice[150]; /*MC1 , T, M, MC2 */

Sub_device::Sub_device(QObject *parent) :
    QThread(parent)
{
}

void Sub_device::run()
{
    int dev_count = 0;
    char swver[2]={0};

    /*ctm version request 148 device */

    for(dev_count = 0; dev_count< SUB_DEVICE_CNT ;dev_count++) {
        send_cop2avc_cmd_sw_version_request(swver,g_SubDevice[dev_count].DeviceAddr4,g_SubDevice[dev_count].DeviceAddr3,
                g_SubDevice[dev_count].DeviceAddr2,g_SubDevice[dev_count].DeviceAddr1	);
            Sub_device::msleep(20);
    }


    emit finish_request();
}
