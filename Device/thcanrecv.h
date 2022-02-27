#ifndef THCANRECV_H
#define THCANRECV_H

#include <QThread>
#include <QObject>

#include <linux/can.h>
#include <linux/can/raw.h>

class thCanRecv : public QThread
{
    Q_OBJECT
public:
    thCanRecv();

    QString mRecvDevice;
    QString mRecvSpeed;

protected:
    void run();

signals:
    void sendCanData(char*);
};

#endif // THCANRECV_H
