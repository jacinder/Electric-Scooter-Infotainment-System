#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "devices.h"
#include "gpios.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <QDebug>
#include <QDateTime>
#include <QDate>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    putSystemMessage("[System] GUI Initialization - OK.");



    tmrRtcStat = new QTimer(this);
    tmrHeadLight = new QTimer(this);
    tmrTurnSignal = new QTimer(this);
    tmrScooterStat = new QTimer(this);
    tmrAccelBrake = new QTimer(this);
    tmrTemperature = new QTimer(this);
    tmrSpeedStat = new QTimer(this);
    tmrCdsStat = new QTimer(this);


    tmrHeadLight->start(50);
    tmrTurnSignal->start(250);
    tmrScooterStat->start(50);
    tmrAccelBrake->start(50);
    tmrTemperature->start(50);
    tmrSpeedStat->start(50);
    tmrCdsStat->start(50);
    
    putSystemMessage("[System] Main timer Created!");


    recvCanData = new thCanRecv();
    recvCanData->start();
    putSystemMessage("[System] Data Recv Thread Start!");

    connect(tmrHeadLight, SIGNAL(timeout()), SLOT(slotTimerHeadLightState()));
    // Timer Event
    connect(recvCanData, SIGNAL(sendCanData(char*)),this, SLOT(slotRecvCanDataPacket(char*))); //CanPacket을 받으면 발생되는 시그널
    connect(tmrRtcStat, SIGNAL(timeout()), SLOT(slotTimerRtcState())); //1초마다 발생
    connect(tmrTurnSignal, SIGNAL(timeout()), SLOT(slotTimerLampDirectionState()));
    connect(tmrScooterStat, SIGNAL(timeout()), SLOT(slotTimerScooterStatus()));
    connect(tmrAccelBrake,SIGNAL(timeout()), SLOT(slotTimerAccelBrakeStatus()));
    connect(tmrTemperature, SIGNAL(timeout()),SLOT(slotTimerTemperatureStat()));
    connect(tmrSpeedStat, SIGNAL(timeout()),SLOT(slotTimerSpeedStat()));
    connect(tmrCdsStat, SIGNAL(timeout()),SLOT(slotCdsStat()));
    connect(ui->btnDebugMsgClear, SIGNAL(clicked()), SLOT(slotDebugMsgClear()));
}

