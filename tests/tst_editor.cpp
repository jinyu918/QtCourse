#include "mainwindow.h"
#include "searchdialog.h"
#include "codeeditor.h"

#include <QAction>
#include <QPlainTextEdit>
#include <QTest>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QClipboard>
#include <QLabel>
#include <QMimeData>
#include <QToolBar>
#include <QStatusBar>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>
#include <QPointer>
#include <QScrollBar>
#include <QFontDialog>
#include <QColorDialog>

class EditorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void designerInterface();
    void fileRoundTripAndSaveAs();
    void failedIoPreservesDocument();
    void closeCancelAndDiscard();
    void editingActionStates();
    void viewControlsAndStatus();
    void findWrapAndMissing();
    void replaceAllBoundaries();
    void replaceWholeWordAndCase();
    void lineNumbersAndScrolling();
    void newWindowIsIndependent();
    void closeSaveAndCancelSaveAs();
    void appearanceDialogs();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
};

void EditorTest::initTestCase()
{
    qApp->installEventFilter(this);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
}

bool EditorTest::eventFilter(QObject *object, QEvent *event)
{
    // Qt 5.12 Windows message boxes dereference a missing native interface offscreen.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")
            && qobject_cast<QMessageBox *>(object) && event->type() == QEvent::Show)
        return true;
    return QObject::eventFilter(object, event);
}

void EditorTest::designerInterface()
{
    MainWindow window;
    QVERIFY(window.findChild<QPlainTextEdit *>("editor"));
    QVERIFY(window.findChild<QAction *>("actionSave"));
    QVERIFY(window.findChild<QAction *>("actionFind"));
    QVERIFY(window.findChild<QAction *>("actionWrap")->isChecked());
}

void EditorTest::fileRoundTripAndSaveAs()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    const QString text = QStringLiteral("中文文本\nsecond line\n末行");
    editor->insertPlainText(text);
    QVERIFY(window.isWindowModified());
    const QString first = directory.filePath(QStringLiteral("中文.txt"));
    QString error;
    QVERIFY2(window.saveFile(first, &error), qPrintable(error));
    QVERIFY(!window.isWindowModified());
    QVERIFY(!window.findChild<QAction *>("actionSave")->isEnabled());
    MainWindow reopened;
    QVERIFY2(reopened.loadFile(first, &error), qPrintable(error));
    QCOMPARE(reopened.findChild<QPlainTextEdit *>("editor")->toPlainText(), text);
    const QString second = directory.filePath("second.txt");
    QVERIFY(window.saveFile(second, &error));
    QCOMPARE(window.filePath(), second);
    editor->insertPlainText(QStringLiteral("修改"));
    window.findChild<QAction *>("actionSave")->trigger();
    QFile original(first);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(original.readAll()), text);
    QVERIFY(reopened.loadFile(second, &error));
    QCOMPARE(reopened.findChild<QPlainTextEdit *>("editor")->toPlainText(), editor->toPlainText());
}

void EditorTest::failedIoPreservesDocument()
{
    QTemporaryDir directory;
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    editor->insertPlainText(QStringLiteral("必须保留的内容"));
    QString error;
    QVERIFY(!window.loadFile(directory.filePath("missing.txt"), &error));
    QVERIFY(!error.isEmpty());
    QFile binary(directory.filePath("binary.txt"));
    QVERIFY(binary.open(QIODevice::WriteOnly));
    binary.write(QByteArray::fromHex("fffe0000"));
    binary.close();
    QVERIFY(!window.loadFile(binary.fileName(), &error));
    QVERIFY(!window.saveFile(directory.filePath("missing/file.txt"), &error));
    QCOMPARE(editor->toPlainText(), QStringLiteral("必须保留的内容"));
    QVERIFY(window.filePath().isEmpty());
    QVERIFY(window.isWindowModified());
}

void EditorTest::closeCancelAndDiscard()
{
    MainWindow window;
    window.show();
    window.findChild<QPlainTextEdit *>("editor")->insertPlainText("unsaved");
    QTimer::singleShot(50, [] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        box->button(QMessageBox::Cancel)->click();
    });
    QVERIFY(!window.close());
    QVERIFY(window.isVisible());
    QTimer::singleShot(50, [] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        box->button(QMessageBox::Discard)->click();
    });
    QVERIFY(window.close());
}

void EditorTest::editingActionStates()
{
    QApplication::clipboard()->clear();
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    const auto action = [&window](const char *name) { return window.findChild<QAction *>(name); };
    QVERIFY(!action("actionUndo")->isEnabled());
    QVERIFY(!action("actionRedo")->isEnabled());
    QVERIFY(!action("actionCut")->isEnabled());
    QVERIFY(!action("actionCopy")->isEnabled());
    QVERIFY(!action("actionPaste")->isEnabled());
    editor->insertPlainText("alpha beta");
    QVERIFY(action("actionUndo")->isEnabled());
    action("actionSelectAll")->trigger();
    QVERIFY(action("actionCopy")->isEnabled());
    action("actionCut")->trigger();
    QCOMPARE(editor->toPlainText(), QString());
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("alpha beta"));
    QVERIFY(action("actionPaste")->isEnabled());
    action("actionPaste")->trigger();
    QCOMPARE(editor->toPlainText(), QStringLiteral("alpha beta"));
    action("actionUndo")->trigger();
    QVERIFY(editor->toPlainText().isEmpty());
    QVERIFY(action("actionRedo")->isEnabled());
    action("actionRedo")->trigger();
    QCOMPARE(editor->toPlainText(), QStringLiteral("alpha beta"));
    QApplication::clipboard()->setMimeData(new QMimeData);
    QVERIFY(!action("actionPaste")->isEnabled());
}

