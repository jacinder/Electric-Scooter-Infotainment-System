#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "devices.h"
#include "cmd_type.h"

#include "gpios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <QDebug>

struct tm * g_SysTimeinfo;
time_t g_SysRawTime;;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int retv;

    monTimer = new QTimer(this);
    monTimer->start(1000);
    inputTimer = new QTimer(this); // Read from Dev
    inputTimer->start(500);
    tmrEmergency = new QTimer(this);
    tmrDirection = new QTimer(this);

    m_LampDirTimerState = false;
    m_EmergencyValue = false;
    m_EmergencyLampValue = false;
    m_LampDirState = false;

    qDebug("[System] GUI Initialization - OK.");


    mI2cFd = open(I2C_DEVICE, O_RDWR);
    if(mI2cFd<0) {
        qDebug("[System] I2C Line Opened.");
    }

    retv = openCanDevice();
    if(retv<0)
        qDebug("[System] CAN Device Failed!");


    // Init
    slotBtnLampOffClicked();
    slotBtnDirOffClicked();
    slotBtnEmergencyClicked();
    setGpioLedInitialization();
//    setGpioPushInitialization();
    setGpioLedControl(GP_LEDA, B_OFF);

//    setI2cExpGpioRotaryInit(&mI2cFd);




    connect(monTimer, SIGNAL(timeout()), SLOT(slotTimeDisplay()));
    connect(inputTimer, SIGNAL(timeout()), SLOT(slotInputDevState()));
    connect(tmrEmergency, SIGNAL(timeout()), SLOT(slotLampEmerState()));
    connect(tmrDirection, SIGNAL(timeout()), SLOT(slotLampDirectionState()));

    connect(ui->btnLampEmergency, SIGNAL(clicked()), SLOT(slotBtnEmergencyClicked(void)));

    connect(ui->btnLampLow, SIGNAL(clicked()), SLOT(slotBtnLampLowClicked()));
    connect(ui->btnLampFront, SIGNAL(clicked()), SLOT(slotBtnLampFrontClicked()));
    connect(ui->btnLampTail, SIGNAL(clicked()), SLOT(slotBtnLampTailClicked()));
    connect(ui->btnLampOff, SIGNAL(clicked()), SLOT(slotBtnLampOffClicked()));


    connect(ui->btnLampDirLeft, SIGNAL(clicked()), SLOT(slotBtnDirLeftClicked()));
    connect(ui->btnLampDirOff, SIGNAL(clicked()), SLOT(slotBtnDirOffClicked()));
    connect(ui->btnLampDirRight, SIGNAL(clicked()), SLOT(slotBtnDirRightClicked()));




}

// Warning
void MainWindow::slotLampEmerState(void)
{
    if(m_EmergencyLampValue) {
        ui->lbStateEmergency->setStyleSheet("QLabel { background-color : orange; border-image: url(:/images/system/warning.png);}");
        setGpioLedControl(GP_LED1, B_ON);
        setGpioLedControl(GP_LED2, B_ON);
        m_EmergencyLampValue=false;
    } else {
        ui->lbStateEmergency->setStyleSheet("QLabel { background-color : rgb(240,240,240); border-image: url(:/images/system/warning.png);}");
        setGpioLedControl(GP_LED1, B_OFF);
        setGpioLedControl(GP_LED2, B_OFF);
        m_EmergencyLampValue=true;
    }
}

void MainWindow::slotBtnEmergencyClicked(void)
{

    if(m_EmergencyValue) {
        m_EmergencyValue = false;
        ui->btnLampEmergency->setStyleSheet("QPushButton { background-color : red; border-image: url(:/images/system/warning.png);}");
        // void sendCanDataPacket(unsigned char id, unsigned char devid, unsigned char func, unsigned char value);
        sendCanDataPacket(0x20, 0x01, 0x30, 0x01,0x01);
        tmrEmergency->start(1000);

    } else {
        m_EmergencyValue = true;
        ui->btnLampEmergency->setStyleSheet("QPushButton { background-color : rgb(240,240,240); border-image: url(:/images/system/warning.png);}");
        ui->lbStateEmergency->setStyleSheet("QLabel { background-color : rgb(240,240,240);  border-image: url(:/images/system/warning.png);}");
        sendCanDataPacket(0x20, 0x01, 0x30, 0x00,0x01);
        setGpioLedControl(GP_LED1, B_OFF);
        setGpioLedControl(GP_LED2, B_OFF);
        tmrEmergency->stop();
    }
}

// Turn sig
void MainWindow::slotBtnDirColorReset(void)
{
    ui->btnLampDirOff->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampDirLeft->setStyleSheet("QPushButton { background-color : rgb(240,240,240); border-image: url(:/images/system/left.png);}");
    ui->btnLampDirRight->setStyleSheet("QPushButton { background-color : rgb(240,240,240); border-image: url(:/images/system/right.png);}");
}