void MainWindow::slotDebugMsgClear(void){
    ui->teDebugMessage->clear();
}
void MainWindow::slotRecvCanDataPacket(char* recvCData)
{
    QString strData;
    char data[13];
    memcpy(data,recvCData,sizeof(data));
    strData.sprintf("%04x %02x | %02x %02x %02x %02x %02x %02x %02x %02x",data[0]<<24|data[1]<<16|data[2]<<8|data[3],data[4],data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12]);



    // Data : 5(0) | 6(1) | 7(2) | 8(3) | 9(4) | 10(5) | 11(6) | 12(7)
    // 0 Type:         | Packet Type

    // Type 0
    // 1-6 RTC

    // Type 1
    // 1 SignalLight   | 00 OFF / 01 LEFT / 02 RIGHT / 03 EMERGENCY
    // 2 Speed         | 속도 그대로
    // 3 ScooterStatus | 00 RED / 01 BLUE / 02 GREEN
    // 4 HeadLight     | 00 Man OFF / 01 Man ON / 02 Auto OFF / 03 Auto ON
    // 5 Accel & brake | 00 OFF /
    //                 | 01 Brake 1sec / 02 Brake 2sec / 03 Brake 3sec / 04 Brake 4sec
    //                 | 05 Accel 1sec / 06 Accel 2sec / 07 Accel 3sec / 08 Accel 4sec
    //

    // Type 2
    // 1 Cds           | 교수님 코드에 있는 조도센서값 포맷대로
    // 2-3 Temperature | 강의자료에 나와있는대로


    if(data[5]==0){
        putSystemMessage("[System] Type0 Data Receive!");
        //rtc
        year  = ((((data[6]&0xF0)>>4)*10) + (data[6]&0xF)) + 2000;
        month = ((((data[7]&0x10)>>4)*10)  + (data[7]&0xF));
        day  = ((((data[8]&0x30)>>4)*10)  + (data[8]&0xF));
        hour  = ((((data[9]&0x30)>>4)*10) + (data[9]&0xF));
        min   = ((((data[10]&0x70)>>4)*10) + (data[10]&0xF));
        sec   = ((((data[11]&0x70)>>4)*10) + (data[11]&0xF));

        qDebug("yyyy mm dd:%X %X %X\n", data[6], data[7], data[8]);

        QString datestat, timestat;
        datestat.sprintf("%04d-%02d-%02d",year,month,day);
        timestat.sprintf("%02d:%02d:%02d\n",hour,min,sec);
        putSystemMessage(datestat);
        putSystemMessage(timestat);

        ui->lbDispDate->setText(datestat);
        ui->lbDispTime->setText(timestat);

        tmrRtcStat->start(1000);

    }
    if (data[5] == 1){
        putSystemMessage("[System] Type1 Data Receive!");
        //Packet type 1
        //turn signal & emergency light
        switch(data[6]){
            case 0x00: //OFF
                optTurnSignal = 0;
                break;
            case 0x01: //LEFT
                optTurnSignal = 1;
                break;
            case 0x02: //RIGHT
                optTurnSignal = 2;
                break;
            case 0x03: //emergency
                optTurnSignal = 3;
                break;
            default :
                break;
        }

        //current Speed State
        statSpeed = data[7];
        //current Scooter State
        statScooter = data[8];
 
        switch(data[9]){
            //head light
            case 0x00: //manual(F), off(F)
                optHeadLight = false;
                statHeadLight = false;
                break;
            case 0x01: //manual(F), on(T)
                optHeadLight = false;
                statHeadLight = true;
                break;
            case 0x02: //auto(T), off(F)
                optHeadLight = true;
                statHeadLight = false;
                break;
            case 0x03: //auto(T), on(T)
                optHeadLight = true;
                statHeadLight = true;
                break;

        }
        switch(data[10]){
            //accel & brake
            case 0x00: // OFF
                braketimer = 0;
                acceltimer = 0;
                statAccel = false;
                statBrake = false;
                break;
            case 0x01: // brake ON 1sec..
                statAccel = false;
                statBrake = true;
                braketimer = 1;
                break;
            case 0x02: // 2sec..
                statAccel = false;
                statBrake = true;
                braketimer = 2;
                break;
            case 0x03: // 3sec..
                statAccel = false;
                statBrake = true;
                braketimer = 3;
                break;
            case 0x04: // 4sec..
                statAccel = false;
                statBrake = true;
                braketimer = 4;
                break;
            case 0x05: // accel ON 1sec...
                statAccel = true;
                statBrake = false;
                acceltimer = 1;
                break;
            case 0x06: // 2sec..
                statAccel = true;
                statBrake = false;
                acceltimer = 2;
                break;
            case 0x07: // 3sec..
                statAccel = true;
                statBrake = false;
                acceltimer = 3;
                break;
            case 0x08: // 4sec..
                statAccel = true;
                statBrake = false;
                acceltimer = 4;
                break;
        }



    }
    else if(data[5] == 2){

        putSystemMessage("[System] Type2 Data Receive!");

        //Packet type 2
        //data[5] cds sensor
        statCdsSensor = ((255-data[6])/255.0)*100;

        //data[7]~data[8] temperature sensor
        statTemperature = (((data[7]<<8)|data[8])>>4)*0.0625;

    }

}


MainWindow::~MainWindow()
{
    tmrHeadLight->stop();
    tmrTurnSignal->stop();
    tmrScooterStat->stop();
    tmrAccelBrake->stop();
    tmrTemperature->stop();
    tmrCdsStat->stop();

    recvCanData->exit();

    /* Implement a destructor. */


    
    delete ui;
}

void MainWindow::slotTimerRtcState(void)
{

    sec ++;
    if(sec > 60){
        sec = 0;
        min += 1;
    }
    if(min > 60){
        min = 0;
        hour += 1 ;
    }
    if(hour > 24){
        hour = 0;
        day += 1;
    }

    QString datestat, timestat;
    datestat.sprintf("%04d-%02d-%02d",year,month,day);
    timestat.sprintf("%02d:%02d:%02d",hour,min,sec);

    ui->lbDispDate->setText(datestat);
    ui->lbDispTime->setText(timestat);


}
void MainWindow::putSystemMessage(QString msg)
{
    ui->teDebugMessage->append(msg);
}
void MainWindow::slotCdsStat(void)
{
    QString temp;
    temp.sprintf("%lf",statCdsSensor);
    ui->leCdsValue->setText(temp);
}
void MainWindow::slotTimerSpeedStat(void)
{
    QString temp;
    temp.sprintf("%d",statSpeed);
    ui->leSpeedValue->setText(temp);

    if (statSpeed <= 12){
        ui->pbGear->setText("1");
    } else if (statSpeed <= 24){
        ui->pbGear->setText("2");
    }
}
void MainWindow::slotTimerTemperatureStat(void)
{
    QString temp;
    temp.sprintf("%lf",statTemperature);
    ui->leTempValue->setText(temp);
}

