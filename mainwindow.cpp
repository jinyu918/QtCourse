#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_aboutdialog.h"
#include "searchdialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSaveFile>
#include <QTextCodec>
#include <QClipboard>
#include <QColorDialog>
#include <QFontDialog>
#include <QLabel>
#include <QSignalBlocker>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusbar->showMessage(tr("就绪"));
    ui->editor->setFocus();
    connect(ui->editor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateTitle);
    updateTitle();
    setUpEditing();
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
    updateActions();
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

void MainWindow::setUpEditing()
{
    connect(ui->actionUndo, &QAction::triggered, ui->editor, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->editor, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, ui->editor, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, ui->editor, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, ui->editor, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered, ui->editor, &QPlainTextEdit::selectAll);
    connect(ui->editor, &QPlainTextEdit::undoAvailable, this, &MainWindow::updateActions);
    connect(ui->editor, &QPlainTextEdit::redoAvailable, this, &MainWindow::updateActions);
    connect(ui->editor, &QPlainTextEdit::copyAvailable, this, &MainWindow::updateActions);
    connect(ui->editor, &QPlainTextEdit::textChanged, this, &MainWindow::updateActions);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateActions);

    connect(ui->actionFont, &QAction::triggered, this, [this] {
        bool accepted = false;
        const QFont font = QFontDialog::getFont(&accepted, ui->editor->font(), this, tr("设置字体和字号"));
        if (accepted) ui->editor->setFont(font);
    });
    connect(ui->actionTextColor, &QAction::triggered, this, [this] {
        const QColor color = QColorDialog::getColor(ui->editor->palette().color(QPalette::Text),
                                                    this, tr("设置字体颜色"));
        if (!color.isValid()) return;
        QPalette palette = ui->editor->palette();
        palette.setColor(QPalette::Text, color);
        ui->editor->setPalette(palette);
    });
    connect(ui->actionBackgroundColor, &QAction::triggered, this, [this] {
        const QColor color = QColorDialog::getColor(ui->editor->palette().color(QPalette::Base),
                                                    this, tr("设置编辑器背景颜色"));
        if (!color.isValid()) return;
        QPalette palette = ui->editor->palette();
        palette.setColor(QPalette::Base, color);
        ui->editor->setPalette(palette);
    });
    connect(ui->actionWrap, &QAction::toggled, this, [this](bool checked) {
        ui->editor->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    });
    connect(ui->actionLineNumbers, &QAction::toggled, ui->editor, &CodeEditor::setLineNumbersVisible);
    connect(ui->actionToolbar, &QAction::toggled, this, [this](bool checked) {
        ui->fileToolBar->setVisible(checked);
        ui->formatToolBar->setVisible(checked);
    });
    const auto syncToolbarAction = [this] {
        const QSignalBlocker blocker(ui->actionToolbar);
        ui->actionToolbar->setChecked(!ui->fileToolBar->isHidden() || !ui->formatToolBar->isHidden());
    };
    connect(ui->fileToolBar, &QToolBar::visibilityChanged, this, syncToolbarAction);
    connect(ui->formatToolBar, &QToolBar::visibilityChanged, this, syncToolbarAction);
    connect(ui->actionStatusbar, &QAction::toggled, ui->statusbar, &QStatusBar::setVisible);

    statisticsLabel = new QLabel(this);
    statisticsLabel->setObjectName(QStringLiteral("statisticsLabel"));
    positionLabel = new QLabel(this);
    positionLabel->setObjectName(QStringLiteral("positionLabel"));
    ui->statusbar->addPermanentWidget(statisticsLabel, 1);
    ui->statusbar->addPermanentWidget(positionLabel);
    ui->statusbar->addPermanentWidget(new QLabel(QStringLiteral("  UTF-8  "), this));
    connect(ui->editor, &QPlainTextEdit::textChanged, this, &MainWindow::updateStatus);
    connect(ui->editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatus);

    ui->fileToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui->formatToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui->actionNew->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->actionOpen->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    ui->actionSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->actionUndo->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    ui->actionRedo->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    ui->actionFont->setIconText(tr("字体 / 字号"));
    ui->actionTextColor->setIconText(tr("字体颜色"));
    ui->actionBackgroundColor->setIconText(tr("背景颜色"));
    updateActions();
    updateStatus();
}

void MainWindow::updateActions()
{
    ui->actionUndo->setEnabled(ui->editor->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(ui->editor->document()->isRedoAvailable());
    const bool selected = ui->editor->textCursor().hasSelection();
    ui->actionCut->setEnabled(selected);
    ui->actionCopy->setEnabled(selected);
    ui->actionPaste->setEnabled(ui->editor->canPaste());
    ui->actionSelectAll->setEnabled(!ui->editor->document()->isEmpty());
    ui->actionSave->setEnabled(currentPath.isEmpty() || ui->editor->document()->isModified());
}

void MainWindow::updateStatus()
{
    const QTextCursor cursor = ui->editor->textCursor();
    statisticsLabel->setText(tr("字符：%1    行数：%2").arg(
        ui->editor->document()->characterCount() - 1).arg(ui->editor->blockCount()));
    positionLabel->setText(tr("第 %1 行，第 %2 列    ").arg(
        cursor.blockNumber() + 1).arg(cursor.positionInBlock() + 1));
}

void MainWindow::showSearch(bool replaceMode)
{
    SearchDialog *&dialog = replaceMode ? replaceDialog : findDialog;
    if (!dialog) {
        dialog = new SearchDialog(ui->editor, replaceMode, this);
        dialog->setObjectName(replaceMode ? QStringLiteral("replaceDialog") : QStringLiteral("findDialog"));
    }
    const QString selected = ui->editor->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
        dialog->setSearchText(selected);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::on_actionFind_triggered()
{
    showSearch(false);
}

void MainWindow::on_actionReplace_triggered()
{
    showSearch(true);
}

void MainWindow::on_actionAbout_triggered()
{
    QDialog dialog(this);
    Ui::AboutDialog about;
    about.setupUi(&dialog);
    dialog.exec();
}
