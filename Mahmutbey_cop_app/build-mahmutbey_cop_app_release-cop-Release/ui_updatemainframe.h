/********************************************************************************
** Form generated from reading UI file 'updatemainframe.ui'
**
** Created: Thu Sep 20 17:21:24 2018
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UPDATEMAINFRAME_H
#define UI_UPDATEMAINFRAME_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCalendarWidget>
#include <QtGui/QCheckBox>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_updateMainFrame
{
public:
    QPushButton *pb_schedule;
    QPushButton *pb_log;
    QCalendarWidget *calendar;
    QCheckBox *cb_avc;
    QCheckBox *cb_nvr;
    QPushButton *pb_prev;
    QPushButton *pb_next;
    QLabel *lb_year;
    QPushButton *pb_SwUpdate;
    QLabel *lb_AVC_Schedule;
    QLabel *USB_INSERT;
    QLabel *STATUS_LABEL_BG;

    void setupUi(QFrame *updateMainFrame)
    {
        if (updateMainFrame->objectName().isEmpty())
            updateMainFrame->setObjectName(QString::fromUtf8("updateMainFrame"));
        updateMainFrame->resize(800, 360);
        updateMainFrame->setFrameShape(QFrame::StyledPanel);
        updateMainFrame->setFrameShadow(QFrame::Raised);
        pb_schedule = new QPushButton(updateMainFrame);
        pb_schedule->setObjectName(QString::fromUtf8("pb_schedule"));
        pb_schedule->setGeometry(QRect(10, 10, 181, 61));
        QFont font;
        font.setPointSize(15);
        font.setBold(true);
        font.setWeight(75);
        pb_schedule->setFont(font);
        pb_log = new QPushButton(updateMainFrame);
        pb_log->setObjectName(QString::fromUtf8("pb_log"));
        pb_log->setGeometry(QRect(10, 120, 181, 61));
        pb_log->setFont(font);
        calendar = new QCalendarWidget(updateMainFrame);
        calendar->setObjectName(QString::fromUtf8("calendar"));
        calendar->setGeometry(QRect(310, 90, 461, 251));
        QFont font1;
        font1.setPointSize(17);
        calendar->setFont(font1);
        calendar->setGridVisible(false);
        calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
        calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        calendar->setNavigationBarVisible(false);
        cb_avc = new QCheckBox(updateMainFrame);
        cb_avc->setObjectName(QString::fromUtf8("cb_avc"));
        cb_avc->setGeometry(QRect(20, 190, 111, 21));
        QFont font2;
        font2.setPointSize(18);
        cb_avc->setFont(font2);
        cb_nvr = new QCheckBox(updateMainFrame);
        cb_nvr->setObjectName(QString::fromUtf8("cb_nvr"));
        cb_nvr->setGeometry(QRect(160, 190, 85, 21));
        cb_nvr->setFont(font2);
        pb_prev = new QPushButton(updateMainFrame);
        pb_prev->setObjectName(QString::fromUtf8("pb_prev"));
        pb_prev->setGeometry(QRect(310, 20, 101, 61));
        pb_prev->setFont(font);
        pb_next = new QPushButton(updateMainFrame);
        pb_next->setObjectName(QString::fromUtf8("pb_next"));
        pb_next->setGeometry(QRect(670, 20, 101, 61));
        pb_next->setFont(font);
        lb_year = new QLabel(updateMainFrame);
        lb_year->setObjectName(QString::fromUtf8("lb_year"));
        lb_year->setGeometry(QRect(480, 31, 141, 41));
        QFont font3;
        font3.setPointSize(20);
        lb_year->setFont(font3);
        pb_SwUpdate = new QPushButton(updateMainFrame);
        pb_SwUpdate->setObjectName(QString::fromUtf8("pb_SwUpdate"));
        pb_SwUpdate->setGeometry(QRect(10, 250, 181, 61));
        pb_SwUpdate->setFont(font);
        lb_AVC_Schedule = new QLabel(updateMainFrame);
        lb_AVC_Schedule->setObjectName(QString::fromUtf8("lb_AVC_Schedule"));
        lb_AVC_Schedule->setGeometry(QRect(10, 80, 221, 41));
        QFont font4;
        font4.setPointSize(15);
        lb_AVC_Schedule->setFont(font4);
        USB_INSERT = new QLabel(updateMainFrame);
        USB_INSERT->setObjectName(QString::fromUtf8("USB_INSERT"));
        USB_INSERT->setGeometry(QRect(220, 250, 61, 81));
        QFont font5;
        font5.setPointSize(15);
        font5.setBold(false);
        font5.setWeight(50);
        USB_INSERT->setFont(font5);
        USB_INSERT->setLayoutDirection(Qt::LeftToRight);
        USB_INSERT->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        STATUS_LABEL_BG = new QLabel(updateMainFrame);
        STATUS_LABEL_BG->setObjectName(QString::fromUtf8("STATUS_LABEL_BG"));
        STATUS_LABEL_BG->setGeometry(QRect(0, 0, 801, 361));
        QPalette palette;
        QBrush brush(QColor(0, 0, 0, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
        QBrush brush1(QColor(170, 255, 255, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Button, brush1);
        QBrush brush2(QColor(255, 255, 255, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Light, brush2);
        QBrush brush3(QColor(212, 255, 255, 255));
        brush3.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Midlight, brush3);
        QBrush brush4(QColor(85, 127, 127, 255));
        brush4.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Dark, brush4);
        QBrush brush5(QColor(113, 170, 170, 255));
        brush5.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Active, QPalette::Text, brush);
        palette.setBrush(QPalette::Active, QPalette::BrightText, brush2);
        palette.setBrush(QPalette::Active, QPalette::ButtonText, brush);
        palette.setBrush(QPalette::Active, QPalette::Base, brush2);
        palette.setBrush(QPalette::Active, QPalette::Window, brush1);
        palette.setBrush(QPalette::Active, QPalette::Shadow, brush);
        palette.setBrush(QPalette::Active, QPalette::AlternateBase, brush3);
        QBrush brush6(QColor(255, 255, 220, 255));
        brush6.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::ToolTipBase, brush6);
        palette.setBrush(QPalette::Active, QPalette::ToolTipText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Button, brush1);
        palette.setBrush(QPalette::Inactive, QPalette::Light, brush2);
        palette.setBrush(QPalette::Inactive, QPalette::Midlight, brush3);
        palette.setBrush(QPalette::Inactive, QPalette::Dark, brush4);
        palette.setBrush(QPalette::Inactive, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Inactive, QPalette::Text, brush);
        palette.setBrush(QPalette::Inactive, QPalette::BrightText, brush2);
        palette.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Base, brush2);
        palette.setBrush(QPalette::Inactive, QPalette::Window, brush1);
        palette.setBrush(QPalette::Inactive, QPalette::Shadow, brush);
        palette.setBrush(QPalette::Inactive, QPalette::AlternateBase, brush3);
        palette.setBrush(QPalette::Inactive, QPalette::ToolTipBase, brush6);
        palette.setBrush(QPalette::Inactive, QPalette::ToolTipText, brush);
        palette.setBrush(QPalette::Disabled, QPalette::WindowText, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::Button, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Light, brush2);
        palette.setBrush(QPalette::Disabled, QPalette::Midlight, brush3);
        palette.setBrush(QPalette::Disabled, QPalette::Dark, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::Mid, brush5);
        palette.setBrush(QPalette::Disabled, QPalette::Text, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::BrightText, brush2);
        palette.setBrush(QPalette::Disabled, QPalette::ButtonText, brush4);
        palette.setBrush(QPalette::Disabled, QPalette::Base, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Window, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Shadow, brush);
        palette.setBrush(QPalette::Disabled, QPalette::AlternateBase, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::ToolTipBase, brush6);
        palette.setBrush(QPalette::Disabled, QPalette::ToolTipText, brush);
        STATUS_LABEL_BG->setPalette(palette);
        QFont font6;
        font6.setPointSize(20);
        font6.setBold(false);
        font6.setItalic(false);
        font6.setWeight(50);
        STATUS_LABEL_BG->setFont(font6);
        STATUS_LABEL_BG->setAlignment(Qt::AlignCenter);
        STATUS_LABEL_BG->raise();
        pb_schedule->raise();
        pb_log->raise();
        calendar->raise();
        cb_avc->raise();
        cb_nvr->raise();
        pb_prev->raise();
        pb_next->raise();
        lb_year->raise();
        pb_SwUpdate->raise();
        lb_AVC_Schedule->raise();
        USB_INSERT->raise();

        retranslateUi(updateMainFrame);

        QMetaObject::connectSlotsByName(updateMainFrame);
    } // setupUi

    void retranslateUi(QFrame *updateMainFrame)
    {
        updateMainFrame->setWindowTitle(QApplication::translate("updateMainFrame", "Frame", 0, QApplication::UnicodeUTF8));
        pb_schedule->setText(QString());
        pb_log->setText(QString());
        cb_avc->setText(QApplication::translate("updateMainFrame", "AVC", 0, QApplication::UnicodeUTF8));
        cb_nvr->setText(QApplication::translate("updateMainFrame", "NVR", 0, QApplication::UnicodeUTF8));
        pb_prev->setText(QApplication::translate("updateMainFrame", "<", 0, QApplication::UnicodeUTF8));
        pb_next->setText(QApplication::translate("updateMainFrame", ">", 0, QApplication::UnicodeUTF8));
        lb_year->setText(QApplication::translate("updateMainFrame", "2018. 09", 0, QApplication::UnicodeUTF8));
        pb_SwUpdate->setText(QApplication::translate("updateMainFrame", "SW Update Menu", 0, QApplication::UnicodeUTF8));
        lb_AVC_Schedule->setText(QApplication::translate("updateMainFrame", "2018-09-27 11:22:22", 0, QApplication::UnicodeUTF8));
        USB_INSERT->setText(QString());
        STATUS_LABEL_BG->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class updateMainFrame: public Ui_updateMainFrame {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UPDATEMAINFRAME_H