void MainWindow::slotTimerAccelBrakeStatus(void){
    //Accel & brake | 00 OFF / 01 Brake / 02 Accel
    //bool statAccel;
    //bool statBrake;
    if(statAccel){
        ui->pbCtrlAccelState->setStyleSheet("QPushButton {background-color:orange;}");
    }
    else{
        ui->pbCtrlAccelState->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
    }
    if(statBrake){
        ui->pbCtrlBrakeState->setStyleSheet("QPushButton {background-color:orange;}");
    }
    else{
        ui->pbCtrlBrakeState->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
    }
    if(braketimer == 0 && acceltimer == 0){
        ui->pbSpeedTimer1->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
        ui->pbSpeedTimer2->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
        ui->pbSpeedTimer3->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
        ui->pbSpeedTimer4->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
    }
    else if(braketimer == 1 || acceltimer == 1){
        ui->pbSpeedTimer1->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer2->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
        ui->pbSpeedTimer3->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
        ui->pbSpeedTimer4->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
    }
    else if(braketimer == 2 || acceltimer == 2){
        ui->pbSpeedTimer1->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer2->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer3->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
        ui->pbSpeedTimer4->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
    }
    else if(braketimer == 3 || acceltimer == 3){
        ui->pbSpeedTimer1->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer2->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer3->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer4->setStyleSheet("QPushButton {background-color:rgb(40,40,40);}");
    }
    else if(braketimer == 4 || acceltimer == 4){
        ui->pbSpeedTimer1->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer2->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer3->setStyleSheet("QPushButton {background-color:orange;}");
        ui->pbSpeedTimer4->setStyleSheet("QPushButton {background-color:orange;}");
    }


}

void MainWindow::slotTimerScooterStatus(void){
    // 00 RED / 01 BLUE / 02 GREEN
    if (statScooter == 0x00){
        ui->btnColorLedState -> setStyleSheet("QPushButton {background-color:rgb(255,0,0); }");
        ui->leColorLedValue -> setText("FIRE WARNING");
    }
    else if (statScooter == 0x01){
        ui->btnColorLedState -> setStyleSheet("QPushButton {background-color:orange; }");
        ui->leColorLedValue -> setText("TURN OFF");
    }
    else if (statScooter == 0x02){
        ui->btnColorLedState -> setStyleSheet("QPushButton {background-color:rgb(0,255,0); }");
        ui->leColorLedValue -> setText("TURN ON");
    }
    else if(statScooter == 0x03){
        ui->btnColorLedState -> setStyleSheet("QPushButton {background-color:rgb(0,0,255); }");
        ui->leColorLedValue -> setText("FREEZING WARNING");
    }
}



void MainWindow::slotTimerHeadLightState(void)
{
    if(statHeadLight){ //on
        // picture UI
        ui->pbHeadLight->setStyleSheet("QPushButton { background-color: orange; color:rgb(40,40,40);}");
        // button UI
        ui->btnLampStateOn->setStyleSheet("QPushButton { background-color: orange; color:rgb(40,40,40);}");
        ui->btnLampStateOff->setStyleSheet("QPushButton { background-color: rgb(40,40,40); color:white; }");
    }
    else{ //off
        // picture UI
        ui->pbHeadLight->setStyleSheet("QPushButton { background-color: rgb(40,40,40);}");
        // button UI
        ui->btnLampStateOn->setStyleSheet("QPushButton { background-color: rgb(40,40,40); color:white; }");
        ui->btnLampStateOff->setStyleSheet("QPushButton { background-color: orange; color:rgb(40,40,40);}");
    }
    if(optHeadLight){ //auto
        ui->btnLampOption->setStyleSheet("QPushButton {background-color:rgb(236,103,83); color:white;} ");
        ui->btnLampOption->setText("AUTO");

    }
    else{
        ui->btnLampOption->setStyleSheet("QPushButton {background-color:rgb(2,63,63); color:white;} ");
        ui->btnLampOption->setText("MANUAL");
    }

}

