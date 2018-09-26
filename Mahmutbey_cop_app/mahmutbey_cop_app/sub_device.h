#ifndef SUB_DEVICE_H
#define SUB_DEVICE_H

#include <QThread>

class Sub_device : public QThread
{
    Q_OBJECT
public:
    explicit Sub_device(QObject *parent = 0);
private:
    void run();
    
signals:
    void finish_request(void);
public slots:
    
};

#endif // SUB_DEVICE_H
