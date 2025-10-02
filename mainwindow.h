
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

// 前置声明，不需要包含Graph.hpp
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
    // 文件选择
    void on_selectInputButton_clicked();
    void on_selectOutputButton_clicked();

    // 运行分析
    void on_runAnalysisButton_clicked();

    // 处理后端程序输出
    void on_processReadyRead();
    void on_processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // 查看结果
    void on_viewResultsButton_clicked();

private:
    Ui::MainWindow* ui;
    QProcess* backendProcess;

    // 工具函数
    bool validateInputs();
    void updateRunButtonState();
};
#endif // MAINWINDOW_H