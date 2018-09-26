#-------------------------------------------------
#
# Project created by QtCreator 2015-05-02T20:04:43
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = cop__app
TEMPLATE = app

#QMAKE_LFLAGS_WINDOWS += --stack,1000000
QMAKE_CXXFLAGS_RELEASE *= -O3

LIBS += /opt/arm-eabi-broadcast-uclibc-linux-2012.11/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/lib/mysql/libmysqlclient.a

INCLUDEPATH += /opt/arm-eabi-broadcast-uclibc-linux-2012.11/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/include/mysql

SOURCES += main.cpp\
        mainwindow.cpp \
    orig/watchdog_trigger.c \
    orig/version.c \
    orig/software_update.c \
    orig/send_cob2avc_cmd.c \
    orig/safe_read_write.c \
    orig/recv_avc2cob_cmd.c \
    orig/mtd_nandwrite.c \
    orig/mtd_nanddump.c \
    orig/mtd_info.c \
    orig/libmtd_legacy.c \
    orig/libmtd.c \
    orig/lcd.c \
    orig/gpio_key_dev.c \
    orig/get_pid_by_name.c \
    orig/ftpget.c \
    orig/font_8x8.c \
    orig/flash_erase.c \
    orig/debug_multicast.c \
    orig/crc16.c \
    orig/copyfd.c \
    orig/cob_process.c \
    orig/bmp_24bpp_image.c \
    orig/bcd.c \
    orig/avc_tcp.c \
    orig/avc_cycle_msg.c \
    orig/audio_multicast.c \
    orig/audio_ctrl.c \
    orig/main_cop.c \
    cop_main.cpp \
    orig/pid_multicast.c \
    scribblewidget.cpp \
    calibration.cpp \
    orig/func_db_update.c \
    orig/g711.c

HEADERS  += mainwindow.h \
    orig/xalloc.h \
    orig/watchdog_trigger.h \
    orig/vs1053.h \
    orig/version.h \
    orig/software_update.h \
    orig/send_cob2avc_cmd.h \
    orig/safe_read_write.h \
    orig/recv_avc2cob_cmd.h \
    orig/mtdutil_common.h \
    orig/mtd-user.h \
    orig/lm1971_ctrl.h \
    orig/libmtd_int.h \
    orig/libmtd.h \
    orig/lcd.h \
    orig/HUMETRO_logo.h \
    orig/Humetro_256.h \
    orig/gpio_key_dev.h \
    orig/ftpget.h \
    orig/font.h \
    orig/Fire_119.h \
    orig/fbutils.h \
    orig/debug_multicast.h \
    orig/crc16.h \
    orig/copyfd.h \
    orig/cob_system_error.h \
    orig/cob_process.h \
    orig/Call_119.h \
    orig/bmp_image.h \
    orig/bcd.h \
    orig/avc_tcp.h \
    orig/avc_cycle_msg.h \
    orig/audio_multicast.h \
    orig/audio_ctrl.h \
    cop_main.h \
    scribblewidget.h \
    calibration.h \
    orig/g711.h

FORMS    += mainwindow.ui

OTHER_FILES +=

RESOURCES += \
    mahmutbey_cop.qrc