void MainWindow::slotLampDirectionState(void)
{
    // Direction LED
    if(m_LampDirState)
        m_LampDirState=false;
    else
        m_LampDirState=true;

    switch(m_LampDirValue)
    {
        case 0:
           if(m_LampDirState) {
               setGpioLedControl(GP_LED0, B_ON);
               setGpioLedControl(GP_LED3, B_OFF);
           } else {
               setGpioLedControl(GP_LED0, B_OFF);
               setGpioLedControl(GP_LED3, B_OFF);
           }
           break;
        case 1:
           setGpioLedControl(GP_LED0, B_OFF);
           setGpioLedControl(GP_LED3, B_OFF);
           break;
        case 2:
           if(m_LampDirState) {
               setGpioLedControl(GP_LED0, B_OFF);
               setGpioLedControl(GP_LED3, B_ON);
           } else {
               setGpioLedControl(GP_LED0, B_OFF);
               setGpioLedControl(GP_LED3, B_OFF);
           }
           break;
        default:
           break;
    }
}

void MainWindow::slotBtnDirOffClicked()
{
    slotBtnDirColorReset();
    ui->btnLampDirOff->setStyleSheet("QPushButton { background-color : red; }");
    m_LampDirValue = 0x01;
}

void MainWindow::slotBtnDirLeftClicked()
{
    slotBtnDirColorReset();
    ui->btnLampDirLeft->setStyleSheet("QPushButton { background-color : red; border-image: url(:/images/system/left.png);}");
    m_LampDirValue = 0x00;
    slotLampDirectionState();
}

void MainWindow::slotBtnDirRightClicked()
{
    slotBtnDirColorReset();
    ui->btnLampDirRight->setStyleSheet("QPushButton { background-color : red; border-image: url(:/images/system/right.png);}");
    m_LampDirValue = 0x02;
}

// Light
void MainWindow::slotBtnLampColorReset(void)
{
    ui->btnLampLow->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampFront->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampTail->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
    ui->btnLampOff->setStyleSheet("QPushButton { background-color : rgb(240,240,240); }");
}

void MainWindow::slotBtnLampLowClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampLow->setStyleSheet("QPushButton { background-color : red; }");
    m_LampValue = 0x03;
}

void MainWindow::slotBtnLampFrontClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampFront->setStyleSheet("QPushButton { background-color : red; }");
    m_LampValue = 0x02;
}

void MainWindow::slotBtnLampTailClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampTail->setStyleSheet("QPushButton { background-color : red; }");
    m_LampValue = 0x01;
}

void MainWindow::slotBtnLampOffClicked(void)
{
    slotBtnLampColorReset();
    ui->btnLampOff->setStyleSheet("QPushButton { background-color : red; }");
    m_LampValue = 0x00;
}

// Can Connection
int MainWindow::openCanDevice(void)
{
    if ((m_CanFd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) == -1)
    {
        qDebug("CAN Device Open Failed");
        return -1;
    } else {
        qDebug("CAN Device Opened!");
    }

    strcpy(m_Canifr.ifr_name, CAN_DEVICE);
    ioctl(m_CanFd, SIOCGIFINDEX, &m_Canifr);

    m_CanAddr.can_family  = AF_CAN;
    m_CanAddr.can_ifindex = m_Canifr.ifr_ifindex;


    if (bind(m_CanFd, (struct sockaddr *)&m_CanAddr, sizeof(m_CanAddr)) == -1) {
        qDebug("CAN Binding Error");
        return -1;
    }

    return 0;
}

void MainWindow::sendCanDataPacket(unsigned char id, unsigned char type, unsigned char func, unsigned char value1, unsigned char value2)
{
    int nbytes;

    m_CanSendFrame.can_id  = id;
    m_CanSendFrame.can_dlc = CAN_DEF_PACKET_SIZE;
    memset(m_CanSendFrame.data,0,sizeof(m_CanSendFrame.data));
    m_CanSendFrame.data[TICK] = 0; // Not used
    m_CanSendFrame.data[TYPE] = type;
    m_CanSendFrame.data[FUNC] = func;
    m_CanSendFrame.data[VAL1] = value1;
    m_CanSendFrame.data[VAL2] = value2;

    nbytes = write(m_CanFd, &m_CanSendFrame, sizeof(struct can_frame));
    if(nbytes<1)
    {
       qDebug("Can data send failed!");
    }
}


// I2C Set
void MainWindow::slotInputDevState(void)
{
    // Push Button


    // Rotary Button


    // Push Button Switch
}

void MainWindow::setGpioLedInitialization(void)
{
    system("echo 6 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 16 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 17 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 18 > /sys/class/gpio/export");
    usleep(1000);

    system("echo out > /sys/class/gpio/gpio6/direction");
    usleep(1000);
    system("echo out > /sys/class/gpio/gpio16/direction");
    usleep(1000);
    system("echo out > /sys/class/gpio/gpio17/direction");
    usleep(1000);
    system("echo out > /sys/class/gpio/gpio18/direction");
}

