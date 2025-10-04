#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序信息
    a.setApplicationName("聚类分析工具");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("生物实验室");

    MainWindow w;
    w.show();
    return a.exec();
}