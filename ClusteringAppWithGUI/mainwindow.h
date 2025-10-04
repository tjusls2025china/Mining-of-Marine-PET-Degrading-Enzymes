
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

// ǰ������������Ҫ����Graph.hpp
QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // �ļ�ѡ��
    void on_selectInputButton_clicked();
    void on_selectOutputButton_clicked();

    // ���з���
    void on_runAnalysisButton_clicked();

    // �����˳������
    void on_processReadyRead();
    void on_processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // �鿴���
    void on_viewResultsButton_clicked();

private:
    Ui::MainWindow* ui;
    QProcess* backendProcess;

    // ���ߺ���
    bool validateInputs();
    void updateRunButtonState();
};
#endif // MAINWINDOW_H