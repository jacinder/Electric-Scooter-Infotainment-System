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
    tmrAccelBrake = new QTimer(this);
    tmrDirection = new QTimer(this);
    tmrDirection->start(1000);
    tmrSpeed = new QTimer(this);
    tmrSpeed->start(1000);
    tmrHeadLightAuto = new QTimer(this);
    tmrHeadLightAuto -> start(1000);
    tmrHeadLightControl = new QTimer(this);
    tmrHeadLightControl -> start(500);

    cantype2 = new QTimer(this);
    cantype2 -> start(5000);

    connect(cantype2, SIGNAL(timeout()), SLOT(SensorCanSend()));

    m_EmergencyValue = true;
    m_AccelBrakeValue = true;
    speed = 0;
    m_GPIOLedValue = 0;

    qDebug("[System] GUI Initialization - OK.");


    mI2cFd = open(I2C_DEVICE, O_RDWR);
    if(mI2cFd<0) {
        qDebug("[System] I2C Line Opened.");
    }

    retv = openCanDevice();
    if(retv<0)
        qDebug("[System] CAN Device Failed!");


    // Init
    start = false;
    status = 0x01;
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    ui->lcdDigitNumber->display(0);
    ui->btnStart->setStyleSheet("QPushButton { border-image : url(:/images/system/state_critical.png); font:20px;}");
    ui->btnStart->setText("Start/Stop");
    ui->btnAccel->setStyleSheet("border-image:url(:/images/system/state_disable.png); ");
    ui->btnAccel->setEnabled(false);
    ui->btnBrake->setStyleSheet("border-image:url(:/images/system/state_disable.png); ");
    ui->btnBrake->setEnabled(false);
    ui->lbBattery->setStyleSheet("background : transparent; border-image:url(:/images/system/fire.png);");
    ui->lbTemp->setStyleSheet("background : transparent; border-image:url(:/images/system/ice.png);");

    slotBtnDirOffClicked();
    setGpioLedInitialization();
    setGpioPushInitialization();
    setI2cExpGpioSWLEDInit(&mI2cFd);
    setI2cExpGpioRotaryInit(&mI2cFd);
    SWLedRgbCalc();

    connect(ui->btnStart, SIGNAL(clicked()), SLOT(slotBtnStartStopClicked()));

    connect(monTimer, SIGNAL(timeout()), SLOT(slotTimeDisplay()));
    connect(inputTimer, SIGNAL(timeout()), SLOT(slotInputDevState()));
    connect(tmrDirection, SIGNAL(timeout()), SLOT(slotLampDirectionState()));
    connect(tmrAccelBrake, SIGNAL(timeout()), SLOT(setTimerLedControl()));
    connect(ui->btnLampEmergency, SIGNAL(clicked()), SLOT(slotBtnEmergencyClicked(void)));

    // Accel Brake
    connect(ui->btnAccel, SIGNAL(pressed()), SLOT(slotBtnAccelPressed(void)));
    connect(ui->btnAccel, SIGNAL(released()), SLOT(slotBtnAccelReleased(void)));
    connect(ui->btnBrake, SIGNAL(pressed()), SLOT(slotBtnBrakePressed(void)));
    connect(ui->btnBrake, SIGNAL(released()), SLOT(slotBtnBrakeReleased(void)));

    connect(ui->btnLampDirLeft, SIGNAL(clicked()), SLOT(slotBtnDirLeftClicked()));
    connect(ui->btnLampDirOff, SIGNAL(clicked()), SLOT(slotBtnDirOffClicked()));
    connect(ui->btnLampDirRight, SIGNAL(clicked()), SLOT(slotBtnDirRightClicked()));

    // Light
    connect(tmrHeadLightAuto, SIGNAL(timeout()), SLOT(slotHeadLightAuto(void)));
    connect(ui->btnLampAuto, SIGNAL(clicked()), SLOT(slotBtnHeadLightAutoClicked()));
    connect(ui->btnLampManual, SIGNAL(clicked()), SLOT(slotBtnHeadLightManualClicked()));
    connect(ui->btnLampOn, SIGNAL(clicked()), SLOT(slotBtnHeadLightOnClicked()));
    connect(ui->btnLampOff, SIGNAL(clicked()), SLOT(slotBtnHeadLightOffClicked()));
    connect(tmrHeadLightControl, SIGNAL(timeout()), SLOT(slotHeadLightControl(void)));


}




// sensor can
void MainWindow::SensorCanSend(){
    sendCanDataPacket(0x20, type3, cds_value, temp_reg0 , temp_reg1, 0, 0, 0, 0);
}

