#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QStringList>

namespace Ui {
class secondwindow;
}

class secondwindow : public QWidget
{
    Q_OBJECT

public:
    explicit secondwindow(QWidget *parent = nullptr);
    ~secondwindow();
    void updateVideoLabel(const QImage &frame);
    int  second_flag;
    void Button_Looked_0n_Clicked();
    void Button_detect_0n_Clicked();

private slots:
    void Button_Backed_0n_Clicked();
    void Button_Shoted_0n_Clicked();
    void Button_prev_0n_Clicked();
    void Button_next_0n_Clicked();

private:
    Ui::secondwindow *ui;
    QLabel *labelVideo;
    QImage curframe;
    QStringList imagePaths;           // 存储/root目录下的jpg图片路径列表
    int currentImageIndex;            // 当前显示的图片索引
    void showImageByIndex(int index); //显示指定索引的图片
};

#endif // SECONDWINDOW_H
