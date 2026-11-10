#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSaveFile>
#include <QTextCodec>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusbar->showMessage(tr("就绪"));
    ui->editor->setFocus();
    connect(ui->editor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateTitle);
    updateTitle();
}

MainWindow::~MainWindow() = default;

QString MainWindow::filePath() const
{
    return currentPath;
}

bool MainWindow::loadFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        if (error) *error = file.errorString();
        return false;
    }
    QTextCodec::ConverterState state;
    const QString text = QTextCodec::codecForName("UTF-8")->toUnicode(
                bytes.constData(), bytes.size(), &state);
    if (state.invalidChars || state.remainingChars || text.contains(QChar::Null)) {
        if (error) *error = tr("该文件不是有效的 UTF-8 文本，请先转换编码后打开。");
        return false;
    }
    ui->editor->setPlainText(text);
    currentPath = QFileInfo(path).absoluteFilePath();
    ui->editor->document()->setModified(false);
    updateTitle();
    ui->statusbar->showMessage(tr("已打开 %1").arg(currentPath), 3000);
    return true;
}

bool MainWindow::saveFile(const QString &path, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QByteArray bytes = ui->editor->toPlainText().toUtf8();
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    currentPath = QFileInfo(path).absoluteFilePath();
    ui->editor->document()->setModified(false);
    updateTitle();
    ui->statusbar->showMessage(tr("已保存 %1").arg(currentPath), 3000);
    return true;
}

void MainWindow::updateTitle()
{
    const QString name = currentPath.isEmpty() ? tr("未命名") : QFileInfo(currentPath).fileName();
    setWindowTitle(tr("%1[*] - 简单文本编辑器").arg(name));
    setWindowModified(ui->editor->document()->isModified());
    setWindowFilePath(currentPath);
}

void MainWindow::on_actionNew_triggered()
{
    auto *window = new MainWindow;
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
}

void MainWindow::on_actionOpen_triggered()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("打开 UTF-8 文本"),
        currentPath, tr("文本文件 (*.txt *.md *.cpp *.h *.csv *.log);;所有文件 (*)"));
    if (path.isEmpty()) return;

    const bool reuse = currentPath.isEmpty() && !ui->editor->document()->isModified()
            && ui->editor->toPlainText().isEmpty();
    MainWindow *target = reuse ? this : new MainWindow;
    QString error;
    if (!target->loadFile(path, &error)) {
        if (!reuse) delete target;
        QMessageBox::warning(this, tr("打开失败"), tr("无法打开 %1\n%2").arg(path, error));
        return;
    }
    if (!reuse) target->setAttribute(Qt::WA_DeleteOnClose);
    target->show();
}

bool MainWindow::on_actionSave_triggered()
{
    if (currentPath.isEmpty()) return on_actionSaveAs_triggered();
    QString error;
    if (saveFile(currentPath, &error)) return true;
    QMessageBox::warning(this, tr("保存失败"), error);
    return false;
}

bool MainWindow::on_actionSaveAs_triggered()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("另存为 UTF-8 文本"),
        currentPath.isEmpty() ? tr("未命名.txt") : currentPath,
        tr("文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return false;
    QString error;
    if (saveFile(path, &error)) return true;
    QMessageBox::warning(this, tr("保存失败"), error);
    return false;
}

bool MainWindow::maybeSave()
{
    if (!ui->editor->document()->isModified()) return true;
    QMessageBox prompt(QMessageBox::Warning, tr("未保存的修改"),
        tr("%1 已修改。关闭前是否保存？").arg(
            currentPath.isEmpty() ? tr("未命名") : QFileInfo(currentPath).fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
    prompt.setDefaultButton(QMessageBox::Cancel);
    const int answer = prompt.exec();
    if (answer == QMessageBox::Save) return on_actionSave_triggered();
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) event->accept();
    else event->ignore();
}

void MainWindow::on_actionClose_triggered()
{
    close();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::closeAllWindows();
}
