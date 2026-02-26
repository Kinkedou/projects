#include "secondwindow.h"
#include "ui_secondwindow.h"
#include <QtDebug>
#include <QPixmap>
#include <QDateTime>
#include <QDir>          // 添加目录操作头文件
#include <QFileInfoList> // 添加文件信息列表头文件
#include <QImageReader>  // 添加图片读取头文件
#include "rpc_client.h"
#include "video_thread.h"

extern VideoThread g_vthread;

secondwindow::secondwindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::secondwindow),
    currentImageIndex(-1)  // 初始化索引为-1（表示暂无图片）
{
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, &secondwindow::Button_Backed_0n_Clicked);
    // 连接点击信号到槽函数
    connect(ui->pushButton_2, &QPushButton::clicked, this, &secondwindow::Button_Shoted_0n_Clicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &secondwindow::Button_Looked_0n_Clicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &secondwindow::Button_prev_0n_Clicked);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &secondwindow::Button_next_0n_Clicked);
    connect(ui->pushButton_6, &QPushButton::clicked, this, &secondwindow::Button_detect_0n_Clicked);
    second_flag=1;            // 1为监控模式
    ui->pushButton_4->hide(); // 隐藏上一张按钮
    ui->pushButton_5->hide(); // 隐藏下一张按钮
}

secondwindow::~secondwindow()
{
    delete ui;
}

//移动监测开关
void secondwindow::Button_detect_0n_Clicked()
{
    static int status = 1;
    rpc_detect_control(status);
    status = !status;
}

//更新图像显示区域
void secondwindow::updateVideoLabel(const QImage &frame)
{
        QPixmap pixmap;
        pixmap= QPixmap::fromImage(frame);
        ui->label->setPixmap(pixmap);
        curframe=frame;

}

//返回键
void secondwindow::Button_Backed_0n_Clicked()
{
    second_flag = 1;
    this->close();
    ui->pushButton_4->hide();
    ui->pushButton_5->hide();
    ui->pushButton_2->show();
    ui->pushButton_3->setText("查看监控");
}

//记录键
void secondwindow::Button_Shoted_0n_Clicked()
{
    if (curframe.isNull()) {
        qDebug() << "错误：当前无图片可保存";
        return;
    }
    QString picturesDir = "/root";
    // 检查目录是否存在，不存在则创建
    QDir saveDir(picturesDir);
    if (!saveDir.exists()) {
        if (!saveDir.mkpath(".")) {
            qDebug() << "错误：无法创建保存目录：" << picturesDir;
            return;
        }
    }
    // 生成带时间戳的唯一文件名，避免覆盖
    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    QString fileName = QString("warning_%1.jpg").arg(timeStamp);
    QString fullPath = saveDir.absoluteFilePath(fileName);
    // 保存图片并检查结果
    bool saveSuccess = curframe.save(fullPath, "JPG", 95); // 95是JPG质量（0-100）
    if (saveSuccess) {
        qDebug() << "截图保存成功：" << fullPath;
    } else {
        qDebug() << "错误：截图保存失败！路径：" << fullPath;
        qDebug() << "请检查：1.目录权限 2.磁盘空间 3.路径是否合法";
    }
}

//查看记录按钮
void secondwindow::Button_Looked_0n_Clicked()
{
    if(second_flag)
    {
        g_vthread.noSend();
        second_flag=0;
        // 清空label显示
        ui->label->clear();
        // 清空原有列表
        imagePaths.clear();
        currentImageIndex = -1;
        // 定义要扫描的目录
        QDir rootDir("/root");
        // 设置过滤器：只显示jpg文件（不区分大小写）
        QStringList filters;
        filters << "*.jpg" << "*.JPG";
        rootDir.setNameFilters(filters);
        // 获取文件列表并按名称排序（按时间命名的话，排序后就是拍摄顺序）
        QFileInfoList fileList = rootDir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        // 提取文件路径
        for (const QFileInfo &fileInfo : fileList) {
            imagePaths.append(fileInfo.absoluteFilePath());
        }
        // 如果有图片，默认显示第一张
        if (!imagePaths.isEmpty()) {
            currentImageIndex = 0;
            showImageByIndex(currentImageIndex);
            qDebug() << "找到" << imagePaths.size() << "张jpg图片";
        } else {
            qDebug() << "/root目录下未找到jpg图片";
        }
        ui->pushButton_4->show();
        ui->pushButton_5->show();
        ui->pushButton_2->hide();
        ui->pushButton_3->setText("返回监控");
    }
    else
    {
        second_flag=1;
        g_vthread.canSend();
        ui->pushButton_4->hide();
        ui->pushButton_5->hide();
        ui->pushButton_2->show();
        ui->pushButton_3->setText("查看监控");
    }

}

//上一张
void secondwindow::Button_prev_0n_Clicked()
{
    // 没有图片或已经是第一张，直接返回
    if (imagePaths.isEmpty() || currentImageIndex <= 0) {
        qDebug() << "已经是第一张图片/无图片可切换";
        return;
    }
    // 索引减1，显示上一张
    currentImageIndex--;
    showImageByIndex(currentImageIndex);
}

void secondwindow::Button_next_0n_Clicked()
{
    // 没有图片或已经是最后一张，直接返回
    if (imagePaths.isEmpty() || currentImageIndex >= imagePaths.size() - 1) {
        qDebug() << "已经是最后一张图片/无图片可切换";
        return;
    }
    // 索引加1，显示下一张
    currentImageIndex++;
    showImageByIndex(currentImageIndex);
}

void secondwindow::showImageByIndex(int index)
{
    if (index < 0 || index >= imagePaths.size()) {
        qDebug() << "无效的图片索引：" << index;
        return;
    }
    // 读取图片
    QImage image(imagePaths[index]);
    if (image.isNull()) {
        qDebug() << "图片读取失败：" << imagePaths[index];
        ui->label->clear();
        return;
    }
    // 调用已有函数更新label显示（适配大小）
    updateVideoLabel(image);
    qDebug() << "当前显示：" << imagePaths[index];
}
