#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QTimer* monTimer;
    QTimer* inputTimer;
    QTimer* tmrAccelBrake;
    QTimer* tmrDirection;
    QTimer* tmrSpeed;
    QTimer* tmrHeadLightAuto;
    QTimer* tmrHeadLightControl;
    QTimer* cantype2;

    int G_1 = 1000;
    int G_2 = 1500;
    int G_3 = 2000;

    int G_interval[3] = {4,4,12};
    double rpminc = 0;
    int rpmval = 0;
    int Gear = 1;
    bool start = false;

    bool rtcsend = true;

    int HighTemp = 31;
    int LowTemp = 24;

    char SWLedRgb_Val = 0x7f;
    unsigned char m_SigValue;
    unsigned char m_SpeedValue;
    bool m_EmergencyValue;
    bool m_EmergencyLampValue;
    bool m_LampDirState;
    bool m_LampDirTimerState;
    bool m_AccelBrakeValue; // true : Accel , false : brake
    bool m_CanConnection = false;
    bool m_CanWrite = false;
    bool headLightOpt = false;
    bool head_light = false;
    unsigned char type1 = 1;
    unsigned char type2 = 0;
    unsigned char m_LampDirValue; // turn sig, warning
    int speed = 0;
    unsigned char status = 0;
    unsigned char light = 0;
    char temp_reg0;
    char temp_reg1;
    unsigned char m_RotaryLedValue;
    unsigned char m_GPIOLedValue; // accel, brake

    unsigned char cds_value;
    unsigned char c0, c1, c2, c4, c5, c6;

    unsigned char type3 = 2;

    double tempValue;
    double cds_ratio = 0;
    double threshold = 80;
    int timing =1;
    int Speed_int = 0;
    int AccelBreaktiming = 0;
    int m_CanFd;
    struct sockaddr_can m_CanAddr;
    struct can_frame m_CanSendFrame;
    struct ifreq m_Canifr;

    // I2C R/W
    int mI2cFd;
    int setI2cDevice(int *fd, int addr);
    int setI2cRegister(int *fd, unsigned char reg);
    int setI2cRegister(int *fd, unsigned char hb, unsigned char lb);

    // Temperature
    int getI2cTempRegValue(int *fd, size_t size);
    QString getI2cTemperatureValue(void);

    // SW4,5 & Speed LED
    int setI2cExpGpioSWLEDInit(int *fd);

    int setI2cExpGpioPushLed(int *fd);
    int setI2cExpSWRgbLed(int *fd);




    // Illumination
    unsigned char getI2cIlluminRegValue(int *fd);
    QString getI2cIlluminationValue(void);

    // Rotary Switch
    int parseCanPacketData(void);
    unsigned char getI2cExpGpioRotaryValue(int *fd);

    // GPIO LED

    void setGpioLedInitialization(void);

    void SWLedRgbCalc(void);

    // RTC
    void getRTCvalue(int *fd);


public slots:
    // can send
    void SensorCanSend(void);

    void slotBtnStartStopClicked(void);

    void setTimerLedControl(void);
    void setI2cExpGpioSpeedControl(void);
    void slotBtnEmergencyClicked(void);
    void slotBtnDirColorReset(void);

    void slotBtnDirLeftClicked(void);
    void slotBtnDirOffClicked(void);
    void slotBtnDirRightClicked(void);

    void slotTimeDisplay(void);

    void slotLampDirectionState(void);

    int setI2cExpGpioRotaryInit(int *fd);
    int setI2cExpGpioRotaryLed(int *fd);

    void slotBtnAccelPressed(void);
    void slotBtnBrakePressed(void);

    void slotBtnAccelReleased(void);
    void slotBtnBrakeReleased(void);

    int openCanDevice(void);
    void sendCanDataPacket(unsigned char id, unsigned char func, unsigned char value1, unsigned char value2,unsigned char value3,unsigned char value4,unsigned char value5,unsigned char value6,unsigned char value7);

    void slotInputDevState(void);
    void setGpioPushInitialization();

    void slotBtnHeadLightAutoClicked(void);
    void slotBtnHeadLightManualClicked(void);
    void slotHeadLightAuto(void);
    void slotBtnHeadLightOnClicked(void);
    void slotBtnHeadLightOffClicked(void);
    void slotHeadLightControl(void);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