void MainWindow::SWLedRgbCalc(){
    int RGB_val, Light_val, Can_val;
    if(tempValue >= HighTemp){
        RGB_val = FIRE;
        if(status != 0x00){
            status = 0x00;
            sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        }
        status = 0x00;
        ui->lbStatus->setStyleSheet("background:red; font:30px; color: white;");
        ui->lbStatus->setText("FIRE");
    } else if(tempValue < LowTemp){
        RGB_val = FREZZING;
        if(status != 0x03){
            status = 0x03;
            sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        }
        status = 0x03;
        ui->lbStatus->setStyleSheet("background:blue; font:30px; color: white;");
        ui->lbStatus->setText("FREZZING");
    } else if(start) {
        RGB_val = START;
        if(status != 0x02){
            status = 0x02;
            sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        }
        status = 0x02;
        ui->lbStatus->setStyleSheet("background:green;font:30px; color: white;");
        ui->lbStatus->setText("Power On");
    } else {
        RGB_val = OFF;
        if(status != 0x01){
            status = 0x01;
            sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        }
        status = 0x01;
        ui->lbStatus->setStyleSheet("background:orange;font:30px; color: white;");
        ui->lbStatus->setText("Power Off");

    }

    if(head_light){
        Light_val = LIGHT_ON;
    } else {
        Light_val = LIGHT_OFF;
    }

    if(m_CanConnection && m_CanWrite) {
        Can_val = CAN_OK;
        ui->lbCan->setStyleSheet("color : white; background: limegreen;");
        ui->lbCan->setText("CAN CONNECTED");
        qDebug("canc : %d",m_CanConnection);
        qDebug("\ncanw : %d",m_CanWrite);
    } else if(m_CanConnection  && !m_CanWrite){
        qDebug("canc : %d",m_CanConnection);
        qDebug("\ncanw : %d",m_CanWrite);
        system("sudo ip link set can0 down");
        system("sudo ip link set can0 up type can bitrate 125000");
        Can_val = CAN_ERR;
        ui->lbCan->setStyleSheet("color : red; background: transparent;");
        ui->lbCan->setText("CAN NO SIGNAL");
    } else{
        qDebug("canc : %d",m_CanConnection);
        qDebug("\ncanw : %d",m_CanWrite);
        Can_val = CAN_ERR;
        ui->lbCan->setStyleSheet("color : red; background: transparent;");
        ui->lbCan->setText("CAN NO SIGNAL");
        openCanDevice();
    }

    SWLedRgb_Val = (char)((RGB_val << 4) | (Light_val << 2) | (Can_val<<3));
    setI2cExpSWRgbLed(&mI2cFd);
    // qDebug("SWLedRgb_val = %d\n", (int)SWLedRgb_Val);
}

void MainWindow::slotBtnStartStopClicked(void){
    if(!start){
        start = true;
        status = 0x02;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        ui->btnStart->setStyleSheet("QPushButton {border-image : url(:/images/system/state_good.png);font:20px;}");
        ui->btnStart->setText("Start/Stop");
        ui->btnAccel->setStyleSheet("background-color: rgb(215,215,158); border-image:url(:/images/system/up.png); ");
        ui->btnAccel->setEnabled(true);
        ui->btnBrake->setStyleSheet("background-color:rgb(215,215,158); border-image:url(:/images/system/down.png); ");
        ui->btnBrake->setEnabled(true);
        slotBtnDirOffClicked();
        setGpioLedInitialization();
        setGpioPushInitialization();
        setI2cExpGpioSWLEDInit(&mI2cFd);
        setI2cExpGpioRotaryInit(&mI2cFd);
    } else {
        start = false;
        rtcsend = true;
        status = 0x01;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        speed=0;
        ui->lcdDigitNumber->display(speed);
        ui->btnStart->setStyleSheet("QPushButton { border-image : url(:/images/system/state_critical.png);font:20px;}");
        ui->btnStart->setText("Start/Stop");
        ui->btnAccel->setStyleSheet("border-image:url(:/images/system/state_disable.png); ");
        ui->btnAccel->setEnabled(false);
        ui->btnBrake->setStyleSheet("border-image:url(:/images/system/state_disable.png); ");
        ui->btnBrake->setEnabled(false);
        slotBtnDirOffClicked();
        setGpioLedInitialization();
        setGpioPushInitialization();
        setI2cExpGpioSWLEDInit(&mI2cFd);
        setI2cExpGpioRotaryInit(&mI2cFd);
    }
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
}

