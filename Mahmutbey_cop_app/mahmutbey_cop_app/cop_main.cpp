#include "cop_main.h"
#include <QtCore>

COP_main::COP_main(QObject *parent) :
    QThread(parent)
{
}

void COP_main::run()
{
    qDebug() << "Start Worker Thread : " << currentThreadId();

    main_call(1, 1);
    //main_call(0, 1);    // Watch-dog disable
}

