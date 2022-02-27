#ifndef PASSWORD_H
#define PASSWORD_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class password;
}

class password : public QDialog
{
    Q_OBJECT

public:
    explicit password(QWidget *parent = nullptr);
    ~password();

    char registered[100] = "12345678";
    char userinput[100] = "";
    int cursor = 0;

public slots:
    void slotBtnLoginClicked(void);
    void slotBtnPassword1Clicked(void);
    void slotBtnPassword2Clicked(void);
    void slotBtnPassword3Clicked(void);
    void slotBtnPassword4Clicked(void);
    void slotBtnPassword5Clicked(void);
    void slotBtnPassword6Clicked(void);
    void slotBtnPassword7Clicked(void);
    void slotBtnPassword8Clicked(void);
    void slotBtnPassword9Clicked(void);



private:
    Ui::password *ui;
    MainWindow * w;
};

#endif // PASSWORD_H