void MainWindow::slotInputDevState(void)
{
    // Push Button


    // Rotary Button


    // Push Button Switch
}

// Init
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

    system("echo 1 > /sys/class/gpio/gpio6/value");
    system("echo 1 > /sys/class/gpio/gpio16/value");
    system("echo 1 > /sys/class/gpio/gpio17/value");
    system("echo 1 > /sys/class/gpio/gpio18/value");
}

void MainWindow::setGpioPushInitialization(void)
{
    system("echo 0 > /sys/class/gpio/export");
    usleep(1000);
    system("echo 5 > /sys/class/gpio/export");
    usleep(1000);

    system("echo in > /sys/class/gpio/gpio0/direction");
    usleep(1000);
    system("echo in > /sys/class/gpio/gpio5/direction");
    usleep(1000);
}


// I2C
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


// TEMP
int MainWindow::getI2cTempRegValue(int *fd, size_t size)
{
    int retv;
    char regv[2]={0,0};
    retv = read(*fd, regv, size);
    temp_reg0 = regv[0];
    temp_reg1 = regv[1];
    tempValue = (((regv[0]<<8) | regv[1])>>4) * 0.0625;

    SWLedRgbCalc();


    return retv;
}

QString MainWindow::getI2cTemperatureValue(void)
{
    QString strValue;
    unsigned char regs[2]={0x00,0x00};
    // Get temperature data
    setI2cDevice(&mI2cFd, TEMP_ADDR);
    setI2cRegister(&mI2cFd,regs[0],regs[1]);
    getI2cTempRegValue(&mI2cFd, 2);
    if(tempValue >= HighTemp){
        ui->lbBattery->setStyleSheet("background-color : red; border-image:url(:/images/system/fire.png);");
        ui->lbTemp->setStyleSheet("background : transparent; border-image:url(:/images/system/ice.png);");
    } else if(tempValue < HighTemp && tempValue > LowTemp){
        ui->lbBattery->setStyleSheet("background : transparent; border-image:url(:/images/system/fire.png);");
        ui->lbTemp->setStyleSheet("background : transparent; border-image:url(:/images/system/ice.png);");
    } else {
        ui->lbBattery->setStyleSheet("background : transparent; border-image:url(:/images/system/fire.png);");
        ui->lbTemp->setStyleSheet("background-color : blue; border-image:url(:/images/system/ice.png);");
    }
    return strValue.sprintf("%2.1f C",tempValue);
}


// SW4,5 & Speed LED
int MainWindow::setI2cExpGpioSWLEDInit(int *fd)
{
    int retv;
    unsigned char regs[2]={0,};

    retv = ioctl(*fd, I2C_SLAVE, LEDSW_ADDR);
    usleep(1000);

    regs[0]=0x07;
    regs[1]=0x00; // I2C Ch1 - Output
    write(*fd, regs, sizeof(regs));
    usleep(1000);

    // 8LED
    regs[0]=0x03; // Output Register for Ch1
    regs[1]=0x00; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));

    regs[0]=0x03; // Output Register for Ch1
    regs[1]=0xff; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));

    // SW
    regs[0]=0x06; // Output Register for Ch0
    regs[1]=0x00; // sw & rgbled
    write(*fd, regs, sizeof(regs));

    regs[0]=0x02;
    regs[1]=0xff; // all off
    write(*fd, regs, sizeof(regs));
}

void MainWindow::setI2cExpGpioSpeedControl(void){
    // qDebug("speed : %d\n", speed);
    switch (speed/3)
    {
    case 0:
        m_SpeedValue = 0xFF;
        break;
    case 1:
        m_SpeedValue = 0xFE;
        break;
    case 2:
        m_SpeedValue = 0xFC;
        break;
    case 3:
        m_SpeedValue = 0xF8;
        break;
    case 4:
        m_SpeedValue = 0xF0;
        break;
    case 5:
        m_SpeedValue = 0xE0;
        break;
    case 6:
        m_SpeedValue = 0xC0;
        break;
    case 7:
        m_SpeedValue = 0x80;
        break;
    case 8:
        m_SpeedValue = 0x00;
        break;
    default:
        break;
    }
    setI2cExpGpioPushLed(&mI2cFd);
}

int MainWindow::setI2cExpGpioPushLed(int *fd)
{

    int retv;
    unsigned char regs[2]={0,};

    retv = ioctl(*fd, I2C_SLAVE, LEDSW_ADDR);
    usleep(1000);

    regs[0]=0x03; // Output Register for Ch1
    regs[1]=m_SpeedValue; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));
    usleep(1000);

}

