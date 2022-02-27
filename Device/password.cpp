#include "password.h"
#include "mainwindow.h"
#include "string.h"
#include "ui_password.h"
#include <QDebug>

password::password(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::password)
{
    ui->setupUi(this);
    w = new MainWindow();
    connect(ui->pbLogin,SIGNAL(clicked()),SLOT(slotBtnLoginClicked()));
    connect(ui->pbPassword1,SIGNAL(clicked()),SLOT(slotBtnPassword1Clicked()));
    connect(ui->pbPassword2,SIGNAL(clicked()),SLOT(slotBtnPassword2Clicked()));
    connect(ui->pbPassword3,SIGNAL(clicked()),SLOT(slotBtnPassword3Clicked()));
    connect(ui->pbPassword4,SIGNAL(clicked()),SLOT(slotBtnPassword4Clicked()));
    connect(ui->pbPassword5,SIGNAL(clicked()),SLOT(slotBtnPassword5Clicked()));
    connect(ui->pbPassword6,SIGNAL(clicked()),SLOT(slotBtnPassword6Clicked()));
    connect(ui->pbPassword7,SIGNAL(clicked()),SLOT(slotBtnPassword7Clicked()));
    connect(ui->pbPassword8,SIGNAL(clicked()),SLOT(slotBtnPassword8Clicked()));
    connect(ui->pbPassword9,SIGNAL(clicked()),SLOT(slotBtnPassword9Clicked()));



}
void password::slotBtnLoginClicked(void){
    qDebug("login button clicked");
    if(strcmp(userinput, registered) == 0){
        userinput[0] = '\0';
        cursor = 0;
        w -> show();
        this -> hide();
    }

}

void password::slotBtnPassword1Clicked(void){
    userinput[cursor] = '1';
    cursor ++;
}
void password::slotBtnPassword2Clicked(void){
    userinput[cursor] = '2';
    cursor ++;
}
void password::slotBtnPassword3Clicked(void){
    userinput[cursor] = '3';
    cursor ++;
}
void password::slotBtnPassword4Clicked(void){
    userinput[cursor] = '4';
    cursor ++;
}
void password::slotBtnPassword5Clicked(void){
    userinput[cursor] = '5';
    cursor ++;
}
void password::slotBtnPassword6Clicked(void){
    userinput[cursor] = '6';
    cursor ++;
}
void password::slotBtnPassword7Clicked(void){
    userinput[cursor] = '7';
    cursor ++;
}
void password::slotBtnPassword8Clicked(void){
    userinput[cursor] = '8';
    cursor ++;
}
void password::slotBtnPassword9Clicked(void){
    userinput[cursor] = '9';
    cursor ++;
}


password::~password()
{
    delete ui;
}
