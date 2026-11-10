#include "mainwindow.h"

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

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
};

void EditorTest::initTestCase()
{
    qApp->installEventFilter(this);
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

QTEST_MAIN(EditorTest)
#include "tst_editor.moc"