void EditorTest::viewControlsAndStatus()
{
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    window.findChild<QAction *>("actionWrap")->setChecked(false);
    QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::NoWrap);
    window.findChild<QAction *>("actionWrap")->setChecked(true);
    QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
    window.findChild<QAction *>("actionToolbar")->setChecked(false);
    QVERIFY(window.findChild<QToolBar *>("fileToolBar")->isHidden());
    QVERIFY(window.findChild<QToolBar *>("formatToolBar")->isHidden());
    window.findChild<QAction *>("actionToolbar")->setChecked(true);
    QVERIFY(!window.findChild<QToolBar *>("fileToolBar")->isHidden());
    window.findChild<QAction *>("actionStatusbar")->setChecked(false);
    QVERIFY(window.statusBar()->isHidden());
    editor->insertPlainText("one\ntwo");
    QVERIFY(window.findChild<QLabel *>("statisticsLabel")->text().contains("7"));
    QVERIFY(window.findChild<QLabel *>("positionLabel")->text().contains("2"));
}

void EditorTest::findWrapAndMissing()
{
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    editor->setPlainText("cat dog cat");
    editor->moveCursor(QTextCursor::Start);
    window.findChild<QAction *>("actionFind")->trigger();
    auto *dialog = window.findChild<SearchDialog *>("findDialog");
    QVERIFY(dialog);
    auto *search = dialog->findChild<QLineEdit *>("searchEdit");
    auto *next = dialog->findChild<QPushButton *>("findButton");
    QVERIFY(!next->isEnabled());
    search->setText("cat");
    next->click();
    QCOMPARE(editor->textCursor().selectionStart(), 0);
    next->click();
    QCOMPARE(editor->textCursor().selectionStart(), 8);
    next->click();
    QCOMPARE(editor->textCursor().selectionStart(), 0);
    search->setText("absent");
    next->click();
    QVERIFY(dialog->findChild<QLabel *>("feedbackLabel")->text().contains(QStringLiteral("未找到")));
    QCOMPARE(editor->textCursor().selectionStart(), 0);
}

void EditorTest::replaceAllBoundaries()
{
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    editor->setPlainText("a a");
    window.findChild<QAction *>("actionReplace")->trigger();
    auto *dialog = window.findChild<SearchDialog *>("replaceDialog");
    auto *search = dialog->findChild<QLineEdit *>("searchEdit");
    auto *replacement = dialog->findChild<QLineEdit *>("replaceEdit");
    auto *all = dialog->findChild<QPushButton *>("replaceAllButton");
    search->setText("a");
    replacement->setText("aa");
    all->click();
    QCOMPARE(editor->toPlainText(), QStringLiteral("aa aa"));
    QVERIFY(dialog->findChild<QLabel *>("feedbackLabel")->text().contains("2"));
    editor->undo();
    QCOMPARE(editor->toPlainText(), QStringLiteral("a a"));
    replacement->clear();
    all->click();
    QCOMPARE(editor->toPlainText(), QStringLiteral(" "));
    editor->undo();
    QCOMPARE(editor->toPlainText(), QStringLiteral("a a"));
    replacement->setText("a");
    all->click();
    QCOMPARE(editor->toPlainText(), QStringLiteral("a a"));
    search->clear();
    QVERIFY(!all->isEnabled());
}

void EditorTest::replaceWholeWordAndCase()
{
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    editor->setPlainText("cat scatter Cat cat");
    window.findChild<QAction *>("actionReplace")->trigger();
    auto *dialog = window.findChild<SearchDialog *>("replaceDialog");
    dialog->findChild<QLineEdit *>("searchEdit")->setText("cat");
    dialog->findChild<QLineEdit *>("replaceEdit")->setText("dog");
    dialog->findChild<QCheckBox *>("caseCheck")->setChecked(true);
    dialog->findChild<QCheckBox *>("wholeWordCheck")->setChecked(true);
    editor->moveCursor(QTextCursor::Start);
    dialog->findChild<QPushButton *>("findButton")->click();
    dialog->findChild<QPushButton *>("replaceButton")->click();
    QCOMPARE(editor->toPlainText(), QStringLiteral("dog scatter Cat cat"));
    dialog->findChild<QPushButton *>("replaceAllButton")->click();
    QCOMPARE(editor->toPlainText(), QStringLiteral("dog scatter Cat dog"));
}