int MainWindow::setI2cExpSWRgbLed(int *fd)
{

    int retv;
    unsigned char regs[2]={0,};

    retv = ioctl(*fd, I2C_SLAVE, LEDSW_ADDR);
    usleep(1000);

    regs[0]=0x02; // Output Register for Ch1
    regs[1]=SWLedRgb_Val; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));

}

// Light
// headLightOpt : true 자동 / false 수동
void MainWindow::slotBtnHeadLightAutoClicked(void){
    headLightOpt = true;
    if(head_light){
        light = 0x03;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    } else {
        light = 0x02;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    }
    ui->btnLampAuto->setStyleSheet("QPushButton{ background-color:orange;}");
    ui->btnLampManual->setStyleSheet("QPushButton{ background-color:rgb(40,40,40) ;}");
}
void MainWindow::slotBtnHeadLightManualClicked(void){
    headLightOpt = false;
    if(head_light){
        light = 0x01;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    } else {
        light = 0x00;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    }
    ui->btnLampManual->setStyleSheet("QPushButton{ background-color:orange;}");
    ui->btnLampAuto->setStyleSheet("QPushButton{ background-color:rgb(40,40,40); color:white;}");

}
void MainWindow::slotHeadLightAuto(void) // 오토라이트라면
{
    if(headLightOpt){ // 자동으로 지정되어있을 때만
        if(cds_ratio < threshold){ // 특정값 이하라면
            if(!head_light){
                light = 0x03;
                SWLedRgbCalc();
                sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
            }
            head_light = true;
        }
        else{
            if(head_light){
                light = 0x02;
                SWLedRgbCalc();
                sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
            }
            head_light = false;
        }
    }
}
//수동라이트라면
void MainWindow::slotBtnHeadLightOnClicked(void){
    if(!headLightOpt){ // 수동으로 지정이 되어있을때만
        head_light = true;
        light = 0x01;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        SWLedRgbCalc();
    }
}
void MainWindow::slotBtnHeadLightOffClicked(void){
    if(!headLightOpt){ // 수동으로 지정이 되어있을때만
        head_light = false;
        light = 0x00;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
        SWLedRgbCalc();
    }
}
void MainWindow::slotHeadLightControl(void){
    if(head_light){
        //ui 제어
        ui->lbLightInd->setStyleSheet("background-color:lightblue;border-image: url(:/images/system/light.png);");
        ui->btnLampOn->setStyleSheet("QPushButton{ background-color:orange;}");
        ui->btnLampOff->setStyleSheet("QPushButton{ background-color:rgb(40,40,40); color:white;}");

        //led 제어

    }else{
        //ui 제어
        ui->lbLightInd->setStyleSheet("background:transparent;border-image: url(:/images/system/light.png);");
        ui->btnLampOn->setStyleSheet("QPushButton{ background-color:rgb(40,40,40); color:white}");
        ui->btnLampOff->setStyleSheet("QPushButton{ background-color:orange;}");
    }
}


//ADC
// ADC로부터 조도 값을 가져온다.
unsigned char MainWindow::getI2cIlluminRegValue(int *fd)
{

    unsigned char regv;
    read(*fd, &regv, sizeof(regv));
    cds_value = regv;
    cds_ratio = ((255-regv)/255.0)*100;  // 백분율 계산

    // sendCanDataPacket(0x20, type2, cds_value , c6, c5,c4, c2, c1, c0);

    return (unsigned char)cds_ratio;
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

    return strValue.sprintf("Illum : %2d %%",retv);
}



// ROTARY
int MainWindow::setI2cExpGpioRotaryInit(int *fd)
{

    int retv;
    unsigned char regs[2]={0,};

    retv = ioctl(*fd, I2C_SLAVE, MOD_ROTARY);
    usleep(1000);

    regs[0]=0x07;
    regs[1]=0x00; // I2C Ch1 - Output
    write(*fd, regs, sizeof(regs));
    usleep(1000);

    regs[0]=0x03; // Output Register for Ch1
    regs[1]=0x00; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));
    usleep(1000);

}

int MainWindow::setI2cExpGpioRotaryLed(int *fd)
{

    int retv;
    unsigned char regs[2]={0,};

    retv = ioctl(*fd, I2C_SLAVE, MOD_ROTARY);
    usleep(1000);

    regs[0]=0x03; // Output Register for Ch1
    regs[1]=m_RotaryLedValue; // 8Bit(LED0~LED8);
    write(*fd, regs, sizeof(regs));
    usleep(1000);


}