void MainWindow::slotTimerLampDirectionState(void)
{
    if(blinkTurnSignal){
        switch(optTurnSignal){
            case 0: // OFF
                ui->pbTurnSignalR1->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                ui->pbTurnSignalR2->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                ui->pbTurnSignalR3->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");

                ui->pbTurnSignalL1->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                ui->pbTurnSignalL2->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                ui->pbTurnSignalL3->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");

                ui->statTurnSignalL->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                ui->statSignalOff->setStyleSheet("QPushButton { background-color : orange; color:white;}");
                ui->statTurnSignalR->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                ui->statEmerSignal->setStyleSheet("QLabel { background-color: rgb(148,169,216); }");
                break;

            case 1: // LEFT
                if(timing == 1){
                    // 1. TurnSignalR을 확실하게 OFF
                    ui->pbTurnSignalR1->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                    ui->pbTurnSignalR2->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                    ui->pbTurnSignalR3->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");

                    // 2. TurnSignalL을 순차적으로 ON
                    ui->pbTurnSignalL1->setStyleSheet("QPushButton { background-color : orange; }");

                    // Button UI도 업데이트
                    ui->statTurnSignalL->setStyleSheet("QPushButton { background-color : orange; color:rgb(40,40,40);}");
                    ui->statSignalOff->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                    ui->statTurnSignalR->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                    ui->statEmerSignal->setStyleSheet("QLabel { background-color: rgb(148,169,216); }");

                    timing++;
                }
                else if(timing == 2){
                    ui->pbTurnSignalL2->setStyleSheet("QPushButton { background-color : orange; }");
                    timing++;
                }
                else{
                    ui->pbTurnSignalL3->setStyleSheet("QPushButton { background-color : orange; }");
                    blinkTurnSignal=false;
                    timing=1;
                }
                break;



            case 2: // RIGHT
                if (timing == 1){
                    // 1. TrunSignalL을 확실하게 OFF
                    ui->pbTurnSignalL1->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                    ui->pbTurnSignalL2->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
                    ui->pbTurnSignalL3->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");

                    // 2. TurnSignalR을 순차적으로 ON
                    ui->pbTurnSignalR1->setStyleSheet("QPushButton { background-color : orange; }");

                    // 3. Button UI도 업데이트
                    ui->statTurnSignalR->setStyleSheet("QPushButton { background-color : orange; color:rgb(40,40,40);}");
                    ui->statTurnSignalL->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                    ui->statSignalOff->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                    ui->statEmerSignal->setStyleSheet("QLabel { background-color: rgb(148,169,216); }");

                    timing++;
                }
                else if(timing == 2){
                    ui->pbTurnSignalR2->setStyleSheet("QPushButton { background-color : orange; }");
                    timing++;
                }
                else{
                    ui->pbTurnSignalR3->setStyleSheet("QPushButton { background-color : orange; }");
                    timing = 1;
                    blinkTurnSignal=false;
                }
                break;

            case 3: // Emergency
                blinkTurnSignal=false;

                // 그림UI update
                ui->pbTurnSignalR1->setStyleSheet("QPushButton { background-color : orange; }");
                ui->pbTurnSignalR2->setStyleSheet("QPushButton { background-color : orange; }");
                ui->pbTurnSignalR3->setStyleSheet("QPushButton { background-color : orange; }");

                ui->pbTurnSignalL1->setStyleSheet("QPushButton { background-color : orange; }");
                ui->pbTurnSignalL2->setStyleSheet("QPushButton { background-color : orange; }");
                ui->pbTurnSignalL3->setStyleSheet("QPushButton { background-color : orange; }");

                // Button UI도 업데이트
                ui->statTurnSignalR->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                ui->statTurnSignalL->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                ui->statSignalOff->setStyleSheet("QPushButton { background-color : rgb(40,40,40); color:white;}");
                ui->statEmerSignal->setStyleSheet("QLabel { background-color: rgb(248,226,76); }");
               break;

            default :
                break;
        }
    } else { //blink 꺼지는 순서에서는 그림UI 한꺼번에 OFF
        blinkTurnSignal=true;
        ui->pbTurnSignalL1->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
        ui->pbTurnSignalL2->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
        ui->pbTurnSignalL3->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");

        ui->pbTurnSignalR1->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
        ui->pbTurnSignalR2->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
        ui->pbTurnSignalR3->setStyleSheet("QPushButton { background-color : rgb(40,40,40); }");
    }
}




