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

    // ���ô��ڱ���
    setWindowTitle(QStringLiteral("����������� - �������ݷ���"));

    // ��ʼ����˽���
    backendProcess = new QProcess(this);
    connect(backendProcess, &QProcess::readyRead, this, &MainWindow::on_processReadyRead);
    //connect(backendProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),        this, &MainWindow::on_processFinished);

    connect(backendProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::on_processFinished);


    // ����Ĭ�ϲ���ֵ
    ui->smallThresholdEdit->setText("0.5");
    ui->mediumThresholdEdit->setText("0.7");
    ui->largeThresholdEdit->setText("0.9");
    ui->minNeighborsEdit->setText("3");

    // ��ʼ״̬
    ui->runAnalysisButton->setEnabled(false);
    ui->viewResultsButton->setEnabled(false);
    ui->progressBar->setVisible(false);

    // �����ı��ı��ź������°�ť״̬
    connect(ui->inputFileEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->outputDirEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->smallThresholdEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->mediumThresholdEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->largeThresholdEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);
    connect(ui->minNeighborsEdit, &QLineEdit::textChanged, this, &MainWindow::updateRunButtonState);

    // ��ʾ��ӭ��Ϣ
    ui->logTextEdit->append(QStringLiteral("��ӭʹ�þ���������ߣ�"));
    ui->logTextEdit->append(QStringLiteral("��ѡ�������ļ������Ŀ¼��Ȼ�����÷���������"));
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
        "ѡ�����������ļ�", "", "�ı��ļ� (*.txt);;�����ļ� (*.*)");

    if (!fileName.isEmpty()) {
        ui->inputFileEdit->setText(fileName);
    }
}

void MainWindow::on_selectOutputButton_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this,
        "ѡ�����Ŀ¼", "", QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        // ȷ��Ŀ¼·����б�ܽ�β
        if (!dirName.endsWith('/') && !dirName.endsWith('\\')) {
            dirName += '/';
        }
        ui->outputDirEdit->setText(dirName);
    }
}

bool MainWindow::validateInputs()
{
    // ����ļ�·��
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

    // �����ֵ����
    bool ok1, ok2, ok3, ok4;
    double smallThresh = ui->smallThresholdEdit->text().toDouble(&ok1);
    double mediumThresh = ui->mediumThresholdEdit->text().toDouble(&ok2);
    double largeThresh = ui->largeThresholdEdit->text().toDouble(&ok3);
    int minNeighbors = ui->minNeighborsEdit->text().toInt(&ok4);

    if (!ok1 || !ok2 || !ok3 || !ok4) {
        return false;
    }

    // �����ֵ��Χ
    if (smallThresh < 0 || smallThresh > 1 ||
        mediumThresh < 0 || mediumThresh > 1 ||
        largeThresh < 0 || largeThresh > 1) {
        return false;
    }

    // �����ֵ��С��ϵ
    if (smallThresh >= mediumThresh || mediumThresh >= largeThresh) {
        return false;
    }

    // �����С�ھ���
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
        //QMessageBox::warning(this, "����", "�������ڽ����У���ȴ����");
        return;
    }

    if (!validateInputs()) {
        //QMessageBox::warning(this, "����", "������������Ƿ���ȷ");
        return;
    }

    // ����UI״̬
    ui->runAnalysisButton->setEnabled(false);
    ui->viewResultsButton->setEnabled(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0, 0); // ���޽�����

    // �����־����ʾ��ʼ��Ϣ
    ui->logTextEdit->clear();
    ui->logTextEdit->append("��ʼ�������...");
    ui->logTextEdit->append("�����ļ�: " + ui->inputFileEdit->text());
    ui->logTextEdit->append("���Ŀ¼: " + ui->outputDirEdit->text());
    ui->logTextEdit->append("��������:");
    ui->logTextEdit->append("  - ������ֵ: " + ui->smallThresholdEdit->text());
    ui->logTextEdit->append("  - �е���ֵ: " + ui->mediumThresholdEdit->text());
    ui->logTextEdit->append("  - �ϸ���ֵ: " + ui->largeThresholdEdit->text());
    ui->logTextEdit->append("  - ��С�ھ���: " + ui->minNeighborsEdit->text());
    ui->logTextEdit->append("----------------------------------------");

    // ׼�������в���
    QStringList arguments;
    arguments << ui->inputFileEdit->text()
        << ui->outputDirEdit->text()
        << ui->smallThresholdEdit->text()
        << ui->mediumThresholdEdit->text()
        << ui->largeThresholdEdit->text()
        << ui->minNeighborsEdit->text();

    // ������˽���
    // ע�⣺���������Ŀ�ִ���ļ���Ϊ Clustering.exe
    backendProcess->start("Clustering.exe", arguments);

    if (!backendProcess->waitForStarted(5000)) {
        ui->logTextEdit->append(QStringLiteral("�����޷�������������"));
        on_processFinished(-1, QProcess::CrashExit);
    }
}

void MainWindow::on_processReadyRead()
{
    QByteArray output = backendProcess->readAllStandardOutput();
    QString text = QString::fromLocal8Bit(output);

    if (!text.isEmpty()) {
        ui->logTextEdit->append(text);
        // �Զ��������ײ�
        QTextCursor cursor = ui->logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->logTextEdit->setTextCursor(cursor);
    }
}

void MainWindow::on_processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    //Q_UNUSED(exitStatus)

        // ����UI״̬
        ui->progressBar->setVisible(false);
    ui->runAnalysisButton->setEnabled(true);

    if (exitCode == 0) {
        ui->logTextEdit->append("----------------------------------------");
        ui->logTextEdit->append(QStringLiteral("������ɣ�"));
        ui->viewResultsButton->setEnabled(true);

        //QMessageBox::information(this, QStringLiteral("���"),            "�����������ɣ�\n\n"+         "����ļ��ѱ��浽:\n" + ui->outputDirEdit->text() + "\n\n"+            "���\"�鿴���\"��ť�����Ŀ¼��");
        QMessageBox::information(this, "title", "completed");


    }
    else {
        ui->logTextEdit->append("----------------------------------------");
        ui->logTextEdit->append("����ʧ�ܣ��˳�����: " + QString::number(exitCode));
        /*
        QMessageBox::critical(this, "����",
            "���������г��ִ���\n\n"
            "���飺\n"
            "1. �����ļ���ʽ�Ƿ���ȷ\n"
            "2. ���������Ƿ����\n"
            "3. �鿴��־�˽���ϸ������Ϣ");
            */
        QMessageBox::information(this, "title", "error");
    }
}

void MainWindow::on_viewResultsButton_clicked()
{
    QString outputDir = ui->outputDirEdit->text();
    QUrl url = QUrl::fromLocalFile(outputDir);
    QDesktopServices::openUrl(url);
}