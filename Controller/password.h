#ifndef PASSWORD_H
#define PASSWORD_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class Password;
}

class Password : public QDialog
{
    Q_OBJECT

public:
    explicit Password(QWidget *parent = nullptr);
    ~Password();

private:
    Ui::Password *ui;
    MainWindow* w;
};

#endif // PASSWORD_H
