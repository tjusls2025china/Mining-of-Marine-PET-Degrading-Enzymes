#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // ����Ӧ�ó�����Ϣ
    a.setApplicationName("�����������");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("����ʵ����");

    MainWindow w;
    w.show();
    return a.exec();
}