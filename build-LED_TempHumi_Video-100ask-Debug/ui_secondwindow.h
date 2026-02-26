/********************************************************************************
** Form generated from reading UI file 'secondwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SECONDWINDOW_H
#define UI_SECONDWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_secondwindow
{
public:
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;

    void setupUi(QWidget *secondwindow)
    {
        if (secondwindow->objectName().isEmpty())
            secondwindow->setObjectName(QString::fromUtf8("secondwindow"));
        secondwindow->resize(1024, 600);
        pushButton = new QPushButton(secondwindow);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(20, 20, 111, 41));
        QFont font;
        font.setPointSize(25);
        pushButton->setFont(font);
        pushButton_2 = new QPushButton(secondwindow);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(270, 500, 201, 71));
        QFont font1;
        font1.setPointSize(33);
        pushButton_2->setFont(font1);
        pushButton_3 = new QPushButton(secondwindow);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        pushButton_3->setGeometry(QRect(490, 500, 201, 71));
        pushButton_3->setFont(font1);

        retranslateUi(secondwindow);

        QMetaObject::connectSlotsByName(secondwindow);
    } // setupUi

    void retranslateUi(QWidget *secondwindow)
    {
        secondwindow->setWindowTitle(QApplication::translate("secondwindow", "Form", nullptr));
        pushButton->setText(QApplication::translate("secondwindow", "\350\277\224  \345\233\236", nullptr));
        pushButton_2->setText(QApplication::translate("secondwindow", "\350\256\260\345\275\225", nullptr));
        pushButton_3->setText(QApplication::translate("secondwindow", "\346\237\245\347\234\213\350\256\260\345\275\225", nullptr));
    } // retranslateUi

};

namespace Ui {
    class secondwindow: public Ui_secondwindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SECONDWINDOW_H