unsigned char MainWindow::getI2cExpGpioRotaryValue(int *fd)
{

    int retv;
    unsigned char reg;

    retv = ioctl(*fd, I2C_SLAVE, MOD_ROTARY);
    usleep(1000);

    reg=0x00; // Output Register for Ch1

    read(*fd, &reg, sizeof(reg));

    return reg;

}



// Can
int MainWindow::openCanDevice(void)
{
    if ((m_CanFd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) == -1)
    {
        qDebug("CAN Device Open Failed");
        m_CanConnection = false;
        return -1;
    } else {
        qDebug("CAN Device Opened!");
        m_CanConnection = true;
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

void MainWindow::sendCanDataPacket(unsigned char id, unsigned char func, unsigned char value1, unsigned char value2,unsigned char value3,unsigned char value4,unsigned char value5,unsigned char value6,unsigned char value7)
{
    int nbytes;

    m_CanSendFrame.can_id  = id;
    m_CanSendFrame.can_dlc = CAN_DEF_PACKET_SIZE;
    memset(m_CanSendFrame.data,0,sizeof(m_CanSendFrame.data));
    m_CanSendFrame.data[FUNC] = func;
    m_CanSendFrame.data[VAL1] = value1;
    m_CanSendFrame.data[VAL2] = value2;
    m_CanSendFrame.data[VAL3] = value3;
    m_CanSendFrame.data[VAL4] = value4;
    m_CanSendFrame.data[VAL5] = value5;
    m_CanSendFrame.data[VAL6] = value6;
    m_CanSendFrame.data[VAL7] = value7;

    nbytes = write(m_CanFd, &m_CanSendFrame, sizeof(struct can_frame));
    if(nbytes<1)
    {
       m_CanWrite = false;
       qDebug("Can data send failed!");
    } else {
        m_CanWrite = true;
    }
}





// RPM
void MainWindow::slotBtnAccelPressed(void){
    ui->btnAccel->setStyleSheet("background-color:orange; border-image:url(:/images/system/up.png); ");
    m_AccelBrakeValue = true;
    AccelBreaktiming = 5;
    if(Gear == 1){
        tmrAccelBrake->start(G_1);
    } else if(Gear == 2){
        tmrAccelBrake->start(G_2);
    }

}

void MainWindow::slotBtnBrakePressed(void){
    ui->btnBrake->setStyleSheet("background-color:orange; border-image:url(:/images/system/down.png); ");
    m_AccelBrakeValue = false;
    AccelBreaktiming = 1;
    tmrAccelBrake->start(1000);
}

void MainWindow::slotBtnAccelReleased(void){
    ui->btnAccel->setStyleSheet("background: transparent; border-image:url(:/images/system/up.png); ");
    tmrAccelBrake->stop();

    m_GPIOLedValue = 0;
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    system("echo 1 > /sys/class/gpio/gpio6/value");
    system("echo 1 > /sys/class/gpio/gpio16/value");
    system("echo 1 > /sys/class/gpio/gpio17/value");
    system("echo 1 > /sys/class/gpio/gpio18/value");
}

void MainWindow::slotBtnBrakeReleased(void){
    ui->btnBrake->setStyleSheet("background:transparent; border-image:url(:/images/system/down.png); ");
    tmrAccelBrake->stop();
    m_GPIOLedValue = 0;
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    system("echo 1 > /sys/class/gpio/gpio6/value");
    system("echo 1 > /sys/class/gpio/gpio16/value");
    system("echo 1 > /sys/class/gpio/gpio17/value");
    system("echo 1 > /sys/class/gpio/gpio18/value");
}

void MainWindow::setTimerLedControl(void)
{
    // qDebug("m_GPIOLEDVALUE : %c\n", m_GPIOLedValue);
    if(m_AccelBrakeValue){ // Accel
        // qDebug("Accel Pressed\n");
        if(speed <= 12){
            Gear = 1;
            ui->lcdGear->display(Gear);
        } else if(speed < 24){
            Gear = 2;
            ui->lcdGear->display(Gear);
        }

        switch(AccelBreaktiming)
        {
        case 0x05:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 1 > /sys/class/gpio/gpio16/value");
            system("echo 1 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");
            m_GPIOLedValue = AccelBreaktiming;
            if(speed < 22){
                speed += 3;
            }
            AccelBreaktiming++;
            break;

        case 0x06:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 1 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");
            m_GPIOLedValue = AccelBreaktiming;
            if(speed < 22){
                speed += 3;
            }
            AccelBreaktiming++;
            break;

        case 0x07:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 0 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");
            m_GPIOLedValue = AccelBreaktiming;
            if(speed < 22){
                speed += 3;
            }
            AccelBreaktiming++;
            break;

        case 0x08:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 0 > /sys/class/gpio/gpio17/value");
            system("echo 0 > /sys/class/gpio/gpio18/value");
            m_GPIOLedValue = AccelBreaktiming;
            if(speed < 22){
                speed += 3;
            }
            AccelBreaktiming++;
            break;
        case 0x09:
                system("echo 1 > /sys/class/gpio/gpio6/value");
                system("echo 1 > /sys/class/gpio/gpio16/value");
                system("echo 1 > /sys/class/gpio/gpio17/value");
                system("echo 1 > /sys/class/gpio/gpio18/value");

                AccelBreaktiming++;

            break;
        case 0x0a:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 0 > /sys/class/gpio/gpio17/value");
            system("echo 0 > /sys/class/gpio/gpio18/value");
            AccelBreaktiming++;

            break;
        case 0x0b:
            system("echo 1 > /sys/class/gpio/gpio6/value");
            system("echo 1 > /sys/class/gpio/gpio16/value");
            system("echo 1 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");
            AccelBreaktiming++;

            break;
        default :
            if(Gear == 1){
                Gear++;
                slotBtnAccelPressed();
            } else {
                if(AccelBreaktiming%2==0){
                    system("echo 0 > /sys/class/gpio/gpio6/value");
                    system("echo 0 > /sys/class/gpio/gpio16/value");
                    system("echo 0 > /sys/class/gpio/gpio17/value");
                    system("echo 0 > /sys/class/gpio/gpio18/value");
                    AccelBreaktiming++;
                }else {
                    system("echo 1 > /sys/class/gpio/gpio6/value");
                    system("echo 1 > /sys/class/gpio/gpio16/value");
                    system("echo 1 > /sys/class/gpio/gpio17/value");
                    system("echo 1 > /sys/class/gpio/gpio18/value");
                    AccelBreaktiming++;
                }
            }
            break;
        }
    } else { // Brake
        // qDebug("Brake Pressed\n");
        if(speed < 12){
            Gear = 1;
            ui->lcdGear->display(Gear);
        } else if(speed < 24){
            Gear = 2;
            ui->lcdGear->display(Gear);
        }
        switch(AccelBreaktiming)
        {
        case 0x01:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 1 > /sys/class/gpio/gpio16/value");
            system("echo 1 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");
            m_GPIOLedValue = AccelBreaktiming;
            if(speed > 2){
                speed -= 3;
            }
            AccelBreaktiming++;
            break;

        case 0x02:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 1 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");
            m_GPIOLedValue = AccelBreaktiming;
            if(speed > 2){
                speed -= 3;
            }
            AccelBreaktiming++;
            break;

        case 0x03:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 0 > /sys/class/gpio/gpio17/value");
            system("echo 1 > /sys/class/gpio/gpio18/value");            m_GPIOLedValue = AccelBreaktiming;
            m_GPIOLedValue = AccelBreaktiming;
            if(speed > 2){
                speed -= 3;
            }
            AccelBreaktiming++;
            break;

        case 0x04:
            system("echo 0 > /sys/class/gpio/gpio6/value");
            system("echo 0 > /sys/class/gpio/gpio16/value");
            system("echo 0 > /sys/class/gpio/gpio17/value");
            system("echo 0 > /sys/class/gpio/gpio18/value");            m_GPIOLedValue = AccelBreaktiming;
            m_GPIOLedValue = AccelBreaktiming;
            if(speed > 2){
                speed -= 3;
            }
            AccelBreaktiming=1;
            break;

        default :
            break;
        }
    }

    ui->lcdDigitNumber->display(speed);
    setI2cExpGpioSpeedControl();
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
}






// Warning
void MainWindow::slotBtnEmergencyClicked(void)
{
    slotBtnDirColorReset();
    if(m_EmergencyValue) {
        m_EmergencyValue = false;
        ui->btnLampEmergency->setStyleSheet("QPushButton { background-color : red; border-image: url(:/images/warning.png);}");
        if(m_LampDirValue != 0x03){
            timing = 1;
        }
        m_LampDirValue = 0x03;
        sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);

    } else {
        m_EmergencyValue = true;
        slotBtnDirOffClicked();
    }
}






// Turn sig
void MainWindow::slotBtnDirColorReset(void)
{
    ui->btnLampDirOff->setStyleSheet("QPushButton { background-color : rgb(215,215,158); }");
    ui->btnLampDirLeft->setStyleSheet("QPushButton { background-color : rgb(215,215,158); border-image: url(:/images/system/left.png);}");
    ui->btnLampDirRight->setStyleSheet("QPushButton { background-color : rgb(215,215,158); border-image: url(:/images/system/right.png);}");
    ui->btnLampEmergency->setStyleSheet("QPushButton { background-color : rgb(215,215,158); border-image: url(:/images/warning.png);}");
    ui->lbStateEmergency->setStyleSheet("QLabel { background : rgb(215,215,158);  border-image: url(:/images/warning.png);}");
}

void MainWindow::slotLampDirectionState(void)
{
    switch(m_LampDirValue)
    {
        case 0:
            m_SigValue = 0x00;
           break;
        case 1: // left
           switch(timing){
                case 1:
                    ui->lbLeftSig->setStyleSheet("QLabel { background-color : orange; border-image: url(:/images/system/left.png);}");
                    m_SigValue = 2;
                    timing++;
                    break;
                case 2:
                    ui->lbLeftSig->setStyleSheet("QLabel { color : rgb(215,215,158); border-image: url(:/images/system/left.png);}");
                    m_SigValue = 3;
                    timing++;
                    break;
                case 3:
                    ui->lbLeftSig->setStyleSheet("QLabel { background-color : orange; border-image: url(:/images/system/left.png);}");
                    m_SigValue = 4;
                    timing++;
                    break;
                case 4:
                    ui->lbLeftSig->setStyleSheet("QLabel { color : rgb(215,215,158); border-image: url(:/images/system/left.png);}");
                    m_SigValue = 5;
                    timing++;
                    break;
                default:
                    m_SigValue = 0x00;
                    timing = 1;
                    break;
            }
           break;
        case 2: //right
            switch(timing){
                case 1:
                    ui->lbRightSig->setStyleSheet("QLabel { background-color : orange; border-image: url(:/images/system/right.png);}");
                    m_SigValue = 6;
                    timing++;
                    break;
                case 2:
                    ui->lbRightSig->setStyleSheet("QLabel { color : rgb(215,215,158); border-image: url(:/images/system/right.png);}");
                    m_SigValue = 7;
                    timing++;
                    break;
                case 3:
                    ui->lbRightSig->setStyleSheet("QLabel { background-color : orange; border-image: url(:/images/system/right.png);}");
                    m_SigValue = 8;
                    timing++;
                    break;
                case 4:
                    ui->lbRightSig->setStyleSheet("QLabel { color : rgb(215,215,158); border-image: url(:/images/system/right.png);}");
                    m_SigValue = 9;
                    timing++;
                    break;
                default:
                    m_SigValue = 0;
                    timing = 1;
                    break;
            }
           break;
        case 3: //Emergency
            switch(timing){
                case 1:
                    ui->lbStateEmergency->setStyleSheet("QLabel { background-color : orange; border-image: url(:/images/warning.png);}");
                    m_SigValue = 1;
                    timing++;
                    break;
                default:
                    ui->lbStateEmergency->setStyleSheet("QLabel { color : rgb(215,215,158); border-image: url(:/images/warning.png);}");
                    m_SigValue = 0;
                    timing = 1;
                    break;
            }
           break;
        default:
           break;
    }
}

void MainWindow::slotBtnDirOffClicked()
{
    slotBtnDirColorReset();
    m_EmergencyValue = true;
    ui->btnLampDirOff->setStyleSheet("QPushButton { background-color : red; }");
    ui->lbRightSig->setStyleSheet("QLabel { color : rgb(215,215,158); border-image: url(:/images/system/right.png);}");
    ui->lbLeftSig->setStyleSheet("QLabel { color : rgb(215,215,158); border-image: url(:/images/system/left.png);}");
    m_LampDirValue = 0x00;
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
    timing=1;

}

void MainWindow::slotBtnDirLeftClicked()
{
    slotBtnDirOffClicked();
    slotBtnDirColorReset();
    m_EmergencyValue = true;
    if(m_LampDirValue != 0x01){
        timing = 1;
    }
    ui->btnLampDirLeft->setStyleSheet("QPushButton { background-color : red; border-image: url(:/images/system/left.png);}");
    m_LampDirValue = 0x01;
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
}

void MainWindow::slotBtnDirRightClicked()
{
    slotBtnDirOffClicked();
    slotBtnDirColorReset();
    m_EmergencyValue = true;
    ui->btnLampDirRight->setStyleSheet("QPushButton { background-color : red; border-image: url(:/images/system/right.png);}");
    if(m_LampDirValue != 0x02){
        timing = 1;
    }
    m_LampDirValue = 0x02;
    sendCanDataPacket(0x20, type1, m_LampDirValue , (unsigned char)speed, status, light, m_GPIOLedValue, 0, 0);
}




void MainWindow::slotTimeDisplay(void)
{


    ui->lbDispLabelTop->setText(getI2cTemperatureValue());
    ui->lbDispLabelMiddle->setText(getI2cIlluminationValue());



    switch(m_SigValue)
    {
        case 0: // OFF               1111 1111
            m_RotaryLedValue = 0xFF;
        break;
        case 1: // ALL               0000 0000
            m_RotaryLedValue = 0x00;
        break;
        case 2: //left test          1111 0111
            m_RotaryLedValue = 0xF7;
        break;
        case 3:  //                    1111 0011
            m_RotaryLedValue = 0xF3;
            break;
        case 4:    //                  1111 0001
            m_RotaryLedValue = 0xF1;
            break;
        case 5:       //               1111 0000
            m_RotaryLedValue = 0xF0;
            break;
        case 6: //right test          1110 1111
            m_RotaryLedValue = 0xEF;
        break;
        case 7:  //                    1100 1111
            m_RotaryLedValue = 0xCF;
            break;
        case 8:   //                   1000 1111
            m_RotaryLedValue = 0x8F;
            break;
        case 9:   //                   0000 1111
            m_RotaryLedValue = 0xF;
            break;

        default:
        break;
    }
    setI2cExpGpioRotaryLed(&mI2cFd);
    usleep(1000);

    getRTCvalue(&mI2cFd);
}



void MainWindow::getRTCvalue(int *fd)
{
    int retv;

    unsigned char clock[10];
    unsigned char reg=0, bh, bl;

    int year;
    int month;
    int day;

    int hour;
    int min;
    int sec;

    char str[12]={0,};

    char strTempArray[30];

    retv = ioctl(*fd, I2C_SLAVE, RTC_ADDR);
    usleep(1000);

    // read Rtc
    reg = 0x00;
    write(*fd, &reg ,1);

    read(*fd,clock,7);
    c6 = clock[6]; // y
    c5 = clock[5]; // m
    c4 = clock[4]; // d
    c2 = clock[2]; // h
    c1 = clock[1]; // min
    c0 = clock[0]; // sec

    if(rtcsend && start){
        qDebug("yyyy mm dd : %X %X %X\n", c6, c5, c4);
        sendCanDataPacket(0x20, type2, c6, c5,c4, c2, c1, c0, 0);
        rtcsend = false;
    }

    sprintf(strTempArray,"%04d/%02d/%02d", (((clock[6]&0xF0)>>4)*10+(clock[6]&0xF))+2000,((clock[5]&0x10)>>4)*10+(clock[5]&0xF),((clock[4]&0x30)>>4)*10+(clock[4]&0xF));
    ui->lbDispDate->setText(strTempArray);

    sprintf(strTempArray,"%02d:%02d:%02d",((clock[2]&0x30)>>4)*10+(clock[2]&0xF),((clock[1]&0x70)>>4)*10+(clock[1]&0xF),((clock[0]&0x70)>>4)*10+(clock[0]&0xF));
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

    setI2cExpGpioSWLEDInit(&mI2cFd);
    m_RotaryLedValue = 0xff;
    setI2cExpGpioRotaryLed(&mI2cFd);

    start = false;
    speed=0;
    ui->lcdDigitNumber->display(speed);
    ui->btnStart->setStyleSheet("QPushButton { border-image : url(:/images/system/state_critical.png); font:20px;}");
    ui->btnStart->setText("Start/Stop");
    ui->btnAccel->setStyleSheet("border-image:url(:/images/system/state_disable.png); ");
    ui->btnAccel->setEnabled(false);
    ui->btnBrake->setStyleSheet("border-image:url(:/images/system/state_disable.png); ");
    ui->btnBrake->setEnabled(false);
    ui->lbBattery->setStyleSheet("background : transparent; border-image:url(:/images/system/fire.png);");
    ui->lbTemp->setStyleSheet("background : transparent; border-image:url(:/images/system/ice.png);");

    ::close(mI2cFd);

    delete ui;
}


