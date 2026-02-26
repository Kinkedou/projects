#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "secondwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QLabel *GetHumiLabel();
    QLabel *GetTempLabel();
    secondwindow *p_swindow; //声明新界面的指针

private slots:
    void Button_LED_0n_Clicked();
    void Button_Video_0n_Clicked();
    void Button_gaojing_0n_Clicked();
    void Button_detect_0n_Clicked();

private:
    Ui::MainWindow *ui;
    QLabel* labelHumi;
    QLabel *labelTemp;
};
#endif // MAINWINDOW_H
