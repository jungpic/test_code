/********************************************************************************
** Form generated from reading UI file 'progressdlg.ui'
**
** Created: Thu Sep 20 17:21:24 2018
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROGRESSDLG_H
#define UI_PROGRESSDLG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>

QT_BEGIN_NAMESPACE

class Ui_progressDlg
{
public:
    QProgressBar *progressBar;
    QProgressBar *progressBar_2;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;

    void setupUi(QDialog *progressDlg)
    {
        if (progressDlg->objectName().isEmpty())
            progressDlg->setObjectName(QString::fromUtf8("progressDlg"));
        progressDlg->resize(400, 259);
        progressBar = new QProgressBar(progressDlg);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(50, 50, 321, 51));
        progressBar->setValue(24);
        progressBar_2 = new QProgressBar(progressDlg);
        progressBar_2->setObjectName(QString::fromUtf8("progressBar_2"));
        progressBar_2->setGeometry(QRect(50, 170, 321, 51));
        progressBar_2->setValue(24);
        label = new QLabel(progressDlg);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(50, 10, 101, 31));
        QFont font;
        font.setPointSize(25);
        label->setFont(font);
        label_2 = new QLabel(progressDlg);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(50, 130, 71, 31));
        label_2->setFont(font);
        label_3 = new QLabel(progressDlg);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(170, 10, 101, 31));
        label_3->setFont(font);

        retranslateUi(progressDlg);

        QMetaObject::connectSlotsByName(progressDlg);
    } // setupUi

    void retranslateUi(QDialog *progressDlg)
    {
        progressDlg->setWindowTitle(QApplication::translate("progressDlg", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("progressDlg", "TOTAL", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("progressDlg", "FILE", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("progressDlg", "(0/0)", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class progressDlg: public Ui_progressDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROGRESSDLG_H
