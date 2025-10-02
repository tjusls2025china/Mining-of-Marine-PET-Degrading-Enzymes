#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 设置窗口标题
    setWindowTitle(QStringLiteral("聚类分析工具 - 生物数据分析"));

    // 初始化后端进程
    backendProcess = new QProcess(this);
    connect(backendProcess, &QProcess::readyRead, this, &MainWindow::on_processReadyRead);
    //connect(backendProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),        this, &MainWindow::on_processFinished);

    connect(backendProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::on_processFinished);


    // 设置默认参数值
    ui->smallThresholdEdit->setText("0.5");
    ui->mediumThresholdEdit->setText("0.7");
    ui->largeThresholdEdit->setText("0.9");
    ui->minNeighborsEdit->setText("3");

    // 初始状态
    ui->runAnalysisButton->setEnabled(false);
    ui->viewResultsButton->setEnabled(false);
    ui->progressBar->setVisible(false);

    // 连接文本改变信号来更新按钮状态
    connect(ui->inputFileEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->outputDirEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->smallThresholdEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->mediumThresholdEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->largeThresholdEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->minNeighborsEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);

    // 显示欢迎信息
    ui->logTextEdit->append(QStringLiteral("欢迎使用聚类分析工具！"));
    ui->logTextEdit->append(QStringLiteral("请选择输入文件和输出目录，然后设置分析参数。"));
}

MainWindow::~MainWindow()
{
    if (backendProcess->state() == QProcess::Running) {
        backendProcess->terminate();
        backendProcess->waitForFinished(3000);
    }
    delete ui;
}

void MainWindow::on_selectInputButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "选择输入数据文件", "", "文本文件 (*.txt);;所有文件 (*.*)");

    if (!fileName.isEmpty()) {
        ui->inputFileEdit->setText(fileName);
    }
}

void MainWindow::on_selectOutputButton_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this,
        "选择输出目录", "", QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        // 确保目录路径以斜杠结尾
        if (!dirName.endsWith('/') && !dirName.endsWith('\\')) {
            dirName += '/';
        }
        ui->outputDirEdit->setText(dirName);
    }
}

bool MainWindow::validateInputs()
{
    // 检查文件路径
    if (ui->inputFileEdit->text().isEmpty()) {
        return false;
    }

    QFileInfo inputFile(ui->inputFileEdit->text());
    if (!inputFile.exists() || !inputFile.isFile()) {
        return false;
    }

    if (ui->outputDirEdit->text().isEmpty()) {
        return false;
    }

    // 检查数值参数
    bool ok1, ok2, ok3, ok4;
    double smallThresh = ui->smallThresholdEdit->text().toDouble(&ok1);
    double mediumThresh = ui->mediumThresholdEdit->text().toDouble(&ok2);
    double largeThresh = ui->largeThresholdEdit->text().toDouble(&ok3);
    int minNeighbors = ui->minNeighborsEdit->text().toInt(&ok4);

    if (!ok1 || !ok2 || !ok3 || !ok4) {
        return false;
    }

    // 检查阈值范围
    if (smallThresh < 0 || smallThresh > 1 ||
        mediumThresh < 0 || mediumThresh > 1 ||
        largeThresh < 0 || largeThresh > 1) {
        return false;
    }

    // 检查阈值大小关系
    if (smallThresh >= mediumThresh || mediumThresh >= largeThresh) {
        return false;
    }

    // 检查最小邻居数
    if (minNeighbors < 0) {
        return false;
    }

    return true;
}

void MainWindow::updateRunButtonState()
{
    ui->runAnalysisButton->setEnabled(validateInputs() && backendProcess->state() != QProcess::Running);
}

void MainWindow::on_runAnalysisButton_clicked()
{
    if (backendProcess->state() == QProcess::Running) {
        QMessageBox::warning(this, "警告", "分析正在进行中，请等待完成");
        return;
    }

    if (!validateInputs()) {
        QMessageBox::warning(this, "警告", "请检查输入参数是否正确");
        return;
    }

    // 更新UI状态
    ui->runAnalysisButton->setEnabled(false);
    ui->viewResultsButton->setEnabled(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0, 0); // 无限进度条

    // 清空日志并显示开始信息
    ui->logTextEdit->clear();
    ui->logTextEdit->append("开始聚类分析...");
    ui->logTextEdit->append("输入文件: " + ui->inputFileEdit->text());
    ui->logTextEdit->append("输出目录: " + ui->outputDirEdit->text());
    ui->logTextEdit->append("参数设置:");
    ui->logTextEdit->append("  - 宽松阈值: " + ui->smallThresholdEdit->text());
    ui->logTextEdit->append("  - 中等阈值: " + ui->mediumThresholdEdit->text());
    ui->logTextEdit->append("  - 严格阈值: " + ui->largeThresholdEdit->text());
    ui->logTextEdit->append("  - 最小邻居数: " + ui->minNeighborsEdit->text());
    ui->logTextEdit->append("----------------------------------------");

    // 准备命令行参数
    QStringList arguments;
    arguments << ui->inputFileEdit->text()
        << ui->outputDirEdit->text()
        << ui->smallThresholdEdit->text()
        << ui->mediumThresholdEdit->text()
        << ui->largeThresholdEdit->text()
        << ui->minNeighborsEdit->text();

    // 启动后端进程
    // 注意：这里假设你的可执行文件名为 Clustering.exe
    backendProcess->start("Clustering.exe", arguments);

    if (!backendProcess->waitForStarted(5000)) {
        ui->logTextEdit->append(QStringLiteral("错误：无法启动分析程序"));
        on_processFinished(-1, QProcess::CrashExit);
    }
}

void MainWindow::on_processReadyRead()
{
    QByteArray output = backendProcess->readAllStandardOutput();
    QString text = QString::fromLocal8Bit(output);

    if (!text.isEmpty()) {
        ui->logTextEdit->append(text);
        // 自动滚动到底部
        QTextCursor cursor = ui->logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->logTextEdit->setTextCursor(cursor);
    }
}

void MainWindow::on_processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    //Q_UNUSED(exitStatus)

        // 更新UI状态
        ui->progressBar->setVisible(false);
    ui->runAnalysisButton->setEnabled(true);

    if (exitCode == 0) {
        ui->logTextEdit->append("----------------------------------------");
        ui->logTextEdit->append(QStringLiteral("分析完成！"));
        ui->viewResultsButton->setEnabled(true);

        QMessageBox::information(this, QStringLiteral("完成"),        
            "聚类分析已完成！\n\n"
            "结果文件已保存到:\n" + ui->outputDirEdit->text() + "\n\n"       
            "点击\"查看结果\"按钮打开输出目录。");
        //QMessageBox::information(this, "title", "completed");


    }
    else {
        ui->logTextEdit->append("----------------------------------------");
        ui->logTextEdit->append("分析失败！退出代码: " + QString::number(exitCode));
        
        QMessageBox::critical(this, "错误",
            "分析过程中出现错误！\n\n"
            "请检查：\n"
            "1. 输入文件格式是否正确\n"
            "2. 参数设置是否合理\n"
            "3. 查看日志了解详细错误信息");
            
        //QMessageBox::information(this, "title", "error");
    }
}

void MainWindow::on_viewResultsButton_clicked()
{
    QString outputDir = ui->outputDirEdit->text();
    QUrl url = QUrl::fromLocalFile(outputDir);
    QDesktopServices::openUrl(url);
}