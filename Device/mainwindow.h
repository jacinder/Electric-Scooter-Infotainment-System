#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "thcanrecv.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QTimer* tmrRtcStat;
    QTimer* tmrHeadLight;
    QTimer* tmrTurnSignal;
    QTimer* tmrScooterStat;
    QTimer* tmrAccelBrake;
    QTimer* tmrTemperature;
    QTimer* tmrSpeedStat;
    QTimer* tmrCdsStat;

    thCanRecv *recvCanData;

    int optTurnSignal = 0;
    bool blinkTurnSignal = true;
    int timing = 1;

    bool statHeadLight = false; // on or off
    bool optHeadLight = false; // auto or manual
    
    bool statAccel = false;
    bool statBrake = false;
    int acceltimer = 0;
    int braketimer = 0;

    int statSpeed = 0;

    int statScooter = 1;

    double statCdsSensor = 0;
    double statTemperature = 0;

    int year=2021, month=12, day=6, hour=3, min=32, sec=17;



    void putSystemMessage(QString);
    void refreshTime(void);

public slots:
    void slotTimerRtcState(void);
    void slotTimerLampDirectionState(void);
    void slotTimerHeadLightState(void);
    void slotTimerScooterStatus(void);
    void slotTimerAccelBrakeStatus(void);
    void slotTimerTemperatureStat(void);
    void slotTimerSpeedStat(void);
    void slotCdsStat(void);
    void slotRecvCanDataPacket(char*);
    void slotDebugMsgClear(void);


private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