void MainWindow::setGpioLedControl(unsigned char led, bool state)
{
    if(state) // Off
    {
        switch(led)
        {
        case 0x00:
            system("echo 1 > /sys/class/gpio/gpio6/value");

            break;
        case 0x01:
            system("echo 1 > /sys/class/gpio/gpio16/value");

            break;
        case 0x02:
            system("echo 1 > /sys/class/gpio/gpio17/value");

            break;
        case 0x03:
            system("echo 1 > /sys/class/gpio/gpio18/value");

            break;
        case 0x04:
            system("echo 1 > /sys/class/gpio/gpio6/value");
            system("echo 1 > /sys/class/gpio/gpio16/value");
            system("echo 1 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");

            break;
        default :
            break;
        }

    } else { // On
        switch(led)
        {
        case 0x00:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            break;
        case 0x01:
            system("echo 0 > /sys/class/gpio/gpio16/value");
            break;
        case 0x02:
            system("echo 0 > /sys/class/gpio/gpio17/value");
            break;
        case 0x03:
            system("echo 0 > /sys/class/gpio/gpio18/value");
            break;
        case 0x04:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 0 > /sys/class/gpio/gpio17/value");
            system("echo 0 > /sys/class/gpio/gpio18/value");
            break;
        default :
            break;
        }

    }
}

int MainWindow::setI2cDevice(int *fd, int addr)
{
    return ioctl(*fd, I2C_SLAVE, addr);
}

int MainWindow::setI2cRegister(int *fd, unsigned char reg)
{
    return write(*fd, &reg,sizeof(reg));
}

int MainWindow::setI2cRegister(int *fd, unsigned char hb, unsigned char lb)
{
    unsigned char regs[2]={0,};
    regs[0] = hb;
    regs[1] = lb;
    write(*fd, regs,sizeof(regs));
}


// I2C Get

// Temp
int MainWindow::getI2cTempRegValue(int *fd, double* value,  size_t size)
{
    int retv;
    char regv[2]={0,0};
    retv = read(*fd, regv, size);

    *value = (((regv[0]<<8) | regv[1])>>4) * 0.0625;

    return retv;
}

QString MainWindow::getI2cTemperatureValue(void)
{
    QString strValue;
    double tempValue;
    unsigned char regs[2]={0x00,0x00};
    // Get temperature data
    setI2cDevice(&mI2cFd, TEMP_ADDR);
    setI2cRegister(&mI2cFd,regs[0],regs[1]);
    getI2cTempRegValue(&mI2cFd, &tempValue, 2);
    return strValue.sprintf("%2.1f C",tempValue);
}

// ADC
// ADC로부터 조도 값을 가져온다.
unsigned char MainWindow::getI2cIlluminRegValue(int *fd)
{
    double temp;

    unsigned char regv;
    read(*fd, &regv, sizeof(regv));

    temp = ((255-regv)/255.0)*100;  // 백분율 계산

    return (unsigned char)temp;
}
// 조도값을 화면에 출력한다
QString MainWindow::getI2cIlluminationValue(void)
{
    int retv;
    QString strValue;

    unsigned char reg = 0;  // Chnnel selected - 0

    setI2cDevice(&mI2cFd, ADC_CDS);  // 0x4C
    retv=setI2cRegister(&mI2cFd, reg);
    if(retv!=1)
        qDebug("[System] ADC error!.");
    retv=getI2cIlluminRegValue(&mI2cFd);

    return strValue.sprintf("%2d %%",retv);
}

// Time
void MainWindow::slotTimeDisplay(void)
{


    ui->lbDispLabelTop->setText(getI2cTemperatureValue());
    ui->lbDispLabelMiddle->setText(getI2cIlluminationValue());

    refreshTime();
}

void MainWindow::refreshTime()
{
    char strTempArray[30];

    time(&g_SysRawTime);
    g_SysTimeinfo = localtime(&g_SysRawTime);
    sprintf(strTempArray,"%04d/%02d/%02d", g_SysTimeinfo->tm_year+1900,g_SysTimeinfo->tm_mon,g_SysTimeinfo->tm_mday);
    ui->lbDispDate->setText(strTempArray);
    sprintf(strTempArray,"%02d:%02d:%02d",g_SysTimeinfo->tm_hour,g_SysTimeinfo->tm_min,g_SysTimeinfo->tm_sec);
    ui->lbDispTime->setText(strTempArray);
}

MainWindow::~MainWindow()
{
    /* Implement a destructor. */

    // GPIO LED OFF
    system("echo 1 > /sys/class/gpio/gpio6/value");
    system("echo 1 > /sys/class/gpio/gpio16/value");
    system("echo 1 > /sys/class/gpio/gpio17/value");
    system("echo 1 > /sys/class/gpio/gpio18/value");

    // Device close
    system("echo 6 > /sys/class/gpio/unexport");
    system("echo 16 > /sys/class/gpio/unexport");
    system("echo 17 > /sys/class/gpio/unexport");
    system("echo 18 > /sys/class/gpio/unexport");

    ::close(mI2cFd);

    delete ui;
}