void EditorTest::lineNumbersAndScrolling()
{
    MainWindow window;
    window.resize(620, 300);
    window.show();
    auto *editor = window.findChild<CodeEditor *>("editor");
    QVERIFY(editor);
    const int narrow = editor->lineNumberAreaWidth();
    QStringList lines;
    for (int i = 1; i <= 150; ++i) lines << QString::number(i) + QString(120, QLatin1Char('x'));
    editor->setPlainText(lines.join(QLatin1Char('\n')));
    QVERIFY(editor->lineNumberAreaWidth() > narrow);
    QCOMPARE(editor->blockCount(), 150);
    auto *gutter = editor->findChild<QWidget *>("lineNumberArea");
    QVERIFY(gutter->isVisible());
    const int leftWithNumbers = editor->viewport()->geometry().left();
    window.findChild<QAction *>("actionLineNumbers")->setChecked(false);
    QVERIFY(!editor->lineNumbersVisible());
    QVERIFY(gutter->isHidden());
    QVERIFY(editor->viewport()->geometry().left() < leftWithNumbers);
    window.findChild<QAction *>("actionLineNumbers")->setChecked(true);
    QCOMPARE(editor->viewport()->geometry().left(), leftWithNumbers);
    editor->moveCursor(QTextCursor::End);
    editor->ensureCursorVisible();
    QVERIFY(editor->verticalScrollBar()->value() > 0);
    QVERIFY(!gutter->grab().isNull());
    window.findChild<QAction *>("actionWrap")->setChecked(false);
    QVERIFY(editor->horizontalScrollBar()->maximum() > 0);
    editor->document()->setModified(false);
}

void EditorTest::newWindowIsIndependent()
{
    QTemporaryDir directory;
    MainWindow first;
    auto *firstEditor = first.findChild<CodeEditor *>("editor");
    firstEditor->insertPlainText("first");
    first.findChild<QAction *>("actionNew")->trigger();
    QPointer<MainWindow> second;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        auto *candidate = qobject_cast<MainWindow *>(widget);
        if (candidate && candidate != &first) second = candidate;
    }
    QVERIFY(second);
    auto *secondEditor = second->findChild<CodeEditor *>("editor");
    QVERIFY(secondEditor->toPlainText().isEmpty());
    secondEditor->insertPlainText("second");
    QVERIFY(second->saveFile(directory.filePath("second.txt")));
    QVERIFY(first.filePath().isEmpty());
    QVERIFY(first.isWindowModified());
    QCOMPARE(firstEditor->toPlainText(), QStringLiteral("first"));
    first.findChild<QAction *>("actionWrap")->setChecked(false);
    QCOMPARE(secondEditor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
    QVERIFY(second->close());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(second.isNull());
}

void EditorTest::closeSaveAndCancelSaveAs()
{
    QTemporaryDir directory;
    MainWindow saved;
    auto *editor = saved.findChild<CodeEditor *>("editor");
    QVERIFY(saved.saveFile(directory.filePath("saved.txt")));
    editor->insertPlainText("saved on close");
    saved.show();
    QTimer::singleShot(50, [] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        box->button(QMessageBox::Save)->click();
    });
    QVERIFY(saved.close());
    QFile file(saved.filePath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("saved on close"));

    MainWindow unnamed;
    unnamed.findChild<CodeEditor *>("editor")->insertPlainText("keep me");
    unnamed.show();
    QTimer::singleShot(50, [] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        QTimer::singleShot(100, [] {
            auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->reject();
        });
        box->button(QMessageBox::Save)->click();
    });
    QVERIFY(!unnamed.close());
    QVERIFY(unnamed.isVisible());
    QVERIFY(unnamed.filePath().isEmpty());
    QCOMPARE(unnamed.findChild<CodeEditor *>("editor")->toPlainText(), QStringLiteral("keep me"));
    unnamed.findChild<CodeEditor *>("editor")->document()->setModified(false);
}

void EditorTest::appearanceDialogs()
{
    MainWindow window;
    auto *editor = window.findChild<CodeEditor *>("editor");
    QTimer::singleShot(50, [] {
        auto *dialog = qobject_cast<QFontDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        QFont font = dialog->currentFont();
        font.setPointSize(18);
        dialog->setCurrentFont(font);
        dialog->accept();
    });
    window.findChild<QAction *>("actionFont")->trigger();
    QCOMPARE(editor->font().pointSize(), 18);
    QTimer::singleShot(50, [] {
        auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->setCurrentColor(QColor("#123456"));
        dialog->accept();
    });
    window.findChild<QAction *>("actionTextColor")->trigger();
    QCOMPARE(editor->palette().color(QPalette::Text), QColor("#123456"));
    QTimer::singleShot(50, [] {
        auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->setCurrentColor(QColor("#fff4d6"));
        dialog->accept();
    });
    window.findChild<QAction *>("actionBackgroundColor")->trigger();
    QCOMPARE(editor->palette().color(QPalette::Base), QColor("#fff4d6"));
    QVERIFY(!editor->document()->isModified());
}

QTEST_MAIN(EditorTest)
#include "tst_editor.moc"
