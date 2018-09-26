#ifndef COP_MAIN_H
#define COP_MAIN_H

#include <QThread>
#include <QTimer>
#include <QDebug>

extern "C" {
int main_call(int arg1, int arg2);
}

class COP_main : public QThread
{
    Q_OBJECT
public:
    explicit COP_main(QObject *parent = 0);
    void run();
    
signals:

public slots:
private slots:
private:
    
};

#endif // COP_MAIN_H
